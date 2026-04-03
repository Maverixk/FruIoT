#pragma once
#include <Arduino.h>

/**
 * Modulo MQ135 — lettura gas e stima CO2.
 *
 */
namespace mq135 {

struct Data {
    float  co2ppm;  // stima CO2 in ppm 
    float  ratio;   // RS / R0: >1 aria pulita, <1 presenza di gas
    bool   ok;
};

/**
 * Inizializza il sensore, gestisce warm-up e calibrazione R0.
 * Funzione bloccante per la durata del warm-up.
 * Deve essere chiamata una volta in setup() prima di poll().
 *
 * Flusso:
 *   NVS vuoto → warm-up 90s + calibrazione R0 + salvataggio NVS → return
 *   NVS ok    → warm-up secondo MQ135_WARMUP_STRATEGY → return
 *
 * Per forzare la ricalibrazione chiamare resetCalibration() prima di init().
 */
void init();

/**
 * Legge il sensore e restituisce co2ppm, ratio e ok.
 * Chiamare solo dopo init().
 */
Data poll();

/**
 * Cancella R0 dalla NVS.
 * Al prossimo avvio verrà eseguita la calibrazione completa.
 * Utile se il sensore è invecchiato o spostato in un ambiente diverso.
 */
void resetCalibration();

} // namespace mq135