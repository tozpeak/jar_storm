#pragma once

#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

#include <physics.h>
#include <attacks.h>

typedef char DamageTarget;
enum DamageTargetOptions
{
    DMG_SELF = 1 << 0,
    DMG_OTHER = 1 << 1,
};

typedef Vector2 PositionComponent;
typedef Vector2 VelocityComponent;

typedef struct
{
    short hp;
    short maxHp;
} HealthComponent;
typedef struct
{
    short damage;
    //Layer damageMask;
    DamageTarget target;
} DealDamageComponent;

typedef AttackAbility PrimaryAttackComponent;
typedef AttackAbility SecondaryAttackComponent;

typedef char PlayerIdComponent;

typedef int StateFlagsComponent;

enum ComponentId
{
    CID_Position = 0,
    CID_Velocity,
    CID_Collider, //defined in physics.h
    CID_HasCollisions, //defined in physics.h
    CID_DrawShape, //defined in shapes.h
    
    CID_Health,
    CID_DealDamage,
    
    CID_PrimaryAttack,
    CID_SecondaryAttack,
    CID_AttackIntention, //defined in attacks.h
    CID_PlayerId,
    
    CID_StateFlags,
    
    CID_HasHpBar,
    CID_IsKilled,
    CID_IsWanderer,
    CID_AiAttack,
    
    CID_Count
};

void InitComponents();
