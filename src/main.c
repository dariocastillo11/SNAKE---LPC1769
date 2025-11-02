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
#include "lpc17xx_uart.h"
#include "lpc17xx_timer.h"
#define DIRECCION_LCD 0x27

/**
 * @brief Configura los pines necesarios para la comunicación I2C1.
 * Utiliza P0.0 (SDA1) y P0.1 (SCL1) en modo función 3.
 */
void cfgPin(void);
void cfgtimer(void);
void cfguart(void);
/**
 * @brief Inicializa el periférico I2C1 a 100kHz.
 */
void cfgI2c(void);

int main(void) {
    SystemInit();    // Inicializa el sistema y los relojes
    cfgPin();        // Configura los pines para I2C
    cfgI2c();        // Inicializa el periférico I2C
    cfgtimer();      // Timer0 para UART (será deshabilitado por dino_game_init)
    cfguart();
    lcd_init();      // Inicializa el LCD

    lcd_borrarPantalla();     // Limpia la pantalla

    /* Arrancar el juego inmediatamente: no escribimos textos estáticos para evitar
        que sobrescriban la pantalla del juego en el arranque. */
    dino_game_init();  // Esto deshabilitará TIMER0_IRQn internamente

    while (1) {
        /* Ejecutar lógica del juego en el bucle principal. No bloquea. */
        dino_game_run();
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
void cfgtimer(void){
	TIM_TIMERCFG_Type timer_config;
	TIM_MATCHCFG_Type match_config;

    timer_config.prescaleOption = TIM_USVAL;
    timer_config.prescaleValue = 1000; //1ms

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_config);

    //Configuracion del match
    match_config.matchChannel = TIM_MATCH_0;
    match_config.intOnMatch = ENABLE;
    match_config.resetOnMatch = ENABLE;
    match_config.stopOnMatch = DISABLE;
    match_config.extMatchOutputType = TIM_NOTHING;
    match_config.matchValue = 1000; // 1s quiero transmitir

    TIM_ConfigMatch(LPC_TIM0, &match_config);

    //Habilitar interrupcion?
    NVIC_EnableIRQ(TIMER0_IRQn);

    //Iniciar timer
    TIM_Cmd(LPC_TIM0, ENABLE);
}

void cfguart(void){
    UART_CFG_Type uart_config;

    uart_config.Baud_rate = 9600;
    uart_config.Databits = UART_DATABIT_8;
    uart_config.Parity = UART_PARITY_NONE;//sin paridad
    uart_config.Stopbits = UART_STOPBIT_1;//1 bit de stop
    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &uart_config);
//CARGO ESTRUCTURA

    UART_FIFO_CFG_Type fifo_config;

    fifo_config.FIFO_DMAMode = DISABLE;
    fifo_config.FIFO_Level = UART_FIFO_TRGLEV1;//4 caracteres
    fifo_config.FIFO_ResetRxBuf = ENABLE;//no lo uso
    fifo_config.FIFO_ResetTxBuf = ENABLE;
    UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &fifo_config);
//CARGO ESTRUCTURA



    //Habilitar transmisor
    UART_TxCmd((LPC_UART_TypeDef *)LPC_UART0, ENABLE);


}
/**
 * @brief Inicializa el periférico I2C1 a 100kHz y lo habilita.
 */
void cfgI2c(void) {
    LPC_I2C_TypeDef* I2CDEV = LPC_I2C1;
    I2C_Init(I2CDEV, 100000);
    I2C_Cmd(I2CDEV, ENABLE);
}

void TIMER0_IRQHandler(void){
    static const char mensaje[] = "Snake";
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)){
        UART_Send((LPC_UART_TypeDef *)LPC_UART0, (uint8_t *)mensaje, sizeof(mensaje) - 1, BLOCKING);
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        lcd_desplazarIzquierda();
    }
}

