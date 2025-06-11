#pragma once

#include <stdint.h>
#include <shapes.h>

typedef char Layer;

enum LayerName
{
    LN_PLAYER = 0,
    LN_ENEMY = 1,
    LN_WALL = 2,
    LN_PL_BULLET = 3,
    LN_EN_BULLET = 4,
    LN_PL_TRIGGER = 5,
    
    LN_COUNT,
};

/*enum PhysicsFlags {
    PH_IS_ALIVE = 1 << 0,
};*/

typedef struct {
    int entityA;
    int entityB;
    //char flags;
} CollisionData;

typedef struct
{
    Shape shape;
    Layer layer;
} ColliderComponent;

typedef struct
{
    uint32_t firstCollisionIndex;
    uint32_t lastCollisionIndex;
} HasCollisionsComponent;

typedef struct
{
    uint32_t entityId;
    HasCollisionsComponent *hasCollisions;
    uint32_t lastIndex;
    uint32_t other;
    CollisionData *collisionData;
} CollisionIterator;

extern Layer g_layerMask[];

#define MASK( _layerName ) 1 << _layerName

//void IntersectLayers(Layer a, Layer b);
void InitPhysics();
void System_Collide(float deltaTime);
void System_ClearCollisions();

/* Usage example:
        CollisionIterator iterator = { 0 };
        printf("InitCollisionIterator\n");
        InitCollisionIterator(&iterator, qr->list[i]);
        printf("TryGetNextCollision\n");
        while (TryGetNextCollision(&iterator)) {
            printf("while start\n");
            
            uint32_t entB = iterator.other;
            CollisionData* cData = iterator.collisionData;
            // process collision between ent and entB
        }
*/
//CollisionIteration can be rewritten as macro if needed to boost performance
void InitCollisionIterator(CollisionIterator* iterator, uint32_t entityId);
bool TryGetNextCollision(
    CollisionIterator* iterator
);
