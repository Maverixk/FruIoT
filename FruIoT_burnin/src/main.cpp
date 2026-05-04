#include <Arduino.h>
#include "heltec.h"

#define BURNIN_HOURS 24

static void updateDisplay(int ore, int minuti, int secondi) {
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawString(64, 0, "MQ135 Burn-in");

    Heltec.display->setFont(ArialMT_Plain_24);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", ore, minuti, secondi);
    Heltec.display->drawString(64, 22, buf);

    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(64, 52, "obiettivo: 24h");

    Heltec.display->display();
}

void setup() {
    Heltec.begin(true, false, true);
    Serial.println("=== BURN-IN MQ135 ===");
    updateDisplay(0, 0, 0);
}

void loop() {
    unsigned long elapsed_s = millis() / 1000;
    int ore     = elapsed_s / 3600;
    int minuti  = (elapsed_s % 3600) / 60;
    int secondi = elapsed_s % 60;

    updateDisplay(ore, minuti, secondi);

    if (ore >= BURNIN_HOURS) {
        Heltec.display->clear();
        Heltec.display->setFont(ArialMT_Plain_16);
        Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
        Heltec.display->drawString(64, 20, "COMPLETATO!");
        Heltec.display->drawString(64, 42, "24h raggiunte");
        Heltec.display->display();
        Serial.println("=== BURN-IN COMPLETATO ===");
        while (true) { delay(10000); }
    }

    delay(1800000); // aggiornamento ogni 30 minuti
}
