#pragma once

#include <raylib.h>

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

#define AI_DIST_OBSTACLE 254
#define AI_DIST_INFINITY 255

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

bool IsTilePit(int x, int y);
bool IsPositionInPit(Vector2 position);
