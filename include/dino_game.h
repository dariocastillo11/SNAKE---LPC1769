/**
 * @file dino_game.h
 * @brief Mini-juego "Dinosaurio" para LCD I2C.
 *
 * Interfaz pública para inicializar y ejecutar el juego. Usa las funciones del
 * driver `lcd_i2c` ya presentes en el proyecto. No modifica otros módulos.
 */

#ifndef DINO_GAME_H
#define DINO_GAME_H

#include <stdint.h>

/** Inicializa el juego (configura pin de botón y timer). Llamar una vez. */
void dino_game_init(void);

/** Debe llamarse periódicamente desde el bucle principal (while) para procesar
 *  actualizaciones y dibujado. No bloquea; el temporizador genera ticks.
 */
void dino_game_run(void);

/** Reinicia el juego (vuelve a empezar tras game over). */
void dino_game_restart(void);

#endif // DINO_GAME_H
