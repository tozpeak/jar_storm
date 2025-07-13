#pragma once

typedef void (*GameStateHandler)();

typedef struct
{
    GameStateHandler onEnter;
    GameStateHandler onLoop;
    GameStateHandler onExit;
} GameStateConfig;

enum GameStateOptions
{
    GST_MAIN = 0,
    GST_GAMEPLAY,
    
    GST_COUNT,
};

void GST_Empty();

void GameState_InitStates();
void GameState_SwitchTo(unsigned char _newState);
void GameState_LoopCurrent();
