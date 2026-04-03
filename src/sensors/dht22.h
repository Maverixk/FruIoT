#pragma once
#include <Arduino.h>

namespace dht22 {
 
struct Data {
    float temperatureC;  // temperatura (°C)
    float humidityPct;   // umidità relativa (%)
    bool  ok;
};
 
void init();
Data poll();
 
} // namespace dht22