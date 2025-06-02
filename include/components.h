#pragma once

#include <raylib.h>
#include <raymath.h>

#include <physics.h>

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
    Vector2 offset;
    float radius;
    //Vector2 size;
    Color color;
} DrawShapeComponent;
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
typedef int StateFlagsComponent;

enum ComponentId
{
    CID_Position = 0,
    CID_Velocity,
    CID_Collider, //defined in physics.h
    CID_HasCollisions, //defined in physics.h
    CID_DrawShape,
    CID_Health,
    CID_DealDamage,
    CID_StateFlags,
    CID_HasHpBar,
    CID_IsKilled,
    CID_IsWanderer,
    
    CID_Count
};

void InitComponents();
