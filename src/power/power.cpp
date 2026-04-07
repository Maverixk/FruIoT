#include "power.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_INA219.h>

/**
 * Modulo Power — implementazione.
 *
 * L'INA219 è collegato sul bus I2C secondario dell'ESP32-S3 (Wire1)
 * per evitare conflitti con il display OLED della Heltec che usa Wire
 * sui pin interni GPIO17/18.
 *
 * Il sensore è cablato in serie con l'alimentazione 5V dell'MQ-135:
 *   5V (USB) → Vin+ (INA219) → Vin- (INA219) → VCC (MQ-135)
 *
 * Questo permette di misurare la corrente assorbita dal solo fornetto
 * dell'MQ-135 senza influenzare il resto del circuito.
 */

namespace power {

// ─────────────────────────────────────────────────────────────────────────────
// Stato interno del modulo
// ─────────────────────────────────────────────────────────────────────────────
static Adafruit_INA219 s_ina219(INA219_I2C_ADDR);
static bool            s_ina219_ok = false;  // true se l'INA219 è stato inizializzato con successo

// ─────────────────────────────────────────────────────────────────────────────
// API pubblica
// ─────────────────────────────────────────────────────────────────────────────

void init() {
    // Inizializza il bus I2C secondario (Wire1) sui pin custom
    Wire1.begin(INA219_SDA, INA219_SCL);

    // Inizializza l'INA219 sul bus Wire1
    s_ina219_ok = s_ina219.begin(&Wire1);

    if (s_ina219_ok) {
        Serial.println("[INA219] Inizializzato correttamente su Wire1.");
        Serial.printf("[INA219] Indirizzo I2C: 0x%02X | SDA: GPIO%d | SCL: GPIO%d\n",
                      INA219_I2C_ADDR, INA219_SDA, INA219_SCL);
    } else {
        Serial.println("[INA219] ERRORE: sensore non trovato! Controllare cablaggio.");
    }
}

PowerData readINA219() {
    PowerData data{};

    if (!s_ina219_ok) {
        Serial.println("[INA219] ERRORE: sensore non inizializzato. Chiamare init() prima.");
        data.ok = false;
        return data;
    }

    data.shuntVoltage_mV = s_ina219.getShuntVoltage_mV();
    data.busVoltage_V    = s_ina219.getBusVoltage_V();
    data.current_mA      = s_ina219.getCurrent_mA();
    data.power_mW        = s_ina219.getPower_mW();
    data.ok              = true;

    return data;
}

float getBatteryVoltage() {
    // TODO: implementare lettura batteria LiPo dalla Heltec V3
    return -1.0f;
}

void goToDeepSleep() {
    // TODO: implementare deep sleep (attualmente gestito direttamente in main.cpp)
}

} // namespace power
