//#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

#include <components.h>
#include <physics.h>
#include <shapes.h>
#include <spawners.h>
#include <interactions.h>
#include <helpers.h>
#include <ecs_helpers.h>

#define BACKGROUND_COLOR BLACK

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

void DrawChessboard() 
{
    int screenX = g_screenSettings.width * g_screenSettings.levelScale,
        screenY = g_screenSettings.height * g_screenSettings.levelScale;
    Vector2 tileSize = g_screenSettings.tileSize;
    int offsetTiles = g_screenSettings.marginTiles;
    int offsetTopTiles = g_screenSettings.marginTopTiles;
    Color tileColor = DARKGRAY;
    
    /*DrawRectangle(
        offsetTiles * tileSize.x,
        offsetTopTiles * tileSize.y,
        screenX - ( offsetTiles * 2 ) * tileSize.x,
        screenY - ( offsetTiles + offsetTopTiles ) * tileSize.y,
        GRAY
    );*/
    
    for (int i = offsetTiles; (i + offsetTiles) * tileSize.x < screenX; i++) 
    {
        for (int j = offsetTopTiles; (j + offsetTiles - 1) * tileSize.y < screenY; j++) 
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

void System_FallOnGravity()
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_HasGravity);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        if ( ecs_has(entId, CID_Vertical) ) continue;
        
        ECS_GET_NEW(pos, entId, Position);
        if( !IsPositionInPit(*pos) ) continue;
        
        VerticalComponent vert = { 0, 0 };
        ecs_add(entId, CID_Vertical, &vert);
    }
}

void System_DropOnGround(float deltaTime)
{
    bool onTheGround = true;
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_Vertical);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        ECS_GET_NEW(vert, entId, Vertical);
        
        //if crossing plane z=0
        if (
            vert->zpos > 0
            && vert->zvel < 0
            && vert->zpos < -vert->zvel * deltaTime
        ) {
            ECS_GET_NEW(pos, entId, Position);
            onTheGround = !IsPositionInPit(*pos);
            if (!onTheGround) continue;
            vert->zpos = 0;
            vert->zvel = 0;
            ecs_remove(entId, CID_Vertical);
        }
    }
}

void System_MoveVertical(float deltaTime)
{
    const float gravity = 16 * 6.f;
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_Vertical);
    for (i = 0; i < qr->count; ++i) {
        ECS_GET_NEW(vert, qr->list[i], Vertical);
        vert->zpos += vert->zvel * deltaTime;
        vert->zvel -= gravity * deltaTime;
    }
}

void System_FallOutOfMap()
{
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_Vertical);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        ECS_GET_NEW(vert, entId, Vertical);
        if (vert->zpos > -16 * 1.5f) continue;
        
        vert->zpos = 1;
        
        if( ecs_has(entId, CID_Health) ) {
            ECS_GET_NEW(hp, entId, Health);
            hp->hp -= 16;
        }
        
        ECS_GET_NEW(pos, entId, Position);
        Vector2 newPos = *pos;
        if( ecs_has(entId, CID_Velocity) ) {
            ECS_GET_NEW(vel, entId, Velocity);
            Vector2 dPos = Vector2Scale(*vel, 0.25f);
            for(int j = -1; j < 10; j++) {
                newPos = Vector2Subtract(newPos, dPos);
                if( j >=0 && !IsPositionInPit(newPos) ) break;
            }
        }
        if( IsPositionInPit(newPos) ) newPos = (Vector2) { 32, 32 };
        *pos = newPos;
    }
}

void Systems_VerticalMovement(float deltaTime)
{
    System_FallOnGravity();
    System_DropOnGround(deltaTime);
    System_MoveVertical(deltaTime);
    System_FallOutOfMap();
}

void System_DealDamageNew(float delta)
{
    SYSTEM_TIMER(delta, 0.1f);
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_DealDamage, CID_HasCollisions);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entA = qr->list[i];
        DealDamageComponent *dam = (DealDamageComponent*)ecs_get(entA, CID_DealDamage);
        HasCollisionsComponent *hc = (HasCollisionsComponent*)ecs_get(entA, CID_HasCollisions);
        
        bool onTick = dam->flags & DMG_ON_TICK;
        if(onTick && timer_ticks < 1) continue;
        
        short dmgMult = onTick ? (short)timer_ticks : 1;
        
        CollisionIterator iterator = { 0 };
        InitCollisionIterator(&iterator, entA);
        while (TryGetNextCollision(&iterator)) {
            uint32_t entB = iterator.other;
            CollisionData* cData = iterator.collisionData;
            
            if (dam->flags & DMG_TARGET_OTHER
                && ecs_has(entB, CID_Health)) {
                HealthComponent *hp = (HealthComponent*)ecs_get(entB, CID_Health);
                
                hp->hp -= dam->damage * dmgMult;
                if(hp->hp <= 0) 
                {
                    if ( !ecs_has(entB, CID_IsKilled) )
                    {
                        ecs_add(entB, CID_IsKilled, NULL);
                        
                        Entity eKillEvent = ecs_create();
                        ecs_add(eKillEvent.id, CID_ParentId, &entA);
                        ecs_add(eKillEvent.id, CID_TargetId, &entB);
                        ecs_add(eKillEvent.id, CID_EventKill, NULL);
                        ecs_add(eKillEvent.id, CID_IsKilled, NULL); // events live only single frame
                    }
                }
            }
            
            if (dam->flags & DMG_TARGET_SELF
                && ecs_has(entA, CID_Health))
            {
                HealthComponent *hpDam = (HealthComponent*)ecs_get(entA, CID_Health);
                hpDam->hp -= dam->damage * dmgMult;
                if(hpDam->hp <= 0) ecs_add(entA, CID_IsKilled, NULL);
            }
        }
        
    }
}

void System_KeepWandererInBounds()
{
    Vector2 tileSize = g_screenSettings.tileSize;
    
    Rectangle levelBounds = {
        tileSize.x * g_screenSettings.marginTiles,
        tileSize.y * g_screenSettings.marginTopTiles,
        g_screenSettings.width * g_screenSettings.levelScale - tileSize.x * (g_screenSettings.marginTiles * 2),
        g_screenSettings.height * g_screenSettings.levelScale - tileSize.y * (g_screenSettings.marginTiles + g_screenSettings.marginTopTiles),
    };
    Vector2 levelCenter = {
        g_screenSettings.width / 2,
        g_screenSettings.height / 2,
    };
    VelocityComponent newVel;
    
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_Position, CID_Velocity, CID_IsWanderer);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        
        if ( !CheckCollisionPointRec(*pos, levelBounds) )
        {
            VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
            float velLinear = Vector2Length(*vel);
            newVel = Vector2Scale(
                Vector2Normalize( 
                    Vector2Subtract (levelCenter, *pos) 
                ),
                velLinear
            );
            
            *vel = newVel;
        }
    }
}

void System_EnemyWanderer(float deltaTime)
{
    const int CHANCE_SAMPLE_SIZE = 100000;
    float changeDirProbabilityF = deltaTime * 1 / 5; // once in 5 seconds
    int changeDirProbability = round(CHANCE_SAMPLE_SIZE * changeDirProbabilityF);
    VelocityComponent newVel;
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Velocity, CID_IsWanderer);
    for (i = 0; i < qr->count; ++i) {
        if (rand() % CHANCE_SAMPLE_SIZE > changeDirProbability) continue;
        
        VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
        
        newVel = Vector2Rotate(
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
    float shotCooldownState = 0;
    float shotCooldown = 0.25f;
    
    uint32_t i;
    QueryResult *qr = ecs_query(5, CID_Position, CID_PrimaryAttack, CID_SecondaryAttack, CID_PlayerId, CID_PlayerInput);
    for (i = 0; i < qr->count; ++i) {
        uint32_t playerEntId = qr->list[i];
        
        context.entityId = playerEntId;
        
        bool isFalling = ecs_has(playerEntId, CID_Vertical);
        //PlayerIdComponent playerId = * (PlayerIdComponent*)ecs_get(playerEntId, CID_PlayerId);
        
        walkInput = Vector2Zero();
        
        if (IsKeyDown(KEY_D)) walkInput.x += 1;
        if (IsKeyDown(KEY_A)) walkInput.x -= 1;
        if (IsKeyDown(KEY_W)) walkInput.y -= 1;
        if (IsKeyDown(KEY_S)) walkInput.y += 1;
        
        StatsComponent *stats = (StatsComponent*) ecs_get(playerEntId, CID_Stats);
        
        walkInput = Vector2Scale(
            Vector2Normalize(walkInput),
            stats->velocity
        );
        
        if ( !isFalling ) {
            ecs_add(playerEntId, CID_Velocity, &walkInput);
        }
        
        if (!isFalling && IsKeyDown(KEY_SPACE)) {
            VerticalComponent vert = { 0, 16 * 2 };
            ecs_add(playerEntId, CID_Vertical, &vert);
        }
        
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
    
    int levelSizeX = g_screenSettings.width * g_screenSettings.levelScale,
        levelSizeY = g_screenSettings.height * g_screenSettings.levelScale;
        
    float spawnRadius = 16 * 12;
    
    QueryResult *qr = ecs_query(2, CID_Position, CID_PlayerId);
    
    if (qr->count < 1) return;
    
    ECS_GET_NEW(playerPos, qr->list[0], Position);
    Vector2 position;
    Vector2 up = (Vector2) { 0, 1 };
    
    int iterationsAllowed = 10;
    
    do {
        position = Vector2Add(
            *playerPos,
            Vector2Scale(
                Vector2Rotate( up, (rand() % 628) / 100.f ), // rand (0 .. 2*PI)
                spawnRadius * ( 10 + rand() % 90 ) / 100.f // rand (0.1 .. 1.0)
            )
        );
        iterationsAllowed--;
        
        if (iterationsAllowed <= 0) return;
        
    } while (
        position.x < spawnMargin
        || position.x > (levelSizeX - spawnMargin)
        || position.y < spawnMargin
        || position.y > (levelSizeY - spawnMargin)
    );
    
    
    
    if (isLizard)
        Spawn_Enemy_Lizard(position);
    else
        Spawn_Enemy(position);
}

void System_PickItem()
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Item, CID_HasCollisions);
    
    int itemsUpdatedCount = 0;
    uint32_t firstEntity = 0;
    for (i = 0; i < qr->count; ++i) {
        
        uint32_t entItem = qr->list[i];
        
        if ( ecs_has(entItem, CID_ParentId) ) continue;
        
        CollisionIterator iterator = { 0 };
        InitCollisionIterator(&iterator, entItem);
        while (TryGetNextCollision(&iterator)) {
            uint32_t entPlayer = iterator.other;
            CollisionData* cData = iterator.collisionData;
            
            if( !ecs_has(entPlayer, CID_PlayerId) ) continue;
            
            if(itemsUpdatedCount == 0) firstEntity = entItem;
            itemsUpdatedCount++;
            ecs_add(entItem, CID_ParentId, &entPlayer);
            ecs_remove(entItem, CID_Position);
            ecs_add(entPlayer, CID_InventoryIsDirty, NULL);
            
            break;
        }
    }
    
    //compress items in "stacks"
    //optimization: no need to make complex enumerations if zero items were picked
    if (itemsUpdatedCount < 1) return;
    
    qr = ecs_query(2, CID_Item, CID_ParentId);
    
    int j;
    i = 0;
    //skip regular items
    while(qr->list[i] < firstEntity) { i++; }
    
    for (; i < qr->count; ++i) {
        uint32_t entItemA = qr->list[i];
        
        if( ecs_has(entItemA, CID_IsKilled) ) continue;
        
        ItemComponent *itemA = (ItemComponent*)ecs_get(entItemA, CID_Item);
        
        for (j = 0; j < qr->count; ++j) {
            uint32_t entItemB = qr->list[j];
            if(entItemB == entItemA) continue;
            if( ecs_has(entItemB, CID_IsKilled) ) continue;
            
            ItemComponent *itemB = (ItemComponent*)ecs_get(entItemB, CID_Item);
            if (itemA->type != itemB->type) continue;
            
            if(
                (*(ParentIdComponent*)ecs_get(entItemA, CID_ParentId)) 
                != (*(ParentIdComponent*)ecs_get(entItemB, CID_ParentId))
            ) continue;
            
            //different item entities, same item type, same parent
            //compress: add item counts into the first, kill the other
            itemB->count += itemA->count;
            itemA->count = 0;
            ecs_add(entItemA, CID_IsKilled, NULL);
            break;
        }
    }
}

void System_UpdateDirtyInventory()
{
    uint32_t i;
    QueryResult *qr = NULL;
    
    uint32_t entInventory;
    QueryResult dirtyQr = { .list = &entInventory, .cap = 1 };
    
    while (true) {
        ecs_query_ex(&dirtyQr, 1, CID_InventoryIsDirty);
        if(dirtyQr.count < 1) break;
        
        if(qr == NULL) { //making request once since we can't filter the items other than enumerating
            qr = ecs_query(2, CID_Item, CID_ParentId);
        }
        
        StatsComponent *stats = ecs_get(entInventory, CID_Stats);
        
        if ( ecs_has(entInventory, CID_ParentId) ) {
            ParentIdComponent *entParent = ecs_get(entInventory, CID_ParentId);
            ecs_add(entInventory, CID_Stats, ecs_get(*entParent, CID_Stats) );
        }
        else {
            *stats = (StatsComponent) { 1, 1, 1 }; //default values
        }
        
        for(i = 0; i < qr->count; ++i) {
            uint32_t entItem = qr->list[i];
            
            if (
                (*(ParentIdComponent*)ecs_get(entItem, CID_ParentId))
                != entInventory
            ) continue;
            
            ItemComponent *item = (ItemComponent*)ecs_get(entItem, CID_Item);
            
            if ( ecs_has(entItem, CID_Stats) ) {
                StatsComponent *itemStats = ecs_get(entItem, CID_Stats);
                
                stats->velocity         += itemStats->velocity          * item->count;
                stats->attackSpeedMult  += itemStats->attackSpeedMult   * item->count;
                stats->dmgMult          += itemStats->dmgMult           * item->count;
            }
        }
        
        ecs_remove(entInventory, CID_InventoryIsDirty);
    }
}

void System_GetCoinsOnKill()
{
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_EventKill);
    
    for (i = 0; i < qr->count; ++i) {
        uint32_t entEvent = qr->list[i];
        
        ECS_GET_NEW(entProjectile, entEvent, ParentId);
        ECS_GET_NEW(entTarget, entEvent, TargetId);
        
        if ( !ecs_has(*entProjectile, CID_ParentId) ) continue;
        ECS_GET_NEW(entShooter, *entProjectile, ParentId);
        
        if ( !ecs_has(*entShooter, CID_Coins) ) continue;
        if ( !ecs_has(*entTarget, CID_Coins) ) continue;
        ECS_GET_NEW(coinsShooter, *entShooter, Coins);
        ECS_GET_NEW(coinsTarget, *entTarget, Coins);
        
        coinsShooter->amount += coinsTarget->amount;
        coinsTarget->amount = 0;
    }
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
        Vector2 pos = *playerPos;
        Vector2 dz = Vector2Zero();
        
        if ( ecs_has(playerEntId, CID_Vertical) ) {
            ECS_GET_NEW(vert, playerEntId, Vertical);
            dz.y -= vert->zpos;
            pos = Vector2Add(pos, dz);
        }
        
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

void System_DrawHUD_HeaderBackground()
{
    int boundsX = g_screenSettings.width * g_screenSettings.levelScale,
        boundsY = g_screenSettings.height * g_screenSettings.levelScale;
    
    DrawRectangle(
        0,
        0,
        g_screenSettings.width,
        2 * g_screenSettings.tileSize.y,
        (Color) { 0, 0, 0, 198 }
    );
}

void System_DrawHUD_Coins()
{
    QueryResult *qr = ecs_query(1, CID_PlayerId);
    uint32_t entPlayer = qr->list[0];
    
    CoinsComponent *coins = (CoinsComponent*) ecs_get(entPlayer, CID_Coins);
    
    
    DrawText(
        TextFormat("C:%d", coins->amount),
        64,
        8,
        16, DARKGREEN
    );
}

void System_DrawHUD_Items()
{
    const char* labels[] = {
        "MVS",
        "ATS",
        "DMG",
        "MSH"
    };

    QueryResult *qr = ecs_query(1, CID_PlayerId);
    uint32_t entPlayer = qr->list[0];
    
    uint32_t i;
    uint32_t j = 0;
    qr = ecs_query(2, CID_Item, CID_ParentId);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entItem = qr->list[i];
        
        ParentIdComponent *parent = (ParentIdComponent*) ecs_get(entItem, CID_ParentId);
        
        if (*parent != entPlayer) continue;
        
        ItemComponent *item = (ItemComponent*) ecs_get(entItem, CID_Item);
        
        DrawText(
            TextFormat("%s:%d", labels[item->type], item->count),
            128 + 64 * ( j++ ),
            8,
            16, DARKGREEN
        );
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

bool System_DebugPause()
{
    static bool isPaused = false;
    
    if(IsKeyPressed(KEY_P)) isPaused = !isPaused;
    return isPaused;
}

void System_KillOutOfBounds() 
{
    int boundsX = g_screenSettings.width * g_screenSettings.levelScale,
        boundsY = g_screenSettings.height * g_screenSettings.levelScale;
    
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_Position);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        if (pos->x > boundsX
         || pos->x < 0
         || pos->y > boundsY
         || pos->y < 0
        ) ecs_add(qr->list[i], CID_IsKilled, NULL);
    }
}


void Systems_GameLoop()
{
    bool globalPause = System_DebugPause();
    if(globalPause) return;

    float delta = GetFrameTime();
        
    System_Move(delta);
    Systems_VerticalMovement(delta);
    System_ClearCollisions();
    System_Collide(delta);
    
    System_UpdateAttackCooldown(delta);
    System_EvaluateAiAttack();
    System_PlayerInput(delta);
    System_PerformAttack();
    System_DealDamageNew(delta);
    System_EnemyWanderer(delta);
    System_KeepWandererInBounds();
    System_SpawnRandomUnit(delta);
    
    System_PickItem();
    System_UpdateDirtyInventory();
    
    Systems_Interactions();
    
    System_GetCoinsOnKill();
    
    System_KillOutOfBounds();
    System_SaveKilledPlayer();
    System_DestroyKilled();
}

void Systems_DrawLoop()
{
    DrawChessboard();
    
    System_Draw();
    System_DrawPlayer();
    Systems_DrawInteractions();

    System_DrawDebugCollisions();

    System_DrawEnemyHP();
}

void Systems_DrawUILoop()
{
    System_DrawHUD_HeaderBackground();
    System_DrawHUD_Coins();
    System_DrawHUD_Items();
}

void GenerateLevel()
{
    int pillarCount = 20 * g_screenSettings.levelScale * g_screenSettings.levelScale;
    int interactableCount = 48;

    int screenX = g_screenSettings.width * g_screenSettings.levelScale,
        screenY = g_screenSettings.height * g_screenSettings.levelScale;
    Vector2 tileSize = g_screenSettings.tileSize;
    Rectangle levelRect = {
        g_screenSettings.marginTiles * tileSize.x,
        g_screenSettings.marginTopTiles * tileSize.y,
        screenX - ( g_screenSettings.marginTiles * 2 ) * tileSize.x,
        screenY - ( g_screenSettings.marginTiles + g_screenSettings.marginTopTiles ) * tileSize.y,
    };

    Spawn_Teleporter((Vector2) { screenX - 32, screenY - 32 } );
    //Spawn_RandomItem((Vector2) { 256, 256 } );
    
    for (int i = 0; i < pillarCount; i++) {
        Spawn_Pillar( (Vector2) {
            rand() % (int)round(levelRect.width) + levelRect.x,
            rand() % (int)round(levelRect.height) + levelRect.y,
        } );
    }
    
    for (int i = 0; i < interactableCount; i++) {
        Spawn_Interactable((Vector2) {
            rand() % (int)round(levelRect.width) + levelRect.x,
            rand() % (int)round(levelRect.height) + levelRect.y,
        } );
    }
    
    /*for (int i = 0; i < 4; i++) {
        Spawn_Interactable( (Vector2) { 96 + i * 4, 96 } );
    }*/
}

int test_main();

int test_limits()
{
    if (CID_Count >= 32) {
        printf( "CID_Count is %d >= 32!\n"
                "Move flag components to separate bitmask or make the current mask bigger.\n"
                , CID_Count);
        return 1;
    }
    return 0;
}

int main ()
{
    int limitsResult = test_limits();
    if(limitsResult > 0) return limitsResult;
    
    //return test_main();

    // Tell the window to use vsync and work on high DPI displays
    //SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    //SetTargetFPS(120);

    // Create the window and OpenGL context
    const int screenScale = 2;

    InitWindow(g_screenSettings.width * screenScale, g_screenSettings.height * screenScale, "Hello Raylib");
    //SetWindowSize(g_screenSettings.width * screenScale, g_screenSettings.height * screenScale);

    Vector2 centerScreenOffset = (Vector2){ g_screenSettings.width / 2.f, g_screenSettings.height / 2.f };
    
    Camera2D *camera = g_screenSettings.camera;
    camera->target = (Vector2){ 0 };
    camera->offset = Vector2Scale(centerScreenOffset, screenScale);
    camera->rotation = 0.0f;
    camera->zoom = screenScale;


    InitPhysics();
    InitComponents();
    Attack_InitConfig();
    
    srand(time(NULL));

    Entity player = Spawn_Player(
        (Vector2) { 32, 32 },
        0
    );
    
    GenerateLevel();
    
    //SpawnEntireFieldOfEnemies();

    // game loop
    while (!WindowShouldClose())        // run the loop untill the user presses ESCAPE or presses the Close button on the window
    {
        Systems_GameLoop();
        
        // drawing
        BeginDrawing();
        
        // Setup the back buffer for drawing (clear color and depth buffers)
        ClearBackground(BACKGROUND_COLOR);
        
        ECS_GET_NEW(playerPos, player.id, Position);
        camera->target = *playerPos;
        BeginMode2D(*camera);
        Systems_DrawLoop();
        EndMode2D();
        
        camera->target = centerScreenOffset;
        BeginMode2D(*camera);
        Systems_DrawUILoop();
        EndMode2D();
        camera->target = *playerPos;
        
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
        ClearBackground(BACKGROUND_COLOR);

        Systems_DrawLoop();
        Systems_DrawUILoop();

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }
    // destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}
