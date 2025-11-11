/**
 * @file menu_juegos.c
 * @brief Implementación del menú de selección de juegos
 *
 * Menú navegable con joystick que muestra opciones de juegos en el LCD.
 * Usa un puntero ">" para indicar la opción seleccionada.
 * 
 * Soporta navegación por joystick físico y comandos Bluetooth.
 *
 * @date Noviembre 2025
 */

#include "menu_juegos.h"
#include "lcd_i2c.h"
#include "joystick_adc.h"
#include "bluetooth_uart.h"  // Comandos Bluetooth
#include "LPC17xx.h"
#include <string.h>

/* === CONFIGURACIÓN DEL MENÚ === */
#define MENU_DEBOUNCE_TICKS 15   // Ticks de debounce para navegación (evita saltos)

/* === NOMBRES DE LOS JUEGOS === */
static const char* nombres_juegos[NUM_JUEGOS] = {
    "1. DINO CHROME",
    "2. SNAKE"
};

/* === VARIABLES DE ESTADO === */
static uint8_t opcion_actual = 0;        // Opción seleccionada (0 = Dino, 1 = Snake)
static uint8_t menu_activo = 0;          // 1 = menú visible, 0 = juego activo
static uint8_t juego_seleccionado = 0;   // 1 = usuario presionó botón
static uint16_t debounce_counter = 0;    // Contador para debounce de navegación
static uint16_t last_ejeY = 2048;        // Último valor del eje Y (centro)

/* === FUNCIONES INTERNAS === */

/**
 * @brief Dibuja el menú en el LCD
 */
static void dibujar_menu(void) {
    lcd_borrarPantalla();
    
    // Título en la primera línea
    lcd_setCursor(0, 0);
    lcd_escribir("  SELECCIONA JUEGO");
    
    // Mostrar opciones con puntero ">"
    for (uint8_t i = 0; i < NUM_JUEGOS; i++) {
        lcd_setCursor(i + 1, 0);  // Opciones en líneas 1 y 2
        
        if (i == opcion_actual) {
            lcd_escribir("> ");  // Puntero en opción actual
        } else {
            lcd_escribir("  ");  // Sin puntero
        }
        
        lcd_escribir(nombres_juegos[i]);
    }
    
    // Instrucciones en la última línea
    lcd_setCursor(3, 0);
    lcd_escribir("Arriba/Abajo/Boton");
}

/**
 * @brief Lee el estado del botón P0.4 o comando Bluetooth
 * @return 1 si está presionado (físico o Bluetooth), 0 si no
 */
static uint8_t leer_boton_p04(void) {
    /* Leer botón físico P0.4 */
    uint8_t physical = (LPC_GPIO0->FIOPIN & (1u << 4)) ? 0 : 1;
    
    /* Leer comando Bluetooth */
    uint8_t bt_button = bt_get_button_command();
    
    /* Si cualquiera está presionado, retornar 1 */
    if (bt_button) {
        bt_clear_button_command();  // Limpiar después de leer
        return 1;
    }
    
    return physical;
}

/**
 * @brief Procesa la entrada del joystick para navegación
 * @return 1 si se seleccionó un juego, 0 si no
 */
static uint8_t procesar_entrada(void) {
    // Detectar presión del botón P0.4 PRIMERO (antes de cualquier debounce)
    static uint8_t boton_anterior = 0;
    static uint8_t boton_inicializado = 0;
    
    uint8_t boton_actual = leer_boton_p04();
    
    // En la primera lectura, inicializar boton_anterior sin detectar flanco
    if (!boton_inicializado) {
        boton_anterior = boton_actual;
        boton_inicializado = 1;
    } else {
        // Detectar flanco ascendente (0 -> 1)
        if (boton_actual && !boton_anterior) {
            // Flanco ascendente del botón: seleccionar juego
            juego_seleccionado = 1;
            debounce_counter = MENU_DEBOUNCE_TICKS;
            boton_anterior = boton_actual;
            return 1;
        }
        boton_anterior = boton_actual;
    }
    
    // Leer el joystick
    uint16_t ejeY = joystick_leer_adc(1);  // Leer eje Y
    
    // Debounce DEL JOYSTICK: evitar cambios muy rápidos
    if (debounce_counter > 0) {
        debounce_counter--;
        return 0;
    }
    
    // Detectar movimiento ARRIBA (Y < 500)
    if (ejeY < 500 && last_ejeY >= 500) {
        if (opcion_actual > 0) {
            opcion_actual--;
            dibujar_menu();
            debounce_counter = MENU_DEBOUNCE_TICKS;
        }
    }
    
    // Detectar movimiento ABAJO (Y > 3500)
    if (ejeY > 3500 && last_ejeY <= 3500) {
        if (opcion_actual < NUM_JUEGOS - 1) {
            opcion_actual++;
            dibujar_menu();
            debounce_counter = MENU_DEBOUNCE_TICKS;
        }
    }
    
    last_ejeY = ejeY;
    return 0;
}

/* === FUNCIONES PÚBLICAS === */

void menu_init(void) {
    opcion_actual = 0;
    menu_activo = 1;
    juego_seleccionado = 0;
    debounce_counter = 0;
    last_ejeY = 2048;
    dibujar_menu();
}

int8_t menu_run(void) {
    if (!menu_activo) {
        return -1;  // Menú no activo
    }
    
    if (procesar_entrada()) {
        // Usuario seleccionó un juego
        menu_activo = 0;
        
        // El juego se encargará de limpiar la pantalla y mostrar su contenido
        // No mostramos mensaje de carga porque puede quedarse pegado
        
        return (int8_t)opcion_actual;
    }
    
    return -1;  // Aún no hay selección
}

void menu_reset(void) {
    opcion_actual = 0;
    menu_activo = 1;
    juego_seleccionado = 0;
    debounce_counter = 0;
    last_ejeY = 2048;
    dibujar_menu();
}
