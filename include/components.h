#pragma once

#include <raylib.h>
#include <raymath.h>

#include <physics.h>

typedef Vector2 PositionComponent;
typedef Vector2 VelocityComponent;
typedef struct
{
    float radius;
    Layer layer;
} ColliderComponent;
typedef struct 
{
    Vector2 offset;
    float radius;
    //Vector2 size;
    Color color;
} DrawRectangleComponent;
typedef struct
{
    short hp;
    short maxHp;
} HealthComponent;
typedef int StateFlagsComponent;

enum ComponentId
{
    CID_Position = 0,
    CID_Velocity,
    CID_Collider,
    CID_DrawRectangle,
    CID_Health,
    CID_StateFlags,
    CID_HasCollision,
    CID_IsBullet,
    CID_IsKilled,
    CID_IsWanderer,
    
    CID_Count
};

void InitComponents();
