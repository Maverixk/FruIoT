#ifndef CONFIG_H
#define CONFIG_H

// --- CONFIGURAZIONE GENERALE ---
#define CURRENT_MONITOR 0  // 0 = disabilitato, 1 = abilitato (misura corrente istantanea durante il polling MQ135)
#define USE_TRANSISTOR  0  // 0 = MQ135 sempre alimentato (workaround powerbank), 1 = controllato da transistor S8050

// --- PINOUT SENSORI ---
#define DHT_PIN         5  // Pin GPIO 5 per il DHT22 sulla scheda Heltec V3
#define DHT_TYPE        DHT22

#define MQ135_PIN       4   // Pin ADC (GPIO4) per la scheda Heltec V3

// --- TRANSISTOR S8050 (alimentazione MQ135) ---
#define TRANSISTOR_PIN  3   // GPIO3: HIGH = MQ135 alimentato, LOW = MQ135 spento

// --- INA219 (Power Monitor) ---
#define INA219_SDA      6       // Pin SDA per il bus I2C dell'INA219
#define INA219_SCL      7       // Pin SCL per il bus I2C dell'INA219
#define INA219_I2C_ADDR 0x40    // Indirizzo I2C default (A0=GND, A1=GND)

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
