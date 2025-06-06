#include <stdlib.h>
#include <components.h>
#include <physics.h>
#include <shapes.h>
#include <attacks.h>

#include <spawners.h>

void Spawn_Melee(Vector2 aimDirection)
{
    float radius = 6.0f;
    Entity e = ecs_create();
    PositionComponent pos = aimDirection;
    Shape shape = Shapes_NewCircle(
        Vector2Zero(),
        radius
    );
    DrawShapeComponent draw = { 
        ORANGE, shape
    };
    ColliderComponent col = { 
        shape, 
        (Layer)LN_EN_BULLET 
    };
    DealDamageComponent dam = { 24, DMG_SELF | DMG_OTHER };
    HealthComponent hp = { 1, 1 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_Health, &hp );
}


void Spawn_Fireball(Vector2 aimFrom, Vector2 aimDirection, float speed) 
{
    float radius = 6.0f;
    Vector2 velocity = Vector2Scale(aimDirection, speed);
    
    Entity e = ecs_create();
    PositionComponent pos = aimFrom;
    VelocityComponent vel = velocity;
    DrawShapeComponent shape = { ORANGE, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), radius), 
        (Layer)LN_EN_BULLET 
    };
    DealDamageComponent dam = { 36, DMG_SELF | DMG_OTHER };
    HealthComponent hp = { 1, 1 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_Health, &hp );
    //ecs_add(e.id, CID_HasHpBar, NULL );
}

void Spawn_Bullet(Vector2 aimFrom, Vector2 aimDirection, float speed) 
{
    Vector2 velocity = Vector2Scale(aimDirection, speed);
    
    Entity e = ecs_create();
    PositionComponent pos = aimFrom;
    VelocityComponent vel = velocity;
    Shape shape = Shapes_NewLine(
        Vector2Zero(),
        Vector2Scale( velocity, 0.02f )
    );
    DrawShapeComponent draw = { 
        YELLOW, shape
    };
    ColliderComponent col = { 
        shape, 
        (Layer)LN_PL_BULLET 
    };
    DealDamageComponent dam = { 4, DMG_SELF | DMG_OTHER };
    HealthComponent hp = { 1, 1 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_Health, &hp );
}

void Spawn_BigBullet(Vector2 aimFrom, Vector2 aimDirection, float speed) 
{
    float radius = 10.0f;
    Vector2 velocity = Vector2Scale(aimDirection, speed);
    
    Entity e = ecs_create();
    PositionComponent pos = aimFrom;
    VelocityComponent vel = velocity;
    DrawShapeComponent shape = { SKYBLUE, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), radius), 
        (Layer)LN_PL_BULLET 
    };
    DealDamageComponent dam = { 4, DMG_SELF | DMG_OTHER };
    HealthComponent hp = { 36, 36 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_Health, &hp );
    //ecs_add(e.id, CID_HasHpBar, NULL );
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
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_PrimaryAttack, &primAtt );
    ecs_add(e.id, CID_IsWanderer, NULL );
    ecs_add(e.id, CID_AiAttack, NULL );
    ecs_add(e.id, CID_HasHpBar, NULL );
    
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
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_PrimaryAttack, &primAtt );
    ecs_add(e.id, CID_SecondaryAttack, &secAtt );
    ecs_add(e.id, CID_IsWanderer, NULL );
    ecs_add(e.id, CID_AiAttack, NULL );
    ecs_add(e.id, CID_HasHpBar, NULL );
    
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
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_PrimaryAttack, &primAtt );
    ecs_add(e.id, CID_SecondaryAttack, &secAtt );
    ecs_add(e.id, CID_PlayerId, &id );
    ecs_add(e.id, CID_HasHpBar, NULL );
    ecs_add(e.id, CID_PlayerInput, NULL );
    
    return e;
} 
