#include "network.h"
#include <WiFi.h>
#include "ThingSpeak.h"
#include "../include/secrets.h"
#include "config.h"

namespace network {

    // Usiamo un puntatore o lo inizializziamo con cautela
    static WiFiClient client; 

    void init() {
        WiFi.mode(WIFI_STA);
        ThingSpeak.begin(client); 
    }

    bool wifi_connected(){
        return (WiFi.status() == WL_CONNECTED);
    }

    void connect_to_wifi() {
        if (WiFi.status() != WL_CONNECTED){
            Serial.print("[network] Connecting to WiFi...");
            WiFi.begin(WIFI_SSID_LUCA, WIFI_PASS_LUCA);
            
            int attempt = 0;
            while (WiFi.status() != WL_CONNECTED && attempt < 20) {
                delay(500);
                Serial.print(".");
                attempt++;
            }
            Serial.println(""); 
        }
        
        // Relinking the client after every wake up
        ThingSpeak.begin(client); 
    }

    int send_via_wifi(DataPacket data) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[network] Error: WiFi not connected!");
            return -1;
        }
        
        ThingSpeak.setField(1, data.mq135Raw);
        ThingSpeak.setField(2, data.mq135Ratio);
        ThingSpeak.setField(3, data.dht22Temp);
        ThingSpeak.setField(4, data.dht22Hum);
        #if CURRENT_MONITOR == 1
            ThingSpeak.setField(5, data.warmup_current);
            ThingSpeak.setField(6, data.polling_current);
        #endif

        return ThingSpeak.writeFields(SECRET_CH_ID_LUCA, SECRET_WRITE_APIKEY_LUCA);
    }
}
