#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

namespace display {

    // Inizializza il display OLED della Heltec V3
    void init();

    // Mostra un messaggio di stato generico sul display
    void showMessage(const String& line1, const String& line2 = "", const String& line3 = "");

    // Mostra i dati dei sensori sul display
    void showSensorData(float temp, float humidity, float ratio);

    // Mostra un QR code di provisioning sul display.
    // Il payload deve essere la stringa che l'app ESP BLE Provisioning si aspetta:
    // {"ver":"v1","name":"PROV_FruIoT","pop":"fruiot1234","transport":"ble"}
    void showProvisioningQR(const String& device_name, const String& pop);

    // Spegne il display (prima del deep sleep)
    void off();

}

#endif // DISPLAY_H
