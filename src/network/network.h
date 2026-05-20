#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

namespace network {

    struct DataPacket {
        int   mq135Raw;
        float mq135Ratio;
        float dht22Temp;
        float dht22Hum;
        int   spoilage_status;
        float warmup_current;
        float polling_current;
        float inference_time_us;
    };

    void init();
    bool wifi_connected();
    bool await_wifi(int timeout_ms = 10000);
    int send_via_wifi(DataPacket data);

}

#endif // NETWORK_H
