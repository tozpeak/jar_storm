#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>

#include <game_states/main_menu.h>

void GST_Main_OnLoop()
{
    //autostart
    GameState_SwitchTo(GST_GAMEPLAY);
}

GameStateConfig GST_MainMenu_Create()
{
    return (GameStateConfig) {
        .onEnter = GST_Empty,
        .onLoop  = GST_Main_OnLoop,
        .onExit  = GST_Empty,
    };
}
