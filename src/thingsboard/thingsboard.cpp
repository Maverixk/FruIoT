#include "thingsboard.h"
#include "../../include/secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>

namespace thingsboard {

    // ─────────────────────────────────────────────────────────────────────────
    // Costanti
    // ─────────────────────────────────────────────────────────────────────────
    static const char* NVS_NS         = "thingsboard";   // namespace NVS dedicato
    static const char* NVS_KEY_TOKEN  = "access_token";

    // Topic standard ThingsBoard per telemetria
    static const char* TB_TOPIC_TELEMETRY = "v1/devices/me/telemetry";

    // ─────────────────────────────────────────────────────────────────────────
    // ISRG Root X2 CA (Let's Encrypt root ECDSA, valida fino al 2040).
    //
    // ThingsBoard Cloud usa certificati Let's Encrypt firmati dall'intermediate
    // R13 (ECDSA), che ha come root ISRG Root X2.
    // Verificato dalla catena: thingsboard.cloud → R13 → ISRG Root X2.
    // ─────────────────────────────────────────────────────────────────────────
    static const char* TB_ROOT_CA PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
)EOF";

    // ─────────────────────────────────────────────────────────────────────────
    // Stato modulo
    // ─────────────────────────────────────────────────────────────────────────
    static String s_access_token;
    static WiFiClientSecure s_tls_client;
    static PubSubClient s_mqtt(s_tls_client);
    static bool s_initialized = false;

    // ─────────────────────────────────────────────────────────────────────────
    // Persistenza token in NVS
    // ─────────────────────────────────────────────────────────────────────────
    static String loadTokenFromNVS() {
        Preferences prefs;
        prefs.begin(NVS_NS, true);
        String t = prefs.getString(NVS_KEY_TOKEN, "");
        prefs.end();
        return t;
    }

    static void saveTokenToNVS(const String& token) {
        Preferences prefs;
        prefs.begin(NVS_NS, false);
        prefs.putString(NVS_KEY_TOKEN, token);
        prefs.end();
        Serial.println("[thingsboard] Access Token salvato in NVS.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // API pubbliche
    // ─────────────────────────────────────────────────────────────────────────
    void init() {
        // 1. Provo a caricare il token da NVS
        s_access_token = loadTokenFromNVS();

        // 2. Se NVS è vuoto, fallback a secrets.h e salvo per i boot successivi
        if (s_access_token.length() == 0) {
            Serial.println("[thingsboard] Token assente in NVS, carico da secrets.h.");
            s_access_token = String(TB_ACCESS_TOKEN);
            if (s_access_token.length() > 0 &&
                s_access_token != "YOUR_THINGSBOARD_ACCESS_TOKEN") {
                saveTokenToNVS(s_access_token);
            } else {
                Serial.println("[thingsboard] ATTENZIONE: TB_ACCESS_TOKEN non configurato in secrets.h.");
                return;
            }
        } else {
            Serial.println("[thingsboard] Token caricato da NVS.");
        }

        // 3. Configuro il client TLS con il CA root corretto (ISRG Root X2).
        // ThingsBoard Cloud usa la catena: server cert → R13 (Let's Encrypt
        // ECDSA intermediate) → ISRG Root X2. Embeddiamo la root nel firmware
        // per verificare l'autenticità del server e prevenire MITM.
        s_tls_client.setCACert(TB_ROOT_CA);
        s_mqtt.setServer(TB_HOST, TB_PORT);
        s_mqtt.setBufferSize(512);  // payload può crescere, riserva buffer adeguato

        s_initialized = true;
        Serial.printf("[thingsboard] Modulo inizializzato (host=%s:%d).\n", TB_HOST, TB_PORT);
    }

    bool publish_alert(int status, sensors::SensorData data) {
        // 1. Pubblica SOLO per stati critici (matura o marcia)
        if (status < 1) {
            return false;  // status == 0 (unripe) → nessuna notifica
        }

        if (!s_initialized) {
            Serial.println("[thingsboard] ERRORE: modulo non inizializzato.");
            return false;
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[thingsboard] ERRORE: WiFi non connesso.");
            return false;
        }

        // 2. Connessione MQTT (single-shot: connetti, pubblica, disconnetti)
        Serial.printf("[thingsboard] Connessione a %s:%d...\n", TB_HOST, TB_PORT);
        // Client ID: usa un identificativo univoco. Il token viene passato come
        // username MQTT secondo la convenzione ThingsBoard. Password vuota.
        if (!s_mqtt.connect("fruiot_device", s_access_token.c_str(), "")) {
            Serial.printf("[thingsboard] ERRORE connessione MQTT: state=%d\n", s_mqtt.state());
            return false;
        }
        Serial.println("[thingsboard] Connesso a ThingsBoard.");

        // 3. Costruzione payload JSON minimale (solo la prediction)
        char payload[64];
        const char* status_label = (status == 1) ? "mature" : "ruined";
        snprintf(payload, sizeof(payload),
                 "{\"status\":%d,\"status_label\":\"%s\"}",
                 status, status_label);

        // 4. Pubblicazione
        bool ok = s_mqtt.publish(TB_TOPIC_TELEMETRY, payload);
        if (ok) {
            Serial.printf("[thingsboard] Alert pubblicato (status=%s): %s\n",
                          status_label, payload);
        } else {
            Serial.println("[thingsboard] ERRORE pubblicazione MQTT.");
        }

        // 5. Disconnessione immediata per risparmiare batteria
        s_mqtt.disconnect();

        return ok;
    }

    void clearToken() {
        Preferences prefs;
        prefs.begin(NVS_NS, false);
        prefs.clear();
        prefs.end();
        s_access_token = "";
        s_initialized = false;
        Serial.println("[thingsboard] Access Token cancellato da NVS.");
    }

} // namespace thingsboard
