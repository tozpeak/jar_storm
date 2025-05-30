#include <stdint.h>
#include <stddef.h>
#include <ecs.h>

#include <components.h>
#include <physics.h>

Layer g_layerMask[LN_COUNT] = { 0 };

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

void InitPhysics() 
{
    IntersectLayers(LN_PLAYER, LN_ENEMY);
    IntersectLayers(LN_PLAYER, LN_WALL);
    IntersectLayers(LN_PLAYER, LN_EN_BULLET);
    IntersectLayers(LN_ENEMY,  LN_WALL);
    IntersectLayers(LN_ENEMY, LN_PL_BULLET);
    IntersectLayers(LN_WALL, LN_PL_BULLET);
    IntersectLayers(LN_WALL, LN_EN_BULLET);
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
		    
		    ecs_add(ent, CID_HasCollision, NULL);
		    ecs_add(entB, CID_HasCollision, NULL);
		}
	}
}

void System_ClearCollisions() 
{
    uint32_t i;
	QueryResult *qr = ecs_query(1, CID_HasCollision);
	for (i = 0; i < qr->count; ++i) {
		ecs_remove(qr->list[i], CID_HasCollision);
	}
}
