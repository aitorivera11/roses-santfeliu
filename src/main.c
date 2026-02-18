#include "raylib.h"
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

#define PLAYER_SPEED 3.0f
#define MAX_ROSES 10

typedef enum {
    SCREEN_PALAU = 0,
    SCREEN_CATEDRAL,
    SCREEN_ROSERAR,
    SCREEN_CARPA
} GameScreen;

typedef struct {
    Vector2 position;
    bool collected;
} Rose;

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Roses de Sant Feliu - La Ruta Orgullosa");

    SetTargetFPS(60);

    GameScreen currentScreen = SCREEN_PALAU;

    Rectangle player = {100, 200, 32, 32};

    Rose roses[MAX_ROSES];
    int totalRoses = 5;
    int rosesCollected = 0;

    // Inicialitzar roses (només al Roserar)
    for (int i = 0; i < totalRoses; i++)
    {
        roses[i].position = (Vector2){200 + i * 80, 200};
        roses[i].collected = false;
    }

    while (!WindowShouldClose())
    {
        // --- INPUT ---
        if (IsKeyDown(KEY_RIGHT)) player.x += PLAYER_SPEED;
        if (IsKeyDown(KEY_LEFT))  player.x -= PLAYER_SPEED;
        if (IsKeyDown(KEY_UP))    player.y -= PLAYER_SPEED;
        if (IsKeyDown(KEY_DOWN))  player.y += PLAYER_SPEED;

        // Limitar moviment a pantalla
        if (player.x < 0) player.x = 0;
        if (player.y < 0) player.y = 0;
        if (player.x + player.width > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.width;
        if (player.y + player.height > SCREEN_HEIGHT) player.y = SCREEN_HEIGHT - player.height;

        // --- LOGICA DE PORTES ---
        if (currentScreen == SCREEN_PALAU && player.x > SCREEN_WIDTH - 40)
        {
            currentScreen = SCREEN_CATEDRAL;
            player.x = 10;
        }
        else if (currentScreen == SCREEN_CATEDRAL && player.x < 5)
        {
            currentScreen = SCREEN_PALAU;
            player.x = SCREEN_WIDTH - 50;
        }
        else if (currentScreen == SCREEN_CATEDRAL && player.y > SCREEN_HEIGHT - 40)
        {
            currentScreen = SCREEN_ROSERAR;
            player.y = 10;
        }
        else if (currentScreen == SCREEN_ROSERAR && player.y < 5)
        {
            currentScreen = SCREEN_CATEDRAL;
            player.y = SCREEN_HEIGHT - 50;
        }
        else if (currentScreen == SCREEN_ROSERAR && player.x > SCREEN_WIDTH - 40)
        {
            currentScreen = SCREEN_CARPA;
            player.x = 10;
        }

        // --- COL·LECCIONABLES ---
        if (currentScreen == SCREEN_ROSERAR)
        {
            for (int i = 0; i < totalRoses; i++)
            {
                if (!roses[i].collected &&
                    CheckCollisionRecs(player,
                        (Rectangle){roses[i].position.x, roses[i].position.y, 20, 20}))
                {
                    roses[i].collected = true;
                    rosesCollected++;
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // --- DIBUIXAR ESCENA ---
        switch (currentScreen)
        {
            case SCREEN_PALAU:
                DrawText("Palau Falguera (Sortida)", 20, 20, 20, DARKBLUE);
                DrawText("Ves a la dreta →", 20, 50, 18, GRAY);
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(SKYBLUE, 0.2f));
                break;

            case SCREEN_CATEDRAL:
                DrawText("Catedral", 20, 20, 20, DARKPURPLE);
                DrawText("↓ Roserar", 20, 50, 18, GRAY);
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(LIGHTGRAY, 0.3f));
                break;

            case SCREEN_ROSERAR:
                DrawText("Roserar - Recull les roses!", 20, 20, 20, DARKGREEN);
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(GREEN, 0.15f));

                for (int i = 0; i < totalRoses; i++)
                {
                    if (!roses[i].collected)
                    {
                        DrawCircle(roses[i].position.x + 10,
                                   roses[i].position.y + 10,
                                   10,
                                   PINK);
                    }
                }
                break;

            case SCREEN_CARPA:
                DrawText("Carpa Orgullosament Santfeliuenc", 20, 20, 20, MAROON);
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(ORANGE, 0.2f));

                if (rosesCollected == totalRoses)
                {
                    DrawText("Ruta completada!", 250, 200, 30, DARKGREEN);
                }
                else
                {
                    DrawText("Encara falten roses...", 250, 200, 20, RED);
                }
                break;
        }

        // --- HUD ---
        DrawText(TextFormat("Roses: %d / %d", rosesCollected, totalRoses),
                 600, 20, 20, BLACK);

        DrawRectangleRec(player, BLUE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
