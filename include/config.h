#ifndef CONFIG_H
#define CONFIG_H

// --- PINOUT SENSORI ---
#define DHT_PIN         21  // TODO: Modificare con il pin corretto
#define DHT_TYPE        DHT22

#define MQ135_PIN       4   // Pin ADC (GPIO4) per la scheda Heltec V3

// --- TIMING E SLEEP ---
#define SLEEP_INTERVAL_MINUTES  5
#define uS_TO_S_FACTOR 1000000ULL  // Fattore di conversione da microsecondi a secondi

// =============================================================================
// STRATEGIA DI WARM-UP — modificare questa riga per cambiare modalità
//
// Le tre opzioni permettono di testare il tradeoff tra precisione e consumo
// energetico in funzione del duty cycle del deep sleep.
//
//   WARMUP_FULL  — 90s di warm-up + calibrazione R0 ad ogni avvio.
//                  Massima precisione. Usare solo al primo avvio o dopo reset.
//                  Consumo stimato: ~90s attivo ogni SLEEP_INTERVAL_MINUTES.
//
//   WARMUP_SHORT — 30s di warm-up, R0 caricato da NVS (no ricalibrazione).
//                  Buon compromesso. Letture leggermente meno precise.
//                  Consumo stimato: ~30s attivo ogni SLEEP_INTERVAL_MINUTES.
//
//   WARMUP_SKIP  — Nessun warm-up. Lettura immediata dopo il boot.
//                  Minimo consumo energetico. Precisione ridotta:
//                  l'elemento riscaldante non ha raggiunto Toperativa.
//                  Utile per stimare il consumo minimo teorico del sistema.
//                  Consumo stimato: ~2-3s attivo ogni SLEEP_INTERVAL_MINUTES.
//
// NOTA: WARMUP_SKIP e WARMUP_SHORT richiedono che R0 sia già salvato in NVS.
//       Se NVS è vuoto, il sistema forza automaticamente WARMUP_FULL.
// =============================================================================
#define WARMUP_FULL   0
#define WARMUP_SHORT  1
#define WARMUP_SKIP   2
 
#define MQ135_WARMUP_STRATEGY   WARMUP_FULL   // <-- cambia qui per i test
 
// Durate warm-up (ms)
#define MQ135_WARMUP_FULL_MS    90000UL
#define MQ135_WARMUP_SHORT_MS   30000UL

// ---ADC / PARTITORE ---
#define ADC_MAX_COUNTS      4095.0f
#define ADC_REF_VOLTAGE     3.3f
#define MQ135_DIVIDER_RATIO 2.0f    // R1=R2=100kΩ → Vgpio = Vao / 2
#define MQ135_VCC           5.0f    // alimentazione sensore (V)
#define MQ135_RL_KOHM       1.0f    // resistenza di carico sulla scheda (kΩ)


// Media su più campioni per stabilizzare la lettura del sensore, che può essere rumorosa.
// Un numero maggiore di campioni migliora la stabilità ma aumenta il tempo di lettura e il consumo.
// TODO: vedere se calibrarlo
#define MQ135_SAMPLES       32      

// =============================================================================
// COEFFICIENTI CO2 — Metodo Gironi (2014)
//
// Fonte: Davide Gironi, "Cheap CO2 meter using the MQ135 sensor with AVR ATmega"
// https://davidegironi.blogspot.com/2014/01/cheap-co2-meter-using-mq135-sensor-with.html
//
// Coefficienti A e B ottenuti tramite regressione MATLAB correlando
// l'uscita raw del MQ135 con un sensore NDIR professionale MHZ14.
// Formula: co2_ppm = MQ135_CO2_A * (RS / R0) ^ MQ135_CO2_B
// =============================================================================
#define MQ135_CO2_A         56.0820f
#define MQ135_CO2_B         (-5.9603f)
#define MQ135_CO2_CLEAN_AIR 420.0f    // CO2 atmosferica attuale (ppm), 2025


// --- Persistenza R0 tra i riavvii, salvataggio in NVS, memoria non volatile ---
#define NVS_NAMESPACE       "fruiot"
#define NVS_KEY_R0          "mq135_r0"
 
// Parametri calibrazione R0
#define MQ135_CALIB_SAMPLES     50
#define MQ135_CALIB_DELAY_MS    200UL
 
#endif // CONFIG_H
