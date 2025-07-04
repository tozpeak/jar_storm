#include <stdlib.h>
#include <math.h>

#include <level.h>
#include <components.h>
#include <spawners.h>
#include <ecs_helpers.h>

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
AIMapTile   aiMap       [ LVL_WIDTH * LVL_HEIGHT ] = { 0 };
AIMapTile   outOfBoundsAITile = {
    .aiDistance = AI_DIST_OBSTACLE,
};

LevelSettings g_level = {
    .width = LVL_WIDTH,
    .height = LVL_HEIGHT,
    .tileSize = { 16, 16 },
    .tiles = levelTiles,
    .aiMap = aiMap,
};

Vector2Int GetTilePosition(Vector2 worldPosition)
{
    return (Vector2Int) {
        (int)floor(worldPosition.x / g_level.tileSize.x),
        (int)floor(worldPosition.y / g_level.tileSize.y)
    };
}

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

bool IsTilePitV(Vector2Int tPosition)
{
    return IsTilePit(tPosition.x, tPosition.y);
}

bool IsPositionInPit(Vector2 position)
{
    return IsTilePitV( GetTilePosition(position) );
}

AIMapTile *Level_GetAITileForTilePos(Vector2Int tPos)
{
    if(tPos.x < 0 || tPos.y < 0) return &outOfBoundsAITile;
    if(tPos.x >= g_level.width || tPos.y >= g_level.height) return &outOfBoundsAITile;
    
    return g_level.aiMap + tPos.x + tPos.y * g_level.width;
}

AIMapTile *Level_GetAITileForWorldPos(Vector2 pos)
{
    return Level_GetAITileForTilePos( GetTilePosition(pos) );
}

bool Level_RandomFreePosInRect(Rectangle* rect, int attempts, Vector2 *result)
{
    for (int i = 0; i < attempts; i++) {
        *result = (Vector2) {
            rand() % (int)round(rect->width) + rect->x,
            rand() % (int)round(rect->height) + rect->y,
        };
        AIMapTile *tile = Level_GetAITileForWorldPos( *result );
        if ( tile->aiDistance <= AI_DIST_EMPTY ) return true;
    }
    return false;
}

void FillAIObstacleByRadius(Vector2 worldPos, float radius)
{
    Vector2 tileSize = g_level.tileSize;
    Vector2Int tPos = GetTilePosition(worldPos);
    AIMapTile *tile = Level_GetAITileForTilePos(tPos);
    tile->aiDistance = AI_DIST_OBSTACLE;
    
    int tRadius = 2 + (int)ceil(
        radius
        / fmin( tileSize.x, tileSize.y )
    );
    
    for (int i = tPos.x - tRadius; i < tPos.x + tRadius; i++) {
        for (int j = tPos.y - tRadius; j < tPos.y + tRadius; j++) {
            Vector2 checkPos = {
                (i + 0.5f) * g_level.tileSize.x,
                (j + 0.5f) * g_level.tileSize.y,
            };
            
            if ( Vector2Distance (checkPos, worldPos) > radius ) continue;
            
            tile = Level_GetAITileForTilePos( (Vector2Int) { i, j } );
            tile->aiDistance = AI_DIST_OBSTACLE;
        }
    }
}

void Level_Generate()
{
    int pillarCount = 6 * 4;
    int interactableCount = 48;

    Vector2 tileSize = g_level.tileSize;
    Rectangle levelRect = {
        0,
        0,
        g_level.width * tileSize.x,
        g_level.height * tileSize.y,
    };
    
    for (int x = 0; x < g_level.width; x++) {
        for (int y = 0; y < g_level.height; y++) {
            AIMapTile* tile = g_level.aiMap + x + y * g_level.width;
            tile->aiDistance = ( IsTilePit(x, y) ) ? AI_DIST_OBSTACLE : AI_DIST_EMPTY;
        }
    }

    Spawn_Teleporter((Vector2) { levelRect.width - 32, levelRect.height - 32 } );
    //Spawn_RandomItem((Vector2) { 256, 256 } );
    
    Vector2 randomPos;
    
    for (int i = 0; i < pillarCount; i++) {
        if( !Level_RandomFreePosInRect(&levelRect, 10, &randomPos) ) continue;
        uint32_t entPillar = Spawn_Pillar( randomPos );
        ECS_GET_NEW(col, entPillar, Collider);
        
        FillAIObstacleByRadius( randomPos, col->shape.circle.radius );
    }
    
    for (int i = 0; i < interactableCount; i++) {
        if( !Level_RandomFreePosInRect(&levelRect, 10, &randomPos) ) continue;
        Spawn_Interactable( randomPos );
    }
    
    /*for (int i = 0; i < 4; i++) {
        Spawn_Interactable( (Vector2) { 96 + i * 4, 96 } );
    }*/
}
