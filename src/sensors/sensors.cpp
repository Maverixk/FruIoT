#include "sensors.h"
#include "mq135.h"
#include "dht22.h"

namespace sensors {

    void init() {
        mq135::init();
        dht22::init();
    }

    SensorData poll() {
        SensorData data{};

        mq135::Data mq = mq135::poll();
        data.mq135Raw   = mq.raw;
        data.mq135Rs    = mq.rs;
        data.mq135Ratio = mq.ratio;
        data.mq135Ok    = mq.ok;

        dht22::Data dht = dht22::poll();
        data.temperatureC = dht.temperatureC;
        data.humidityPct  = dht.humidityPct;
        data.dhtOk        = dht.ok;

        return data;
    }

    void resetMQ135Calibration() { mq135::resetCalibration(); }
    void forceMQ135R0(float r0)  { mq135::forceR0(r0); }

    int getWarmupNumSamples()            { return mq135::getWarmupNumSamples(); }
    int getPollingNumSamples()           { return mq135::getPollingNumSamples(); }
    float getWarmupCurrentSample(int i)  { return mq135::getWarmupCurrentSample(i); }
    float getPollingCurrentSample(int i) { return mq135::getPollingCurrentSample(i); }

}
