#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

namespace sensors {

    struct SensorData {
        // --- MQ135 ---
        int   mq135Raw;
        float mq135Rs;
        float mq135Ratio;
        bool  mq135Ok;

        // --- DHT22 ---
        float temperatureC;
        float humidityPct;
        bool  dhtOk;
    };

    void init();
    SensorData poll();

    void resetMQ135Calibration();
    void forceMQ135R0(float r0);

    int getWarmupNumSamples();
    int getPollingNumSamples();
    float getWarmupCurrentSample(int index);
    float getPollingCurrentSample(int index);

} // namespace sensors

#endif // SENSORS_H
