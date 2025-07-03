#pragma once

#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>

#include <physics.h>
#include <attacks.h>

typedef char DamageFlags;
enum DamageFlagOptions
{
    DMG_TARGET_SELF = 1 << 0,
    DMG_TARGET_OTHER = 1 << 1,
    DMG_ON_TICK = 1 << 2,
};

//DynamicVector allows us to use mostly 2d logic,
//while allocating 3d vector, which will be used
//in fixed amount of contexts
typedef Vector2 DynamicVector2;
typedef Vector3 DynamicVector3;
typedef union
{
    DynamicVector2 v2;
    DynamicVector3 v3;
} DynamicVector;

typedef DynamicVector2 PositionComponent;
typedef DynamicVector2 VelocityComponent;

typedef struct
{
    short hp;
    short maxHp;
} HealthComponent;
typedef struct
{
    short damage;
    //Layer damageMask;
    DamageFlags flags;
} DealDamageComponent;

typedef AttackAbility PrimaryAttackComponent;
typedef AttackAbility SecondaryAttackComponent;

typedef char PlayerIdComponent;

typedef uint32_t ParentIdComponent;
typedef uint32_t TargetIdComponent;

typedef struct
{
    float velocity;
    float attackSpeedMult;
    float dmgMult;
} StatsComponent;

typedef char ItemType;

typedef struct
{
    char count;
    ItemType type;
} ItemComponent;

typedef struct
{
    short amount;
} CoinsComponent;

/*typedef char EventId;
enum EventIdOptions 
{
    EID_Kill = 0,
    EID_Interact,
};
typedef struct
{
    EventId id;
} EventComponent;*/

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
    
    CID_ParentId,
    CID_Stats,
    CID_Item,
    
    CID_Coins,
    CID_TargetId,
    //CID_Event,
    
    CID_StateFlags,
    
    CID_HasHpBar,
    CID_HasGravity,
    CID_IsFalling,
    CID_IsKilled,
    CID_IsWanderer,
    CID_AiAttack,
    CID_PlayerInput,
    CID_InventoryIsDirty,
    CID_IsInteractable,
    
    CID_PriceInCoins,
    
    CID_EventInteraction,
    CID_EventKill,
    
    CID_Count
};

void InitComponents();
