/**
 * @file joystick_adc.c
 * @brief Implementación del control de joystick analógico con ADC y LEDs.
 *
 * Este módulo lee las posiciones X e Y del joystick mediante el ADC y
 * enciende LEDs indicadores según la dirección detectada. Compatible con
 * el proyecto DinoChrome sin conflictos con timers ni pines ya utilizados.
 *
 * Soporta control dual: joystick físico + comandos Bluetooth (UART0).
 * Los comandos Bluetooth sobrescriben el joystick físico cuando están activos.
 *
 * @date Noviembre 2025
 */

#include "joystick_adc.h"
#include "bluetooth_uart.h"
#include "LPC17xx.h"

/* === DEFINICIÓN DE PINES Y CONSTANTES === */

// Pines del ADC para el joystick:
#define VRX_PIN     23      // AD0.0 (P0.23) -> eje X del joystick
#define VRY_PIN     24      // AD0.1 (P0.24) -> eje Y del joystick
#define SW_PIN      10      // P2.10 -> botón del joystick (interrupción)

// Asignación de LEDs a pines GPIO según especificación:
// P0.9, P0.8, P0.7, P0.6, P0.0
#define LED_UP      9       // P0.9 -> LED indicador ARRIBA
#define LED_DOWN    8       // P0.8 -> LED indicador ABAJO
#define LED_LEFT    7       // P0.7 -> LED indicador IZQUIERDA
#define LED_RIGHT   6       // P0.6 -> LED indicador DERECHA
#define LED_BTN     0       // P0.0 -> LED indicador BOTÓN/CENTRO
#define LED_ACTIVE_LOW 0    // 0 = LEDs se encienden con 1 (activo alto)

/* === UMBRALES DEL ADC === */
// Los valores del ADC van de 0 a 4095 (12 bits)
// Joystick arriba:    Y ≈ 7
// Joystick abajo:     Y ≈ 4095
// Joystick izquierda: X ≈ 6
// Joystick derecha:   X ≈ 4095
#define ADC_MIN         100     // Umbral para detectar posición extrema baja
#define ADC_MAX         4000    // Umbral para detectar posición extrema alta
#define ADC_CENTER      2048    // Centro teórico del ADC
#define ADC_DEADZONE    500     // Zona muerta alrededor del centro (±500, aumentado)
#define ADC_SAMPLES     4       // Número de muestras para promedio

/* === VARIABLES GLOBALES === */
static volatile uint16_t ejeX = 2048;         // Última lectura del eje X (inicializar en centro)
static volatile uint16_t ejeY = 2048;         // Última lectura del eje Y (inicializar en centro)
static volatile uint8_t boton_presionado = 0; // Estado del botón
static uint16_t prev_ejeX = 2048;             // Valor anterior de X (para filtrado)
static uint16_t prev_ejeY = 2048;             // Valor anterior de Y (para filtrado)

/* === FUNCIONES INTERNAS === */

/**
 * @brief Apaga todos los LEDs del joystick.
 */
static inline void leds_all_off(void) {
    if (LED_ACTIVE_LOW) {
        // Activo bajo: escribir 1 apaga
        LPC_GPIO0->FIOSET = (1U << LED_UP) | (1U << LED_DOWN) | 
                            (1U << LED_LEFT) | (1U << LED_RIGHT) | (1U << LED_BTN);
    } else {
        // Activo alto: escribir 0 apaga
        LPC_GPIO0->FIOCLR = (1U << LED_UP) | (1U << LED_DOWN) | 
                            (1U << LED_LEFT) | (1U << LED_RIGHT) | (1U << LED_BTN);
    }
}

/**
 * @brief Enciende un LED específico.
 * @param led Número de pin del LED
 */
static inline void led_on(uint32_t led) {
    if (LED_ACTIVE_LOW)
        LPC_GPIO0->FIOCLR = (1U << led); // Activo bajo: 0 enciende
    else
        LPC_GPIO0->FIOSET = (1U << led); // Activo alto: 1 enciende
}

/**
 * @brief Configura el ADC para lectura de joystick.
 */
static void config_adc(void) {
    // 1. Encender el periférico ADC
    LPC_SC->PCONP |= (1 << 12);  // Bit PCADC en registro PCONP

    // 2. Configurar pines como entradas analógicas
    LPC_PINCON->PINSEL1 &= ~((3 << 14) | (3 << 16));  // Limpiar config previa
    LPC_PINCON->PINSEL1 |=  ((1 << 14) | (1 << 16));  // P0.23 y P0.24 como ADC

    // 3. Configurar el ADC:
    // - PDN = 1 (encendido)
    // - CLKDIV = 24 (frecuencia de muestreo = PCLK/(CLKDIV+1))
    LPC_ADC->ADCR = (1 << 21) | (24 << 8);
}

/**
 * @brief Configura los LEDs del joystick como salidas.
 */
static void config_gpio_leds(void) {
    // Configurar P0.0, P0.6-P0.9 como GPIO (función 00 en PINSEL)
    LPC_PINCON->PINSEL0 &= ~(3 << 0);   // P0.0
    LPC_PINCON->PINSEL0 &= ~(3 << 12);  // P0.6
    LPC_PINCON->PINSEL0 &= ~(3 << 14);  // P0.7
    LPC_PINCON->PINSEL0 &= ~(3 << 16);  // P0.8
    LPC_PINCON->PINSEL0 &= ~(3 << 18);  // P0.9

    // Configurar como salidas
    LPC_GPIO0->FIODIR |= (1U << LED_UP) | (1U << LED_DOWN) | 
                         (1U << LED_LEFT) | (1U << LED_RIGHT) | (1U << LED_BTN);

    // Apagar todos los LEDs al inicio
    leds_all_off();
}

/**
 * @brief Configura el botón del joystick con interrupción.
 */
static void config_gpio_interrupt(void) {
    // Configurar P2.10 como entrada con interrupción
    LPC_PINCON->PINSEL4 &= ~(3 << 20);   // P2.10 como GPIO
    LPC_GPIO2->FIODIR   &= ~(1 << SW_PIN);// Como entrada
    LPC_GPIOINT->IO2IntEnF |= (1 << SW_PIN); // Interrumpir en flanco de bajada
    LPC_GPIOINT->IO2IntClr  = (1 << SW_PIN); // Limpiar banderas pendientes
    NVIC_EnableIRQ(EINT3_IRQn);           // Habilitar interrupción
}

/**
 * @brief Muestra la dirección del joystick en los LEDs.
 * @param x Valor del eje X (0-4095)
 * @param y Valor del eje Y (0-4095)
 */
static void mostrar_direccion(uint16_t x, uint16_t y) {
    // Apagar todos los LEDs
    leds_all_off();

    // Prioridad: 1° botón, 2° eje Y, 3° eje X
    if (boton_presionado) {
        led_on(LED_BTN);
        return;
    }

    // Eje Y (vertical):
    if (y < ADC_MIN) {
        // Joystick hacia arriba
        led_on(LED_UP);
    } else if (y > ADC_MAX) {
        // Joystick hacia abajo
        led_on(LED_DOWN);
    } else if (x < ADC_MIN) {
        // Joystick hacia izquierda
        led_on(LED_LEFT);
    } else if (x > ADC_MAX) {
        // Joystick hacia derecha
        led_on(LED_RIGHT);
    }
    // Si no se cumple ninguna condición, los LEDs quedan apagados (centro)
}

/* === FUNCIONES PÚBLICAS === */

void joystick_init(void) {
    config_adc();
    config_gpio_leds();
    config_gpio_interrupt();
}

/**
 * @brief Actualiza las lecturas del joystick (físico + Bluetooth)
 * 
 * Prioridad:
 * 1. Si hay comandos Bluetooth activos, usar valores simulados
 * 2. Si no hay comandos Bluetooth, usar lecturas del joystick físico
 * 
 * Los valores combinados se almacenan en ejeX/ejeY para que los
 * juegos los lean de forma transparente.
 */
void joystick_update(void) {
    /* Procesar comandos Bluetooth (actualiza valores simulados) */
    bt_process_commands();
    
    /* Leer valores simulados de Bluetooth */
    uint16_t bt_x = bt_get_simulated_x();
    uint16_t bt_y = bt_get_simulated_y();
    
    /* Si Bluetooth está en centro (2048), leer joystick físico */
    if (bt_x == 2048 && bt_y == 2048) {
        /* Leer ADC físico con promediado para reducir ruido */
        uint32_t temp;
        uint32_t sum_x = 0, sum_y = 0;
        
        // Tomar múltiples muestras y promediar
        for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
            // Leer canal 0 (eje X)
            LPC_ADC->ADCR &= ~((0xFF) | (7 << 24));
            LPC_ADC->ADCR |= (1U << 0);
            LPC_ADC->ADCR |= (1U << 24);
            do { temp = LPC_ADC->ADDR0; } while (!(temp & (1U << 31)));
            sum_x += (uint16_t)((temp >> 4) & 0xFFF);
            
            // Leer canal 1 (eje Y)
            LPC_ADC->ADCR &= ~((0xFF) | (7 << 24));
            LPC_ADC->ADCR |= (1U << 1);
            LPC_ADC->ADCR |= (1U << 24);
            do { temp = LPC_ADC->ADDR1; } while (!(temp & (1U << 31)));
            sum_y += (uint16_t)((temp >> 4) & 0xFFF);
        }
        
        // Calcular promedios
        uint16_t raw_x = (uint16_t)(sum_x / ADC_SAMPLES);
        uint16_t raw_y = (uint16_t)(sum_y / ADC_SAMPLES);
        
        /* Filtro adicional: promedio móvil con valor anterior (suavizado) */
        raw_x = (prev_ejeX + raw_x) / 2;
        raw_y = (prev_ejeY + raw_y) / 2;
        prev_ejeX = raw_x;
        prev_ejeY = raw_y;
        
        /* Aplicar zona muerta (deadzone) AMPLIADA para filtrar ruido del ADC
           Si el valor está dentro de ±DEADZONE del centro, forzar a centro */
        if (raw_x > (ADC_CENTER - ADC_DEADZONE) && raw_x < (ADC_CENTER + ADC_DEADZONE)) {
            ejeX = ADC_CENTER;  // Forzar a centro (sin ruido)
        } else {
            ejeX = raw_x;  // Usar valor real
        }
        
        if (raw_y > (ADC_CENTER - ADC_DEADZONE) && raw_y < (ADC_CENTER + ADC_DEADZONE)) {
            ejeY = ADC_CENTER;  // Forzar a centro (sin ruido)
        } else {
            ejeY = raw_y;  // Usar valor real
        }
    } else {
        /* Usar valores simulados de Bluetooth (prioridad) */
        ejeX = bt_x;
        ejeY = bt_y;
    }
    
    /* Actualizar LEDs según la dirección final (físico o BT) */
    mostrar_direccion(ejeX, ejeY);
}

uint16_t joystick_leer_adc(uint8_t canal) {
    /* IMPORTANTE: Ahora retorna los valores ya procesados (BT + físico)
       almacenados en ejeX/ejeY por joystick_update() */
    
    if (canal == 0) {
        return ejeX;  // Retornar valor combinado del eje X
    } else if (canal == 1) {
        return ejeY;  // Retornar valor combinado del eje Y
    }
    
    return 2048;  // Centro por defecto para canales inválidos
}

uint8_t joystick_boton_presionado(void) {
    return boton_presionado;
}

/* === MANEJADOR DE INTERRUPCIÓN === */

/**
 * @brief Rutina de atención de interrupción para el botón del joystick.
 * 
 * NOTA: Esta función puede entrar en conflicto si ya existe otra implementación
 * de EINT3_IRQHandler en el proyecto. En ese caso, integrar el código dentro
 * de la función existente.
 */
void EINT3_IRQHandler(void) {
    // Verificar si la interrupción proviene de P2.10 (botón del joystick)
    if (LPC_GPIOINT->IO2IntStatF & (1 << SW_PIN)) {
        boton_presionado = !boton_presionado; // Alternar estado
        LPC_GPIOINT->IO2IntClr = (1 << SW_PIN);// Limpiar bandera
    }
}
