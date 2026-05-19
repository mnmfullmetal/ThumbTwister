#include <windows.h>
#include <GameInput.h>
#include <ViGEmClient.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm> 
#include <random>
#include <shellapi.h>

#include "Calibration.h"
#include "Visualiser.h"

// -- SYSTEM TRAY CONSTANTS --
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

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
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

using namespace GameInput::v3;

static bool isRunning = true;

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


// -- CALLBACK THAT LISTENS FOR RIGHT CLICK ON THE TRAY ICON -- 
LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TRAYICON)
    {
        // if user right clicks the tray icon
        if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit ThumbTwister"));

            // prevent the menu from getting stuck on screen
            SetForegroundWindow(hwnd);

            // show popup menu
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
    }
    else if (msg == WM_COMMAND)
    {
		// if the user clicks exit thumbtwister from the tray menu
        if (LOWORD(wParam) == ID_TRAY_EXIT)
        {
            isRunning = false;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main()
{
    // -- GAMEINPUT SETUP --
    IGameInput* gameInput = nullptr;
    IGameInputDevice* physicalDevice = nullptr;
    if (FAILED(GameInputCreate(&gameInput))) return -1;
    gameInput->SetFocusPolicy(GameInputEnableBackgroundInput);

    // -- VIGEM SETUP --
    PVIGEM_CLIENT client = vigem_alloc();
    PVIGEM_TARGET vPad = nullptr;
    if (client && VIGEM_SUCCESS(vigem_connect(client)))
    {
        vPad = vigem_target_x360_alloc();
        vigem_target_add(client, vPad);
    }

    // -- SYSTEM TRAY SETUP --
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("ThumbTwisterTrayClass");
    RegisterClass(&wc);

    // message only window to handle tray icon clicks
    HWND hwndTray = CreateWindow(TEXT("ThumbTwisterTrayClass"), TEXT("ThumbTwisterTray"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);

    // initialise the data structure required by Windows to create the tray icon
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwndTray;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, 128, L"ThumbTwister Background Service");


    // -- INITIALISE ENGINE --
    CalibrationManager calib;
    StartVisualiser();
    bool windowOpen = true;

    while (isRunning)
    {
        // 1. Process Win32 Messages (Keeps the tray icon right-click menu responsive)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) isRunning = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!isRunning) break;

        IGameInputReading* reading = nullptr;
        GameInputGamepadState state = {};

        if (windowOpen)
        {
            // listen for calib button click on ui
            if (CheckCalibrateLeft()) calib.Start(true);
            if (CheckCalibrateRight()) calib.Start(false);
        }
       
        float rawLX = 0.0f, rawLY = 0.0f, rawRX = 0.0f, rawRY = 0.0f;
        float adjustedLX = 0.0f, adjustedLY = 0.0f, adjustedRX = 0.0f, adjustedRY = 0.0f;

        if (SUCCEEDED(gameInput->GetCurrentReading(GameInputKindGamepad, physicalDevice, &reading)))
        {
            // if physical device not locked, capture its pointer from the first valid reading
            if (physicalDevice == nullptr)
            {
                reading->GetDevice(&physicalDevice);
            }

            reading->GetGamepadState(&state);

            rawLX = state.leftThumbstickX;
            rawLY = state.leftThumbstickY;
            rawRX = state.rightThumbstickX;
            rawRY = state.rightThumbstickY;

            // feed the stick data to the calibration brain if its active
            if (calib.active)
            {
                if (calib.isLeftStick) calib.Update(rawLX, rawLY);
                else calib.Update(rawRX, rawRY);
                SetCalibrationDots(calib.capturedPoints, calib.isLeftStick);
            }

            // apply rotation offset based on manual slider or auto calibration
            float leftOffset = GetLeftOffset();
            float rightOffset = GetRightOffset();
        
            ApplyRotation(rawLX, rawLY, leftOffset, adjustedLX, adjustedLY);
            ApplyRotation(rawRX, rawRY, rightOffset, adjustedRX, adjustedRY);

            // determine final output based on UI toggles
            float finalLX = GetLeftEnabled() ? adjustedLX : rawLX;
            float finalLY = GetLeftEnabled() ? adjustedLY : rawLY;
            float finalRX = GetRightEnabled() ? adjustedRX : rawRX;
            float finalRY = GetRightEnabled() ? adjustedRY : rawRY;

			// clamp float conversion to prevent overflow from bad calibration or input, and convert to SHORT
            auto FloatToShort = [](float val) -> SHORT 
            {
                float scaled = val * 32767.0f;
                if (scaled > 32767.0f) return 32767;
                if (scaled < -32768.0f) return -32768;
                return (SHORT)scaled;
            };

			// initialise the XUSB report with the final thumbstick, trigger, and button values
            XUSB_REPORT report = {0};
            XUSB_REPORT_INIT(&report);
            report.sThumbLX = FloatToShort(finalLX);
            report.sThumbLY = FloatToShort(finalLY);
            report.sThumbRX = FloatToShort(finalRX);
            report.sThumbRY = FloatToShort(finalRY);
            report.bLeftTrigger = (BYTE)(state.leftTrigger * 255.0f);
            report.bRightTrigger = (BYTE)(state.rightTrigger * 255.0f);
            report.wButtons = (USHORT)state.buttons;

			// send the report to the virtual controller
            if (vPad && physicalDevice) vigem_target_x360_update(client, vPad, report);
            reading->Release();

			// -- UI STATE MANAGEMENT ---
            if (windowOpen)
            {
				// check if visualiser window was closed
                if (VisualiserShouldClose())
                {
                    windowOpen = false;
                    StopVisualiser(); 

                    // create system tray icon 
                    Shell_NotifyIcon(NIM_ADD, &nid);
                }
                else
                {
                    // draw to the screen
                    DrawControllerState(rawLX, rawLY, adjustedLX, adjustedLY, rawRX, rawRY, adjustedRX, adjustedRY);
                }
            }
            else
            {
                Sleep(1);
            }
        }
         else if (!windowOpen)
        {
            Sleep(1);
        }
    }

    // --- CLEANUP ---
    if (physicalDevice) physicalDevice->Release();
    Shell_NotifyIcon(NIM_DELETE, &nid);

    if (vPad)
    {
        vigem_target_remove(client, vPad);
        vigem_target_free(vPad);
    }
    if (client)
    {
        vigem_disconnect(client);
        vigem_free(client);
    }

    if (windowOpen) StopVisualiser();

    return 0;
}