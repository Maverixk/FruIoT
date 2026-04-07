#ifndef POWER_H
#define POWER_H

#include <Arduino.h>

/**
 * Modulo Power — monitoraggio energetico e gestione alimentazione.
 *
 * Gestisce l'INA219 per misurare tensione, corrente e potenza
 * del carico monitorato (MQ-135 heater intercettato su Vin+/Vin-).
 */
namespace power {

/**
 * Dati letti dall'INA219.
 *
 * L'INA219 misura:
 *   - busVoltage_V  : tensione sul lato carico (Vin- verso GND), in Volt
 *   - current_mA    : corrente che attraversa lo shunt, in milliAmpere
 *   - power_mW      : potenza calcolata dal chip, in milliWatt
 *   - shuntVoltage_mV: caduta di tensione sullo shunt interno, in milliVolt
 *   - ok            : true se la lettura è andata a buon fine
 */
struct PowerData {
    float busVoltage_V;
    float current_mA;
    float power_mW;
    float shuntVoltage_mV;
    bool  ok;
};

/**
 * Inizializza il modulo power.
 * Configura il bus I2C secondario (Wire1) su GPIO6/7 e inizializza l'INA219.
 * Deve essere chiamata una volta in setup() prima di readINA219().
 */
void init();

/**
 * Legge tensione, corrente e potenza dall'INA219.
 * Chiamare solo dopo init().
 */
PowerData readINA219();

/**
 * Legge la tensione della batteria LiPo dalla Heltec V3.
 * TODO: implementare quando si collegherà la batteria.
 */
float getBatteryVoltage();

/**
 * Mette l'ESP32 in Deep Sleep per il numero di minuti definito in config.h.
 */
void goToDeepSleep();

}

#endif // POWER_H
