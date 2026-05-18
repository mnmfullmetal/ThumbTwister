#include <iostream>
#include <cmath>
#include <GameInput.h>
#include <windows.h>
#include <ViGEmClient.h>

#pragma comment(lib, "GameInput.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winmm.lib")

using namespace GameInput::v3;

void StartVisualiser();
void StopVisualiser();
bool VisualiserShouldClose();
void DrawControllerState(float r_lx, float r_ly, float s_lx, float s_ly, float r_rx, float r_ry, float a_rx, float a_ry);

void ApplyRotation(float raw_x, float raw_y, float offsetDeg, float& out_x, float& out_y)
{
    if (raw_x == 0.0f && raw_y == 0.0f) {
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

int main()
{
    PVIGEM_CLIENT client = vigem_alloc();
    PVIGEM_TARGET virtualPad = nullptr;
    if (client && VIGEM_SUCCESS(vigem_connect(client)))
    {
        virtualPad = vigem_target_x360_alloc();
        vigem_target_add(client, virtualPad);
    }
    else
    {
        std::cerr << "Failed to initialise ViGEmBus" << std::endl;
        return -1;
    }

    IGameInput* gameInput = nullptr;
    if (FAILED(GameInputCreate(&gameInput)))
    {
        std::cerr << "Failed to create GameInput instance" << std::endl;
        return -1;
    }
    gameInput->SetFocusPolicy(GameInputEnableBackgroundInput);

    std::cout << "Listening for input...\n\n";

    StartVisualiser();
    while (!VisualiserShouldClose())
    {
        IGameInputReading* reading = nullptr;
        GameInputGamepadState state{};

        if (SUCCEEDED(gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &reading)))
        {
            reading->GetGamepadState(&state);

            float raw_lx = state.leftThumbstickX;
            float raw_ly = state.leftThumbstickY;
            float raw_rx = state.rightThumbstickX;
            float raw_ry = state.rightThumbstickY;

            float adj_lx, adj_ly, adj_rx, adj_ry;

            ApplyRotation(raw_lx, raw_ly, 15.0f, adj_lx, adj_ly);
            ApplyRotation(raw_rx, raw_ry, 15.0f, adj_rx, adj_ry);

            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);
            report.sThumbLX = (SHORT)(state.leftThumbstickX * 32767.0f);
            report.sThumbLY = (SHORT)(state.leftThumbstickY * 32767.0f);
            report.sThumbRX = (SHORT)(state.rightThumbstickX * 32767.0f);
            report.sThumbRY = (SHORT)(state.rightThumbstickY * 32767.0f);

            if (virtualPad) vigem_target_x360_update(client, virtualPad, report);
            reading->Release();

            DrawControllerState(raw_lx, raw_ly, adj_lx, adj_ly, raw_rx, raw_ry, adj_rx, adj_ry);
        }
    }

    if (virtualPad) 
    { 
        vigem_target_remove(client, virtualPad); vigem_target_free(virtualPad); 
    }
    if (client) 
    { 
        vigem_disconnect(client); vigem_free(client); 
    }
    if (gameInput) gameInput->Release();

    StopVisualiser();
    return 0;
}