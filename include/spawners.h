#pragma once

#include <raymath.h>
#include <ecs.h>


Entity Spawn_Projectile(
    Vector2 projPosition, 
    short dmg, 
    short maxHp, 
    AttackContext *context
);
void Spawn_AddVelocity(Entity e, float linearVelocity, Vector2 aimDirection);
void Spawn_AddShape(Entity e, Shape shape, Color color, Layer layer);
void Spawn_AddShapeBullet(Entity e, Color color, Layer layer);
void Spawn_AddShapeFromConfig(Entity e, AttackProjectile *config, Layer layer);

Entity Spawn_BuildGenericProjectile(
    Vector2 projPosition, 
    Vector2 projDirection, 
    Layer layer, 
    AttackContext *context
);

uint32_t Spawn_Pillar(Vector2 position);
uint32_t Spawn_Teleporter(Vector2 position);
uint32_t Spawn_Enemy(Vector2 position);
uint32_t Spawn_Enemy_Lizard(Vector2 position);
Entity Spawn_Player(Vector2 position, char id);
uint32_t Spawn_Interactable(Vector2 position);
uint32_t Spawn_RandomItem(Vector2 position);
