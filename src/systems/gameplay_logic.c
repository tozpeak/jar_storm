#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#include <ecs.h>
#include <components.h>
#include <physics.h>
#include <level.h>
#include <spawners.h>
#include <helpers.h>
#include <interactions.h>
#include <ecs_helpers.h>

#include <systems/gameplay_logic.h>

float RandomFloat()
{
    const int RAND_SAMPLE_SIZE = 
        (100000 > RAND_MAX) 
        ? RAND_MAX 
        : 100000;
    
    return (
        (float) (rand() % RAND_SAMPLE_SIZE )
        / RAND_SAMPLE_SIZE
    );
}

bool System_DebugPause()
{
    static bool isPaused = false;
    
    if(IsKeyPressed(KEY_P)) isPaused = !isPaused;
    return isPaused;
}

// MOVEMENT BLOCK

void System_Move(float deltaTime) 
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_Velocity);
    for (i = 0; i < qr->count; ++i) {
        DynamicVector3 *pos = (DynamicVector3*)ecs_get(qr->list[i], CID_Position);
        DynamicVector3 *vel = (DynamicVector3*)ecs_get(qr->list[i], CID_Velocity);
        *pos = Vector3Add(
            *pos,
            Vector3Scale( *vel, deltaTime )
        );
    }
}

void System_FallFromFloorOnGravity()
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_HasGravity);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        if ( ecs_has(entId, CID_IsFalling) ) continue;
        
        ECS_GET_NEW(pos, entId, Position);
        if( !IsPositionInPit(*pos) ) continue;
        
        ecs_add(entId, CID_IsFalling, NULL);
    }
}

void System_DropOnGround(float deltaTime)
{
    bool onTheGround = true;
    
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_Position, CID_Velocity, CID_IsFalling);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        DynamicVector3 *pos = (DynamicVector3*)ecs_get(entId, CID_Position);
        DynamicVector3 *vel = (DynamicVector3*)ecs_get(entId, CID_Velocity);
        float prevPosZ = pos->z - (vel->z * deltaTime);
        //if crossed plane z=0
        if (
            pos->z < 0
            && prevPosZ > 0
        ) {
            onTheGround = !IsPositionInPit(*(DynamicVector2*)pos);
            if (!onTheGround) continue;
            pos->z = 0;
            vel->z = 0;
            ecs_remove(entId, CID_IsFalling);
        }
    }
}

void System_MoveGravity(float deltaTime)
{
    const float gravity = 16 * 6.f;
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Velocity, CID_IsFalling);
    for (i = 0; i < qr->count; ++i) {
        DynamicVector3 *vel = (DynamicVector3*)ecs_get(qr->list[i], CID_Velocity);
        vel->z -= gravity * deltaTime;
    }
}

void System_FallOutOfVerticalBounds()
{
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_Velocity, CID_HasGravity, CID_IsFalling);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        DynamicVector *pos = (DynamicVector*)ecs_get(entId, CID_Position);
        DynamicVector *vel = (DynamicVector*)ecs_get(entId, CID_Velocity);
        if (pos->v3.z > -16 * 1.5f) continue;
        
        pos->v3.z = 1;
        
        if( ecs_has(entId, CID_Health) ) {
            ECS_GET_NEW(hp, entId, Health);
            hp->hp -= 16;
            if(hp->hp <= 0) ecs_add(entId, CID_IsKilled, NULL);
        }
        
        Vector2 newPos = pos->v2;
        
        Vector2 dPos = Vector2Scale(vel->v2, 0.25f);
        //game design decision:
        //making 2 steps back before looking for the ground
        //to prevent respawning across the pit
        for(int j = -1; j < 10; j++) {
            newPos = Vector2Subtract(newPos, dPos);
            if( j >=0 && !IsPositionInPit(newPos) ) break;
        }
        
        if( IsPositionInPit(newPos) ) newPos = g_level.spawnPoint;
        pos->v2 = newPos;
    }
}

void Systems_VerticalMovement(float deltaTime)
{
    System_FallFromFloorOnGravity();
    System_DropOnGround(deltaTime);
    System_MoveGravity(deltaTime);
    System_FallOutOfVerticalBounds();
}

// ATTACK BLOCK

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
        
        bool isFalling = ecs_has(playerEntId, CID_IsFalling);
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
            DynamicVector3 *vel3 = (DynamicVector3*)ecs_get(playerEntId, CID_Velocity);
            vel3->z = 16 * 2;
            ecs_add(playerEntId, CID_IsFalling, NULL);
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

void System_AutoHealPlayers(float delta)
{
    SYSTEM_TIMER(delta, 1.0f);
    if(timer_ticks < 1) return;
    
    short restoreAmount = 1;
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_PlayerId, CID_Health);
    
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        
        ECS_GET_NEW(hp, entId, Health);
        if (hp->hp <= 0) continue;
        if (hp->hp >= hp->maxHp) continue;
        
        hp->hp += restoreAmount * (short)timer_ticks;
        if (hp->hp >= hp->maxHp)
            hp->hp = hp->maxHp;
    }
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

// AI ON LEVEL BLOCK

void System_EnemyWanderer(float deltaTime)
{
    float timerDuration = 0.1f;
    SYSTEM_TIMER(deltaTime, timerDuration);
    if(timer_ticks < 1) return;
    
    float probability = (timer_ticks * timerDuration) * 1 / 5; // once in 5 seconds
    VelocityComponent newVel;
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Velocity, CID_IsWanderer);
    for (i = 0; i < qr->count; ++i) {
        if (RandomFloat() > probability) continue;
        
        VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
        
        newVel = Vector2Rotate(
            *vel,
            (rand() % 628) / 100.0f
        );
        *vel = newVel;
    }
}

void System_FillDistanceMap(float delta)
{
    SYSTEM_TIMER(delta, 0.3f);
    if(timer_ticks < 1) return;
    
    const char MAX_DISTANCE = 15;
    
    //Set all as empty
    AIMapTile *aiMap = g_level.aiMap;
    AIMapTile *aiTile;
    int i, j;
    for (i = 0; i < g_level.width; i++) {
        for (j = 0; j < g_level.height; j++) {
            aiTile = aiMap + LVL_INDEX(i,j);
            if (aiTile->aiDistance >= AI_DIST_EMPTY) continue;
            aiTile->aiDistance = AI_DIST_EMPTY;
        }
    }
    
    //prototype:
    //filled by circle, ai can stick to obstacles
    //better way would be to fill with BFS
    Vector2Int tPos, tFrom, tTo;
    uint32_t k;
    QueryResult *qr = ecs_query(2, CID_Position, CID_PlayerId);
    for (k = 0; k < qr->count; ++k) {
        uint32_t entPlayer = qr->list[k];
        
        ECS_GET_NEW(pos, entPlayer, Position);
        tPos = GetTilePosition(*pos);
        
        //eval tFrom, tTo
        tFrom = tPos;
        tFrom.x -= MAX_DISTANCE; tFrom.y -= MAX_DISTANCE;
        tTo = tPos;
        tTo.x += MAX_DISTANCE+1; tTo.y += MAX_DISTANCE+1;
        
        for (i = tFrom.x; i < tTo.x; i++) {
            if (i < 0 || i >= g_level.width) continue;
            
            for (j = tFrom.y; j < tTo.y; j++) {
                if (j < 0 || j >= g_level.height) continue;
                
                aiTile = aiMap + LVL_INDEX(i,j);
                if (aiTile->aiDistance >= AI_DIST_OBSTACLE) continue;
                
                unsigned char myDistance = (unsigned char) floor( 
                    sqrt(
                        (tPos.x - i)*(tPos.x - i) + (tPos.y - j)*(tPos.y - j)
                    )
                );
                if (myDistance > MAX_DISTANCE) continue;
                if (aiTile->aiDistance < myDistance) continue;
                aiTile->aiDistance = myDistance;
            }
        }
    }
}

void System_AiFollower(float delta)
{
    SYSTEM_TIMER(delta, 0.5f);
    if(timer_ticks < 1) return;
    
    AIMapTile *aiMap = g_level.aiMap;
    AIMapTile *aiTile;
    Vector2Int tPos;
    int i, j;
    
    uint32_t k;
    QueryResult *qr = ecs_query(3, CID_Position, CID_Velocity, CID_AiFollower);
    for (k = 0; k < qr->count; ++k) {
        uint32_t ent = qr->list[k];
        ECS_GET_NEW(pos, ent, Position);
        ECS_GET_NEW(aiFollow, ent, AiFollower);
        tPos = GetTilePosition(*pos);
        
        unsigned char targetDistance = aiFollow->targetDistance;
        unsigned char myDistance = Level_GetAITileForTilePos(tPos)->aiDistance;
        unsigned char bestDistance = myDistance;
        Vector2Int bestPos = tPos;
        
        for (i = tPos.x-1; i <= tPos.x+1; i++) {
            if (i < 0 || i >= g_level.width) continue;
            
            for (j = tPos.y-1; j <= tPos.y+1; j++) {
                if (j < 0 || j >= g_level.height) continue;
                
                aiTile = aiMap + LVL_INDEX(i,j);
                
                if (
                    abs (aiTile->aiDistance - targetDistance)
                    >=
                    abs (bestDistance - targetDistance)
                ) continue;
                
                bestDistance = aiTile->aiDistance;
                bestPos.x = i;
                bestPos.y = j;
            }
        }
        
        if (bestDistance >= AI_DIST_EMPTY) 
            ecs_add(ent, CID_IsWanderer, NULL);
        else
            ecs_remove(ent, CID_IsWanderer);
        
        //already in best place
        if (myDistance == bestDistance) continue;
        
        Vector2 walkInput = {
            bestPos.x - tPos.x,
            bestPos.y - tPos.y,
        };
        
        walkInput = Vector2Normalize(walkInput);
        
        ECS_GET_NEW(vel, ent, Velocity);
        
        *vel = Vector2Scale(
            walkInput,
            Vector2Length(*vel)
        );
    }
}

void System_KeepWandererInBounds()
{
    Vector2 tileSize = g_level.tileSize;
    
    float boundsMarginTiles = 0.5f;
    
    Rectangle levelBounds = {
        boundsMarginTiles * tileSize.x,
        boundsMarginTiles * tileSize.y,
        (g_level.width  - 2*boundsMarginTiles) * tileSize.x,
        (g_level.height - 2*boundsMarginTiles) * tileSize.y,
    };
    Vector2 levelCenter = {
        levelBounds.width / 2,
        levelBounds.height / 2,
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

void System_KeepWandererFromObstacles(float delta)
{
    SYSTEM_TIMER(delta, 0.05f);
    if(timer_ticks < 1) return;
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Position, CID_Velocity);
    for (i = 0; i < qr->count; ++i) {
        if( !ecs_has(qr->list[i], CID_IsWanderer)
            && !ecs_has(qr->list[i], CID_AiFollower) ) continue;
        
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
        
        Vector2 posNextSecond = Vector2Add(*pos, *vel);
        AIMapTile *tileInfo = Level_GetAITileForWorldPos(*pos);
        
        if ( tileInfo->aiDistance >= AI_DIST_OBSTACLE ) {
            *vel = Vector2Negate(*vel);
        }
    }
}

void System_SpawnRandomUnit(float deltaTime)
{
    float timerDuration = 0.1f;
    SYSTEM_TIMER(deltaTime, timerDuration);
    if(timer_ticks < 1) return;
    
    float probability = (timer_ticks * timerDuration) * 2 / 5;
    
    if(RandomFloat() > probability) return;
    
    bool isLizard = (rand() % 4 == 0);
    
    int spawnMargin = 16 * 4;
    
    int levelSizeX = g_level.width * g_level.tileSize.x,
        levelSizeY = g_level.height * g_level.tileSize.y;
        
    float spawnRadius = 16 * 12;
    
    QueryResult *qr = ecs_query(2, CID_Position, CID_PlayerId);
    
    if (qr->count < 1) return;
    
    ECS_GET_NEW(playerPos, qr->list[0], Position);
    Vector2 position;
    Vector2 up = (Vector2) { 0, 1 };
    
    int iterationsAllowed = 10;
    AIMapTile *aiTile;
    
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
        
        aiTile = Level_GetAITileForWorldPos(position);
        
    } while ( aiTile->aiDistance > AI_DIST_EMPTY );
    
    
    
    if (isLizard)
        Spawn_Enemy_Lizard(position);
    else
        Spawn_Enemy(position);
}

// INVENTORY BLOCK

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

// EVENT PROCESSING BLOCK

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

void System_ProcessKilledPlayer()
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_IsKilled, CID_PlayerId);
    for (i = 0; i < qr->count; ++i) {
        ecs_remove(qr->list[i], CID_IsKilled);
        ecs_remove(qr->list[i], CID_PlayerInput);
        ecs_remove(qr->list[i], CID_Velocity);
    }
}

// CLEANUP BLOCK

void System_KillOutOfBounds() 
{
    int boundsExtention = (g_level.width * g_level.tileSize.x) / 2;
    int boundsX = g_level.width * g_level.tileSize.x,
        boundsY = g_level.height * g_level.tileSize.y;
    
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_Position);
    for (i = 0; i < qr->count; ++i) {
        PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
        if (pos->x > boundsX + boundsExtention
         || pos->x < - boundsExtention
         || pos->y > boundsY + boundsExtention
         || pos->y < - boundsExtention
        ) ecs_add(qr->list[i], CID_IsKilled, NULL);
    }
}

void System_DestroyKilled()
{
    static DynamicVector3 zero = (Vector3) { 0 };
    
    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_IsKilled);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entId = qr->list[i];
        //set 0 into dynamic vectors to prevent z component from leaking
        ecs_add(entId, CID_Position, &zero);
        ecs_add(entId, CID_Velocity, &zero);
        ecs_kill(entId);
    }
}


void Systems_GameLoop()
{
    bool globalPause = System_DebugPause();
    if(globalPause) return;

    float delta = GetFrameTime();
        
    System_Move(delta);
    Systems_VerticalMovement(delta);
    
    //physics.h
    System_ClearCollisions();
    System_Collide(delta);
    System_PushRigidbodyFromStatic();
    
    System_UpdateAttackCooldown(delta);
    System_EvaluateAiAttack();
    System_PlayerInput(delta);
    System_PerformAttack();
    System_AutoHealPlayers(delta);
    System_DealDamageNew(delta);
    
    System_EnemyWanderer(delta);
    System_FillDistanceMap(delta);
    System_AiFollower(delta);
    System_KeepWandererFromObstacles(delta);
    System_SpawnRandomUnit(delta);
    
    System_PickItem();
    System_UpdateDirtyInventory();
    
    //interactions.h
    Systems_Interactions();
    
    System_GetCoinsOnKill();
    //System_SaveKilledPlayer();
    System_ProcessKilledPlayer();
    
    System_KillOutOfBounds();
    System_DestroyKilled();
}
