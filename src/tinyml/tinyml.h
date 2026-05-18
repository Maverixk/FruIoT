#ifndef TINYML_H
#define TINYML_H

#include "sensors/sensors.h"
#include <Arduino.h>

// Only called ONCE when the device cold-boots (not after deep sleep)
bool init_model();

// Call this to get a prediction based on sensor readings
// Returns: 0 (Unripe), 1 (Mature), 2 (Ruined), or -1 (Error)
int predict_status(sensors::SensorData data);

#endif // TINYML_H