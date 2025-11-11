/**
 * @file menu_juegos.h
 * @brief Menú de selección de juegos con navegación por joystick
 *
 * Permite al usuario seleccionar entre diferentes juegos usando el joystick:
 * - Joystick ARRIBA/ABAJO: navegar opciones
 * - Botón del joystick: seleccionar juego
 *
 * @date Noviembre 2025
 */

#ifndef MENU_JUEGOS_H
#define MENU_JUEGOS_H

#include <stdint.h>

/**
 * @brief Identificadores de los juegos disponibles
 */
typedef enum {
    JUEGO_DINO = 0,
    JUEGO_SNAKE = 1,
    NUM_JUEGOS = 2
} JuegoID;

/**
 * @brief Inicializa el sistema de menú
 * 
 * Debe llamarse una vez al inicio, después de inicializar LCD y joystick.
 */
void menu_inicializar(void);

/**
 * @brief Muestra el menú en el LCD y procesa entrada del joystick
 * 
 * Esta función debe llamarse en el loop principal mientras no hay
 * juego activo. Retorna el ID del juego seleccionado o -1 si no hay selección.
 * 
 * @return JuegoID del juego seleccionado, o -1 si aún no se seleccionó
 */
int8_t menu_ejecutar(void);

/**
 * @brief Reinicia el menú para volver a mostrar las opciones
 * 
 * Llamar cuando un juego termina y se quiere volver al menú.
 */
void menu_reiniciar(void);

#endif // MENU_JUEGOS_H
