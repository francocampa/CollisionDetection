#include "raylib.h"
#include <cmath>
#include <vector>

#define MAX_BUILDINGS   100

Vector2 handle_movement(){
    Vector2 res(0);
    if (IsKeyDown(KEY_A)) {
        res.x -= 1.0f;
    }
    if (IsKeyDown(KEY_D)) {
        res.x += 1.0f;
    }
    if (IsKeyDown(KEY_W)) {
        res.y -= 1.0f;
    }
    if (IsKeyDown(KEY_S)) {
        res.y += 1.0f;
    }
    
    return res;
}

int main(void)
{
    SetRandomSeed(2);
    InitWindow(800, 450, "broad phase example");
    SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    bool showMessageBox = false;

    float vel = 100.0f;

    Camera2D camera;
    camera.target = (Vector2){ 0.0f, 0.0f};
    camera.zoom = 1.0f;
    camera.offset = Vector2(0.0f, 0.0f);
    camera.rotation = 0.0f;

    Rectangle buildings[MAX_BUILDINGS] = { 0 };
    Color buildColors[MAX_BUILDINGS] = { 0 };

    int spacing = 0;
    for (int i = 0; i < MAX_BUILDINGS; i++)
    {
        buildings[i].width = (float)GetRandomValue(50, 200);
        buildings[i].height = (float)GetRandomValue(100, 800);
        buildings[i].y = GetScreenHeight() - 130.0f - buildings[i].height;
        buildings[i].x = -6000.0f + spacing;

        spacing += (int)buildings[i].width;

        buildColors[i] = (Color){
            (unsigned char)GetRandomValue(200, 240),
            (unsigned char)GetRandomValue(200, 240),
            (unsigned char)GetRandomValue(200, 250),
            255};
    }


    std::vector<Vector2> floor;

    float floor_start = -1000.0f;
    float floor_end = -floor_start;
    float step = 50.0f;
    float ground_y = 1000.0f;

    for (float x = floor_start; x <= floor_end; x += step) {
        float y = std::sin(x * 0.05f) * 20.0f;
        floor.push_back({x, y});
    }
    
    while (!WindowShouldClose())
    {
        Vector2 movement = handle_movement();

        float delta = GetFrameTime();
        float deltaVel = delta * vel;

        camera.target = Vector2(camera.target.x + movement.x * deltaVel, camera.target.y + movement.y * deltaVel);

        BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode2D(camera);
                for (int i = 0; i < MAX_BUILDINGS; i++) DrawRectangleRec(buildings[i], buildColors[i]);
                
                for (size_t i = 0; i + 1 < floor.size(); ++i) {
                    Vector2 a = floor[i];
                    Vector2 b = floor[i + 1];

                    Vector2 c = {b.x, ground_y};
                    Vector2 d = {a.x, ground_y};

                    DrawTriangle(a, d, c, GREEN);
                    DrawTriangle(a, c, b, GREEN);
                }
            EndMode2D();
            
            
            DrawText(TextFormat("X: %f Y: %f", camera.target.x, camera.target.y), 10, 30, 18, WHITE);
            // DrawText(TextFormat("X: %f Y: %f", GetMousePosition().x, GetMousePosition().y), 10, 60, 18, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}