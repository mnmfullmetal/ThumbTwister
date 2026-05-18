#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void StartVisualiser()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1200, 800, "ThumbTwister Visualizer");
    SetTargetFPS(500);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(WHITE));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(WHITE));
}

void DrawControllerState(float rawLeftX, float rawLeftY, float snapLeftX, float snapLeftY, float rawRightX, float rawRightY, float adjRightX, float adjRightY)
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    float circleRadius = screenHeight * 0.20f;
    float leftCircleCenterX = screenWidth * 0.25f;
    float leftCircleCenterY = screenHeight * 0.45f;
    float rightCircleCenterX = screenWidth * 0.75f;
    float rightCircleCenterY = screenHeight * 0.45f;

    float leftBoxX = 40.0f;
    float leftBoxY = 40.0f;
    float leftBoxWidth = (screenWidth / 2.0f) - 60.0f;
    float leftBoxHeight = screenHeight - 80.0f;
    GuiGroupBox(Rectangle{ leftBoxX, leftBoxY, leftBoxWidth, leftBoxHeight }, "Left Stick");

    DrawCircleLines((int)leftCircleCenterX, (int)leftCircleCenterY, circleRadius, LIGHTGRAY);
    DrawText("LEFT STICK", (int)(leftCircleCenterX - 60), (int)(leftCircleCenterY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawLeftX), (int)(leftCircleCenterX - circleRadius), (int)(leftCircleCenterY + circleRadius + 20), 20, RED);
    DrawText(TextFormat("RAW Y: %0.4f", rawLeftY), (int)(leftCircleCenterX - circleRadius), (int)(leftCircleCenterY + circleRadius + 50), 20, RED);

    DrawCircle((int)(leftCircleCenterX + rawLeftX * circleRadius), (int)(leftCircleCenterY - rawLeftY * circleRadius), 14, RED);

    float rightBoxX = (screenWidth / 2.0f) + 20.0f;
    float rightBoxY = 40.0f;
    float rightBoxWidth = (screenWidth / 2.0f) - 60.0f;
    float rightBoxHeight = screenHeight - 80.0f;
    GuiGroupBox(Rectangle{ rightBoxX, rightBoxY, rightBoxWidth, rightBoxHeight }, "Right Stick");

    DrawCircleLines((int)rightCircleCenterX, (int)rightCircleCenterY, circleRadius, LIGHTGRAY);
    DrawText("RIGHT STICK", (int)(rightCircleCenterX - 60), (int)(rightCircleCenterY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawRightX), (int)(rightCircleCenterX - circleRadius), (int)(rightCircleCenterY + circleRadius + 20), 20, BLUE);
    DrawText(TextFormat("RAW Y: %0.4f", rawRightY), (int)(rightCircleCenterX - circleRadius), (int)(rightCircleCenterY + circleRadius + 50), 20, BLUE);

    DrawCircle((int)(rightCircleCenterX + rawRightX * circleRadius), (int)(rightCircleCenterY - rawRightY * circleRadius), 14, BLUE);

    DrawLine((int)(leftCircleCenterX - circleRadius), (int)leftCircleCenterY, (int)(leftCircleCenterX + circleRadius), (int)leftCircleCenterY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)leftCircleCenterX, (int)(leftCircleCenterY - circleRadius), (int)leftCircleCenterX, (int)(leftCircleCenterY + circleRadius), Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)(rightCircleCenterX - circleRadius), (int)rightCircleCenterY, (int)(rightCircleCenterX + circleRadius), (int)rightCircleCenterY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)rightCircleCenterX, (int)(rightCircleCenterY - circleRadius), (int)rightCircleCenterX, (int)(rightCircleCenterY + circleRadius), Fade(LIGHTGRAY, 0.3f));

    EndDrawing();
}

void StopVisualiser() {
    CloseWindow();
}

bool VisualiserShouldClose() {
    return WindowShouldClose();
}