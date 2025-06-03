//#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <ecs.h>

#include <components.h>
#include <physics.h>
#include <shapes.h>

typedef struct
{
    int width;
    int height;
} ScreenSettings;

const ScreenSettings g_screenSettings = { 640, 480 };

void DrawChessboard() 
{
    int screenX = g_screenSettings.width, screenY = g_screenSettings.height;
    Vector2 tileSize = { 16, 12 };
    int offsetTiles = 1;
    Color tileColor = DARKGRAY;
    for (int i = offsetTiles; (i + offsetTiles) * tileSize.x < screenX; i++) 
    {
        for (int j = 2 + offsetTiles + (i % 2); (j + offsetTiles) * tileSize.y < screenY; j += 2) 
        {
            DrawRectangle(
                i * tileSize.x,
                j * tileSize.y,
                tileSize.x,
                tileSize.y,
                tileColor
            );
        }
    }
}

void DrawCharacter (Vector2 position, Vector2 size) 
{
    Vector2 offset = { -size.x / 2, -size.y };
    Color charColor = GREEN;
    
    DrawRectangleV( Vector2Add(position, offset), size, charColor );
}

void DrawGun (Vector2 aimFrom, Vector2 aimDirection) 
{
    float aimLineLength = 12;
    
    DrawLineEx (
        aimFrom,
        Vector2Add( aimFrom, Vector2Scale(aimDirection, aimLineLength) ),
        5,
        DARKGREEN
    );
}

void System_Move(float deltaTime) 
{
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Position, CID_Velocity);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
		pos->x += vel->x * deltaTime;
		pos->y += vel->y * deltaTime;
		/*if (pos->x > RENDER_WIDTH || pos->x < 0)
			vel->x *= -1;
		if (pos->y > RENDER_HEIGHT || pos->y < 0)
			vel->y *= -1;*/
	}
}


void System_DealDamage()
{
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Health, CID_HasCollisions);
	for (i = 0; i < qr->count; ++i) {
		HealthComponent *hp = (HealthComponent*)ecs_get(qr->list[i], CID_Health);
		hp->hp -= 4;
		if(hp->hp <= 0) ecs_add(qr->list[i], CID_IsKilled, NULL);
	}
}

void System_DealDamageNew()
{
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Health, CID_HasCollisions);
	for (i = 0; i < qr->count; ++i) {
		HealthComponent *hp = (HealthComponent*)ecs_get(qr->list[i], CID_Health);
		HasCollisionsComponent *hc = (HasCollisionsComponent*)ecs_get(qr->list[i], CID_HasCollisions);
		
        CollisionIterator iterator = { 0 };
        InitCollisionIterator(&iterator, qr->list[i]);
        while (TryGetNextCollision(&iterator)) {
            uint32_t entB = iterator.other;
            CollisionData* cData = iterator.collisionData;
            
            DealDamageComponent *dam = (DealDamageComponent*)ecs_get(entB, CID_DealDamage);
            
            if (dam->target & DMG_OTHER)
            {
                hp->hp -= dam->damage;
		        if(hp->hp <= 0) ecs_add(qr->list[i], CID_IsKilled, NULL);
		    }
		    
		    if (dam->target & DMG_SELF)
		    {
		        HealthComponent *hpDam = (HealthComponent*)ecs_get(entB, CID_Health);
		        hpDam->hp -= dam->damage;
		        if(hpDam->hp <= 0) ecs_add(entB, CID_IsKilled, NULL);
		    }
        }
        
	}
}

void System_EnemyWanderer(float deltaTime)
{
    const int CHANCE_SAMPLE_SIZE = 100000;
    float changeDirProbabilityF = deltaTime * 1 / 5; // once in 5 seconds
    int changeDirProbability = round(CHANCE_SAMPLE_SIZE * changeDirProbabilityF);
    
    uint32_t i;
    QueryResult *qr = ecs_query(2, CID_Velocity, CID_IsWanderer);
	for (i = 0; i < qr->count; ++i) {
	    if (rand() % CHANCE_SAMPLE_SIZE > changeDirProbability) continue;
	    
		VelocityComponent *vel = (VelocityComponent*)ecs_get(qr->list[i], CID_Velocity);
		
		VelocityComponent newVel = Vector2Rotate(
		    *vel,
		    (rand() % 628) / 100.0f
		);
		
		*vel = newVel;
	}
}

void System_DestroyKilled()
{
    uint32_t i;
	QueryResult *qr = ecs_query(1, CID_IsKilled);
	for (i = 0; i < qr->count; ++i) {
		ecs_kill(qr->list[i]);
	}
}

void System_Draw() 
{
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Position, CID_DrawShape);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		DrawShapeComponent *shape = (DrawShapeComponent*)ecs_get(qr->list[i], CID_DrawShape);
		
		Shapes_Draw(pos, &shape->shape, shape->color);
	}
}

void System_DrawEnemyHP() 
{
    Vector2 offset = { 0, 3 };
    Vector2 right = { 1, 0 };
    uint32_t i;
	QueryResult *qr = ecs_query(2, CID_Position, CID_Health, CID_HasHpBar);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		HealthComponent *hp = (HealthComponent*)ecs_get(qr->list[i], CID_Health);
		
		float maxHp = hp->maxHp;
		
		float sizeFactor = 4 * log2(maxHp)/maxHp;
		Vector2 lp = Vector2Add(
		    Vector2Add(*pos, offset),
		    Vector2Scale(right, - maxHp * sizeFactor / 2)
	    );
	    Vector2 rpFull = Vector2Add( lp, Vector2Scale(right, maxHp * sizeFactor) );
	    Vector2 rpPart = Vector2Add( lp, Vector2Scale(right, hp->hp * sizeFactor) );
	    
	    DrawLineEx( lp, rpFull, 4, GRAY );
	    DrawLineEx( lp, rpPart, 2, RED );
	}
}

void System_DrawDebugCollisions()
{
    uint32_t i;
	QueryResult *qr = ecs_query(3, CID_Position, CID_DrawShape, CID_HasCollisions);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		DrawShapeComponent *shape = (DrawShapeComponent*)ecs_get(qr->list[i], CID_DrawShape);
		Color color = RED;
		
		Shapes_Draw(pos, &shape->shape, color);
	}
}

void System_ClearOutOfBounds() 
{
    uint32_t i;
	QueryResult *qr = ecs_query(1, CID_Position);
	for (i = 0; i < qr->count; ++i) {
		PositionComponent *pos = (PositionComponent*)ecs_get(qr->list[i], CID_Position);
		if (pos->x > g_screenSettings.width 
	     || pos->x < 0
	     || pos->y > g_screenSettings.height
	     || pos->y < 0
	    ) ecs_kill(qr->list[i]);
	}
}

void AddBullet(Vector2 aimFrom, Vector2 aimDirection, float speed) 
{
    Vector2 velocity = Vector2Scale(aimDirection, speed);
    
    Entity e = ecs_create();
    PositionComponent pos = aimFrom;
    VelocityComponent vel = velocity;
    Shape shape = Shapes_NewLine(
        Vector2Zero(),
        Vector2Scale( velocity, 0.02f )
    );
    DrawShapeComponent draw = { 
        YELLOW, shape
    };
    ColliderComponent col = { 
        shape, 
        (Layer)LN_PL_BULLET 
    };
    DealDamageComponent dam = { 4, DMG_SELF | DMG_OTHER };
    HealthComponent hp = { 1, 1 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_Health, &hp );
}

void AddBigBullet(Vector2 aimFrom, Vector2 aimDirection, float speed) 
{
    float radius = 10.0f;
    Vector2 velocity = Vector2Scale(aimDirection, speed);
    
    Entity e = ecs_create();
    PositionComponent pos = aimFrom;
    VelocityComponent vel = velocity;
    DrawShapeComponent shape = { SKYBLUE, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), radius), 
        (Layer)LN_PL_BULLET 
    };
    DealDamageComponent dam = { 4, DMG_SELF | DMG_OTHER };
    HealthComponent hp = { 36, 36 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_DealDamage, &dam );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_HasHpBar, NULL );
}

uint32_t AddEnemy(Vector2 position) 
{
    float radius = 6.0f;
    Entity e = ecs_create();
    PositionComponent pos = position;
	VelocityComponent vel = Vector2Rotate(
	    (Vector2) { 5, 0 },
	    (rand() % 628) / 100.0f
	);
    DrawShapeComponent shape = { WHITE, Shapes_NewCircle(Vector2Zero(), radius) };
    ColliderComponent col = { 
        Shapes_NewCircle(Vector2Zero(), radius), 
        (Layer)LN_ENEMY 
    };
    HealthComponent hp = { 24, 32 };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &shape );
    ecs_add(e.id, CID_Collider, &col );
    ecs_add(e.id, CID_Health, &hp );
    ecs_add(e.id, CID_IsWanderer, NULL );
    ecs_add(e.id, CID_HasHpBar, NULL );
    
    return e.id;
}

int test_main();

int main ()
{
    //return test_main();

	// Tell the window to use vsync and work on high DPI displays
	//SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	//SetTargetFPS(120);

	// Create the window and OpenGL context
	const int screenScale = 2;
	
	InitWindow(g_screenSettings.width * screenScale, g_screenSettings.height * screenScale, "Hello Raylib");
	//SetWindowSize(g_screenSettings.width * screenScale, g_screenSettings.height * screenScale);
	
    Camera2D camera = { 0 };
    camera.target = (Vector2){ 0 };
    camera.offset = (Vector2){ 0 }; //-g_screenSettings.width/2.0f, -g_screenSettings.height/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = screenScale;

	
	InitPhysics();
	InitComponents();
    
    Vector2 playerPos = { 32, 32 };
    Vector2 playerSize = { 12, 24 };
    Vector2 aimFromOffset = { 0, -playerSize.y / 2 };
    Vector2 aimDirection = { 0, 0 };
    float playerSpeed = 16 * 4;
    float shotCooldownState = 0;
    float shotCooldown = 0.25f;
    float bulletSpeed = 16 * 128;
    float bigBulletSpeed = 16 * 4;
    
    for (int i = 1; i < 10; i++) {
        for (int j = 1; j < 8; j++) {
            AddEnemy((Vector2) { i * 16 * 4, j * 16 * 4 } );
        }
    }

	// game loop
	while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
        float delta = GetFrameTime();

        float dSpeed = playerSpeed * delta;
        
        if (IsKeyDown(KEY_D)) playerPos.x += dSpeed;
        if (IsKeyDown(KEY_A)) playerPos.x -= dSpeed;
        if (IsKeyDown(KEY_W)) playerPos.y -= dSpeed;
        if (IsKeyDown(KEY_S)) playerPos.y += dSpeed;
        
        Vector2 aimTo = GetScreenToWorld2D(GetMousePosition(), camera);
        Vector2 aimFrom = Vector2Add (playerPos, aimFromOffset);
        
        aimDirection = Vector2Subtract(aimTo, aimFrom);
        aimDirection = Vector2Normalize(aimDirection);
        
        shotCooldownState += delta;
        
        if (IsMouseButtonDown(0) && (shotCooldownState > shotCooldown)) {
            AddBullet(aimFrom, aimDirection, bulletSpeed);
            shotCooldownState = 0;
        }
        
        if (IsMouseButtonPressed(1) && (shotCooldownState > shotCooldown)) {
            AddBigBullet(aimFrom, aimDirection, bigBulletSpeed);
            shotCooldownState = 0;
        }
        
        System_Move(delta);
        System_ClearCollisions();
        System_Collide(delta);
        
        System_DealDamageNew();
        System_EnemyWanderer(delta);
        
        System_DestroyKilled();
        System_ClearOutOfBounds();
        
		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);
		
		BeginMode2D(camera);
		
		DrawChessboard();

		DrawCharacter(playerPos, playerSize);
		DrawGun(aimFrom, aimDirection);
		
        System_Draw();
        System_DrawDebugCollisions();
        
        System_DrawEnemyHP();
        
        EndMode2D();
        
        DrawFPS(1, 1);
        DrawText(
            TextFormat("%d entt", ecs_query(0)->count),
            1, 24,
            16, DARKGREEN
        );
        
        DrawText(
            TextFormat("%d enemies", ecs_query(1, CID_Health)->count),
            1, 24 * 2,
            16, DARKGREEN
        );
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}
	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}

int test_main() 
{
	// Create the window and OpenGL context
	InitWindow(g_screenSettings.width, g_screenSettings.height, "Hello Raylib");
	
	InitPhysics();
	InitComponents();
	
	Entity e;
	Shape shape;
	
	//enemy
	e = ecs_create();
	float radius = 12;
	shape = Shapes_NewCircle(Vector2Zero(), 12);
    PositionComponent pos = { 0, 64 };
	VelocityComponent vel = { 16, 0 };
    DrawShapeComponent draw = { WHITE, shape };
    ColliderComponent col = { 
        shape, 
        (Layer)LN_ENEMY
    };
    
    ecs_add(e.id, CID_Position, &pos );
    //ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
    
	//static bullet
	e = ecs_create();
    shape = Shapes_NewLine(Vector2Zero(), (Vector2){ 64, 64 });
    pos = (Vector2) { 64, 64 + 8 };
    draw = (DrawShapeComponent) { GOLD, shape };
    col = (ColliderComponent) { 
        shape, 
        (Layer)LN_PL_BULLET
    };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
	//moving wall
	e = ecs_create();
    shape = Shapes_NewLine(Vector2Zero(), (Vector2){ -16, 128 });
    pos = (Vector2) { 32, 64 };
    vel = (Vector2) { 16, 0 };
    draw = (DrawShapeComponent) { GREEN, shape };
    col = (ColliderComponent) { 
        shape, 
        (Layer)LN_WALL
    };
    
    ecs_add(e.id, CID_Position, &pos );
    ecs_add(e.id, CID_Velocity, &vel );
    ecs_add(e.id, CID_DrawShape, &draw );
    ecs_add(e.id, CID_Collider, &col );
    
	
    while (!WindowShouldClose())		// run the loop untill the user presses ESCAPE or presses the Close button on the window
	{
        float delta = GetFrameTime();
        
        System_Move(delta);
        System_ClearCollisions();
        System_Collide(delta);
        
        System_DealDamageNew();
        //System_EnemyWanderer(delta);
        
        System_DestroyKilled();
        System_ClearOutOfBounds();
        
		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);
		
		DrawChessboard();

		//DrawCharacter(playerPos, playerSize);
		//DrawGun(aimFrom, aimDirection);
		
        System_Draw();
        System_DrawDebugCollisions();
        
        System_DrawEnemyHP();
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}
	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
