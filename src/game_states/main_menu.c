#include <stdio.h>
#include <stdint.h>
#include <raylib.h>
#include <raymath.h>

#include <level.h>
#include <game_states/main_menu.h>

typedef struct
{
    char *text;
    void (*action)();
} MenuOption;

typedef struct
{
    char *header;
    char *subtext;
    int menuCount;
    MenuOption* options;
} MenuConfig;

MenuConfig *currentMenu = NULL;
int currentOption = 0;

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
    MenuAction_SetMenu(&howTo);
}

extern bool g_closeTheGame;
void MenuAction_Main_Exit()
{
    g_closeTheGame = true;
}


void MenuAction_HowTo_Back()
{
    MenuAction_SetMenu(&mainMenu);
}


void MenuAction_SetMenu(MenuConfig *newMenu)
{
    currentMenu = newMenu;
    currentOption = 0;
}

int String_LineCount(char *str)
{
    int lineCount = 1;
    
    while (*str != '\0') {
        if(*str == '\n') lineCount++;
        str++;
    }
    
    return lineCount;
}



void Menu_SelectOption()
{
    bool isSelectPressed = 
        IsKeyPressed(KEY_E)
        || IsKeyPressed(KEY_ENTER);
    
    if (isSelectPressed)
    {
        currentMenu->options[currentOption].action();
        return; //do not process input after option is selected
    }
    
    if ( IsKeyPressed(KEY_W) ) currentOption--;
    if ( IsKeyPressed(KEY_S) ) currentOption++;
    
    if (currentOption < 0)
        currentOption = 0;
    if (currentOption >= currentMenu->menuCount)
        currentOption = currentMenu->menuCount - 1;
}

void MainMenu_DrawMenu()
{
    int x, y;
    x = 40;
    y = 40;
    int textHeight;
    
    textHeight = 32;
    DrawText(
        currentMenu->header,
        x, y, textHeight, WHITE
    );
    y += textHeight;
    
    textHeight = 16;
    char *subtext = currentMenu->subtext;
    if (subtext != NULL) {
        int lineCount = String_LineCount(subtext);
        
        DrawText(
            subtext,
            x, y, textHeight, GRAY
        );
        y += textHeight * lineCount;
    }
    
    textHeight = 24;
    for (int i=0; i<currentMenu->menuCount; i++) {
        MenuOption *option = currentMenu->options + i;
        bool isSelected = i == currentOption;
        
        DrawText(
            option->text,
            x, y, textHeight,
            (isSelected) ? ORANGE : RED
        );
        y += textHeight;
    }
}

Camera2D *camera;
void GST_Main_OnEnter()
{
    MenuAction_SetMenu(&mainMenu);
    
    Vector2 centerScreenOffset = (Vector2){ g_screenSettings.width / 2.f, g_screenSettings.height / 2.f };
    camera = g_screenSettings.camera;
    camera->target = centerScreenOffset;
}

void GST_Main_OnLoop()
{
    BeginDrawing();
    
    ClearBackground(BLACK);
    
    BeginMode2D(*camera);
    MainMenu_DrawMenu();
    EndMode2D();
    
    EndDrawing();
    
    Menu_SelectOption();
}

GameStateConfig GST_MainMenu_Create()
{
    return (GameStateConfig) {
        .onEnter = GST_Main_OnEnter,
        .onLoop  = GST_Main_OnLoop,
        .onExit  = GST_Empty,
    };
}
