#ifndef POWER_H
#define POWER_H

#include <Arduino.h>

namespace power {

    // Inizializza il modulo power (pin batteria, ecc.)
    void init();

    // Legge la tensione della batteria LiPo dalla Heltec V3
    float getBatteryVoltage();

    // Mette l'ESP32 in Deep Sleep per il numero di minuti definito in config.h
    void goToDeepSleep();

}

#endif // POWER_H
