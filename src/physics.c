#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ecs.h>

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
		    
		    PositionComponent *posB = (PositionComponent*)ecs_get(entB, CID_Position);
		    float distSqr = Vector2DistanceSqr(*pos, *posB);
		    float minDist = col->radius + colB->radius;
		    if(distSqr > minDist * minDist) continue;
		    
		    uint32_t collisionId = NewCollision();
		    CollisionData* collision = &( state.data[collisionId] );
		    collision->entityA = ent;
		    collision->entityB = entB;
		    
		    AddCollisionToEntity(ent, entB, collisionId);
		    AddCollisionToEntity(entB, ent, collisionId);
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
            iterator->collisionData = &(state.data[i]);
            iterator->lastIndex = i+1;
            return true;
        }
        if (state.data[i].entityB == iterator->entityId) {
            iterator->other = state.data[i].entityA;
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

