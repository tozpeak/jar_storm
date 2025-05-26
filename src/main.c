//#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

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

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);
		
		DrawChessboard();

		DrawCharacter(playerPos, playerSize);
		DrawGun(aimFrom, aimDirection);
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}
	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
