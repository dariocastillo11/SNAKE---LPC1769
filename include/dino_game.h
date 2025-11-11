/**
 * @file dino_game.h
 * @brief Mini-juego "Dinosaurio" para LCD I2C.
 *
 * Interfaz pública para inicializar y ejecutar el juego. Usa las funciones de la libreria
 * creada  `lcd_i2c` ya presentes en el proyecto.
 */

#ifndef DINO_GAME_H
#define DINO_GAME_H

#include <stdint.h>

/** Inicializa el juego Dinosaurio (configura pin de botón y timer). Llamar una vez. */
void juego_dinosaurio_inicializar(void);

/** Debe llamarse periódicamente desde el bucle principal (while) para procesar
 *  actualizaciones y dibujado. No bloquea; el temporizador genera ticks.
 */
void juego_dinosaurio_ejecutar(void);

/** Reinicia el juego (vuelve a empezar tras game over). */
void juego_dinosaurio_reiniciar(void);

/** Retorna el estado del juego: 0 = jugando, 1 = game over, 2 = volver al menú */
uint8_t juego_dinosaurio_ha_terminado(void);

#endif // DINO_GAME_H
