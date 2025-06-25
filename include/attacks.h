#pragma once

#include <stdint.h>
#include <raylib.h>

typedef char AttackId;
enum AttackIdOptions 
{
    ATK_ID_SHOT_PISTOLS,
    ATK_ID_SHOT_ENERGY_BLAST,
    ATK_ID_SHOT_PISTOLS_ROUND,
    ATK_ID_SHOT_SHOTGUN,
    ATK_ID_SHOT_REVOLVER,
    ATK_ID_MELEE_SLASH,
    
    ATK_ID_SHOT_FIREBALL,
    ATK_ID_SHOT_BEAM,
    ATK_ID_MELEE_BITE,
    ATK_ID_MELEE_CLAW,
    ATK_ID_MELEE_CLAP,
    
    ATK_ID_COUNT
};

typedef char AttackState;
enum AttackStateOptions
{
    ATK_ST_READY,
    ATK_ST_BUILD_UP,
    ATK_ST_PERFORM,
    ATK_ST_COOLDOWN
};

typedef struct
{
    float cooldown;
    AttackId attackId;
    AttackState state;
} AttackAbility;

typedef struct
{
    Vector2 aimAt;
    bool isPrimary;
} AttackIntentionComponent;

typedef struct
{
    uint32_t entityId;
    AttackAbility *ability;
    AttackIntentionComponent *intention;
    float aiEvaluation;
} AttackContext;

typedef struct
{
    float radius; // 0 means line shape
    float velocity; // 0 means no velocity
    short baseDmg;
    short hp;
    Color color;
} AttackProjectile;

typedef struct 
{
    //float buildUpTime;
    //float performTime;
    float cooldownTime;
    void (* performStrategy)(AttackContext *context);
    float (* aiPriorityStrategy)(AttackContext *context);
    
    AttackProjectile projectile;
} AttackConfig;

void Attack_InitConfig();
AttackConfig *Attack_GetConfigFor(AttackId id);

void Attack_Perform(AttackContext *context);
void Attack_EvaluateAi(AttackContext *context);
