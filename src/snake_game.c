/**
 * @file snake_game.c
 * @brief Implementación del juego Snake para LCD I2C
 *
 * Juego Snake completo con:
 * - Serpiente que crece al comer
 * - Control con joystick (4 direcciones)
 * - Detección de colisiones
 * - Sistema de puntuación
 * - Pantalla de Game Over con opción de volver al menú
 *
 * @date Noviembre 2025
 */

#include "snake_game.h"
#include "lcd_i2c.h"
#include "joystick_adc.h"
#include "melodias_dac.h"
#include "bluetooth_uart.h"  // Comandos Bluetooth
#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include <string.h>
#include <stdlib.h>

/* === CONFIGURACIÓN DEL JUEGO === */
#define COLUMNAS_LCD_SERPIENTE 20        // Ancho del LCD
#define FILAS_LCD_SERPIENTE 4         // Alto del LCD
#define LONGITUD_MAXIMA_SERPIENTE 50      // Longitud máxima de la serpiente
#define TICK_MS_SERPIENTE 50         // Período de tick (50ms)
#define TICKS_VELOCIDAD_SERPIENTE 10      // Ticks entre movimientos (500ms inicial - más lento)

/* === ESTRUCTURAS === */
typedef struct {
    uint8_t x;
    uint8_t y;
} Posicion;

typedef enum {
    DIR_ARRIBA = 0,
    DIR_ABAJO = 1,
    DIR_IZQUIERDA = 2,
    DIR_DERECHA = 3
} Direccion;

/* === VARIABLES DE ESTADO === */
static Posicion snake[LONGITUD_MAXIMA_SERPIENTE];  // Cuerpo de la serpiente
static uint8_t snake_length = 3;          // Longitud actual
static Direccion direccion_actual = DIR_DERECHA;
static Direccion direccion_siguiente = DIR_DERECHA;
static Posicion comida;                   // Posición de la comida
static uint32_t score = 0;                // Puntuación
static uint8_t game_over = 0;             // 1 = juego terminado
static uint8_t game_started = 0;          // 1 = juego iniciado
static uint8_t paused = 0;                // 1 = juego pausado
static volatile uint8_t tick_flag = 0;    // Flag de tick del timer
static uint8_t move_counter = 0;          // Contador para velocidad
static uint8_t speed_ticks = TICKS_VELOCIDAD_SERPIENTE;
static uint8_t buffer_lcd[FILAS_LCD_SERPIENTE][COLUMNAS_LCD_SERPIENTE + 1]; // Buffer para LCD

/* === FUNCIONES AUXILIARES === */

/**
 * @brief Generador pseudo-aleatorio simple
 */
static uint16_t prng_seed = 0xACE1u;
static uint8_t rand_range(uint8_t min, uint8_t max) {
    prng_seed = (prng_seed >> 1) ^ (-(prng_seed & 1u) & 0xB400u);
    return min + (prng_seed % (max - min + 1));
}

/**
 * @brief Genera nueva posición de comida (evitando la serpiente)
 * 
 * Genera coordenadas aleatorias dentro del área del LCD y verifica que
 * no coincidan con ningún segmento del cuerpo de la serpiente. Si hay
 * colisión, regenera hasta encontrar una posición válida.
 */
static void generar_comida(void) {
    uint8_t valida;
    do {
        valida = 1;
        comida.x = rand_range(0, COLUMNAS_LCD_SERPIENTE - 1);
        comida.y = rand_range(0, FILAS_LCD_SERPIENTE - 1);
        
        // Verificar que no esté sobre la serpiente
        for (uint8_t i = 0; i < snake_length; i++) {
            if (snake[i].x == comida.x && snake[i].y == comida.y) {
                valida = 0;
                break;
            }
        }
    } while (!valida);
}

/**
 * @brief Inicializa el estado del juego
 * 
 * Resetea todas las variables a sus valores iniciales:
 * - Serpiente de 3 segmentos en el centro del LCD
 * - Dirección inicial hacia la derecha
 * - Puntuación en cero
 * - Velocidad inicial (TICKS_VELOCIDAD_SERPIENTE = 6)
 * - Genera primera comida en posición aleatoria
 */
static void inicializar_estado(void) {
    // Serpiente inicial en el centro
    snake[0].x = 10;
    snake[0].y = 2;
    snake[1].x = 9;
    snake[1].y = 2;
    snake[2].x = 8;
    snake[2].y = 2;
    
    snake_length = 3;
    direccion_actual = DIR_DERECHA;
    direccion_siguiente = DIR_DERECHA;
    score = 0;
    game_over = 0;
    game_started = 1;
    paused = 0;
    move_counter = 0;
    speed_ticks = TICKS_VELOCIDAD_SERPIENTE;
    
    generar_comida();
}

/**
 * @brief Limpia el buffer del LCD
 * 
 * Rellena todas las posiciones del buffer con espacios (' ') y
 * agrega terminadores nulos al final de cada fila. Esto prepara
 * el buffer para dibujar un nuevo frame del juego.
 */
static void limpiar_buffer(void) {
    for (uint8_t y = 0; y < FILAS_LCD_SERPIENTE; y++) {
        memset(buffer_lcd[y], ' ', COLUMNAS_LCD_SERPIENTE);
        buffer_lcd[y][COLUMNAS_LCD_SERPIENTE] = '\0';
    }
}

/**
 * @brief Dibuja el juego en el buffer
 * 
 * Renderiza el estado actual del juego en el buffer de memoria:
 * - Serpiente: 'O' para la cabeza, 'o' para el cuerpo
 * - Comida: '*'
 * - Espacios vacíos: ' '
 * 
 * El buffer se actualiza completamente en cada frame para evitar
 * artefactos visuales. Luego se envía al LCD con actualizar_lcd().
 */
static void dibujar_en_buffer(void) {
    limpiar_buffer();
    
    // Dibujar serpiente
    for (uint8_t i = 0; i < snake_length; i++) {
        if (snake[i].x < COLUMNAS_LCD_SERPIENTE && snake[i].y < FILAS_LCD_SERPIENTE) {
            buffer_lcd[snake[i].y][snake[i].x] = (i == 0) ? 'O' : 'o';  // Cabeza 'O', cuerpo 'o'
        }
    }
    
    // Dibujar comida
    if (comida.x < COLUMNAS_LCD_SERPIENTE && comida.y < FILAS_LCD_SERPIENTE) {
        buffer_lcd[comida.y][comida.x] = '*';
    }
}

/**
 * @brief Envía el buffer al LCD
 * 
 * Transfiere el contenido del buffer de memoria al LCD físico,
 * fila por fila. Esta separación entre buffer y LCD permite
 * preparar todo el frame antes de mostrarlo, evitando parpadeos.
 */
static void actualizar_lcd(void) {
    for (uint8_t y = 0; y < FILAS_LCD_SERPIENTE; y++) {
        lcd_establecer_cursor(y, 0);
        lcd_escribir((char*)buffer_lcd[y]);
    }
}

/**
 * @brief Lee el estado del botón P0.4 o comando Bluetooth
 * 
 * El botón físico está conectado a GND con pull-up interno (activo BAJO):
 * - Pin LOW (0) cuando está presionado → retorna 1
 * - Pin HIGH (1) cuando NO está presionado → retorna 0
 * 
 * También verifica si hay comando 'B' desde Bluetooth.
 * 
 * @return 1 si está presionado (físico o Bluetooth), 0 si no
 */
static uint8_t leer_boton_p04(void) {
    /* Leer botón físico P0.4 */
    uint8_t physical = (LPC_GPIO0->FIOPIN & (1u << 4)) ? 0 : 1;
    
    /* Leer comando Bluetooth */
    uint8_t bt_button = bt_obtener_comando_boton();
    
    /* Si cualquiera está presionado, retornar 1 */
    if (bt_button) {
        bt_limpiar_comando_boton();  // Limpiar después de leer
        return 1;
    }
    
    return physical;
}

/**
 * @brief Procesa entrada del joystick
 * 
 * Lee los ejes X e Y del joystick analógico (valores ADC 0-4095):
 * - Eje Y < 500: Joystick arriba → DIR_ARRIBA
 * - Eje Y > 3500: Joystick abajo → DIR_ABAJO
 * - Eje X < 500: Joystick izquierda → DIR_IZQUIERDA
 * - Eje X > 3500: Joystick derecha → DIR_DERECHA
 * 
 * Previene giros de 180° (no permite cambiar a dirección opuesta).
 * 
 * El botón P0.4 pausa/reanuda el juego (edge detection para evitar
 * múltiples activaciones por una sola pulsación).
 */
static void procesar_entrada(void) {
    uint16_t ejeX = joystick_leer_adc(0);
    uint16_t ejeY = joystick_leer_adc(1);
    
    // Cambiar dirección según joystick (evitando 180°)
    if (ejeY < 500 && direccion_actual != DIR_ABAJO) {
        direccion_siguiente = DIR_ARRIBA;
    } else if (ejeY > 3500 && direccion_actual != DIR_ARRIBA) {
        direccion_siguiente = DIR_ABAJO;
    } else if (ejeX < 500 && direccion_actual != DIR_DERECHA) {
        direccion_siguiente = DIR_IZQUIERDA;
    } else if (ejeX > 3500 && direccion_actual != DIR_IZQUIERDA) {
        direccion_siguiente = DIR_DERECHA;
    }
    
    // Botón: pausar/reanudar
    static uint8_t boton_anterior = 0;
    uint8_t boton_actual = leer_boton_p04();
    if (boton_actual && !boton_anterior) {
        paused = !paused;
    }
    boton_anterior = boton_actual;
}

/**
 * @brief Mueve la serpiente
 * 
 * Secuencia de actualización:
 * 1. Calcula nueva posición de la cabeza según dirección actual
 * 2. Detecta colisiones:
 *    - Paredes (límites del LCD) → game_over
 *    - Propio cuerpo → game_over
 * 3. Verifica si comió:
 *    - Incrementa score
 *    - Aumenta longitud (hasta LONGITUD_MAXIMA_SERPIENTE)
 *    - Genera nueva comida
 *    - Aumenta velocidad cada 5 comidas (reduce speed_ticks)
 * 4. Mueve el cuerpo (desde la cola hacia adelante)
 * 5. Coloca nueva cabeza
 * 
 * Si hay colisión, reproduce melodía de game over y detiene el juego.
 */
static void mover_serpiente(void) {
    direccion_actual = direccion_siguiente;
    
    // Nueva posición de la cabeza
    Posicion nueva_cabeza = snake[0];
    switch (direccion_actual) {
        case DIR_ARRIBA:    nueva_cabeza.y--; break;
        case DIR_ABAJO:     nueva_cabeza.y++; break;
        case DIR_IZQUIERDA: nueva_cabeza.x--; break;
        case DIR_DERECHA:   nueva_cabeza.x++; break;
    }
    
    // Verificar colisión con paredes
    if (nueva_cabeza.x >= COLUMNAS_LCD_SERPIENTE || nueva_cabeza.y >= FILAS_LCD_SERPIENTE) {
        game_over = 1;
        melodias_iniciar(melodia_game_over);  // Cambiar a melodía de Game Over
        return;
    }
    
    // Verificar colisión con el propio cuerpo
    for (uint8_t i = 0; i < snake_length; i++) {
        if (snake[i].x == nueva_cabeza.x && snake[i].y == nueva_cabeza.y) {
            game_over = 1;
            melodias_iniciar(melodia_game_over);  // Cambiar a melodía de Game Over
            return;
        }
    }
    
    // Verificar si comió
    uint8_t comio = (nueva_cabeza.x == comida.x && nueva_cabeza.y == comida.y);
    
    if (comio) {
        score++;
        if (snake_length < LONGITUD_MAXIMA_SERPIENTE) {
            snake_length++;
        }
        generar_comida();
        
        // NO reproducir efecto (interrumpe música de fondo)
        // melodias_iniciar(melodia_salto);
        
        // Aumentar velocidad cada 5 comidas
        if (speed_ticks > 2 && score % 5 == 0) {
            speed_ticks--;
        }
    }
    
    // Mover el cuerpo (desde la cola hacia la cabeza)
    for (uint8_t i = snake_length - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    
    // Colocar nueva cabeza
    snake[0] = nueva_cabeza;
}

/**
 * @brief Muestra pantalla de Game Over
 * 
 * Renderiza en el LCD:
 * - Línea 0: "GAME OVER!"
 * - Línea 1: "Puntuacion: XXX" (score final)
 * - Línea 3: "Boton:Volver al menu" (instrucción para el usuario)
 * 
 * Convierte el score (uint32_t) a string manualmente para mostrar.
 */
static void mostrar_game_over(void) {
    lcd_borrarPantalla();
    lcd_establecer_cursor(0, 0);
    lcd_escribir("   GAME OVER!");
    lcd_establecer_cursor(1, 0);
    lcd_escribir("  Puntuacion: ");
    
    char score_str[10];
    uint8_t idx = 0;
    uint32_t temp_score = score;
    if (temp_score == 0) {
        score_str[idx++] = '0';
    } else {
        char temp[10];
        uint8_t temp_idx = 0;
        while (temp_score > 0) {
            temp[temp_idx++] = '0' + (temp_score % 10);
            temp_score /= 10;
        }
        for (int8_t i = temp_idx - 1; i >= 0; i--) {
            score_str[idx++] = temp[i];
        }
    }
    score_str[idx] = '\0';
    lcd_escribir(score_str);
    
    lcd_establecer_cursor(3, 0);
    lcd_escribir("Boton:Volver al menu");
}

/* === CONFIGURACIÓN DEL TIMER === */
/**
 * @brief Configura TIMER3 para generar ticks de juego cada 50ms
 * 
 * Configuración:
 * - Prescaler: 1000 (1ms por tick del timer)
 * - Match value: 50 (50 ticks = 50ms)
 * - Reset automático en match (contador continuo)
 * - Interrupción habilitada (setea tick_flag en ISR)
 * 
 * Frecuencia resultante: 20 Hz (20 ticks por segundo)
 */
static void config_timer(void) {
    TIM_TIMERCFG_Type timer_cfg;
    TIM_MATCHCFG_Type match_cfg;
    
    // Configurar timer a 50ms (usando prescaler en microsegundos)
    timer_cfg.prescaleOption = TIM_USVAL;
    timer_cfg.prescaleValue = 1000;  // 1ms por tick
    TIM_Init(LPC_TIM3, TIM_TIMER_MODE, &timer_cfg);
    
    // Match cada 50 ticks = 50ms
    match_cfg.matchChannel = TIM_MATCH_CHANNEL_0;
    match_cfg.intOnMatch = ENABLE;
    match_cfg.resetOnMatch = ENABLE;
    match_cfg.stopOnMatch = DISABLE;
    match_cfg.extMatchOutputType = TIM_NOTHING;
    match_cfg.matchValue = 50;  // 50ms
    TIM_ConfigMatch(LPC_TIM3, &match_cfg);
    
    NVIC_EnableIRQ(TIMER3_IRQn);
    TIM_Cmd(LPC_TIM3, ENABLE);
}

/* === MANEJADOR DE INTERRUPCIÓN === */
/**
 * @brief Handler de interrupción de TIMER3
 * 
 * Se ejecuta cada 50ms cuando TIMER3 alcanza el valor de match.
 * Setea tick_flag = 1 para indicar al main loop que debe procesar
 * un tick de juego. Mantiene la ISR breve (solo setea flag).
 */
void TIMER3_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM3, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM3, TIM_MR0_INT);
        tick_flag = 1;
    }
}

/* === FUNCIONES PÚBLICAS === */

/**
 * @brief Inicializa el juego Snake
 * 
 * Llamar esta función una vez antes de entrar al loop del juego.
 * Inicializa el estado del juego, configura TIMER3, borra la pantalla
 * y dibuja el primer frame.
 */
void juego_serpiente_inicializar(void) {
    inicializar_estado();
    config_timer();
    
    lcd_borrarPantalla();
    dibujar_en_buffer();
    actualizar_lcd();
}

/**
 * @brief Loop principal del juego Snake
 * 
 * Debe ser llamada continuamente desde el main loop. Procesa un tick
 * de juego cada vez que tick_flag está seteado (cada 50ms).
 * 
 * Estados:
 * - game_over == 0: Juego activo
 *   - Procesa entrada (joystick + botón)
 *   - Si pausado: muestra "PAUSA"
 *   - Si no pausado: mueve serpiente cada speed_ticks
 * 
 * - game_over == 1: Pantalla de Game Over
 *   - Muestra puntuación final
 *   - Espera botón P0.4 para volver al menú
 * 
 * - game_over == 2: Volver al menú (solicitado por usuario)
 *   - El main loop debe detectar este estado y cambiar a menú
 */
void juego_serpiente_ejecutar(void) {
    if (!game_started) return;
    
    if (game_over == 1) {
        static uint8_t game_over_mostrado = 0;
        
        if (!game_over_mostrado) {
            mostrar_game_over();
            game_over_mostrado = 1;
        }
        
        // Esperar botón P0.4 para volver al menú
        static uint8_t boton_anterior = 0;
        uint8_t boton_actual = leer_boton_p04();
        if (boton_actual && !boton_anterior) {
            // Usuario quiere volver al menú
            game_over = 2;  // Estado especial: volver al menú solicitado
            game_over_mostrado = 0;
        }
        boton_anterior = boton_actual;
        return;
    }
    
    if (!tick_flag) return;
    tick_flag = 0;
    
    procesar_entrada();
    
    if (paused) {
        // Mostrar indicador de pausa
        lcd_establecer_cursor(0, 0);
        lcd_escribir("PAUSA");
        return;
    }
    
    move_counter++;
    if (move_counter >= speed_ticks) {
        move_counter = 0;
        mover_serpiente();
        dibujar_en_buffer();
        actualizar_lcd();
    }
}

/**
 * @brief Reinicia el juego Serpiente
 * 
 * Resetea todas las variables al estado inicial sin borrar la pantalla
 * (el main loop se encarga de eso antes de volver al menú).
 * Útil para reiniciar después de game over.
 */
void juego_serpiente_reiniciar(void) {
    inicializar_estado();
    // No borrar pantalla aquí - el main lo hace antes de volver al menú
}

/**
 * @brief Retorna el estado del juego
 * 
 * @return Estado del juego:
 *         - 0: Jugando normalmente
 *         - 1: Game over (mostrando pantalla de game over)
 *         - 2: Usuario solicitó volver al menú (presionó botón en game over)
 */
uint8_t juego_serpiente_ha_terminado(void) {
    // Retorna: 0 = jugando, 1 = game over (pantalla mostrada), 2 = volver al menú
    return game_over;
}

/**
 * @brief Obtiene la puntuación actual
 * 
 * @return Puntuación actual (número de comidas consumidas)
 */
uint32_t juego_serpiente_obtener_puntuacion(void) {
    return score;
}
