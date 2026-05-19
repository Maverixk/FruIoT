#include "network.h"
#include <WiFi.h>
#include "ThingSpeak.h"
#include "../include/secrets.h"
#include "config.h"

namespace network {

    static WiFiClient client;

    void init() {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        Serial.println("[network] WiFi avviato in background.");
    }

    bool wifi_connected() {
        return (WiFi.status() == WL_CONNECTED);
    }

    bool await_wifi(int timeout_ms) {
        if (wifi_connected()) {
            Serial.println("[network] WiFi gia' connesso.");
            ThingSpeak.begin(client);
            return true;
        }

        Serial.print("[network] Attesa connessione WiFi...");
        unsigned long start = millis();
        while (!wifi_connected() && (millis() - start) < (unsigned long)timeout_ms) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("");

        if (wifi_connected()) {
            Serial.println("[network] WiFi connesso.");
            ThingSpeak.begin(client);
            return true;
        }

        Serial.println("[network] ERRORE: timeout connessione WiFi.");
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

        return ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_API);
    }
}
