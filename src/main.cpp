#include "heltec.h"

#include "config.h"
#include "display/display.h"
#include "network/network.h"
#include "power/power.h"
#include "secrets.h"
#include "sensors/sensors.h"

void setup() {

  // Inizializzazione Heltec: display OLED ttivo, LoRa disattivo, seriale attivo
  Heltec.begin(true, false, true);

  Serial.println("\n=============================");
  Serial.println("  FruIoT - Avvio sistema");
  Serial.println("=============================\n");

  // --- Transistor S8050: accende l'MQ135 ---
  pinMode(TRANSISTOR_PIN, OUTPUT);
  digitalWrite(TRANSISTOR_PIN, HIGH);
  Serial.println("[main] Transistor APERTO: MQ135 alimentato.");

  // --- Inizializzazione moduli ---
  display::init();
  display::showMessage("FruIoT", "Welcome to the final show...");

#if CURRENT_MONITOR == 1
  power::init();
#endif

  // --- WiFi: avvia connessione in background ---
  network::init();

  // --- Sensori: warmup MQ135 (intanto il WiFi si connette su Core 0) ---
  // Per forzare ricalibrazione R0, decommentare:
  // sensors::resetMQ135Calibration();
  // sensors::forceMQ135R0(18.26f);
  sensors::init();

  // --- WiFi: verifica connessione (dopo warmup è già connesso) ---
  network::await_wifi();
  sensors::SensorData data = sensors::poll();

  // --- Stampa dati ---
  if (data.mq135Ok) {
    Serial.printf("[main] MQ135: raw=%d ratio=%.5f\n", data.mq135Raw,
                  data.mq135Ratio);
  } else {
    Serial.println("[main] ERRORE: lettura MQ135 fallita.");
  }

  if (data.dhtOk) {
    Serial.printf("[main] Temperature: %.2f °C\n", data.temperatureC);
    Serial.printf("[main] Humidity: %.2f %%\n", data.humidityPct);
  } else {
    Serial.println("[main] ERRORE: lettura DHT22 fallita.");
  }

  // --- Corrente media warm-up ---
  float avg_warmup_current = 0;
#if CURRENT_MONITOR == 1
  if (sensors::getWarmupNumSamples() > 0) {
    float total = 0;
    for (int i = 0; i < sensors::getWarmupNumSamples(); i++)
      total += sensors::getWarmupCurrentSample(i);
    avg_warmup_current = total / sensors::getWarmupNumSamples();
    Serial.printf("[main] Corrente media warm-up: %.2f mA\n",
                  avg_warmup_current);
  }
#endif

  // --- Corrente media polling ---
  float avg_polling_current = 0;
#if CURRENT_MONITOR == 1
  if (sensors::getPollingNumSamples() > 0) {
    float total = 0;
    for (int i = 0; i < sensors::getPollingNumSamples(); i++)
      total += sensors::getPollingCurrentSample(i);
    avg_polling_current = total / sensors::getPollingNumSamples();
    Serial.printf("[main] Corrente media polling: %.2f mA\n",
                  avg_polling_current);
  }
#endif

  // --- Invio dati a ThingSpeak ---
  network::DataPacket packet = {(float)data.mq135Raw, data.mq135Ratio,
                                data.temperatureC,    data.humidityPct,
                                avg_warmup_current,   avg_polling_current};
  int response = network::send_via_wifi(packet);
  if (response == 200) {
    Serial.println(
        "[network] Data successfully delivered to ThingSpeak channel!");
  } else {
    Serial.println(
        "[network] ThingSpeak channel update failed with HTTP error code " +
        String(response));
  }

  // --- Mostra i dati sul display OLED ---
  display::showSensorData(data.temperatureC, data.humidityPct, data.mq135Ratio);
  delay(3000); // Visualizza i dati per 3 secondi prima di spegnersi

  // --- Spegnimento e Deep Sleep ---
  display::off();

  digitalWrite(TRANSISTOR_PIN, LOW);
  Serial.println("[main] Transistor CHIUSO: MQ135 spento.");

  Serial.println("[main] Entro in deep sleep...");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_MINUTES * 60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {}
