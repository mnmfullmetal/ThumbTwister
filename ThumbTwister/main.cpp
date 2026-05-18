#include <windows.h>
#include <GameInput.h>
#include <ViGEmClient.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm> 
#include <random>    

#ifndef RAYLIB_VECTOR2_DEFINITION
#define RAYLIB_VECTOR2_DEFINITION
typedef struct Vector2 {
    float x;
    float y;
} Vector2;
#endif

#pragma comment(lib, "GameInput.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winmm.lib")

using namespace GameInput::v3;

void StartVisualiser();
bool VisualiserShouldClose();
void StopVisualiser();
void DrawControllerState(float r_lx, float r_ly, float s_lx, float s_ly, float r_rx, float r_ry, float s_rx, float s_ry);
float GetLeftOffset();
float GetRightOffset();
void SetLeftOffset(float val);
void SetRightOffset(float val);
bool CheckCalibrateLeft();
bool CheckCalibrateRight();

void ApplyRotation(float raw_x, float raw_y, float offsetDeg, float& out_x, float& out_y) 
{
    if (raw_x == 0.0f && raw_y == 0.0f) 
    { 
        out_x = 0.0f; 
        out_y = 0.0f; 
        return; 
    }
    float offsetRad = offsetDeg * (3.14159265f / 180.0f);
    out_x = (raw_x * std::cos(offsetRad)) - (raw_y * std::sin(offsetRad));
    out_y = (raw_x * std::sin(offsetRad)) + (raw_y * std::cos(offsetRad));
    if (out_x > 1.0f) out_x = 1.0f; if (out_x < -1.0f) out_x = -1.0f;
    if (out_y > 1.0f) out_y = 1.0f; if (out_y < -1.0f) out_y = -1.0f;
}

enum CalibStep { WAITING_FOR_PUSH, WAITING_FOR_CENTRE };

struct CalibrationManager
{
    bool active = false;
    bool isLeftStick = true;
    CalibStep step = WAITING_FOR_CENTRE;
    std::vector<Vector2> capturedPoints;

    void Start(bool left) 
    {
        active = true;
        isLeftStick = left;
        capturedPoints.clear();
        std::cout << "Calibration Started\n";
    }

    void Update(float x, float y) 
    {
        if (!active) return;
    }
};

int main()
{
    PVIGEM_CLIENT client = vigem_alloc();
    PVIGEM_TARGET vPad = nullptr;
    if (client && VIGEM_SUCCESS(vigem_connect(client)))
    {
        vPad = vigem_target_x360_alloc();
        vigem_target_add(client, vPad);
    }

    IGameInput* gameInput = nullptr;
    if (FAILED(GameInputCreate(&gameInput))) return -1;
    gameInput->SetFocusPolicy(GameInputEnableBackgroundInput);

    CalibrationManager calib;
    StartVisualiser();

    while (!VisualiserShouldClose())
    {
        IGameInputReading* reading = nullptr;
        GameInputGamepadState state = {};

        if (CheckCalibrateLeft()) calib.Start(true);
        if (CheckCalibrateRight()) calib.Start(false);

        if (SUCCEEDED(gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &reading)))
        {
            reading->GetGamepadState(&state);

            float raw_lx = state.leftThumbstickX;
            float raw_ly = state.leftThumbstickY;
            float raw_rx = state.rightThumbstickX;
            float raw_ry = state.rightThumbstickY;

            if (calib.active) 
            {
                if (calib.isLeftStick) calib.Update(raw_lx, raw_ly);
                else calib.Update(raw_rx, raw_ry);
            }

            float leftOffset = GetLeftOffset();
            float rightOffset = GetRightOffset();
            float adj_lx, adj_ly, adj_rx, adj_ry;

            ApplyRotation(raw_lx, raw_ly, leftOffset, adj_lx, adj_ly);
            ApplyRotation(raw_rx, raw_ry, rightOffset, adj_rx, adj_ry);

            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);
            report.sThumbLX = (SHORT)(state.leftThumbstickX * 32767.0f);
            report.sThumbLY = (SHORT)(state.leftThumbstickY * 32767.0f);
            report.sThumbRX = (SHORT)(state.rightThumbstickX * 32767.0f);
            report.sThumbRY = (SHORT)(state.rightThumbstickY * 32767.0f);

            if (vPad) vigem_target_x360_update(client, vPad, report);
            reading->Release();

            DrawControllerState(raw_lx, raw_ly, adj_lx, adj_ly, raw_rx, raw_ry, adj_rx, adj_ry);
        }
    }

    StopVisualiser();
    return 0;
}