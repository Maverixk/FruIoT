#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

namespace network {

    struct DataPacket {
        float mq135CO2;
        float dht22Temp;
        float dht22Hum;
    } ;

    // Sets up Wifi connection with 
    void init();

    // Checks if the Wifi is connected
    bool wifi_connected();

    // Checks if the Wifi connection is active
    void connect_to_wifi();

    // Sends data to ThingSpeak server via Wifi
    int send_via_wifi(DataPacket data);

}

#endif // NETWORK_H
