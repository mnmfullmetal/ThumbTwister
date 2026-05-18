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
float GetLeftOffset();
float GetRightOffset();
void SetLeftOffset(float val);
void SetRightOffset(float val);
void SetRotationOffset(float val);
void SetCalibrationUI(const char* text, bool requireCentre);
void StopCalibrationUI();
bool CheckCalibrateLeft();
bool CheckCalibrateRight();
void DrawControllerState(float r_lx, float r_ly, float s_lx, float s_ly, float r_rx, float r_ry, float s_rx, float s_ry);
void StopVisualiser();
void SetCalibrationDots(std::vector<Vector2> points, bool isLeft);
void ClearCalibrationDots();

void ApplyRotation(float raw_x, float raw_y, float offsetDeg, float& out_x, float& out_y)
{
    if (raw_x == 0.0f && raw_y == 0.0f) 
    {
        out_x = 0.0f; out_y = 0.0f;
        return;
    }

    float offsetRad = offsetDeg * (3.14159265f / 180.0f);

    float cosTheta = std::cos(offsetRad);
    float sinTheta = std::sin(offsetRad);

    out_x = (raw_x * cosTheta) - (raw_y * sinTheta);
    out_y = (raw_x * sinTheta) + (raw_y * cosTheta);

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
    std::vector<float> targetAngles;
    int currentIdx = 0;
    float sumCosError = 0.0f;
    float sumSinError = 0.0f;

    bool IsValidSequence() {
        for (size_t i = 1; i < targetAngles.size(); i++)
        {
            if (targetAngles[i] == targetAngles[i - 1]) return false;
        }

        for (size_t i = 2; i < targetAngles.size(); i++)
        {
            float d1 = targetAngles[i - 1] - targetAngles[i - 2];
            while (d1 > 180.0f) d1 -= 360.0f;
            while (d1 < -180.0f) d1 += 360.0f;

            float d2 = targetAngles[i] - targetAngles[i - 1];
            while (d2 > 180.0f) d2 -= 360.0f;
            while (d2 < -180.0f) d2 += 360.0f;

            if (std::abs(d1) == 90.0f && d1 == d2)
            {
                return false;
            }
        }
        return true;
    }

    void Start(bool left)
    {
        capturedPoints.clear();

        targetAngles.clear();
        for (int i = 0; i < 5; i++)
        {
            targetAngles.push_back(90.0f);
            targetAngles.push_back(270.0f);
            targetAngles.push_back(180.0f);
            targetAngles.push_back(0.0f);
        }

        active = true;
        isLeftStick = left;
        currentIdx = 0;
        step = WAITING_FOR_CENTRE;

        sumCosError = 0.0f;
        sumSinError = 0.0f;

        std::random_device rd;
        std::mt19937 g(rd());
        do {
            std::shuffle(targetAngles.begin(), targetAngles.end(), g);
        } while (!IsValidSequence());
    }

    void Update(float x, float y)
    {
        if (!active) return;

        float mag = std::sqrtf(x * x + y * y);

        if (step == WAITING_FOR_CENTRE)
        {
            SetCalibrationUI("RETURN TO CENTER", true);
            if (mag < 0.2f) step = WAITING_FOR_PUSH;
        }
        else if (step == WAITING_FOR_PUSH)
        {
            float targetDeg = targetAngles[currentIdx];
            float target_x = 0.0f;
            float target_y = 0.0f;

            if (targetDeg == 90.0f) { SetCalibrationUI("PUSH UP", false);    target_x = 0.0f;  target_y = 1.0f; }
            else if (targetDeg == 270.0f) { SetCalibrationUI("PUSH DOWN", false);  target_x = 0.0f;  target_y = -1.0f; }
            else if (targetDeg == 180.0f) { SetCalibrationUI("PUSH LEFT", false);  target_x = -1.0f; target_y = 0.0f; }
            else if (targetDeg == 0.0f) { SetCalibrationUI("PUSH RIGHT", false); target_x = 1.0f;  target_y = 0.0f; }

            if (mag > 0.85f)
            {
                capturedPoints.push_back(Vector2{ x, y });
                std::cout << "Captured Point [" << currentIdx + 1 << "/20]: X = " << x << ", Y = " << y << "\n";

                float norm_x = x / mag;
                float norm_y = y / mag;

                float cosError = (norm_x * target_x) + (norm_y * target_y);
                float sinError = (norm_x * target_y) - (norm_y * target_x);

                sumCosError += cosError;
                sumSinError += sinError;

                currentIdx++;

                if (currentIdx >= targetAngles.size())
                {
                    Finish();
                }
                else
                {
                    step = WAITING_FOR_CENTRE;
                }
            }
        }
    }

    void Finish()
    {
        float avgCos = sumCosError / (float)targetAngles.size();
        float avgSin = sumSinError / (float)targetAngles.size();

        float averageOffset = std::atan2f(avgSin, avgCos) * (180.0f / 3.14159265f);

        if (isLeftStick)
        {
            SetLeftOffset(-averageOffset);
        }
        else
        {
            SetRightOffset(-averageOffset);
        }

        active = false;
        StopCalibrationUI();
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
                SetCalibrationDots(calib.capturedPoints, calib.isLeftStick);
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