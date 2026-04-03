// dht22.cpp
#include "dht22.h"
#include "config.h"
#include <DHT.h>

static DHT dht(DHT_PIN, DHT_TYPE);

namespace dht22 {

void init() {
    dht.begin();
    Serial.println("[DHT22] Pronto.");
}

Data poll() {
    // aspettare almeno 2 secondi tra una lettura e l'altra
    Data data{};
    data.temperatureC = dht.readTemperature();
    data.humidityPct  = dht.readHumidity();
    // isnan() controlla se la lettura è fallita (sensore non collegato o timeout)
    data.ok = !isnan(data.temperatureC) && !isnan(data.humidityPct);
    if (!data.ok) Serial.println("[DHT22] ERRORE: lettura fallita.");
    return data;
}

} // namespace dht22