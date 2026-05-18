
#include <iostream>
#include <GameInput.h>
#include <windows.h>
#include <ViGEmClient.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winmm.lib")

using namespace GameInput::v3;

void StartVisualiser();
void DrawControllerState(float leftX, float leftY, float rightX, float rightY);
bool VisualiserShouldClose();
void StopVisualiser();


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

	std::cout << "Listening for input)\n\n";

	StartVisualiser();
	while (!VisualiserShouldClose())
	{

		IGameInputReading* reading = nullptr;
		GameInputGamepadState state{};

		if (SUCCEEDED(gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &reading)))
		{
			reading->GetGamepadState(&state);

			XUSB_REPORT report;
			XUSB_REPORT_INIT(&report);
			report.sThumbLX = (SHORT)(state.leftThumbstickX * 32767.0f);
			report.sThumbLY = (SHORT)(state.leftThumbstickY * 32767.0f);
			report.sThumbRX = (SHORT)(state.rightThumbstickX * 32767.0f);
			report.sThumbRY = (SHORT)(state.rightThumbstickY * 32767.0f);

			if (virtualPad) vigem_target_x360_update(client, virtualPad, report);
			reading->Release();

			DrawControllerState(state.leftThumbstickX, state.leftThumbstickY, state.rightThumbstickX, state.rightThumbstickY);
		}
	}

	if (virtualPad)
	{
		vigem_target_remove(client, virtualPad);
		vigem_target_free(virtualPad);
	}
	if (client)
	{
		vigem_disconnect(client);
		vigem_free(client);
	}
	if (gameInput)
	{
		gameInput->Release();
	}

	StopVisualiser();
	return 0;
}

