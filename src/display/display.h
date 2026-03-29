#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

namespace display {

    // Inizializza il display OLED della Heltec V3
    void init();

    // Mostra un messaggio di stato generico sul display
    void showMessage(const String& line1, const String& line2 = "", const String& line3 = "");

    // Mostra i dati dei sensori sul display
    void showSensorData(float temp, float humidity, float gasPPM, float batteryV);

    // Spegne il display (prima del deep sleep)
    void off();

}

#endif // DISPLAY_H
