#pragma once

typedef char Layer;

enum LayerName
{
    LN_PLAYER = 0,
    LN_ENEMY = 1,
    LN_WALL = 2,
    LN_PL_BULLET = 3,
    LN_EN_BULLET = 4,
    
    LN_COUNT,
};

extern Layer g_layerMask[];

#define MASK( _layerName ) 1 << _layerName

//void IntersectLayers(Layer a, Layer b);
void InitPhysics();
void System_Collide(float deltaTime);
void System_ClearCollisions();
