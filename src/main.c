#include "raylib.h"
#include <math.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

#define WORLD_WIDTH 2000
#define WORLD_HEIGHT 1200

#define PLAYER_SPEED 210.0f   // px/s

#define MAX_OBSTACLES 32
#define MAX_ROSES 16

typedef struct {
    Vector2 pos;      // top-left in world
    bool collected;
} Rose;

static float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

static bool collides_any(Rectangle r, const Rectangle *obs, int obsCount) {
    for (int i = 0; i < obsCount; i++) {
        if (CheckCollisionRecs(r, obs[i])) return true;
    }
    return false;
}

// Robust axis-separated movement
static Vector2 move_with_collisions(Vector2 pos, Vector2 delta, Vector2 size,
                                    const Rectangle *obs, int obsCount)
{
    // X axis
    if (delta.x != 0.0f) {
        Rectangle next = (Rectangle){ pos.x + delta.x, pos.y, size.x, size.y };
        if (!collides_any(next, obs, obsCount)) {
            pos.x += delta.x;
        } else {
            float step = (delta.x > 0) ? 1.0f : -1.0f;
            while (fabsf(delta.x) > 0.0f) {
                Rectangle tryr = (Rectangle){ pos.x + step, pos.y, size.x, size.y };
                if (collides_any(tryr, obs, obsCount)) break;
                pos.x += step;
                delta.x -= step;
                if (fabsf(delta.x) < 1.0f) break;
            }
        }
    }

    // Y axis
    if (delta.y != 0.0f) {
        Rectangle next = (Rectangle){ pos.x, pos.y + delta.y, size.x, size.y };
        if (!collides_any(next, obs, obsCount)) {
            pos.y += delta.y;
        } else {
            float step = (delta.y > 0) ? 1.0f : -1.0f;
            while (fabsf(delta.y) > 0.0f) {
                Rectangle tryr = (Rectangle){ pos.x, pos.y + step, size.x, size.y };
                if (collides_any(tryr, obs, obsCount)) break;
                pos.y += step;
                delta.y -= step;
                if (fabsf(delta.y) < 1.0f) break;
            }
        }
    }

    return pos;
}

static void draw_texture_centered(Texture2D tex, Vector2 centerWorld, float scale, Color tint) {
    float w = (float)tex.width * scale;
    float h = (float)tex.height * scale;
    Rectangle src = (Rectangle){ 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dst = (Rectangle){ centerWorld.x, centerWorld.y, w, h };
    Vector2 origin = (Vector2){ w * 0.5f, h * 0.5f };
    DrawTexturePro(tex, src, dst, origin, 0.0f, tint);
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Roses de Sant Feliu - La Ruta Orgullosa");
    SetTargetFPS(60);

    // Load sprites
    // Important: in web build, assets must be packed with --preload-file assets@assets
    Texture2D texPlayer = LoadTexture("assets/player.png");
    Texture2D texRose   = LoadTexture("assets/rose.png");
    Texture2D texTent   = LoadTexture("assets/tent.png");

    bool hasPlayer = (texPlayer.id != 0);
    bool hasRose   = (texRose.id != 0);
    bool hasTent   = (texTent.id != 0);

    // Player hitbox based on sprite size (or fallback)
    Vector2 playerSize;
    if (hasPlayer) {
        playerSize = (Vector2){ (float)texPlayer.width * 0.70f, (float)texPlayer.height * 0.70f };
        // Minimum so it never becomes tiny
        if (playerSize.x < 18) playerSize.x = 18;
        if (playerSize.y < 18) playerSize.y = 18;
    } else {
        playerSize = (Vector2){ 26, 26 };
    }

    Vector2 playerPos = (Vector2){ 180.0f, 220.0f };

    // Obstacles (placeholder layout)
    Rectangle obstacles[MAX_OBSTACLES];
    int obsCount = 0;

    obstacles[obsCount++] = (Rectangle){ 120, 140, 520, 20 };
    obstacles[obsCount++] = (Rectangle){ 120, 140, 20, 260 };
    obstacles[obsCount++] = (Rectangle){ 620, 140, 20, 260 };
    obstacles[obsCount++] = (Rectangle){ 120, 380, 520, 20 };

    obstacles[obsCount++] = (Rectangle){ 800, 220, 220, 60 };
    obstacles[obsCount++] = (Rectangle){ 1040, 120, 60, 260 };
    obstacles[obsCount++] = (Rectangle){ 1180, 320, 260, 70 };
    obstacles[obsCount++] = (Rectangle){ 1460, 180, 90, 260 };

    obstacles[obsCount++] = (Rectangle){ 520, 620, 300, 220 };

    obstacles[obsCount++] = (Rectangle){ 1200, 760, 520, 18 };
    obstacles[obsCount++] = (Rectangle){ 1200, 940, 520, 18 };
    obstacles[obsCount++] = (Rectangle){ 1200, 760, 18, 198 };
    obstacles[obsCount++] = (Rectangle){ 1702, 760, 18, 198 };

    obstacles[obsCount++] = (Rectangle){ 1680, 420, 250, 18 };
    obstacles[obsCount++] = (Rectangle){ 1680, 420, 18, 180 };
    obstacles[obsCount++] = (Rectangle){ 1912, 420, 18, 180 };
    obstacles[obsCount++] = (Rectangle){ 1680, 582, 250, 18 };

    // Roses
    Rose roses[MAX_ROSES];
    int totalRoses = 8;
    int rosesCollected = 0;

    roses[0] = (Rose){ (Vector2){ 300, 250 }, false };
    roses[1] = (Rose){ (Vector2){ 520, 300 }, false };
    roses[2] = (Rose){ (Vector2){ 900, 180 }, false };
    roses[3] = (Rose){ (Vector2){ 1080, 420 }, false };
    roses[4] = (Rose){ (Vector2){ 620, 760 }, false };
    roses[5] = (Rose){ (Vector2){ 1320, 840 }, false };
    roses[6] = (Rose){ (Vector2){ 1560, 840 }, false };
    roses[7] = (Rose){ (Vector2){ 1820, 520 }, false };

    // Carpa zone and tent position
    Rectangle carpaZone = (Rectangle){ 1720, 460, 160, 120 };
    Vector2 tentCenter = (Vector2){ carpaZone.x + carpaZone.width * 0.5f, carpaZone.y + carpaZone.height * 0.5f };

    bool win = false;

    // Camera
    Camera2D cam = {0};
    cam.offset = (Vector2){ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f };
    cam.target = (Vector2){ playerPos.x + playerSize.x * 0.5f, playerPos.y + playerSize.y * 0.5f };
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Input direction
        Vector2 dir = (Vector2){0};
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.y += 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1.0f;

        // Normalize diagonal
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
        if (len > 0.0f) {
            dir.x /= len;
            dir.y /= len;
        }

        Vector2 delta = (Vector2){ dir.x * PLAYER_SPEED * dt, dir.y * PLAYER_SPEED * dt };

        // Move with collisions
        playerPos = move_with_collisions(playerPos, delta, playerSize, obstacles, obsCount);

        // Clamp within world bounds
        playerPos.x = clampf(playerPos.x, 0.0f, (float)WORLD_WIDTH - playerSize.x);
        playerPos.y = clampf(playerPos.y, 0.0f, (float)WORLD_HEIGHT - playerSize.y);

        Rectangle playerRect = (Rectangle){ playerPos.x, playerPos.y, playerSize.x, playerSize.y };

        // Collect roses
        for (int i = 0; i < totalRoses; i++) {
            if (!roses[i].collected) {
                Rectangle roseRect = (Rectangle){ roses[i].pos.x, roses[i].pos.y, 18, 18 };
                if (CheckCollisionRecs(playerRect, roseRect)) {
                    roses[i].collected = true;
                    rosesCollected++;
                }
            }
        }

        // Win condition
        if (!win && rosesCollected == totalRoses && CheckCollisionRecs(playerRect, carpaZone)) {
            win = true;
        }

        // Camera follow (smooth)
        Vector2 desired = (Vector2){ playerPos.x + playerSize.x * 0.5f, playerPos.y + playerSize.y * 0.5f };
        cam.target.x += (desired.x - cam.target.x) * 0.12f;
        cam.target.y += (desired.y - cam.target.y) * 0.12f;

        // Keep camera inside world
        float halfW = (SCREEN_WIDTH * 0.5f) / cam.zoom;
        float halfH = (SCREEN_HEIGHT * 0.5f) / cam.zoom;
        cam.target.x = clampf(cam.target.x, halfW, (float)WORLD_WIDTH - halfW);
        cam.target.y = clampf(cam.target.y, halfH, (float)WORLD_HEIGHT - halfH);

        if (IsKeyPressed(KEY_Q)) cam.zoom = clampf(cam.zoom - 0.1f, 0.6f, 2.0f);
        if (IsKeyPressed(KEY_E)) cam.zoom = clampf(cam.zoom + 0.1f, 0.6f, 2.0f);

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(cam);

        // Background
        DrawRectangle(0, 0, WORLD_WIDTH, WORLD_HEIGHT, (Color){245, 245, 245, 255});
        DrawRectangleLines(0, 0, WORLD_WIDTH, WORLD_HEIGHT, LIGHTGRAY);

        // Labels (debug/placeholder)
        DrawText("Palau Falguera", 160, 110, 20, DARKBLUE);
        DrawText("Catedral", 600, 590, 20, DARKPURPLE);
        DrawText("Roserar", 1320, 720, 20, DARKGREEN);
        DrawText("Carpa Orgullosament", 1700, 390, 20, MAROON);

        // Obstacles
        for (int i = 0; i < obsCount; i++) {
            DrawRectangleRec(obstacles[i], (Color){120, 120, 120, 255});
        }

        // Carpa zone (visual)
        DrawRectangleRec(carpaZone, Fade(ORANGE, 0.18f));
        DrawRectangleLinesEx(carpaZone, 2, ORANGE);

        // Tent sprite centered in carpa zone
        if (hasTent) {
            draw_texture_centered(texTent, tentCenter, 1.0f, WHITE);
        } else {
            DrawRectangle((int)carpaZone.x + 30, (int)carpaZone.y + 25, 100, 70, ORANGE);
            DrawText("CARPA", (int)carpaZone.x + 52, (int)carpaZone.y + 50, 16, MAROON);
        }

        // Roses sprites
        for (int i = 0; i < totalRoses; i++) {
            if (!roses[i].collected) {
                if (hasRose) {
                    // Draw rose at top-left pos, scaled to 18x18-ish
                    float scale = 18.0f / (float)texRose.width;
                    DrawTextureEx(texRose, roses[i].pos, 0.0f, scale, WHITE);
                } else {
                    DrawCircleV((Vector2){roses[i].pos.x + 9, roses[i].pos.y + 9}, 9, PINK);
                }
            }
        }

        // Player sprite
        if (hasPlayer) {
            Vector2 playerCenter = (Vector2){ playerPos.x + playerSize.x * 0.5f, playerPos.y + playerSize.y * 0.5f };
            float scale = (playerSize.x / (float)texPlayer.width);
            draw_texture_centered(texPlayer, playerCenter, scale, WHITE);
        } else {
            DrawRectangleRec(playerRect, BLUE);
        }

        EndMode2D();

        // HUD
        DrawRectangle(12, 12, 250, 62, Fade(WHITE, 0.92f));
        DrawRectangleLines(12, 12, 250, 62, LIGHTGRAY);
        DrawText("Roses de Sant Feliu", 22, 18, 16, BLACK);
        DrawText(TextFormat("Roses: %d / %d", rosesCollected, totalRoses), 22, 38, 18, BLACK);
        DrawText("Q/E: zoom", 22, 58, 12, GRAY);

        if (win) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.35f));
            DrawText("RUTA COMPLETADA!", 240, 190, 34, RAYWHITE);
            DrawText("Orgullosament Santfeliuenc", 235, 230, 20, RAYWHITE);
        }

        EndDrawing();
    }

    if (hasPlayer) UnloadTexture(texPlayer);
    if (hasRose)   UnloadTexture(texRose);
    if (hasTent)   UnloadTexture(texTent);

    CloseWindow();
    return 0;
}
