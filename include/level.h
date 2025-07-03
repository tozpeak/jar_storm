#pragma once

#include <raylib.h>

typedef struct
{
    int width;
    int height;
    Vector2 tileSize;
    int marginTiles;
    int marginTopTiles;
    int levelScale;
    Camera2D *camera;
} ScreenSettings;
extern const ScreenSettings g_screenSettings;

bool IsTilePit(int x, int y);
bool IsPositionInPit(Vector2 position);
