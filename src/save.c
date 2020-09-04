#include "global.h"
#include "agb_flash.h"
#include "gba/flash_internal.h"
#include "fieldmap.h"
#include "save.h"
#include "task.h"
#include "decompress.h"
#include "load_save.h"
#include "overworld.h"
#include "pokemon_storage_system.h"
#include "main.h"
#include "trainer_hill.h"
#include "link.h"
#include "constants/game_stat.h"

static u16 CalculateChecksum(void *, u16);
static bool8 DoReadFlashWholeSection(u8, struct SaveSection *);
static u8 GetSaveValidStatus(const struct SaveBlockChunk *);
static u8 ReadAllSaveSectorsCurrentSlot(u16 a1, const struct SaveBlockChunk *);
static u8 EraseChunk(u16 a1, const struct SaveBlockChunk *);
static u8 TryWriteSector(u8 sector, u8 *);
static u8 WriteSingleChunk(u16 a1, const struct SaveBlockChunk *);

// Divide save blocks into individual chunks to be written to flash sectors

// Each 4 KiB flash sector contains 3968 bytes of actual data followed by a 128 byte footer
#define SECTOR_DATA_SIZE   3968
#define SECTOR_FOOTER_SIZE 128

/*
 * Sector Layout:
 *
 * Sectors 0 - 13:      Save Slot 1
 * Sectors 14 - 27:     Save Slot 2
 * Sectors 28 - 29:     Hall of Fame
 * Sector 30:           Trainer Hill
 * Sector 31:           Recorded Battle
 *
 * There are two save slots for saving the player's game data. We alternate between
 * them each time the game is saved, so that if the current save slot is corrupt,
 * we can load the previous one. We also rotate the sectors in each save slot
 * so that the same data is not always being written to the same sector. This
 * might be done to reduce wear on the flash memory, but I'm not sure, since all
 * 14 sectors get written anyway.
 */

// (u8 *)structure was removed from the first statement of the macro in Emerald.
// This is because malloc is used to allocate addresses so storing the raw
// addresses should not be done in the offsets information.

// maybe sector -> {whichSector, sectorId}
#define SAVEBLOCK_OFFSET(struct, sector)                                \
{                                                                           \
    sector * SECTOR_DATA_SIZE,                                            \
    min(sizeof(struct) - sector * SECTOR_DATA_SIZE, SECTOR_DATA_SIZE)  \
}                                                                           \

// Alternatively SaveblockChunks? Shards?
static const struct SaveblockOffset sSaveblockOffsets[] =
    {
        // Saveblock 2
        [SLOT_SECTOR_0]  = SAVEBLOCK_OFFSET(gSaveblock2, 0),

        // Saveblock 1
        [SLOT_SECTOR_1]  = SAVEBLOCK_OFFSET(gSaveblock1, 0),
        [SLOT_SECTOR_2]  = SAVEBLOCK_OFFSET(gSaveblock1, 1),
        [SLOT_SECTOR_3]  = SAVEBLOCK_OFFSET(gSaveblock1, 2),
        [SLOT_SECTOR_4]  = SAVEBLOCK_OFFSET(gSaveblock1, 3),

        // Pokémon Storage
        [SLOT_SECTOR_5]  = SAVEBLOCK_OFFSET(gPokemonStorage, 0),
        [SLOT_SECTOR_6]  = SAVEBLOCK_OFFSET(gPokemonStorage, 1),
        [SLOT_SECTOR_7]  = SAVEBLOCK_OFFSET(gPokemonStorage, 2),
        [SLOT_SECTOR_8]  = SAVEBLOCK_OFFSET(gPokemonStorage, 3),
        [SLOT_SECTOR_9]  = SAVEBLOCK_OFFSET(gPokemonStorage, 4),
        [SLOT_SECTOR_10] = SAVEBLOCK_OFFSET(gPokemonStorage, 5),
        [SLOT_SECTOR_11] = SAVEBLOCK_OFFSET(gPokemonStorage, 6),
        [SLOT_SECTOR_12] = SAVEBLOCK_OFFSET(gPokemonStorage, 7),
        [SLOT_SECTOR_13] = SAVEBLOCK_OFFSET(gPokemonStorage, 8),
};

// iwram common
u16 gFirstSaveSector;
u32 gPrevSaveCounter;
u16 gLastKnownGoodSector;
u32 gDamagedSaveSectors;
u32 gSaveCounter;
struct SaveSection *gFastSaveSection;
// Seems to be used a sector index
u16 gCurSaveChunk;
u16 gSaveUnusedVar;
u16 gSaveFileStatus;
void (*gGameContinueCallback)(void);
struct SaveBlockChunk gRamSaveBuffers[SAVE_SLOT_SECTORS];
u16 gSaveUnusedVar2;
u16 gSaveAttemptStatus;

EWRAM_DATA struct SaveSection gSaveDataBuffer = {0};
EWRAM_DATA static u8 sUnusedVar = 0;

void Save_EraseAllData(void)
{
    u16 i;

    // Splits save file in halves and clears them simultaneously.
    // Note: it does not split by save slots. 
    for (i = 0; i < NUM_SECTORS / 2; i++)
    {
        EraseFlashSector(i);
        EraseFlashSector(i + NUM_SECTORS / 2);
    }
}

void Save_ResetSaveCounters(void)
{
    gSaveCounter = 0;
    gFirstSaveSector = 0;
    gDamagedSaveSectors = 0;
}

static bool32 SetSectorDamagedStatus(u8 op, u8 sectorNum)
{
    bool32 retVal = FALSE;

    switch (op)
    {
    case SECTOR_DAMAGED:
        gDamagedSaveSectors |= (1 << sectorNum);
        break;
    case SECTOR_OK:
        gDamagedSaveSectors &= ~(1 << sectorNum);
        break;
    case SECTOR_CHECK: // unused
        if (gDamagedSaveSectors & (1 << sectorNum))
            retVal = TRUE;
        break;
    }

    return retVal;
}

static u8 SaveWriteToFlash(u16 sector, const struct SaveBlockChunk *chunk)
{
    u32 status;
    u16 i;

    gFastSaveSection = &gSaveDataBuffer;

    if (sector != 0xFFFF) // for link
    {
        status = WriteSingleChunk(sector, chunk);
    }
    else
    {
        gLastKnownGoodSector = gFirstSaveSector; // backup the current written sector before attempting to write.
        gPrevSaveCounter = gSaveCounter;
        gFirstSaveSector++;
        gFirstSaveSector = gFirstSaveSector % SAVE_SLOT_SECTORS; // array count save sector locations
        gSaveCounter++;
        status = SAVE_STATUS_OK;

        for (i = 0; i < SAVE_SLOT_SECTORS; i++)
            WriteSingleChunk(i, chunk);

        if (gDamagedSaveSectors != 0) // skip the damaged sector.
        {
            status = SAVE_STATUS_ERROR;
            gFirstSaveSector = gLastKnownGoodSector;
            gSaveCounter = gPrevSaveCounter;
        }
    }

    return status;
}

static u8 WriteSingleChunk(u16 chunkId, const struct SaveBlockChunk *chunk)
{
    u16 i;
    u16 sectorNum;
    u8 *chunkData;
    u16 chunkSize;

    sectorNum = chunkId + gFirstSaveSector;
    sectorNum %= SAVE_SLOT_SECTORS;
    sectorNum += SAVE_SLOT_SECTORS * (gSaveCounter % 2);

    chunkData = chunk[chunkId].data;
    chunkSize = chunk[chunkId].size;

    // clear save section.
    for (i = 0; i < sizeof(struct SaveSection); i++)
        ((char *)gFastSaveSection)[i] = 0;

    gFastSaveSection->id = chunkId;
    gFastSaveSection->security = FILE_SIGNATURE;
    gFastSaveSection->counter = gSaveCounter;

    for (i = 0; i < chunkSize; i++)
        gFastSaveSection->data[i] = chunkData[i];

    gFastSaveSection->checksum = CalculateChecksum(chunkData, chunkSize);
    return TryWriteSector(sectorNum, gFastSaveSection->data);
}

static u8 HandleWriteSectorNBytes(u8 sector, u8 *data, u16 size)
{
    u16 i;
    struct SaveSection *section = &gSaveDataBuffer;

    for (i = 0; i < sizeof(struct SaveSection); i++)
        ((char *)section)[i] = 0;

    section->security = FILE_SIGNATURE;

    for (i = 0; i < size; i++)
        section->data[i] = data[i];

    section->id = CalculateChecksum(data, size); // though this appears to be incorrect, it might be some sector checksum instead of a whole save checksum and only appears to be relevent to HOF data, if used.
    return TryWriteSector(sector, section->data);
}

static u8 TryWriteSector(u8 sector, u8 *data)
{
    if (ProgramFlashSectorAndVerify(sector, data) != 0) // is damaged?
    {
        SetSectorDamagedStatus(SECTOR_DAMAGED, sector); // set damaged sector bits.
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetSectorDamagedStatus(SECTOR_OK, sector); // unset damaged sector bits. it's safe now.
        return SAVE_STATUS_OK;
    }
}

// Called by FullSaveGame_Link()
// - This one updates the last written sector
static u32 RestoreSaveBackupVarsAndIncrement(const struct SaveBlockChunk *chunk) // chunk is unused
{
    gFastSaveSection = &gSaveDataBuffer;
    gLastKnownGoodSector = gFirstSaveSector;
    gPrevSaveCounter = gSaveCounter;
    gFirstSaveSector++;
    gFirstSaveSector %= SAVE_SLOT_SECTORS;
    gSaveCounter++;
    gCurSaveChunk = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

// Called by FullSaveGame()
static u32 RestoreSaveBackupVars(const struct SaveBlockChunk *chunk) // only ever called once, and gSaveBlock2 is passed to this function. chunk is unused
{
    gFastSaveSection = &gSaveDataBuffer;
    gLastKnownGoodSector = gFirstSaveSector;
    gPrevSaveCounter = gSaveCounter;
    gCurSaveChunk = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

static u8 WriteSingleChunkAndIncrement(u16 sectorLimit, const struct SaveBlockChunk *chunk)
{
    u8 status;

    if (gCurSaveChunk < sectorLimit - 1)
    {
        status = SAVE_STATUS_OK;
        WriteSingleChunk(gCurSaveChunk, chunk);
        gCurSaveChunk++;
        if (gDamagedSaveSectors)
        {
            status = SAVE_STATUS_ERROR;
            gFirstSaveSector = gLastKnownGoodSector;
            gSaveCounter = gPrevSaveCounter;
        }
    }
    else
    {
        status = SAVE_STATUS_ERROR;
    }

    return status;
}

// Probably TryClearSector
static u8 ErasePreviousChunk(u16 chunkId, const struct SaveBlockChunk *chunk)
{
    u8 status = SAVE_STATUS_OK;

    EraseChunk(chunkId - 1, chunk);

    if (gDamagedSaveSectors)
    {
        status = SAVE_STATUS_ERROR;
        gFirstSaveSector = gLastKnownGoodSector;
        gSaveCounter = gPrevSaveCounter;
    }
    return status;
}

// Seems to be WriteSector
// Similar to SaveWriteToFlash() and WriteSingleChunk()
static u8 EraseChunk(u16 chunkId, const struct SaveBlockChunk *chunk)
{
    u16 i;
    u16 sector;
    u8 *data;
    u16 size;
    u8 status;

    sector = chunkId + gFirstSaveSector;
    sector %= SAVE_SLOT_SECTORS;
    sector += SAVE_SLOT_SECTORS * (gSaveCounter % 2);

    data = chunk[chunkId].data;
    size = chunk[chunkId].size;

    // clear temp save section.
    for (i = 0; i < sizeof(struct SaveSection); i++)
        ((char *)gFastSaveSection)[i] = 0;

    gFastSaveSection->id = chunkId;
    gFastSaveSection->security = FILE_SIGNATURE;
    gFastSaveSection->counter = gSaveCounter;

    // set temp section's data.
    for (i = 0; i < size; i++)
        gFastSaveSection->data[i] = data[i];

    // calculate checksum.
    gFastSaveSection->checksum = CalculateChecksum(data, size);

    EraseFlashSector(sector);

    status = SAVE_STATUS_OK;

    for (i = 0; i < sizeof(struct UnkSaveSection); i++)
    {
        if (ProgramFlashByte(sector, i, ((u8 *)gFastSaveSection)[i]))
        {
            status = SAVE_STATUS_ERROR;
            break;
        }
    }

    if (status == SAVE_STATUS_ERROR)
    {
        SetSectorDamagedStatus(SECTOR_DAMAGED, sector);
        return SAVE_STATUS_ERROR;
    }
    
    status = SAVE_STATUS_OK;
    for (i = 0; i < 7; i++)
    {
        if (ProgramFlashByte(sector, 0xFF9 + i, ((u8 *)gFastSaveSection)[0xFF9 + i]))
        {
            status = SAVE_STATUS_ERROR;
            break;
        }
    }

    if (status == SAVE_STATUS_ERROR)
    {
        SetSectorDamagedStatus(SECTOR_DAMAGED, sector);
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetSectorDamagedStatus(SECTOR_OK, sector);
        return SAVE_STATUS_OK;
    }
}

// Prev if sector should be chunkId + gFirstSaveSector - 1, like in berry_fix flash.c
static u8 WriteSomeFlashByte0x25ToCurSector(u16 chunkId, const struct SaveBlockChunk *chunk)
{
    u16 sector;

    sector = chunkId + gFirstSaveSector; // no sub 1?
    sector %= SAVE_SLOT_SECTORS;
    sector += SAVE_SLOT_SECTORS * (gSaveCounter % 2);

    if (ProgramFlashByte(sector, sizeof(struct UnkSaveSection), 0x25))
    {
        // sector is damaged, so SECTOR_DAMAGED the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetSectorDamagedStatus(SECTOR_DAMAGED, sector);
        gFirstSaveSector = gLastKnownGoodSector;
        gSaveCounter = gPrevSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetSectorDamagedStatus(SECTOR_OK, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 WriteSomeFlashByteToPrevSector(u16 sectorId, const struct SaveBlockChunk *chunk)
{
    u16 sector;

    sector = sectorId + gFirstSaveSector - 1;
    sector %= SAVE_SLOT_SECTORS;
    sector += SAVE_SLOT_SECTORS * (gSaveCounter % 2);

    if (ProgramFlashByte(sector, sizeof(struct UnkSaveSection), ((u8 *)gFastSaveSection)[sizeof(struct UnkSaveSection)]))
    {
        // sector is damaged, so SECTOR_DAMAGED the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetSectorDamagedStatus(SECTOR_DAMAGED, sector);
        gFirstSaveSector = gLastKnownGoodSector;
        gSaveCounter = gPrevSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetSectorDamagedStatus(SECTOR_OK, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 WriteSomeFlashByte0x25ToPrevSector(u16 sectorId, const struct SaveBlockChunk *chunk)
{
    u16 sector;

    sector = sectorId + gFirstSaveSector - 1;
    sector %= SAVE_SLOT_SECTORS;
    sector += SAVE_SLOT_SECTORS * (gSaveCounter % 2);

    if (ProgramFlashByte(sector, sizeof(struct UnkSaveSection), 0x25))
    {
        // sector is damaged, so SECTOR_DAMAGED the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetSectorDamagedStatus(SECTOR_DAMAGED, sector);
        gFirstSaveSector = gLastKnownGoodSector;
        gSaveCounter = gPrevSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetSectorDamagedStatus(SECTOR_OK, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 TryReadAllSaveSectorsCurrentSlot(u16 a1, const struct SaveBlockChunk *chunk)
{
    u8 status;
    gFastSaveSection = &gSaveDataBuffer;
    if (a1 != 0xFFFF)
    {
        status = SAVE_STATUS_ERROR;
    }
    else
    {
        status = GetSaveValidStatus(chunk);
        ReadAllSaveSectorsCurrentSlot(0xFFFF, chunk);
    }

    return status;
}

static u8 ReadAllSaveSectorsCurrentSlot(u16 a1, const struct SaveBlockChunk *chunk)
{
    u16 i;
    u16 checksum;
    u16 sector = SAVE_SLOT_SECTORS * (gSaveCounter % 2);
    u16 id;

    for (i = 0; i < SAVE_SLOT_SECTORS; i++)
    {
        DoReadFlashWholeSection(i + sector, gFastSaveSection);
        id = gFastSaveSection->id;
        if (id == 0)
            gFirstSaveSector = i;
        checksum = CalculateChecksum(gFastSaveSection->data, chunk[id].size);
        if (gFastSaveSection->security == FILE_SIGNATURE
         && gFastSaveSection->checksum == checksum)
        {
            u16 j;
            for (j = 0; j < chunk[id].size; j++)
                ((u8 *)chunk[id].data)[j] = gFastSaveSection->data[j];
        }
    }

    return SAVE_STATUS_OK;
}

static u8 GetSaveValidStatus(const struct SaveBlockChunk *chunk)
{
    u16 i;
    u16 checksum;
    u32 saveSlot1Counter = 0;
    u32 saveSlot2Counter = 0;
    u32 validSectors = 0;
    bool8 signatureValid = FALSE;
    u8 saveSlot1Status;
    u8 saveSlot2Status;

    // check save slot 1.
    for (i = 0; i < SAVE_SLOT_SECTORS; i++)
    {
        DoReadFlashWholeSection(i, gFastSaveSection);
        if (gFastSaveSection->security == FILE_SIGNATURE)
        {
            signatureValid = TRUE;
            checksum = CalculateChecksum(gFastSaveSection->data, chunk[gFastSaveSection->id].size);
            if (gFastSaveSection->checksum == checksum)
            {
                saveSlot1Counter = gFastSaveSection->counter;
                validSectors |= 1 << gFastSaveSection->id;
            }
        }
    }

    if (signatureValid)
    {
        if (validSectors == ALL_SECTORS)
            saveSlot1Status = SAVE_STATUS_OK;
        else
            saveSlot1Status = SAVE_STATUS_ERROR;
    }
    else
    {
        saveSlot1Status = SAVE_STATUS_EMPTY;
    }

    validSectors = 0;
    signatureValid = FALSE;

    // check save slot 2.
    for (i = 0; i < SAVE_SLOT_SECTORS; i++)
    {
        DoReadFlashWholeSection(i + SAVE_SLOT_SECTORS, gFastSaveSection);
        if (gFastSaveSection->security == FILE_SIGNATURE)
        {
            signatureValid = TRUE;
            checksum = CalculateChecksum(gFastSaveSection->data, chunk[gFastSaveSection->id].size);
            if (gFastSaveSection->checksum == checksum)
            {
                saveSlot2Counter = gFastSaveSection->counter;
                validSectors |= 1 << gFastSaveSection->id;
            }
        }
    }

    if (signatureValid)
    {
        if (validSectors == 0x3FFF)
            saveSlot2Status = SAVE_STATUS_OK;
        else
            saveSlot2Status = SAVE_STATUS_ERROR;
    }
    else
    {
        saveSlot2Status = SAVE_STATUS_EMPTY;
    }

    if (saveSlot1Status == SAVE_STATUS_OK && saveSlot2Status == SAVE_STATUS_OK)
    {
        if ((saveSlot1Counter == -1 && saveSlot2Counter == 0) || (saveSlot1Counter == 0 && saveSlot2Counter == -1))
        {
            if ((unsigned)(saveSlot1Counter + 1) < (unsigned)(saveSlot2Counter + 1))
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        else
        {
            if (saveSlot1Counter < saveSlot2Counter)
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        return SAVE_STATUS_OK;
    }

    if (saveSlot1Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot1Counter;
        if (saveSlot2Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR;
        return SAVE_STATUS_OK;
    }

    if (saveSlot2Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot2Counter;
        if (saveSlot1Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR;
        return SAVE_STATUS_OK;
    }

    if (saveSlot1Status == SAVE_STATUS_EMPTY && saveSlot2Status == SAVE_STATUS_EMPTY)
    {
        gSaveCounter = 0;
        gFirstSaveSector = 0;
        return SAVE_STATUS_EMPTY;
    }

    gSaveCounter = 0;
    gFirstSaveSector = 0;
    return SAVE_STATUS_CORRUPT;
}

// Used for Hall of Fame
static u8 ReadSomeUnknownSectorAndVerify(u8 sectorId, u8 *data, u16 size)
{
    u16 i;
    struct SaveSection *section = &gSaveDataBuffer;
    DoReadFlashWholeSection(sectorId, section);
    if (section->security == FILE_SIGNATURE)
    {
        u16 checksum = CalculateChecksum(section->data, size);
        if (section->id == checksum)
        {
            for (i = 0; i < size; i++)
                data[i] = section->data[i];
            return SAVE_STATUS_OK;
        }
        else
        {
            return SAVE_STATUS_CORRUPT;
        }
    }
    else
    {
        return SAVE_STATUS_EMPTY;
    }
}

// Return value always ignored
static bool8 DoReadFlashWholeSection(u8 sector, struct SaveSection *section)
{
    ReadFlash(sector, 0, section->data, sizeof(struct SaveSection));
    return TRUE;
}

static u16 CalculateChecksum(void *data, u16 size)
{
    u16 i;
    u32 checksum = 0;

    for (i = 0; i < (size / 4); i++)
    {
        checksum += *((u32 *)data);
        data += sizeof(u32);
    }

    return ((checksum >> 16) + checksum);
}

static void UpdateSaveAddresses(void)
{
    s32 i = SLOT_SAVEBLOCK_2;

    gRamSaveBuffers[i].data = (void *)(gSaveBlock2Ptr) + sSaveblockOffsets[i].offset;
    gRamSaveBuffers[i].size = sSaveblockOffsets[i].size;

    for (i = SLOT_SAVEBLOCK_1; i < SLOT_POKESTORE; i++)
    {
        gRamSaveBuffers[i].data = (void *)(gSaveBlock1Ptr) + sSaveblockOffsets[i].offset;
        gRamSaveBuffers[i].size = sSaveblockOffsets[i].size;
    }

    for (i = SLOT_POKESTORE; i < SLOT_END; i++)
    {
        gRamSaveBuffers[i].data = (void *)(gPokemonStoragePtr) + sSaveblockOffsets[i].offset;
        gRamSaveBuffers[i].size = sSaveblockOffsets[i].size;

        i++;i--; // needed to match
    }
}

u8 HandleSavingData(u8 saveType)
{
    u8 i;
    u32 *backupVar = gTrainerHillVBlankCounter;
    u8 *tempAddr;

    gTrainerHillVBlankCounter = NULL;
    UpdateSaveAddresses();
    switch (saveType)
    {
        // Unused: clears Hall of Fame, Trainer Hill, and recorded video
        case SAVE_CLEAR_RECORDS:
            for (i = SECTOR_HOF_1; i < NUM_SECTORS; i++)
                EraseFlashSector(i);
        case SAVE_HALL_OF_FAME:
            if (GetGameStat(GAME_STAT_ENTERED_HOF) < 999)
                IncrementGameStat(GAME_STAT_ENTERED_HOF);
            SavePartyAndObjects();
            SaveWriteToFlash(-1, gRamSaveBuffers);
            tempAddr = gDecompressionBuffer;
            HandleWriteSectorNBytes(SECTOR_HOF_1, tempAddr, SECTOR_DATA_SIZE);
            HandleWriteSectorNBytes(SECTOR_HOF_2, tempAddr + SECTOR_DATA_SIZE, SECTOR_DATA_SIZE);
            break;
        case SAVE_NORMAL: // also called to overwrite your own save.
        default:
            SavePartyAndObjects();
            SaveWriteToFlash(-1, gRamSaveBuffers);
            break;
        case SAVE_LINK:  // Link and Battle Frontier
        case SAVE_LINK2: // Unused
            SavePartyAndObjects();
            for(i = SLOT_SAVEBLOCK_2; i < SLOT_POKESTORE; i++)
                EraseChunk(i, gRamSaveBuffers);
            for (i = SLOT_SAVEBLOCK_2; i < SLOT_POKESTORE; i++)
                WriteSomeFlashByte0x25ToCurSector(i, gRamSaveBuffers);
            break;
        // Support for Ereader was removed in Emerald.
        /*
        case EREADER_SAVE: // used in mossdeep "game corner" before/after battling old man e-reader trainer
            SavePartyAndObjects();
            SaveWriteToFlash(0, gRamSaveBuffers);
            break;
        */
        case SAVE_OVERWRITE_DIFFERENT_FILE:
            for (i = SECTOR_HOF_1; i < NUM_SECTORS; i++)
                EraseFlashSector(i); // erase HOF.
            SavePartyAndObjects();
            SaveWriteToFlash(-1, gRamSaveBuffers);
            break;
    }
    gTrainerHillVBlankCounter = backupVar;
    return 0;
}

u8 TrySavingData(u8 saveType) // Entrypoint
{
    if (gFlashMemoryPresent != TRUE)
    {
        gSaveAttemptStatus = SAVE_STATUS_ERROR;
        return SAVE_STATUS_ERROR;
    }

    HandleSavingData(saveType);
    if (!gDamagedSaveSectors)
    {
        gSaveAttemptStatus = SAVE_STATUS_OK;
        return SAVE_STATUS_OK;
    }
    else
    {
        DoSaveFailedScreen(saveType);
        gSaveAttemptStatus = SAVE_STATUS_ERROR;
        return SAVE_STATUS_ERROR;
    }
}

// Very similar to FullSaveGame()
bool8 FullSaveGame_Link(void) // trade.c
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;
    UpdateSaveAddresses();
    SavePartyAndObjects();
    RestoreSaveBackupVarsAndIncrement(gRamSaveBuffers);

    // Missing a function here
    return FALSE;
}

bool8 sub_81533AC(void) // trade.c
{
    u8 status = WriteSingleChunkAndIncrement(SAVE_SLOT_SECTORS, gRamSaveBuffers);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    if (status == SAVE_STATUS_ERROR)
        return TRUE;
    else
        return FALSE;
}

bool8 sub_81533E0(void) // trade.c
{
    ErasePreviousChunk(SAVE_SLOT_SECTORS, gRamSaveBuffers);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

bool8 sub_8153408(void) // trade.c
{
    WriteSomeFlashByteToPrevSector(SAVE_SLOT_SECTORS, gRamSaveBuffers);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

u8 FullSaveGame(void)
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;

    UpdateSaveAddresses();
    SavePartyAndObjects();
    // Does not update the last written sector
    RestoreSaveBackupVars(gRamSaveBuffers);

    // Possibly delete the other save file?
    // gCurSaveChunk will always be 0 here
    ErasePreviousChunk(gCurSaveChunk + 1, gRamSaveBuffers);
    return FALSE;
}

bool8 CheckSaveFile(void)
{
    u8 retVal = FALSE;
    u16 sectorId = ++gCurSaveChunk;
    if (sectorId < SLOT_POKESTORE) // Saveblocks 1 and 2
    {
        ErasePreviousChunk(gCurSaveChunk + 1, gRamSaveBuffers);
        WriteSomeFlashByte0x25ToPrevSector(sectorId, gRamSaveBuffers);
    }
    else // Pokémon storage
    {
        WriteSomeFlashByte0x25ToPrevSector(sectorId, gRamSaveBuffers);
        retVal = TRUE;
    }
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_LINK);
    return retVal;
}

u8 Save_LoadGameData(u8 saveType)
{
    u8 status;

    if (gFlashMemoryPresent != TRUE)
    {
        gSaveFileStatus = SAVE_STATUS_NO_FLASH;
        return SAVE_STATUS_ERROR;
    }

    UpdateSaveAddresses();
    switch (saveType)
    {
    case SAVE_NORMAL:
    default:
        status = TryReadAllSaveSectorsCurrentSlot(0xFFFF, gRamSaveBuffers);
        LoadPartyAndObjects();
        gSaveFileStatus = status;
        gGameContinueCallback = 0;
        break;
    case SAVE_HALL_OF_FAME:
        status = ReadSomeUnknownSectorAndVerify(SECTOR_HOF_1, gDecompressionBuffer, SECTOR_DATA_SIZE);
        if (status == SAVE_STATUS_OK)
            status = ReadSomeUnknownSectorAndVerify(SECTOR_HOF_2, gDecompressionBuffer + SECTOR_DATA_SIZE, SECTOR_DATA_SIZE);
        break;
    }

    return status;
}

u16 sub_815355C(void)
{
    u16 i, v3;
    struct SaveSection* savSection;

    savSection = gFastSaveSection = &gSaveDataBuffer;
    if (gFlashMemoryPresent != TRUE)
        return SAVE_STATUS_EMPTY;
    UpdateSaveAddresses();
    GetSaveValidStatus(gRamSaveBuffers);
    v3 = SAVE_SLOT_SECTORS * (gSaveCounter % 2);
    for (i = 0; i < SAVE_SLOT_SECTORS; i++)
    {
        DoReadFlashWholeSection(i + v3, gFastSaveSection);
        if (gFastSaveSection->id == 0)
            return savSection->data[10] +
                   savSection->data[11] +
                   savSection->data[12] +
                   savSection->data[13];
    }
    return SAVE_STATUS_EMPTY;
}

u32 TryReadSpecialSaveSection(u8 sector, u8* dst)
{
    s32 i;
    s32 size;
    u8* savData;

    if (sector != SECTOR_TRAINER_HILL && sector != SECTOR_BATTLE_RECORD)
        return SAVE_STATUS_ERROR;
    ReadFlash(sector, 0, (u8 *)&gSaveDataBuffer, sizeof(struct SaveSection));
    if (*(u32*)(&gSaveDataBuffer.data[0]) != SPECIAL_SECTION_SENTINEL)
        return SAVE_STATUS_ERROR;
    // copies whole save section except u32 counter
    i = 0;
    size = 0xFFB;
    savData = &gSaveDataBuffer.data[4];
    for (; i <= size; i++)
        dst[i] = savData[i];
    return SAVE_STATUS_OK;
}

u32 TryWriteSpecialSaveSection(u8 sector, u8* src)
{
    s32 i;
    s32 size;
    u8* savData;
    void* savDataBuffer;

    if (sector != SECTOR_TRAINER_HILL && sector != SECTOR_BATTLE_RECORD)
        return SAVE_STATUS_ERROR;

    savDataBuffer = &gSaveDataBuffer;
    *(u32*)(savDataBuffer) = SPECIAL_SECTION_SENTINEL;

    // copies whole save section except u32 counter
    i = 0;
    size = 0xFFB;
    savData = &gSaveDataBuffer.data[4];
    for (; i <= size; i++)
        savData[i] = src[i];
    if (ProgramFlashSectorAndVerify(sector, savDataBuffer) != 0)
        return SAVE_STATUS_ERROR;
    return SAVE_STATUS_OK;
}

#define tState       data[0]
#define tTimer       data[1]
#define tPartialSave data[2]

// Save the game while linked
void Task_LinkSave(u8 taskId)
{
    s16* data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        gSoftResetDisabled = TRUE;
        tState = 1;
        break;
    case 1:
        SetLinkStandbyCallback();
        tState = 2;
        break;
    case 2:
        if (IsLinkTaskFinished())
        {
            if (!tPartialSave)
                save_serialize_map();
            tState = 3;
        }
        break;
    case 3:
        if (!tPartialSave)
            SetContinueGameWarpStatusToDynamicWarp();
        FullSaveGame_Link();
        tState = 4;
        break;
    case 4:
        if (++tTimer == 5)
        {
            tTimer = 0;
            tState = 5;
        }
        break;
    case 5:
        if (sub_81533AC())
            tState = 6;
        else
            tState = 4;
        break;
    case 6:
        sub_81533E0();
        tState = 7;
        break;
    case 7:
        if (!tPartialSave)
            ClearContinueGameWarpStatus2();
        SetLinkStandbyCallback();
        tState = 8;
        break;
    case 8:
        if (IsLinkTaskFinished())
        {
            sub_8153408();
            tState = 9;
        }
        break;
    case 9:
        SetLinkStandbyCallback();
        tState = 10;
        break;
    case 10:
        if (IsLinkTaskFinished())
            tState++;
        break;
    case 11:
        if (++tTimer > 5)
        {
            gSoftResetDisabled = FALSE;
            DestroyTask(taskId);
        }
        break;
    }
}

#undef tState
#undef tTimer
#undef tPartialSave
