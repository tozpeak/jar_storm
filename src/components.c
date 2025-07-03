#include <stdint.h>
#include <stddef.h>
#include <ecs.h>

#include <components.h>
#include <shapes.h>

void InitComponents() 
{
    ecs_init(CID_Count, 
        sizeof(DynamicVector), //CID_Position
        sizeof(DynamicVector), //CID_Velocity
        sizeof(ColliderComponent), 
        sizeof(HasCollisionsComponent), 
        sizeof(DrawShapeComponent),
        
        sizeof(HealthComponent),
        sizeof(DealDamageComponent),
        
        sizeof(PrimaryAttackComponent),
        sizeof(SecondaryAttackComponent),
        sizeof(AttackIntentionComponent),
        sizeof(PlayerIdComponent),
        
        sizeof(ParentIdComponent),
        sizeof(StatsComponent),
        sizeof(ItemComponent),
        
        sizeof(CoinsComponent),
        sizeof(TargetIdComponent),
        //sizeof(EventComponent),
        
        sizeof(StateFlagsComponent),
        
        0, //CID_StaticCollider
        0, //CID_Rigidbody
        0, //CID_HasHpBar
        0, //CID_HasGravity
        0, //CID_IsKilled
        0, //CID_IsWanderer
        0, //CID_AiAttack
        0, //CID_PlayerInput
        0, //CID_InventoryIsDirty
        0, //CID_IsInteractable
        0, //CID_PriceInCoins
        
        0, //CID_EventInteraction
        0, //CID_EventKill
        
        0 //CID_Count
    );
}
