#include "mq135.h"
#include "config.h"
#include <Arduino.h>
#include <Preferences.h>
#include <math.h>

namespace mq135 {

// ─────────────────────────────────────────────────────────────────────────────
// Stato interno del modulo
//
// Queste variabili sopravvivono per tutta la durata del programma (fino al
// deep sleep). Mantengono lo stato del sensore tra le chiamate a poll() e init().
// ─────────────────────────────────────────────────────────────────────────────
static float       s_r0          = -1.0f;  // R0 calibrato (kΩ); -1 = non ancora calibrato
static bool        s_r0_from_nvs = false;  // true se R0 è stato caricato dalla memoria NVS
static Preferences s_prefs;               // interfaccia alla memoria NVS dell'ESP32

// ─────────────────────────────────────────────────────────────────────────────
// Funzioni private
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Legge il pin ADC `samples` volte e restituisce la media intera.
 *
 * L'ADC dell'ESP32 è rumoroso: ogni singola lettura può variare di ±50-100
 * counts anche a segnale stabile. Mediare più campioni riduce il rumore
 * statistico e produce un valore più stabile.
 *
 * Il delay(2) tra un campione e l'altro serve a far stabilizzare il
 * condensatore interno del campionatore ADC : senza pausa, letture consecutive
 * si influenzano a vicenda (effetto "crosstalk").
 */
static int readAverageAnalog(uint8_t pin, int samples) {
    uint32_t sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(2); // stabilizzazione condensatore ADC interno
    }
    return static_cast<int>(sum / samples);
}

/**
 * Converte il raw ADC nella resistenza RS del sensore (in kΩ).
 *
 * Il MQ135 è internamente una resistenza variabile (RS) che diminuisce
 * al crescere della concentrazione di gas. Sul modulo AZDelivery è presente
 * una resistenza di carico RL=1kΩ che forma un partitore con RS :
 *
 *   Vao = VCC * RL / (RS + RL)   →   RS = RL * (VCC - Vao) / Vao
 *
 * Poiché tra il pin AO del sensore e il GPIO dell'ESP è presente il partitore
 * resistivo 100kΩ/100kΩ, la tensione misurata dall'ADC è la metà di Vao.
 * Per risalire a Vao moltiplichiamo per DIVIDER_RATIO (= 2).
 *
 * Il guard "vsensor <= 0.001f" evita una divisione per zero se il sensore
 * non è collegato o l'ADC legge 0.
 */
static float computeRS(int raw) {
    float vadc    = (static_cast<float>(raw) / ADC_MAX_COUNTS) * ADC_REF_VOLTAGE;
    float vsensor = vadc * MQ135_DIVIDER_RATIO; // tensione reale su AO prima del partitore
    if (vsensor <= 0.001f) return 9999.0f;
    return MQ135_RL_KOHM * (MQ135_VCC - vsensor) / vsensor;
}

/**
 * Stima la concentrazione di CO2 in ppm dal rapporto RS/R0.
 *
 * Il datasheet del MQ135 fornisce curve RS/R0 vs ppm in scala log-log.
 * Queste curve seguono una legge di potenza: ppm = A * (RS/R0)^B
 *
 * I coefficienti A=56.0820, B=-5.9603 sono stati ricavati da Davide Gironi
 * (2014) tramite regressione MATLAB, correlando l'output raw del MQ135 con
 * le misure di un sensore CO2 NDIR professionale (MHZ14).
 * Fonte: https://davidegironi.blogspot.com/2014/01/cheap-co2-meter-using-mq135-sensor-with.html
 *
 * Nota: B è negativo perché RS/R0 e ppm sono inversamente correlati —
 * più gas c'è, più RS scende, più il rapporto RS/R0 si abbassa.
 */
static float estimateCO2ppm(float ratio) {
    if (ratio <= 0.0f) return 0.0f;
    return MQ135_CO2_A * powf(ratio, MQ135_CO2_B);
}

/**
 * Tenta di caricare R0 dalla memoria NVS (Non-Volatile Storage).
 *
 * La NVS è una piccola area di memoria flash dell'ESP32 che sopravvive
 * ai riavvii e al deep sleep. Salviamo R0 qui dopo la calibrazione così
 * non è necessario ricalibrate ad ogni accensione.
 *
 * Restituisce true se R0 è stato trovato e caricato, false se NVS è vuoto
 * (tipicamente al primo avvio assoluto o dopo resetCalibration()).
 */
static bool loadR0fromNVS() {
    s_prefs.begin(NVS_NAMESPACE, true);
    bool exists = s_prefs.isKey(NVS_KEY_R0);  // ← controlla prima
    float stored = exists ? s_prefs.getFloat(NVS_KEY_R0, -1.0f) : -1.0f;
    s_prefs.end();
    if (stored > 0.0f) {
        s_r0          = stored;
        s_r0_from_nvs = true;
        return true;
    }
    return false;
}

/**
 * Salva R0 in NVS per i riavvii successivi.
 *
 * La scrittura su NVS ha un costo energetico alto, ma siccome viene fatta solo una volta ogni tanti cicli,
 * è un buon compromesso per evitare di ripetere la calibrazione dopo il deep sleep/riavvio.
 */
static void saveR0toNVS(float r0) {
    s_prefs.begin(NVS_NAMESPACE, false); // false = lettura/scrittura
    s_prefs.putFloat(NVS_KEY_R0, r0);
    s_prefs.end();
    Serial.printf("[MQ135] R0 salvato in NVS: %.2f kΩ\n", r0);
}

/**
 * Warm-up bloccante: mantiene il sensore acceso per `duration_ms` millisecondi.
 *
 * Il MQ135 ha un elemento riscaldante interno che deve raggiungere la
 * temperatura operativa prima che le letture siano affidabili.
 *
 * La durata dipende dalla strategia scelta in config.h:
 *   WARMUP_FULL  → 90s (primo avvio, massima precisione)
 *   WARMUP_SHORT → 30s (riavvio da deep sleep, R0 già noto)
 *   WARMUP_SKIP  → 0s  (minimo consumo, precisione ridotta)
 *
 * Il Serial stampa un aggiornamento ogni 5 secondi per non intasare il buffer.
 */
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
        }
        delay(500); // ogni mezzo secondo controlla se è il momento di stampare l'aggiornamento, ma non intasare il buffer seriale con troppe stampe
    }
    // nel caso peggiore il warm-up è già finito da 499ms prima che il loop se ne accorga,
    // ma va bene così: 500ms su 90 secondi è uno scarto dello 0.5%
    Serial.println("[MQ135] Warm-up completato.");
}

/**
 * Calibra R0 misurando RS in aria pulita e lo salva in NVS.
 *
 * R0 è la resistenza del sensore in aria pulita (~420 ppm CO2 atmosferica).
 * È il punto di riferimento: tutti i valori successivi RS/R0 sono relativi
 * a questa baseline. Senza R0 corretto, la stima CO2 è sistematicamente
 * sbagliata.
 *
 * Derivazione matematica:
 *   Dalla formula Gironi: ppm = A * (RS/R0)^B
 *   In aria pulita sappiamo che ppm = CO2_CLEAN_AIR (420 ppm)
 *   Quindi: RS/R0 = (CO2_CLEAN_AIR / A)^(1/B)   →  R0 = RS / (CO2_CLEAN_AIR / A)^(1/B)
 *
 * Per ridurre l'effetto del rumore, RS viene mediato su MQ135_CALIB_SAMPLES
 * misure separate da MQ135_CALIB_DELAY_MS millisecondi.
 *
 * IMPORTANTE : il sensore deve essere in aria pulita (all'aperto o vicino
 * a una finestra aperta) durante tutta la calibrazione.
 */
static void calibrateR0() {
    Serial.println("[MQ135] Calibrazione R0 in corso...");

    float sum = 0.0f;
    for (int i = 0; i < MQ135_CALIB_SAMPLES; i++) {
        sum += computeRS(readAverageAnalog(MQ135_PIN, MQ135_SAMPLES));
        delay(MQ135_CALIB_DELAY_MS);
    }

    float rs_clean     = sum / MQ135_CALIB_SAMPLES;
    float ratio_target = powf(MQ135_CO2_CLEAN_AIR / MQ135_CO2_A, 1.0f / MQ135_CO2_B);
    s_r0               = rs_clean / ratio_target;

    Serial.printf("[MQ135] R0 = %.2f kΩ (CO2 verifica: %.1f ppm)\n",
                  s_r0, estimateCO2ppm(rs_clean / s_r0));

    saveR0toNVS(s_r0);
}

// ─────────────────────────────────────────────────────────────────────────────
// API pubblica
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Inizializza il modulo sensori e gestisce warm-up e calibrazione R0.
 *
 * Questa funzione è bloccante per la durata del warm-up scelto.
 * Deve essere chiamata una sola volta in setup() prima di qualsiasi poll().
 *
 * R0 viene calibrato solo se NVS è vuoto (primo avvio assoluto o dopo resetCalibration()).
 */
void init() {
    analogReadResolution(12); // ESP32-S3: ADC a 12 bit (0–4095)

    #ifdef ARDUINO_ARCH_ESP32
        // ADC_11db = attenuazione massima → range di ingresso 0–3.3V
        // necessario perché il segnale in ingresso all'ESP32 può arrivare fino a ~2.25V per utilizzo partitore
        analogSetAttenuation(ADC_11db);
    #endif

    pinMode(MQ135_PIN, INPUT);

    bool nvs_ok = loadR0fromNVS();
    if (!nvs_ok) {
        Serial.println("[MQ135] *** Assicurarsi che il sensore sia in aria pulita! ***");
        doWarmup(MQ135_WARMUP_FULL_MS); // warm-up completo per stabilizzare il sensore prima della calibrazione
        calibrateR0();
        Serial.println("[MQ135] Pronto.");
        return;
    }

    // Da qui R0 è sempre disponibile — NVS aveva il valore salvato.
    Serial.printf("[MQ135] R0 da NVS: %.2f kΩ\n", s_r0);
    const char* strategy_name =
        (MQ135_WARMUP_STRATEGY == WARMUP_FULL)  ? "WARMUP_FULL  (90s, R0 da NVS)" :
        (MQ135_WARMUP_STRATEGY == WARMUP_SHORT) ? "WARMUP_SHORT (30s, R0 da NVS)" :
                                                  "WARMUP_SKIP  (0s, lettura immediata)";
    Serial.printf("[MQ135] Strategia warm-up: %s\n", strategy_name);

    switch (MQ135_WARMUP_STRATEGY) {
        case WARMUP_FULL:
            doWarmup(MQ135_WARMUP_FULL_MS);
            break;
        case WARMUP_SHORT:
            doWarmup(MQ135_WARMUP_SHORT_MS);
            break;
        case WARMUP_SKIP:
            Serial.println("[MQ135] Warm-up saltato — letture iniziali meno precise.");
            break;
    }

    Serial.println("[MQ135] Pronto.");
}

/**
 * Legge dal sensore e restituisce i dati elaborati.
 *
 * Pipeline di elaborazione MQ135:
 *   raw ADC → Vadc → Vsensor (corretto per partitore) → RS (kΩ) → RS/R0 → CO2 (ppm)
 *
 * Deve essere chiamata solo dopo init(). Se R0 non è disponibile
 * (init() non è stato chiamato) restituisce un SensorData con mq135Ok=false.
 */
Data poll() {
    Data data{};

    if (s_r0 <= 0.0f) {
        Serial.println("[MQ135] ERRORE: R0 non disponibile. Chiamare init() prima.");
        data.ok = false;
        return data;
    }

    int   raw   = readAverageAnalog(MQ135_PIN, MQ135_SAMPLES);
    float rs    = computeRS(raw);
    float ratio = rs / s_r0; // RS/R0: >1 aria pulita, <1 presenza di gas
    float co2   = estimateCO2ppm(ratio);

    data.co2ppm = co2;
    data.ratio  = ratio;
    data.ok     = true;

    return data;
}

/**
 * Cancella R0 dalla NVS.
 *
 * Al prossimo avvio il sistema non troverà R0 in NVS e eseguirà
 * automaticamente il WARMUP_FULL con ricalibrazione completa.
 * Utile se il sensore è invecchiato o è stato spostato in un ambiente diverso.
 */
void resetCalibration() {
    s_prefs.begin(NVS_NAMESPACE, false);
    s_prefs.remove(NVS_KEY_R0);
    s_prefs.end();
    s_r0          = -1.0f;
    s_r0_from_nvs = false;
    Serial.println("[MQ135] NVS R0 cancellato. Ricalibrazione al prossimo avvio.");
}

} // namespace mq135