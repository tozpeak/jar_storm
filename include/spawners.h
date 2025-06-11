#pragma once

#include <raymath.h>
#include <ecs.h>

void Spawn_Melee(Vector2 aimDirection);
void Spawn_Fireball(Vector2 aimFrom, Vector2 aimDirection, float speed);
void Spawn_Bullet(Vector2 aimFrom, Vector2 aimDirection, float speed);
void Spawn_BigBullet(Vector2 aimFrom, Vector2 aimDirection, float speed);
uint32_t Spawn_Pillar(Vector2 position);
uint32_t Spawn_Teleporter(Vector2 position);
uint32_t Spawn_Enemy(Vector2 position);
uint32_t Spawn_Enemy_Lizard(Vector2 position);
Entity Spawn_Player(Vector2 position, char id);
