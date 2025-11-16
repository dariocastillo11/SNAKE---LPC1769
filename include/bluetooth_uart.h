/**
 * @file bluetooth_uart.h
 * @brief Driver de comunicación Bluetooth por UART0 (HC-05)
 *
 * Hardware:
 * - P0.2: TXD0 (transmisión a módulo Bluetooth)
 * - P0.3: RXD0 (recepción desde módulo Bluetooth)
 * - Velocidad: 9600 bps
 *
 * Comandos soportados:
 * - 'W' o 'w': Arriba
 * - 'S' o 's': Abajo
 * - 'A' o 'a': Izquierda
 * - 'D' o 'd': Derecha
 * - 'B' o 'b': Botón
 * - 'P' o 'p': Pausa / Reintentar
 *
 * @date Noviembre 2025
 */

#ifndef BLUETOOTH_UART_H
#define BLUETOOTH_UART_H

#include <stdint.h>

/* === CONFIGURACIÓN === */
#define BT_VELOCIDAD_UART0    9600

/* === FUNCIONES PÚBLICAS === */

/**
 * @brief Inicializa UART0 para comunicación Bluetooth
 * 
 * Configura:
 * - P0.2 como TXD0 (PINSEL función 01)
 * - P0.3 como RXD0 (PINSEL función 01)
 * - Velocidad: 9600 bps
 * - Formato: 8 bits, sin paridad, 1 stop bit (8N1)
 * - FIFO habilitado
 */
void bt_inicializar(void);

/**
 * @brief Lee un carácter desde Bluetooth (no bloqueante)
 * 
 * @return Carácter recibido (0-255) o -1 si no hay datos disponibles
 */
int bt_leer_caracter_no_bloqueante(void);

/**
 * @brief Actualiza el buffer UART leyendo caracteres disponibles
 * 
 * Debe llamarse periódicamente desde el loop principal para recibir
 * comandos Bluetooth. Procesa automáticamente los comandos recibidos.
 */
void bt_actualizar_buffer(void);

/**
 * @brief Envía un carácter por Bluetooth
 * 
 * @param caracter Carácter a enviar
 */
void bt_escribir_caracter(char caracter);

/**
 * @brief Envía una cadena por Bluetooth
 * 
 * @param cadena Cadena terminada en '\0' a enviar
 */
void bt_escribir_cadena(const char *cadena);

/**
 * @brief Procesa comandos Bluetooth recibidos
 * 
 * Lee caracteres disponibles y actualiza los valores simulados
 * del joystick para que los juegos interpreten los comandos
 * como si vinieran del joystick físico.
 * 
 * Debe llamarse periódicamente desde el bucle principal.
 */
void bt_procesar_comandos(void);

/**
 * @brief Obtiene el valor simulado del eje X del joystick
 * 
 * @return Valor ADC simulado (0-4095):
 *         - < 500: Izquierda
 *         - 2048: Centro (neutral)
 *         - > 3500: Derecha
 */
uint16_t bt_obtener_x_simulado(void);

/**
 * @brief Obtiene el valor simulado del eje Y del joystick
 * 
 * @return Valor ADC simulado (0-4095):
 *         - < 500: Arriba
 *         - 2048: Centro (neutral)
 *         - > 3500: Abajo
 */
uint16_t bt_obtener_y_simulado(void);

/**
 * @brief Verifica si hay un comando de botón pendiente
 * 
 * @return 1 si se recibió comando de botón, 0 en caso contrario
 */
uint8_t bt_obtener_comando_boton(void);

/**
 * @brief Limpia el flag de comando de botón
 * 
 * Debe llamarse después de procesar un comando de botón
 * para evitar múltiples activaciones.
 */
void bt_limpiar_comando_boton(void);

#endif /* BLUETOOTH_UART_H */
