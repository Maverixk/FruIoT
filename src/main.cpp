#include "heltec.h"

#include "config.h"
#include "display/display.h"
#include "network/network.h"
#include "secrets.h"
#include "sensors/sensors.h"
#include "tinyml/tinyml.h"

void setup() {

  // Inizializzazione Heltec: display OLED attivo, LoRa disattivo, seriale attivo
  Heltec.begin(true, false, true);

  Serial.println("\n=============================");
  Serial.println("  FruIoT - Avvio sistema");
  Serial.println("=============================\n");

  if (!init_model()) {
    Serial.println("[main] ERROR: TinyML loading failed!");
  } else {
    Serial.println("[main] TinyML model loaded and ready.");
  }

#if USE_TRANSISTOR == 1
  // --- Boost converter (XL6009 + MQ-135): accensione tramite S8050 ---
  pinMode(TRANSISTOR_PIN, OUTPUT);
  digitalWrite(TRANSISTOR_PIN, HIGH);
  Serial.println("[main] Boost converter ON (GPIO6 HIGH): MQ-135 alimentato.");
#endif

  // --- Inizializzazione moduli ---
  display::init();
  display::showMessage("FruIoT", "Welcome to the final show...");

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

  // Local inference with TinyML
  int status = predict_status(data);

  Serial.print("[main] Spoilage status prediction: ");
  if (status == 0) Serial.println("0 - UNRIPE");
  else if (status == 1) Serial.println("1 - MATURE");
  else if (status == 2) Serial.println("2 - RUINED");
  else Serial.println("Error in prediction (-1)");


  // --- Invio dati a ThingSpeak ---
  network::DataPacket packet = {data.mq135Raw,     data.mq135Ratio,
                                data.temperatureC, data.humidityPct,
                                status};
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

#if USE_TRANSISTOR == 1
  digitalWrite(TRANSISTOR_PIN, LOW);
  Serial.println("[main] Boost converter OFF (GPIO6 LOW): MQ-135 spento.");
#endif

  digitalWrite(DHT_VCC_PIN, LOW);
  Serial.println("[main] DHT22 VCC OFF (GPIO47 LOW).");

  Serial.println("[main] Entro in deep sleep...");
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_INTERVAL_MINUTES * 60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {}
