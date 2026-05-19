#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include <vector> 
#include <cstring> 
#include <cmath>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// -- CUSTOM COLOURS -- 
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


bool GetLeftEnabled() { return leftEnabled; }
bool GetRightEnabled() { return rightEnabled; }

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
    float scaleFactor = fminf(screenWidth / 1200.0f, screenHeight / 800.0f);

    int baseFontSize = (int)(10 * scaleFactor);
    GuiSetStyle(DEFAULT, TEXT_SIZE, baseFontSize);

    float btnWidth = 80.0f * scaleFactor;
    float btnHeight = 20.0f * scaleFactor;
    float sliderHeight = 18.0f * scaleFactor;
    float uiPadding = 40.0f * scaleFactor;
    float botPadding = 28.0f * scaleFactor;
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
        DrawCircle((int)(leftCircleCentreX + drawableAdjLeftX * circleRadius), (int)(leftCircleCentreY - drawableAdjLeftY * circleRadius), 14, GREEN);
        DrawCircle((int)(leftCircleCentreX + drawableRawLeftX * circleRadius), (int)(leftCircleCentreY - drawableRawLeftY * circleRadius), 14, RED);

    }
    else if (!needsCentre) 
    {
        // draw the calib target quadrant
        float sectorStartAngle = 0, sectorEndAngle = 0;
        if (strstr(currentInstruction, "UP")) { sectorStartAngle = 225; sectorEndAngle = 315; }
        else if (strstr(currentInstruction, "DOWN")) { sectorStartAngle = 45; sectorEndAngle = 135; }
        else if (strstr(currentInstruction, "LEFT")) { sectorStartAngle = 135; sectorEndAngle = 225; }
        else if (strstr(currentInstruction, "RIGHT")) { sectorStartAngle = 315; sectorEndAngle = 405; }
        DrawCircleSector(Vector2{ leftCircleCentreX, leftCircleCentreY }, circleRadius, sectorStartAngle, sectorEndAngle, 32, Fade(BLUE, 0.4f));
    }

    DrawText("LEFT STICK", (int)(leftCircleCentreX - 60), (int)(leftCircleCentreY - circleRadius - 40), 20, LIGHTGRAY);
    int textPadding = 50;
    const char* textBuffer;
    int textWidth;

    // raw values text
    textBuffer = TextFormat("RAW X: %0.4f", rawLeftX);
    textWidth = MeasureText(textBuffer, 20);
    DrawText(textBuffer, (int)(leftCircleCentreX - textWidth - textPadding), (int)(leftCircleCentreY + circleRadius + 20), 20, RED);
    textBuffer = TextFormat("RAW Y: %0.4f", rawLeftY);
    textWidth = MeasureText(textBuffer, 20);
    DrawText(textBuffer, (int)(leftCircleCentreX - textWidth - textPadding), (int)(leftCircleCentreY + circleRadius + 50), 20, RED);

    // adjusted values text 
    textBuffer = TextFormat("ADJ X: %0.4f", adjLeftX);
    DrawText(textBuffer, (int)(leftCircleCentreX + textPadding), (int)(leftCircleCentreY + circleRadius + 20), 20, GREEN);
    textBuffer = TextFormat("ADJ Y: %0.4f", adjLeftY);
    DrawText(textBuffer, (int)(leftCircleCentreX + textPadding), (int)(leftCircleCentreY + circleRadius + 50), 20, GREEN);

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
        DrawCircle((int)(rightCircleCentreX + drawableAdjRightX * circleRadius), (int)(rightCircleCentreY - drawableAdjRightY * circleRadius), 14, GREEN);
        DrawCircle((int)(rightCircleCentreX + drawableRawRightX * circleRadius), (int)(rightCircleCentreY - drawableRawRightY * circleRadius), 14, RED);

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

    textBuffer = TextFormat("RAW X: %0.4f", rawRightX);
    textWidth = MeasureText(textBuffer, 20);
    DrawText(textBuffer, (int)(rightCircleCentreX - textWidth - textPadding), (int)(rightCircleCentreY + circleRadius + 20), 20, RED);
    textBuffer = TextFormat("RAW Y: %0.4f", rawRightY);
    textWidth = MeasureText(textBuffer, 20);
    DrawText(textBuffer, (int)(rightCircleCentreX - textWidth - textPadding), (int)(rightCircleCentreY + circleRadius + 50), 20, RED);

    textBuffer = TextFormat("ADJ X: %0.4f", adjRightX);
    DrawText(textBuffer, (int)(rightCircleCentreX + textPadding), (int)(rightCircleCentreY + circleRadius + 20), 20, GREEN);
    textBuffer = TextFormat("ADJ Y: %0.4f", adjRightY);
    DrawText(textBuffer, (int)(rightCircleCentreX + textPadding), (int)(rightCircleCentreY + circleRadius + 50), 20, GREEN);

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
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(RED));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, ColorToInt(MAROON));
    GuiSetStyle(TOGGLE, TEXT_COLOR_NORMAL, ColorToInt(RED));
    GuiSetStyle(TOGGLE, TEXT_COLOR_FOCUSED, ColorToInt(MAROON));
    GuiSetStyle(TOGGLE, TEXT_COLOR_PRESSED, ColorToInt(DARKGREEN));

    GuiToggle(Rectangle{ leftBoxX + 20, leftBoxY + 20, btnWidth, btnHeight }, "Enable", & leftEnabled);
    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - btnWidth - 20, leftBoxY + 20, btnWidth, btnHeight }, "Calibrate"))
    {
        calibrateLeftClicked = true;
        ClearCalibrationDots();
    }
    bool prevLeftManual = leftManualMode;
    GuiCheckBox(Rectangle{ leftBoxX + 20, leftBoxY + leftBoxHeight - botPadding + (btnHeight - 24.0f) / 2.0f, 24, 24 }, "Manual Mode", &leftManualMode);
    if (leftManualMode && !prevLeftManual) ClearCalibrationDots();

    if (!leftManualMode) GuiDisable();

    float sliderWidth = leftBoxWidth - (btnWidth * 1.5f) - (uiPadding * 4) - btnHeight;
    GuiSlider(Rectangle{ leftBoxX + btnHeight + (uiPadding * 2) + MeasureText("Manual Mode", baseFontSize) - 40.0f, leftBoxY + leftBoxHeight - botPadding, sliderWidth, sliderHeight }, NULL, TextFormat("%.2f", leftOffsetDeg), &leftOffsetDeg, -180.0f, 180.0f);
    leftOffsetDeg = roundf(leftOffsetDeg * 10.0f) / 10.0f;

    if (GuiButton(Rectangle{ leftBoxX + leftBoxWidth - btnWidth - 20, leftBoxY + leftBoxHeight - botPadding, btnWidth, btnHeight }, "Reset"))
    {
        leftOffsetDeg = 0.0f;
    }
    GuiEnable();

    // -- RIGHT UI CONTROLS ---
    GuiToggle(Rectangle{ rightBoxX + 20, rightBoxY + 20, btnWidth, btnHeight }, "Enable", &rightEnabled);
    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - btnWidth - 20, rightBoxY + 20, btnWidth, btnHeight }, "Calibrate"))
    {
        calibrateRightClicked = true;
        ClearCalibrationDots();
    }

    bool prevRightManual = rightManualMode;
    GuiCheckBox(Rectangle{ rightBoxX + 20, rightBoxY + rightBoxHeight - botPadding + (btnHeight - 24.0f) / 2.0f, 24, 24 }, "Manual Mode", &rightManualMode);;
    if (rightManualMode && !prevRightManual) ClearCalibrationDots();

    if (!rightManualMode) GuiDisable();

    GuiSlider(Rectangle{ rightBoxX + btnHeight + (uiPadding * 2) + MeasureText("Manual Mode", baseFontSize) - 40.0f, rightBoxY + rightBoxHeight - botPadding, sliderWidth, sliderHeight }, NULL, TextFormat("%.2f", rightOffsetDeg), &rightOffsetDeg, -180.0f, 180.0f);
    rightOffsetDeg = roundf(rightOffsetDeg * 10.0f) / 10.0f;

    if (GuiButton(Rectangle{ rightBoxX + rightBoxWidth - btnWidth - 20, rightBoxY + rightBoxHeight - botPadding, btnWidth, btnHeight }, "Reset"))
    {
        rightOffsetDeg = 0.0f;
    }
    GuiEnable();

    // -- SCREEN DIMMING DURING CALIB --
    if (currentState == CALIBRATING)
    {
        if (calibratingLeft)
        {
            // dim the entire right half of the screen
            DrawRectangle((int)(screenWidth / 2.0f), 0, (int)(screenWidth / 2.0f), (int)screenHeight, Fade(BLACK, 0.7f));
        }
        else
        {
            // dim the entire left half of the screen
            DrawRectangle(0, 0, (int)(screenWidth / 2.0f), (int)screenHeight, Fade(BLACK, 0.7f));
        }
    }

    // --- TOP STATUS OVERLAY ---
    if (currentState == CALIBRATING)
    {
        float activeCentreX = calibratingLeft ? leftCircleCentreX : rightCircleCentreX;

        const char* title = "CALIBRATION IN PROGRESS";
        int titleWidth = MeasureText(title, 30);
        DrawText(title, (int)(activeCentreX - titleWidth / 2.0f), 100, 30, YELLOW);

        if (needsCentre)
        {
            const char* msg = "RETURN TO CENTRE";
            int messageWidth = MeasureText(msg, 35);
            DrawText(msg, (int)(activeCentreX - messageWidth / 2.0f), 150, 35, ORANGE);
        }
    }

    EndDrawing();
}

void StopVisualiser() {
    CloseWindow();
}