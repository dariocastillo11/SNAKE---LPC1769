/**
 * @file joystick_adc.h
 * @brief Control de joystick analógico mediante ADC y visualización con LEDs.
 *
 * Hardware:
 * - P0.23 (AD0.0): Eje X del joystick
 * - P0.24 (AD0.1): Eje Y del joystick
 * - P2.10: Botón del joystick (interrupción)
 * - P0.9:  LED indicador ARRIBA
 * - P0.8:  LED indicador ABAJO
 * - P0.7:  LED indicador IZQUIERDA
 * - P0.6:  LED indicador DERECHA
 * - P0.0:  LED indicador BOTÓN/CENTRO
 *
 * @date Noviembre 2025
 */

#ifndef JOYSTICK_ADC_H
#define JOYSTICK_ADC_H

#include <stdint.h>

/**
 * @brief Inicializa el ADC, los pines del joystick y los LEDs indicadores.
 * 
 * Configura:
 * - ADC para lectura de ejes X e Y
 * - GPIO para LEDs (P0.0, P0.6-P0.9) como salidas
 * - Interrupción del botón (P2.10)
 */
void joystick_init(void);

/**
 * @brief Lee el estado actual del joystick y actualiza los LEDs.
 * 
 * Debe llamarse periódicamente desde el bucle principal.
 * Lee los valores ADC del eje X e Y y enciende el LED correspondiente
 * según la dirección detectada o el estado del botón.
 */
void joystick_update(void);

/**
 * @brief Lee el valor del ADC en un canal específico.
 * 
 * @param canal Canal del ADC (0 para eje X, 1 para eje Y)
 * @return Valor ADC de 12 bits (0-4095)
 */
uint16_t joystick_leer_adc(uint8_t canal);

/**
 * @brief Obtiene el estado del botón del joystick.
 * 
 * @return 1 si el botón está presionado, 0 si no
 */
uint8_t joystick_boton_presionado(void);

#endif // JOYSTICK_ADC_H
