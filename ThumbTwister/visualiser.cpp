#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include <cmath>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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
bool VisualiserShouldClose() 
{ 
    return WindowShouldClose(); 
}
void StopVisualiser() 
{ 
    CloseWindow(); 
}
float GetLeftOffset() 
{ 
    return leftOffsetDeg; 
}
float GetRightOffset() 
{
    return rightOffsetDeg;
}
void SetLeftOffset(float val) 
{
    leftOffsetDeg = val; 
}
void SetRightOffset(float val) 
{
    rightOffsetDeg = val;
}
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

    auto ClampToCircle = [](float& x, float& y) 
    {
        float mag = sqrtf(x * x + y * y);
        if (mag > 1.0f) 
        {
            x /= mag;
            y /= mag;
        }
    };

    float drawableRawLeftX = rawLeftX, drawableRawLeftY = rawLeftY;
    float drawableSnapLeftX = snapLeftX, drawableAdjLeftY = snapLeftY;
    ClampToCircle(drawableRawLeftX, drawableRawLeftY);
    ClampToCircle(drawableSnapLeftX, drawableAdjLeftY);

    float drawableRawRightX = rawRightX, drawableRawRightY = rawRightY;
    float drawableAdjRightX = snapRightX, drawableSnapRightY = snapRightY;
    ClampToCircle(drawableRawRightX, drawableRawRightY);
    ClampToCircle(drawableAdjRightX, drawableSnapRightY);

    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    float circleRadius = screenHeight * 0.20f;
    float leftCircleCentreX = screenWidth * 0.25f;
    float leftCircleCenterY = screenHeight * 0.45f;
    float rightCircleCentreX = screenWidth * 0.75f;
    float rightCircleCentreY = screenHeight * 0.45f;

    float leftBoxX = 40.0f, leftBoxY = 40.0f;
    float leftBoxWidth = (screenWidth / 2.0f) - 60.0f, leftBoxHeight = screenHeight - 80.0f;
    GuiGroupBox(Rectangle{ leftBoxX, leftBoxY, leftBoxWidth, leftBoxHeight }, "Left Stick");

    DrawCircleLines((int)leftCircleCentreX, (int)leftCircleCenterY, circleRadius, LIGHTGRAY);
    DrawCircleLines((int)leftCircleCentreX, (int)leftCircleCenterY, circleRadius * 0.2f, Fade(YELLOW, 0.2f));

    float distanceLeft = sqrtf(rawLeftX * rawLeftX + rawLeftY * rawLeftY) * circleRadius;
    if (distanceLeft > circleRadius) distanceLeft = circleRadius;

    if (distanceLeft > 5.0f)
    {
        float startAngleLeft = atan2f(-rawLeftY, rawLeftX) * (180.0f / 3.14159265f);
        float endAngleLeft = startAngleLeft - leftOffsetDeg;
        float minAngleLeft = fminf(startAngleLeft, endAngleLeft);
        float maxAngleLeft = fmaxf(startAngleLeft, endAngleLeft);
        DrawRing(Vector2{ leftCircleCentreX, leftCircleCenterY }, distanceLeft - 1.5f, distanceLeft + 1.5f, minAngleLeft, maxAngleLeft, 32, WHITE);
    }

    DrawCircle((int)(leftCircleCentreX + drawableRawLeftX * circleRadius), (int)(leftCircleCenterY - drawableRawLeftY * circleRadius), 14, RED);
    DrawCircle((int)(leftCircleCentreX + drawableSnapLeftX * circleRadius), (int)(leftCircleCenterY - drawableAdjLeftY * circleRadius), 14, GREEN);

    DrawText("LEFT STICK", (int)(leftCircleCentreX - 60), (int)(leftCircleCenterY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawLeftX), (int)(leftCircleCentreX - circleRadius), (int)(leftCircleCenterY + circleRadius + 20), 20, RED);
    DrawText(TextFormat("RAW Y: %0.4f", rawLeftY), (int)(leftCircleCentreX - circleRadius), (int)(leftCircleCenterY + circleRadius + 50), 20, RED);
    DrawText(TextFormat("SNP X: %0.4f", snapLeftX), (int)(leftCircleCentreX + 20), (int)(leftCircleCenterY + circleRadius + 20), 20, GREEN);
    DrawText(TextFormat("SNP Y: %0.4f", snapLeftY), (int)(leftCircleCentreX + 20), (int)(leftCircleCenterY + circleRadius + 50), 20, GREEN);

    float rightBoxX = (screenWidth / 2.0f) + 20.0f, rightBoxY = 40.0f;
    float rightBoxWidth = (screenWidth / 2.0f) - 60.0f, rightBoxHeight = screenHeight - 80.0f;
    GuiGroupBox(Rectangle{ rightBoxX, rightBoxY, rightBoxWidth, rightBoxHeight }, "Right Stick");

    DrawCircleLines((int)rightCircleCentreX, (int)rightCircleCentreY, circleRadius, LIGHTGRAY);
    DrawCircleLines((int)rightCircleCentreX, (int)rightCircleCentreY, circleRadius * 0.2f, Fade(YELLOW, 0.2f));

    float distanceRight = sqrtf(rawRightX * rawRightX + rawRightY * rawRightY) * circleRadius;
    if (distanceRight > circleRadius) distanceRight = circleRadius;

    if (distanceRight > 5.0f)
    {
        float startAngleRight = atan2f(-rawRightY, rawRightX) * (180.0f / 3.14159265f);
        float endAngleRight = startAngleRight - rightOffsetDeg;
        float minAngleRight = fminf(startAngleRight, endAngleRight);
        float maxAngleRight = fmaxf(startAngleRight, endAngleRight);
        DrawRing(Vector2{ rightCircleCentreX, rightCircleCentreY }, distanceRight - 1.5f, distanceRight + 1.5f, minAngleRight, maxAngleRight, 32, WHITE);
    }

    DrawCircle((int)(rightCircleCentreX + drawableRawRightX * circleRadius), (int)(rightCircleCentreY - drawableRawRightY * circleRadius), 14, BLUE);
    DrawCircle((int)(rightCircleCentreX + drawableAdjRightX * circleRadius), (int)(rightCircleCentreY - drawableSnapRightY * circleRadius), 14, GREEN);

    DrawText("RIGHT STICK", (int)(rightCircleCentreX - 60), (int)(rightCircleCentreY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawRightX), (int)(rightCircleCentreX - circleRadius), (int)(rightCircleCentreY + circleRadius + 20), 20, BLUE);
    DrawText(TextFormat("RAW Y: %0.4f", rawRightY), (int)(rightCircleCentreX - circleRadius), (int)(rightCircleCentreY + circleRadius + 50), 20, BLUE);
    DrawText(TextFormat("SNP X: %0.4f", snapRightX), (int)(rightCircleCentreX + 20), (int)(rightCircleCentreY + circleRadius + 20), 20, GREEN);
    DrawText(TextFormat("SNP Y: %0.4f", snapRightY), (int)(rightCircleCentreX + 20), (int)(rightCircleCentreY + circleRadius + 50), 20, GREEN);

    GuiToggle(Rectangle{ leftBoxX + 20, leftBoxY + 20, 96, 24 }, "Enable", &leftEnabled);
    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - 116, leftBoxY + 20, 96, 24 }, "Calibrate")) calibrateLeftClicked = true;
    GuiCheckBox(Rectangle{ leftBoxX + 20, leftBoxY + leftBoxHeight - 40, 24, 24 }, "Manual Mode", &leftManualMode);
    if (!leftManualMode) GuiDisable();
    GuiSlider(Rectangle{ leftBoxX + 150, leftBoxY + leftBoxHeight - 40, leftBoxWidth - 280, 24 }, NULL, TextFormat("%.2f", leftOffsetDeg), &leftOffsetDeg, -180.0f, 180.0f);
    leftOffsetDeg = roundf(leftOffsetDeg * 10.0f) / 10.0f; 
    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - 70, leftBoxY + leftBoxHeight - 40, 50, 24 }, "Reset")) leftOffsetDeg = 0.0f;
    GuiEnable();

    GuiToggle(Rectangle{ rightBoxX + 20, rightBoxY + 20, 96, 24 }, "Enable", &rightEnabled);
    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - 116, rightBoxY + 20, 96, 24 }, "Calibrate")) calibrateRightClicked = true;
    GuiCheckBox(Rectangle{ rightBoxX + 20, rightBoxY + rightBoxHeight - 40, 24, 24 }, "Manual Mode", &rightManualMode);
    if (!rightManualMode) GuiDisable();
    GuiSlider(Rectangle{ rightBoxX + 150, rightBoxY + rightBoxHeight - 40, rightBoxWidth - 280, 24 }, NULL, TextFormat("%.2f", rightOffsetDeg), &rightOffsetDeg, -180.0f, 180.0f);
    rightOffsetDeg = roundf(rightOffsetDeg * 10.0f) / 10.0f; 
    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - 70, rightBoxY + rightBoxHeight - 40, 50, 24 }, "Reset")) rightOffsetDeg = 0.0f;
    GuiEnable();

    DrawLine((int)(leftCircleCentreX - circleRadius), (int)leftCircleCenterY, (int)(leftCircleCentreX + circleRadius), (int)leftCircleCenterY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)leftCircleCentreX, (int)(leftCircleCenterY - circleRadius), (int)leftCircleCentreX, (int)(leftCircleCenterY + circleRadius), Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)(rightCircleCentreX - circleRadius), (int)rightCircleCentreY, (int)(rightCircleCentreX + circleRadius), (int)rightCircleCentreY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)rightCircleCentreX, (int)(rightCircleCentreY - circleRadius), (int)rightCircleCentreX, (int)(rightCircleCentreY + circleRadius), Fade(LIGHTGRAY, 0.3f));

    EndDrawing();
}