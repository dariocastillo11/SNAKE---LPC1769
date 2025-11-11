/**
 * @file melodias_dac.h
 * @brief Sistema de reproducción de melodías usando DAC y Timer
 * @details Permite reproducir melodías musicales en segundo plano sin bloquear
 *          la ejecución del programa principal.
 *
 * Hardware requerido:
 * - P0.26: Salida DAC (AOUT)
 * - P0.22: LED indicador de actividad (opcional)
 *
 * Uso básico:
 * 1. Llamar melodias_init() al inicio del programa
 * 2. Llamar melodias_iniciar(melodia) para comenzar a tocar
 * 3. Llamar melodias_actualizar() en el loop principal
 * 4. Verificar melodias_esta_sonando() para saber si hay audio
 *
 * @date Noviembre 2025
 */

#ifndef MELODIAS_DAC_H
#define MELODIAS_DAC_H

#include <stdint.h>

/* ========================== DEFINICIONES DE NOTAS ========================= */

// Frecuencias de notas musicales (en Hz) - Escala temperada
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

#define SILENCIO 0

// Duraciones de notas (en milisegundos)
#define REDONDA      2000
#define BLANCA       1000
#define NEGRA        500
#define CORCHEA      250
#define SEMICORCHEA  125

/* ========================== ESTRUCTURA DE NOTA ============================ */

/**
 * @brief Estructura que define una nota musical
 * @param frecuencia Frecuencia en Hz (0 = silencio)
 * @param duracion Duración en milisegundos
 */
typedef struct {
    uint16_t frecuencia;
    uint16_t duracion;
} Nota;

/* ============================= MELODÍAS ===================================== */

// Melodías predefinidas (terminan con {SILENCIO, 0})
extern const Nota melodia_happy_birthday[];
extern const Nota melodia_mario[];
extern const Nota melodia_tetris[];
extern const Nota melodia_nokia[];
extern const Nota melodia_game_over[];  // Melodía corta para game over
extern const Nota melodia_salto[];      // Efecto de sonido para salto
extern const Nota melodia_fondo[];      // Melodía de fondo larga (tipo Mario Bros)

/* ==================== FUNCIONES PÚBLICAS ================================== */

/**
 * @brief Inicializa el sistema de melodías (DAC + Timer0 + GPIO)
 * @note Debe llamarse una vez al inicio del programa, antes de usar melodías
 * @note Configura P0.26 como AOUT y P0.22 como GPIO para LED
 */
void melodias_init(void);

/**
 * @brief Inicia la reproducción de una melodía de forma no bloqueante
 * @param melodia Puntero al arreglo de notas (debe terminar con {SILENCIO, 0})
 * @note La melodía se reproduce en segundo plano usando interrupciones
 * @note Si hay una melodía sonando, esta se detiene y empieza la nueva
 */
void melodias_iniciar(const Nota *melodia);

/**
 * @brief Inicia una melodía en modo loop continuo (música de fondo)
 * @param melodia Puntero al arreglo de notas (debe terminar con {SILENCIO, 0})
 * @note La melodía se repite automáticamente al terminar
 * @note Ideal para música de fondo en juegos
 */
void melodias_iniciar_loop(const Nota *melodia);

/**
 * @brief Detiene la reproducción de la melodía actual
 * @note Silencia el DAC y limpia el estado interno
 */
void melodias_detener(void);

/**
 * @brief Actualiza el estado de la melodía (avanza notas si es necesario)
 * @note DEBE llamarse continuamente en el while(1) del main
 * @note Es NO BLOQUEANTE - retorna inmediatamente
 */
void melodias_actualizar(void);

/**
 * @brief Verifica si hay una melodía activa en reproducción
 * @return 1 si hay una melodía sonando, 0 si no
 * @note Retorna 1 incluso durante pausas entre notas de la misma melodía
 */
uint8_t melodias_esta_sonando(void);

/**
 * @brief Obtiene el tiempo transcurrido desde el inicio en milisegundos
 * @return Tiempo en milisegundos
 * @note Útil para el timing del juego
 */
uint32_t melodias_obtener_tiempo_ms(void);

/**
 * @brief Configura el volumen relativo del DAC (0-100)
 * @param volumen Porcentaje de volumen (0 = silencio, 100 = máximo)
 * @note Afecta la amplitud de la onda generada
 */
void melodias_set_volumen(uint8_t volumen);

#endif /* MELODIAS_DAC_H */
