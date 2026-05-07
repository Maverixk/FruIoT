#include "display.h"
#include "heltec.h"

namespace display {

    void init() {
        // Il display OLED deve essere inizializzato tramite Heltec.begin()
        // che verrà chiamato nel main.
        Heltec.display->clear();
        Heltec.display->setFont(ArialMT_Plain_10);
        Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
        Heltec.display->display();
    }

    void showMessage(const String& line1, const String& line2, const String& line3) {
        Heltec.display->clear();
        Heltec.display->setFont(ArialMT_Plain_16);
        Heltec.display->drawString(0, 0, line1);

        if (line2.length() > 0) {
            Heltec.display->setFont(ArialMT_Plain_10);
            Heltec.display->drawString(0, 22, line2);
        }
        if (line3.length() > 0) {
            Heltec.display->setFont(ArialMT_Plain_10);
            Heltec.display->drawString(0, 38, line3);
        }

        Heltec.display->display();
    }

    void showSensorData(float temp, float humidity, float ratio) {
        Heltec.display->clear();

        Heltec.display->setFont(ArialMT_Plain_16);
        Heltec.display->drawString(0, 0, "FruIoT Monitor");

        Heltec.display->setFont(ArialMT_Plain_10);
        Heltec.display->drawString(0, 20, "Temp:  " + String(temp, 1) + " C");
        Heltec.display->drawString(0, 32, "Hum:   " + String(humidity, 1) + " %");
        Heltec.display->drawString(0, 44, "Ratio: " + String(ratio, 2));

        Heltec.display->display();
    }

    void off() {
        Heltec.display->clear();
        Heltec.display->display();
        Heltec.display->displayOff();
    }

}
