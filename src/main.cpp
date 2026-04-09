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
    power::init();
    // --- Fase 2: Connessione Wi-Fi ---
    network::init();
    network::connect_to_wifi();
    Serial.printf("[network] ESP32 successfully connected to %s.\n", WIFI_SSID_LINDA);

    // --- Fase 3: Inizializzazione e lettura sensori ---

    // forzare la ricalibrazione completa al primo utilizzo, commentare dopo
    //sensors::resetMQ135Calibration(); 

    //sensors::forceMQ135R0(18.26f);
    
    // init() gestisce warm-up e calibrazione R0 secondo MQ135_WARMUP_STRATEGY
    sensors::init();
 
    
    sensors::SensorData data = sensors::poll();

   
    // --- Stampa dati sensore ---

    // --- MQ135 ---
    if (data.mq135Ok) {
        Serial.printf("[main] CO2: %.2f ppm\n", data.mq135CO2ppm);
    } else {
        Serial.println("[main] ERRORE: lettura MQ135 fallita.");
    }

    if (data.dhtOk) {
        Serial.printf("[main] Temperature: %.2f °C\n", data.temperatureC);
        Serial.printf("[main] Humidity: %.2f %%\n", data.humidityPct);
    } else {
        Serial.println("[main] ERRORE: lettura DHT22 fallita.");
    }

    
    // Calcola la corrente media durante il warm-up e il polling
    float total_warmup_current = 0;
    for (int i = 0; i < sensors::getWarmupNumSamples(); i++) {
        total_warmup_current += sensors::getWarmupCurrentSample(i);
    }
    float avg_warmup_current = total_warmup_current / sensors::getWarmupNumSamples();
    Serial.printf("[main] Corrente media durante warm-up: %.2f mA\n", avg_warmup_current);

    float total_polling_current = 0;
    for (int i = 0; i < sensors::getPollingNumSamples(); i++) {
        total_polling_current += sensors::getPollingCurrentSample(i);
    }
    float avg_polling_current = total_polling_current / sensors::getPollingNumSamples();
    Serial.printf("[main] Corrente media durante polling: %.2f mA\n", avg_polling_current);




    // --- Fase 4: Invio dati a ThingSpeak --- 
    network::DataPacket packet = {
        (float)data.mq135Raw,
        data.mq135CO2ppm, 
        data.temperatureC,
        data.humidityPct,
        avg_warmup_current,
        avg_polling_current
    };
    int response = network::send_via_wifi(packet);
    if(response == 200) {
        Serial.println("[network] Data successfully delivered to ThingSpeak channel!");
    }
    else {
        Serial.println("[network] ThingSpeak channel update failed with HTTP error code " + String(response));
    }


    // --- Fase 5: Deep Sleep ---
    Serial.println("[main] Entro in deep sleep...");
    Serial.flush(); // svuota il buffer Serial prima di dormire
    esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_MINUTES * 60 * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
    

}

void loop() {
    // Non raggiunto: il deep sleep riavvia sempre da setup().
    // Richiesto dal framework Arduino per la compilazione.
}