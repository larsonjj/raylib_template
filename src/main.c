#include "box2d/box2d.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

typedef struct Conversion
{
    float scale;
    float tileSize;
    float screenWidth;
    float screenHeight;
} Conversion;

typedef struct Entity
{
    b2BodyId bodyId;
    Texture texture;
} Entity;

Vector2 ConvertWorldToScreen(b2Vec2 p, Conversion cv)
{
    Vector2 result = {cv.scale * p.x + 0.5f * cv.screenWidth,
                      0.5f * cv.screenHeight - cv.scale * p.y};
    return result;
}

void DrawEntity(const Entity *entity, Conversion cv)
{
    b2Vec2 p =
        b2Body_GetWorldPoint(entity->bodyId, (b2Vec2){-0.5f * cv.tileSize, 0.5f * cv.tileSize});
    float radians = b2Body_GetAngle(entity->bodyId);

    Vector2 ps = ConvertWorldToScreen(p, cv);

    float textureScale = cv.tileSize * cv.scale / (float)entity->texture.width;

    // Have to negate rotation to account for y-flip
    DrawTextureEx(entity->texture, ps, -RAD2DEG * radians, textureScale, WHITE);

    // I used these circles to ensure the coordinate transformation was correct
    DrawCircleV(ps, 5.0f, BLACK);
    p = b2Body_GetWorldPoint(entity->bodyId, (b2Vec2){0.0f, 0.0f});
    ps = ConvertWorldToScreen(p, cv);
    DrawCircleV(ps, 5.0f, BLUE);
    p = b2Body_GetWorldPoint(entity->bodyId, (b2Vec2){0.5f * cv.tileSize, -0.5f * cv.tileSize});
    ps = ConvertWorldToScreen(p, cv);
    DrawCircleV(ps, 5.0f, RED);
}

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "raylib box2d - custom frame control");

    // Custom timming variables
    double previousTime = GetTime(); // Previous time measure
    double currentTime = 0.0;        // Current time measure
    double updateDrawTime = 0.0;     // Update + Draw time
    double waitTime = 0.0;           // Wait time (if target fps required)
    float deltaTime = 0.0f;          // Frame time (Update + Draw + Wait time)

    float timeCounter = 0.0f; // Accumulative time counter (seconds)
    float position = 0.0f;    // Circle position
    bool pause = false;       // Pause control flag

    int targetFPS = 60; // Our initial target fps
    float targetDeltaTime = 1.0f / (float)targetFPS;

    float tileSize = 1.0f;
    float scale = 50.0f;

    Conversion cv = {scale, tileSize, (float)screenWidth, (float)screenHeight};

    b2WorldDef worldDef = b2DefaultWorldDef();
    b2WorldId worldId = b2CreateWorld(&worldDef);

    Texture2D textures[2] = {0};
    textures[0] = LoadTexture("assets/ground.png");
    textures[1] = LoadTexture("assets/box.png");

    b2Polygon tilePolygon = b2MakeSquare(0.5f * tileSize);

    Entity groundEntities[20] = {0};
    for (int i = 0; i < 20; ++i)
    {
        Entity *entity = groundEntities + i;
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.position = (b2Vec2){(1.0f * i - 10.0f) * tileSize, -4.5f - 0.5f * tileSize};

        // I used this rotation to test the world to screen transformation
        //bodyDef.angle = 0.25f * b2_pi * i;

        entity->bodyId = b2CreateBody(worldId, &bodyDef);
        entity->texture = textures[0];
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        b2CreatePolygonShape(entity->bodyId, &shapeDef, &tilePolygon);
    }

    Entity boxEntities[4] = {4};
    for (int i = 0; i < 4; ++i)
    {
        Entity *entity = boxEntities + i;
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = (b2Vec2){0.5f * tileSize * i, -4.0f + tileSize * i};
        entity->bodyId = b2CreateBody(worldId, &bodyDef);
        entity->texture = textures[1];
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.restitution = 0.1f;
        b2CreatePolygonShape(entity->bodyId, &shapeDef, &tilePolygon);
    }
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // if (updateDrawTime > targetDeltaTime)
        // {

        // Update
        //----------------------------------------------------------------------------------
        PollInputEvents(); // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)

        if (IsKeyPressed(KEY_SPACE))
            pause = !pause;

        if (IsKeyPressed(KEY_UP))
            targetFPS += 1000;
        else if (IsKeyPressed(KEY_DOWN))
            targetFPS -= 1000;

        if (targetFPS < 0)
            targetFPS = 0;

        if (!pause)
        {
            position += 200 * deltaTime; // We move at 200 pixels per second
            if (position >= (float)GetScreenWidth())
                position = 0;
            timeCounter += deltaTime; // We count time (seconds)
            b2World_Step(worldId, deltaTime, 4);
            timeCounter += deltaTime; // We count time (seconds)
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        for (int i = 0; i < 20; ++i)
        {
            DrawEntity(groundEntities + i, cv);
        }

        for (int i = 0; i < 4; ++i)
        {
            DrawEntity(boxEntities + i, cv);
        }

        const char *message = "Hello Box2D!";
        int fontSize = 36;
        int textWidth = MeasureText("Hello Box2D!", fontSize);
        DrawText(TextFormat("timeCounter: %03.0f ms", timeCounter * 1000.0f),
                 40,
                 GetScreenHeight() / 2 - 100,
                 20,
                 MAROON);
        DrawText(TextFormat("deltaTime: %f ms", deltaTime),
                 40,
                 GetScreenHeight() / 2 - 50,
                 20,
                 MAROON);
        DrawText(TextFormat("updateDrawTime: %f ms", updateDrawTime),
                 40,
                 GetScreenHeight() / 2 - 0,
                 20,
                 MAROON);
        DrawText(message, (screenWidth - textWidth) / 2, 50, fontSize, LIGHTGRAY);


        for (int i = 0; i < GetScreenWidth() / 200; i++)
            DrawRectangle(200 * i, 0, 1, GetScreenHeight(), SKYBLUE);

        DrawCircle((int)position, GetScreenHeight() / 2 - 25, 50, RED);

        DrawText("Circle is moving at a constant 200 pixels/sec,\nindependently of the frame rate.",
                 10,
                 10,
                 20,
                 DARKGRAY);
        DrawText("PRESS SPACE to PAUSE MOVEMENT", 10, GetScreenHeight() - 60, 20, GRAY);
        DrawText("PRESS UP | DOWN to CHANGE TARGET FPS", 10, GetScreenHeight() - 30, 20, GRAY);
        DrawText(TextFormat("TARGET FPS: %i", targetFPS), GetScreenWidth() - 220, 10, 20, LIME);
        DrawText(TextFormat("CURRENT FPS: %i", (int)(1.0f / deltaTime)),
                 GetScreenWidth() - 220,
                 40,
                 20,
                 GREEN);

        EndDrawing();

        // NOTE: In case raylib is configured to SUPPORT_CUSTOM_FRAME_CONTROL,
        // Events polling, screen buffer swap and frame time control must be managed by the user

        SwapScreenBuffer(); // Flip the back buffer to screen (front buffer)
        // }

        currentTime = GetTime();
        updateDrawTime = currentTime - previousTime;

        // if (targetFPS > 0) // We want a fixed frame rate
        // {
        //     waitTime = targetDeltaTime - updateDrawTime;
        //     if (waitTime > 0.0)
        //     {
        //         WaitTime((float)waitTime * 0.95f);
        //         currentTime = GetTime();
        //         deltaTime = (float)(currentTime - previousTime);
        //     }
        // }
        // else
        // {
        deltaTime = (float)updateDrawTime; // Framerate could be variable
        // }

        previousTime = currentTime;
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(textures[0]);
    UnloadTexture(textures[1]);

    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
