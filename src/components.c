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
        
        sizeof(PrimaryAttackComponent),
        sizeof(SecondaryAttackComponent),
        sizeof(AttackIntentionComponent),
        sizeof(PlayerIdComponent),
        
        sizeof(StateFlagsComponent),
        
        0, //CID_HasHpBar
        0, //CID_IsKilled
        0, //CID_IsWanderer
        0 //CID_AiAttack
    );
}
