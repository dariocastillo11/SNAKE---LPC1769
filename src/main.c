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
    lcd_init();      // Inicializa el LCD

    lcd_borrarPantalla();     // Limpia la pantalla
    lcd_setCursor(0, 0); lcd_escribir("  dariioI");
    lcd_setCursor(1, 0); lcd_escribir("  Addr: 0x27     ");
    lcd_setCursor(2, 0); lcd_escribir("LPC1769 Test   ");
    lcd_setCursor(3, 0); lcd_escribir("POR");

    while (1) {
        // Aquí podría ir la lógica principal del programa
    }
}

/**
 * @brief Configura los pines P0.0 y P0.1 para I2C1.
 */
void cfgPin(void) {
    PINSEL_CFG_Type PinCfg;
    PinCfg.openDrain = 0;
    PinCfg.pinMode = 0;

    PinCfg.portNum = 0;
    PinCfg.pinNum = 0; // SDA1
    PinCfg.funcNum = 3;
    PINSEL_ConfigPin(&PinCfg);

    PinCfg.pinNum = 1; // SCL1
    PINSEL_ConfigPin(&PinCfg);
}

/**
 * @brief Inicializa el periférico I2C1 a 100kHz y lo habilita.
 */
void cfgI2c(void) {
    LPC_I2C_TypeDef* I2CDEV = LPC_I2C1;
    I2C_Init(I2CDEV, 100000);
    I2C_Cmd(I2CDEV, ENABLE);
}