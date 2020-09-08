#include "global.h"
#include "battle_pyramid.h"
#include "bg.h"
#include "fieldmap.h"
#include "fldeff.h"
#include "fldeff_misc.h"
#include "frontier_util.h"
#include "menu.h"
#include "mirage_tower.h"
#include "overworld.h"
#include "palette.h"
#include "pokenav.h"
#include "script.h"
#include "secret_base.h"
#include "trainer_hill.h"
#include "tv.h"
#include "constants/rgb.h"

struct ConnectionFlags
{
    u8 south:1;
    u8 north:1;
    u8 west:1;
    u8 east:1;
};

// Stores local map data, including border data
EWRAM_DATA static u16 sBackupMapData[MAX_MAP_DATA_SIZE] = {0};
EWRAM_DATA struct MapHeader gMapHeader = {0};
EWRAM_DATA struct Camera gCamera = {0};
EWRAM_DATA static struct ConnectionFlags sMapConnectionFlags = {0};
EWRAM_DATA static u32 sFiller_02037344 = 0; // without this, the next file won't align properly

struct BackupMapLayout gBackupMapLayout;

static const struct ConnectionFlags sDummyConnectionFlags = {0};

static void InitMapLayoutData(struct MapHeader *mapHeader);
static void InitBackupMapData(u16 *, u16, u16);
static void StoreMapInBackup(s32, s32, struct MapHeader const *, s32, s32, s32, s32);
static void FillSouthConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void FillNorthConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void FillWestConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void FillEastConnection(struct MapHeader const *, struct MapHeader const *, s32);
static void InitBackupMapLayoutConnections(struct MapHeader *);
static void LoadSavedMapView(void);
static void sub_80885C4(u8);
static s32 GetConnectionTypeWithOffset(s32, s32);
static struct MapConnection *GetConnection(u8, s32, s32);
static void MovePlayerToNewMap(struct MapConnection *, s32, s32, s32);
static bool8 IntersectsConnOnMap(u8, s32, s32, struct MapConnection *);
static bool8 InRangeOnMap(s32, s32, s32, s32);
static bool32 InRange(s32, s32);
static bool32 IntersectsConn(struct MapConnection *, s32, s32);
static bool8 SkipCopyingMetatileFromSavedMap(u16*, u16, u8);

struct MapHeader const *const GetMapHeaderFromConnection(struct MapConnection *connection)
{
    return Overworld_GetMapHeaderByGroupAndId(connection->mapGroup, connection->mapNum);
}

void InitMap(void)
{
    InitMapLayoutData(&gMapHeader);
    SetOccupiedSecretBaseEntranceMetatiles(gMapHeader.events);
    RunOnLoadMapScript();
}

void InitMapFromSavedGame(void)
{
    InitMapLayoutData(&gMapHeader);
    InitSecretBaseAppearance(FALSE);
    SetOccupiedSecretBaseEntranceMetatiles(gMapHeader.events);
    LoadSavedMapView();
    RunOnLoadMapScript();
    UpdateTVScreensOnMap(gBackupMapLayout.width, gBackupMapLayout.height);
}

void InitBattlePyramidMap(bool8 setPlayerPosition)
{
    CpuFastFill(0x03ff03ff, sBackupMapData, sizeof(sBackupMapData));
    GenerateBattlePyramidFloorLayout(sBackupMapData, setPlayerPosition);
}

void InitTrainerHillMap(void)
{
    CpuFastFill(0x03ff03ff, sBackupMapData, sizeof(sBackupMapData));
    GenerateTrainerHillFloorLayout(sBackupMapData);
}

static void InitMapLayoutData(struct MapHeader *mapHeader)
{
    struct MapLayout const *mapLayout;
    s32 width;
    s32 height;
    mapLayout = mapHeader->mapLayout;
    CpuFastFill16(0x03ff, sBackupMapData, sizeof(sBackupMapData));
    gBackupMapLayout.map = sBackupMapData;
    width = mapLayout->width + 2 * MAP_BORDER + 1;
    gBackupMapLayout.width = width;
    height = mapLayout->height + 2 * MAP_BORDER;
    gBackupMapLayout.height = height;
    if (width * height <= MAX_MAP_DATA_SIZE)
    {
        InitBackupMapData(mapLayout->map, mapLayout->width, mapLayout->height);
        InitBackupMapLayoutConnections(mapHeader);
    }
}

static void InitBackupMapData(u16 *map, u16 width, u16 height)
{
    u16 *dest;
    s32 y;
    dest = gBackupMapLayout.map;
    
    // Skip top border and left border of first row
    dest += gBackupMapLayout.width * MAP_BORDER + MAP_BORDER;
    for (y = 0; y < height; y++)
    {
        CpuCopy16(map, dest, width * sizeof(u16));
        
        // Skip right border and left border of next row
        dest += width + 2 * MAP_BORDER + 1;
        map += width;
    }
}

static void InitBackupMapLayoutConnections(struct MapHeader *mapHeader)
{
    s32 i;
    s32 count;
    s32 offset;
    struct MapConnection *conn;
    struct MapHeader const *nextMapHeader;

    if (mapHeader->connections)
    {
        count = mapHeader->connections->count;
        conn = mapHeader->connections->connections;
        sMapConnectionFlags = sDummyConnectionFlags;
        for (i = 0; i < count; i++, conn++)
        {
            nextMapHeader = GetMapHeaderFromConnection(conn);
            offset = conn->offset;
            switch (conn->type)
            {
            case DIR_SOUTH:
                FillSouthConnection(mapHeader, nextMapHeader, offset);
                sMapConnectionFlags.south = TRUE;
                break;
            case DIR_NORTH:
                FillNorthConnection(mapHeader, nextMapHeader, offset);
                sMapConnectionFlags.north = TRUE;
                break;
            case DIR_WEST:
                FillWestConnection(mapHeader, nextMapHeader, offset);
                sMapConnectionFlags.west = TRUE;
                break;
            case DIR_EAST:
                FillEastConnection(mapHeader, nextMapHeader, offset);
                sMapConnectionFlags.east = TRUE;
                break;
            }
        }
    }
}

static void StoreMapInBackup(s32 backupCol, s32 backupRow, struct MapHeader const *nextMapHeader, s32 mapCol, s32 mapRow, s32 width, s32 height)
{
    s32 i;
    u16 *src;
    u16 *dest;
    s32 mapWidth;

    mapWidth = nextMapHeader->mapLayout->width;
    src = &nextMapHeader->mapLayout->map[mapWidth * mapRow + mapCol];
    dest = &gBackupMapLayout.map[gBackupMapLayout.width * backupRow + backupCol];

    for (i = 0; i < height; i++)
    {
        CpuCopy16(src, dest, width * sizeof(u16));
        dest += gBackupMapLayout.width;
        src += mapWidth;
    }
}

static void FillSouthConnection(struct MapHeader const *mapHeader, struct MapHeader const *nextMapHeader, s32 offset)
{
    int backupCol, backupRow;
    int col;
    int width;
    int nextWidth;

    if (nextMapHeader == NULL)
        return;

    nextWidth = nextMapHeader->mapLayout->width;
    backupCol = offset + MAP_BORDER;
    backupRow = mapHeader->mapLayout->height + MAP_BORDER;

    if (backupCol < 0)
    {
        col = -backupCol;
        backupCol += nextWidth;

        if (backupCol < gBackupMapLayout.width)
            width = backupCol;
        else
            width = gBackupMapLayout.width;

        backupCol = 0;
    }
    else
    {
        col = 0;
        if (backupCol + nextWidth < gBackupMapLayout.width)
            width = nextWidth;
        else
            width = gBackupMapLayout.width - backupCol;
    }

    StoreMapInBackup(
        backupCol, backupRow,
        nextMapHeader,
        col, /*row*/ 0,
        width, /*height*/ MAP_BORDER);
}

static void FillNorthConnection(struct MapHeader const *mapHeader, struct MapHeader const *nextMapHeader, s32 offset)
{
    int backupCol;
    int col, row;
    int width;
    int nextWidth, nextHeight;

    if (nextMapHeader == NULL)
        return;

    nextWidth = nextMapHeader->mapLayout->width;
    nextHeight = nextMapHeader->mapLayout->height;
    backupCol = offset + MAP_BORDER;
    row = nextHeight - MAP_BORDER;
    if (backupCol < 0)
    {
        col = -backupCol;
        backupCol += nextWidth;
 
        if (backupCol < gBackupMapLayout.width)
            width = backupCol;
        else
            width = gBackupMapLayout.width;

        backupCol = 0;
    }
    else
    {
        col = 0;
        if (backupCol + nextWidth < gBackupMapLayout.width)
        {
            width = nextWidth;
        }
        else
        {
            width = gBackupMapLayout.width - backupCol;
        }
    }

    StoreMapInBackup(
        backupCol, /*backupRow*/ 0,
        nextMapHeader,
        col, row,
        width, /*height*/ MAP_BORDER);
}

static void FillWestConnection(struct MapHeader const *mapHeader, struct MapHeader const *nextMapHeader, s32 offset)
{
    int backupRow;
    int col, row;
    int height;
    int nextWidth, nextHeight;

    if (nextMapHeader == NULL)
        return;

    nextWidth = nextMapHeader->mapLayout->width;
    nextHeight = nextMapHeader->mapLayout->height;
    backupRow = offset + MAP_BORDER;
    col = nextWidth - MAP_BORDER;
    if (backupRow < 0)
    {
        row = -backupRow;

        if (backupRow + nextHeight < gBackupMapLayout.height)
            height = backupRow + nextHeight;
        else
            height = gBackupMapLayout.height;

        backupRow = 0;
    }
    else
    {
        row = 0;

        if (backupRow + nextHeight < gBackupMapLayout.height)
            height = nextHeight;
        else
            height = gBackupMapLayout.height - backupRow;
    }

    StoreMapInBackup(
        /*backupCol*/ 0, backupRow,
        nextMapHeader,
        col, row,
        /*width*/ MAP_BORDER, height);
}

static void FillEastConnection(struct MapHeader const *mapHeader, struct MapHeader const *nextMapHeader, s32 offset)
{
    int backupCol, backupRow;
    int row;
    int height;
    int nextHeight;
    
    if (nextMapHeader == NULL)
        return;

    nextHeight = nextMapHeader->mapLayout->height;
    backupCol = mapHeader->mapLayout->width + MAP_BORDER;
    backupRow = offset + MAP_BORDER;
    if (backupRow < 0)
    {
        row = -backupRow;

        if (backupRow + nextHeight < gBackupMapLayout.height)
            height = backupRow + nextHeight;
        else
            height = gBackupMapLayout.height;

        backupRow = 0;
    }
    else
    {
        row = 0;
        if (backupRow + nextHeight < gBackupMapLayout.height)
            height = nextHeight;
        else
            height = gBackupMapLayout.height - backupRow;
    }

    StoreMapInBackup(
        backupCol, backupRow,
        nextMapHeader,
        /*col*/ 0, row,
        /*width*/ MAP_BORDER + 1, height);
}

union Block
{
    struct
    {
        u16 block:10;
        u16 collision:2;
        u16 elevation:4;
    } block;
    u16 value;
};

u8 MapGridGetZCoordAt(int x, int y)
{
    u16 block;
    int i;
    u16 *border;

    if (x >= 0 && x < gBackupMapLayout.width
     && y >= 0 && y < gBackupMapLayout.height)
    {
        block = gBackupMapLayout.map[x + gBackupMapLayout.width * y];
    }
    else
    {
        border = gMapHeader.mapLayout->border;
        i = (x + 1) & 1;
        i += ((y + 1) & 1) * 2;
        block = gMapHeader.mapLayout->border[i];
        block |= METATILE_COLLISION_MASK;
    }

    if (block == METATILE_ID_UNDEFINED)
    {
        return 0;
    }

    return block >> METATILE_ELEVATION_SHIFT;
}

u8 MapGridIsImpassableAt(int x, int y)
{
    u16 block;
    int i;
    u16 *border;

    if (x >= 0 && x < gBackupMapLayout.width
     && y >= 0 && y < gBackupMapLayout.height)
    {
        block = gBackupMapLayout.map[x + gBackupMapLayout.width * y];
    }
    else
    {
        border = gMapHeader.mapLayout->border;
        i = (x + 1) & 1;
        i += ((y + 1) & 1) * 2;
        block = gMapHeader.mapLayout->border[i];
        block |= METATILE_COLLISION_MASK;
    }
    if (block == METATILE_ID_UNDEFINED)
    {
        return 1;
    }
    return (block & METATILE_COLLISION_MASK) >> METATILE_COLLISION_SHIFT;
}

u32 MapGridGetMetatileIdAt(int x, int y)
{
    u16 block;
    int i;
    int j;
    struct MapLayout const *mapLayout;
    u16 *border;
    u16 block2;

    if (x >= 0 && x < gBackupMapLayout.width
     && y >= 0 && y < gBackupMapLayout.height)
    {
        block = gBackupMapLayout.map[x + gBackupMapLayout.width * y];
    }
    else
    {
        mapLayout = gMapHeader.mapLayout;
        i = (x + 1) & 1;
        i += ((y + 1) & 1) * 2;
        block = mapLayout->border[i] | METATILE_COLLISION_MASK;
    }
    if (block == METATILE_ID_UNDEFINED)
    {
        border = gMapHeader.mapLayout->border;
        j = (x + 1) & 1;
        j += ((y + 1) & 1) * 2;
        block2 = gMapHeader.mapLayout->border[j];
        // This OR is completely pointless.
        block2 |= METATILE_COLLISION_MASK;
        return block2 & METATILE_ID_MASK;
    }
    return block & METATILE_ID_MASK;
}

u32 MapGridGetMetatileBehaviorAt(int x, int y)
{
    u16 metatile;
    metatile = MapGridGetMetatileIdAt(x, y);
    return GetBehaviorByMetatileId(metatile) & 0xff;
}

u8 MapGridGetMetatileLayerTypeAt(int x, int y)
{
    u16 metatile;
    metatile = MapGridGetMetatileIdAt(x, y);
    return (GetBehaviorByMetatileId(metatile) & METATILE_ELEVATION_MASK) >> METATILE_ELEVATION_SHIFT;
}

void MapGridSetMetatileIdAt(int x, int y, u16 metatile)
{
    int i;
    if (x >= 0 && x < gBackupMapLayout.width
     && y >= 0 && y < gBackupMapLayout.height)
    {
        i = x + y * gBackupMapLayout.width;
        gBackupMapLayout.map[i] = (gBackupMapLayout.map[i] & METATILE_ELEVATION_MASK) | (metatile & ~METATILE_ELEVATION_MASK);
    }
}

void MapGridSetMetatileEntryAt(int x, int y, u16 metatile)
{
    int i;
    if (x >= 0 && x < gBackupMapLayout.width
     && y >= 0 && y < gBackupMapLayout.height)
    {
        i = x + gBackupMapLayout.width * y;
        gBackupMapLayout.map[i] = metatile;
    }
}

u16 GetBehaviorByMetatileId(u16 metatile)
{
    u16 *attributes;
    if (metatile < NUM_METATILES_IN_PRIMARY)
    {
        attributes = gMapHeader.mapLayout->primaryTileset->metatileAttributes;
        return attributes[metatile];
    }
    else if (metatile < NUM_METATILES_TOTAL)
    {
        attributes = gMapHeader.mapLayout->secondaryTileset->metatileAttributes;
        return attributes[metatile - NUM_METATILES_IN_PRIMARY];
    }
    else
    {
        return 0xFF;
    }
}

void SaveMapView(void)
{
    s32 i, j;
    s32 x, y;
    u16 *mapView;
    s32 width;
    mapView = gSaveBlock1Ptr->mapView;
    width = gBackupMapLayout.width;
    x = gSaveBlock1Ptr->pos.x;
    y = gSaveBlock1Ptr->pos.y;
    for (i = y; i < y + 2 * MAP_BORDER; i++)
    {
        for (j = x; j < x + 2 * MAP_BORDER + 1; j++)
        {
            *mapView++ = sBackupMapData[width * i + j];
        }
    }
}

static bool32 SavedMapViewIsEmpty(void)
{
    u16 i;
    u32 marker = 0;

#ifndef UBFIX
    // BUG: This loop extends past the bounds of the mapView array. Its size is only 0x100.
    for (i = 0; i < 0x200; i++)
        marker |= gSaveBlock1Ptr->mapView[i];
#else
    // UBFIX: Only iterate over 0x100
    for (i = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->mapView); i++)
        marker |= gSaveBlock1Ptr->mapView[i];
#endif


    if (marker == 0)
        return TRUE;
    else
        return FALSE;
}

static void ClearSavedMapView(void)
{
    CpuFill16(0, gSaveBlock1Ptr->mapView, sizeof(gSaveBlock1Ptr->mapView));
}

static void LoadSavedMapView(void)
{
    u8 yMode;
    int i, j;
    int x, y;
    u16 *mapView;
    int width;
    mapView = gSaveBlock1Ptr->mapView;
    if (!SavedMapViewIsEmpty())
    {
        width = gBackupMapLayout.width;
        x = gSaveBlock1Ptr->pos.x;
        y = gSaveBlock1Ptr->pos.y;
        for (i = y; i < y + 14; i++)
        {
            if (i == y && i != 0)
                yMode = 0;
            else if (i == y + 13 && i != gMapHeader.mapLayout->height - 1)
                yMode = 1;
            else
                yMode = 0xFF;

            for (j = x; j < x + 15; j++)
            {
                if (!SkipCopyingMetatileFromSavedMap(&sBackupMapData[j + width * i], width, yMode))
                    sBackupMapData[j + width * i] = *mapView;
                mapView++;
            }
        }
        for (j = x; j < x + 15; j++)
        {
            if (y != 0)
                FixLongGrassMetatilesWindowTop(j, y - 1);
            if (i < gMapHeader.mapLayout->height - 1)
                FixLongGrassMetatilesWindowBottom(j, y + 13);
        }
        ClearSavedMapView();
    }
}

static void sub_80885C4(u8 connType)
{
    int width;
    u16 *mapView;
    int x0, y0;
    int x2, y2;
    u16 *src, *dest;
    int srci, desti;
    int r9, r8;
    int x, y;
    int i, j;
    mapView = gSaveBlock1Ptr->mapView;
    width = gBackupMapLayout.width;
    r9 = 0;
    r8 = 0;
    x0 = gSaveBlock1Ptr->pos.x;
    y0 = gSaveBlock1Ptr->pos.y;
    x2 = 15;
    y2 = 14;
    switch (connType)
    {
    case CONNECTION_NORTH:
        y0 += 1;
        y2 = 13;
        break;
    case CONNECTION_SOUTH:
        r8 = 1;
        y2 = 13;
        break;
    case CONNECTION_WEST:
        x0 += 1;
        x2 = 14;
        break;
    case CONNECTION_EAST:
        r9 = 1;
        x2 = 14;
        break;
    }
    for (y = 0; y < y2; y++)
    {
        i = 0;
        j = 0;
        for (x = 0; x < x2; i++, j++, x++)
        {
            desti = width * (y + y0);
            srci = (y + r8) * 15 + r9;
            src = &mapView[srci + i];
            dest = &sBackupMapData[x0 + desti + j];
            *dest = *src;
        }
    }
    ClearSavedMapView();
}

s32 GetConnectionType(s32 x, s32 y)
{
    struct MapLayout const *mapLayout;
    u16 block, block2;
    s32 i, j;
    if (x >= 0 && x < gBackupMapLayout.width
     && y >= 0 && y < gBackupMapLayout.height)
    {
        i = gBackupMapLayout.width;
        i *= y;
        block = gBackupMapLayout.map[x + i];
        if (block == METATILE_ID_UNDEFINED)
        {
            goto fail;
        }
    }
    else
    {
        mapLayout = gMapHeader.mapLayout;
        j = (x + 1) & 1;
        j += ((y + 1) & 1) * 2;
        block2 = METATILE_COLLISION_MASK | mapLayout->border[j];
        if (block2 == METATILE_ID_UNDEFINED)
        {
            goto fail;
        }
    }
    goto success;
fail:
    return CONNECTION_INVALID;
success:

    if (x >= (gBackupMapLayout.width - MAP_BORDER - 1))
    {
        if (!sMapConnectionFlags.east)
        {
            return CONNECTION_INVALID;
        }
        return CONNECTION_EAST;
    }
    else if (x < MAP_BORDER)
    {
        if (!sMapConnectionFlags.west)
        {
            return CONNECTION_INVALID;
        }
        return CONNECTION_WEST;
    }
    else if (y >= (gBackupMapLayout.height - MAP_BORDER))
    {
        if (!sMapConnectionFlags.south)
        {
            return CONNECTION_INVALID;
        }
        return CONNECTION_SOUTH;
    }
    else if (y < MAP_BORDER)
    { 
        if (!sMapConnectionFlags.north)
        {
            return CONNECTION_INVALID;
        }
        return CONNECTION_NORTH;
    }
    else
    {
        return CONNECTION_NONE;
    }
}

static s32 GetConnectionTypeWithOffset(s32 dx, s32 dy)
{
    return GetConnectionType(gSaveBlock1Ptr->pos.x + MAP_BORDER + dx, gSaveBlock1Ptr->pos.y + MAP_BORDER + dy);
}

bool32 CanCameraMoveInDirection(u32 direction)
{
    s32 x, y;
    x = gSaveBlock1Ptr->pos.x + MAP_BORDER + gDirectionToVectors[direction].x;
    y = gSaveBlock1Ptr->pos.y + MAP_BORDER + gDirectionToVectors[direction].y;
    if (GetConnectionType(x, y) == CONNECTION_INVALID)
    {
        return FALSE;
    }
    return TRUE;
}

static void MovePlayerToNewMap(struct MapConnection *connection, s32 connType, s32 dx, s32 dy)
{
    struct MapHeader const *mapHeader;
    mapHeader = GetMapHeaderFromConnection(connection);
    switch (connType)
    {
    case CONNECTION_EAST:
        gSaveBlock1Ptr->pos.x = -dx;
        gSaveBlock1Ptr->pos.y -= connection->offset;
        break;
    case CONNECTION_WEST:
        gSaveBlock1Ptr->pos.x = mapHeader->mapLayout->width;
        gSaveBlock1Ptr->pos.y -= connection->offset;
        break;
    case CONNECTION_SOUTH:
        gSaveBlock1Ptr->pos.x -= connection->offset;
        gSaveBlock1Ptr->pos.y = -dy;
        break;
    case CONNECTION_NORTH:
        gSaveBlock1Ptr->pos.x -= connection->offset;
        gSaveBlock1Ptr->pos.y = mapHeader->mapLayout->height;
        break;
    }
}

bool8 MovePlayerAndCamera(s32 dx, s32 dy)
{
    u32 connType;
    struct MapConnection *conn;
    s32 xPrev, yPrev;
    gCamera.active = FALSE;

    connType = GetConnectionTypeWithOffset(dx, dy);
 
    // If not in connection, simply move the player
    if (connType + 1 <= CONNECTION_SOUTH) // CONNECTION_NONE
    {
        gSaveBlock1Ptr->pos.x += dx;
        gSaveBlock1Ptr->pos.y += dy;
    }
    else // Move through connection
    {
        SaveMapView();
        ClearMirageTowerPulseBlendEffect();
        xPrev = gSaveBlock1Ptr->pos.x;
        yPrev = gSaveBlock1Ptr->pos.y;
        conn = GetConnection(connType, gSaveBlock1Ptr->pos.x, gSaveBlock1Ptr->pos.y);
        MovePlayerToNewMap(conn, connType, dx, dy);
        LoadMapFromCameraTransition(conn->mapGroup, conn->mapNum);
        gCamera.active = TRUE;
        gCamera.dx = xPrev - gSaveBlock1Ptr->pos.x;
        gCamera.dy = yPrev - gSaveBlock1Ptr->pos.y;
        gSaveBlock1Ptr->pos.x += dx;
        gSaveBlock1Ptr->pos.y += dy;
        sub_80885C4(connType);
    }
    return gCamera.active;
}

static struct MapConnection *GetConnection(u8 direction, s32 x, s32 y)
{
    s32 i, count;
    struct MapConnection *conn;
    count = gMapHeader.connections->count;
    conn = gMapHeader.connections->connections;
    for (i = 0; i < count; i++, conn++)
    {
        if (conn->type == direction && IntersectsConnOnMap(direction, x, y, conn) == TRUE)
            return conn;
    }
    return NULL;
}

// Determines whether the player would hit MapConnection `conn` if they walked
// parallel to direction `dir` from point (x, y).
//
// x- and y-coordinates relative to map content (not border).
static bool8 IntersectsConnOnMap(u8 dir, s32 x, s32 y, struct MapConnection *conn)
{
    struct MapHeader const *mapHeader;
    mapHeader = GetMapHeaderFromConnection(conn);
    switch (dir)
    {
    case DIR_SOUTH:
    case DIR_NORTH:
        return InRangeOnMap(x, gMapHeader.mapLayout->width, mapHeader->mapLayout->width, conn->offset);
    case DIR_WEST:
    case DIR_EAST:
        return InRangeOnMap(y, gMapHeader.mapLayout->height, mapHeader->mapLayout->height, conn->offset);
    }
    return FALSE;
}

// Like InRange(), but also checks that the position is on the current map.
// offset: spaces right or below current map where next map starts
static bool8 InRangeOnMap(s32 pos, s32 mapLength, s32 nextMapLength, s32 offset)
{
    s32 offset_ = offset;

    if (offset_ < 0)
        offset_ = 0;

    if (nextMapLength + offset < mapLength)
        mapLength = nextMapLength + offset;

    if (offset_ <= pos && pos <= mapLength)
        return TRUE;

    return FALSE;
}

static bool32 InRange(s32 val, s32 max)
{
    if (val >= 0 && val < max)
        return TRUE;

    return FALSE;
}

static bool32 IntersectsConn(struct MapConnection *conn, s32 x, s32 y)
{
    struct MapHeader const *mapHeader;
    mapHeader = GetMapHeaderFromConnection(conn);
    switch (conn->type)
    {
    case CONNECTION_SOUTH:
    case CONNECTION_NORTH:
        return InRange(x - conn->offset, mapHeader->mapLayout->width);
    case CONNECTION_WEST:
    case CONNECTION_EAST:
        return InRange(y - conn->offset, mapHeader->mapLayout->height);
    }
    return FALSE;
}

struct MapConnection *GetConnectionAtCoords(s16 x, s16 y)
{
    int count;
    struct MapConnection *conn;
    int i;
    u8 connType;
    if (!gMapHeader.connections)
        return NULL;

    count = gMapHeader.connections->count;
    conn = gMapHeader.connections->connections;
    for (i = 0; i < count; i++, conn++)
    {
        connType = conn->type;

        // Skip points that aren't in the border (i.e. connection)
        if ((connType == CONNECTION_DIVE || connType == CONNECTION_EMERGE)
            || (connType == CONNECTION_NORTH && y >= MAP_BORDER)
            || (connType == CONNECTION_SOUTH && y < gMapHeader.mapLayout->height + MAP_BORDER)
            || (connType == CONNECTION_WEST && x >= MAP_BORDER)
            || (connType == CONNECTION_EAST && x < gMapHeader.mapLayout->width + MAP_BORDER))
            continue;
        
        if (IntersectsConn(conn, x - MAP_BORDER, y - MAP_BORDER) == TRUE)
            return conn;
    }
    return NULL;
}

void SetCameraFocusCoords(u16 x, u16 y)
{
    gSaveBlock1Ptr->pos.x = x - MAP_BORDER;
    gSaveBlock1Ptr->pos.y = y - MAP_BORDER;
}

void GetCameraFocusCoords(u16 *x, u16 *y)
{
    *x = gSaveBlock1Ptr->pos.x + MAP_BORDER;
    *y = gSaveBlock1Ptr->pos.y + MAP_BORDER;
}

// Unused
static void SetCameraCoords(u16 x, u16 y)
{
    gSaveBlock1Ptr->pos.x = x;
    gSaveBlock1Ptr->pos.y = y;
}

void GetCameraCoords(u16 *x, u16 *y)
{
    *x = gSaveBlock1Ptr->pos.x;
    *y = gSaveBlock1Ptr->pos.y;
}

void MapGridSetMetatileImpassabilityAt(int x, int y, bool32 impassable)
{
    if (x >= 0 && x < gBackupMapLayout.width && y >= 0 && y < gBackupMapLayout.height)
    {
        if (impassable)
            gBackupMapLayout.map[x + gBackupMapLayout.width * y] |= METATILE_COLLISION_MASK;
        else
            gBackupMapLayout.map[x + gBackupMapLayout.width * y] &= ~METATILE_COLLISION_MASK;
    }
}

static bool8 SkipCopyingMetatileFromSavedMap(u16* mapMetatilePtr, u16 mapWidth, u8 yMode)
{
    if (yMode == 0xFF)
        return FALSE;

    if (yMode == 0)
        mapMetatilePtr -= mapWidth;
    else
        mapMetatilePtr += mapWidth;

    if (IsLargeBreakableDecoration(*mapMetatilePtr & METATILE_ID_MASK, yMode) == TRUE)
        return TRUE;
    return FALSE;
}

static void CopyTilesetToVram(struct Tileset const *tileset, u16 numTiles, u16 offset)
{
    if (tileset)
    {
        if (!tileset->isCompressed)
            LoadBgTiles(2, tileset->tiles, numTiles * 32, offset);
        else
            DecompressAndCopyTileDataToVram(2, tileset->tiles, numTiles * 32, offset, 0);
    }
}

static void CopyTilesetToVramUsingHeap(struct Tileset const *tileset, u16 numTiles, u16 offset)
{
    if (tileset)
    {
        if (!tileset->isCompressed)
            LoadBgTiles(2, tileset->tiles, numTiles * 32, offset);
        else
            DecompressAndLoadBgGfxUsingHeap(2, tileset->tiles, numTiles * 32, offset, 0);
    }
}

void nullsub_3(u16 a0, u16 a1)
{

}

void nullsub_90(void)
{

}

void LoadTilesetPalette(struct Tileset const *tileset, u16 destOffset, u16 size)
{
    u16 black = RGB_BLACK;

    if (tileset)
    {
        if (tileset->isSecondary == FALSE)
        {
            LoadPalette(&black, destOffset, 2);
            LoadPalette(((u16*)tileset->palettes) + 1, destOffset + 1, size - 2);
            nullsub_3(destOffset + 1, (size - 2) >> 1);
        }
        else if (tileset->isSecondary == TRUE)
        {
            LoadPalette(((u16*)tileset->palettes) + (NUM_PALS_IN_PRIMARY * 16), destOffset, size);
            nullsub_3(destOffset, size >> 1);
        }
        else
        {
            LoadCompressedPalette((u32*)tileset->palettes, destOffset, size);
            nullsub_3(destOffset, size >> 1);
        }
    }
}

void CopyPrimaryTilesetToVram(struct MapLayout const *mapLayout)
{
    CopyTilesetToVram(mapLayout->primaryTileset, NUM_TILES_IN_PRIMARY, 0);
}

void CopySecondaryTilesetToVram(struct MapLayout const *mapLayout)
{
    CopyTilesetToVram(mapLayout->secondaryTileset, NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY, NUM_TILES_IN_PRIMARY);
}

void CopySecondaryTilesetToVramUsingHeap(struct MapLayout const *mapLayout)
{
    CopyTilesetToVramUsingHeap(mapLayout->secondaryTileset, NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY, NUM_TILES_IN_PRIMARY);
}

static void LoadPrimaryTilesetPalette(struct MapLayout const *mapLayout)
{
    LoadTilesetPalette(mapLayout->primaryTileset, 0, NUM_PALS_IN_PRIMARY * 16 * 2);
}

void LoadSecondaryTilesetPalette(struct MapLayout const *mapLayout)
{
    LoadTilesetPalette(mapLayout->secondaryTileset, NUM_PALS_IN_PRIMARY * 16, (NUM_PALS_TOTAL - NUM_PALS_IN_PRIMARY) * 16 * 2);
}

void CopyMapTilesetsToVram(struct MapLayout const *mapLayout)
{
    if (mapLayout)
    {
        CopyTilesetToVramUsingHeap(mapLayout->primaryTileset, NUM_TILES_IN_PRIMARY, 0);
        CopyTilesetToVramUsingHeap(mapLayout->secondaryTileset, NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY, NUM_TILES_IN_PRIMARY);
    }
}

void LoadMapTilesetPalettes(struct MapLayout const *mapLayout)
{
    if (mapLayout)
    {
        LoadPrimaryTilesetPalette(mapLayout);
        LoadSecondaryTilesetPalette(mapLayout);
    }
}
