#include "power_monitor.h"
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=========================================");
  Serial.println(" FruIoT MONITOR — INA219 energy logger");
  Serial.println("=========================================");

  // Avvia il polling INA219 in un task FreeRTOS dedicato (Core 0).
  initPowerMonitor();

  Serial.println("=========================================");
}

void loop() {
  // Tutto il lavoro è nel task del power monitor. Liberiamo il loopTask.
  vTaskDelete(NULL);
}
