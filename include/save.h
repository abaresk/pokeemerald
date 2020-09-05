#ifndef GUARD_SAVE_H
#define GUARD_SAVE_H

// This indices are used relative to the Save Slot
enum {
    SLOT_SECTOR_0,
    SLOT_SECTOR_1,
    SLOT_SECTOR_2,
    SLOT_SECTOR_3,
    SLOT_SECTOR_4,
    SLOT_SECTOR_5,
    SLOT_SECTOR_6,
    SLOT_SECTOR_7,
    SLOT_SECTOR_8,
    SLOT_SECTOR_9,
    SLOT_SECTOR_10,
    SLOT_SECTOR_11,
    SLOT_SECTOR_12,
    SLOT_SECTOR_13,
    SLOT_SECTOR_END,
};

// Relative to Saveblock sector
#define SLOT_START SLOT_SECTOR_0
#define SLOT_SAVEBLOCK_2 SLOT_SECTOR_0
#define SLOT_SAVEBLOCK_1 SLOT_SECTOR_1
#define SLOT_POKESTORE SLOT_SECTOR_5
#define SLOT_END SLOT_SECTOR_END
#define SAVE_SLOT_SECTORS (SLOT_END - SLOT_START)

// Absolute sector locations
#define SECTOR_HOF_1 28
#define SECTOR_HOF_2 29
#define SECTOR_TRAINER_HILL 30
#define SECTOR_BATTLE_RECORD 31
#define NUM_SECTORS 32

// Bitmask of all saveblock sectors
#define ALL_SECTORS ((1 << SAVE_SLOT_SECTORS) - 1)

// It's more of a pointer than a buffer
struct SaveBlockChunk {
    void* data;
    u16 size;
};

struct SaveSection {
    u8 data[0xFF4];
    u16 id;
    u16 checksum;
    u32 security;
    u32 counter;
}; // size is 0x1000

// headless save section?
struct UnkSaveSection {
    u8 data[0xFF4];
    u32 security;
}; // size is 0xFF8

struct SaveblockOffset {
    u16 offset;
    u16 size;
};

#define FILE_SIGNATURE 0x8012025
#define SPECIAL_SECTION_SENTINEL 0xB39D

// SetDamagedSectorBits states
enum {
    SECTOR_DAMAGED,
    SECTOR_OK,
    SECTOR_CHECK // unused
};

// Do save types
enum {
    SAVE_NORMAL,
    SAVE_LINK,
    // EREADER_SAVE, // deprecated in Emerald
    SAVE_LINK2, // unknown 2nd link save
    SAVE_HALL_OF_FAME,
    SAVE_OVERWRITE_DIFFERENT_FILE,
    SAVE_CLEAR_RECORDS // unused
};

#define SAVE_STATUS_EMPTY 0
#define SAVE_STATUS_OK 1
#define SAVE_STATUS_CORRUPT 2
#define SAVE_STATUS_NO_FLASH 4
#define SAVE_STATUS_ERROR 0xFF

extern u16 gFirstSaveSector;
extern u32 gPrevSaveCounter;
extern u16 gLastKnownGoodSector;
extern u32 gDamagedSaveSectors;
extern u32 gSaveCounter;
extern struct SaveSection* gFastSaveSection;
extern u16 gCurSaveChunk;
extern u16 gSaveFileStatus;
extern void (*gGameContinueCallback)(void);
extern struct SaveBlockChunk gRamSaveBuffers[];
extern u16 gUnknown_03006294;

extern struct SaveSection gSaveDataBuffer;

void Save_EraseAllData(void);
void Save_ResetSaveCounters(void);
u8 HandleSavingData(u8 saveType);
u8 TrySavingData(u8 saveType);
bool8 FullSaveGame_Link(void);
bool8 sub_81533AC(void);
bool8 sub_81533E0(void);
bool8 sub_8153408(void);
bool8 FullSaveGame(void);
bool8 CheckSaveFile(void);
u8 Save_LoadGameData(u8 saveType);
u16 sub_815355C(void);
u32 TryReadSpecialSaveSection(u8 sector, u8* dst);
u32 TryWriteSpecialSaveSection(u8 sector, u8* src);
void Task_LinkSave(u8 taskId);

// save_failed_screen.c
void DoSaveFailedScreen(u8 saveType);

#endif // GUARD_SAVE_H
