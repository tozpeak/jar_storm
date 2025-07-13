#include <stdio.h>
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
    
    return g_level.tiles[ LVL_INDEX (x,y) ].type < TLT_WALKABLE;
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

void Level_LoadFromFile()
{
    FILE *file = fopen("res/test.pbm", "rb");
    
    char line[16];
    int c;
    int w, h;
    
    TileInfo *level = g_level.tiles;
    
    if (file != NULL) {
        //skip magic line (P4)
        if( !fgets(line, sizeof(line), file) ) return;
        while ( fgetc(file) == '#' ) {
            //if comment line
            //skip line
            while ( true ) {
                c = fgetc(file);
                if (c == EOF) return;
                if (c == '\n') break;
            }
        }
        //rewind first not-comment line symbol
        fseek(file, -1, SEEK_CUR);
        
        if( !fgets(line, sizeof(line), file) ) return;
        
        //read data size
        int res = sscanf(line, "%d %d", &w, &h);
        
        if (w != g_level.width
            || h != g_level.height) return;
        
        //suffixes: B = Bytes, b = bits
        //we need to iterate each line byte by bit
        int wB = (int)ceil(w / 8.f);
        
        for (int y = 0; y < h; ++y) {
            for (int xB = 0; xB < wB; ++xB) {
                c = fgetc(file);
                if(c == EOF) return;
                
                for (int xb = 0; xb < 8; ++xb) {
                    int x = xB*8 + xb;
                    if(x >= w) break;
                    
                    unsigned char mask = 1 << ( 7-xb );
                    level[ LVL_INDEX(x,y) ].type =
                        ( (c & mask) == mask )
                        ? TLT_PIT
                        : TLT_FLOOR;
                }
            }
        }
        
        fclose(file);
    }
}

void Level_SetSpawnPoint()
{
    for(int i = 0; i < g_level.width; i++) {
        for(int j = 0; j < g_level.height; j++) {
            if (g_level.tiles[ LVL_INDEX(i,j) ].type >= TLT_WALKABLE) {
                g_level.spawnPoint = g_level.tileSize;
                g_level.spawnPoint.x *= i + 0.5f;
                g_level.spawnPoint.y *= j + 0.5f;
                return;
            }
        }
    }
}

void Level_GenerateTiles()
{
    char newTile;
    for(int i = 0; i < g_level.width; i++) {
        for(int j = 0; j < g_level.height; j++) {
            newTile = TLT_FLOOR;
            
            if(
                ((i % 32) >= 30)
                && ((j % 12) < 10)
            ) newTile = TLT_PIT;
            if(
                ((i % 32) >= 30)
                && ((j % 12) < 10)
            ) newTile = TLT_PIT;
            
            g_level.tiles[ LVL_INDEX(i,j) ].type = newTile;
        }
    }
}

void Level_GenerateEntities()
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

void SpawnEntireFieldOfEnemies()
{
    for (int i = 1; i < 10; i++) {
        for (int j = 1; j < 8; j++) {
            Vector2 position = { i * 16 * 4, j * 16 * 4 };
            bool even = (i % 2 == 0) && (j % 2 == 0);
            if (even)
                Spawn_Enemy_Lizard(position);
            else
                Spawn_Enemy(position);
        }
    }
}

void Level_Setup()
{
    Level_LoadFromFile();
    //Level_GenerateTiles();
    Level_SetSpawnPoint();
    Level_GenerateEntities();
    
    //SpawnEntireFieldOfEnemies();
    
}
