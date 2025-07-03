#include <math.h>

#include <level.h>

Camera2D mainCamera = { 0 };
const ScreenSettings g_screenSettings = { 
    .width = 640, 
    .height = 480,
    .tileSize = { 16, 16 },
    .marginTiles = 0,
    .marginTopTiles = 0,
    .levelScale = 2,
    .camera = &mainCamera,
};

bool IsTilePit(int x, int y)
{
    if (x < 0) return true;
    if (x >= (g_screenSettings.width / g_screenSettings.tileSize.x) * g_screenSettings.levelScale) return true;
    if (y < 0) return true;
    if (y >= (g_screenSettings.height / g_screenSettings.tileSize.y) * g_screenSettings.levelScale) return true;
    
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
        (int)floor(position.x / g_screenSettings.tileSize.x),
        (int)floor(position.y / g_screenSettings.tileSize.y)
    );
}
