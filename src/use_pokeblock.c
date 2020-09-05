#include "global.h"
#include "main.h"
#include "pokeblock.h"
#include "malloc.h"
#include "decompress.h"
#include "graphics.h"
#include "palette.h"
#include "pokenav.h"
#include "menu_specialized.h"
#include "scanline_effect.h"
#include "text.h"
#include "bg.h"
#include "window.h"
#include "text_window.h"
#include "constants/rgb.h"
#include "sound.h"
#include "constants/songs.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "menu.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "pokemon_summary_screen.h"
#include "item_menu.h"

/*
    This file handles the screen where the player chooses
    which pokemon to give a pokeblock to. The subsequent scene
    of feeding the pokeblock to the pokemon is handled by
    pokeblock_feed.c, and the rest of the pokeblock menu (and
    other pokeblock-related functions) are in pokeblock.c
*/

enum { WIN_NAME, WIN_NATURE, WIN_TEXT, WIN_COUNT };

#define TAG_UP_DOWN   0
#define TAG_CONDITION 1

// At any one time, the currently selected mon and its two adjacent neighbors can be loaded
// IDs to refer to one of these 3 are called "load id" in this file
#define NUM_SELECTIONS_LOADED 3

struct UsePokeblockSession {
    void (*callback)(void);
    void (*exitCallback)(void);
    struct Pokeblock* pokeblock;
    struct Pokemon* mon;
    u8 stringBuffer[64];
    u8 mainState;
    u8 unused1;
    u8 timer;
    u8 statId;
    u8 numEnhancements;
    u8 unused2;
    bool8 monInTopHalf;
    u8 conditionsBeforeBlock[FLAVOR_COUNT];
    u8 conditionsAfterBlock[FLAVOR_COUNT];
    u8 enhancements[FLAVOR_COUNT];
    s16 pokeblockStatBoosts[FLAVOR_COUNT];
    u8 numSelections; // num in party + 1 (for Cancel)
    u8 curSelection;
    bool8 (*loadNewSelection)(void);
    u8 helperState;
    u8 unused3;
    u8 natureText[34];
};

// This struct is identical to PokenavMonList, the struct used for managing lists of pokemon in the pokenav
// Given that this screen is essentially duplicated in the poknav, this struct was probably the same one with
// a more general name/purpose
// TODO: Once the pokenav conditions screens are documented, resolve the above
struct UsePokeblockMenuPokemon {
    u8 boxId; // Because this screen is never used for the PC this is always set to TOTAL_BOXES_COUNT to refer to party
    u8 monId;
    u16 data; // never read
};

struct UsePokeblockMenu {
    u32 unused;
    u16 partyPalettes[PARTY_SIZE][0x40];
    u8 partySheets[NUM_SELECTIONS_LOADED][0x2000];
    u8 unusedBuffer[0x1000];
    u8 tilemapBuffer[BG_SCREEN_SIZE + 2];
    u8 selectionIconSpriteIds[PARTY_SIZE + 1];
    s16 curMonXOffset;
    u8 curMonSpriteId;
    u16 curMonPalette;
    u16 curMonSheet;
    u8* curMonTileStart;
    struct Sprite* sparkles[MAX_CONDITION_SPARKLES];
    struct Sprite* condition[2];
    u8 toLoadSelection;
    u8 locationStrings[NUM_SELECTIONS_LOADED][24]; // Gets an "in party" or "in box #" string that never gets printed
    u8 monNameStrings[NUM_SELECTIONS_LOADED][64];
    struct ConditionGraph graph;
    u8 numSparkles[NUM_SELECTIONS_LOADED];
    s8 curLoadId;
    s8 nextLoadId;
    s8 prevLoadId;
    s8 toLoadId;
    struct UsePokeblockMenuPokemon party[PARTY_SIZE];
    struct UsePokeblockSession info;
};

static void SetUsePokeblockCallback(void (*func)(void));
static void LoadUsePokeblockMenu(void);
static void CB2_UsePokeblockMenu(void);
static void CB2_ReturnToUsePokeblockMenu(void);
static void ShowUsePokeblockMenu(void);
static void CB2_ShowUsePokeblockMenuForResults(void);
static void ShowUsePokeblockMenuForResults(void);
static void LoadPartyInfo(void);
static void LoadAndCreateSelectionIcons(void);
static u8 GetSelectionIdFromPartyId(u8);
static bool8 LoadConditionTitle(void);
static bool8 LoadUsePokeblockMenuGfx(void);
static void UpdateMonPic(u8);
static void UpdateMonInfoText(u16, bool8);
static void UsePokeblockMenu(void);
static void UpdateSelection(bool8);
static void CloseUsePokeblockMenu(void);
static void AskUsePokeblock(void);
static s8 HandleAskUsePokeblockInput(void);
static bool8 IsSheenMaxed(void);
static void PrintWontEatAnymore(void);
static void FeedPokeblockToMon(void);
static void EraseMenuWindow(void);
static u8 GetPartyIdFromSelectionId(u8);
static void ShowPokeblockResults(void);
static void CalculateConditionEnhancements(void);
static void LoadAndCreateUpDownSprites(void);
static void CalculateNumAdditionalSparkles(u8);
static void PrintFirstEnhancement(void);
static bool8 TryPrintNextEnhancement(void);
static void BufferEnhancedStatText(u8*, u8, s16);
static void PrintMenuWindowText(const u8*);
static void CalculatePokeblockEffectiveness(struct Pokeblock*, struct Pokemon*);
static void SpriteCB_UpDown(struct Sprite*);
static void LoadInitialMonInfo(void);
static void LoadMonInfo(s16, u8);
static bool8 LoadNewSelection_CancelToMon(void);
static bool8 LoadNewSelection_MonToCancel(void);
static bool8 LoadNewSelection_MonToMon(void);
static void SpriteCB_SelectionIconPokeball(struct Sprite*);
static void SpriteCB_SelectionIconCancel(struct Sprite*);
static void SpriteCB_MonPic(struct Sprite*);
static void SpriteCB_Condition(struct Sprite*);

extern const u16 gConditionGraphData_Pal[];
extern const u16 gConditionText_Pal[];

// The below 3 are saved for returning to the screen after feeding a pokeblock to a mon
// so that the rest of the data can be freed
static EWRAM_DATA struct UsePokeblockSession* sInfo = NULL;
static EWRAM_DATA void (*sExitCallback)(void) = NULL;
static EWRAM_DATA struct Pokeblock* sPokeblock = NULL;
EWRAM_DATA u8 gPokeblockMonId = 0;
EWRAM_DATA s16 gPokeblockGain = 0;
static EWRAM_DATA u8* sGraph_Tilemap = NULL;
static EWRAM_DATA u8* sGraph_Gfx = NULL;
static EWRAM_DATA u8* sMonFrame_TilemapPtr = NULL;
static EWRAM_DATA struct UsePokeblockMenu* sMenu = NULL;

static const u32 sMonFrame_Pal[] = INCBIN_U32("graphics/pokeblock/use_screen/mon_frame_pal.bin");
static const u32 sMonFrame_Gfx[] = INCBIN_U32("graphics/pokeblock/use_screen/mon_frame.4bpp");
static const u32 sMonFrame_Tilemap[] = INCBIN_U32("graphics/pokeblock/use_screen/mon_frame.bin");
static const u32 sGraphData_Tilemap[] = INCBIN_U32("graphics/pokeblock/use_screen/graph_data.bin");

// The condition/flavors aren't listed in their normal order in this file, they're listed as shown on the graph going
// counter-clockwise Normally they would go Cool/Spicy, Beauty/Dry, Cute/Sweet, Smart/Bitter, Tough/Sour (also graph
// order, but clockwise)
static const u32 sMonDataConditions[FLAVOR_COUNT] = { MON_DATA_COOL, MON_DATA_TOUGH, MON_DATA_SMART, MON_DATA_CUTE,
                                                      MON_DATA_BEAUTY };

static const u8 sFlavors[FLAVOR_COUNT] = { FLAVOR_SPICY, FLAVOR_SOUR, FLAVOR_BITTER, FLAVOR_SWEET, FLAVOR_DRY };

static const u8 sNatureTextColors[] = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_BLUE, TEXT_COLOR_WHITE };

static const struct BgTemplate sBgTemplates[4] = { { .bg = 0,
                                                     .charBaseIndex = 2,
                                                     .mapBaseIndex = 0x1F,
                                                     .screenSize = 0,
                                                     .paletteMode = 0,
                                                     .priority = 0,
                                                     .baseTile = 0 },
                                                   { .bg = 1,
                                                     .charBaseIndex = 0,
                                                     .mapBaseIndex = 0x1E,
                                                     .screenSize = 0,
                                                     .paletteMode = 0,
                                                     .priority = 3,
                                                     .baseTile = 0 },
                                                   { .bg = 3,
                                                     .charBaseIndex = 3,
                                                     .mapBaseIndex = 0x1D,
                                                     .screenSize = 0,
                                                     .paletteMode = 0,
                                                     .priority = 2,
                                                     .baseTile = 0x100 },
                                                   { .bg = 2,
                                                     .charBaseIndex = 0,
                                                     .mapBaseIndex = 0x17,
                                                     .screenSize = 0,
                                                     .paletteMode = 0,
                                                     .priority = 1,
                                                     .baseTile = 0 } };

static const struct WindowTemplate sWindowTemplates[WIN_COUNT + 1] = { [WIN_NAME] = { .bg = 0,
                                                                                      .tilemapLeft = 13,
                                                                                      .tilemapTop = 1,
                                                                                      .width = 13,
                                                                                      .height = 4,
                                                                                      .paletteNum = 15,
                                                                                      .baseBlock = 1 },
                                                                       [WIN_NATURE] = { .bg = 0,
                                                                                        .tilemapLeft = 0,
                                                                                        .tilemapTop = 14,
                                                                                        .width = 11,
                                                                                        .height = 2,
                                                                                        .paletteNum = 15,
                                                                                        .baseBlock = 0x35 },
                                                                       [WIN_TEXT] = { .bg = 0,
                                                                                      .tilemapLeft = 1,
                                                                                      .tilemapTop = 17,
                                                                                      .width = 28,
                                                                                      .height = 2,
                                                                                      .paletteNum = 15,
                                                                                      .baseBlock = 0x4B },
                                                                       DUMMY_WIN_TEMPLATE };

static const struct WindowTemplate sUsePokeblockYesNoWinTemplate = {
    .bg = 0, .tilemapLeft = 24, .tilemapTop = 11, .width = 5, .height = 4, .paletteNum = 15, .baseBlock = 0x83
};

static const u8* const sContestStatNames[] = { gText_Coolness, gText_Toughness, gText_Smartness, gText_Cuteness,
                                               gText_Beauty3 };

static const struct SpriteSheet sSpriteSheet_UpDown = { gUsePokeblockUpDown_Gfx, 0x200, TAG_UP_DOWN };

static const struct SpritePalette sSpritePalette_UpDown = { gUsePokeblockUpDown_Pal, TAG_UP_DOWN };

static const s16 sUpDownCoordsOnGraph[FLAVOR_COUNT][2] = {
    { 156, 36 }, { 117, 59 }, { 117, 118 }, { 197, 118 }, { 197, 59 }
};

static const struct OamData sOam_UpDown = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const union AnimCmd sAnim_Up[] = { ANIMCMD_FRAME(0, 5), ANIMCMD_END };

static const union AnimCmd sAnim_Down[] = { ANIMCMD_FRAME(8, 5), ANIMCMD_END };

static const union AnimCmd* const sAnims_UpDown[] = { sAnim_Up, sAnim_Down };

static const struct SpriteTemplate sSpriteTemplate_UpDown = {
    .tileTag = TAG_UP_DOWN,
    .paletteTag = TAG_UP_DOWN,
    .oam = &sOam_UpDown,
    .anims = sAnims_UpDown,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOam_Condition = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const union AnimCmd sAnim_Condition_0[] = { ANIMCMD_FRAME(0, 5), ANIMCMD_END };

static const union AnimCmd sAnim_Condition_1[] = { ANIMCMD_FRAME(32, 5), ANIMCMD_END };

static const union AnimCmd sAnim_Condition_2[] = { ANIMCMD_FRAME(64, 5), ANIMCMD_END };

static const union AnimCmd* const sAnims_Condition[] = { sAnim_Condition_0, sAnim_Condition_1, sAnim_Condition_2 };

static const struct SpriteTemplate sSpriteTemplate_Condition = {
    .tileTag = TAG_CONDITION,
    .paletteTag = TAG_CONDITION,
    .oam = &sOam_Condition,
    .anims = sAnims_Condition,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Condition,
};

static const struct SpritePalette sSpritePalette_Condition = { gUsePokeblockCondition_Pal, TAG_CONDITION };

// When first opening the selection screen
void ChooseMonToGivePokeblock(struct Pokeblock* pokeblock, void (*callback)(void)) {
    sMenu = AllocZeroed(sizeof(*sMenu));
    sInfo = &sMenu->info;
    sInfo->pokeblock = pokeblock;
    sInfo->exitCallback = callback;
    SetUsePokeblockCallback(LoadUsePokeblockMenu);
    SetMainCallback2(CB2_UsePokeblockMenu);
}

// When returning to the selection screen after feeding a pokeblock to a mon
static void CB2_ReturnAndChooseMonToGivePokeblock(void) {
    sMenu = AllocZeroed(sizeof(*sMenu));
    sInfo = &sMenu->info;
    sInfo->pokeblock = sPokeblock;
    sInfo->exitCallback = sExitCallback;
    gPokeblockMonId = GetSelectionIdFromPartyId(gPokeblockMonId);
    sInfo->monInTopHalf = (gPokeblockMonId <= PARTY_SIZE / 2) ? FALSE : TRUE;
    SetUsePokeblockCallback(LoadUsePokeblockMenu);
    SetMainCallback2(CB2_ReturnToUsePokeblockMenu);
}

static void CB2_ReturnToUsePokeblockMenu(void) {
    sInfo->callback();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    if (sInfo->callback == ShowUsePokeblockMenu) {
        sInfo->mainState = 0;
        SetMainCallback2(CB2_ShowUsePokeblockMenuForResults);
    }
}

static void CB2_ShowUsePokeblockMenuForResults(void) {
    ShowUsePokeblockMenuForResults();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void CB2_UsePokeblockMenu(void) {
    sInfo->callback();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

static void VBlankCB_UsePokeblockMenu(void) {
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    sub_81D2108(&sMenu->graph);
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void SetUsePokeblockCallback(void (*func)(void)) {
    sInfo->callback = func;
    sInfo->mainState = 0;
}

static void LoadUsePokeblockMenu(void) {
    switch (sInfo->mainState) {
        case 0:
            sMenu->curMonSpriteId = 0xFF;
            sub_81D1ED4(&sMenu->graph);
            sInfo->mainState++;
            break;
        case 1:
            ResetSpriteData();
            FreeAllSpritePalettes();
            sInfo->mainState++;
            break;
        case 2:
            SetVBlankCallback(NULL);
            CpuFill32(0, (void*)(VRAM), VRAM_SIZE);
            sInfo->mainState++;
            break;
        case 3:
            ResetBgsAndClearDma3BusyFlags(0);
            InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
            InitWindows(sWindowTemplates);
            DeactivateAllTextPrinters();
            LoadUserWindowBorderGfx(0, 0x97, 0xE0);
            sInfo->mainState++;
            break;
        case 4:
            sInfo->mainState++;
            break;
        case 5:
            if (!LoadConditionTitle())
                sInfo->mainState++;
            break;
        case 6:
            gKeyRepeatStartDelay = 20;
            LoadPartyInfo();
            sInfo->mainState++;
            break;
        case 7:
            if (!LoadUsePokeblockMenuGfx())
                sInfo->mainState++;
            break;
        case 8:
            UpdateMonPic(0);
            LoadAndCreateSelectionIcons();
            sInfo->mainState++;
            break;
        case 9:
            if (!MoveConditionMonOnscreen(&sMenu->curMonXOffset))
                sInfo->mainState++;
            break;
        case 10:
            sInfo->mainState++;
            break;
        case 11:
            sub_81D2754(sMenu->graph.unk0[0], sMenu->graph.unk14[0]);
            sub_81D20AC(&sMenu->graph);
            sInfo->mainState++;
            break;
        case 12:
            if (!sub_81D20BC(&sMenu->graph)) {
                sub_81D1F84(&sMenu->graph, sMenu->graph.unk14[0], sMenu->graph.unk14[0]);
                sInfo->mainState++;
            }
            break;
        case 13:
            sub_81D2230(&sMenu->graph);
            sInfo->mainState++;
            break;
        case 14:
            PutWindowTilemap(WIN_NAME);
            PutWindowTilemap(WIN_NATURE);
            UpdateMonInfoText(0, TRUE);
            sInfo->mainState++;
            break;
        case 15:
            SetUsePokeblockCallback(ShowUsePokeblockMenu);
            break;
    }
}

static void ShowUsePokeblockMenu(void) {
    switch (sInfo->mainState) {
        case 0:
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
            SetVBlankCallback(VBlankCB_UsePokeblockMenu);
            ShowBg(0);
            ShowBg(1);
            ShowBg(3);
            ShowBg(2);
            sInfo->mainState++;
            break;
        case 1:
            if (!gPaletteFade.active) {
                ResetConditionSparkleSprites(sMenu->sparkles);
                if (sMenu->info.curSelection != sMenu->info.numSelections - 1) {
                    u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
                    CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
                }

                SetUsePokeblockCallback(UsePokeblockMenu);
            }
            break;
    }
}

enum {
    STATE_HANDLE_INPUT,
    STATE_UPDATE_SELECTION,
    STATE_2, // unused state
    STATE_CLOSE,
    STATE_4, // unused state
    STATE_CONFIRM_SELECTION,
    STATE_HANDLE_CONFIRMATION,
    STATE_WAIT_MSG,
};

static void UsePokeblockMenu(void) {
    bool8 loading;

    switch (sInfo->mainState) {
        case STATE_HANDLE_INPUT:
            if (JOY_HELD(DPAD_UP)) {
                PlaySE(SE_SELECT);
                UpdateSelection(TRUE);
                DestroyConditionSparkleSprites(sMenu->sparkles);
                sInfo->mainState = STATE_UPDATE_SELECTION;
            } else if (JOY_HELD(DPAD_DOWN)) {
                PlaySE(SE_SELECT);
                UpdateSelection(FALSE);
                DestroyConditionSparkleSprites(sMenu->sparkles);
                sInfo->mainState = STATE_UPDATE_SELECTION;
            } else if (JOY_NEW(B_BUTTON)) {
                PlaySE(SE_SELECT);
                sInfo->mainState = STATE_CLOSE;
            } else if (JOY_NEW(A_BUTTON)) {
                PlaySE(SE_SELECT);

                // If last item, selected Cancel. Otherwise selected mon
                if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
                    sInfo->mainState = STATE_CLOSE;
                else
                    sInfo->mainState = STATE_CONFIRM_SELECTION;
            }
            break;
        case STATE_UPDATE_SELECTION:
            loading = sMenu->info.loadNewSelection();
            if (!loading)
                sInfo->mainState = STATE_HANDLE_INPUT;
            break;
        case STATE_2:
            break;
        case STATE_CLOSE:
            SetUsePokeblockCallback(CloseUsePokeblockMenu);
            break;
        case STATE_4:
            break;
        case STATE_CONFIRM_SELECTION:
            AskUsePokeblock();
            sInfo->mainState++;
            break;
        case STATE_HANDLE_CONFIRMATION:
            switch (HandleAskUsePokeblockInput()) {
                case 1: // NO
                case MENU_B_PRESSED:
                    sInfo->mainState = STATE_HANDLE_INPUT;
                    break;
                case 0: // YES
                    if (IsSheenMaxed()) {
                        PrintWontEatAnymore();
                        sInfo->mainState = STATE_WAIT_MSG;
                    } else {
                        SetUsePokeblockCallback(FeedPokeblockToMon);
                    }
                    break;
            }
            break;
        case STATE_WAIT_MSG:
            if (JOY_NEW(A_BUTTON | B_BUTTON)) {
                EraseMenuWindow();
                sInfo->mainState = STATE_HANDLE_INPUT;
            }
            break;
    }
}

static void FeedPokeblockToMon(void) {
    switch (sInfo->mainState) {
        case 0:
            gPokeblockMonId = GetPartyIdFromSelectionId(sMenu->info.curSelection);
            sExitCallback = sInfo->exitCallback;
            sPokeblock = sInfo->pokeblock;
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
            sInfo->mainState++;
            break;
        case 1:
            if (!gPaletteFade.active) {
                SetVBlankCallback(NULL);
                FREE_AND_SET_NULL(sGraph_Tilemap);
                FREE_AND_SET_NULL(sGraph_Gfx);
                FREE_AND_SET_NULL(sMonFrame_TilemapPtr);
                FREE_AND_SET_NULL(sMenu);
                FreeAllWindowBuffers();
                gMain.savedCallback = CB2_ReturnAndChooseMonToGivePokeblock;
                CB2_PreparePokeblockFeedScene();
            }
            break;
    }
}

static void ShowUsePokeblockMenuForResults(void) {
    bool8 loading;

    switch (sInfo->mainState) {
        case 0:
            if (sMenu->info.curSelection != gPokeblockMonId) {
                UpdateSelection(sInfo->monInTopHalf);
                sInfo->mainState++;
            } else {
                sInfo->mainState = 3;
            }
            break;
        case 1:
            loading = sMenu->info.loadNewSelection();
            if (!loading)
                sInfo->mainState = 0;
            break;
        case 2:
            break;
        case 3:
            BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
            sInfo->mainState++;
            break;
        case 4:
            ShowBg(0);
            ShowBg(1);
            ShowBg(3);
            ShowBg(2);
            sInfo->mainState++;
            break;
        case 5:
            SetVBlankCallback(VBlankCB_UsePokeblockMenu);
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
            sInfo->mainState++;
            break;
        case 6:
            if (!gPaletteFade.active) {
                ResetConditionSparkleSprites(sMenu->sparkles);
                SetUsePokeblockCallback(ShowPokeblockResults);
                SetMainCallback2(CB2_UsePokeblockMenu);
            }
            break;
    }
}

static void ShowPokeblockResults(void) {
    u8 var;

    switch (sInfo->mainState) {
        case 0:
            sInfo->mon = gPlayerParty;
            sInfo->mon += sMenu->party[sMenu->info.curSelection].monId;
            DestroyConditionSparkleSprites(sMenu->sparkles);
            sInfo->mainState++;
            break;
        case 1:
            if (JOY_NEW(A_BUTTON | B_BUTTON))
                sInfo->mainState++;
            break;
        case 2:
            CalculateConditionEnhancements();
            sub_81D2754(sInfo->conditionsAfterBlock, sMenu->graph.unk14[3]);
            sub_81D1F84(&sMenu->graph, sMenu->graph.unk14[sMenu->curLoadId], sMenu->graph.unk14[3]);
            LoadAndCreateUpDownSprites();
            sInfo->mainState++;
            break;
        case 3:
            var = TransitionConditionGraph(&sMenu->graph);
            if (!var) {
                CalculateNumAdditionalSparkles(GetPartyIdFromSelectionId(sMenu->info.curSelection));
                if (sMenu->info.curSelection != sMenu->info.numSelections - 1) {
                    u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
                    CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
                }

                sInfo->timer = 0;
                sInfo->mainState++;
            }
            break;
        case 4:
            if (++sInfo->timer > 16) {
                PrintFirstEnhancement();
                sInfo->mainState++;
            }
            break;
        case 5:
            if (JOY_NEW(A_BUTTON | B_BUTTON) && !TryPrintNextEnhancement()) {
                TryClearPokeblock((u8)gSpecialVar_ItemId);
                SetUsePokeblockCallback(CloseUsePokeblockMenu);
            }
            break;
    }
}

static void CloseUsePokeblockMenu(void) {
    u8 i;

    switch (sInfo->mainState) {
        case 0:
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
            sInfo->mainState++;
            break;
        case 1:
            if (!gPaletteFade.active)
                sInfo->mainState = 2;
            break;
        case 2:
            gScanlineEffect.state = 3;
            ScanlineEffect_InitHBlankDmaTransfer();
            sInfo->mainState++;
            break;
        case 3:
            SetMainCallback2(sInfo->exitCallback);
            FreeConditionSparkles(sMenu->sparkles);
            for (i = 0; i < ARRAY_COUNT(sMenu->selectionIconSpriteIds); i++)
                DestroySprite(&gSprites[sMenu->selectionIconSpriteIds[i]]);

            FreeSpriteTilesByTag(TAG_UP_DOWN);
            FreeSpriteTilesByTag(TAG_CONDITION);
            FreeSpritePaletteByTag(TAG_UP_DOWN);
            FreeSpritePaletteByTag(TAG_CONDITION);

            for (i = 0; i < ARRAY_COUNT(sMenu->condition); i++)
                DestroySprite(sMenu->condition[i]);

            if (sMenu->curMonSpriteId != 0xFF)
                DestroySprite(&gSprites[sMenu->curMonSpriteId]);

            SetVBlankCallback(NULL);
            FREE_AND_SET_NULL(sGraph_Tilemap);
            FREE_AND_SET_NULL(sGraph_Gfx);
            FREE_AND_SET_NULL(sMonFrame_TilemapPtr);
            FREE_AND_SET_NULL(sMenu);
            FreeAllWindowBuffers();
            break;
    }
}

static void AskUsePokeblock(void) {
    u8 stringBuffer[0x40];

    GetMonData(&gPlayerParty[GetPartyIdFromSelectionId(sMenu->info.curSelection)], MON_DATA_NICKNAME, stringBuffer);
    StringGetEnd10(stringBuffer);
    StringAppend(stringBuffer, gText_GetsAPokeBlockQuestion);
    StringCopy(gStringVar4, stringBuffer);
    FillWindowPixelBuffer(WIN_TEXT, 17);
    DrawTextBorderOuter(WIN_TEXT, 151, 14);
    AddTextPrinterParameterized(WIN_TEXT, 1, gStringVar4, 0, 1, 0, NULL);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, 3);
    CreateYesNoMenu(&sUsePokeblockYesNoWinTemplate, 151, 14, 0);
}

static s8 HandleAskUsePokeblockInput(void) {
    s8 menuItem = Menu_ProcessInputNoWrapClearOnChoose();

    switch (menuItem) {
        case 0: // YES
            break;
        case MENU_B_PRESSED:
        case 1: // NO
            PlaySE(SE_SELECT);
            rbox_fill_rectangle(2);
            ClearWindowTilemap(2);
            break;
    }

    return menuItem;
}

static void PrintFirstEnhancement(void) {
    DrawTextBorderOuter(WIN_TEXT, 151, 14);
    FillWindowPixelBuffer(WIN_TEXT, 17);

    for (sInfo->statId = 0; sInfo->statId < FLAVOR_COUNT; sInfo->statId++) {
        if (sInfo->enhancements[sInfo->statId] != 0)
            break;
    }

    if (sInfo->statId < FLAVOR_COUNT)
        BufferEnhancedStatText(gStringVar4, sInfo->statId, sInfo->enhancements[sInfo->statId]);
    else
        BufferEnhancedStatText(gStringVar4, sInfo->statId, 0);

    PrintMenuWindowText(gStringVar4);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, 3);
}

static bool8 TryPrintNextEnhancement(void) {
    FillWindowPixelBuffer(WIN_TEXT, 17);

    while (1) {
        sInfo->statId++;
        if (sInfo->statId < FLAVOR_COUNT) {
            if (sInfo->enhancements[sInfo->statId] != 0)
                break;
        } else {
            sInfo->statId = FLAVOR_COUNT;
            return FALSE;
        }
    }

    BufferEnhancedStatText(gStringVar4, sInfo->statId, sInfo->enhancements[sInfo->statId]);
    PrintMenuWindowText(gStringVar4);
    CopyWindowToVram(WIN_TEXT, 2);

    return TRUE;
}

static void PrintWontEatAnymore(void) {
    FillWindowPixelBuffer(WIN_TEXT, 17);
    DrawTextBorderOuter(WIN_TEXT, 151, 14);
    AddTextPrinterParameterized(WIN_TEXT, 1, gText_WontEatAnymore, 0, 1, 0, NULL);
    PutWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, 3);
}

static void EraseMenuWindow(void) {
    rbox_fill_rectangle(WIN_TEXT);
    ClearWindowTilemap(WIN_TEXT);
    CopyWindowToVram(WIN_TEXT, 3);
}

static void PrintMenuWindowText(const u8* message) {
    AddTextPrinterParameterized(WIN_TEXT, 1, gStringVar4, 0, 1, 0, NULL);
}

static void BufferEnhancedStatText(u8* dest, u8 statId, s16 enhancement) {
    switch (enhancement) {
        case 1 ... 32767: // if > 0
            enhancement = 0;
            // fallthrough
        case -32768 ... - 1: // if < 0
            if (enhancement)
                dest[(u16)enhancement] += 0; // something you can't imagine
            StringCopy(dest, sContestStatNames[statId]);
            StringAppend(dest, gText_WasEnhanced);
            break;
        case 0:
            StringCopy(dest, gText_NothingChanged);
            break;
    }
}

static void GetMonConditions(struct Pokemon* mon, u8* data) {
    u16 i;

    for (i = 0; i < FLAVOR_COUNT; i++)
        data[i] = GetMonData(mon, sMonDataConditions[i]);
}

static void AddPokeblockToConditions(struct Pokeblock* pokeblock, struct Pokemon* mon) {
    u16 i;
    s16 cstat;
    u8 data;

    if (GetMonData(mon, MON_DATA_SHEEN) != 255) {
        CalculatePokeblockEffectiveness(pokeblock, mon);
        for (i = 0; i < FLAVOR_COUNT; i++) {
            data = GetMonData(mon, sMonDataConditions[i]);
            cstat = data + sInfo->pokeblockStatBoosts[i];
            if (cstat < 0)
                cstat = 0;
            if (cstat > 255)
                cstat = 255;
            data = cstat;
            SetMonData(mon, sMonDataConditions[i], &data);
        }

        cstat = (u8)(GetMonData(mon, MON_DATA_SHEEN)) + pokeblock->feel;
        if (cstat > 255)
            cstat = 255;

        data = cstat;
        SetMonData(mon, MON_DATA_SHEEN, &data);
    }
}

static void CalculateConditionEnhancements(void) {
    u16 i;
    struct Pokemon* mon = gPlayerParty;
    mon += sMenu->party[sMenu->info.curSelection].monId;

    GetMonConditions(mon, sInfo->conditionsBeforeBlock);
    AddPokeblockToConditions(sInfo->pokeblock, mon);
    GetMonConditions(mon, sInfo->conditionsAfterBlock);
    for (i = 0; i < FLAVOR_COUNT; i++)
        sInfo->enhancements[i] = sInfo->conditionsAfterBlock[i] - sInfo->conditionsBeforeBlock[i];
}

static void CalculatePokeblockEffectiveness(struct Pokeblock* pokeblock, struct Pokemon* mon) {
    s8 i, direction, taste;

    sInfo->pokeblockStatBoosts[0] = pokeblock->spicy;
    sInfo->pokeblockStatBoosts[1] = pokeblock->sour;
    sInfo->pokeblockStatBoosts[2] = pokeblock->bitter;
    sInfo->pokeblockStatBoosts[3] = pokeblock->sweet;
    sInfo->pokeblockStatBoosts[4] = pokeblock->dry;

    if (gPokeblockGain > 0)
        direction = 1;
    else if (gPokeblockGain < 0)
        direction = -1;
    else
        return;

    for (i = 0; i < FLAVOR_COUNT; i++) {
        s16 amount = sInfo->pokeblockStatBoosts[i];
        s8 boost = amount / 10;

        if (amount % 10 >= 5) // round to the nearest
            boost++;

        taste = GetMonFlavorRelation(mon, sFlavors[i]);
        if (taste == direction)
            sInfo->pokeblockStatBoosts[i] += boost * taste;
    }
}

static bool8 IsSheenMaxed(void) {
    if (GetBoxOrPartyMonData(sMenu->party[sMenu->info.curSelection].boxId, sMenu->party[sMenu->info.curSelection].monId,
                             MON_DATA_SHEEN, NULL) == 255)
        return TRUE;
    else
        return FALSE;
}

static u8 GetPartyIdFromSelectionId(u8 selectionId) {
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++) {
        if (!GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG)) {
            if (selectionId == 0)
                return i;
            selectionId--;
        }
    }

    return 0;
}

// Eggs are not viewable on the condition screen, so count how many are skipped over to reach the party id
static u8 GetSelectionIdFromPartyId(u8 partyId) {
    u8 i, numEggs;
    for (i = 0, numEggs = 0; i < partyId; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
            numEggs++;
    }

    return partyId - numEggs;
}

// Unused
static u8 GetPartyIdFromSelectionId_(u8 selectionId) {
    return GetPartyIdFromSelectionId(selectionId);
}

static void LoadAndCreateUpDownSprites(void) {
    u16 i, spriteId;

    LoadSpriteSheet(&sSpriteSheet_UpDown);
    LoadSpritePalette(&sSpritePalette_UpDown);
    sInfo->numEnhancements = 0;

    for (i = 0; i < FLAVOR_COUNT; i++) {
        if (sInfo->enhancements[i] != 0) {
            spriteId = CreateSprite(&sSpriteTemplate_UpDown, sUpDownCoordsOnGraph[i][0], sUpDownCoordsOnGraph[i][1], 0);
            if (spriteId != MAX_SPRITES) {
                if (sInfo->enhancements[i] != 0) // Always true here
                    gSprites[spriteId].callback = SpriteCB_UpDown;
                sInfo->numEnhancements++;
            }
        }
    }
}

static void SpriteCB_UpDown(struct Sprite* sprite) {
    if (sprite->data[0] < 6)
        sprite->pos2.y -= 2;
    else if (sprite->data[0] < 12)
        sprite->pos2.y += 2;

    if (++sprite->data[0] > 60) {
        DestroySprite(sprite);
        sInfo->numEnhancements--;
    }
}

static void LoadPartyInfo(void) {
    u16 i;
    u16 numMons;

    for (i = 0, numMons = 0; i < CalculatePlayerPartyCount(); i++) {
        if (!GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG)) {
            sMenu->party[numMons].boxId = TOTAL_BOXES_COUNT;
            sMenu->party[numMons].monId = i;
            sMenu->party[numMons].data = 0;
            numMons++;
        }
    }

    sMenu->info.curSelection = 0;
    sMenu->info.numSelections = numMons + 1;
    LoadInitialMonInfo();
}

static void LoadInitialMonInfo(void) {
    s16 nextSelection, prevSelection;

    LoadMonInfo(sMenu->info.curSelection, 0);
    sMenu->curLoadId = 0;
    sMenu->nextLoadId = 1;
    sMenu->prevLoadId = 2;

    nextSelection = sMenu->info.curSelection + 1;
    if (nextSelection >= sMenu->info.numSelections)
        nextSelection = 0;

    prevSelection = sMenu->info.curSelection - 1;
    if (prevSelection < 0)
        prevSelection = sMenu->info.numSelections - 1;

    LoadMonInfo(nextSelection, 1);
    LoadMonInfo(prevSelection, 2);
}

static void LoadMonInfo(s16 partyId, u8 loadId) {
    u8 boxId = sMenu->party[partyId].boxId;
    u8 monId = sMenu->party[partyId].monId;
    u8 numSelections = sMenu->info.numSelections;
    bool8 excludesCancel = FALSE; // whether or not numSelections excludes Cancel from the count

    GetConditionMenuMonNameAndLocString(sMenu->locationStrings[loadId], sMenu->monNameStrings[loadId], boxId, monId,
                                        partyId, numSelections, excludesCancel);
    GetConditionMenuMonConditions(&sMenu->graph, sMenu->numSparkles, boxId, monId, partyId, loadId, numSelections,
                                  excludesCancel);
    GetConditionMenuMonGfx(sMenu->partySheets[loadId], sMenu->partyPalettes[loadId], boxId, monId, partyId,
                           numSelections, excludesCancel);
}

static void UpdateMonPic(u8 loadId) {
    u8 spriteId;
    struct SpriteTemplate spriteTemplate;
    struct SpriteSheet spriteSheet;
    struct SpritePalette spritePal;

    if (sMenu->curMonSpriteId == 0xFF) {
        LoadConditionMonPicTemplate(&spriteSheet, &spriteTemplate, &spritePal);
        spriteSheet.data = sMenu->partySheets[loadId];
        spritePal.data = sMenu->partyPalettes[loadId];
        sMenu->curMonPalette = LoadSpritePalette(&spritePal);
        sMenu->curMonSheet = LoadSpriteSheet(&spriteSheet);
        spriteId = CreateSprite(&spriteTemplate, 38, 104, 0);
        sMenu->curMonSpriteId = spriteId;
        if (spriteId == MAX_SPRITES) {
            FreeSpriteTilesByTag(TAG_CONDITION_MON);
            FreeSpritePaletteByTag(TAG_CONDITION_MON);
            sMenu->curMonSpriteId = 0xFF;
        } else {
            sMenu->curMonSpriteId = spriteId;
            gSprites[sMenu->curMonSpriteId].callback = SpriteCB_MonPic;
            gSprites[sMenu->curMonSpriteId].pos2.y -= 34;
            sMenu->curMonTileStart = (void*)(OBJ_VRAM0 + (sMenu->curMonSheet * 32));
            sMenu->curMonPalette = (sMenu->curMonPalette * 16) + 0x100;
        }
    } else {
        do {
        } while (0); // Only needed to match, feel free to remove.
        DmaCopy16Defvars(3, sMenu->partySheets[loadId], sMenu->curMonTileStart, 0x800);
        LoadPalette(sMenu->partyPalettes[loadId], sMenu->curMonPalette, 32);
    }
}

static void LoadAndCreateSelectionIcons(void) {
    u16 i, spriteId;
    struct SpriteSheet spriteSheets[4];
    struct SpriteTemplate spriteTemplate;
    struct SpritePalette spritePals[3];
    struct SpriteSheet spriteSheet2;
    struct SpritePalette spritePal2;

    LoadConditionSelectionIcons(spriteSheets, &spriteTemplate, spritePals);
    LoadSpriteSheets(spriteSheets);
    LoadSpritePalettes(spritePals);

    // Fill pokeball selection icons up to number in party
    for (i = 0; i < sMenu->info.numSelections - 1; i++) {
        spriteId = CreateSprite(&spriteTemplate, 226, (i * 20) + 8, 0);
        if (spriteId != MAX_SPRITES) {
            sMenu->selectionIconSpriteIds[i] = spriteId;
            gSprites[spriteId].data[0] = i;
            gSprites[spriteId].callback = SpriteCB_SelectionIconPokeball;
        } else {
            sMenu->selectionIconSpriteIds[i] = -1;
        }
    }

    // Fill placeholder icons for remaining (empty) party slots
    spriteTemplate.tileTag = TAG_CONDITION_BALL_PLACEHOLDER;
    for (; i < PARTY_SIZE; i++) {
        spriteId = CreateSprite(&spriteTemplate, 230, (i * 20) + 8, 0);
        if (spriteId != MAX_SPRITES) {
            sMenu->selectionIconSpriteIds[i] = spriteId;
            gSprites[spriteId].oam.size = 0;
        } else {
            sMenu->selectionIconSpriteIds[i] = -1;
        }
    }

    // Add cancel selection icon at bottom
    spriteTemplate.tileTag = TAG_CONDITION_CANCEL;
    spriteTemplate.callback = SpriteCB_SelectionIconCancel;
    spriteId = CreateSprite(&spriteTemplate, 222, (i * 20) + 8, 0);
    if (spriteId != MAX_SPRITES) {
        sMenu->selectionIconSpriteIds[i] = spriteId;
        gSprites[spriteId].oam.shape = SPRITE_SHAPE(32x16);
        gSprites[spriteId].oam.size = SPRITE_SIZE(32x16);
    } else {
        sMenu->selectionIconSpriteIds[i] = -1;
    }

    LoadConditionSparkle(&spriteSheet2, &spritePal2);
    LoadSpriteSheet(&spriteSheet2);
    LoadSpritePalette(&spritePal2);
}

static bool8 LoadUsePokeblockMenuGfx(void) {
    switch (sMenu->info.helperState) {
        case 0:
            ChangeBgX(0, 0, 0);
            ChangeBgY(0, 0, 0);
            ChangeBgX(1, 0, 0);
            ChangeBgY(1, 0, 0);
            ChangeBgX(2, 0, 0);
            ChangeBgY(2, 0, 0);
            ChangeBgX(3, 0, 0);
            ChangeBgY(3, 136 << 6, 0);
            SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1);
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(11, 4));
            break;
        case 1:
            sGraph_Gfx = Alloc(6656);
            sGraph_Tilemap = Alloc(1280);
            sMonFrame_TilemapPtr = Alloc(1280);
            break;
        case 2:
            LZ77UnCompVram(sMonFrame_Tilemap, sMonFrame_TilemapPtr);
            break;
        case 3:
            LoadBgTiles(3, sMonFrame_Gfx, 224, 0);
            break;
        case 4:
            LoadBgTilemap(3, sMonFrame_TilemapPtr, 1280, 0);
            break;
        case 5:
            LoadPalette(sMonFrame_Pal, 208, 32);
            sMenu->curMonXOffset = -80;
            break;
        case 6:
            LZ77UnCompVram(gUsePokeblockGraph_Gfx, sGraph_Gfx);
            break;
        case 7:
            LZ77UnCompVram(gUsePokeblockGraph_Tilemap, sGraph_Tilemap);
            LoadPalette(gUsePokeblockGraph_Pal, 32, 32);
            break;
        case 8:
            LoadBgTiles(1, sGraph_Gfx, 6656, 160 << 2);
            break;
        case 9:
            SetBgTilemapBuffer(1, sGraph_Tilemap);
            CopyToBgTilemapBufferRect(1, gUsePokeblockNatureWin_Pal, 0, 13, 12, 4);
            CopyBgTilemapBufferToVram(1);
            break;
        case 10:
            LZ77UnCompVram(sGraphData_Tilemap, sMenu->tilemapBuffer);
            break;
        case 11:
            LoadBgTilemap(2, sMenu->tilemapBuffer, 1280, 0);
            LoadPalette(gConditionGraphData_Pal, 48, 32);
            LoadPalette(gConditionText_Pal, 240, 32);
            sub_81D21DC(2);
            break;
        default:
            sMenu->info.helperState = 0;
            return FALSE;
    }

    sMenu->info.helperState++;
    return TRUE;
}

static void UpdateMonInfoText(u16 loadId, bool8 firstPrint) {
    u8 partyIndex;
    u8 nature;
    u8* str;

    FillWindowPixelBuffer(WIN_NAME, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_NATURE, PIXEL_FILL(0));
    if (sMenu->info.curSelection != sMenu->info.numSelections - 1) {
        AddTextPrinterParameterized(WIN_NAME, 1, sMenu->monNameStrings[loadId], 0, 1, 0, NULL);
        partyIndex = GetPartyIdFromSelectionId(sMenu->info.curSelection);
        nature = GetNature(&gPlayerParty[partyIndex]);
        str = StringCopy(sMenu->info.natureText, gText_NatureSlash);
        str = StringCopy(str, gNatureNamePointers[nature]);
        AddTextPrinterParameterized3(WIN_NATURE, 1, 2, 1, sNatureTextColors, 0, sMenu->info.natureText);
    }

    if (firstPrint) {
        CopyWindowToVram(WIN_NAME, 3);
        CopyWindowToVram(WIN_NATURE, 3);
    } else {
        CopyWindowToVram(WIN_NAME, 2);
        CopyWindowToVram(WIN_NATURE, 2);
    }
}

static void UpdateSelection(bool8 up) {
    u16 newLoadId;
    bool32 startedOnMon, endedOnMon;

    if (up)
        newLoadId = sMenu->prevLoadId;
    else
        newLoadId = sMenu->nextLoadId;

    sub_81D1F84(&sMenu->graph, sMenu->graph.unk14[sMenu->curLoadId], sMenu->graph.unk14[newLoadId]);

    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        startedOnMon = FALSE; // moving off of Cancel
    else
        startedOnMon = TRUE;

    if (up) {
        sMenu->prevLoadId = sMenu->nextLoadId; // temporarily store nextLoadId, prevLoadId no longer needed
        sMenu->nextLoadId = sMenu->curLoadId;
        sMenu->curLoadId = newLoadId;
        sMenu->toLoadId = sMenu->prevLoadId; // next load will be the mon that's one up from new selection

        // Check for wrap to bottom of list
        sMenu->info.curSelection =
            (sMenu->info.curSelection == 0) ? sMenu->info.numSelections - 1 : sMenu->info.curSelection - 1;

        sMenu->toLoadSelection =
            (sMenu->info.curSelection == 0) ? sMenu->info.numSelections - 1 : sMenu->info.curSelection - 1;
    } else {
        sMenu->nextLoadId = sMenu->prevLoadId; // temporarily store prevLoadId, nextLoadId no longer needed
        sMenu->prevLoadId = sMenu->curLoadId;
        sMenu->curLoadId = newLoadId;
        sMenu->toLoadId = sMenu->nextLoadId; // next load will be the mon that's one down from new selection

        // Check for wrap to top of list
        sMenu->info.curSelection =
            (sMenu->info.curSelection < sMenu->info.numSelections - 1) ? sMenu->info.curSelection + 1 : 0;

        sMenu->toLoadSelection =
            (sMenu->info.curSelection < sMenu->info.numSelections - 1) ? sMenu->info.curSelection + 1 : 0;
    }

    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        endedOnMon = FALSE; // moving onto Cancel
    else
        endedOnMon = TRUE;

    DestroyConditionSparkleSprites(sMenu->sparkles);

    if (!startedOnMon)
        sMenu->info.loadNewSelection = LoadNewSelection_CancelToMon;
    else if (!endedOnMon)
        sMenu->info.loadNewSelection = LoadNewSelection_MonToCancel;
    else
        sMenu->info.loadNewSelection = LoadNewSelection_MonToMon;
}

static bool8 LoadNewSelection_CancelToMon(void) {
    switch (sMenu->info.helperState) {
        case 0:
            UpdateMonPic(sMenu->curLoadId);
            sMenu->info.helperState++;
            break;
        case 1:
            UpdateMonInfoText(sMenu->curLoadId, FALSE);
            sMenu->info.helperState++;
            break;
        case 2:
            if (!TryUpdateConditionMonTransitionOn(&sMenu->graph, &sMenu->curMonXOffset)) {
                // Load the new adjacent pokemon (not the one being shown)
                LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
                sMenu->info.helperState++;
            }
            break;
        case 3:
            ResetConditionSparkleSprites(sMenu->sparkles);
            if (sMenu->info.curSelection != sMenu->info.numSelections - 1) {
                u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
                CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
            }

            sMenu->info.helperState = 0;
            return FALSE;
    }

    return TRUE;
}

static bool8 LoadNewSelection_MonToCancel(void) {
    switch (sMenu->info.helperState) {
        case 0:
            if (!TryUpdateConditionMonTransitionOff(&sMenu->graph, &sMenu->curMonXOffset))
                sMenu->info.helperState++;
            break;
        case 1:
            UpdateMonInfoText(sMenu->curLoadId, FALSE);
            sMenu->info.helperState++;
            break;
        case 2:
            LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
            sMenu->info.helperState++;
            break;
        case 3:
            sMenu->info.helperState = 0;
            return FALSE;
    }

    return TRUE;
}

static bool8 LoadNewSelection_MonToMon(void) {
    switch (sMenu->info.helperState) {
        case 0:
            TransitionConditionGraph(&sMenu->graph);
            if (!MoveConditionMonOffscreen(&sMenu->curMonXOffset)) {
                UpdateMonPic(sMenu->curLoadId);
                sMenu->info.helperState++;
            }
            break;
        case 1:
            UpdateMonInfoText(sMenu->curLoadId, FALSE);
            sMenu->info.helperState++;
            break;
        case 2:
            if (!TryUpdateConditionMonTransitionOn(&sMenu->graph, &sMenu->curMonXOffset)) {
                // Load the new adjacent pokemon (not the one being shown)
                LoadMonInfo(sMenu->toLoadSelection, sMenu->toLoadId);
                sMenu->info.helperState++;
            }
            break;
        case 3:
            ResetConditionSparkleSprites(sMenu->sparkles);
            if (sMenu->info.curSelection != sMenu->info.numSelections - 1) {
                u8 numSparkles = sMenu->numSparkles[sMenu->curLoadId];
                CreateConditionSparkleSprites(sMenu->sparkles, sMenu->curMonSpriteId, numSparkles);
            }

            sMenu->info.helperState = 0;
            return FALSE;
    }

    return TRUE;
}

static void SpriteCB_MonPic(struct Sprite* sprite) {
    sprite->pos1.x = sMenu->curMonXOffset + 38;
}

static void SpriteCB_SelectionIconPokeball(struct Sprite* sprite) {
    if (sprite->data[0] == sMenu->info.curSelection)
        StartSpriteAnim(sprite, 0);
    else
        StartSpriteAnim(sprite, 1);
}

static void SpriteCB_SelectionIconCancel(struct Sprite* sprite) {
    if (sMenu->info.curSelection == sMenu->info.numSelections - 1)
        sprite->oam.paletteNum = IndexOfSpritePaletteTag(TAG_CONDITION_BALL);
    else
        sprite->oam.paletteNum = IndexOfSpritePaletteTag(TAG_CONDITION_CANCEL);
}

// Calculate the max id for sparkles/stars that appear around the pokemon on the condition screen
// All pokemon start with 1 sparkle (added by CreateConditionSparkleSprites), so the number here +1
// is the total number of sparkles that appear
static void CalculateNumAdditionalSparkles(u8 monIndex) {
    u8 sheen = GetMonData(&gPlayerParty[monIndex], MON_DATA_SHEEN);

    sMenu->numSparkles[sMenu->curLoadId] =
        (sheen != 255) ? sheen / (255 / (MAX_CONDITION_SPARKLES - 1) + 1) : MAX_CONDITION_SPARKLES - 1;
}

static void LoadConditionGfx(void) {
    struct CompressedSpriteSheet spriteSheet;
    struct SpritePalette spritePalette;

    spritePalette = sSpritePalette_Condition;
    spriteSheet.data = gUsePokeblockCondition_Gfx;
    spriteSheet.size = 0x800;
    spriteSheet.tag = TAG_CONDITION;
    LoadCompressedSpriteSheet(&spriteSheet);
    LoadSpritePalette(&spritePalette);
}

static void CreateConditionSprite(void) {
    u16 i;
    s16 xDiff, xStart;
    int yStart = 17;
    int var = 8;
    struct Sprite** sprites = sMenu->condition;
    const struct SpriteTemplate* template = &sSpriteTemplate_Condition;

    for (i = 0, xDiff = 64, xStart = -96; i < 2; i++) {
        u8 spriteId = CreateSprite(template, i * xDiff + xStart, yStart, 0);
        if (spriteId != MAX_SPRITES) {
            gSprites[spriteId].data[0] = var;
            gSprites[spriteId].data[1] = (i * xDiff) | 0x20;
            gSprites[spriteId].data[2] = i;
            StartSpriteAnim(&gSprites[spriteId], i);
            sprites[i] = &gSprites[spriteId];
        }
    }
}

static bool8 LoadConditionTitle(void) {
    switch (sMenu->info.helperState) {
        case 0:
            LoadConditionGfx();
            sMenu->info.helperState++;
            return TRUE;
        case 1:
            CreateConditionSprite();
            sMenu->info.helperState = 0;
            return FALSE;
    }

    return FALSE;
}

// Literally the word "Condition", the title block that appears over the mon icon
static void SpriteCB_Condition(struct Sprite* sprite) {
    s16 prevX = sprite->pos1.x;

    sprite->pos1.x += sprite->data[0];
    if ((prevX <= sprite->data[1] && sprite->pos1.x >= sprite->data[1]) ||
        (prevX >= sprite->data[1] && sprite->pos1.x <= sprite->data[1])) {
        sprite->pos1.x = sprite->data[1];
        sprite->callback = SpriteCallbackDummy;
    }
}
