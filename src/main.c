//#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

#include <components.h>
#include <physics.h>
#include <shapes.h>
#include <spawners.h>

typedef struct
{
    int width;
    int height;
    Camera2D *camera;
} ScreenSettings;

Camera2D mainCamera = { 0 };
const ScreenSettings g_screenSettings = { 
    .width = 640, 
    .height = 480,
    .camera = &mainCamera,
 };

void DrawChessboard() 
{
    int screenX = g_screenSettings.width, screenY = g_screenSettings.height;
    Vector2 tileSize = { 16, 12 };
    int offsetTiles = 1;
    Color tileColor = DARKGRAY;
    for (int i = offsetTiles; (i + offsetTiles) * tileSize.x < screenX; i++) 
    {
        for (int j = 2 + offsetTiles + (i % 2); (j + offsetTiles) * tileSize.y < screenY; j += 2) 
        {
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

void System_Move(float deltaTime) 
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_Velocity);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
        pos->x += vel->x * deltaTime;
        pos->y += vel->y * deltaTime;
        /*if (pos->x > RENDER_WIDTH || pos->x < 0)
            vel->x *= -1;
        if (pos->y > RENDER_HEIGHT || pos->y < 0)
            vel->y *= -1;*/
    }
}


void System_DealDamageNew()
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_DealDamage, CID_HasCollisions);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entA = qr->list[i];
        DealDamageComponent *dam = (DealDamageComponent*)ecs_get(entA, CID_DealDamage);
        HasCollisionsComponent *hc = (HasCollisionsComponent*)ecs_get(entA, CID_HasCollisions);
        
        CollisionIterator iterator = { 0 };
        InitCollisionIterator(&iterator, entA);
        while (TryGetNextCollision(&iterator)) {
            uint32_t entB = iterator.other;
            CollisionData* cData = iterator.collisionData;
            
            if (dam->target & DMG_OTHER
                && ecs_has(entB, CID_Health)) {
                HealthComponent *hp = (HealthComponent*)ecs_get(entB, CID_Health);
                
                hp->hp -= dam->damage;
                if(hp->hp <= 0) ecs_add(entB, CID_IsKilled, NULL);
            }
            
            if (dam->target & DMG_SELF
                && ecs_has(entA, CID_Health))
            {
                HealthComponent *hpDam = (HealthComponent*)ecs_get(entA, CID_Health);
                hpDam->hp -= dam->damage;
                if(hpDam->hp <= 0) ecs_add(entA, CID_IsKilled, NULL);
            }
        }
        
    }
}

void System_EnemyWanderer(float deltaTime)
{
    const int CHANCE_SAMPLE_SIZE = 100000;
    float changeDirProbabilityF = deltaTime * 1 / 5; // once in 5 seconds
    int changeDirProbability = round(CHANCE_SAMPLE_SIZE * changeDirProbabilityF);
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Velocity, CID_IsWanderer);
    for (i = 0; i < qr->count; ++i) {
        if (rand() % CHANCE_SAMPLE_SIZE > changeDirProbability) continue;
        
        VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
        
        VelocityComponent newVel = Vector2Rotate(
            *vel,
            (rand() % 628) / 100.0f
        );
        
        *vel = newVel;
    }
}

void UpdateAbilityCooldown(AttackAbility *ability, float delta)
{
    if (ability->state == ATK_ST_READY) return;
    
    ability->cooldown -= delta;
    if (ability->cooldown < 0)
    {
        ability->cooldown = 0;
        ability->state = ATK_ST_READY;
    }
}

void System_UpdateAttackCooldown(float deltaTime)
{
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_PrimaryAttack);
    for (i = 0; i < qr->count; ++i) {
        PrimaryAttackComponent *primAtt = (PrimaryAttackComponent*) ecs_get(qr->list[i], CID_PrimaryAttack);
        UpdateAbilityCooldown(primAtt, deltaTime);
        
        if (ecs_has(qr->list[i], CID_SecondaryAttack))
        {
            SecondaryAttackComponent *secAtt = (SecondaryAttackComponent*) ecs_get(qr->list[i], CID_SecondaryAttack);
            UpdateAbilityCooldown(secAtt, deltaTime);
        }
    }
}

void System_EvaluateAiAttack()
{
    AttackContext primaryContext = { 0 };
    AttackContext secondaryContext = { 0 };
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_PrimaryAttack, CID_AiAttack);
    for (i = 0; i < qr->count; ++i) {
        AttackIntentionComponent primaryIntention = {
            .isPrimary = true
        };
        primaryContext.entityId = qr->list[i];
        primaryContext.ability = (PrimaryAttackComponent*) ecs_get(qr->list[i], CID_PrimaryAttack);
        primaryContext.intention = &primaryIntention;
        Attack_EvaluateAi( &primaryContext );
        
        secondaryContext.aiEvaluation = 0;
        if (ecs_has(qr->list[i], CID_SecondaryAttack)) {
            AttackIntentionComponent secondaryIntention = {
                .isPrimary = false
            };
            secondaryContext.entityId = qr->list[i];
            secondaryContext.ability = (SecondaryAttackComponent*) ecs_get(qr->list[i], CID_SecondaryAttack);
            secondaryContext.intention = &secondaryIntention;
            Attack_EvaluateAi( &secondaryContext );
        }
        
        AttackContext *bestContext = 
            ( primaryContext.aiEvaluation > secondaryContext.aiEvaluation )
            ? &primaryContext
            : &secondaryContext;
        
        if  ( bestContext->aiEvaluation > 0 ) {
            ecs_add(qr->list[i], CID_AttackIntention, bestContext->intention);
        }
    }
}

void System_PlayerInput()
{
    AttackIntentionComponent intention = { 0 };
    AttackContext context = { .intention = &intention, };
    Vector2 walkInput = Vector2Zero();
    
    Vector2 aimFromOffset = { 0, -12 }; //{ 0, -playerSize.y / 2 };
    Vector2 aimDirection = { 0, 0 };
    float playerSpeed = 16 * 4;
    float shotCooldownState = 0;
    float shotCooldown = 0.25f;
    
    uint32_t i;
    QueryResult *qr = ecs_query(5, CID_Position, CID_PrimaryAttack, CID_SecondaryAttack, CID_PlayerId, CID_PlayerInput);
    for (i = 0; i < qr->count; ++i) {
        uint32_t playerEntId = qr->list[i];
        
        context.entityId = playerEntId;
        
        //PlayerIdComponent playerId = * (PlayerIdComponent*)ecs_get(playerEntId, CID_PlayerId);
        
        walkInput = Vector2Zero();
        
        if (IsKeyDown(KEY_D)) walkInput.x += 1;
        if (IsKeyDown(KEY_A)) walkInput.x -= 1;
        if (IsKeyDown(KEY_W)) walkInput.y -= 1;
        if (IsKeyDown(KEY_S)) walkInput.y += 1;
        
        walkInput = Vector2Scale(
            Vector2Normalize(walkInput),
            playerSpeed
        );
        ecs_add(playerEntId, CID_Velocity, &walkInput);
        
        
        PositionComponent* playerPos = (PositionComponent*)ecs_get(playerEntId, CID_Position);
        
        bool primaryInput = IsMouseButtonDown(0);
        bool secondaryInput = IsMouseButtonDown(1);
        
        if (primaryInput || secondaryInput)
        {
            Vector2 aimTo = GetScreenToWorld2D(GetMousePosition(), *(g_screenSettings.camera));
            Vector2 aimFrom = Vector2Add (*playerPos, aimFromOffset);
            
            aimDirection = Vector2Subtract(aimTo, aimFrom);
            aimDirection = Vector2Normalize(aimDirection);
            
            context.intention->aimAt = aimDirection;
            context.intention->isPrimary = primaryInput;
            
            ecs_add(playerEntId, CID_AttackIntention, context.intention);
        }
        else
        {
            ecs_remove(playerEntId, CID_AttackIntention);
        }
    }
}

void System_PerformAttack()
{
    AttackContext context = { 0 };
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_AttackIntention);
    for (i = 0; i < qr->count; ++i) {
        
        context.entityId = qr->list[i];
        context.intention = (AttackIntentionComponent*) ecs_get(qr->list[i], CID_AttackIntention);
        uint32_t attackComponentId = context.intention->isPrimary ? CID_PrimaryAttack : CID_SecondaryAttack;
        if (! ecs_has(context.entityId, attackComponentId)) continue;
        
        context.ability = (AttackAbility*) ecs_get(qr->list[i], attackComponentId);
        
        if (context.ability->cooldown > 0) continue;
        
        Attack_Perform( &context );
        ecs_remove(qr->list[i], CID_AttackIntention);
    }
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

void System_SpawnRandomUnit(float deltaTime)
{
    const int CHANCE_SAMPLE_SIZE = 100000;
    
    float changeDirProbabilityF = deltaTime * 2 / 5;
    int changeDirProbability = round(CHANCE_SAMPLE_SIZE * changeDirProbabilityF);
    
    if(rand() % CHANCE_SAMPLE_SIZE > changeDirProbability) return;
    
    bool isLizard = (rand() % 4 == 0);
    
    int spawnMargin = 16 * 4;
    
    Vector2 position = {
        .x = ( rand() % (g_screenSettings.width  - 2 * spawnMargin) + spawnMargin ),
        .y = ( rand() % (g_screenSettings.height - 2 * spawnMargin) + spawnMargin ),
    };
    
    if (isLizard)
        Spawn_Enemy_Lizard(position);
    else
        Spawn_Enemy(position);
}

void System_SaveKilledPlayer()
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_IsKilled, CID_PlayerId);
    for (i = 0; i < qr->count; ++i) {
        ecs_remove(qr->list[i], CID_IsKilled);
    }
}

void System_DestroyKilled()
{
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_IsKilled);
    for (i = 0; i < qr->count; ++i) {
        ecs_kill(qr->list[i]);
    }
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
        
        PositionComponent* playerPos = (PositionComponent*)ecs_get(playerEntId, CID_Position);
        
        Vector2 aimTo = GetScreenToWorld2D(GetMousePosition(), *(g_screenSettings.camera));
        Vector2 aimFrom = Vector2Add (*playerPos, aimFromOffset);
        
        aimDirection = Vector2Subtract(aimTo, aimFrom);
        aimDirection = Vector2Normalize(aimDirection);
        
        DrawCharacter(*playerPos, playerSize);
        DrawGun(aimFrom, aimDirection);
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

void System_DrawDebugCollisions()
{
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_Position, CID_DrawShape, CID_HasCollisions);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        DrawShapeComponent *shape = (DrawShapeComponent*)ecs_get(qr->list[i], CID_DrawShape);
        Color color = RED;
        
        Shapes_Draw(pos, &shape->shape, color);
    }
}

void System_KillOutOfBounds() 
{
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_Position);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        if (pos->x > g_screenSettings.width 
         || pos->x < 0
         || pos->y > g_screenSettings.height
         || pos->y < 0
        ) ecs_add(qr->list[i], CID_IsKilled, NULL);
    }
}


void Systems_GameLoop()
{
    float delta = GetFrameTime();
        
    System_Move(delta);
    System_ClearCollisions();
    System_Collide(delta);
    
    System_UpdateAttackCooldown(delta);
    System_EvaluateAiAttack();
    System_PlayerInput();
    System_PerformAttack();
    System_DealDamageNew();
    System_EnemyWanderer(delta);
    System_SpawnRandomUnit(delta);
    
    System_KillOutOfBounds();
    System_SaveKilledPlayer();
    System_DestroyKilled();
}

void Systems_DrawLoop()
{
    DrawChessboard();
    
    System_Draw();
    System_DrawPlayer();

    System_DrawDebugCollisions();

    System_DrawEnemyHP();
}

int test_main();

int main ()
{
    //return test_main();

    // Tell the window to use vsync and work on high DPI displays
    //SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    //SetTargetFPS(120);

    // Create the window and OpenGL context
    const int screenScale = 2;

    InitWindow(g_screenSettings.width * screenScale, g_screenSettings.height * screenScale, "Hello Raylib");
    //SetWindowSize(g_screenSettings.width * screenScale, g_screenSettings.height * screenScale);

    Camera2D *camera = g_screenSettings.camera;
    camera->target = (Vector2){ 0 };
    camera->offset = (Vector2){ 0 };
    camera->rotation = 0.0f;
    camera->zoom = screenScale;


    InitPhysics();
    InitComponents();
    Attack_InitConfig();

    Entity player = Spawn_Player(
        (Vector2) { 32, 32 },
        0
    );
    
    //SpawnEntireFieldOfEnemies();

    // game loop
    while (!WindowShouldClose())        // run the loop untill the user presses ESCAPE or presses the Close button on the window
    {
        Systems_GameLoop();
        
        // drawing
        BeginDrawing();

        // Setup the back buffer for drawing (clear color and depth buffers)
        ClearBackground(BLACK);

        BeginMode2D(*camera);

        Systems_DrawLoop();
        
        EndMode2D();
        
        DrawFPS(1, 1);
        DrawText(
            TextFormat("%d entt", ecs_query(0)->count),
            1, 24,
            16, DARKGREEN
        );
        
        DrawText(
            TextFormat("%d enemies", ecs_query(1, CID_Health)->count),
            1, 24 * 2,
            16, DARKGREEN
        );

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }
    // destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}

int test_main() 
{
    // Create the window and OpenGL context
    InitWindow(g_screenSettings.width, g_screenSettings.height, "Hello Raylib");

    InitPhysics();
    InitComponents();
    Attack_InitConfig();

    Entity e;
    Shape shape;

    //enemy
    e = ecs_create();
    float radius = 12;
    shape = Shapes_NewCircle(Vector2Zero(), 12);
    PositionComponent pos = { 0, 64 };
    VelocityComponent vel = { 16, 0 };
    DrawShapeComponent draw = { WHITE, shape };
    ColliderComponent col = { 
        shape, 
        (Layer)LN_ENEMY
    };
    
    ecs_add(e.id, CID_Position, &pos );
    //ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
    
    //static bullet
    e = ecs_create();
    shape = Shapes_NewLine(Vector2Zero(), (Vector2){ 64, 64 });
    pos = (Vector2) { 64, 64 + 8 };
    draw = (DrawShapeComponent) { GOLD, shape };
    col = (ColliderComponent) { 
        shape, 
        (Layer)LN_PL_BULLET
    };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
    //moving wall
    e = ecs_create();
    shape = Shapes_NewLine(Vector2Zero(), (Vector2){ -16, 128 });
    pos = (Vector2) { 32, 64 };
    vel = (Vector2) { 16, 0 };
    draw = (DrawShapeComponent) { GREEN, shape };
    col = (ColliderComponent) { 
        shape, 
        (Layer)LN_WALL
    };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
    
    while (!WindowShouldClose())        // run the loop untill the user presses ESCAPE or presses the Close button on the window
    {
        Systems_GameLoop();
        
        // drawing
        BeginDrawing();

        // Setup the back buffer for drawing (clear color and depth buffers)
        ClearBackground(BLACK);

        DrawChessboard();

        //DrawCharacter(playerPos, playerSize);
        //DrawGun(aimFrom, aimDirection);

        System_Draw();
        System_DrawDebugCollisions();
        
        System_DrawEnemyHP();

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }
    // destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}
