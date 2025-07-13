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

void GameState_InitStates();
void GameState_SwitchToMainMenu();
void GameState_SwitchToGameplay();
void GameState_LoopCurrent();
