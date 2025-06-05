#pragma once

#include <raymath.h>
#include <ecs.h>

void Spawn_Melee(Vector2 aimDirection);
void Spawn_Bullet(Vector2 aimFrom, Vector2 aimDirection, float speed);
void Spawn_BigBullet(Vector2 aimFrom, Vector2 aimDirection, float speed);
uint32_t Spawn_Enemy(Vector2 position);
Entity Spawn_Player(Vector2 position, char id);
