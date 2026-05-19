#pragma once
#include <vector>
#include "Calibration.h" 

bool GetLeftEnabled();
bool GetRightEnabled();
void StartVisualiser();
bool VisualiserShouldClose();
float GetLeftOffset();
float GetRightOffset();
void SetLeftOffset(float val);
void SetRightOffset(float val);
void SetCalibrationUI(const char* text, bool requireCentre);
void StopCalibrationUI();
bool CheckCalibrateLeft();
bool CheckCalibrateRight();
void DrawControllerState(float r_lx, float r_ly, float s_lx, float s_ly, float r_rx, float r_ry, float s_rx, float s_ry);
void StopVisualiser();
void SetCalibrationDots(std::vector<CalibPoint> points, bool isLeft);
void ClearCalibrationDots();