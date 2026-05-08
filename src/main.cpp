#include "heltec.h"
#include "config.h"
#include "display/display.h"
#include "network/network.h"
#include "power/power.h"
#include "secrets.h"
#include "sensors/sensors.h"

void setup() {
  // Inizializzazione hardware base
  Heltec.begin(true, false, true);

  Serial.println("\n=============================");
  Serial.println("  FruIoT - Avvio HACK Powerbank");
  Serial.println("=============================\n");

  // Inizializza il pin del transistor ma lo tiene chiuso per ora
  pinMode(TRANSISTOR_PIN, OUTPUT);
  digitalWrite(TRANSISTOR_PIN, LOW);

  display::init();
  display::showMessage("FruIoT", "Modo 1 Attivo", "Powerbank Hack");

#if CURRENT_MONITOR == 1
  power::init();
#endif

  // Avvia il WiFi per la prima volta
  network::init();
}

void loop() {
  Serial.println("\n--- INIZIO NUOVO CICLO ---");

  // --- 1. Riaccende il Display OLED ---
  Heltec.display->displayOn(); 

  // --- 2. Accende il sensore ---
  digitalWrite(TRANSISTOR_PIN, HIGH);
  Serial.println("[main] Transistor APERTO: MQ135 alimentato.");

  // --- 3. Inizializza e fa il Warm-up dell'MQ135 (90s) ---
  sensors::init();

  // --- 4. Controlla il WiFi e legge i dati ---
  network::await_wifi();
  sensors::SensorData data = sensors::poll();

  // --- 5. Stampa a schermo seriale ---
  if (data.mq135Ok) {
    Serial.printf("[main] MQ135: raw=%d ratio=%.5f\n", data.mq135Raw, data.mq135Ratio);
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
    Serial.printf("[main] Corrente media warm-up: %.2f mA\n", avg_warmup_current);
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
    Serial.printf("[main] Corrente media polling: %.2f mA\n", avg_polling_current);
  }
#endif

  // --- 6. Invio a ThingSpeak ---
  network::DataPacket packet = {data.mq135Raw, data.mq135Ratio, data.temperatureC, data.humidityPct, avg_warmup_current, avg_polling_current};
  int response = network::send_via_wifi(packet);
  
  if (response == 200) {
    Serial.println("[network] Data successfully delivered to ThingSpeak channel!");
  } else {
    Serial.println("[network] ThingSpeak error: " + String(response));
  }

  // --- 7. Mostra dati su OLED ---
  display::showSensorData(data.temperatureC, data.humidityPct, data.mq135Ratio);
  delay(3000); // Visualizza i dati per 3 secondi

  // --- 8. Spegnimento di tutto e attesa ---
  display::off();
  digitalWrite(TRANSISTOR_PIN, LOW);
  Serial.println("[main] Transistor CHIUSO: MQ135 spento.");

  Serial.println("[main] Inizio pausa attiva (Delay)... Il powerbank dovrebbe rimanere acceso.");
  Serial.flush();

  // Invece del deep sleep, facciamo un delay() usando i minuti del config.h
  // NB: 1 minuto = 60000 millisecondi
  unsigned long delay_ms = SLEEP_INTERVAL_MINUTES * 60 * 1000UL;
  delay(delay_ms);
}