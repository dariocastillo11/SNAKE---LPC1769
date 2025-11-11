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
#define PIN_EJE_X       23      // AD0.0 (P0.23) -> eje X del joystick
#define PIN_EJE_Y       24      // AD0.1 (P0.24) -> eje Y del joystick
#define PIN_BOTON       10      // P2.10 -> botón del joystick (interrupción)

// Asignación de LEDs a pines GPIO según especificación:
// P0.9, P0.8, P0.7, P0.6, P0.0
#define LED_ARRIBA      9       // P0.9 -> LED indicador ARRIBA
#define LED_ABAJO       8       // P0.8 -> LED indicador ABAJO
#define LED_IZQUIERDA   7       // P0.7 -> LED indicador IZQUIERDA
#define LED_DERECHA     6       // P0.6 -> LED indicador DERECHA
#define LED_BOTON       0       // P0.0 -> LED indicador BOTÓN/CENTRO
#define LED_ACTIVO_BAJO 0       // 0 = LEDs se encienden con 1 (activo alto)

/* === UMBRALES DEL ADC === */
// Los valores del ADC van de 0 a 4095 (12 bits)
// Joystick arriba:    Y ≈ 7
// Joystick abajo:     Y ≈ 4095
// Joystick izquierda: X ≈ 6
// Joystick derecha:   X ≈ 4095
#define UMBRAL_MINIMO_ADC   100     // Umbral para detectar posición extrema baja
#define UMBRAL_MAXIMO_ADC   4000    // Umbral para detectar posición extrema alta
#define CENTRO_ADC          2048    // Centro teórico del ADC
#define ZONA_MUERTA_ADC     500     // Zona muerta alrededor del centro (±500, aumentado)
#define MUESTRAS_PROMEDIO   4       // Número de muestras para promedio

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
    if (LED_ACTIVO_BAJO) {
        // Activo bajo: escribir 1 apaga
        LPC_GPIO0->FIOSET = (1U << LED_ARRIBA) | (1U << LED_ABAJO) | 
                            (1U << LED_IZQUIERDA) | (1U << LED_DERECHA) | (1U << LED_BOTON);
    } else {
        // Activo alto: escribir 0 apaga
        LPC_GPIO0->FIOCLR = (1U << LED_ARRIBA) | (1U << LED_ABAJO) | 
                            (1U << LED_IZQUIERDA) | (1U << LED_DERECHA) | (1U << LED_BOTON);
    }
}

/**
 * @brief Enciende un LED específico.
 * @param led Número de pin del LED
 */
static inline void led_on(uint32_t led) {
    if (LED_ACTIVO_BAJO)
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
    LPC_GPIO0->FIODIR |= (1U << LED_ARRIBA) | (1U << LED_ABAJO) | 
                         (1U << LED_IZQUIERDA) | (1U << LED_DERECHA) | (1U << LED_BOTON);

    // Apagar todos los LEDs al inicio
    leds_all_off();
}

/**
 * @brief Configura el botón del joystick con interrupción.
 */
static void config_gpio_interrupt(void) {
    // Configurar P2.10 como entrada con interrupción
    LPC_PINCON->PINSEL4 &= ~(3 << 20);   // P2.10 como GPIO
    LPC_GPIO2->FIODIR   &= ~(1 << PIN_BOTON);// Como entrada
    LPC_GPIOINT->IO2IntEnF |= (1 << PIN_BOTON); // Interrumpir en flanco de bajada
    LPC_GPIOINT->IO2IntClr  = (1 << PIN_BOTON); // Limpiar banderas pendientes
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
        led_on(LED_BOTON);
        return;
    }

    // Eje Y (vertical):
    if (y < UMBRAL_MINIMO_ADC) {
        // Joystick hacia arriba
        led_on(LED_ARRIBA);
    } else if (y > UMBRAL_MAXIMO_ADC) {
        // Joystick hacia abajo
        led_on(LED_ABAJO);
    } else if (x < UMBRAL_MINIMO_ADC) {
        // Joystick hacia izquierda
        led_on(LED_IZQUIERDA);
    } else if (x > UMBRAL_MAXIMO_ADC) {
        // Joystick hacia derecha
        led_on(LED_DERECHA);
    }
    // Si no se cumple ninguna condición, los LEDs quedan apagados (centro)
}

/* === FUNCIONES PÚBLICAS === */

void joystick_inicializar(void) {
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
void joystick_actualizar(void) {
    /* Procesar comandos Bluetooth (actualiza valores simulados) */
    bt_procesar_comandos();
    
    /* Leer valores simulados de Bluetooth */
    uint16_t bt_x = bt_obtener_x_simulado();
    uint16_t bt_y = bt_obtener_y_simulado();
    
    /* Si Bluetooth está en centro (2048), leer joystick físico */
    if (bt_x == 2048 && bt_y == 2048) {
        /* Leer ADC físico con promediado para reducir ruido */
        uint32_t temp;
        uint32_t sum_x = 0, sum_y = 0;
        
        // Tomar múltiples muestras y promediar
        for (uint8_t i = 0; i < MUESTRAS_PROMEDIO; i++) {
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
        uint16_t raw_x = (uint16_t)(sum_x / MUESTRAS_PROMEDIO);
        uint16_t raw_y = (uint16_t)(sum_y / MUESTRAS_PROMEDIO);
        
        /* Filtro adicional: promedio móvil con valor anterior (suavizado) */
        raw_x = (prev_ejeX + raw_x) / 2;
        raw_y = (prev_ejeY + raw_y) / 2;
        prev_ejeX = raw_x;
        prev_ejeY = raw_y;
        
        /* Aplicar zona muerta (deadzone) AMPLIADA para filtrar ruido del ADC
           Si el valor está dentro de ±ZONA_MUERTA_ADC del centro, forzar a centro */
        if (raw_x > (CENTRO_ADC - ZONA_MUERTA_ADC) && raw_x < (CENTRO_ADC + ZONA_MUERTA_ADC)) {
            ejeX = CENTRO_ADC;  // Forzar a centro (sin ruido)
        } else {
            ejeX = raw_x;  // Usar valor real
        }
        
        if (raw_y > (CENTRO_ADC - ZONA_MUERTA_ADC) && raw_y < (CENTRO_ADC + ZONA_MUERTA_ADC)) {
            ejeY = CENTRO_ADC;  // Forzar a centro (sin ruido)
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
/**
 * @brief Ejecuta un reset por software del chip
 * Reinicia el microcontrolador
 */
void joystick_hacer_reset(void) {
    NVIC_SystemReset();  // Reset por software
}

void EINT3_IRQHandler(void) {
    if (LPC_GPIOINT->IO2IntStatF & (1 << PIN_BOTON)) {
        joystick_hacer_reset();  // Ejecuta reset del chip
        LPC_GPIOINT->IO2IntClr = (1 << PIN_BOTON);
    }
}
