#pragma once

#include <raylib.h>

typedef struct
{
    int x;
    int y;
} Vector2Int;

typedef struct
{
    int width;
    int height;
    Camera2D *camera;
} ScreenSettings;

typedef struct
{
    char type;
} TileInfo;

#define AI_DIST_EMPTY    254
#define AI_DIST_OBSTACLE 255

typedef struct
{
    unsigned char aiDistance;
} AIMapTile;

typedef struct
{
    int width;
    int height;
    Vector2 tileSize;
    TileInfo* tiles;
    AIMapTile* aiMap;
} LevelSettings;

extern const ScreenSettings g_screenSettings;
extern LevelSettings g_level;

#define LVL_INDEX(x,y) ( (x) + (y) * g_level.width )

Vector2Int GetTilePosition(Vector2 worldPosition);
bool IsTilePit(int x, int y);
bool IsTilePitV(Vector2Int tPosition);
bool IsPositionInPit(Vector2 position);

AIMapTile *Level_GetAITileForTilePos(Vector2Int tPos);
AIMapTile *Level_GetAITileForWorldPos(Vector2 pos);
bool Level_RandomFreePosInRect(Rectangle* rect, int attempts, Vector2 *result);

void Level_Generate();
