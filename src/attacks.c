#include <ecs.h>
#include <components.h>
#include <spawners.h>
#include <physics.h>

#include <attacks.h>

AttackConfig configArr[ATK_ID_COUNT] = { 0 };

void Perform_ShotPistols(AttackContext *context)
{
    Vector2 aimFromOffset = { 0, -12 }; //{ 0, -playerSize.y / 2 };
    PositionComponent* playerPos = (PositionComponent*)ecs_get(context->entityId, CID_Position);
    Vector2 aimFrom = Vector2Add (*playerPos, aimFromOffset);
    
    float bulletSpeed = 16 * 128;
    //TODO: apply dmgMult
    Spawn_Bullet(aimFrom, context->intention->aimAt, bulletSpeed);
}

void Perform_EnergyBlast(AttackContext *context)
{
    Vector2 aimFromOffset = { 0, -12 }; //{ 0, -playerSize.y / 2 };
    PositionComponent* playerPos = (PositionComponent*)ecs_get(context->entityId, CID_Position);
    Vector2 aimFrom = Vector2Add (*playerPos, aimFromOffset);
    
    float bulletSpeed = 16 * 16;
    //TODO: apply dmgMult
    Spawn_BigBullet(aimFrom, context->intention->aimAt, bulletSpeed);
}

void Perform_ShotFireball(AttackContext *context)
{
    float bulletSpeed = 16 * 12;
    PositionComponent* shooterPos = (PositionComponent*)ecs_get(context->entityId, CID_Position);
    Spawn_Fireball(*shooterPos, context->intention->aimAt, bulletSpeed);
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

void Perform_MeleeBite(AttackContext *context)
{
    Spawn_Melee(context->intention->aimAt);
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

void Attack_InitConfig()
{
    configArr[ATK_ID_SHOT_PISTOLS] = (AttackConfig){
        .cooldownTime = 0.25f,
        .performStrategy = Perform_ShotPistols,
    };
    configArr[ATK_ID_SHOT_ENERGY_BLAST] = (AttackConfig){
        .cooldownTime = 5.0f,
        .performStrategy = Perform_EnergyBlast,
    };

    configArr[ATK_ID_SHOT_FIREBALL] = (AttackConfig){
        .cooldownTime = 8.0f,
        .performStrategy = Perform_ShotFireball,
        .aiPriorityStrategy = AiPriority_ShotFireball,
    };
    configArr[ATK_ID_MELEE_BITE] = (AttackConfig){
        .cooldownTime = 3.0f,
        .performStrategy = Perform_MeleeBite,
        .aiPriorityStrategy = AiPriority_MeleeBite,
    };
    configArr[ATK_ID_MELEE_CLAW] = (AttackConfig){
        .cooldownTime = 3.0f,
        .performStrategy = Perform_MeleeBite,
        .aiPriorityStrategy = AiPriority_MeleeBite,
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
