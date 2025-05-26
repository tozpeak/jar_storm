//#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

typedef Vector2 PositionComponent;
typedef Vector2 VelocityComponent;
typedef struct 
{
    Vector2 offset;
    //Vector2 size;
    Color color;
} DrawRectangleComponent;
typedef int StateFlagsComponent;


const uint32_t CID_Position = 0;
const uint32_t CID_Velocity = 1;
const uint32_t CID_DrawRectangle = 2;
const uint32_t CID_StateFlags = 3;
//update Count accordingly
const uint32_t CID_Count = 4;

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

void System_Draw() 
{
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Position, CID_DrawRectangle);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		DrawRectangleComponent *rect = (DrawRectangleComponent*)ecs_get(qr->list[i], CID_DrawRectangle);
		DrawPixelV(*pos, rect->color);
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

void AddBullet(Vector2 aimFrom, Vector2 aimDirection) 
{
    Vector2 velocity = Vector2Scale(aimDirection, 16);
    //aimFrom = Vector2Add(aimFrom, velocity);
    
    Entity e = ecs_create();
    PositionComponent pos = aimFrom;
    VelocityComponent vel = velocity;
    DrawRectangleComponent rect = { Vector2Zero(), WHITE };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawRectangle, &rect );/**/
}

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	//SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	// Create the window and OpenGL context
	InitWindow(640, 480, "Hello Raylib");
    
    Vector2 playerPos = { 32, 32 };
    Vector2 playerSize = { 12, 24 };
    Vector2 aimFromOffset = { 0, -playerSize.y / 2 };
    Vector2 aimDirection = { 0, 0 };
    float speed = 16 * 4;
    
    ecs_init(CID_Count, 
        sizeof(PositionComponent), 
        sizeof(VelocityComponent), 
        sizeof(DrawRectangleComponent), 
        sizeof(StateFlagsComponent)
    );

	// game loop
	while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
		// drawing
		BeginDrawing();

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
        
        if (IsKeyDown(KEY_SPACE)) AddBullet(aimFrom, aimDirection);
        
        System_Move(delta);
        //System_ClearOutOfBounds();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);
		
		DrawChessboard();

		DrawCharacter(playerPos, playerSize);
		DrawGun(aimFrom, aimDirection);
		
        System_Draw();
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}
	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
