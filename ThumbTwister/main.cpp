#include <windows.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
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
#define ID_TRAY_OPEN 1002

#pragma comment(lib, "SDL3.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

static bool isRunning = true;
static bool windowOpen = true;
static NOTIFYICONDATA nid = { 0 };

// -- WINDOW HOOK VARS --
static WNDPROC originalRaylibProc = nullptr;
static bool closeRequested = false;

// Intercepts the Win32 Close command before Raylib can destroy the window
LRESULT CALLBACK CustomRaylibProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE)
    {
        closeRequested = true;
        return 0; // Consume the command
    }
    return CallWindowProc(originalRaylibProc, hwnd, msg, wParam, lParam);
}

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

float NormalizeSDLAxis(Sint16 val)
{
    if (val < 0) return (float)val / 32768.0f;
    return (float)val / 32767.0f;
}

USHORT TranslateButtons(SDL_Gamepad* pad)
{
    USHORT xusbButtons = 0;

    // D-Pad
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP))       xusbButtons |= XUSB_GAMEPAD_DPAD_UP;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))     xusbButtons |= XUSB_GAMEPAD_DPAD_DOWN;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))     xusbButtons |= XUSB_GAMEPAD_DPAD_LEFT;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))    xusbButtons |= XUSB_GAMEPAD_DPAD_RIGHT;

    // Start / Select 
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START))         xusbButtons |= XUSB_GAMEPAD_START;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK))          xusbButtons |= XUSB_GAMEPAD_BACK;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_GUIDE))         xusbButtons |= XUSB_GAMEPAD_GUIDE;

    // Stick Clicks
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_STICK))    xusbButtons |= XUSB_GAMEPAD_LEFT_THUMB;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK))   xusbButtons |= XUSB_GAMEPAD_RIGHT_THUMB;

    // Bumpers
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))    xusbButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))   xusbButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;

    // Face Buttons
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH))             xusbButtons |= XUSB_GAMEPAD_A;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST))              xusbButtons |= XUSB_GAMEPAD_B;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST))              xusbButtons |= XUSB_GAMEPAD_X;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH))             xusbButtons |= XUSB_GAMEPAD_Y;

    return xusbButtons;
}

std::string GetHidHidePath()
{
    std::string path = "";
    HKEY hKey;
    char value[MAX_PATH];
    DWORD bufferSize = sizeof(value);

    // try to find path to HidHideCLI.exe in Windows Registry 
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Nefarius Software Solutions\\HidHide", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "InstallDir", NULL, NULL, (LPBYTE)value, &bufferSize) == ERROR_SUCCESS)
        {
            path = std::string(value) + "\\x64\\HidHideCLI.exe";
        }
        RegCloseKey(hKey);
    }

    // if registry lookup failed, look in the same folder as the app
    if (path.empty())
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string exePath(buffer);
        path = exePath.substr(0, exePath.find_last_of("\\/")) + "\\HidHideCLI.exe";
    }

    return path;
}

void SetHidHideState(bool enabled)
{
    std::string cliPath = GetHidHidePath();
    DWORD dwAttrib = GetFileAttributesA(cliPath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }

    SHELLEXECUTEINFOA sei = { 0 };
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = cliPath.c_str();
    sei.lpParameters = enabled ? "--cloak-on" : "--cloak-off";
    sei.nShow = SW_HIDE;

    if (ShellExecuteExA(&sei))
    {
        // wait for the process to finish before exit
        if (sei.hProcess != NULL)
        {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
    }
}


// -- CALLBACK THAT LISTENS FOR RIGHT CLICK ON THE TRAY ICON -- 
LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TRAYICON)
    {
        // if user double left clicks the tray icon 
        if (lParam == WM_LBUTTONDBLCLK)
        {
            if (!windowOpen)
            {
                windowOpen = true;
                closeRequested = false;
                ShowVisualiser();
                Shell_NotifyIcon(NIM_DELETE, &nid);
            }
        }

        // if user right clicks the tray icon
        else if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();

            AppendMenu(hMenu, MF_STRING, ID_TRAY_OPEN, TEXT("Open Visualiser"));
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
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

        // if the user clicks open from the tray menu
        else if (LOWORD(wParam) == ID_TRAY_OPEN)
        {
            if (!windowOpen)
            {
                windowOpen = true;
                StartVisualiser();
                Shell_NotifyIcon(NIM_DELETE, &nid);
            }
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


SDL_Gamepad* AcquirePhysicalController(SDL_JoystickID ignoredVirtualId)
{
    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);

    if (gamepads)
    {
        for (int i = 0; i < count; ++i)
        {
            if (gamepads[i] != ignoredVirtualId)
            {
                SDL_Gamepad* pad = SDL_OpenGamepad(gamepads[i]);
                if (pad)
                {
                    SDL_free(gamepads);
                    return pad;
                }
            }
        }
        SDL_free(gamepads);
    }
    return nullptr;
}

int main()
{
	// enable hidhide device hiding to conceal the physical controller from windows
    SetHidHideState(true);

    // -- SDL3 INPUT SETUP --
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "0");
    SDL_SetHint("SDL_JOYSTICK_GAMEINPUT", "0");

    if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK))
    {
        return -1;
    }

    // snapshot existing controller IDs before spawning the vigem target
    SDL_PumpEvents();
    std::vector<SDL_JoystickID> preVigemIds;
    int preCount = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&preCount);
    if (joysticks)
    {
        for (int i = 0; i < preCount; ++i) preVigemIds.push_back(joysticks[i]);
        SDL_free(joysticks);
    }

    
    // -- VIGEM SETUP --
    PVIGEM_CLIENT client = vigem_alloc();
    PVIGEM_TARGET vPad = nullptr;
    SDL_JoystickID virtualControllerId = -1;
    if (client && VIGEM_SUCCESS(vigem_connect(client)))
    {
        vPad = vigem_target_x360_alloc();
        if (vPad && VIGEM_SUCCESS(vigem_target_add(client, vPad)))
        {
            // detect/ isolate the spawned virtual controllers SDL_JoystickID
            DWORD startWait = GetTickCount();
            while (GetTickCount() - startWait < 500 && virtualControllerId == 0)
            {
                SDL_PumpEvents();
                int currentCount = 0;
                SDL_JoystickID* currentJoysticks = SDL_GetJoysticks(&currentCount);
                if (currentJoysticks)
                {
                    for (int i = 0; i < currentCount; ++i)
                    {
                        if (std::find(preVigemIds.begin(), preVigemIds.end(), currentJoysticks[i]) == preVigemIds.end())
                        {
                            virtualControllerId = currentJoysticks[i];
                            break;
                        }
                    }
                    SDL_free(currentJoysticks);
                }
                if (virtualControllerId == 0) Sleep(10);
            }
        }
    }
    SDL_Gamepad* physicalDevice = AcquirePhysicalController(virtualControllerId);
     
    // -- SYSTEM TRAY SETUP --
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("ThumbTwisterTrayClass");
    RegisterClass(&wc);

    HWND hwndTray = CreateWindow(TEXT("ThumbTwisterTrayClass"), TEXT("ThumbTwisterTray"), 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);

    // initialise the data structure required by Windows to create the tray icon
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

    // hook the window so the hide state can be controlled independently
    HWND raylibWnd = FindWindowA(NULL, "ThumbTwister Visualizer");
    if (raylibWnd)
    {
        originalRaylibProc = (WNDPROC)SetWindowLongPtr(raylibWnd, GWLP_WNDPROC, (LONG_PTR)CustomRaylibProc);
    }

    while (isRunning)
    {
        // process Win32 Messages for the system tray icon and visualiser window
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) isRunning = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!isRunning) break;

        SDL_PumpEvents();

        // Handle hot-unplugging and reconnection of the physical controller
        if (physicalDevice && !SDL_GamepadConnected(physicalDevice))
        {
            SDL_CloseGamepad(physicalDevice);
            physicalDevice = nullptr;
        }
        if (!physicalDevice)
        {
            physicalDevice = AcquirePhysicalController(virtualControllerId);
        }

        if (windowOpen)
        {
            // listen for calib button click on ui
            if (CheckCalibrateLeft()) calib.Start(true);
            if (CheckCalibrateRight()) calib.Start(false);
        }

        float rawLX = 0.0f, rawLY = 0.0f, rawRX = 0.0f, rawRY = 0.0f;
        float adjustedLX = 0.0f, adjustedLY = 0.0f, adjustedRX = 0.0f, adjustedRY = 0.0f;

        if (physicalDevice)
        {
            // SDL3 Y-axes map to positive-down, requiring inversion for our positive-up logic
            rawLX = NormalizeSDLAxis(SDL_GetGamepadAxis(physicalDevice, SDL_GAMEPAD_AXIS_LEFTX));
            rawLY = -NormalizeSDLAxis(SDL_GetGamepadAxis(physicalDevice, SDL_GAMEPAD_AXIS_LEFTY));
            rawRX = NormalizeSDLAxis(SDL_GetGamepadAxis(physicalDevice, SDL_GAMEPAD_AXIS_RIGHTX));
            rawRY = -NormalizeSDLAxis(SDL_GetGamepadAxis(physicalDevice, SDL_GAMEPAD_AXIS_RIGHTY));

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

            Sint16 rawTrigL = SDL_GetGamepadAxis(physicalDevice, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
            Sint16 rawTrigR = SDL_GetGamepadAxis(physicalDevice, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

            // initialise the XUSB report with the final thumbstick, trigger, and button values
            XUSB_REPORT report = { 0 };
            XUSB_REPORT_INIT(&report);
            report.sThumbLX = FloatToShort(finalLX);
            report.sThumbLY = FloatToShort(finalLY);
            report.sThumbRX = FloatToShort(finalRX);
            report.sThumbRY = FloatToShort(finalRY);
            report.bLeftTrigger = (BYTE)((std::max<int>(0, rawTrigL) * 255) / 32767);
            report.bRightTrigger = (BYTE)((std::max<int>(0, rawTrigR) * 255) / 32767);
            report.wButtons = TranslateButtons(physicalDevice);

            // send the report to the virtual controller
            if (vPad) vigem_target_x360_update(client, vPad, report);
        }

        // -- UI STATE MANAGEMENT ---
        if (windowOpen)
        {
            // check if visualiser window was closed
            if (closeRequested)
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

    // --- CLEANUP ---
    if (physicalDevice) SDL_CloseGamepad(physicalDevice);
    SDL_Quit();

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

    SetHidHideState(false);

    return 0;
}