#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <Arduino.h>

namespace provisioning {

    // Carica credenziali WiFi: prima da NVS, poi (se assenti) da BLE provisioning.
    // Riempie ssid_out e pass_out e ritorna true se ok.
    //
    // Se NVS non contiene credenziali, avvia ESP BLE Provisioning e blocca
    // l'esecuzione finché l'utente non completa il setup dal telefono.
    bool getWiFiCredentials(String& ssid_out, String& pass_out);

    // Cancella le credenziali WiFi da NVS (utile per forzare re-provisioning).
    void clearWiFiCredentials();

    // Ritorna true se sono state trovate credenziali WiFi salvate in NVS.
    bool hasStoredCredentials();

} // namespace provisioning

#endif // PROVISIONING_H
