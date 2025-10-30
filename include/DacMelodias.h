/**
 * @file    DacMelodias.h
 * @brief   Header para el generador de melodías mediante DAC
 * @details Declaraciones de funciones para generar diferentes melodías
 *          usando el DAC del LPC1769 con Timer1 para precisión
 * @date    Octubre 2025
 */

#ifndef DACMELODIAS_H_
#define DACMELODIAS_H_

#include <stdint.h>

/* ======================== FUNCIONES PÚBLICAS ============================== */

/**
 * @brief Inicializa el DAC y Timer1 para generación de melodías
 * @note Debe llamarse antes de usar cualquier otra función de este módulo
 */
void DAC_Init_Melodia(void);

/**
 * @brief Genera una nota musical con onda cuadrada
 * @param frecuencia: frecuencia de la nota en Hz (o 0 para silencio)
 * @param duracion: duración de la nota en milisegundos
 */
void DAC_GenerarNota(uint16_t frecuencia, uint16_t duracion);

/**
 * @brief Genera una nota musical con onda senoidal (sonido más suave)
 * @param frecuencia: frecuencia de la nota en Hz (o 0 para silencio)
 * @param duracion: duración de la nota en milisegundos
 */
void DAC_GenerarNota_Seno(uint16_t frecuencia, uint16_t duracion);

/**
 * @brief Reproduce la melodía "Happy Birthday"
 */
void DAC_ReproducirHappyBirthday(void);

/**
 * @brief Reproduce la melodía del tema de Super Mario Bros
 */
void DAC_ReproducirMario(void);

/**
 * @brief Reproduce la melodía del tema de Tetris
 */
void DAC_ReproducirTetris(void);

/**
 * @brief Reproduce la melodía del tema de Star Wars
 */
void DAC_ReproducirStarWars(void);

/**
 * @brief Reproduce la melodía del Nokia Tune
 */
void DAC_ReproducirNokia(void);

/**
 * @brief Reproduce la melodía de Jingle Bells
 */
void DAC_ReproducirJingleBells(void);

/**
 * @brief Genera un tono simple con la frecuencia y duración especificadas
 * @param frecuencia: frecuencia en Hz
 * @param duracion_ms: duración en milisegundos
 */
void DAC_TonoSimple(uint16_t frecuencia, uint16_t duracion_ms);

/**
 * @brief Reproduce una escala musical ascendente (Do, Re, Mi, Fa, Sol, La, Si, Do)
 */
void DAC_EscalaMusical(void);

/**
 * @brief Genera un efecto de sirena
 * @param duracion_total_ms: duración total del efecto en milisegundos
 */
void DAC_EfectoSirena(uint16_t duracion_total_ms);

/**
 * @brief Genera un efecto de láser/disparo (frecuencia descendente rápida)
 */
void DAC_EfectoLaser(void);

/**
 * @brief Genera un efecto de "Game Over" (melodía descendente)
 */
void DAC_EfectoGameOver(void);

/**
 * @brief Genera un efecto de "Victoria" (melodía triunfal)
 */
void DAC_EfectoVictoria(void);

/**
 * @brief Función demo que reproduce todas las melodías y efectos
 * @note Útil para probar todo el sistema de audio
 */
void DAC_DemoMelodias(void);

/* ======================= CONSTANTES DE NOTAS ============================== */

// Si deseas usar frecuencias personalizadas, aquí están las definiciones:
// Octava 3
#define DO_3    131
#define DO_S3   139
#define RE_3    147
#define RE_S3   156
#define MI_3    165
#define FA_3    175
#define FA_S3   185
#define SOL_3   196
#define SOL_S3  208
#define LA_3    220
#define LA_S3   233
#define SI_3    247

// Octava 4 (central)
#define DO_4    262
#define DO_S4   277
#define RE_4    294
#define RE_S4   311
#define MI_4    330
#define FA_4    349
#define FA_S4   370
#define SOL_4   392
#define SOL_S4  415
#define LA_4    440
#define LA_S4   466
#define SI_4    494

// Octava 5
#define DO_5    523
#define DO_S5   554
#define RE_5    587
#define RE_S5   622
#define MI_5    659
#define FA_5    698
#define FA_S5   740
#define SOL_5   784
#define SOL_S5  831
#define LA_5    880
#define LA_S5   932
#define SI_5    988

// Duraciones de notas estándar (en milisegundos)
#define REDONDA      2000
#define BLANCA       1000
#define NEGRA        500
#define CORCHEA      250
#define SEMICORCHEA  125

#endif /* DACMELODIAS_H_ */
