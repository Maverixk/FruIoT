#ifndef THINGSBOARD_H
#define THINGSBOARD_H

#include <Arduino.h>
#include "sensors/sensors.h"

namespace thingsboard {

    // Inizializza il modulo: carica l'Access Token da NVS (fallback a secrets.h
    // se NVS è vuoto). Va chiamato una volta nel setup, dopo network::await_wifi().
    void init();

    // Pubblica un alert su ThingsBoard via MQTT/TLS.
    // Pubblica SOLO se status >= 1 (mature o ruined). Per status == 0 (unripe)
    // ritorna immediatamente senza connettersi.
    //
    // Ritorna true se la pubblicazione è andata a buon fine.
    bool publish_alert(int status, sensors::SensorData data);

    // Forza la riconfigurazione del token (svuota NVS, al prossimo init() rilegge
    // da secrets.h). Utile per cambiare device su ThingsBoard.
    void clearToken();

} // namespace thingsboard

#endif // THINGSBOARD_H
