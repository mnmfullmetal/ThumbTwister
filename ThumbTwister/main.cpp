
#include <iostream>
#include <GameInput.h>
#include <windows.h>
#include <ViGEmClient.h>

using namespace GameInput::v3;

#pragma comment(lib, "setupapi.lib")



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
	IGameInputReading* reading = nullptr;

	while (true)
	{
		if(SUCCEEDED(gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &reading)))
		{
			GameInputGamepadState state{};
			if (reading->GetGamepadState(&state))
			{
				std::cout << "Left Thumbstick: (" << state.leftThumbstickX << ", " << state.leftThumbstickY << ")\n";
				std::cout << "Right Thumbstick: (" << state.rightThumbstickX << ", " << state.rightThumbstickY << ")\n";
				XUSB_REPORT report;
				XUSB_REPORT_INIT(&report);
				report.sThumbLX = (SHORT)(state.leftThumbstickX * 32767.0f);
				report.sThumbLY = (SHORT)(state.leftThumbstickY * 32767.0f);
				report.sThumbRX = (SHORT)(state.rightThumbstickX * 32767.0f);
				report.sThumbRY = (SHORT)(state.rightThumbstickY * 32767.0f);

				if (virtualPad) vigem_target_x360_update(client, virtualPad, report);
			}
			reading->Release();

		}
		Sleep(16);
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

	return 0;
}

