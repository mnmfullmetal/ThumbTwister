#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include <vector> 
#include <cstring> 
#include <cmath>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

Color VERY_DARKGRAY = { 30, 30, 30, 255 };

enum CalibState
{
    IDLE,
    CALIBRATING,
};

static CalibState currentState = IDLE;
static char currentInstruction[64] = "READY";
static bool needsCentre = false;
static float leftOffsetDeg = 0.0f;
static float rightOffsetDeg = 0.0f;

static std::vector<Vector2> calibrationDots;
static bool calibratingLeft = true;

// -- UI STATE VARS --
static bool leftEnabled = true;
static bool rightEnabled = true;
static bool leftManualMode = false;
static bool rightManualMode = false;
static bool calibrateLeftClicked = false;
static bool calibrateRightClicked = false;

void SetCalibrationDots(std::vector<Vector2> points, bool isLeft)
{
    calibrationDots = points;
    calibratingLeft = isLeft;
}

void ClearCalibrationDots()
{
    calibrationDots.clear();
}

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

void SetCalibrationUI(const char* text, bool requireCentre)
{
    currentState = CALIBRATING;
    strcpy(currentInstruction, text);
    needsCentre = requireCentre;
}

void StopCalibrationUI()
{
    currentState = IDLE;
}

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

void DrawControllerState(float rawLeftX, float rawLeftY, float adjLeftX, float adjLeftY, float rawRightX, float rawRightY, float adjRightX, float adjRightY)
{
    BeginDrawing();
    ClearBackground(VERY_DARKGRAY);

    // --- VISUAL CLAMP  ---
    auto ClampToCircle = [](float& x, float& y) 
    {
        float mag = sqrtf(x * x + y * y);
        if (mag > 1.0f) {
            x /= mag;
            y /= mag;
        }
    };

    float drawableRawLeftX = rawLeftX, drawableRawLeftY = rawLeftY;
    float drawableAdjLeftX = adjLeftX, drawableAdjLeftY = adjLeftY;
    ClampToCircle(drawableRawLeftX, drawableRawLeftY);
    ClampToCircle(drawableAdjLeftX, drawableAdjLeftY);

    float drawableRawRightX = rawRightX, drawableRawRightY = rawRightY;
    float drawableAdjRightX = adjRightX, drawableAdjRightY = adjRightY;
    ClampToCircle(drawableRawRightX, drawableRawRightY);
    ClampToCircle(drawableAdjRightX, drawableAdjRightY);

    // --- DYNAMIC SCALING ---
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();
    float circleRadius = screenHeight * 0.20f;
    float leftCircleCentreX = screenWidth * 0.25f;
    float leftCircleCentreY = screenHeight * 0.45f;
    float rightCircleCentreX = screenWidth * 0.75f;
    float rightCircleCentreY = screenHeight * 0.45f;

    // --- LEFT STICK ---
    float leftBoxX = 40.0f;
    float leftBoxY = 40.0f;
    float leftBoxWidth = (screenWidth / 2.0f) - 60.0f;
    float leftBoxHeight = screenHeight - 80.0f;
    GuiGroupBox(Rectangle{ leftBoxX, leftBoxY, leftBoxWidth, leftBoxHeight }, "Left Stick");

    DrawCircleLines((int)leftCircleCentreX, (int)leftCircleCentreY, circleRadius, LIGHTGRAY);
    DrawCircleLines((int)leftCircleCentreX, (int)leftCircleCentreY, circleRadius * 0.2f, Fade(YELLOW, 0.2f));

    // calc ring tracking distance and clamp it to stop squircle bulge
    float distanceLeft = sqrtf(rawLeftX * rawLeftX + rawLeftY * rawLeftY) * circleRadius;
    if (distanceLeft > circleRadius) distanceLeft = circleRadius;

    if (distanceLeft > 5.0f)
    {
        float startAngleLeft = atan2f(-rawLeftY, rawLeftX) * (180.0f / 3.14159265f);
        float endAngleLeft = startAngleLeft - leftOffsetDeg;
        float minAngleLeft = fminf(startAngleLeft, endAngleLeft);
        float maxAngleLeft = fmaxf(startAngleLeft, endAngleLeft);
        DrawRing(Vector2{ leftCircleCentreX, leftCircleCentreY }, distanceLeft - 1.5f, distanceLeft + 1.5f, minAngleLeft, maxAngleLeft, 32, WHITE);
    }

    // hide dots during calibration if this stick is active, otherwise, draw them
    bool hideLeft = (currentState == CALIBRATING && calibratingLeft);
    if (!hideLeft) 
    {
        DrawCircle((int)(leftCircleCentreX + drawableRawLeftX * circleRadius), (int)(leftCircleCentreY - drawableRawLeftY * circleRadius), 14, RED);
        DrawCircle((int)(leftCircleCentreX + drawableAdjLeftX * circleRadius), (int)(leftCircleCentreY - drawableAdjLeftY * circleRadius), 14, GREEN);
    }
    else if (!needsCentre) 
    {
        // draw the calib target quadrant
        float sectorStartAngle = 0, sectorEndAngle = 0;
        if (strstr(currentInstruction, "UP")) { sectorStartAngle = 225; sectorEndAngle = 315; }
        else if (strstr(currentInstruction, "DOWN")) { sectorStartAngle = 45; sectorEndAngle = 135; }
        else if (strstr(currentInstruction, "LEFT")) { sectorStartAngle = 135; sectorEndAngle = 225; }
        else if (strstr(currentInstruction, "RIGHT")) { sectorStartAngle = 315; sectorEndAngle = 405; }
        DrawCircleSector(Vector2{ leftCircleCentreX, leftCircleCentreY }, circleRadius, sectorStartAngle, sectorEndAngle, 32, Fade(GREEN, 0.4f));
    }

    DrawText("LEFT STICK", (int)(leftCircleCentreX - 60), (int)(leftCircleCentreY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawLeftX), (int)(leftCircleCentreX - circleRadius), (int)(leftCircleCentreY + circleRadius + 20), 20, RED);
    DrawText(TextFormat("RAW Y: %0.4f", rawLeftY), (int)(leftCircleCentreX - circleRadius), (int)(leftCircleCentreY + circleRadius + 50), 20, RED);
    DrawText(TextFormat("ADJ X: %0.4f", adjLeftX), (int)(leftCircleCentreX + 20), (int)(leftCircleCentreY + circleRadius + 20), 20, GREEN);
    DrawText(TextFormat("ADJ Y: %0.4f", adjLeftY), (int)(leftCircleCentreX + 20), (int)(leftCircleCentreY + circleRadius + 50), 20, GREEN);

    // --- RIGHT STICK ---
    float rightBoxX = (screenWidth / 2.0f) + 20.0f;
    float rightBoxY = 40.0f;
    float rightBoxWidth = (screenWidth / 2.0f) - 60.0f;
    float rightBoxHeight = screenHeight - 80.0f;
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

    bool hideRight = (currentState == CALIBRATING && !calibratingLeft);
    if (!hideRight) 
    {
        DrawCircle((int)(rightCircleCentreX + drawableRawRightX * circleRadius), (int)(rightCircleCentreY - drawableRawRightY * circleRadius), 14, BLUE);
        DrawCircle((int)(rightCircleCentreX + drawableAdjRightX * circleRadius), (int)(rightCircleCentreY - drawableAdjRightY * circleRadius), 14, GREEN);
    }
    else if (!needsCentre) 
    {
        float sectorStartAngle = 0, sectorEndAngle = 0;
        if (strstr(currentInstruction, "UP")) { sectorStartAngle = 225; sectorEndAngle = 315; }
        else if (strstr(currentInstruction, "DOWN")) { sectorStartAngle = 45; sectorEndAngle = 135; }
        else if (strstr(currentInstruction, "LEFT")) { sectorStartAngle = 135; sectorEndAngle = 225; }
        else if (strstr(currentInstruction, "RIGHT")) { sectorStartAngle = 315; sectorEndAngle = 405; }
        DrawCircleSector(Vector2{ rightCircleCentreX, rightCircleCentreY }, circleRadius, sectorStartAngle, sectorEndAngle, 32, Fade(GREEN, 0.4f));
    }

    DrawText("RIGHT STICK", (int)(rightCircleCentreX - 60), (int)(rightCircleCentreY - circleRadius - 40), 20, LIGHTGRAY);
    DrawText(TextFormat("RAW X: %0.4f", rawRightX), (int)(rightCircleCentreX - circleRadius), (int)(rightCircleCentreY + circleRadius + 20), 20, BLUE);
    DrawText(TextFormat("RAW Y: %0.4f", rawRightY), (int)(rightCircleCentreX - circleRadius), (int)(rightCircleCentreY + circleRadius + 50), 20, BLUE);
    DrawText(TextFormat("ADJ X: %0.4f", adjRightX), (int)(rightCircleCentreX + 20), (int)(rightCircleCentreY + circleRadius + 20), 20, GREEN);
    DrawText(TextFormat("ADJ Y: %0.4f", adjRightY), (int)(rightCircleCentreX + 20), (int)(rightCircleCentreY + circleRadius + 50), 20, GREEN);

    // --- CROSSHAIRS --- 
    DrawLine((int)(leftCircleCentreX - circleRadius), (int)leftCircleCentreY, (int)(leftCircleCentreX + circleRadius), (int)leftCircleCentreY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)leftCircleCentreX, (int)(leftCircleCentreY - circleRadius), (int)leftCircleCentreX, (int)(leftCircleCentreY + circleRadius), Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)(rightCircleCentreX - circleRadius), (int)rightCircleCentreY, (int)(rightCircleCentreX + circleRadius), (int)rightCircleCentreY, Fade(LIGHTGRAY, 0.3f));
    DrawLine((int)rightCircleCentreX, (int)(rightCircleCentreY - circleRadius), (int)rightCircleCentreX, (int)(rightCircleCentreY + circleRadius), Fade(LIGHTGRAY, 0.3f));

    // --- CAPTURED CALIB DOTS ---
    float targetCentreX = calibratingLeft ? leftCircleCentreX : rightCircleCentreX;
    float targetCentreY = calibratingLeft ? leftCircleCentreY : rightCircleCentreY;
    for (const auto& dot : calibrationDots)
    {
        float dotX = dot.x;
        float dotY = dot.y;
        ClampToCircle(dotX, dotY);
        DrawCircle((int)(targetCentreX + dotX * circleRadius), (int)(targetCentreY - dotY * circleRadius), 6, Fade(WHITE, 0.4f));
    }

    // -- LEFT UI CONTROLS ---
    GuiToggle(Rectangle{ leftBoxX + 20, leftBoxY + 20, 96, 24 }, "Enable", &leftEnabled);
    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - 116, leftBoxY + 20, 96, 24 }, "Calibrate"))
    {
        calibrateLeftClicked = true;
        ClearCalibrationDots();
    }

    bool prevLeftManual = leftManualMode;
    GuiCheckBox(Rectangle{ leftBoxX + 20, leftBoxY + leftBoxHeight - 40, 24, 24 }, "Manual Mode", &leftManualMode);
    if (leftManualMode && !prevLeftManual) ClearCalibrationDots();

    if (!leftManualMode) GuiDisable();
    GuiSlider(Rectangle{ leftBoxX + 150, leftBoxY + leftBoxHeight - 40, leftBoxWidth - 280, 24 }, NULL, TextFormat("%.2f", leftOffsetDeg), &leftOffsetDeg, -180.0f, 180.0f);
    leftOffsetDeg = roundf(leftOffsetDeg * 10.0f) / 10.0f; 

    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - 70, leftBoxY + leftBoxHeight - 40, 50, 24 }, "Reset")) 
    {
        leftOffsetDeg = 0.0f;
    }
    GuiEnable();

    // -- RIGHT UI CONTROLS ---
    GuiToggle(Rectangle{ rightBoxX + 20, rightBoxY + 20, 96, 24 }, "Enable", &rightEnabled);
    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - 116, rightBoxY + 20, 96, 24 }, "Calibrate"))
    {
        calibrateRightClicked = true;
        ClearCalibrationDots();
    }

    bool prevRightManual = rightManualMode;
    GuiCheckBox(Rectangle{ rightBoxX + 20, rightBoxY + rightBoxHeight - 40, 24, 24 }, "Manual Mode", &rightManualMode);
    if (rightManualMode && !prevRightManual) ClearCalibrationDots();

    if (!rightManualMode) GuiDisable();
    GuiSlider(Rectangle{ rightBoxX + 150, rightBoxY + rightBoxHeight - 40, rightBoxWidth - 280, 24 }, NULL, TextFormat("%.2f", rightOffsetDeg), &rightOffsetDeg, -180.0f, 180.0f);
    rightOffsetDeg = roundf(rightOffsetDeg * 10.0f) / 10.0f; 

    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - 70, rightBoxY + rightBoxHeight - 40, 50, 24 }, "Reset")) 
    {
        rightOffsetDeg = 0.0f;
    }
    GuiEnable();

    // --- TOP STATUS OVERLAY ---
    if (currentState == CALIBRATING)
    {
        const char* title = "CALIBRATION IN PROGRESS";
        int titleWidth = MeasureText(title, 30);
        DrawText(title, (int)(screenWidth / 2.0f - titleWidth / 2.0f), 20, 30, YELLOW);

        if (needsCentre) 
        {
            const char* msg = "RETURN TO CENT";
            int messageWidth = MeasureText(msg, 35);
            DrawText(msg, (int)(screenWidth / 2.0f - messageWidth / 2.0f), 60, 35, ORANGE);
        }
    }

    EndDrawing();
}

void StopVisualiser() {
    CloseWindow();
}