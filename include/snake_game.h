/**
 * @file snake_game.h
 * @brief Juego Snake para LCD I2C controlado con joystick
 *
 * Juego clásico de la serpiente:
 * - Joystick: controlar dirección de la serpiente
 * - Botón: pausar/reanudar juego
 * - Objetivo: comer comida (*) y crecer sin chocar
 *
 * @date Noviembre 2025
 */

#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include <stdint.h>

/**
 * @brief Inicializa el juego Snake
 * 
 * Configura el estado inicial del juego. Llamar una vez antes de run().
 */
void snake_game_init(void);

/**
 * @brief Ejecuta un tick del juego Snake
 * 
 * Debe llamarse periódicamente desde el bucle principal.
 * Actualiza la lógica del juego y redibuja el LCD.
 */
void snake_game_run(void);

/**
 * @brief Reinicia el juego Snake
 * 
 * Vuelve a empezar desde el estado inicial.
 */
void snake_game_restart(void);

/**
 * @brief Verifica si el juego terminó (Game Over)
 * 
 * @return 1 si el juego terminó, 0 si sigue activo
 */
uint8_t snake_game_is_over(void);

/**
 * @brief Obtiene la puntuación actual
 * 
 * @return Puntuación (número de comidas comidas)
 */
uint32_t snake_game_get_score(void);

#endif // SNAKE_GAME_H
