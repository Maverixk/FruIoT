#include "provisioning.h"
#include "config.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include "../display/display.h"

namespace provisioning {

    // ─────────────────────────────────────────────────────────────────────────
    // Costanti di provisioning
    // ─────────────────────────────────────────────────────────────────────────
    static const char* NVS_NS         = "wifi_creds";     // namespace NVS dedicato
    static const char* NVS_KEY_SSID   = "ssid";
    static const char* NVS_KEY_PASS   = "pass";
    static const char* NVS_KEY_POP    = "pop";            // PoP unico per device

    static const char* PROV_DEVICE_NAME = "PROV_FruIoT";  // nome BLE del device
    static const char* PROV_SERVICE_KEY = NULL;           // no service key
    static const uint8_t PROV_UUID[16]  = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b,
                                          0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03,
                                          0x04, 0x90, 0x1a, 0x02};

    // Buffer del PoP corrente, popolato da getOrGeneratePoP()
    static char s_pop[POP_LENGTH + 1] = {0};

    // Flag globale settato dalle callback BLE quando il provisioning ha successo
    static volatile bool s_provisioning_done   = false;
    static volatile bool s_provisioning_failed = false;

    // ─────────────────────────────────────────────────────────────────────────
    // Callback eventi WiFi/Provisioning
    // ─────────────────────────────────────────────────────────────────────────
    static void sysProvEvent(arduino_event_t* sys_event) {
        switch (sys_event->event_id) {
            case ARDUINO_EVENT_PROV_START:
                Serial.println("[prov] BLE Provisioning avviato. Apri l'app 'ESP BLE Provisioning'.");
                break;

            case ARDUINO_EVENT_PROV_CRED_RECV:
                Serial.println("[prov] Credenziali WiFi ricevute dall'app.");
                Serial.printf("[prov]   SSID: %s\n",
                              (const char*)sys_event->event_info.prov_cred_recv.ssid);
                break;

            case ARDUINO_EVENT_PROV_CRED_FAIL:
                Serial.println("[prov] ERRORE: credenziali WiFi non valide (autenticazione fallita).");
                s_provisioning_failed = true;
                break;

            case ARDUINO_EVENT_PROV_CRED_SUCCESS:
                Serial.println("[prov] Credenziali WiFi valide.");
                break;

            case ARDUINO_EVENT_PROV_END:
                Serial.println("[prov] Provisioning completato.");
                s_provisioning_done = true;
                break;

            default:
                break;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Persistenza in NVS
    // ─────────────────────────────────────────────────────────────────────────
    static bool loadFromNVS(String& ssid, String& pass) {
        Preferences prefs;
        prefs.begin(NVS_NS, true);  // read-only
        bool has = prefs.isKey(NVS_KEY_SSID) && prefs.isKey(NVS_KEY_PASS);
        if (has) {
            ssid = prefs.getString(NVS_KEY_SSID, "");
            pass = prefs.getString(NVS_KEY_PASS, "");
            has = ssid.length() > 0;
        }
        prefs.end();
        return has;
    }

    static void saveToNVS(const String& ssid, const String& pass) {
        Preferences prefs;
        prefs.begin(NVS_NS, false);
        prefs.putString(NVS_KEY_SSID, ssid);
        prefs.putString(NVS_KEY_PASS, pass);
        prefs.end();
        Serial.printf("[prov] Credenziali salvate in NVS (SSID=%s).\n", ssid.c_str());
    }

    // ─────────────────────────────────────────────────────────────────────────
    // PoP (Proof of Possession): generato randomicamente al primo boot e
    // persistito in NVS. Ogni device avrà quindi un PoP unico, riducendo
    // l'efficacia di brute force contro la flotta di device.
    //
    // 12 caratteri alfanumerici → 62^12 ≈ 3.2 × 10^21 combinazioni.
    // Anche con tentativi a 1ms ciascuno, brute force richiederebbe ~10^11 anni.
    // ─────────────────────────────────────────────────────────────────────────
    static const char* getOrGeneratePoP() {
        if (s_pop[0] != '\0') return s_pop;  // già caricato in memoria

        Preferences prefs;
        prefs.begin(NVS_NS, false);

        if (prefs.isKey(NVS_KEY_POP)) {
            // Carico PoP esistente
            String stored = prefs.getString(NVS_KEY_POP, "");
            strncpy(s_pop, stored.c_str(), POP_LENGTH);
            s_pop[POP_LENGTH] = '\0';
            Serial.printf("[prov] PoP caricato da NVS: %s\n", s_pop);
        } else {
            // Genero un nuovo PoP random usando esp_random() (hardware TRNG)
            static const char alphabet[] =
                "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789";
            const int alphabet_len = sizeof(alphabet) - 1;
            for (int i = 0; i < POP_LENGTH; i++) {
                s_pop[i] = alphabet[esp_random() % alphabet_len];
            }
            s_pop[POP_LENGTH] = '\0';
            prefs.putString(NVS_KEY_POP, s_pop);
            Serial.printf("[prov] Nuovo PoP generato e salvato: %s\n", s_pop);
        }

        prefs.end();
        return s_pop;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // API pubbliche
    // ─────────────────────────────────────────────────────────────────────────
    bool hasStoredCredentials() {
        String s, p;
        return loadFromNVS(s, p);
    }

    // Cancella SSID, password E il PoP dal namespace NVS.
    // Conseguenza: al prossimo provisioning verrà generato un nuovo PoP random,
    // ruotando di fatto il segreto. Questo aumenta la resilienza in caso di
    // compromissione del PoP precedente (es. fotografato, social engineering).
    void clearWiFiCredentials() {
        Preferences prefs;
        prefs.begin(NVS_NS, false);
        prefs.clear();  // rimuove TUTTE le chiavi del namespace: ssid, pass, pop
        prefs.end();
        // Resetto anche il buffer in RAM per forzare il reload da NVS al prossimo uso
        s_pop[0] = '\0';
        Serial.println("[prov] Credenziali WiFi e PoP cancellati da NVS.");
        Serial.println("[prov] Al prossimo provisioning verrà generato un nuovo PoP.");
    }

    bool getWiFiCredentials(String& ssid_out, String& pass_out) {
        // --- 1. Provo a leggere da NVS ---
        if (loadFromNVS(ssid_out, pass_out)) {
            Serial.printf("[prov] Credenziali caricate da NVS (SSID=%s).\n", ssid_out.c_str());
            return true;
        }

        // --- 2. NVS vuota → avvio BLE Provisioning ---
        Serial.println("[prov] Nessuna credenziale in NVS. Avvio BLE Provisioning...");

        // Recupero o genero il PoP unico per questo device
        const char* pop = getOrGeneratePoP();

        Serial.printf("[prov] Device name: %s\n", PROV_DEVICE_NAME);
        Serial.printf("[prov] Proof of Possession: %s\n", pop);

        s_provisioning_done   = false;
        s_provisioning_failed = false;

        WiFi.onEvent(sysProvEvent);

        WiFiProv.beginProvision(
            WIFI_PROV_SCHEME_BLE,
            WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
            WIFI_PROV_SECURITY_1,
            pop,
            PROV_DEVICE_NAME,
            PROV_SERVICE_KEY,
            (uint8_t*)PROV_UUID,
            true  // reset_provisioned = true (utile in fase di test)
        );

        // Stampa QR code sul seriale (utile in fase di sviluppo)
        WiFiProv.printQR(PROV_DEVICE_NAME, pop, "ble");

        // Mostra il QR code anche sul display OLED (utile a batteria, senza PC)
        display::showProvisioningQR(PROV_DEVICE_NAME, pop);

        // Backup: parametri per inserimento manuale nell'app
        Serial.println("[prov] In alternativa al QR, inserisci manualmente nell'app:");
        Serial.printf("[prov]   Device: %s\n", PROV_DEVICE_NAME);
        Serial.printf("[prov]   PoP: %s\n", pop);

        // --- 3. Attendo che il provisioning finisca (bloccante) ---
        Serial.printf("[prov] Attesa configurazione da app (timeout %lu s)...\n",
                      BLE_PROVISIONING_TIMEOUT_MS / 1000UL);
        unsigned long start = millis();

        while (!s_provisioning_done && !s_provisioning_failed) {
            if (millis() - start > BLE_PROVISIONING_TIMEOUT_MS) {
                Serial.println("[prov] ERRORE: timeout provisioning. Modalità BLE chiusa.");
                display::showMessage("FruIoT", "Provisioning", "timeout");
                return false;
            }
            delay(500);
        }

        if (s_provisioning_failed) {
            Serial.println("[prov] ERRORE: provisioning fallito.");
            return false;
        }

        // --- 4. Estraggo le credenziali appena ricevute ---
        wifi_config_t conf;
        if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) {
            Serial.println("[prov] ERRORE: impossibile leggere wifi config post-provisioning.");
            return false;
        }
        ssid_out = String((const char*)conf.sta.ssid);
        pass_out = String((const char*)conf.sta.password);

        // --- 5. Salvo in NVS per i boot successivi ---
        saveToNVS(ssid_out, pass_out);

        // --- 6. Aggiorno il display: il QR non serve più ---
        display::showMessage("FruIoT", "WiFi configurato", ssid_out);

        return true;
    }

} // namespace provisioning
