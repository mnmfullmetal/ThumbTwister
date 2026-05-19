#include "Calibration.h"
#include "Visualiser.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>


// -- CALIBRATION VALIDATION ---
bool CalibrationManager::IsValidSequence()
{
    // check for identical back-to-back inputs 
    for (size_t i = 1; i < targetAngles.size(); i++)
    {
        if (targetAngles[i] == targetAngles[i - 1]) return false;
    }

    // check for predictable circles 
    for (size_t i = 2; i < targetAngles.size(); i++)
    {
        // calculate the angle of the first turn
        float d1 = targetAngles[i - 1] - targetAngles[i - 2];
        while (d1 > 180.0f) d1 -= 360.0f;
        while (d1 < -180.0f) d1 += 360.0f;

        // calculate the angle of the second turn
        float d2 = targetAngles[i] - targetAngles[i - 1];
        while (d2 > 180.0f) d2 -= 360.0f;
        while (d2 < -180.0f) d2 += 360.0f;

        // if both turns are +90 or both are -90, it forms a predictable arc, fuck it off bro
        if (std::abs(d1) == 90.0f && d1 == d2)
        {
            return false;
        }
    }
    return true;
}

// triggered when the calib button is clicked
void CalibrationManager::Start(bool left)
{
    capturedPoints.clear();

    // create target angle vectors for calibration input request 
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

    // var to store vector components of angular error
    sumCosError = 0.0f;
    sumSinError = 0.0f;

    // shuffle the directions for randomness
    std::random_device rd;
    std::mt19937 g(rd());
    do {
        std::shuffle(targetAngles.begin(), targetAngles.end(), g);
    } while (!IsValidSequence());
}

void CalibrationManager::Update(float x, float y)
{
    if (!active) return;

    // get magnitude vector of thumbstick
    float mag = std::sqrtf(x * x + y * y);

    // -- DEADZONE CHECK FOR RETURN TO CENTRE --
    if (step == WAITING_FOR_CENTRE)
    {
        SetCalibrationUI("RETURN TO CENTER", true);
        if (mag < 0.2f) step = WAITING_FOR_PUSH;
    }
    // capture when the stick is pushed 
    else if (step == WAITING_FOR_PUSH)
    {
        float targetDeg = targetAngles[currentIdx];
        float target_x = 0.0f;
        float target_y = 0.0f;

        // set UI text based on the target angle and define ideal target vectors
        if (targetDeg == 90.0f) { SetCalibrationUI("PUSH UP", false);    target_x = 0.0f;  target_y = 1.0f; }
        else if (targetDeg == 270.0f) { SetCalibrationUI("PUSH DOWN", false);  target_x = 0.0f;  target_y = -1.0f; }
        else if (targetDeg == 180.0f) { SetCalibrationUI("PUSH LEFT", false);  target_x = -1.0f; target_y = 0.0f; }
        else if (targetDeg == 0.0f) { SetCalibrationUI("PUSH RIGHT", false); target_x = 1.0f;  target_y = 0.0f; }

        //-- ERROR NORMALISATISION --
        if (mag > 0.90f)
        {
            capturedPoints.push_back(CalibPoint{ x, y });
            std::cout << "Captured Point [" << currentIdx + 1 << "/20]: X = " << x << ", Y = " << y << "\n";

            // normalise the raw cartesian coordinates
            float norm_x = x / mag;
            float norm_y = y / mag;

            float cosError = (norm_x * target_x) + (norm_y * target_y);
            float sinError = (norm_x * target_y) - (norm_y * target_x);

            sumCosError += cosError;
            sumSinError += sinError;

            currentIdx++;

            // check if completed all directions for calibration
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

void CalibrationManager::Finish()
{
    float avgCos = sumCosError / (float)targetAngles.size();
    float avgSin = sumSinError / (float)targetAngles.size();

    // convert the final averaged matrix back to polar degrees 
    float averageOffset = std::atan2f(avgSin, avgCos) * (180.0f / 3.14159265f);

    //  invert the offset to correct it (if error is +5, move -5)
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
