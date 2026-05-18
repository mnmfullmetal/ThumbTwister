#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


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

void DrawControllerState(float leftX, float leftY, float rightX, float rightY)
{
	BeginDrawing();
	ClearBackground(DARKGRAY);

	DrawCircle(150 + (int)(leftX * 50), 150 - (int)(leftY * 50), 20, LIGHTGRAY);
	DrawText("Left Thumbstick", 100, 200, 10, BLUE);

	DrawCircle(350 + (int)(rightX * 50), 150 - (int)(rightY * 50), 20, LIGHTGRAY);
	DrawText("Right Thumbstick", 300, 200, 10, RED);
	EndDrawing();

}

void StopVisualiser() {
	CloseWindow();
}

bool VisualiserShouldClose()
{
	return WindowShouldClose();
}