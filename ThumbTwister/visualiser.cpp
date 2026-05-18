#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// -- UI STATE VARS --
static bool leftEnabled = true;
static bool rightEnabled = true;
static bool leftManualMode = false;
static bool rightManualMode = false;
static bool calibrateLeftClicked = false;
static bool calibrateRightClicked = false;
static float leftOffsetDeg = 0.0f;
static float rightOffsetDeg = 0.0f;

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

bool VisualiserShouldClose() { return WindowShouldClose(); }
void StopVisualiser() { CloseWindow(); }

float GetLeftOffset() { return leftOffsetDeg; }
float GetRightOffset() { return rightOffsetDeg; }
void SetLeftOffset(float val) { leftOffsetDeg = val; }
void SetRightOffset(float val) { rightOffsetDeg = val; }

bool CheckCalibrateRight()
{
    bool temp = calibrateRightClicked;
    calibrateRightClicked = false;
    return temp;
}

bool CheckCalibrateLeft()
{
    bool temp = calibrateLeftClicked;
    calibrateLeftClicked = false;
    return temp;
}

void DrawControllerState(float rawLeftX, float rawLeftY, float snapLeftX, float snapLeftY, float rawRightX, float rawRightY, float snapRightX, float snapRightY)
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
    DrawCircle((int)(leftCircleCenterX + rawLeftX * circleRadius), (int)(leftCircleCenterY - rawLeftY * circleRadius), 14, RED);

    DrawText("LEFT STICK", (int)(leftCircleCenterX - 60), (int)(leftCircleCenterY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawLeftX), (int)(leftCircleCenterX - circleRadius), (int)(leftCircleCenterY + circleRadius + 20), 20, RED);
    DrawText(TextFormat("RAW Y: %0.4f", rawLeftY), (int)(leftCircleCenterX - circleRadius), (int)(leftCircleCenterY + circleRadius + 50), 20, RED);

    GuiToggle(Rectangle{ leftBoxX + 20, leftBoxY + 20, 96, 24 }, "Enable", &leftEnabled);
    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - 116, leftBoxY + 20, 96, 24 }, "Calibrate")) calibrateLeftClicked = true;

    GuiCheckBox(Rectangle{ leftBoxX + 20, leftBoxY + leftBoxHeight - 40, 24, 24 }, "Manual Mode", &leftManualMode);

    if (!leftManualMode) GuiDisable();
    GuiSlider(Rectangle{ leftBoxX + 150, leftBoxY + leftBoxHeight - 40, leftBoxWidth - 280, 24 }, NULL, TextFormat("%.2f", leftOffsetDeg), &leftOffsetDeg, -180.0f, 180.0f);
    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - 70, leftBoxY + leftBoxHeight - 40, 50, 24 }, "Reset")) leftOffsetDeg = 0.0f;
    GuiEnable();

    float rightBoxX = (screenWidth / 2.0f) + 20.0f;
    float rightBoxY = 40.0f;
    float rightBoxWidth = (screenWidth / 2.0f) - 60.0f;
    float rightBoxHeight = screenHeight - 80.0f;
    GuiGroupBox(Rectangle{ rightBoxX, rightBoxY, rightBoxWidth, rightBoxHeight }, "Right Stick");

    DrawCircleLines((int)rightCircleCenterX, (int)rightCircleCenterY, circleRadius, LIGHTGRAY);
    DrawCircle((int)(rightCircleCenterX + rawRightX * circleRadius), (int)(rightCircleCenterY - rawRightY * circleRadius), 14, BLUE);

    DrawText("RIGHT STICK", (int)(rightCircleCenterX - 60), (int)(rightCircleCenterY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawRightX), (int)(rightCircleCenterX - circleRadius), (int)(rightCircleCenterY + circleRadius + 20), 20, BLUE);
    DrawText(TextFormat("RAW Y: %0.4f", rawRightY), (int)(rightCircleCenterX - circleRadius), (int)(rightCircleCenterY + circleRadius + 50), 20, BLUE);

    GuiToggle(Rectangle{ rightBoxX + 20, rightBoxY + 20, 96, 24 }, "Enable", &rightEnabled);
    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - 116, rightBoxY + 20, 96, 24 }, "Calibrate")) calibrateRightClicked = true;

    GuiCheckBox(Rectangle{ rightBoxX + 20, rightBoxY + rightBoxHeight - 40, 24, 24 }, "Manual Mode", &rightManualMode);

    if (!rightManualMode) GuiDisable();
    GuiSlider(Rectangle{ rightBoxX + 150, rightBoxY + rightBoxHeight - 40, rightBoxWidth - 280, 24 }, NULL, TextFormat("%.2f", rightOffsetDeg), &rightOffsetDeg, -180.0f, 180.0f);
    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - 70, rightBoxY + rightBoxHeight - 40, 50, 24 }, "Reset")) rightOffsetDeg = 0.0f;
    GuiEnable();

    DrawLine((int)(leftCircleCenterX - circleRadius), (int)leftCircleCenterY, (int)(leftCircleCenterX + circleRadius), (int)leftCircleCenterY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)leftCircleCenterX, (int)(leftCircleCenterY - circleRadius), (int)leftCircleCenterX, (int)(leftCircleCenterY + circleRadius), Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)(rightCircleCenterX - circleRadius), (int)rightCircleCenterY, (int)(rightCircleCenterX + circleRadius), (int)rightCircleCenterY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)rightCircleCenterX, (int)(rightCircleCenterY - circleRadius), (int)rightCircleCenterX, (int)(rightCircleCenterY + circleRadius), Fade(LIGHTGRAY, 0.3f));

    EndDrawing();
}