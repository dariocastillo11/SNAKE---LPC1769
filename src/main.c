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

#define PIN_BOTON 18 // P0.18
#define LCD_FILA_SUELO 3
#define LCD_FILA_AIRE 2
#define LCD_COL_DINO 2
#define LCD_COL_OBSTACULO 15

void cfgPin(void);
void cfgI2c(void);
void delay_ms(uint32_t ms);

int main(void) {
    SystemInit();
    cfgPin();
    cfgI2c();
    lcd_init();

    lcd_borrarPantalla();
    lcd_setCursor(0, 0); lcd_escribir("DINO LCD - Salta!");

    uint8_t dino_fila = LCD_FILA_SUELO;
    uint8_t obstaculo_col = LCD_COL_OBSTACULO;
    uint8_t saltando = 0;
    uint32_t tick = 0;

    while (1) {
        // Leer botón (P0.18)
        if (LPC_GPIO0->FIOPIN & (1 << PIN_BOTON)) {
            if (!saltando && dino_fila == LCD_FILA_SUELO) {
                saltando = 1;
                dino_fila = LCD_FILA_AIRE;
            }
        }

        // Borrar dinosaurio y obstáculo anteriores
        lcd_borrarFila(LCD_FILA_SUELO);
        lcd_borrarFila(LCD_FILA_AIRE);

        // Dibujar dinosaurio
        lcd_setCursor(dino_fila, LCD_COL_DINO);
        lcd_escribir("D"); // El dinosaurio

        // Dibujar obstáculo
        lcd_setCursor(LCD_FILA_SUELO, obstaculo_col);
        lcd_escribir("|"); // El cactus

        // Colisión
        if (obstaculo_col == LCD_COL_DINO && dino_fila == LCD_FILA_SUELO) {
            lcd_borrarPantalla();
            lcd_setCursor(1, 5);
            lcd_escribir("GAME OVER!");
            while (1); // Fin del juego
        }

        // Mover obstáculo
        if (tick % 2 == 0) { // Más lento
            if (obstaculo_col > 0)
                obstaculo_col--;
            else
                obstaculo_col = LCD_COL_OBSTACULO;
        }

        // Control de salto
        if (saltando) {
            delay_ms(400); // Tiempo en el aire
            dino_fila = LCD_FILA_SUELO;
            saltando = 0;
        }

        delay_ms(120);
        tick++;
    }
}

// Configura P0.18 como entrada para el botón
void cfgPin(void) {
    PINSEL_CFG_Type PinCfg;
    PinCfg.portNum = 0;
    PinCfg.pinNum = 18;
    PinCfg.funcNum = 0; // GPIO
    PinCfg.openDrain = 0;
    PinCfg.pinMode = 0; // Pull-up
    PINSEL_ConfigPin(&PinCfg);

    LPC_GPIO0->FIODIR &= ~(1 << PIN_BOTON); // Entrada
}

// Inicializa I2C1 y el LCD
void cfgI2c(void) {
    LPC_I2C_TypeDef* I2CDEV = LPC_I2C1;
    I2C_Init(I2CDEV, 100000);
    I2C_Cmd(I2CDEV, ENABLE);
}

// Retardo simple en ms (bloqueante)
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 8000; i++) {
        __NOP();
    }
}
