/**
 * @file bluetooth_uart.h
 * @brief Driver de comunicación Bluetooth por UART0 (HC-05)
 *
 * Hardware:
 * - P0.2: TXD0 (transmisión a módulo Bluetooth)
 * - P0.3: RXD0 (recepción desde módulo Bluetooth)
 * - Baud rate: 9600 bps
 *
 * Comandos soportados:
 * - 'W' o 'w': Arriba (joystick UP)
 * - 'S' o 's': Abajo (joystick DOWN)
 * - 'A' o 'a': Izquierda (joystick LEFT)
 * - 'D' o 'd': Derecha (joystick RIGHT)
 * - 'B' o 'b': Botón (equivalente a presionar P0.4)
 * - 'P' o 'p': Pausa (Snake) / Reintentar (Dino)
 *
 * @date Noviembre 2025
 */

#ifndef BLUETOOTH_UART_H
#define BLUETOOTH_UART_H

#include <stdint.h>

/* === CONFIGURACIÓN === */
#define BT_UART0_BAUD    9600

/* === FUNCIONES PÚBLICAS === */

/**
 * @brief Inicializa UART0 para comunicación Bluetooth
 * 
 * Configura:
 * - P0.2 como TXD0 (PINSEL función 01)
 * - P0.3 como RXD0 (PINSEL función 01)
 * - Baud rate: 9600 bps
 * - Formato: 8 bits, sin paridad, 1 stop bit (8N1)
 * - FIFO habilitado
 */
void bt_init(void);

/**
 * @brief Lee un carácter desde Bluetooth (no bloqueante)
 * 
 * @return Carácter recibido (0-255) o -1 si no hay datos disponibles
 */
int bt_read_char_non_block(void);

/**
 * @brief Envía un carácter por Bluetooth
 * 
 * @param c Carácter a enviar
 */
void bt_write_char(char c);

/**
 * @brief Envía una cadena por Bluetooth
 * 
 * @param s Cadena terminada en '\0' a enviar
 */
void bt_write_str(const char *s);

/**
 * @brief Procesa comandos Bluetooth recibidos
 * 
 * Lee caracteres disponibles y actualiza los valores simulados
 * del joystick para que los juegos interpreten los comandos
 * como si vinieran del joystick físico.
 * 
 * Debe llamarse periódicamente desde el main loop.
 */
void bt_process_commands(void);

/**
 * @brief Obtiene el valor simulado del eje X del joystick
 * 
 * @return Valor ADC simulado (0-4095):
 *         - < 500: Izquierda ('A')
 *         - 2048: Centro (neutral)
 *         - > 3500: Derecha ('D')
 */
uint16_t bt_get_simulated_x(void);

/**
 * @brief Obtiene el valor simulado del eje Y del joystick
 * 
 * @return Valor ADC simulado (0-4095):
 *         - < 500: Arriba ('W')
 *         - 2048: Centro (neutral)
 *         - > 3500: Abajo ('S')
 */
uint16_t bt_get_simulated_y(void);

/**
 * @brief Verifica si hay un comando de botón pendiente
 * 
 * @return 1 si se recibió comando 'B' (botón), 0 en caso contrario
 */
uint8_t bt_get_button_command(void);

/**
 * @brief Limpia el flag de comando de botón
 * 
 * Debe llamarse después de procesar un comando de botón
 * para evitar múltiples activaciones.
 */
void bt_clear_button_command(void);

#endif /* BLUETOOTH_UART_H */
