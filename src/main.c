/**
 * @file main.c
 * @brief Archivo principal del proyecto. Configura el hardware y muestra texto en el LCD.
 *
 * Este archivo contiene la función principal y la configuración de pines e I2C
 * para el microcontrolador LPC1769. Utiliza la librería lcd_i2c para controlar el display.
 * 
 * Soporta control dual: joystick físico + comandos Bluetooth (UART0 en P0.2/P0.3).
 */

#include "LPC17xx.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lcd_i2c.h"
#include "dino_game.h"
#include "snake_game.h"     // Juego Snake
#include "menu_juegos.h"    // Sistema de menú
#include "melodias_dac.h"   // Sistema de melodías
#include "joystick_adc.h"   // Control de joystick con ADC
#include "bluetooth_uart.h" // Comunicación Bluetooth (UART0)
#include "lpc17xx_timer.h"
#define DIRECCION_LCD 0x27

/**
 * @brief Configura los pines necesarios para la comunicación I2C1.
 * Utiliza P0.0 (SDA1) y P0.1 (SCL1) en modo función 3.
 */
void cfgPin(void);

/**
 * @brief Inicializa el periférico I2C1 a 100kHz.
 */
void cfgI2c(void);

int main(void) {
    SystemInit();    // Inicializa el sistema y los relojes
    cfgPin();        // Configura los pines
    cfgI2c();        // Inicializa el periférico I2C
    bt_init();       // Inicializa Bluetooth UART0 (P0.2 TX, P0.3 RX, 9600 bps)
    melodias_init(); // Inicializa sistema de melodías (DAC + Timer0/Timer1)
    joystick_init(); // Inicializa joystick ADC y LEDs indicadores
    lcd_init();      // Inicializa el LCD

    /* Enviar mensaje de bienvenida por Bluetooth */
    bt_write_str("\r\n=== DINOCHROME ARCADE ===\r\n");
    bt_write_str("Comandos: W(arriba) S(abajo) A(izq) D(der) B(boton)\r\n");
    bt_write_str("Conectado!\r\n\r\n");

    lcd_borrarPantalla();
    
    // Inicializar el menú de selección
    menu_init();
    
    // Estado del sistema
    int8_t juego_actual = -1;  // -1 = en menú, 0 = Dino, 1 = Snake
    uint8_t juego_inicializado = 0;
    int8_t musica_estado_anterior = -2;  // Para detectar cambios de estado

    while (1) {
        // Gestionar música de fondo según el estado
        if (juego_actual != musica_estado_anterior) {
            if (juego_actual == -1) {
                // Música de menú: Nokia
                melodias_iniciar_loop(melodia_nokia);
            } else if (juego_actual == 0) {
                // Música de Dino: Mario completo (melodia_fondo)
                melodias_iniciar_loop(melodia_fondo);
            } else if (juego_actual == 1) {
                // Música de Snake: Tetris
                melodias_iniciar_loop(melodia_tetris);
            }
            musica_estado_anterior = juego_actual;
        }
        
        if (juego_actual == -1) {
            /* === MODO MENÚ === */
            int8_t seleccion = menu_run();
            
            if (seleccion >= 0) {
                // Usuario seleccionó un juego
                juego_actual = seleccion;
                juego_inicializado = 0;  // Marcar para inicializar
            }
        } else {
            /* === MODO JUEGO === */
            
            // Inicializar juego si es necesario
            if (!juego_inicializado) {
                if (juego_actual == 0) {
                    dino_game_init();
                } else if (juego_actual == 1) {
                    snake_game_init();
                }
                juego_inicializado = 1;
            }
            
            // Ejecutar juego activo
            if (juego_actual == 0) {
                dino_game_run();
                
                // Si Dino terminó y usuario presionó botón, volver al menú
                if (dino_game_is_over() == 2) {
                    lcd_borrarPantalla();     // Primero borrar la pantalla
                    dino_game_restart();      // Reiniciar juego para próxima partida
                    juego_actual = -1;        // Volver al menú
                    juego_inicializado = 0;   // Marcar para re-inicializar
                    menu_reset();             // Dibujar menú (DESPUÉS de borrar)
                }
                
            } else if (juego_actual == 1) {
                snake_game_run();
                
                // Si Snake terminó y usuario presionó botón, volver al menú
                if (snake_game_is_over() == 2) {
                    lcd_borrarPantalla();     // Primero borrar la pantalla
                    snake_game_restart();     // Reiniciar juego para próxima partida
                    juego_actual = -1;        // Volver al menú
                    juego_inicializado = 0;   // Marcar para re-inicializar
                    menu_reset();             // Dibujar menú (DESPUÉS de borrar)
                }
            }
        }

        /* Actualizar sistema de melodías (no bloqueante) */
        melodias_actualizar();

        /* Actualizar joystick y LEDs indicadores (no bloqueante) */
        joystick_update();
    }
}

/**
 * @brief Configura los pines necesarios (USART).
 * NOTA: I2C1 (P0.0/P0.1) eliminado para liberar P0.0 para LED del joystick.
 */
void cfgPin(void) {
    // P0.0 y P0.1 ahora están disponibles como GPIO
    // La configuración de I2C0 (P0.27/P0.28) está en cfgI2c()

     //pin transmisor usart. p0.2
    PINSEL_CFG_Type pin_configuration;
    pin_configuration.portNum   = PINSEL_PORT_0;
    pin_configuration.pinNum    = PINSEL_PIN_2;
    pin_configuration.pinMode   = PINSEL_TRISTATE;
    pin_configuration.funcNum   = PINSEL_FUNC_1;  // USART0
    pin_configuration.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&pin_configuration);
    
    // Configurar P0.4 como entrada con PULL-UP para botón del joystick
    // El botón conecta a GND (sin resistencia externa), necesitamos pull-up
    pin_configuration.portNum   = PINSEL_PORT_0;
    pin_configuration.pinNum    = PINSEL_PIN_4;
    pin_configuration.pinMode   = PINSEL_PULLUP;     // Pull-up: 1 cuando no está presionado, 0 cuando presionado
    pin_configuration.funcNum   = PINSEL_FUNC_0;     // GPIO
    pin_configuration.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&pin_configuration);
    
    // Asegurar que P0.4 sea entrada
    LPC_GPIO0->FIODIR &= ~(1 << 4);
}

/**
 * @brief Inicializa el periférico I2C0 a 100kHz y lo habilita.
 * Configuración de pines:
 * - P0.27 como SDA0 (función 1)
 * - P0.28 como SCL0 (función 1)
 */
void cfgI2c(void) {
    PINSEL_CFG_Type pin_cfg;

    // Configurar P0.27 como SDA0 (función 1)
    pin_cfg.portNum = PINSEL_PORT_0;
    pin_cfg.pinNum = PINSEL_PIN_27;
    pin_cfg.funcNum = PINSEL_FUNC_1;  // SDA0
    pin_cfg.pinMode = PINSEL_TRISTATE;  // Sin pull-up/pull-down (I2C usa pull-up externo)
    pin_cfg.openDrain = PINSEL_OD_OPENDRAIN;  // Open-drain (requerido para I2C)
    PINSEL_ConfigPin(&pin_cfg);

    // Configurar P0.28 como SCL0 (función 1)
    pin_cfg.pinNum = PINSEL_PIN_28;
    pin_cfg.funcNum = PINSEL_FUNC_1;  // SCL0
    PINSEL_ConfigPin(&pin_cfg);

    // Inicializar I2C0 a 100kHz
    LPC_I2C_TypeDef* I2CDEV = LPC_I2C0;
    I2C_Init(I2CDEV, 100000);
    I2C_Cmd(I2CDEV, ENABLE);
}


