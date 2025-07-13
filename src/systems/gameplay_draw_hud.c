#include <stdint.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

#include <components.h>
#include <level.h>
#include <ecs_helpers.h>
#include <game_state.h>

#include <systems/gameplay_draw_hud.h>

void System_DrawHUD_HeaderBackground()
{
    DrawRectangle(
        0,
        0,
        g_screenSettings.width,
        2 * g_level.tileSize.y,
        SHADE_UI_COLOR
    );
}

void System_DrawHUD_Coins()
{
    QueryResult *qr = ecs_query(1, CID_PlayerId);
    uint32_t entPlayer = qr->list[0];
    
    CoinsComponent *coins = (CoinsComponent*) ecs_get(entPlayer, CID_Coins);
    
    
    DrawText(
        TextFormat("C:%d", coins->amount),
        64,
        8,
        16, DARKGREEN
    );
}

void System_DrawHUD_Items()
{
    const char* labels[] = {
        "MVS",
        "ATS",
        "DMG",
        "MSH"
    };

    QueryResult *qr = ecs_query(1, CID_PlayerId);
    uint32_t entPlayer = qr->list[0];
    
    uint32_t i;
    uint32_t j = 0;
    qr = ecs_query(2, CID_Item, CID_ParentId);
    for (i = 0; i < qr->count; ++i) {
        uint32_t entItem = qr->list[i];
        
        ParentIdComponent *parent = (ParentIdComponent*) ecs_get(entItem, CID_ParentId);
        
        if (*parent != entPlayer) continue;
        
        ItemComponent *item = (ItemComponent*) ecs_get(entItem, CID_Item);
        
        DrawText(
            TextFormat("%s:%d", labels[item->type], item->count),
            128 + 64 * ( j++ ),
            8,
            16, DARKGREEN
        );
    }
}

void System_DrawHUD_DeathScreen()
{
    QueryResult *qr = ecs_query(2, CID_PlayerId, CID_PlayerInput);
    //there are players alive
    if (qr->count > 0) return;
    
    DrawRectangle(
        0,
        0,
        g_screenSettings.width,
        g_screenSettings.height,
        SHADE_UI_COLOR
    );
    
    const int textSize = 16;
    int posVertical = ( g_screenSettings.height - textSize ) / 2;
    
    const char *text = "YOU DIED";
    DrawText(
        text,
        ( g_screenSettings.width - MeasureText(text, textSize) ) / 2,
        posVertical,
        textSize,
        RED
    );
    posVertical += textSize;
    
    const char *text2 = "[Press ENTER to RESET GAME]";
    DrawText(
        text2,
        ( g_screenSettings.width - MeasureText(text2, textSize) ) / 2,
        posVertical,
        textSize,
        GRAY
    );
    posVertical += textSize;
    
    if( IsKeyPressed(KEY_ENTER) ) {
        /*qr = ecs_query(1, CID_PlayerId);
        uint32_t i;
        for (i = 0; i < qr->count; ++i) {
            uint32_t playerEnt = qr->list[i];
            
            ECS_GET_NEW(hp, playerEnt, Health);
            hp->hp = hp->maxHp;
            
            ecs_add(playerEnt, CID_PlayerInput, NULL);
        }*/
        GameState_SwitchToMainMenu();
    }
}

void Systems_DrawUILoop()
{
    System_DrawHUD_HeaderBackground();
    System_DrawHUD_Coins();
    System_DrawHUD_Items();
    System_DrawHUD_DeathScreen();
}
