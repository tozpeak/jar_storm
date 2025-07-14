#pragma once

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

typedef struct
{
    MenuConfig *menu;
    int currentOption;
} MenuState;


void Menu_ProcessInput(MenuState *state);
void Menu_DrawMenu(MenuState *state);
void Menu_SetMenu(MenuState *state, MenuConfig *newMenu);
