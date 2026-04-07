#include "sensors.h"
#include "mq135.h"
#include "dht22.h"

/**
 * Orchestratore sensori.
 *
 * Questo file non contiene logica di sensore — chiama i moduli specifici
 * e aggrega i risultati in SensorData.
 * Tutta la logica MQ135 è in mq135.cpp, DHT22 in dht22.cpp.
 */
namespace sensors {

    void init() {
        mq135::init();
        dht22::init(); // decommentare quando disponibile
    }

    SensorData poll() {
        SensorData data{};

        mq135::Data mq = mq135::poll();
        data.mq135CO2ppm = mq.co2ppm;
        data.mq135Ratio  = mq.ratio;
        data.mq135Ok     = mq.ok;

        dht22::Data dht = dht22::poll(); // decommentare quando disponibile
        data.temperatureC = dht.temperatureC;
        data.humidityPct  = dht.humidityPct;
        data.dhtOk        = dht.ok;

        return data;
    }

    void resetMQ135Calibration() {
        mq135::resetCalibration();
    }

}