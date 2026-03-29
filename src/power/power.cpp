#include "power.h"
#include "config.h"
#include <WiFi.h>

// Pin ADC per la lettura della batteria sulla Heltec WiFi LoRa 32 V3
// La Heltec V3 usa GPIO1 con un partitore di tensione e un MOSFET su GPIO37
#define BATTERY_ADC_PIN   1
#define VBAT_CTRL_PIN     37

//sta cosa non è che l'ho ben capita

namespace power {

    void init() {
       //
    }

    float getBatteryVoltage() {
        //
    }

    void goToDeepSleep() {
        //
    }

}
