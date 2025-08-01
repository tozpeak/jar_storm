#include <ecs.h>
#include <ecs_helpers.h>
#include <components.h>
#include <spawners.h>
#include <physics.h>
#include <level.h>

#include <attacks.h>

AttackConfig configArr[ATK_ID_COUNT] = { 0 };

Layer GetBulletLayer(AttackContext *context)
{
    ColliderComponent* col = (ColliderComponent*)ecs_get(context->entityId, CID_Collider);
    
    enum LayerName result;
    switch (col->layer)
    {
    case LN_PLAYER:
        result = LN_PL_BULLET;
        break;
    default:
        result = LN_EN_BULLET;
        break;
    }
    return (Layer) result;
}

void Perform_MeleeGeneric(AttackContext *context)
{
    Spawn_BuildGenericProjectile(
        context->intention->aimAt,
        Vector2Zero(),
        GetBulletLayer(context),
        context
    );
}
void Perform_ShotGeneric(AttackContext *context)
{
    PositionComponent* shooterPos = (PositionComponent*)ecs_get(context->entityId, CID_Position);
    Spawn_BuildGenericProjectile(
        *shooterPos,
        context->intention->aimAt,
        GetBulletLayer(context),
        context
    );
}
void Perform_ShotPlayerGeneric(AttackContext *context)
{
    //TODO: config aim offset somewhere else, make it more generic and not hardcoded to player
    Vector2 aimFromOffset = { 0, -12 }; //{ 0, -playerSize.y / 2 };
    PositionComponent* playerPos = (PositionComponent*)ecs_get(context->entityId, CID_Position);
    Vector2 aimFrom = Vector2Add (*playerPos, aimFromOffset);
    
    Spawn_BuildGenericProjectile(
        aimFrom,
        context->intention->aimAt,
        GetBulletLayer(context),
        context
    );
}

float AiPriority_ShotFireball(AttackContext *context)
{
    float activationDistance = 16 * 12;
    
    if (context->ability->cooldown > 0) return 0;
    
    uint32_t enemyEntId = context->entityId;
    
    uint32_t playerEntId;
    QueryResult qr = { .list = &playerEntId, .cap = 1 };
    ecs_query_ex(&qr, 1, CID_PlayerId);
    if (qr.count == 0) return 0;
    
    PositionComponent *enemyPos = (PositionComponent*)ecs_get(enemyEntId, CID_Position);
    PositionComponent *playerPos = (PositionComponent*)ecs_get(playerEntId, CID_Position);
    float distSqr = Vector2DistanceSqr(*enemyPos, *playerPos);
    if(distSqr > activationDistance * activationDistance) return 0;
    
    context->intention->aimAt = Vector2Normalize(
        Vector2Subtract(*playerPos, *enemyPos)
    );
    return 50;
}

float AiPriority_MeleeBite(AttackContext *context)
{
    if (context->ability->cooldown > 0) return 0;
    
    uint32_t entEnemy = context->entityId;
    
    if (!ecs_has(entEnemy, CID_HasCollisions)) return 0;
    
    CollisionIterator iterator = { 0 };
    InitCollisionIterator(&iterator, entEnemy);
    while (TryGetNextCollision(&iterator)) {
        uint32_t entB = iterator.other;
        CollisionData* cData = iterator.collisionData;
        
        if (!ecs_has(entB, CID_PlayerId)) continue;
        if (!ecs_has(entB, CID_Position)) continue;
        
        PositionComponent *playerPos = (PositionComponent*)ecs_get(entB, CID_Position);
        context->intention->aimAt = *playerPos;
        
        return 100;
    }
    return 0;
}

void Perform_ItemMushroomSet(AttackContext *context)
{
    Entity e = { .id = context->entityId };
    //copy config to override some values
    AttackProjectile config = Attack_GetConfigFor(context->ability->attackId)->projectile;
    
    ItemComponent *item = (ItemComponent*) ecs_get(e.id, CID_Item);
    ParentIdComponent *parentId = (ParentIdComponent*) ecs_get(e.id, CID_ParentId);
    
    config.radius += 16 * (item->count - 1) * 0.2f;
    
    ecs_add(
        context->entityId,
        CID_Position,
        (PositionComponent*) ecs_get(*parentId, CID_Position)
    );
    
    Spawn_AddShapeFromConfig (
        e, &config, (Layer)LN_PL_BULLET
    );
    
    DealDamageComponent dam = { 1, DMG_TARGET_OTHER | DMG_ON_TICK };
    
    ecs_add(e.id, CID_DealDamage, &dam );
}

void Perform_ItemMushroomReset(AttackContext *context)
{
    ecs_remove(context->entityId, CID_Position);
    PrimaryAttackComponent *prim = (PrimaryAttackComponent*) ecs_get(context->entityId, CID_PrimaryAttack);
    AttackConfig *primConfig = Attack_GetConfigFor(prim->attackId);
    
    prim->state = ATK_ST_COOLDOWN;
    prim->cooldown = primConfig->cooldownTime;
}

float AiPriority_ItemMushroomSet(AttackContext *context)
{
    if ( !ecs_has(context->entityId, CID_ParentId) ) return 0;
    if (  ecs_has(context->entityId, CID_Position) ) return 0; //already set
    
    ParentIdComponent *parentId = (ParentIdComponent*) ecs_get(context->entityId, CID_ParentId);
    VelocityComponent *parentVelocity = (VelocityComponent*) ecs_get(*parentId, CID_Velocity);
    
    if ( Vector2LengthSqr(*parentVelocity) > 0 ) return 0;
    
    return 50;
}

float AiPriority_ItemMushroomReset(AttackContext *context)
{
    if ( !ecs_has(context->entityId, CID_ParentId) ) return 0;
    //if ( !ecs_has(context->entityId, CID_Position) ) return 0; //already reset
    ParentIdComponent *parentId = (ParentIdComponent*) ecs_get(context->entityId, CID_ParentId);
    VelocityComponent *parentVelocity = (VelocityComponent*) ecs_get(*parentId, CID_Velocity);
    
    if ( Vector2LengthSqr(*parentVelocity) == 0 ) return 0;
    
    return 100;
}


bool IsEventInteraction(AttackContext *context)
{
    if ( !ecs_has(context->entityId, CID_EventInteraction) ) return false;
    if ( !ecs_has(context->entityId, CID_ParentId) ) return false;
    if ( !ecs_has(context->entityId, CID_TargetId) ) return false;
    return true;
}

void Perform_EventGiveCoins(AttackContext *context)
{
    if( !IsEventInteraction(context) ) return;
    
    uint32_t eventEntId = context->entityId;
    
    ParentIdComponent *actorId = (ParentIdComponent*) ecs_get(eventEntId, CID_ParentId);
    TargetIdComponent *targetId = (TargetIdComponent*) ecs_get(eventEntId, CID_TargetId);
    
    CoinsComponent *actorCoins =    (CoinsComponent*) ecs_get(*actorId, CID_Coins);
    CoinsComponent *targetCoins =   (CoinsComponent*) ecs_get(*targetId, CID_Coins);
    
    actorCoins->amount += targetCoins->amount;
    targetCoins->amount = 0;
    ecs_add(*targetId, CID_IsKilled, NULL);
}

void Perform_EventSpawnRandomItem(AttackContext *context)
{
    if( !IsEventInteraction(context) ) return;
    
    uint32_t eventEntId = context->entityId;
    
    ParentIdComponent *actorId = (ParentIdComponent*) ecs_get(eventEntId, CID_ParentId);
    TargetIdComponent *targetId = (TargetIdComponent*) ecs_get(eventEntId, CID_TargetId);
    
    PositionComponent *targetPosition = (PositionComponent*) ecs_get(*targetId, CID_Position);
    
    Spawn_RandomItem(*targetPosition);
    ecs_add(*targetId, CID_IsKilled, NULL);
}

void Perform_EventActivateTeleporter(AttackContext *context)
{
    if( !IsEventInteraction(context) ) return;
    
    uint32_t eventEntId = context->entityId;
    TargetIdComponent *targetId = (TargetIdComponent*) ecs_get(eventEntId, CID_TargetId);
    
    //enable existing CID_DealDamage
    ECS_GET_NEW(dam, *targetId, DealDamage);
    ecs_add(*targetId, CID_DealDamage, dam );
    ecs_add(*targetId, CID_HasHpBar, NULL );
    ecs_add(*targetId, CID_AiAttack, NULL );
    
    ECS_GET_NEW(primAtt, *targetId, PrimaryAttack);
    primAtt->attackId = ATK_ID_EVENT_CHARGE_TELEPORTER;
    ECS_GET_NEW(col, *targetId, Collider);
    col->shape.circle.radius = 16 * 12; //teleporter activation radius
    
    ecs_remove(*targetId, CID_IsInteractable);
}

float AiPriority_EventChargeTeleporter(AttackContext *context)
{
    ECS_GET_NEW(hp, context->entityId, Health);
    return (hp->hp >= hp->maxHp) ? 1.f : 0.f;
}
void Perform_EventChargeTeleporter(AttackContext *context)
{
    uint32_t *targetId = &(context->entityId);
    
    ecs_remove(*targetId, CID_DealDamage);
    ecs_remove(*targetId, CID_HasHpBar);
    ecs_add(*targetId, CID_IsInteractable, NULL);
    
    ECS_GET_NEW(primAtt, *targetId, PrimaryAttack);
    primAtt->attackId = ATK_ID_EVENT_ENTER_TELEPORTER;
    ECS_GET_NEW(draw, *targetId, DrawShape);
    ECS_GET_NEW(col, *targetId, Collider);
    draw->color = GREEN;
    col->shape.circle.radius = draw->shape.circle.radius;
    
}

void Perform_EventEnterTeleporter(AttackContext *context) 
{
    Level_NextLevel();
}

float AiPriority_EventInteraction(AttackContext *context)
{
    //EventInteraction "attacks" are performed by interactions module 
    //and should never be triggered by AI
    return 0;
}

void Attack_InitConfig()
{
    AttackProjectile genericMeleeProjectile = {
        .radius = 6.0f,
        .baseDmg = 24,
        .hp = 1,
        .color = ORANGE,
    };
    
    configArr[ATK_ID_SHOT_PISTOLS] = (AttackConfig){
        .cooldownTime = 0.25f,
        .performStrategy = Perform_ShotPlayerGeneric,
        .projectile = {
            .radius = 0,
            .velocity = 16 * 128,
            .baseDmg = 4,
            .hp = 1,
            .color = YELLOW,
        },
    };
    configArr[ATK_ID_SHOT_ENERGY_BLAST] = (AttackConfig){
        .cooldownTime = 5.0f,
        .performStrategy = Perform_ShotPlayerGeneric,
        .projectile = {
            .radius = 10.0f,
            .velocity = 16 * 16,
            .baseDmg = 4,
            .hp = 36,
            .color = SKYBLUE,
        },
    };

    configArr[ATK_ID_SHOT_FIREBALL] = (AttackConfig){
        .cooldownTime = 8.0f,
        .performStrategy = Perform_ShotGeneric,
        .aiPriorityStrategy = AiPriority_ShotFireball,
        .projectile = {
            .radius = 6.0f,
            .velocity = 16 * 12,
            .baseDmg = 36,
            .hp = 1,
            .color = ORANGE,
        },
    };
    configArr[ATK_ID_MELEE_BITE] = (AttackConfig){
        .cooldownTime = 3.0f,
        .performStrategy = Perform_MeleeGeneric,
        .aiPriorityStrategy = AiPriority_MeleeBite,
        .projectile = genericMeleeProjectile,
    };
    configArr[ATK_ID_MELEE_CLAW] = (AttackConfig){
        .cooldownTime = 3.0f,
        .performStrategy = Perform_MeleeGeneric,
        .aiPriorityStrategy = AiPriority_MeleeBite,
        .projectile = genericMeleeProjectile,
    };
    
    configArr[ATK_ID_ITEM_MUSHROOM_SET] = (AttackConfig){
        .cooldownTime = 2.0f,
        .performStrategy = Perform_ItemMushroomSet,
        .aiPriorityStrategy = AiPriority_ItemMushroomSet,
        .projectile = {
            .radius = 16 * 2.0f,
            .baseDmg = 12,
            .hp = 1,
            .color = (Color){ 255, 0, 0, 127 },
        },
    };
    configArr[ATK_ID_ITEM_MUSHROOM_RESET] = (AttackConfig){
        .cooldownTime = 0.0f,
        .performStrategy = Perform_ItemMushroomReset,
        .aiPriorityStrategy = AiPriority_ItemMushroomReset,
    };
    
    configArr[ATK_ID_EVENT_GIVE_COINS] = (AttackConfig){
        .cooldownTime = 0.0f,
        .performStrategy = Perform_EventGiveCoins,
        .aiPriorityStrategy = AiPriority_EventInteraction,
    };
    configArr[ATK_ID_EVENT_GIVE_RANDOM_ITEM] = (AttackConfig){
        .cooldownTime = 0.0f,
        .performStrategy = Perform_EventSpawnRandomItem,
        .aiPriorityStrategy = AiPriority_EventInteraction,
    };
    configArr[ATK_ID_EVENT_ACTIVATE_TELEPORTER] = (AttackConfig){
        .cooldownTime = 0.0f,
        .performStrategy = Perform_EventActivateTeleporter,
        .aiPriorityStrategy = AiPriority_EventInteraction,
    };
    configArr[ATK_ID_EVENT_CHARGE_TELEPORTER] = (AttackConfig){
        .cooldownTime = 0.0f,
        .performStrategy = Perform_EventChargeTeleporter,
        .aiPriorityStrategy = AiPriority_EventChargeTeleporter,
    };
    configArr[ATK_ID_EVENT_ENTER_TELEPORTER] = (AttackConfig){
        .cooldownTime = 0.0f,
        .performStrategy = Perform_EventEnterTeleporter,
        .aiPriorityStrategy = AiPriority_EventInteraction,
    };
}

AttackConfig *Attack_GetConfigFor(AttackId id)
{
    return &configArr[id];
}

void Attack_Perform(AttackContext *context)
{
    AttackConfig *config = Attack_GetConfigFor(context->ability->attackId);
    config->performStrategy(context);
    context->ability->state = ATK_ST_COOLDOWN;
    
    float cooldownTime = config->cooldownTime;
    if ( ecs_has(context->entityId, CID_Stats) ) {
        StatsComponent *stats = (StatsComponent*) ecs_get(context->entityId, CID_Stats);
        cooldownTime = cooldownTime / stats->attackSpeedMult;
    }
    context->ability->cooldown = cooldownTime;
}

void Attack_EvaluateAi(AttackContext *context)
{
    AttackConfig *config = Attack_GetConfigFor(context->ability->attackId);
    context->aiEvaluation = config->aiPriorityStrategy(context);
}
