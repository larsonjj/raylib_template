#if defined(PLATFORM_DESKTOP) && defined(GRAPHICS_API_OPENGL_ES2)
#include "GLFW/glfw3.h"
#endif
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
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

#if defined(PLATFORM_DESKTOP) && defined(GRAPHICS_API_OPENGL_ES2)
#if defined(__APPLE__)
    glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_METAL);
#elif defined(_WIN32)
    glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_D3D11);
#endif
#endif
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "raylib box2d - custom frame control");

    bool pause = false; // Pause control flag
    int targetFPS = 60;

    double timeCounter = 0.0;
    float fixedFPS = 60.0f;
    float maxFPS = 1200.0f;
    double fixedTimeStep = 1.0f / fixedFPS;
    double maxTimeStep = 1.0f / maxFPS;

    double currentTime = GetTime();
    double accumulator = 0.0;

    float position = 0.0f; // Circle position

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
        bodyDef.angle = 0.25f * b2_pi * i;

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
        //----------------------------------------------------------------------------------

        double newTime = GetTime();
        double frameTime = newTime - currentTime;

        // Avoid spiral of death if CPU  can't keep up with target FPS
        if (frameTime > 0.25)
        {
            frameTime = 0.25;
        }

        currentTime = newTime;

        // Limit frame rate to avoid 100% CPU usage
        if (frameTime < maxTimeStep)
        {
            float waitTime = (float)(maxTimeStep - frameTime);
            WaitTime(1.0f / 1000.0f);
        }


        // Input
        //----------------------------------------------------------------------------------
        PollInputEvents(); // Poll input events (SUPPORT_CUSTOM_FRAME_CONTROL)

        if (IsKeyPressed(KEY_SPACE))
        {
            pause = !pause;
        }

        if (!pause)
        {
            accumulator += frameTime;

            while (accumulator >= fixedTimeStep)
            {
                // Fixed Update
                //--------------------------------------------------------------------------

                position += (float)(200 * fixedTimeStep); // We move at 200 pixels per second
                if (position >= (float)GetScreenWidth())
                    position = 0;
                timeCounter += fixedTimeStep; // We count time (seconds)
                b2World_Step(worldId, (float)fixedTimeStep, 4);

                timeCounter += fixedTimeStep;
                accumulator -= fixedTimeStep;
            }

            // Left over time to be used for interpolation within Update
            const double alpha = accumulator / fixedTimeStep;
        }

        // Drawing
        //----------------------------------------------------------------------------------

        BeginDrawing();

        ClearBackground(GRAY);
        // Draw
        for (int i = 0; i < 20; ++i)
        {
            DrawEntity(groundEntities + i, cv);
        }

        for (int i = 0; i < 4; ++i)
        {
            DrawEntity(boxEntities + i, cv);
        }

        DrawCircle((int)position, GetScreenHeight() / 2, 50, RED);
        DrawText("PRESS SPACE to PAUSE MOVEMENT", 20, GetScreenHeight() - 40, 20, WHITE);
        DrawText(TextFormat("Fixed FPS: %i", (int)fixedFPS), GetScreenWidth() - 200, 20, 20, GREEN);
        DrawText(TextFormat("Current FPS: %i", (int)(1.0 / frameTime)),
                 GetScreenWidth() - 200,
                 40,
                 20,
                 GREEN);

        EndDrawing();

        SwapScreenBuffer(); // Flip the back buffer to screen (front buffer)
    }

    // Cleanup
    //-------------------------------------------------------------------------------------

    UnloadTexture(textures[0]);
    UnloadTexture(textures[1]);

    CloseWindow();

    return 0;
}
