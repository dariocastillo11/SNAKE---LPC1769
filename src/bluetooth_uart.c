/**
 * @file bluetooth_uart.c
 * @brief Driver UART0 para Bluetooth HC-05 (sin DMA)
 */

#include "bluetooth_uart.h"
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include <stddef.h>
#include <string.h>

/* === CONFIGURACIÓN === */
#define TAMAÑO_BUFFER_RX 256

/* === BUFFER CIRCULAR === */
static uint8_t buffer_rx_bt[TAMAÑO_BUFFER_RX];
static volatile uint16_t contador_rx = 0;        // Índice de escritura
static uint16_t contador_lectura = 0;            // Índice de lectura

/* === VARIABLES DE COMANDO === */
static uint16_t valor_x_simulado = 2048;
static uint16_t valor_y_simulado = 2048;
static uint8_t comando_boton = 0;
static uint8_t bandera_boton_procesado = 0;
static uint8_t duracion_comando = 0;  // Duraci del comando en ciclos

/* === FORWARD DECLARATIONS === */
static void procesar_comando_bt(char comando);

/**
 * @brief Inicializa UART0 para Bluetooth (9600 bps, 8N1)
 */
void bt_inicializar(void) {
    /* Configurar pines P0.2 (TXD0) y P0.3 (RXD0) */
    PINSEL_CFG_Type pin_config;
    
    /* P0.2 - TXD0 */
    pin_config.portNum = PINSEL_PORT_0;
    pin_config.pinNum = PINSEL_PIN_2;
    pin_config.funcNum = PINSEL_FUNC_1;
    pin_config.pinMode = PINSEL_TRISTATE;
    pin_config.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&pin_config);
    
    /* P0.3 - RXD0 */
    pin_config.pinNum = PINSEL_PIN_3;
    PINSEL_ConfigPin(&pin_config);
    
    /* Habilitar UART0 en PCONP */
    LPC_SC->PCONP |= (1 << 3);
    
    /* Configurar UART0 directamente por registros: 9600 bps, 8N1 */
    uint32_t pclk = SystemCoreClock / 4;  // PCLK para UART0
    uint32_t divisor_baudrate = pclk / (16 * 9600);
    
    LPC_UART0->LCR = 0x83;  /* 8N1, acceso a divisores de velocidad */
    LPC_UART0->DLL = (divisor_baudrate & 0xFF);
    LPC_UART0->DLM = ((divisor_baudrate >> 8) & 0xFF);
    LPC_UART0->LCR = 0x03;  /* 8N1, fin de acceso a divisores */
    LPC_UART0->FCR = 0x07;  /* Habilitar FIFO, limpiar TX/RX */
    
    /* Limpiar buffers */
    memset(buffer_rx_bt, 0, TAMAÑO_BUFFER_RX);
    contador_rx = 0;
    contador_lectura = 0;
}

/**
 * @brief Lee un carácter del buffer sin bloquear
 * @return Carácter recibido o -1 si no hay datos
 */
int bt_leer_caracter_no_bloqueante(void) {
    if (contador_lectura != contador_rx) {
        uint8_t caracter = buffer_rx_bt[contador_lectura];
        contador_lectura = (contador_lectura + 1) % TAMAÑO_BUFFER_RX;
        return (int)caracter;
    }
    return -1;
}

/**
 * @brief Escribe un carácter por UART0
 */
void bt_escribir_caracter(char caracter) {
    while (!(LPC_UART0->LSR & (1 << 5)));  /* Esperar a que THRE esté listo */
    LPC_UART0->THR = caracter;
}

/**
 * @brief Escribe una cadena por UART0
 */
void bt_escribir_cadena(const char *cadena) {
    while (*cadena) {
        bt_escribir_caracter(*cadena++);
    }
}

/**
 * @brief Procesa todos los comandos Bluetooth en el buffer
 */
void bt_procesar_comandos(void) {
    int caracter;
    while ((caracter = bt_leer_caracter_no_bloqueante()) != -1) {
        procesar_comando_bt((char)caracter);
    }
}

/**
 * @brief Procesa un comando Bluetooth individual
 * Mantiene el comando activo por varios ciclos para que sea detectado
 */
static void procesar_comando_bt(char comando) {
    comando = (comando >= 'a') ? (comando - 32) : comando;  /* Convertir a mayúscula */
    
    /* Resetear duración del comando */
    duracion_comando = 5;  /* Mantener comando activo ~5 ciclos */
    
    switch (comando) {
        case 'W':  /* Arriba */
            valor_x_simulado = 2048;
            valor_y_simulado = 200;
            break;
        case 'S':  /* Abajo */
            valor_x_simulado = 2048;
            valor_y_simulado = 3800;
            break;
        case 'A':  /* Izquierda */
            valor_x_simulado = 200;
            valor_y_simulado = 2048;
            break;
        case 'D':  /* Derecha */
            valor_x_simulado = 3800;
            valor_y_simulado = 2048;
            break;
        case 'B':  /* Botón */
            comando_boton = 1;
            bandera_boton_procesado = 0;
            break;
        default:
            break;
    }
}

/**
 * @brief Obtiene el valor simulado del eje X
 */
uint16_t bt_obtener_x_simulado(void) {
    return valor_x_simulado;
}

/**
 * @brief Obtiene el valor simulado del eje Y
 */
uint16_t bt_obtener_y_simulado(void) {
    return valor_y_simulado;
}

/**
 * @brief Obtiene el estado del botón Bluetooth
 */
uint8_t bt_obtener_comando_boton(void) {
    if (comando_boton && !bandera_boton_procesado) {
        bandera_boton_procesado = 1;
        return 1;
    }
    return 0;
}

/**
 * @brief Limpia el comando del botón
 */
void bt_limpiar_comando_boton(void) {
    comando_boton = 0;
    bandera_boton_procesado = 0;
}

/**
 * @brief Actualiza el buffer leyendo caracteres del UART
 * Se llama periódicamente desde el loop principal
 * Mantiene el comando activo brevemente después de recibirse
 */
void bt_actualizar_buffer(void) {
    /* Leer todos los caracteres disponibles en UART */
    while (LPC_UART0->LSR & (1 << 0)) {  /* Mientras hay datos disponibles */
        uint8_t caracter = (uint8_t)(LPC_UART0->RBR & 0xFF);
        buffer_rx_bt[contador_rx] = caracter;
        contador_rx = (contador_rx + 1) % TAMAÑO_BUFFER_RX;
        
        /* Procesar comando inmediatamente */
        procesar_comando_bt((char)caracter);
    }
    
    /* Decrementar duración del comando */
    if (duracion_comando > 0) {
        duracion_comando--;
        if (duracion_comando == 0) {
            /* Comando expiró, volver al centro */
            valor_x_simulado = 2048;
            valor_y_simulado = 2048;
        }
    }
}

