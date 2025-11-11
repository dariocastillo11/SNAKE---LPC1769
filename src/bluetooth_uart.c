/**
 * @file bluetooth_uart.c
 * @brief Implementación del driver Bluetooth UART0
 *
 * @date Noviembre 2025
 */

#include "bluetooth_uart.h"
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include <stdint.h>

/* === VARIABLES PRIVADAS === */

/**
 * Valores simulados del joystick según comandos Bluetooth
 * - 2048: Centro (neutral)
 * - 100: Extremo mínimo (izquierda/arriba)
 * - 3900: Extremo máximo (derecha/abajo)
 */
static uint16_t simulated_x = 2048;  // Eje X simulado
static uint16_t simulated_y = 2048;  // Eje Y simulado
static uint8_t button_pressed = 0;   // Flag de botón virtual

/**
 * Duración del comando simulado (en ciclos de process)
 * Permite que el comando se mantenga activo brevemente
 * para ser detectado por los juegos.
 */
static uint8_t command_duration = 0;

/* === FUNCIONES PÚBLICAS === */

/**
 * @brief Inicializa UART0 para comunicación Bluetooth (9600 bps, 8N1)
 */
void bt_init(void) {
    /* Configurar P0.2 (TXD0) y P0.3 (RXD0) como función UART0 (FUNC=01) */
    PINSEL_CFG_Type pin_cfg;
    
    /* P0.2 = TXD0 */
    pin_cfg.portNum = PINSEL_PORT_0;
    pin_cfg.pinNum = PINSEL_PIN_2;
    pin_cfg.funcNum = PINSEL_FUNC_1;  // TXD0
    pin_cfg.pinMode = PINSEL_TRISTATE;
    pin_cfg.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);
    
    /* P0.3 = RXD0 */
    pin_cfg.pinNum = PINSEL_PIN_3;
    pin_cfg.funcNum = PINSEL_FUNC_1;  // RXD0
    PINSEL_ConfigPin(&pin_cfg);
    
    /* Habilitar alimentación de UART0 */
    LPC_SC->PCONP |= (1u << 3);
    
    /* Calcular divisor de baudrate
       PCLK_UART0 = SystemCoreClock / 4 = 100MHz / 4 = 25MHz
       Divisor = PCLK / (16 * baudrate) */
    uint32_t PCLK = SystemCoreClock / 4;  // 25 MHz por defecto
    uint32_t DL = PCLK / (16 * BT_UART0_BAUD);
    
    /* Configurar formato: 8 bits, sin paridad, 1 stop bit (8N1) */
    LPC_UART0->LCR = 0x83;  // DLAB=1 (acceso a divisores), 8N1
    LPC_UART0->DLM = (DL >> 8) & 0xFF;  // Divisor MSB
    LPC_UART0->DLL = (DL >> 0) & 0xFF;  // Divisor LSB
    LPC_UART0->FDR = (1 << 4) | 0;      // MULVAL=1, DIVADDVAL=0 (sin fracción)
    
    /* Habilitar y resetear FIFOs */
    LPC_UART0->FCR = 0x07;  // FIFO enable + reset TX/RX FIFOs
    
    /* Cerrar acceso a divisores, mantener 8N1 */
    LPC_UART0->LCR = 0x03;  // DLAB=0, 8N1
}

/**
 * @brief Lee un carácter desde UART0 (no bloqueante)
 * 
 * @return Carácter recibido (0-255) o -1 si no hay datos
 */
int bt_read_char_non_block(void) {
    /* Verificar si hay datos disponibles (RDR bit en LSR) */
    if (LPC_UART0->LSR & (1u << 0)) {
        return (int)LPC_UART0->RBR;  // Leer byte del buffer de recepción
    }
    return -1;  // No hay datos
}

/**
 * @brief Envía un carácter por UART0
 * 
 * @param c Carácter a enviar
 */
void bt_write_char(char c) {
    /* Esperar a que el buffer de transmisión esté vacío (THRE bit en LSR) */
    while ((LPC_UART0->LSR & (1u << 5)) == 0);
    LPC_UART0->THR = c;  // Escribir carácter al buffer de transmisión
}

/**
 * @brief Envía una cadena por UART0
 * 
 * @param s Cadena terminada en '\0'
 */
void bt_write_str(const char *s) {
    while (*s) {
        bt_write_char(*s++);
    }
}

/**
 * @brief Procesa comandos Bluetooth recibidos
 * 
 * Comandos soportados:
 * - 'W'/'w': Arriba (Y = 100)
 * - 'S'/'s': Abajo (Y = 3900)
 * - 'A'/'a': Izquierda (X = 100)
 * - 'D'/'d': Derecha (X = 3900)
 * - 'B'/'b': Botón presionado
 * - Cualquier otro: Volver a centro (X=2048, Y=2048)
 */
void bt_process_commands(void) {
    int ch = bt_read_char_non_block();
    
    if (ch >= 0) {
        /* Resetear duración del comando anterior */
        command_duration = 5;  // Mantener comando activo por ~5 ciclos
        
        switch (ch) {
            case 'W': case 'w':  // Arriba
                simulated_x = 2048;
                simulated_y = 200;   // Joystick arriba (ADC bajo) - ajustado de 100 a 200
                bt_write_str("CMD: UP (X=2048, Y=200)\r\n");
                break;
                
            case 'S': case 's':  // Abajo
                simulated_x = 2048;
                simulated_y = 3800;  // Joystick abajo (ADC alto) - ajustado de 3900 a 3800
                bt_write_str("CMD: DOWN (X=2048, Y=3800)\r\n");
                break;
                
            case 'A': case 'a':  // Izquierda
                simulated_x = 200;   // Joystick izquierda (ADC bajo) - ajustado de 100 a 200
                simulated_y = 2048;
                bt_write_str("CMD: LEFT (X=200, Y=2048)\r\n");
                break;
                
            case 'D': case 'd':  // Derecha
                simulated_x = 3800;  // Joystick derecha (ADC alto) - ajustado de 3900 a 3800
                simulated_y = 2048;
                bt_write_str("CMD: RIGHT (X=3800, Y=2048)\r\n");
                break;
                
            case 'B': case 'b':  // Botón
                button_pressed = 1;
                bt_write_str("CMD: BUTTON\r\n");
                break;
                
            default:  // Comando desconocido, volver a centro
                simulated_x = 2048;
                simulated_y = 2048;
                break;
        }
    } else {
        /* No hay nuevos comandos, decrementar duración */
        if (command_duration > 0) {
            command_duration--;
            if (command_duration == 0) {
                /* Comando expiró, volver a centro */
                simulated_x = 2048;
                simulated_y = 2048;
            }
        }
    }
}

/**
 * @brief Obtiene el valor simulado del eje X
 * 
 * @return Valor ADC simulado del eje X (0-4095)
 */
uint16_t bt_get_simulated_x(void) {
    return simulated_x;
}

/**
 * @brief Obtiene el valor simulado del eje Y
 * 
 * @return Valor ADC simulado del eje Y (0-4095)
 */
uint16_t bt_get_simulated_y(void) {
    return simulated_y;
}

/**
 * @brief Verifica si hay comando de botón pendiente
 * 
 * @return 1 si el botón virtual está presionado, 0 en caso contrario
 */
uint8_t bt_get_button_command(void) {
    return button_pressed;
}

/**
 * @brief Limpia el flag de botón virtual
 * 
 * Debe llamarse después de procesar el comando de botón
 * para evitar activaciones repetidas.
 */
void bt_clear_button_command(void) {
    button_pressed = 0;
}
