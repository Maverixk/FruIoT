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
#include "../include/config.h"
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
    // --- Fase 3: Lettura sensori ---
    // --- Fase 4: Mostra dati su display ---
    // --- Fase 5: Invio dati a ThingSpeak --- 
    // --- Fase 6: Deep Sleep ---
}

void loop() {
    // Con il Deep Sleep, loop() non viene mai raggiunto.
    // L'ESP32 si riavvia sempre da setup() al wakeup.
}