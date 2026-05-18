
#include <iostream>
#include <GameInput.h>
#include <windows.h>

using namespace GameInput::v3;



int main()
{
	IGameInput* gameInput = nullptr;
	if (FAILED(GameInputCreate(&gameInput)))
	{
		std::cerr << "Failed to create GameInput instance." << std::endl;
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
			}
			reading->Release();
		}
		Sleep(100);
	}

	return 0;
}

