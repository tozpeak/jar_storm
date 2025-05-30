#include <stdint.h>
#include <stddef.h>
#include <ecs.h>

#include <components.h>

void InitComponents() 
{
    ecs_init(CID_Count, 
        sizeof(PositionComponent), 
        sizeof(VelocityComponent), 
        sizeof(ColliderComponent), 
        sizeof(DrawRectangleComponent),
        sizeof(HealthComponent),
        sizeof(StateFlagsComponent),
        0, //CID_HasCollision
        0, //CID_IsBullet
        0, //CID_IsDead
        0 //CID_IsWanderer
    );
}
