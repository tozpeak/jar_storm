#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

#include <components.h>
#include <level.h>
#include <spawners.h>
#include <ecs_helpers.h>
#include <systems/gameplay_logic.h>
#include <systems/gameplay_draw_world.h>
#include <systems/gameplay_draw_hud.h>
#include <game_state.h>


int test_main();

int test_limits()
{
    if (CID_Count > 32) {
        printf( "CID_Count is %d > 32!\n"
                "Move flag components to separate bitmask or make the current mask bigger.\n"
                , CID_Count);
        return 1;
    }
    return 0;
}

int main ()
{
    int limitsResult = test_limits();
    if(limitsResult > 0) return limitsResult;
    
    //return test_main();

    // Tell the window to use vsync and work on high DPI displays
    //SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    //SetTargetFPS(120);

    // Create the window and OpenGL context
    const float screenScale = 2.f;
    Vector2Int windowSize = {
        (int) floor(g_screenSettings.width  * screenScale),
        (int) floor(g_screenSettings.height * screenScale),
    };

    InitWindow(windowSize.x, windowSize.y, "Hello Raylib");
    SetExitKey(KEY_NULL);
    //SetWindowSize(g_screenSettings.width * screenScale, g_screenSettings.height * screenScale);

    Vector2 centerScreenOffset = (Vector2){ g_screenSettings.width / 2.f, g_screenSettings.height / 2.f };
    
    Camera2D *camera = g_screenSettings.camera;
    camera->target = (Vector2){ 0 };
    camera->offset = Vector2Scale(centerScreenOffset, screenScale);
    camera->rotation = 0.0f;
    //camera->zoom = screenScale * g_screenSettings.width / (g_level.width * g_level.tileSize.x);
    camera->zoom = screenScale;


    InitPhysics();
    InitComponents();
    Attack_InitConfig();
    
    srand(time(NULL));

    /*Entity player = Spawn_Player(
        Vector2Zero(),
        0
    );*/
    
    GameState_InitStates();
    GameState_SwitchTo(GST_MAIN);
    
    // game loop
    while (!WindowShouldClose())        // run the loop untill the user presses ESCAPE or presses the Close button on the window
    {
        GameState_LoopCurrent();
    }
    // destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}

int test_main() 
{
    // Create the window and OpenGL context
    InitWindow(g_screenSettings.width, g_screenSettings.height, "Hello Raylib");

    InitPhysics();
    InitComponents();
    Attack_InitConfig();

    Entity e;
    Shape shape;

    //enemy
    e = ecs_create();
    float radius = 12;
    shape = Shapes_NewCircle(Vector2Zero(), 12);
    PositionComponent pos = { 0, 64 };
    VelocityComponent vel = { 16, 0 };
    DrawShapeComponent draw = { WHITE, shape };
    ColliderComponent col = { 
        shape, 
        (Layer)LN_ENEMY
    };
    
    ecs_add(e.id, CID_Position, &pos );
    //ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
    
    //static bullet
    e = ecs_create();
    shape = Shapes_NewLine(Vector2Zero(), (Vector2){ 64, 64 });
    pos = (Vector2) { 64, 64 + 8 };
    draw = (DrawShapeComponent) { GOLD, shape };
    col = (ColliderComponent) { 
        shape, 
        (Layer)LN_PL_BULLET
    };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
    //moving wall
    e = ecs_create();
    shape = Shapes_NewLine(Vector2Zero(), (Vector2){ -16, 128 });
    pos = (Vector2) { 32, 64 };
    vel = (Vector2) { 16, 0 };
    draw = (DrawShapeComponent) { GREEN, shape };
    col = (ColliderComponent) { 
        shape, 
        (Layer)LN_WALL
    };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
    
    while (!WindowShouldClose())        // run the loop untill the user presses ESCAPE or presses the Close button on the window
    {
        Systems_GameLoop();
        
        // drawing
        BeginDrawing();

        // Setup the back buffer for drawing (clear color and depth buffers)
        ClearBackground(BACKGROUND_COLOR);

        Systems_DrawLoop();
        Systems_DrawUILoop();

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }
    // destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}
