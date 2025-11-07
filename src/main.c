/**
 * @file main.c
 * @brief Archivo principal del proyecto. Configura el hardware y muestra texto en el LCD.
 *
 * Este archivo contiene la función principal y la configuración de pines e I2C
 * para el microcontrolador LPC1769. Utiliza la librería lcd_i2c para controlar el display.
 */

#include "LPC17xx.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lcd_i2c.h"
#include "dino_game.h"
#include "melodias_dac.h"  // Sistema de melodías
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
    cfgPin();        // Configura los pines para I2C
    cfgI2c();        // Inicializa el periférico I2C
    melodias_init(); // Inicializa sistema de melodías (DAC + Timer0/Timer1)
    lcd_init();      // Inicializa el LCD

    lcd_borrarPantalla();     // Limpia la pantalla

    /* Arrancar el juego inmediatamente: no escribimos textos estáticos para evitar
        que sobrescriban la pantalla del juego en el arranque. */
    dino_game_init();  // Esto deshabilitará TIMER0_IRQn internamente

    while (1) {
        /* Ejecutar lógica del juego en el bucle principal. No bloquea. */
        dino_game_run();
        
        /* Actualizar sistema de melodías (no bloqueante) */
        melodias_actualizar();
    }
}

/**
 * @brief Configura los pines P0.0 y P0.1 para I2C1.
 */
void cfgPin(void) {
    // Configuración de pines para I2C1
    PINSEL_CFG_Type PinCfg;
    PinCfg.openDrain = 0;
    PinCfg.pinMode = 0;

    PinCfg.portNum = 0;
    PinCfg.pinNum = 0; // SDA1
    PinCfg.funcNum = 3;
    PINSEL_ConfigPin(&PinCfg);

    PinCfg.pinNum = 1; // SCL1
    PINSEL_ConfigPin(&PinCfg);

     //pin transmisor usart. p0.2
    PINSEL_CFG_Type pin_configuration;
    pin_configuration.portNum   = PINSEL_PORT_0;
    pin_configuration.pinNum    = PINSEL_PIN_2;
    pin_configuration.pinMode   = PINSEL_TRISTATE;
    pin_configuration.funcNum   = PINSEL_FUNC_1;  // USART0
    pin_configuration.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&pin_configuration);
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


