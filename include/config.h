#ifndef CONFIG_H
#define CONFIG_H

// --- PINOUT SENSORI ---
#define DHT_PIN         21  // TODO: Modificare con il pin corretto
#define DHT_TYPE        DHT22

#define MQ135_PIN       4   // Pin ADC (es. GPIO4) per la scheda Heltec V3

// --- TIMING E SLEEP ---
#define SLEEP_INTERVAL_MINUTES  5
#define uS_TO_S_FACTOR 1000000ULL  // Fattore di conversione da microsecondi a secondi

// --- PARAMETRI MQ-135 ---
// R0 deve essere calcolato all'aria pulita per avere misurazioni accurate.
#define MQ135_R0        10.0 // Da calibrare!

#endif // CONFIG_H
