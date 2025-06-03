#include <stdint.h>
#include <stddef.h>
#include <ecs.h>

#include <components.h>
#include <shapes.h>

void InitComponents() 
{
    ecs_init(CID_Count, 
        sizeof(PositionComponent), 
        sizeof(VelocityComponent), 
        sizeof(ColliderComponent), 
        sizeof(HasCollisionsComponent), 
        sizeof(DrawShapeComponent),
        sizeof(HealthComponent),
        sizeof(DealDamageComponent),
        sizeof(StateFlagsComponent),
        0, //CID_IsBullet
        0, //CID_IsDead
        0 //CID_IsWanderer
    );
}
