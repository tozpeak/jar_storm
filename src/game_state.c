#include <stdio.h>
#include <stdint.h>
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

GameStateConfig gameStates[GST_COUNT];
unsigned char currentState = GST_COUNT;
unsigned char newState = GST_MAIN;

void Empty() { }

void GST_Gameplay_OnEnter()
{
    Level_Setup();
    
    //create player or set it in new spawn point
    QueryResult *qr = ecs_query(1, CID_PlayerId);
    Vector2 spawnPoint = g_level.spawnPoint;
    if (qr->count < 1) {
        Spawn_Player(
            spawnPoint,
            0
        );
    } else {
        uint32_t entPlayer = qr->list[0];
        ecs_add(entPlayer, CID_Position, &spawnPoint);
    }
}

void GST_Gameplay_OnLoop()
{
    Vector2 centerScreenOffset = (Vector2){ g_screenSettings.width / 2.f, g_screenSettings.height / 2.f };
    Camera2D *camera = g_screenSettings.camera;
    QueryResult *qr = ecs_query(1, CID_PlayerId);
    Entity player = {
        .id = qr->list[0],
    };
    
    Systems_GameLoop();
    
    // drawing
    BeginDrawing();
    
    // Setup the back buffer for drawing (clear color and depth buffers)
    ClearBackground(BACKGROUND_COLOR);
    
    ECS_GET_NEW(playerPos, player.id, Position);
    camera->target = *playerPos;
    BeginMode2D(*camera);
    Systems_DrawLoop();
    EndMode2D();
    
    camera->target = centerScreenOffset;
    BeginMode2D(*camera);
    Systems_DrawUILoop();
    EndMode2D();
    camera->target = *playerPos;
    
    DrawFPS(1, 1);
    DrawText(
        TextFormat("%d entt", ecs_query(0)->count),
        1, 24,
        16, DARKGREEN
    );
    
    DrawText(
        TextFormat("%d enemies", ecs_query(1, CID_Health)->count),
        1, 24 * 2,
        16, DARKGREEN
    );

    // end the frame and get ready for the next one  (display frame, poll input, etc...)
    EndDrawing();
}

void GST_Gameplay_OnExit()
{
    static DynamicVector3 zero = (Vector3) { 0 };
    
    uint32_t i;
    QueryResult *qr = ecs_query(0);
    
    for (i = 0; i < qr->count; i++) {
        uint32_t ent = qr->list[i];
        ecs_add(ent, CID_Position, &zero);
        ecs_add(ent, CID_Velocity, &zero);
        ecs_kill(ent);
    }
}

void GST_Main_OnLoop()
{
    //autostart
    GameState_SwitchToGameplay();
}

void GameState_SwitchTo(unsigned char _newState)
{
    if(_newState >= GST_COUNT) {
        printf("Trying to set unexpected state: %d\n", _newState);
        return;
    }
    newState = _newState;
}

void GameState_InitStates()
{
    gameStates[GST_MAIN] = (GameStateConfig) {
        .onEnter = Empty,
        .onLoop  = GST_Main_OnLoop,
        .onExit  = Empty,
    };
    gameStates[GST_GAMEPLAY] = (GameStateConfig) {
        .onEnter = GST_Gameplay_OnEnter,
        .onLoop  = GST_Gameplay_OnLoop,
        .onExit  = GST_Gameplay_OnExit,
    };
}

void GameState_SwitchToMainMenu() { GameState_SwitchTo(GST_MAIN); }
void GameState_SwitchToGameplay() { GameState_SwitchTo(GST_GAMEPLAY); }

void GameState_LoopCurrent()
{
    if (newState != currentState) {
        if(currentState < GST_COUNT) gameStates[currentState].onExit();
        currentState = newState;
        gameStates[currentState].onEnter();
    }
    gameStates[currentState].onLoop();
}
