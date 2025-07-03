#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <raylib.h>
#include <ecs.h>
#include <ecs_helpers.h>

#include <components.h>
#include <physics.h>

#define PH_INITIAL_CAPACITY 32

typedef struct {
    int count;
    int cap;
    CollisionData *data;
} CollisionWorldState;

Layer g_layerMask[LN_COUNT] = { 0 };
CollisionWorldState state = { 0 };
HasCollisionsComponent hasCollisionsTemplate = { 0 };

void InitLayers();
void IntersectLayers(Layer a, Layer b);

void InitCollisionWorldState();
// returns collision index
int NewCollision();
void AddCollisionToEntity(
    uint32_t entityId, 
    uint32_t otherEntityId, 
    uint32_t collisionId
);

bool Collision_Circle2Circle(PositionComponent* pos1, PositionComponent* pos2, ShapeCircle *circle1, ShapeCircle *circle2, CollisionData *collision);
bool Collision_Circle2Line(PositionComponent* pos1, PositionComponent* pos2, ShapeCircle *circle1, ShapeLine *line2);
bool Collision_Line2Line(PositionComponent* pos1, PositionComponent* pos2, ShapeLine *line1, ShapeLine *line2);

// ---

void InitPhysics() 
{
    InitLayers();
    InitCollisionWorldState();
}

void System_Collide(float deltaTime)
{
    uint32_t i, j;
    QueryResult *qr = ecs_query(2, CID_Position, CID_Collider);
    uint32_t* list = qr->list;
    for (i = 0; i < qr->count; ++i) {
        uint32_t ent = list[i];
        PositionComponent *pos = (PositionComponent*)ecs_get(ent, CID_Position);
        ColliderComponent *col = (ColliderComponent*)ecs_get(ent, CID_Collider);
        Layer layerMask = g_layerMask[col->layer];
        
        for (j = i+1; j < qr->count; ++j) {
            uint32_t entB = list[j];
            ColliderComponent *colB = (ColliderComponent*)ecs_get(entB, CID_Collider);
            
            if( !(MASK(colB->layer) & layerMask) ) continue;
            
            //do not support pixel colliders
            if (col->shape.type == SHP_PIXEL || colB->shape.type == SHP_PIXEL) continue;
            
            PositionComponent *posB = (PositionComponent*)ecs_get(entB, CID_Position);
            bool hasCollision = false;
            CollisionData newCollision = { 0 };
            
            switch (col->shape.type)
            {
            case SHP_CIRCLE:                
                switch (colB->shape.type)
                {
                case SHP_CIRCLE:
                    hasCollision = Collision_Circle2Circle( pos, posB, &(col->shape.circle), &(colB->shape.circle), &newCollision );
                    break;
                case SHP_LINE:
                    hasCollision = Collision_Circle2Line( pos, posB, &(col->shape.circle), &(colB->shape.line) );
                    break;
                }
                break;
            case SHP_LINE:
                switch (colB->shape.type)
                {
                case SHP_CIRCLE:
                    hasCollision = Collision_Circle2Line( posB, pos, &(colB->shape.circle), &(col->shape.line) );
                    break;
                case SHP_LINE:
                    hasCollision = Collision_Line2Line( pos, posB, &(col->shape.line), &(colB->shape.line) );
                    break;
                }
                break;
            }
            
            if (!hasCollision) continue;
            
            uint32_t collisionId = NewCollision();
            CollisionData* collision = &( state.data[collisionId] );
            newCollision.entityA = ent;
            newCollision.entityB = entB;
            *collision = newCollision;
            
            AddCollisionToEntity(ent, entB, collisionId);
            AddCollisionToEntity(entB, ent, collisionId);
        }
    }
}

void System_PushRigidbodyFromStatic()
{
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_HasCollisions, CID_StaticCollider);
    for (i = 0; i < qr->count; ++i) {
        CollisionIterator iterator = { 0 };
        InitCollisionIterator(&iterator, qr->list[i]);
        while (TryGetNextCollision(&iterator)) {
            uint32_t entB = iterator.other;
            if( !ecs_has(entB, CID_Rigidbody) ) continue;
            if( ecs_has(entB, CID_StaticCollider) ) continue;
            
            CollisionData* cData = iterator.collisionData;
            if ( cData->depth <= 0 ) continue;
            
            Vector2 normal = iterator.normal;
            ECS_GET_NEW(pos, entB, Position);
            *pos = Vector2Add(
                *pos,
                Vector2Scale(normal, cData->depth)
            );
        }
    }
}

void System_ClearCollisions() 
{
    state.count = 0; //clear all collision info from previous frame

    uint32_t i;
    QueryResult *qr = ecs_query(1, CID_HasCollisions);
    for (i = 0; i < qr->count; ++i) {
        ecs_remove(qr->list[i], CID_HasCollisions);
    }
}

void InitCollisionIterator(CollisionIterator* iterator, uint32_t entityId)
{
    HasCollisionsComponent* hc = (HasCollisionsComponent*) ecs_get(entityId, CID_HasCollisions);
    
    iterator->entityId = entityId;
    iterator->hasCollisions = hc;
    iterator->lastIndex = hc->firstCollisionIndex;
    
}

/*
#define COMPARE_ENTITY( entityA, entityB ) if (state.data[i].entityA == iterator->entity) { \
            iterator->other = state.data[i].entityB; \
            iterator->collisionData = &(state.data[i]); \
            iterator->lastIndex = i+1; \
            return true; \
        }
*/

bool TryGetNextCollision(
    CollisionIterator* iterator
) {
    HasCollisionsComponent* hc = iterator->hasCollisions;
    for(uint32_t i = iterator->lastIndex++;
        i <= hc->lastCollisionIndex;
        i++
    ) {
        /*COMPARE_ENTITY ( entityA, entityB )
        COMPARE_ENTITY ( entityB, entityA )*/
        
        if (state.data[i].entityA == iterator->entityId) {
            iterator->other = state.data[i].entityB;
            iterator->normal = state.data[i].normalA;
            iterator->collisionData = &(state.data[i]);
            iterator->lastIndex = i+1;
            return true;
        }
        if (state.data[i].entityB == iterator->entityId) {
            iterator->other = state.data[i].entityA;
            iterator->normal = Vector2Negate( state.data[i].normalA );
            iterator->collisionData = &(state.data[i]);
            iterator->lastIndex = i+1;
            return true;
        }
    }
    iterator->lastIndex = hc->lastCollisionIndex;
    return false;
}

//---


void IntersectLayers(Layer a, Layer b) 
{
    g_layerMask[a] |= MASK(b);
    g_layerMask[b] |= MASK(a);
}
void InitLayers() 
{
    IntersectLayers(LN_PLAYER, LN_ENEMY);
    IntersectLayers(LN_PLAYER, LN_WALL);
    IntersectLayers(LN_PLAYER, LN_EN_BULLET);
    IntersectLayers(LN_ENEMY,  LN_WALL);
    IntersectLayers(LN_ENEMY, LN_PL_BULLET);
    IntersectLayers(LN_WALL, LN_PL_BULLET);
    IntersectLayers(LN_WALL, LN_EN_BULLET);
    IntersectLayers(LN_PLAYER, LN_PL_TRIGGER);
}

void InitCollisionWorldState()
{
    state.count = 0;
    state.cap = PH_INITIAL_CAPACITY;
    state.data = malloc(PH_INITIAL_CAPACITY * sizeof(CollisionData));
}

int NewCollision() 
{
    if (state.count >= state.cap) {
        CollisionData* newData = realloc(state.data, state.cap * 2 * sizeof(CollisionData));
        if (newData == NULL) {
            printf("Realloc fail %s:%d\n", __FILE__, __LINE__);
            exit(1);
        }
        
        state.data = newData;
        state.cap *= 2;
    }
    
    return state.count++;
}

void AddCollisionToEntity(
    uint32_t entityId, 
    uint32_t otherEntityId, 
    uint32_t collisionId
) {
    HasCollisionsComponent* hasCollisions;
    bool first = false;
    if(!ecs_has(entityId, CID_HasCollisions)) {
        ecs_add(entityId, CID_HasCollisions, &hasCollisionsTemplate);
        first = true;
    }
    HasCollisionsComponent* hc = (HasCollisionsComponent*) ecs_get(entityId, CID_HasCollisions);
    
    if(first)
        hc->firstCollisionIndex = collisionId;
    hc->lastCollisionIndex = collisionId;
}


bool Collision_Circle2Circle(PositionComponent* pos1, PositionComponent* pos2, ShapeCircle *circle1, ShapeCircle *circle2, CollisionData *collision)
{
    Vector2 delta = Vector2Subtract(*pos2, *pos1);
    float distSqr = Vector2LengthSqr(delta);
    float minDist = circle1->radius + circle2->radius;
    bool result = (distSqr < minDist * minDist);
    if (!result) return false;
    
    collision->depth = minDist - sqrt(distSqr);
    collision->normalA = Vector2Normalize(delta);
    
    return result;
}

bool Collision_Circle2Line(PositionComponent* pos1, PositionComponent* pos2, ShapeCircle *circle1, ShapeLine *line2)
{
    /*
        projecting circle position (point c) to line (points a and b)
        c1 is projected point
        vectors are named by points, ex. ab = vector from a to b
        prefix l_ means vector's length
    */
    
    Vector2 a = Vector2Add ( *pos2, line2->start );
    Vector2 ab = Vector2Subtract ( line2->finish, line2->start );
    //Vector2 c = *pos1; // Vector2Add ( *pos1, circle1->offset ); //ignoring offsets for collisions
    Vector2 ac = Vector2Subtract ( *pos1, a );
    
    float l_ab = Vector2Length (ab);
    float l_ac1 = Vector2DotProduct( ab, ac ) / l_ab;
    
    float minDistSqr = circle1->radius * circle1->radius;
    
    if (l_ac1 <= 0)
    {
        return Vector2LengthSqr ( ac ) < minDistSqr;
    }
    if (l_ac1 >= l_ab) 
    {
        Vector2 b = Vector2Add ( *pos2, line2->finish );
        Vector2 bc = Vector2Subtract ( *pos1, b );
        return Vector2DistanceSqr ( *pos1, b ) < minDistSqr;
    }
    
    
    Vector2 cc1 = Vector2Subtract (
        Vector2Scale ( 
            ab,
            l_ac1 / l_ab
        ),
        ac
    );
    
    //DrawText( TextFormat("%f ; %f", l_ac1, l_ab) , 64, 1, 16, DARKGREEN );
    
    return Vector2LengthSqr ( cc1 ) < minDistSqr;
}

bool Collision_Line2Line(PositionComponent* pos1, PositionComponent* pos2, ShapeLine *line1, ShapeLine *line2)
{
    Vector2 collisionPoint = { 0 };
    
    return CheckCollisionLines(
        Vector2Add( *pos1, line1->start ),
        Vector2Add( *pos1, line1->finish ),
        Vector2Add( *pos2, line2->start ),
        Vector2Add( *pos2, line2->finish ),
        &collisionPoint
    );
}
