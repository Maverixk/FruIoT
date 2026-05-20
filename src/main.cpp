#include "heltec.h"

#include "config.h"
#include "display/display.h"
#include "network/network.h"
#include "power/power.h"
#include "provisioning/provisioning.h"
#include "secrets.h"
#include "sensors/sensors.h"
#include "thingsboard/thingsboard.h"
#include "tinyml/tinyml.h"

void setup() {

  // Inizializzazione Heltec: display OLED attivo, LoRa disattivo, seriale attivo
  Heltec.begin(true, false, true);

  Serial.println("\n=============================");
  Serial.println("  FruIoT - Avvio sistema");
  Serial.println("=============================\n");

  // ⚠️ TEMPORANEO: forza re-provisioning cancellando le credenziali WiFi da NVS.
  // Decommenta questa riga se vuoi tornare in modalità provisioning al prossimo boot.
  // NOTA: R0 (namespace "fruiot") NON viene toccato, resta calibrato.
  // provisioning::clearWiFiCredentials();

if (!init_model()) {
    Serial.println("[main] ERROR: TinyML loading failed!");
  } else {
    Serial.println("[main] TinyML model loaded and ready.");
  }

#if USE_TRANSISTOR == 1
  // --- Transistor S8050: accende l'MQ135 ---
  pinMode(TRANSISTOR_PIN, OUTPUT);
  digitalWrite(TRANSISTOR_PIN, HIGH);
  Serial.println("[main] Transistor APERTO: MQ135 alimentato.");
#endif

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
  network::await_wifi(WIFI_CONNECT_TIMEOUT_MS);

  // Inizializza ThingsBoard MQTT client (carica token da NVS o secrets.h)
  //thingsboard::init();
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

  // Local inference with TinyML
  int64_t t0 = esp_timer_get_time();
  int status = predict_status(data);
  int64_t inference_time_us = esp_timer_get_time() - t0;

  Serial.printf("[main] Inference time: %lld us\n", inference_time_us);
  Serial.print("[main] Spoilage status prediction: ");
  if (status == 0) Serial.println("0 - UNRIPE");
  else if (status == 1) Serial.println("1 - MATURE");
  else if (status == 2) Serial.println("2 - RUINED");
  else Serial.println("Error in prediction (-1)");


  // --- Invio dati a ThingSpeak ---
  network::DataPacket packet = {data.mq135Raw,        data.mq135Ratio,
                                data.temperatureC,    data.humidityPct,
                                status,               avg_warmup_current,
                                avg_polling_current,   (float)inference_time_us};
  int response = network::send_via_wifi(packet);
  if (response == 200) {
    Serial.println(
        "[network] Data successfully delivered to ThingSpeak channel!");
  } else {
    Serial.println(
        "[network] ThingSpeak channel update failed with HTTP error code " +
        String(response));
  }

  // --- ThingsBoard: invio alert solo per stati critici (status >= 1) ---
  // L'alert scatena una Rule Chain sul cloud che inoltra una notifica Telegram.
  //thingsboard::publish_alert(status, data);

  // --- Mostra i dati sul display OLED ---
  display::showSensorData(data.temperatureC, data.humidityPct, data.mq135Ratio);
  delay(3000); // Visualizza i dati per 3 secondi prima di spegnersi

  // --- Spegnimento e Deep Sleep ---
  display::off();

#if USE_TRANSISTOR == 1
  digitalWrite(TRANSISTOR_PIN, LOW);
  Serial.println("[main] Transistor CHIUSO: MQ135 spento.");
#endif

  Serial.println("[main] Entro in deep sleep...");
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_INTERVAL_MINUTES * 60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {}
