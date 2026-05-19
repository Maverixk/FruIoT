#include "power_monitor.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// --- Cablaggio I2C (MONITOR side) ---
#define INA219_SDA_PIN  1   // GPIO1 → INA219 SDA
#define INA219_SCL_PIN  2   // GPIO2 → INA219 SCL

// Periodo di campionamento (ms). 100 ms = 10 Hz.
#define POLL_PERIOD_MS  100

static Adafruit_INA219 ina219;
static TaskHandle_t    s_power_task_handle = nullptr;

// Loop infinito del task di monitoraggio: legge INA219, accumula energia
// (mWs) usando il delta di tempo effettivo tra campioni e logga su Serial.
static void vTaskPowerMonitor(void *pvParameters) {
    double   energy_mWs = 0.0;
    uint32_t last_us    = micros();

    for (;;) {
        float bus_V      = ina219.getBusVoltage_V();
        float current_mA = ina219.getCurrent_mA();
        float power_mW   = ina219.getPower_mW();

        uint32_t now_us = micros();
        // dt in secondi a partire da micros() (gestisce il wrap-around dell'uint32)
        float dt_s = (float)(now_us - last_us) / 1e6f;
        last_us = now_us;

        energy_mWs += (double)power_mW * (double)dt_s;

        Serial.printf("[INA219] V=%6.2f V | I=%8.2f mA | P=%8.2f mW | E=%10.2f mWs\n",
                      bus_V, current_mA, power_mW, energy_mWs);

        vTaskDelay(POLL_PERIOD_MS / portTICK_PERIOD_MS);
    }
}

void initPowerMonitor() {
    // Bus I2C primario sui pin custom GPIO1/GPIO2
    Wire.begin(INA219_SDA_PIN, INA219_SCL_PIN);

    if (!ina219.begin(&Wire)) {
        Serial.println("[INA219] ERRORE: sensore non trovato. Controllare SDA/SCL.");
        while (1) { delay(1000); } // hard fault: senza INA219 non c'è nulla da fare
    }

    // Calibrazione 32V / 1A: range adeguato per LiPo 3.7V e correnti < 1A.
    ina219.setCalibration_32V_1A();
    Serial.println("[INA219] Pronto. Avvio task di polling su Core 0.");

    xTaskCreatePinnedToCore(
        vTaskPowerMonitor,    // funzione
        "PowerMonTask",       // nome
        4096,                 // stack
        nullptr,              // parametri
        1,                    // priorità
        &s_power_task_handle, // handle
        0                     // core 0 (Wi-Fi/BT core, qui inutilizzato)
    );
}
