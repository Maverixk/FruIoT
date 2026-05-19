#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

/**
 * MONITOR firmware — INA219 polling loop.
 *
 * Si avvia tramite initPowerMonitor() (chiamata in setup() del main).
 * Lancia un FreeRTOS task pinned su Core 0 che campiona l'INA219 ogni
 * 100 ms, accumula energia in mWs e logga su Serial.
 *
 * Cablaggio (lato LOAD, batteria 3.7V LiPo):
 *   LiPo (+) → INA219 Vin+ → [shunt 0.1Ω] → INA219 Vin- → carico (3.7V rail)
 *   LiPo (−) → COMMON GND
 *
 * Cablaggio I2C (lato MONITOR, ESP32 alimentato via USB):
 *   ESP32 GPIO1 ↔ INA219 SDA
 *   ESP32 GPIO2 ↔ INA219 SCL
 *   ESP32 3V3   → INA219 VCC
 *   ESP32 GND   → INA219 GND → COMMON GND con lato LOAD
 */
void initPowerMonitor();

#endif // POWER_MONITOR_H
