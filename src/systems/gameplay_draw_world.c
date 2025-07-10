#include <stdint.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

#include <components.h>
#include <physics.h>
#include <shapes.h>
#include <level.h>
#include <interactions.h>
#include <ecs_helpers.h>

#include <systems/gameplay_draw_world.h>

void DrawChessboard() 
{
    Vector2 tileSize = g_level.tileSize;
    Color tileColor = DARKGRAY;
    
    for (int i = 0; i < g_level.width; i++) 
    {
        for (int j = 0; j <= g_level.height; j++) 
        {
            tileColor = ((i + j)%2 == 0) ? GRAY : DARKGRAY;
            bool isPit = IsTilePit(i, j);
            if (isPit) tileColor = BACKGROUND_COLOR;
            DrawRectangle(
                i * tileSize.x,
                j * tileSize.y,
                tileSize.x,
                tileSize.y,
                tileColor
            );
            
            if (isPit && !IsTilePit(i, j-1) ) {
                DrawRectangle(
                    i * tileSize.x,
                    j * tileSize.y,
                    tileSize.x,
                    6,
                    (Color){ 40, 40, 40, 255 }
                );
            }
        }
    }
}

void Draw_DebugAiDistance() 
{
    Vector2 tileSize = g_level.tileSize;
    Color
        obstacle = { 255, 0, 0, 65 },
        empty    = { 0, 255, 0, 65 },
        baseDist = { 255, 255, 0, 0 };
    
    Color tileColor;
    
    for (int i = 0; i < g_level.width; i++) 
    {
        for (int j = 0; j < g_level.height; j++) 
        {
            AIMapTile *tileInfo = Level_GetAITileForTilePos( (Vector2Int) { i, j } );
            unsigned char distance = tileInfo->aiDistance;
            if (distance < AI_DIST_EMPTY) {
                tileColor = baseDist;
                tileColor.a = 205 - (distance) * 10;
            }
            else {
                tileColor = (distance == AI_DIST_OBSTACLE) ? obstacle : empty;
            }
            DrawRectangle(
                i * tileSize.x,
                j * tileSize.y,
                tileSize.x,
                tileSize.y,
                tileColor
            );
        }
    }
}

void DrawCharacter (Vector2 position, Vector2 size) 
{
    Vector2 offset = { -size.x / 2, -size.y };
    Color charColor = GREEN;
    
    DrawRectangleV( Vector2Add(position, offset), size, charColor );
}

void DrawGun (Vector2 aimFrom, Vector2 aimDirection) 
{
    float aimLineLength = 12;
    
    DrawLineEx (
        aimFrom,
        Vector2Add( aimFrom, Vector2Scale(aimDirection, aimLineLength) ),
        5,
        DARKGREEN
    );
}

void System_Draw() 
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_DrawShape);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        DrawShapeComponent *shape = (DrawShapeComponent*)ecs_get(qr->list[i], CID_DrawShape);
        
        Shapes_Draw(pos, &shape->shape, shape->color);
    }
}

void System_DrawPlayer()
{
    Vector2 playerSize = { 12, 24 };
    Vector2 aimFromOffset = { 0, -12 }; //{ 0, -playerSize.y / 2 };
    Vector2 aimDirection = { 0, 0 };
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_PlayerId);
    for (i = 0; i < qr->count; ++i) {
        uint32_t playerEntId = qr->list[i];
        
        DynamicVector *playerPos = (DynamicVector*)ecs_get(playerEntId, CID_Position);
        Vector2 pos = playerPos->v2;
        Vector2 dz = Vector2Zero();
        
        dz.y -= playerPos->v3.z;
        pos = Vector2Add(pos, dz);
        
        Vector2 aimTo = GetScreenToWorld2D(
            Vector2Add( GetMousePosition(), dz ),
            *(g_screenSettings.camera)
        );
        Vector2 aimFrom = Vector2Add (pos, aimFromOffset);
        
        aimDirection = Vector2Subtract(aimTo, aimFrom);
        aimDirection = Vector2Normalize(aimDirection);
        
        DrawCharacter(pos, playerSize);
        DrawGun(aimFrom, aimDirection);
    }
}

void System_DrawDebugCollisions()
{
    Color color = RED;
    color.a = 127;
    
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_Position, CID_DrawShape, CID_HasCollisions);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        DrawShapeComponent *shape = (DrawShapeComponent*)ecs_get(qr->list[i], CID_DrawShape);
        
        Shapes_Draw(pos, &shape->shape, color);
    }
}

void System_DrawEnemyHP() 
{
    Vector2 offset = { 0, 3 };
    Vector2 right = { 1, 0 };
    
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_Position, CID_Health, CID_HasHpBar);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        HealthComponent *hp = (HealthComponent*)ecs_get(qr->list[i], CID_Health);
        
        float maxHp = hp->maxHp;
        
        float sizeFactor = 4 * log2(maxHp)/maxHp;
        Vector2 lp = Vector2Add(
            Vector2Add(*pos, offset),
            Vector2Scale(right, - maxHp * sizeFactor / 2)
        );
        Vector2 rpFull = Vector2Add( lp, Vector2Scale(right, maxHp * sizeFactor) );
        Vector2 rpPart = Vector2Add( lp, Vector2Scale(right, hp->hp * sizeFactor) );
        
        DrawLineEx( lp, rpFull, 4, GRAY );
        DrawLineEx( lp, rpPart, 2, RED );
    }
}

void Systems_DrawLoop()
{
    DrawChessboard();
    
    System_Draw();
    System_DrawPlayer();
    Systems_DrawInteractions(); //interactions.h

    System_DrawDebugCollisions();
    //Draw_DebugAiDistance();

    System_DrawEnemyHP();
}
