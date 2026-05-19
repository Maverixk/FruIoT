#ifndef CONFIG_H
#define CONFIG_H

// --- PINOUT SENSORI ---
#define DHT_PIN         5   // DHT22 DATA (GPIO5)
#define DHT_VCC_PIN     47  // DHT22 VCC: HIGH = alimentato, LOW = spento (recovery lock-up)
#define DHT_TYPE        DHT22

#define MQ135_PIN       4   // MQ-135 AOUT su ADC1_CH3 (GPIO4, safe con WiFi attivo)

// --- TRANSISTOR S8050 (switch boost converter XL6009 + MQ-135) ---
// 0 = bypass (MQ-135 sempre alimentato dal rail 5V, nessun controllo)
// 1 = controllato da S8050 (HIGH = boost ON, LOW = boost OFF prima del deep sleep)
#define USE_TRANSISTOR  0
#define TRANSISTOR_PIN  6   // GPIO6 → base S8050: HIGH = XL6009+MQ135 ON, LOW = boost OFF

// --- TIMING E SLEEP ---
#define SLEEP_INTERVAL_MINUTES  20
#define uS_TO_S_FACTOR 1000000ULL  // Fattore di conversione da microsecondi a secondi

// =============================================================================
// STRATEGIA DI WARM-UP — modificare questa riga per cambiare modalità
//
//   WARMUP_FULL  — 90s di warm-up + calibrazione R0 se NVS vuoto.
//   WARMUP_SHORT — 30s di warm-up, R0 caricato da NVS.
//   WARMUP_SKIP  — Nessun warm-up. Lettura immediata dopo il boot.
//
// NOTA: WARMUP_SKIP e WARMUP_SHORT richiedono che R0 sia già salvato in NVS.
//       Se NVS è vuoto, il sistema forza automaticamente WARMUP_FULL.
// =============================================================================
#define WARMUP_FULL   0
#define WARMUP_SHORT  1
#define WARMUP_SKIP   2

#define MQ135_WARMUP_STRATEGY   0  // <-- cambia qui per i test

// Durate warm-up (ms)
#define MQ135_WARMUP_FULL_MS    90000UL
#define MQ135_WARMUP_SHORT_MS   30000UL

// --- ADC / PARTITORE ---
#define ADC_MAX_COUNTS      4095.0f
#define ADC_REF_VOLTAGE     3.3f
#define MQ135_DIVIDER_RATIO 2.0f    // R1=R2=100kΩ → Vgpio = Vao / 2
#define MQ135_VCC           5.0f    // alimentazione sensore (V)
#define MQ135_RL_KOHM       1.0f    // resistenza di carico sulla scheda (kΩ)

// =============================================================================
// FILTRO A MEDIANA — oversampling + mediana per reiezione spike
//
// Per ogni lettura MQ135, vengono raccolti MQ135_MEDIAN_WINDOW campioni ADC.
// I campioni vengono ordinati e si prende il valore centrale (mediana).
// Usare un numero DISPARI per avere una mediana esatta.
//
// MQ135_MEDIAN_WINDOW  : dimensione della finestra (numero di campioni)
// MQ135_SAMPLE_DELAY_MS: ritardo tra un campione e il successivo (ms)
// =============================================================================
#define MQ135_MEDIAN_WINDOW     31
#define MQ135_SAMPLE_DELAY_MS   2       // stabilizzazione condensatore sample-and-hold dell'ADC, indipendente dalla window

// --- Persistenza R0 tra i riavvii, salvataggio in NVS, memoria non volatile ---
#define NVS_NAMESPACE       "fruiot"
#define NVS_KEY_R0          "mq135_r0"

// Parametri calibrazione R0
// Ogni singola lettura è già filtrata con mediana (vedi MQ135_MEDIAN_WINDOW).
// La calibrazione ripete questa lettura MQ135_CALIB_SAMPLES volte, distanziate
// di MQ135_CALIB_DELAY_MS, per catturare la variabilità temporale del sensore
// (fluttuazioni termiche, rumore a bassa frequenza). R0 = media delle RS.
#define MQ135_CALIB_SAMPLES     50
#define MQ135_CALIB_DELAY_MS    200UL   // UL = unsigned long, necessario per confronti con millis()

#endif // CONFIG_H
