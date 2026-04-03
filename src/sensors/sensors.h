#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

/**
 * Modulo sensori — orchestratore.
 *
 * Aggrega i dati di tutti i sensori in SensorData e coordina init() e poll().
 * main.cpp interagisce solo con questo modulo.
 *
 */
namespace sensors {

struct SensorData {
    // --- MQ135 ---
    float  mq135CO2ppm;  
    float  mq135Ratio;   
    bool   mq135Ok;

    // --- DHT22 (decommentare quando disponibile) ---
    // float temperatureC;
    // float humidityPct;
    // bool  dhtOk;
};

void init();

/**
 * Legge tutti i sensori e restituisce i dati aggregati.
 * Chiamare solo dopo init().
 */
SensorData poll();

/**
 * Forza la ricalibrazione di R0 dell'MQ135 al prossimo avvio.
 * Utile se il sensore è invecchiato o spostato in ambiente diverso.
 */
void resetMQ135Calibration();

} // namespace sensors

#endif // SENSORS_H