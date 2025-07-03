#include <math.h>

#include <level.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

Camera2D mainCamera = { 0 };
const ScreenSettings g_screenSettings = { 
    .width = 640,
    .height = 480,
    .camera = &mainCamera,
};

#define LVL_WIDTH 80
#define LVL_HEIGHT 60

TileInfo    levelTiles  [ LVL_WIDTH * LVL_HEIGHT ];
AIMapTile   aiMap       [ LVL_WIDTH * LVL_HEIGHT ];

LevelSettings g_level = {
    .width = LVL_WIDTH,
    .height = LVL_HEIGHT,
    .tileSize = { 16, 16 },
    .tiles = levelTiles,
    .aiMap = aiMap,
};

bool IsTilePit(int x, int y)
{
    if (x < 0) return true;
    if (x >= g_level.width) return true;
    if (y < 0) return true;
    if (y >= g_level.height) return true;
    
    if (
        ((x % 32) >= 30) 
        && ((y % 12) < 10)
    ) return true;
        
    if (
        ((y % 32) >= 30) 
        && ((x % 12) < 10)
    ) return true;
    
    return false;
}

bool IsPositionInPit(Vector2 position)
{
    return IsTilePit(
        (int)floor(position.x / g_level.tileSize.x),
        (int)floor(position.y / g_level.tileSize.y)
    );
}
