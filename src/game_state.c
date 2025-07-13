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

#include <game_states/gameplay.h>
#include <game_states/main_menu.h>
#include <game_state.h>

GameStateConfig gameStates[GST_COUNT];
unsigned char currentState = GST_COUNT;
unsigned char newState = GST_MAIN;

void GST_Empty() { }

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
    gameStates[GST_MAIN] = GST_MainMenu_Create();
    gameStates[GST_GAMEPLAY] = GST_Gameplay_Create();
}

void GameState_LoopCurrent()
{
    if (newState != currentState) {
        if(currentState < GST_COUNT) gameStates[currentState].onExit();
        currentState = newState;
        gameStates[currentState].onEnter();
    }
    gameStates[currentState].onLoop();
}
