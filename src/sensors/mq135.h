#pragma once
#include <Arduino.h>

namespace mq135 {

struct Data {
    int   raw;    // lettura ADC mediana (0–4095)
    float rs;     // resistenza del sensore (kΩ)
    float ratio;  // RS / R0
    bool  ok;
};

void init();
Data poll();
void resetCalibration();
void forceR0(float r0);

int getWarmupNumSamples();
int getPollingNumSamples();
float getWarmupCurrentSample(int index);
float getPollingCurrentSample(int index);

} // namespace mq135
