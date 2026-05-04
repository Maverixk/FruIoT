#include "mq135.h"
#include "config.h"
#include "power/power.h"
#include <Arduino.h>
#include <Preferences.h>

namespace mq135 {

    // --- Stato interno (vive fino al deep sleep) ---
    static float       s_r0          = -1.0f;  // R0 calibrato (kΩ), -1 = non disponibile
    static bool        s_r0_from_nvs = false; // true se R0 letto da NVS
    static Preferences s_prefs;

    // Buffer per campioni di corrente INA219 durante warmup e polling
    const int max_samples = 100;
    float warmup_current_samples[max_samples];
    float polling_current_samples[max_samples];
    int warmup_num_samples = 0;
    int polling_num_samples = 0;

    // ─────────────────────────────────────────────────────────────────────────
    // Filtro a mediana: raccoglie N campioni ADC, li ordina e restituisce
    // il valore centrale. Elimina spike e outlier meglio di una media.
    // ─────────────────────────────────────────────────────────────────────────
    static int readMedianAnalog(uint8_t pin) {
        int buf[MQ135_MEDIAN_WINDOW];

        for (int i = 0; i < MQ135_MEDIAN_WINDOW; i++) {
            buf[i] = analogRead(pin);
            delay(MQ135_SAMPLE_DELAY_MS); // il condensatore sample-and-hold dell'ADC necessita di tempo per stabilizzarsi tra letture consecutive
        }

        // Insertion sort (efficiente per array piccoli)
        for (int i = 1; i < MQ135_MEDIAN_WINDOW; i++) {
            int key = buf[i];
            int j = i - 1;
            while (j >= 0 && buf[j] > key) {
                buf[j + 1] = buf[j];
                j--;
            }
            buf[j + 1] = key;
        }

        return buf[MQ135_MEDIAN_WINDOW / 2];
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Converte raw ADC → resistenza RS del sensore (kΩ).
    //
    // Schema elettrico:
    //   5V → [RS (sensore)] → [RL 1kΩ (sul modulo)] → GND
    //   Il pin AO del modulo legge la tensione su RL: Vao = VCC * RL / (RS + RL)
    //   Tra AO e GPIO dell'ESP c'è un partitore 100k/100k → Vadc = Vao / 2
    //
    // Quindi: Vsensor = Vadc * 2, poi RS = RL * (VCC - Vsensor) / Vsensor
    // ─────────────────────────────────────────────────────────────────────────
    static float computeRS(int raw) {
        float vadc    = (static_cast<float>(raw) / ADC_MAX_COUNTS) * ADC_REF_VOLTAGE;
        float vsensor = vadc * MQ135_DIVIDER_RATIO; // ricostruisce tensione reale su AO
        if (vsensor <= 0.001f) return 9999.0f;      // protezione divisione per zero
        return MQ135_RL_KOHM * (MQ135_VCC - vsensor) / vsensor;
    }

    // --- Persistenza R0 in NVS (sopravvive a deep sleep e riavvii) ---

    static bool loadR0fromNVS() {
        s_prefs.begin(NVS_NAMESPACE, true); // true = sola lettura
        bool exists = s_prefs.isKey(NVS_KEY_R0);
        float stored = exists ? s_prefs.getFloat(NVS_KEY_R0, -1.0f) : -1.0f;
        s_prefs.end();
        if (stored > 0.0f) {
            s_r0          = stored;
            s_r0_from_nvs = true;
            return true;
        }
        return false;
    }

    static void saveR0toNVS(float r0) {
        s_prefs.begin(NVS_NAMESPACE, false); // false = lettura/scrittura
        s_prefs.putFloat(NVS_KEY_R0, r0);
        s_prefs.end();
        Serial.printf("[MQ135] R0 salvato in NVS: %.2f kΩ\n", r0);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Warm-up: mantiene il sensore acceso per duration_ms.
    // L'elemento riscaldante dell'MQ135 deve raggiungere la temperatura
    // operativa prima che le letture siano affidabili.
    // Durante l'attesa campiona la corrente dall'INA219 ogni 5 secondi.
    // ─────────────────────────────────────────────────────────────────────────
    static void doWarmup(unsigned long duration_ms) {
        if (duration_ms == 0) return;
        Serial.printf("[MQ135] Warm-up: %lu secondi...\n", duration_ms / 1000);
        unsigned long start      = millis();
        unsigned long last_print = 0;
        while (millis() - start < duration_ms) {
            if (millis() - last_print >= 5000) {
                unsigned long remaining = (duration_ms - (millis() - start)) / 1000;
                Serial.printf("[MQ135]   %lu s rimanenti\n", remaining);
                last_print = millis();

                power::PowerData pwr = power::readINA219();
                if (pwr.ok && warmup_num_samples < max_samples) {
                    warmup_current_samples[warmup_num_samples++] = pwr.current_mA;
                }
            }
            delay(500);
        }
        Serial.println("[MQ135] Warm-up completato.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Calibrazione R0 in aria pulita.
    // Fa MQ135_CALIB_SAMPLES letture mediane, calcola la media delle RS.
    // R0 = RS medio in aria pulita (senza assumere un valore fisso di CO2).
    // Il valore viene salvato in NVS per i cicli successivi.
    // ─────────────────────────────────────────────────────────────────────────
    static void calibrateR0() {
        Serial.println("[MQ135] Calibrazione R0 in corso (aria pulita)...");

        float sum = 0.0f;
        for (int i = 0; i < MQ135_CALIB_SAMPLES; i++) {
            int raw = readMedianAnalog(MQ135_PIN);
            float rs = computeRS(raw);
            sum += rs;
            Serial.printf("[MQ135]   campione %d/%d — raw=%d rs=%.3f kΩ\n",
                          i + 1, MQ135_CALIB_SAMPLES, raw, rs);
            delay(MQ135_CALIB_DELAY_MS);
        }

        s_r0 = sum / MQ135_CALIB_SAMPLES;
        Serial.printf("[MQ135] R0 = %.2f kΩ (media di %d campioni in aria pulita)\n",
                      s_r0, MQ135_CALIB_SAMPLES);
        saveR0toNVS(s_r0);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // API pubblica
    // ─────────────────────────────────────────────────────────────────────────

    // Configura ADC, carica R0 da NVS, gestisce warm-up e calibrazione.
    // Se NVS è vuoto → warmup completo + calibrazione automatica.
    // Se NVS ha R0   → applica la strategia di warmup scelta in config.h.
    void init() {
        analogReadResolution(12); // 0–4095
        #ifdef ARDUINO_ARCH_ESP32
            analogSetAttenuation(ADC_11db); // range 0–3.3V
        #endif
        pinMode(MQ135_PIN, INPUT);

        bool nvs_ok = loadR0fromNVS();
        if (!nvs_ok) {
            Serial.println("[MQ135] R0 non trovato in NVS — calibrazione necessaria.");
            Serial.println("[MQ135] *** Assicurarsi che il sensore sia in aria pulita! ***");
            doWarmup(MQ135_WARMUP_FULL_MS);
            calibrateR0();
            Serial.println("[MQ135] Pronto.");
            return;
        }

        Serial.printf("[MQ135] R0 da NVS: %.2f kΩ\n", s_r0);

        switch (MQ135_WARMUP_STRATEGY) {
            case WARMUP_FULL:
                doWarmup(MQ135_WARMUP_FULL_MS);
                break;
            case WARMUP_SHORT:
                doWarmup(MQ135_WARMUP_SHORT_MS);
                break;
            case WARMUP_SKIP:
                Serial.println("[MQ135] Warm-up saltato.");
                break;
        }

        Serial.println("[MQ135] Pronto.");
    }

    // Legge il sensore: raw ADC (mediana) → RS → ratio RS/R0.
    // Campiona anche la corrente INA219 durante la lettura.
    Data poll() {
        Data data{};

        if (s_r0 <= 0.0f) {
            Serial.println("[MQ135] ERRORE: R0 non disponibile. Chiamare init() prima.");
            data.ok = false;
            return data;
        }

        int   raw   = readMedianAnalog(MQ135_PIN);
        float rs    = computeRS(raw);
        float ratio = rs / s_r0; // >1 aria pulita, <1 presenza di gas

        data.raw   = raw;
        data.rs    = rs;
        data.ratio = ratio;
        data.ok    = true;

        Serial.printf("[MQ135] raw=%d rs=%.3f kΩ ratio=%.5f r0=%.3f kΩ\n",
                      raw, rs, ratio, s_r0);

        power::PowerData pwr = power::readINA219();
        if (pwr.ok && polling_num_samples < max_samples) {
            polling_current_samples[polling_num_samples++] = pwr.current_mA;
        }

        return data;
    }

    // Cancella R0 da NVS → al prossimo boot farà calibrazione completa
    void resetCalibration() {
        s_prefs.begin(NVS_NAMESPACE, false);
        s_prefs.remove(NVS_KEY_R0);
        s_prefs.end();
        s_r0          = -1.0f;
        s_r0_from_nvs = false;
        Serial.println("[MQ135] NVS R0 cancellato. Ricalibrazione al prossimo avvio.");
    }

    // Scrive un R0 manuale in NVS (utile per test)
    void forceR0(float r0) {
        if (r0 <= 0.0f || isnan(r0) || isinf(r0)) {
            Serial.println("[MQ135] ERRORE: R0 non valido, scrittura NVS annullata.");
            return;
        }
        s_r0 = r0;
        s_r0_from_nvs = true;
        saveR0toNVS(r0);
        Serial.printf("[MQ135] R0 forzato manualmente: %.2f kΩ\n", r0);
    }

    int getWarmupNumSamples() { return warmup_num_samples; }
    int getPollingNumSamples() { return polling_num_samples; }
    float getWarmupCurrentSample(int index) { return warmup_current_samples[index]; }
    float getPollingCurrentSample(int index) { return polling_current_samples[index]; }

} // namespace mq135
