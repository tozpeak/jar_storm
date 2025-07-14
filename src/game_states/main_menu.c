#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>

#include <level.h>
#include <menu.h>
#include <game_states/main_menu.h>

MenuState mainMenuState = {
    .menu = NULL,
    .currentOption = 0,
};

void MenuAction_Main_StartGame();
void MenuAction_Main_HowTo();
void MenuAction_Main_Exit();

void MenuAction_HowTo_Back();


void MenuAction_SetMenu(MenuConfig *newMenu);

MenuOption mainMenu_options[3] = {
    (MenuOption) { "Start Game", MenuAction_Main_StartGame },
    (MenuOption) { "How To Play", MenuAction_Main_HowTo },
    (MenuOption) { "Exit", MenuAction_Main_Exit },
};

MenuConfig mainMenu = {
    .header = "Main Menu",
    .subtext =  "[WASD] to navigate menu\n"
                "[E] or [Enter] to select option\n",
    .menuCount = 3,
    .options = mainMenu_options,
};


MenuOption howTo_options[1] = {
    (MenuOption) { "Back", MenuAction_HowTo_Back },
};

MenuConfig howTo = {
    .header = "How to Play",
    .subtext =  "[WASD] to move\n"
                "[Mouse] to aim\n"
                "[E] to interact\n"
                "[Left Mouse] to attack\n"
                "[Right Mouse] to use alternative attack\n"
                "[Space] to jump\n"
                "[Esc] for pause\n",
    .menuCount = 1,
    .options = howTo_options,
};

void MenuAction_Main_StartGame()
{
    GameState_SwitchTo(GST_GAMEPLAY);
}

void MenuAction_Main_HowTo()
{
    Menu_SetMenu(&mainMenuState, &howTo);
}

extern bool g_closeTheGame;
void MenuAction_Main_Exit()
{
    g_closeTheGame = true;
}

void MenuAction_HowTo_Back()
{
    Menu_SetMenu(&mainMenuState, &mainMenu);
}

Camera2D *camera;
void GST_Main_OnEnter()
{
    Menu_SetMenu(&mainMenuState, &mainMenu);
    
    Vector2 centerScreenOffset = (Vector2){ g_screenSettings.width / 2.f, g_screenSettings.height / 2.f };
    camera = g_screenSettings.camera;
    camera->target = centerScreenOffset;
}

void GST_Main_OnLoop()
{
    BeginDrawing();
    
    ClearBackground(BLACK);
    
    BeginMode2D(*camera);
    Menu_DrawMenu(&mainMenuState);
    EndMode2D();
    
    EndDrawing();
    
    Menu_ProcessInput(&mainMenuState);
}

GameStateConfig GST_MainMenu_Create()
{
    return (GameStateConfig) {
        .onEnter = GST_Main_OnEnter,
        .onLoop  = GST_Main_OnLoop,
        .onExit  = GST_Empty,
    };
}
