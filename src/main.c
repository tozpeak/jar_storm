//#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

typedef char Layer;
typedef Vector2 PositionComponent;
typedef Vector2 VelocityComponent;
typedef struct
{
    float radius;
    Layer layer;
} ColliderComponent;
typedef struct 
{
    Vector2 offset;
    float radius;
    //Vector2 size;
    Color color;
} DrawRectangleComponent;
typedef int StateFlagsComponent;

enum ComponentId
{
    CID_Position = 0,
    CID_Velocity,
    CID_Collider,
    CID_DrawRectangle,
    CID_StateFlags,
    CID_HasCollision,
    
    CID_Count
};

enum LayerName
{
    LN_PLAYER = 0,
    LN_ENEMY = 1,
    LN_WALL = 2,
    LN_PL_BULLET = 3,
    LN_EN_BULLET = 4,
    
    LN_COUNT,
};

Layer g_layerMask[LN_COUNT] = { 0 };

#define MASK( _layerName ) 1 << _layerName

void IntersectLayers(Layer a, Layer b) 
{
    g_layerMask[a] |= MASK(b);
    g_layerMask[b] |= MASK(a);
}

void InitLayers() 
{
    IntersectLayers(LN_PLAYER, LN_ENEMY);
    IntersectLayers(LN_PLAYER, LN_WALL);
    IntersectLayers(LN_PLAYER, LN_EN_BULLET);
    IntersectLayers(LN_ENEMY,  LN_WALL);
    IntersectLayers(LN_ENEMY, LN_PL_BULLET);
    IntersectLayers(LN_WALL, LN_PL_BULLET);
    IntersectLayers(LN_WALL, LN_EN_BULLET);
}

void DrawChessboard() 
{
    int screenX = 640, screenY = 480;
    int tileSize = 16;
    int offsetTiles = 1;
    Color tileColor = DARKGRAY;
    for (int i = offsetTiles; (i + offsetTiles) * tileSize < screenX; i++) 
    {
        for (int j = offsetTiles + (i % 2); (j + offsetTiles) * tileSize < screenY; j += 2) 
        {
            DrawRectangle(
                i * tileSize,
                j * tileSize,
                tileSize,
                tileSize,
                tileColor
            );
        }
    }
}

void DrawCharacter (Vector2 position, Vector2 size) 
{
    Vector2 offset = { -size.x / 2, -size.y };
    Color charColor = GREEN;
    
    DrawRectangleV( Vector2Add(position, offset), size, charColor );
}

void DrawGun (Vector2 aimFrom, Vector2 aimDirection) 
{
    float aimLineLength = 12;
    
    DrawLineEx (
        aimFrom,
        Vector2Add( aimFrom, Vector2Scale(aimDirection, aimLineLength) ),
        5,
        DARKGREEN
    );
}

void System_Move(float deltaTime) 
{
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Position, CID_Velocity);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
		pos->x += vel->x * deltaTime;
		pos->y += vel->y * deltaTime;
		/*if (pos->x > RENDER_WIDTH || pos->x < 0)
			vel->x *= -1;
		if (pos->y > RENDER_HEIGHT || pos->y < 0)
			vel->y *= -1;*/
	}
}

void System_Collide(float deltaTime)
{
    uint32_t i, j;
	QueryResult *qr = ecs_query(2, CID_Position, CID_Collider);
	uint32_t* list = qr->list;
	for (i = 0; i < qr->count; ++i) {
	    uint32_t ent = list[i];
		PositionComponent *pos = (PositionComponent*)ecs_get(ent, CID_Position);
		ColliderComponent *col = (ColliderComponent*)ecs_get(ent, CID_Collider);
		Layer layerMask = g_layerMask[col->layer];
		
		for (j = i+1; j < qr->count; ++j) {
	        uint32_t entB = list[j];
		    ColliderComponent *colB = (ColliderComponent*)ecs_get(entB, CID_Collider);
		    
		    if( !(MASK(colB->layer) & layerMask) ) continue;
		    
		    PositionComponent *posB = (PositionComponent*)ecs_get(entB, CID_Position);
		    float distSqr = Vector2DistanceSqr(*pos, *posB);
		    float minDist = col->radius + colB->radius;
		    if(distSqr > minDist * minDist) continue;
		    
		    ecs_add(ent, CID_HasCollision, NULL);
		    ecs_add(entB, CID_HasCollision, NULL);
		}
	}
}

void System_Draw() 
{
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Position, CID_DrawRectangle);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		DrawRectangleComponent *rect = (DrawRectangleComponent*)ecs_get(qr->list[i], CID_DrawRectangle);
		Color color = rect->color;
		
		if(rect->radius < 0.1f) DrawPixelV(*pos, color);
		else DrawCircleV(*pos, rect->radius, color);
	}
}

void System_DrawDebugCollisions()
{
    uint32_t i;
	QueryResult *qr = ecs_query(3, CID_Position, CID_DrawRectangle, CID_HasCollision);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		DrawRectangleComponent *rect = (DrawRectangleComponent*)ecs_get(qr->list[i], CID_DrawRectangle);
		Color color = RED;
		
		if(rect->radius < 0.1f) DrawPixelV(*pos, color);
		else DrawCircleV(*pos, rect->radius, color);
	}
}

void System_ClearOutOfBounds() 
{
    uint32_t i;
	QueryResult *qr = ecs_query(1, CID_Position);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		if (pos->x > 1000 
	     || pos->x < 0
	     || pos->y > 1000 
	     || pos->y < 0
	    ) ecs_kill(qr->list[i]);
	}
}

void System_ClearCollisions() 
{
    uint32_t i;
	QueryResult *qr = ecs_query(1, CID_HasCollision);
	for (i = 0; i < qr->count; ++i) {
		ecs_remove(qr->list[i], CID_HasCollision);
	}
}

void AddBullet(Vector2 aimFrom, Vector2 aimDirection) 
{
    Vector2 velocity = Vector2Scale(aimDirection, 16);
    //aimFrom = Vector2Add(aimFrom, velocity);
    
    Entity e = ecs_create();
    PositionComponent pos = aimFrom;
    VelocityComponent vel = velocity;
    DrawRectangleComponent rect = { Vector2Zero(), 0, WHITE };
    ColliderComponent col = { 0.5f , (Layer)LN_PL_BULLET };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawRectangle, &rect );
    ecs_add(e.id, CID_Collider, &col );
}

void AddPillar(Vector2 position) 
{
    float radius = 3.0f;
    Entity e = ecs_create();
    PositionComponent pos = position;
    DrawRectangleComponent rect = { Vector2Zero(), radius, WHITE };
    ColliderComponent col = { radius , (Layer)LN_WALL };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawRectangle, &rect );
    ecs_add(e.id, CID_Collider, &col );
}

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	//SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	//SetTargetFPS(120);

	// Create the window and OpenGL context
	InitWindow(640, 480, "Hello Raylib");
	
	InitLayers();
    
    Vector2 playerPos = { 32, 32 };
    Vector2 playerSize = { 12, 24 };
    Vector2 aimFromOffset = { 0, -playerSize.y / 2 };
    Vector2 aimDirection = { 0, 0 };
    float speed = 16 * 4;
    float shotCooldownState = 0;
    float shotCooldown = 0.01f;
    
    ecs_init(CID_Count, 
        sizeof(PositionComponent), 
        sizeof(VelocityComponent), 
        sizeof(ColliderComponent), 
        sizeof(DrawRectangleComponent),      
        sizeof(StateFlagsComponent),
        0 //CID_HasCollision
    );
    
    for (int i = 1; i < 16; i+=4) {
        for (int j = 1; j < 16; j+=4) {
            AddPillar((Vector2) { i * 16, j * 16 } );
        }
    }

	// game loop
	while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
        float delta = GetFrameTime();

        float dSpeed = speed * delta;
        
        if (IsKeyDown(KEY_D)) playerPos.x += dSpeed;
        if (IsKeyDown(KEY_A)) playerPos.x -= dSpeed;
        if (IsKeyDown(KEY_W)) playerPos.y -= dSpeed;
        if (IsKeyDown(KEY_S)) playerPos.y += dSpeed;
        
        Vector2 aimTo = GetMousePosition();
        Vector2 aimFrom = Vector2Add (playerPos, aimFromOffset);
        
        aimDirection = Vector2Subtract(aimTo, aimFrom);
        aimDirection = Vector2Normalize(aimDirection);
        
        shotCooldownState += delta;
        
        if (IsKeyDown(KEY_SPACE) && (shotCooldownState > shotCooldown)) {
            AddBullet(aimFrom, aimDirection);
            shotCooldownState = 0;
        }
        
        System_Move(delta);
        System_ClearCollisions();
        System_Collide(delta);
        System_ClearOutOfBounds();
        
		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);
		
		DrawChessboard();

		DrawCharacter(playerPos, playerSize);
		DrawGun(aimFrom, aimDirection);
		
        System_Draw();
        System_DrawDebugCollisions();
        
        DrawFPS(1, 1);
        DrawText(
            TextFormat("%d", ecs_query(0)->count),
            1, 24,
            16, DARKGREEN
        );
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}
	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
