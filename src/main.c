#include "box2d/box2d.h"
#include "raylib.h"
#include "raymath.h"
// #include <SDL3/SDL.h>
// #include <stdint.h>

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
    int width = 1280, height = 720;
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(width, height, "box2d-raylib");
    SetTargetFPS(60);

    // Custom timming variables
    double previousTime = GetTime(); // Previous time measure
    double currentTime = 0.0;        // Current time measure
    double updateDrawTime = 0.0;     // Update + Draw time
    double waitTime = 0.0;           // Wait time (if target fps required)
    float deltaTime = 0.0f;          // Frame time (Update + Draw + Wait time)

    float timeCounter = 0.0f; // Accumulative time counter (seconds)
    bool pause = false;       // Pause control flag

    int targetFPS = 60; // Our initial target fps

    float tileSize = 1.0f;
    float scale = 50.0f;

    Conversion cv = {scale, tileSize, (float)width, (float)height};

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

    while (!WindowShouldClose())
    {
        // Update
        //---------------------------------------------------------------------
        // uint64_t startPerf = SDL_GetPerformanceCounter();
        // PollInputEvents(); // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)
        if (IsKeyPressed(KEY_P))
        {
            pause = !pause;
        }

        if (IsKeyPressed(KEY_UP))
        {
            targetFPS += 20;
        }
        else if (IsKeyPressed(KEY_DOWN))
        {
            targetFPS -= 20;
        }

        if (targetFPS < 0)
        {
            targetFPS = 0;
        }

        if (pause == false)
        {
            b2World_Step(worldId, deltaTime, 4);
            timeCounter += deltaTime; // We count time (seconds)
        }

        // Draw
        //---------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawFPS(10, 10);

        const char *message = "Hello Box2D!";
        int fontSize = 36;
        int textWidth = MeasureText("Hello Box2D!", fontSize);
        DrawText(TextFormat("%03.0f ms", timeCounter * 1000.0f),
                 40,
                 GetScreenHeight() / 2 - 100,
                 20,
                 MAROON);
        DrawText(TextFormat("%f ms", deltaTime), 40, GetScreenHeight() / 2 - 50, 20, MAROON);
        DrawText(TextFormat("%f ms", updateDrawTime), 40, GetScreenHeight() / 2 - 0, 20, MAROON);
        DrawText(message, (width - textWidth) / 2, 50, fontSize, LIGHTGRAY);

        DrawText("PRESS SPACE to PAUSE MOVEMENT", 10, GetScreenHeight() - 60, 20, GRAY);
        DrawText("PRESS UP | DOWN to CHANGE TARGET FPS", 10, GetScreenHeight() - 30, 20, GRAY);
        DrawText(TextFormat("TARGET FPS: %i", targetFPS), GetScreenWidth() - 240, 10, 20, LIME);
        DrawText(TextFormat("CURRENT FPS: %i", (int)(1.0f / deltaTime)),
                 GetScreenWidth() - 240,
                 40,
                 20,
                 GREEN);

        for (int i = 0; i < 20; ++i)
        {
            DrawEntity(groundEntities + i, cv);
        }

        for (int i = 0; i < 4; ++i)
        {
            DrawEntity(boxEntities + i, cv);
        }

        EndDrawing();

        // NOTE: In case raylib is configured to SUPPORT_CUSTOM_FRAME_CONTROL,
        // Events polling, screen buffer swap and frame time control must be managed by the user

        // SwapScreenBuffer(); // Flip the back buffer to screen (front buffer)

        currentTime = GetTime();
        // uint64_t endPerf = SDL_GetPerformanceCounter();
        updateDrawTime = currentTime - previousTime;
        // float elapsedMS = (endPerf - startPerf) / (float)SDL_GetPerformanceFrequency() * 1000.0f;

        // if (targetFPS > 0) // We want a fixed frame rate
        // {

        //     waitTime = (1.0f / (float)targetFPS) - updateDrawTime;
        //     if (waitTime > 0.0)
        //     {
        //         WaitTime((float)waitTime);
        //         currentTime = GetTime();
        //         deltaTime = (float)(currentTime - previousTime);
        //     }
        // }
        // else
        deltaTime = (float)updateDrawTime; // Framerate could be variable

        previousTime = currentTime;
        // SDL_Delay(floor(16.66666f - elapsedMS));
    }

    // Cleanup
    //---------------------------------------------------------------------
    UnloadTexture(textures[0]);
    UnloadTexture(textures[1]);

    CloseWindow();

    return 0;
}
