#include "network.h"
#include <WiFi.h>
#include "ThingSpeak.h"
#include "../include/secrets.h"
#include "config.h"
#include "../provisioning/provisioning.h"

// USE_BLE_PROVISIONING e WIFI_CONNECT_TIMEOUT_MS sono definiti in config.h.

namespace network {

    static WiFiClient client;

    // Helper: avvia WiFi.begin con le credenziali appropriate (NVS o secrets.h).
    // Ritorna l'SSID che sta tentando per messaggi diagnostici.
    static String startWiFiBegin() {
#if USE_BLE_PROVISIONING == 1
        String ssid, pass;
        if (provisioning::getWiFiCredentials(ssid, pass)) {
            Serial.printf("[network] Connessione a WiFi (SSID=%s)...\n", ssid.c_str());
            WiFi.begin(ssid.c_str(), pass.c_str());
            return ssid;
        }
        Serial.println("[network] Provisioning fallito, fallback a secrets.h.");
#endif
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        return String(WIFI_SSID);
    }

    // Helper: attende la connessione WiFi con timeout. Stampa i dot di progresso.
    static bool waitForConnection(unsigned long timeout_ms) {
        Serial.print("[network] Attesa connessione WiFi...");
        unsigned long start = millis();
        while (!wifi_connected() && (millis() - start) < timeout_ms) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("");
        return wifi_connected();
    }

    void init() {
        WiFi.mode(WIFI_STA);
        startWiFiBegin();
    }

    bool wifi_connected() {
        return (WiFi.status() == WL_CONNECTED);
    }

    bool await_wifi(int timeout_ms) {
        // --- 1. Già connesso? OK ---
        if (wifi_connected()) {
            Serial.println("[network] WiFi gia' connesso.");
            ThingSpeak.begin(client);
            return true;
        }

        // --- 2. Primo tentativo: aspetta entro timeout ---
        if (waitForConnection((unsigned long)timeout_ms)) {
            Serial.println("[network] WiFi connesso.");
            ThingSpeak.begin(client);
            return true;
        }

        Serial.println("[network] ERRORE: timeout connessione WiFi.");

#if USE_BLE_PROVISIONING == 1
        // --- 3. Fallback: le credenziali NVS potrebbero essere obsolete
        //        (es. WiFi di casa cambiato). Cancello e ri-provisiono.
        Serial.println("[network] Le credenziali salvate non funzionano.");
        Serial.println("[network] Avvio nuovo provisioning via BLE...");

        WiFi.disconnect(true);     // chiudi tentativo precedente
        provisioning::clearWiFiCredentials();

        // Riavvio il flusso: getWiFiCredentials() ora vedrà NVS vuota
        // e farà partire il provisioning BLE.
        String new_ssid = startWiFiBegin();

        if (waitForConnection(WIFI_CONNECT_TIMEOUT_MS)) {
            Serial.printf("[network] WiFi connesso con nuove credenziali (SSID=%s).\n",
                          new_ssid.c_str());
            ThingSpeak.begin(client);
            return true;
        }

        Serial.println("[network] ERRORE: connessione fallita anche dopo re-provisioning.");
#endif

        return false;
    }

    int send_via_wifi(DataPacket data) {
        if (!wifi_connected()) {
            Serial.println("[network] Error: WiFi not connected!");
            return -1;
        }

        ThingSpeak.setField(1, data.mq135Raw);
        ThingSpeak.setField(2, data.mq135Ratio);
        ThingSpeak.setField(3, data.dht22Temp);
        ThingSpeak.setField(4, data.dht22Hum);
        ThingSpeak.setField(5, data.spoilage_status);

        ThingSpeak.setField(8, data.inference_time_us);

        return ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_API);
    }
}
