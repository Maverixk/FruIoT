/*
 * FruIoT - Monitoraggio IoT del Deterioramento della Frutta
 * 
 * Direttore d'orchestra: coordina sensori, rete, display e power management.
 * 
 * Ciclo operativo:
 *   1. Wake-up dal Deep Sleep
 *   2. Inizializzazione hardware (sensori, display, Wi-Fi)
 *   3. Lettura dati sensori (DHT22 + MQ-135)
 *   4. Visualizzazione dati su display OLED
 *   5. Invio dati a ThingSpeak
 *   6. Deep Sleep per SLEEP_INTERVAL_MINUTES minuti
 */

#include "heltec.h"

// Moduli del progetto
#include "config.h"
#include "secrets.h"
#include "sensors/sensors.h"
#include "network/network.h"
#include "power/power.h"
#include "display/display.h"

void setup() {
    // --- Inizializzazione Heltec (Serial + OLED + LoRa disabilitato) ---
    Heltec.begin(
        true,   // Display OLED abilitato
        false,  // LoRa disabilitato (non lo usiamo)
        true    // Serial abilitata
    );

    Serial.println("\n=============================");
    Serial.println("  FruIoT - Avvio sistema");
    Serial.println("=============================\n");

    // --- Fase 1: Inizializzazione moduli ---
    display::init();
    display::showMessage("FruIoT", "Welcome to the final show...");
    // --- Fase 2: Connessione Wi-Fi ---
    // --- Fase 3: Inizializzazione e lettura sensori ---

    // forzare la ricalibrazione completa al primo utilizzo, commentare dopo
    sensors::resetMQ135Calibration(); 
    // init() gestisce warm-up e calibrazione R0 secondo MQ135_WARMUP_STRATEGY
    sensors::init();
 
    sensors::SensorData data = sensors::poll();

    // --- Stampa dati sensore ---

    // --- MQ135 ---
    if (data.mq135Ok) {
        Serial.printf("[main] CO2: %.1f ppm\n", data.mq135CO2ppm);
    } else {
        Serial.println("[main] ERRORE: lettura MQ135 fallita.");
    }

    // --- DHT22 ---
    if (data.dhtOk) {
        Serial.printf("[main] Clima: %.1f °C | Umidita': %.1f %%\n", data.temperatureC, data.humidityPct);
    } else {
        Serial.println("[main] ERRORE: lettura DHT22 fallita.");
    }

    // --- Fase 4: Mostra dati su display ---
    // --- Fase 5: Invio dati a ThingSpeak --- 
    // --- Fase 6: Deep Sleep ---
    Serial.println("[main] Entro in deep sleep...");
    Serial.flush(); // svuota il buffer Serial prima di dormire
    esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_MINUTES * 60 * uS_TO_S_FACTOR);
    esp_deep_sleep_start();

}

void loop() {
    // Non raggiunto: il deep sleep riavvia sempre da setup().
    // Richiesto dal framework Arduino per la compilazione.
}