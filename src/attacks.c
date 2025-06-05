#include <ecs.h>
#include <components.h>
#include <spawners.h>
#include <physics.h>

#include <attacks.h>

AttackConfig configArr[ATK_ID_COUNT] = { 0 };

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
}

void Attack_InitConfig()
{
    configArr[ATK_ID_MELEE_BITE] = (AttackConfig){
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
    context->ability->cooldown = config->cooldownTime;
}

void Attack_EvaluateAi(AttackContext *context)
{
    AttackConfig *config = Attack_GetConfigFor(context->ability->attackId);
    context->aiEvaluation = config->aiPriorityStrategy(context);
}
