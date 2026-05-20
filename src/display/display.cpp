#include "display.h"
#include "heltec.h"
extern "C" {
    #include "qrcode.h"  // esp_qrcode dal framework Arduino-ESP32
}

namespace display {

    // ─────────────────────────────────────────────────────────────────────────
    // Callback per esp_qrcode_generate: chiamata una volta dopo che il QR è
    // stato generato. Riceve l'handle, dal quale leggiamo size e moduli.
    // ─────────────────────────────────────────────────────────────────────────
    static void renderQRtoOLED(esp_qrcode_handle_t qrcode) {
        const int size = esp_qrcode_get_size(qrcode);

        // Calcolo automatico dei pixel per modulo per stare nei 64x64 px disponibili
        // (lato sinistro del display 128x64). Lascia 2 px di margine.
        const int max_size_px = 60;
        const int px_per_module = max(1, max_size_px / size);
        const int offset_x = 2;
        const int offset_y = 2;

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                if (esp_qrcode_get_module(qrcode, x, y)) {
                    for (int dy = 0; dy < px_per_module; dy++) {
                        for (int dx = 0; dx < px_per_module; dx++) {
                            Heltec.display->setPixel(offset_x + x * px_per_module + dx,
                                                     offset_y + y * px_per_module + dy);
                        }
                    }
                }
            }
        }
        Heltec.display->display();
    }


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

    // ─────────────────────────────────────────────────────────────────────────
    // QR code di provisioning sull'OLED 128x64.
    //
    // Display Heltec V3: 128 wide × 64 tall pixel.
    // Renderizziamo il QR sulla sinistra (max 64x64 px → 2 px per modulo,
    // QR Version 3 ha 29x29 moduli → occupa 58x58 px). Sulla destra mostriamo
    // il PoP in chiaro per facilitare l'inserimento manuale in caso il QR
    // non venga letto correttamente.
    // ─────────────────────────────────────────────────────────────────────────
    void showProvisioningQR(const String& device_name, const String& pop) {
        // Costruisci il payload nel formato richiesto dall'app ESP BLE Provisioning
        String payload = "{\"ver\":\"v1\",\"name\":\"";
        payload += device_name;
        payload += "\",\"pop\":\"";
        payload += pop;
        payload += "\",\"transport\":\"ble\"}";

        // Prepara display per il rendering
        Heltec.display->clear();

        // Testo accanto al QR (lato destro del display), va settato prima
        // perché renderQRtoOLED chiama display() alla fine
        Heltec.display->setFont(ArialMT_Plain_10);
        Heltec.display->drawString(66, 0,  "Setup");
        Heltec.display->drawString(66, 14, "WiFi:");
        Heltec.display->drawString(66, 26, "scan QR");
        Heltec.display->drawString(66, 40, "PoP:");
        Heltec.display->drawString(66, 52, pop);

        // Genera e renderizza il QR sull'OLED tramite callback
        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
        cfg.display_func = renderQRtoOLED;
        cfg.max_qrcode_version = 5;  // versioni maggiori per payload più lunghi
        esp_qrcode_generate(&cfg, payload.c_str());
    }

}
