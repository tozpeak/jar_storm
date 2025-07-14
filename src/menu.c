#include <stdlib.h>
#include <raylib.h>

#include <menu.h>

void Menu_ProcessInput(MenuState *state)
{
    MenuConfig *currentMenu = state->menu;
    bool isSelectPressed = 
        IsKeyPressed(KEY_E)
        || IsKeyPressed(KEY_ENTER);
    
    if (isSelectPressed)
    {
        currentMenu->options[state->currentOption].action();
        return; //do not process input after option is selected
    }
    
    if ( IsKeyPressed(KEY_W) ) state->currentOption--;
    if ( IsKeyPressed(KEY_S) ) state->currentOption++;
    
    if (state->currentOption < 0)
        state->currentOption = 0;
    if (state->currentOption >= currentMenu->menuCount)
        state->currentOption = currentMenu->menuCount - 1;
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

void Menu_DrawMenu(MenuState *state)
{
    MenuConfig *currentMenu = state->menu;
    
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
        y += (textHeight + 2) * lineCount;
    }
    
    textHeight = 24;
    for (int i=0; i<currentMenu->menuCount; i++) {
        MenuOption *option = currentMenu->options + i;
        bool isSelected = i == state->currentOption;
        
        DrawText(
            option->text,
            x, y, textHeight,
            (isSelected) ? ORANGE : RED
        );
        
        if (isSelected) {
            DrawPoly(
                (Vector2) {
                    x - textHeight * 2 / 3,
                    y + textHeight/2
                },
                3,
                textHeight / 3,
                0.f,
                ORANGE
            );
        }
        
        y += textHeight;
    }
}

void Menu_SetMenu(MenuState *state, MenuConfig *newMenu)
{
    state->menu = newMenu;
    state->currentOption = 0;
}
