#include <raylib.h>
#include <ecs.h>

#include <components.h>
#include <attacks.h>

#define INTERACTION_COLOR GOLD

typedef struct
{
    Entity player;
    Entity interactable;
} InteractionContext;

bool TryGetInteractableForPlayer(Entity player, Entity *interactible)
{
    if ( !ecs_has(player.id, CID_HasCollisions) ) return false;
    HasCollisionsComponent *hc = (HasCollisionsComponent*) ecs_get(player.id, CID_HasCollisions);
    
    CollisionIterator iterator = { 0 };
    InitCollisionIterator(&iterator, player.id);
    while (TryGetNextCollision(&iterator)) {
        uint32_t entB = iterator.other;
        CollisionData* cData = iterator.collisionData;
        
        if ( !ecs_has(entB, CID_IsInteractable) ) continue;
        
        interactible->id = entB;
        return true;
    }
    
    return false;
}

bool HasCoinsForPrice(InteractionContext *context)
{
    //no coin price == success
    if( !ecs_has(context->interactable.id, CID_PriceInCoins )) return true;
    if( !ecs_has(context->interactable.id, CID_Coins )) return true;
    
    //no coins == fail
    if( !ecs_has(context->player.id, CID_Coins )) return false;
    
    CoinsComponent *actorCoins =    (CoinsComponent*) ecs_get(context->player.id,       CID_Coins);
    CoinsComponent *targetCoins =   (CoinsComponent*) ecs_get(context->interactable.id, CID_Coins);
    
    return actorCoins->amount >= targetCoins->amount;
}

void PayCoinPrice(InteractionContext *context)
{
    if( !ecs_has(context->interactable.id, CID_PriceInCoins )) return;
    
    CoinsComponent *actorCoins =    (CoinsComponent*) ecs_get(context->player.id,       CID_Coins);
    CoinsComponent *targetCoins =   (CoinsComponent*) ecs_get(context->interactable.id, CID_Coins);
    
    actorCoins->amount -= targetCoins->amount;
}


bool CheckPlayerPossibleInteraction(InteractionContext *context)
{
    if (!HasCoinsForPrice(context)) return false;
    
    return true;
}

void PerformPreInteraction(InteractionContext *context)
{
    PayCoinPrice(context);
}


void System_PlayerInput_Interact()
{
    InteractionContext context = {
        .player = { 0 },
        .interactable = { 0 },
    };
    
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_PlayerId, CID_PlayerInput, CID_HasCollisions);
    
    for (i = 0; i < qr->count; ++i) {
        context.player.id = qr->list[i];
        
        if ( !IsKeyPressed(KEY_E) ) continue;
        
        if ( !TryGetInteractableForPlayer(context.player, &(context.interactable)) ) continue;
        
        if ( !CheckPlayerPossibleInteraction(&context) ) continue;
        PerformPreInteraction(&context);
        
        Entity event = ecs_create();
        
        ecs_add(event.id, CID_ParentId, &(context.player.id));
        ecs_add(event.id, CID_TargetId, &(context.interactable.id));
        ecs_add(event.id, CID_EventInteraction, NULL);
        
        ecs_add(
            event.id, 
            CID_PrimaryAttack,
            ecs_get(context.interactable.id, CID_PrimaryAttack)
        );
        //ecs_remove(interactible.id, CID_Interactible);
    }
}

void System_PerformInteraction()
{
    Entity event = { 0 };
    AttackIntentionComponent intention = {
        .aimAt = Vector2Zero(),
        .isPrimary = true,
    };
    AttackContext context = {
        .intention = &intention,
        .aiEvaluation = 1,
    };
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_EventInteraction, CID_PrimaryAttack);
    
    for (i = 0; i < qr->count; ++i) {
        event.id = qr->list[i];
        
        ParentIdComponent *actor = (ParentIdComponent*) ecs_get(event.id, CID_ParentId);
        TargetIdComponent *target = (TargetIdComponent*) ecs_get(event.id, CID_TargetId);
        
        context.entityId = event.id;
        context.ability = ecs_get(event.id, CID_PrimaryAttack);
        
        Attack_Perform(&context);
        
        //consider: maybe kill events later at once after all hooks
        //Hovewer, kill doesn't purge the object, so they could be proceed anyway
        ecs_add(event.id, CID_IsKilled, NULL);
    }
}

void System_DrawInteractibleForPlayer()
{
    Entity player = { 0 };
    Entity interactible = { 0 };
    uint32_t i;
    QueryResult *qr = ecs_query(3, CID_PlayerId, CID_PlayerInput, CID_HasCollisions);
    
    for (i = 0; i < qr->count; ++i) {
        player.id = qr->list[i];
        
        if ( !TryGetInteractableForPlayer(player, &interactible) ) continue;
        
        PositionComponent *interPos = (PositionComponent*) ecs_get(interactible.id, CID_Position);
        DrawShapeComponent *interDraw = (DrawShapeComponent*) ecs_get(interactible.id, CID_DrawShape);
        
        Shape shape = interDraw->shape;
        float radius = 
            (shape.type == SHP_CIRCLE)
            ? shape.circle.radius
            : 6;
        
        DrawCircleLinesV(*interPos, radius, INTERACTION_COLOR);
    }
}

void Systems_Interactions()
{
    System_PlayerInput_Interact();
    System_PerformInteraction();
}

void Systems_DrawInteractions()
{
    System_DrawInteractibleForPlayer();
}
