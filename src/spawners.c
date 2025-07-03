#include <stdlib.h>
#include <components.h>
#include <physics.h>
#include <shapes.h>
#include <attacks.h>

#include <spawners.h>

void SetAttackSpawnCooldown(AttackAbility *attack)
{
    attack->state = ATK_ST_COOLDOWN;
    attack->cooldown = 5;
}

Entity Spawn_Projectile(Vector2 projPosition, short dmg, short maxHp, AttackContext *context)
{
    if( ecs_has(context->entityId, CID_Stats) ) {
        StatsComponent *stats = (StatsComponent*) ecs_get(context->entityId, CID_Stats);
        
        dmg     = (short) (dmg      * stats->dmgMult);
        maxHp   = (short) (maxHp    * stats->dmgMult);
    }
    
    Entity e = ecs_create();
    
    DealDamageComponent dam = { dmg, DMG_TARGET_SELF | DMG_TARGET_OTHER };
    HealthComponent hp = { maxHp, maxHp };
    ParentIdComponent parentId = context->entityId;
    
    ecs_add(e.id, CID_Position, &projPosition);
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_ParentId, &parentId );
    
    return e;
}

void Spawn_AddVelocity(Entity e, float linearVelocity, Vector2 aimDirection)
{
    VelocityComponent vel = Vector2Scale(aimDirection, linearVelocity);
    ecs_add(e.id, CID_Velocity, &vel );
}

void Spawn_AddShape(Entity e, Shape shape, Color color, Layer layer)
{
    DrawShapeComponent draw = { 
        color, shape
    };
    ColliderComponent col = { 
        shape, 
        layer
    };
    
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
}

void Spawn_AddShapeBullet(Entity e, Color color, Layer layer)
{
    VelocityComponent *velocity = (VelocityComponent*) ecs_get(e.id, CID_Velocity);
    Shape shape = Shapes_NewLine_0(
        Vector2Scale( *velocity, 0.02f )
    );
    Spawn_AddShape( e, shape, color, layer );
}

void Spawn_AddShapeFromConfig(Entity e, AttackProjectile *config, Layer layer)
{
    if(config->radius > 0) {
        Spawn_AddShape(
            e, 
            Shapes_NewCircle_0(config->radius),
            config->color, 
            layer
        );
    }
    else Spawn_AddShapeBullet(e, config->color, layer);
}

Entity Spawn_BuildGenericProjectile(
    Vector2 projPosition, 
    Vector2 projDirection, 
    Layer layer, 
    AttackContext *context
) {
    AttackProjectile *config = &( 
        Attack_GetConfigFor(context->ability->attackId)->projectile
    );

    Entity e = Spawn_Projectile(
        projPosition,
        config->baseDmg, 
        config->hp, 
        context
    );
    
    if(config->velocity > 0) Spawn_AddVelocity(e, config->velocity, projDirection);
    
    Spawn_AddShapeFromConfig(e, config, layer);
    
    return e;
}


uint32_t Spawn_Pillar(Vector2 position)
{
    float radius = 16 * 1.25f;
    Entity e = ecs_create();
    PositionComponent pos = position;
    
    DrawShapeComponent shape = { BEIGE, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = {
        Shapes_NewCircle(Vector2Zero(), radius),
        (Layer)LN_WALL
    };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_StaticCollider, NULL );
    
    return e.id;
}

uint32_t Spawn_Teleporter(Vector2 position) 
{
    float radius = 6.0f;
    float triggerRadius = 16 * 12;
    Entity e = ecs_create();
    PositionComponent pos = position;
    
    DrawShapeComponent shape = { RED, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), triggerRadius), 
        (Layer)LN_PL_TRIGGER
    };
    HealthComponent hp = { 100, 100 };
    DealDamageComponent dam = { 1, DMG_TARGET_SELF | DMG_ON_TICK };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_HasHpBar, NULL );
    
    return e.id;
}

void Spawn_AddCoins(Entity e, short amount)
{
    CoinsComponent coins = { amount };
    
    ecs_add(e.id, CID_Coins, &coins);
}

uint32_t Spawn_Enemy(Vector2 position) 
{
    float radius = 6.0f;
    Entity e = ecs_create();
    PositionComponent pos = position;
	VelocityComponent vel = Vector2Rotate(
	    (Vector2) { 5, 0 },
	    (rand() % 628) / 100.0f
	);
    DrawShapeComponent shape = { WHITE, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), radius), 
        (Layer)LN_ENEMY 
    };
    HealthComponent hp = { 24, 32 };
    
    PrimaryAttackComponent primAtt = { .attackId = ATK_ID_MELEE_BITE };
    SetAttackSpawnCooldown(&primAtt);
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_PrimaryAttack, &primAtt );
    ecs_add(e.id, CID_Rigidbody, NULL );
    ecs_add(e.id, CID_IsWanderer, NULL );
    ecs_add(e.id, CID_AiAttack, NULL );
    ecs_add(e.id, CID_HasHpBar, NULL );
    ecs_add(e.id, CID_HasGravity, NULL );
    
    Spawn_AddCoins(e, 3);
    
    return e.id;
}

uint32_t Spawn_Enemy_Lizard(Vector2 position) 
{
    float radius = 8.0f;
    Entity e = ecs_create();
    PositionComponent pos = position;
	VelocityComponent vel = Vector2Rotate(
	    (Vector2) { 5, 0 },
	    (rand() % 628) / 100.0f
	);
    DrawShapeComponent shape = { VIOLET, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), radius), 
        (Layer)LN_ENEMY 
    };
    HealthComponent hp = { 48, 48 };
    
    PrimaryAttackComponent primAtt = { .attackId = ATK_ID_MELEE_CLAW };
    SecondaryAttackComponent secAtt = { .attackId = ATK_ID_SHOT_FIREBALL };
    SetAttackSpawnCooldown(&primAtt);
    SetAttackSpawnCooldown(&secAtt);
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_PrimaryAttack, &primAtt );
    ecs_add(e.id, CID_SecondaryAttack, &secAtt );
    ecs_add(e.id, CID_Rigidbody, NULL );
    ecs_add(e.id, CID_IsWanderer, NULL );
    ecs_add(e.id, CID_AiAttack, NULL );
    ecs_add(e.id, CID_HasHpBar, NULL );
    ecs_add(e.id, CID_HasGravity, NULL );
    
    Spawn_AddCoins(e, 8);
    
    return e.id;
}

Entity Spawn_Player(Vector2 position, char id) 
{
    float radius = 6.0f;
    Entity e = ecs_create();
    PositionComponent pos = position;
	VelocityComponent vel = Vector2Zero();
    DrawShapeComponent shape = { 
        (Color){ 0, 0, 0, 127 }, //SHADOW
        Shapes_NewCircle(Vector2Zero(), radius) 
    };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), radius), 
        (Layer)LN_PLAYER
    };
    HealthComponent hp = { 120, 120 };
    
    PrimaryAttackComponent primAtt = { .attackId = ATK_ID_SHOT_PISTOLS };
    SecondaryAttackComponent secAtt = { .attackId = ATK_ID_SHOT_ENERGY_BLAST };
    
    StatsComponent baseStats = {
        .velocity = 16 * 4,
        .attackSpeedMult = 1,
        .dmgMult = 1,
    };
    CoinsComponent coins = { 0 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_PrimaryAttack, &primAtt );
    ecs_add(e.id, CID_SecondaryAttack, &secAtt );
    ecs_add(e.id, CID_PlayerId, &id );
    ecs_add(e.id, CID_Stats, &baseStats );
    ecs_add(e.id, CID_Coins, &coins );
    ecs_add(e.id, CID_Rigidbody, NULL );
    ecs_add(e.id, CID_HasHpBar, NULL );
    ecs_add(e.id, CID_HasGravity, NULL );
    ecs_add(e.id, CID_PlayerInput, NULL );
    
    Entity parent = ecs_create();
    ecs_add(parent.id, CID_Stats, &baseStats);
    
    ecs_add(e.id, CID_ParentId, &(parent.id));
    
    return e;
} 

uint32_t Spawn_Interactable(Vector2 position)
{
    float radius = 6.0f;
    Entity e = ecs_create();
    PositionComponent pos = position;
    Shape shape = Shapes_NewCircle_0(radius);
    DrawShapeComponent draw = { BLUE, shape };
    ColliderComponent col = { 
        shape,
        (Layer)LN_EN_BULLET
    };
    
    PrimaryAttackComponent primAtt = { .attackId = ATK_ID_EVENT_GIVE_COINS };
    CoinsComponent coins = { 5 };
    
    int type = rand() % 2;
    
    switch(type) {
    case 0: //chest
        primAtt.attackId = ATK_ID_EVENT_GIVE_RANDOM_ITEM;
        ecs_add(e.id, CID_PriceInCoins, NULL);
        break;
    case 1: //coins pot
        primAtt.attackId = ATK_ID_EVENT_GIVE_COINS;
        draw.shape.circle.radius = 4.0f;
        break;
    }
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_PrimaryAttack, &primAtt );
    ecs_add(e.id, CID_Coins, &coins );
    ecs_add(e.id, CID_IsInteractable, NULL );
    
    return e.id;
}

uint32_t Spawn_RandomItem(Vector2 position)
{
    float radius = 3.0f;
    Entity e = ecs_create();
    PositionComponent pos = position;
    Shape shape = Shapes_NewCircle_0(radius);
    DrawShapeComponent draw = { LIME, shape };
    ColliderComponent col = { 
        shape,
        (Layer)LN_EN_BULLET
    };
    
    ItemComponent item = { 
        .count = 1,
        .type = rand() % 4
    };
    
    StatsComponent stats = { 0 };
    switch (item.type) {
        case 0: stats.velocity = 16 * 4 * 0.15f; break;
        case 1: stats.attackSpeedMult = 0.15f; break;
        case 2: stats.dmgMult = 0.25f; break;
        case 3: {
            PrimaryAttackComponent primAtt = { .attackId = ATK_ID_ITEM_MUSHROOM_SET };
            SecondaryAttackComponent secAtt = { .attackId = ATK_ID_ITEM_MUSHROOM_RESET };
            
            ecs_add(e.id, CID_PrimaryAttack, &primAtt);
            ecs_add(e.id, CID_SecondaryAttack, &secAtt);
            ecs_add(e.id, CID_AiAttack, NULL);
        } break;
    }
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Item, &item );
    ecs_add(e.id, CID_Stats, &stats );
    
    return e.id;
}
