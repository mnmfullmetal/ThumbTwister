#pragma once
#include <vector>

struct CalibPoint {
    float x;
    float y;
};

enum CalibStep { WAITING_FOR_PUSH, WAITING_FOR_CENTRE };

struct CalibrationManager
{
    bool active = false;
    bool isLeftStick = true;
    CalibStep step = WAITING_FOR_CENTRE;
    std::vector<CalibPoint> capturedPoints;
    std::vector<float> targetAngles;
    int currentIdx = 0;
    float sumCosError = 0.0f;
    float sumSinError = 0.0f;

    bool IsValidSequence();
    void Start(bool left);
    void Update(float x, float y);
    void Finish();
};