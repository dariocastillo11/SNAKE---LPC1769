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
 * @brief Inicializa el juego Serpiente
 * 
 * Configura el estado inicial del juego. Llamar una vez antes de ejecutar().
 */
void juego_serpiente_inicializar(void);

/**
 * @brief Ejecuta un tick del juego Serpiente
 * 
 * Debe llamarse periódicamente desde el bucle principal.
 * Actualiza la lógica del juego y redibuja el LCD.
 */
void juego_serpiente_ejecutar(void);

/**
 * @brief Reinicia el juego Serpiente
 * 
 * Vuelve a empezar desde el estado inicial.
 */
void juego_serpiente_reiniciar(void);

/**
 * @brief Verifica si el juego terminó (Game Over)
 * 
 * @return 1 si el juego terminó, 0 si sigue activo
 */
uint8_t juego_serpiente_ha_terminado(void);

/**
 * @brief Obtiene la puntuación actual
 * 
 * @return Puntuación (número de comidas comidas)
 */
uint32_t juego_serpiente_obtener_puntuacion(void);

#endif // SNAKE_GAME_H
