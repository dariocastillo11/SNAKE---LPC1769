/**
 * @file dino_game.c
 * @brief Implementación del mini-juego "Dinosaurio" usando LCD I2C + DAC melodías.
 *
 * Hardware:
 * - P0.4:  Botón de salto (GPIO input con pull-up)
 * - P0.27: SDA0 del LCD I2C
 * - P0.28: SCL0 del LCD I2C
 * - P0.26: Salida DAC para melodías (usado por melodias_dac.c)
 * 
 * Timers utilizados:
 * - TIMER0: Sistema de melodías DAC (melodias_dac.c) - Generación de audio
 * - TIMER1: Sistema de melodías DAC (melodias_dac.c) - Contador de tiempo
 * - TIMER2: Motor del juego (este archivo) - Ticks de 50ms (20 Hz)
 *
 * Arquitectura:
 * - TIMER2_IRQHandler setea un flag (game_tick_flag) cada 50ms
 * - El main loop procesa el tick: actualiza física, detección, dibuja
 * - Las funciones I2C/LCD se llaman SOLO desde el main loop (nunca desde ISR)
 * - Las melodías se actualizan con melodias_actualizar() (no bloqueante)
 *
 * @date Noviembre 2025
 */

#include "dino_game.h"
#include "lcd_i2c.h"
#include "melodias_dac.h"  // Sistema de melodías
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

#include <stdint.h>
#include <string.h>

/* ========================== CONFIGURACIÓN ================================= */

/* Pin del botón de salto */
#define DINO_BUTTON_PORT 0
#define DINO_BUTTON_PIN  4

/* Dimensiones del área de juego en caracteres LCD (20x4) */
#define DINO_COLS 20          // Ancho del LCD en caracteres
#define DINO_ROW_GROUND 3     // Fila inferior (0=marcadores, 1-3=juego)

/* Dimensiones del sprite del dinosaurio */
#define DINO_WIDTH 1          // Ancho en caracteres (optimizado)
#define DINO_HEIGHT 2         // Alto en caracteres (2 filas)

/* Configuración de timing del juego */
#define TICKS_PER_SEC 20      // Frecuencia de actualización (20 Hz)
#define TICK_MS 50            // Período de tick (50ms = 1000ms/20)

/* Configuración de dificultad del juego */
#define INIT_MOVE_INTERVAL 6       // Intervalo inicial entre movimientos (ticks)
#define MIN_MOVE_INTERVAL 2        // Intervalo mínimo (velocidad máxima)
#define BASE_SPAWN_THRESHOLD 40    // Umbral inicial de spawn de obstáculos (0-255)
#define MAX_SPAWN_THRESHOLD 80     // Umbral máximo de spawn

/* ========================== VARIABLES DE ESTADO ========================== */

/* Flag de tick del timer (seteado por TIMER2_IRQHandler cada 50ms) */
static volatile uint8_t game_tick_flag = 0;

/* Array de obstáculos (0 = vacío, 1-3 = tamaño del obstáculo) */
static uint8_t obstacles[DINO_COLS];

/* Posición del dinosaurio */
static uint8_t dino_col = 2;          /* Columna fija del dinosaurio */
static int8_t dino_vpos = 0;          /* Posición vertical (0=suelo, >0=aire) */
static int8_t dino_velocity = 0;      /* Velocidad vertical (sin usar actualmente) */
static int dino_row = DINO_ROW_GROUND; /* Fila actual del dinosaurio */

/* Generación de obstáculos */
static uint8_t spawn_counter = 0;
static uint32_t prng = 0xACE1u;       /* Semilla para PRNG (LFSR de 16 bits) */
static uint8_t last_obstacle_pos = 0; /* Posición del último obstáculo generado */

/* Estado del juego */
static uint8_t game_over = 0;         /* 1 = juego terminado */
static uint8_t game_started = 0;      /* 1 = juego iniciado por el usuario */
static uint32_t score = 0;            /* Puntuación actual */
static uint32_t ticks_since_start = 0; /* Ticks desde inicio del juego */

/* Control del botón */
static uint8_t button_held = 0;       /* 1 = botón presionado */
static uint8_t last_button_state = 1; /* Estado anterior del botón (1=released) */
static uint8_t debounce_cnt = 0;      /* Contador de debounce */
static uint8_t jump_requested = 0;    /* 1 = salto pendiente */

/* Ajuste dinámico de dificultad */
static uint8_t move_interval = INIT_MOVE_INTERVAL; /* Ticks entre movimientos */
static uint8_t move_counter = 0;                    /* Contador para move_interval */
static uint8_t spawn_threshold = BASE_SPAWN_THRESHOLD; /* Umbral de spawn (0-255) */

/* Animación del dinosaurio */
static int current_frame = 0;        /* Frame actual de animación (0 o 1) */
static int animation_counter = 0;    /* Contador para velocidad de animación */

/* ========================== SPRITES Y ANIMACIÓN ========================== */

/**
 * Frames de animación del dinosaurio caminando.
 * 
 * Cada frame es una matriz 2x2 de caracteres:
 * - Fila 0: Parte superior del dinosaurio (cabeza)
 * - Fila 1: Parte inferior del dinosaurio (patas)
 * 
 * Frame 0: 'D' arriba + 'A' abajo (pata izquierda adelante)
 * Frame 1: 'D' arriba + 'I' abajo (pata derecha adelante)
 * 
 * La alternancia entre frames crea efecto de caminata.
 */
static const char walking_frames[2][2][2] = {
    {   /* Frame 0 - D + A (caminando) */
        {'D', 'A'},  /* columna: D arriba, A abajo */
        {' ', ' '}   /* columna derecha: vacía (ancho de 1 char) */
    },
    {   /* Frame 1 - D + I (caminando) */
        {'D', 'I'},  /* columna: D arriba, I abajo */
        {' ', ' '}   /* columna derecha: vacía (ancho de 1 char) */
    }
};

/* ========================== FUNCIONES AUXILIARES ========================= */

/**
 * @brief Lee el estado del botón de salto.
 * 
 * El botón está configurado con pull-up interno, por lo que:
 * - 1 = no presionado (released)
 * - 0 = presionado
 * 
 * @return 1 si no está presionado, 0 si está presionado
 */
static int read_button(void) {
    return (LPC_GPIO0->FIOPIN & (1u << DINO_BUTTON_PIN)) ? 1 : 0;
}

/**
 * @brief Generador de números pseudo-aleatorios (PRNG) usando LFSR de 16 bits.
 * 
 * Implementa un Linear Feedback Shift Register para generar secuencia
 * pseudo-aleatoria. Usado para spawning de obstáculos.
 * 
 * @return Número pseudo-aleatorio de 16 bits
 */
static uint16_t lfsr16(void) {
    uint16_t l = (uint16_t)prng;
    uint16_t lsb = l & 1u;
    l >>= 1;
    if (lsb) l ^= 0xB400u;
    prng = l;
    return l;
}

/**
 * @brief Dibuja el frame completo del juego en el LCD.
 * 
 * Renderiza fila por fila (filas 1-3, la 0 es para marcadores):
 * - Dinosaurio en su posición con altura de salto calculada
 * - Obstáculos en la fila inferior (pueden ser múltiples '#' consecutivos)
 * 
 * Optimizado para evitar parpadeo usando dibujo directo sin borrado previo.
 */
static void draw_game_screen(void) {
    /* Calcular altura del salto del dinosaurio 
       Altura ajustada dinámicamente según la duración del salto actual
       y la velocidad de caída */
    int height = 0;
    
    /* Ajustar umbrales según si está cayendo rápido o no */
    int fall_speed = (move_interval <= 3) ? 2 : 1;
    
    /* Umbrales más conservadores para evitar parpadeos */
    if (dino_vpos >= 10) {
        height = 2;      /* muy alto */
    } else if (dino_vpos >= 5) {
        height = 1;      /* medio */
    } else {
        height = 0;      /* en el suelo */
    }
    
    int dino_bottom_row = DINO_ROW_GROUND - height;
    
    /* Dibujar fila por fila (filas 1-3, la 0 es para marcadores) */
    for (int row = 1; row <= DINO_ROW_GROUND; row++) {
        lcd_setCursor(row, 0);
        
        for (int col = 0; col < DINO_COLS; col++) {
            char ch = ' '; /* por defecto vacío */
            
            /* ¿Hay dinosaurio en esta posición? (ahora solo 1 columna) */
            if (col == dino_col) {
                /* Verificar si el dino ocupa esta fila */
                if (row >= dino_bottom_row - 1 && row <= dino_bottom_row) {
                    /* Índice de fila relativo al sprite (0=arriba, 1=abajo) */
                    int sprite_row = row - (dino_bottom_row - 1);
                    /* Solo usamos la primera columna del frame (índice 0) */
                    ch = walking_frames[current_frame][0][sprite_row];
                }
            }
            
            /* Si no hay dino, verificar obstáculo en fila inferior 
               Los obstáculos ahora pueden ser múltiples '#' consecutivos */
            if (ch == ' ' && row == DINO_ROW_GROUND) {
                /* Verificar si esta columna tiene obstáculo */
                if (obstacles[col] > 0) {
                    ch = '#';
                } else {
                    /* Verificar si es parte de un obstáculo que empezó antes */
                    for (int back = 1; back < 3; back++) {
                        int check_col = col - back;
                        if (check_col >= 0 && obstacles[check_col] > back) {
                            ch = '#';
                            break;
                        }
                    }
                }
            }
            
            lcd_writeDataByte(ch);
        }
    }
}

/**
 * @brief Comprueba si hay colisión entre el dinosaurio y un obstáculo.
 * 
 * Detecta colisión cuando:
 * 1. El dinosaurio está cerca del suelo (dino_vpos bajo)
 * 2. Hay un obstáculo en la columna del dinosaurio
 * 
 * El umbral de altura segura se ajusta dinámicamente según la velocidad
 * del juego para evitar falsas colisiones durante saltos rápidos.
 * 
 * Si detecta colisión:
 * - Setea game_over = 1
 * - Reproduce melodía de game over
 */
static void check_collision(void) {
    /* Colisión solo cuando el dinosaurio está en el suelo (no saltando)
       y hay un obstáculo en su columna (ahora solo ocupa 1 columna). */
    
    /* Umbral dinámico según velocidad del juego:
       - Velocidad lenta: necesita estar muy bajo (> 4)
       - Velocidad rápida: puede estar un poco más alto (> 2)
       Esto evita colisiones falsas cuando acelera */
    int safe_height = 2 + (move_interval / 2); /* 2 a 5 según velocidad */
    
    if (dino_vpos > safe_height) {
        return; /* saltó sobre el obstáculo */
    }
    
    /* Verificar si hay obstáculo en la columna del dinosaurio */
    if (obstacles[dino_col]) {
        game_over = 1;
        // Reproducir melodía de game over
        melodias_iniciar(melodia_game_over);
    }
}

/**
 * @brief Resetea el estado del juego a valores iniciales.
 * 
 * Limpia:
 * - Array de obstáculos
 * - Posición y velocidad del dinosaurio
 * - Flags de control del botón
 * - Puntuación y tiempo
 * - Parámetros de dificultad (velocidad y spawn rate)
 * - Estado de animación
 */
void dino_game_restart(void) {
    memset(obstacles, 0, sizeof(obstacles));
    dino_vpos = 0;
    dino_velocity = 0;
    button_held = 0;
    jump_requested = 0;
    last_button_state = 1;
    debounce_cnt = 0;
    spawn_counter = 0;
    game_over = 0;
    score = 0;
    ticks_since_start = 0;
    current_frame = 0;
    animation_counter = 0;
    move_interval = INIT_MOVE_INTERVAL;
    move_counter = 0;
    spawn_threshold = BASE_SPAWN_THRESHOLD;
    last_obstacle_pos = 0;
}

/**
 * @brief Actualiza la física del juego en cada tick.
 * 
 * Procesamiento en orden:
 * 1. Actualiza física del salto (caída gravitacional)
 * 2. Mueve obstáculos hacia la izquierda según move_interval
 * 3. Genera nuevos obstáculos aleatoriamente (tamaño 1-3 caracteres)
 * 4. Ajusta dificultad dinámicamente (velocidad y spawn rate)
 * 5. Detecta colisiones
 * 6. Actualiza puntuación al pasar obstáculos
 * 
 * La velocidad de caída se adapta a move_interval para mantener
 * jugabilidad consistente a diferentes velocidades de juego.
 */
static void game_tick_update(void) {
    if (game_over) return;

    /* Contador global de ticks (para ajustar dificultad con el tiempo) */
    ticks_since_start++;

    /* Manejo de salto con velocidad adaptativa:
       - A velocidad lenta: cae 1 tick por frame (suave)
       - A velocidad rápida: cae 2 ticks por frame (rápido)
       Esto hace que la caída sea proporcional a la velocidad del juego */
    if (dino_vpos > 0) {
        /* Calcular velocidad de caída según move_interval:
           - move_interval 6 (lento): decrement = 1
           - move_interval 4 (medio): decrement = 1
           - move_interval 2 (rápido): decrement = 2 */
        int fall_speed = (move_interval <= 3) ? 2 : 1;
        dino_vpos -= fall_speed;
        if (dino_vpos < 0) dino_vpos = 0; /* no bajar de 0 */
    }

    /* Incrementar contador de movimiento y mover obstáculos sólo cuando
       alcanzamos el intervalo. Esto permite ajustar velocidad inicial más
       lenta y aumentarla con el tiempo. */
    move_counter++;
    if (move_counter < move_interval) {
        return; /* no mover obstáculos aún */
    }
    move_counter = 0;
    /* Mover obstáculos hacia la izquierda */
    /* Guardar si había un obstáculo en la columna del dino antes del movimiento
       para poder contar cuando haya pasado con éxito. */
    uint8_t was_at_dino = obstacles[dino_col];
    for (int i = 0; i < DINO_COLS - 1; i++) {
        obstacles[i] = obstacles[i + 1];
    }
    
    /* Actualizar posición del último obstáculo (se mueve a la izquierda) */
    if (last_obstacle_pos > 0) {
        last_obstacle_pos--;
    }

    /* Generar spawn usando un umbral sobre 0..255; spawn_threshold pequeño al
       inicio provoca pocos obstáculos, y lo aumentamos con el tiempo.
       Ahora los obstáculos pueden tener tamaño 1, 2 o 3 caracteres.
       IMPORTANTE: Solo generar si hay al menos 4 espacios desde el último obstáculo */
    uint16_t r = lfsr16() & 0xFFu;
    if (r < spawn_threshold && last_obstacle_pos == 0) {
        /* Generar tamaño aleatorio del obstáculo: 1, 2 o 3 */
        uint8_t size = (lfsr16() % 3) + 1; /* 1, 2 o 3 */
        obstacles[DINO_COLS - 1] = size;
        /* Marcar que generamos obstáculo: mínimo 4 espacios de separación
           (3 vacíos + 1 del obstáculo) */
        last_obstacle_pos = 4 + size; /* separación + tamaño del obstáculo */
    } else {
        obstacles[DINO_COLS - 1] = 0;
    }

     /* Ajustar dificultad dinámicamente según puntuación (cada obstáculo pasado).
         - move_interval baja gradualmente (obstáculos se mueven más rápido).
         - spawn_threshold sube gradualmente (más probabilidad de spawn).
         Esto produce un aumento suave de la dificultad conforme avanza. */
     /* Cada 5 puntos, aumentar velocidad */
     int speed_level = score / 5;
     int new_move = (int)INIT_MOVE_INTERVAL - speed_level;
     if (new_move < MIN_MOVE_INTERVAL) new_move = MIN_MOVE_INTERVAL;
     move_interval = (uint8_t)new_move;

     /* Cada 3 puntos, aumentar probabilidad de spawn; limitar a MAX_SPAWN_THRESHOLD */
     int spawn_level = score / 3;
     int new_spawn = (int)BASE_SPAWN_THRESHOLD + (spawn_level * 2);
     if (new_spawn > MAX_SPAWN_THRESHOLD) new_spawn = MAX_SPAWN_THRESHOLD;
     spawn_threshold = (uint8_t)new_spawn;

    check_collision();

    /* Si antes había un obstáculo en la columna del dino y ahora
       no está ocupada -> +1 punto (pasó correctamente). */
    int now_at_dino = obstacles[dino_col];
    if (was_at_dino && !now_at_dino && !game_over) {
        score++;
    }
}

/**
 * @brief Inicializa TIMER2 para generar ticks de juego cada 50ms.
 * 
 * Configura TIMER2 en modo contador con prescaler de 1ms y match en 50ms.
 * Genera interrupción periódica que setea game_tick_flag para actualización
 * del juego en el main loop.
 */
static void timer2_init(void) {
    TIM_TIMERCFG_Type timer_config;
    TIM_MATCHCFG_Type match_config;

    /* Configurar prescaler para base de tiempo de 1ms */
    timer_config.prescaleOption = TIM_USVAL;
    timer_config.prescaleValue = 1000; /* 1ms base time */

    TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timer_config);

    /* Configurar match para período de tick del juego (50ms) */
    match_config.matchChannel = TIM_MATCH_CHANNEL_0;
    match_config.intOnMatch = ENABLE;        /* Generar interrupción */
    match_config.resetOnMatch = ENABLE;      /* Reset automático en match */
    match_config.stopOnMatch = DISABLE;      /* Continuar después del match */
    match_config.extMatchOutputType = TIM_NOTHING;
    match_config.matchValue = TICK_MS;       /* 50ms tick rate */

    TIM_ConfigMatch(LPC_TIM2, &match_config);

    /* Habilitar interrupción TIMER2 en NVIC */
    NVIC_EnableIRQ(TIMER2_IRQn);
    
    /* Iniciar TIMER2 */
    TIM_Cmd(LPC_TIM2, ENABLE);
}

/**
 * @brief Handler de interrupción de TIMER2.
 * 
 * Se ejecuta cada 50ms (20 Hz) cuando TIMER2 alcanza el valor de match.
 * Setea la bandera game_tick_flag para indicar al main loop que debe
 * procesar un tick de juego. Mantiene la ISR lo más breve posible.
 */
void TIMER2_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)) {
        game_tick_flag = 1;
        TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
    }
}

/**
 * @brief Inicializa el hardware necesario para el juego del dinosaurio.
 * 
 * Configura:
 * - P0.4 como GPIO input con pull-up para el botón de salto
 * - TIMER2 para generar ticks de juego cada 50ms
 * - Estado inicial del juego
 * - Pantalla inicial en LCD
 * 
 * IMPORTANTE: NO deshabilita TIMER0_IRQn ni TIMER1_IRQn porque el sistema
 * de audio (melodias_dac.c) los necesita para generar las melodías.
 * Cada subsistema tiene su timer independiente sin conflictos.
 */
void dino_game_init(void) {
    /* Configurar pin P0.4 como GPIO input (pull-up) */
    PINSEL_CFG_Type pin_cfg;
    pin_cfg.portNum = DINO_BUTTON_PORT;
    pin_cfg.pinNum = DINO_BUTTON_PIN;
    pin_cfg.funcNum = 0; /* GPIO function */
    pin_cfg.openDrain = 0;
    pin_cfg.pinMode = 0; /* pull-up resistor */
    PINSEL_ConfigPin(&pin_cfg);

    /* Configurar P0.4 como entrada */
    LPC_GPIO0->FIODIR &= ~(1u << DINO_BUTTON_PIN);

    /* Iniciar TIMER2 para ticks del juego */
    timer2_init();

    /* Resetear estado del juego */
    dino_game_restart();

    /* Dibujar pantalla inicial */
    lcd_borrarPantalla();
    lcd_setCursor(0, 0);
    lcd_escribir("DINO: boton salta");
    lcd_setCursor(1, 0);
    lcd_escribir("Presiona para jugar");

    /* NOTA: No deshabilitar TIMER0_IRQn - es necesario para el sistema de audio DAC.
       El sistema de melodías usa TIMER0 y TIMER1, el juego usa TIMER2. */
}
/**
 * @brief Debounce del botón y actualización de estado.
 * 
 * Implementa debounce simple:
 * - Detecta cambios de estado del botón
 * - Espera 2 ticks de estabilidad antes de confirmar
 * - Setea jump_requested inmediatamente al presionar (para respuesta rápida)
 * - Actualiza button_held después de confirmar estado estable
 */
static void update_button_state(void) {
    int raw = read_button();
    int pressed = (raw == 0);
    
    if (pressed != last_button_state) {
        debounce_cnt = 0;
        last_button_state = pressed;
        
        /* Si cambió a presionado, marcar solicitud de salto inmediatamente */
        if (pressed) {
            jump_requested = 1;
        }
        return;
    }
    
    if (debounce_cnt < 2) { /* Reducido de 3 a 2 para respuesta más rápida */
        debounce_cnt++;
        return;
    }
    
    /* Estado estable: actualizar button_held */
    button_held = pressed ? 1 : 0;
}

/**
 * @brief Detecta flanco de pulsación del botón (edge detection).
 * 
 * Detecta transición de no presionado → presionado (rising edge).
 * Usado para iniciar juego y reiniciar después de game over.
 * 
 * @return 1 si detectó flanco ascendente, 0 en caso contrario
 */
static int button_pressed_edge(void) {
    static uint8_t last_held = 0;
    int edge = 0;
    
    if (button_held && !last_held) {
        edge = 1; /* flanco ascendente */
    }
    
    last_held = button_held;
    return edge;
}

/* Forward declaration */
static void update_dino_animation(void);

/**
 * @brief Dibuja los marcadores en la fila superior del LCD (fila 0).
 * 
 * Muestra:
 * - Izquierda: "DINO" (identificador del juego)
 * - Centro: Tiempo transcurrido en segundos (formato XXX)
 * - Derecha: Puntuación actual (formato XXX)
 * 
 * Actualizado en cada frame del juego.
 */
static void draw_score_display(void) {
    /* Mostrar etiqueta DINO en esquina superior izquierda */
    lcd_setCursor(0, 0);
    lcd_escribir("DINO");

    /* Mostrar tiempo transcurrido (segundos) en el centro */
    int time_s = (int)(ticks_since_start / TICKS_PER_SEC);
    char time_s_buf[4];
    time_s_buf[3] = '\0';
    int ts = time_s;
    time_s_buf[2] = (ts % 10) + '0'; ts /= 10;
    time_s_buf[1] = (ts % 10) + '0'; ts /= 10;
    time_s_buf[0] = (ts % 10) + '0';
    int time_col = (DINO_COLS - 3) / 2;
    lcd_setCursor(0, time_col);
    lcd_escribir(time_s_buf);

    /* Mostrar puntuación en esquina superior derecha */
    char score_s[4];
    int sc = (int)score;
    score_s[3] = '\0';
    score_s[2] = (sc % 10) + '0'; sc /= 10;
    score_s[1] = (sc % 10) + '0'; sc /= 10;
    score_s[0] = (sc % 10) + '0';
    lcd_setCursor(0, DINO_COLS - 3);
    lcd_escribir(score_s);
}

/**
 * @brief Actualiza el juego si hay un tick pendiente del TIMER2.
 * 
 * Esta función debe ser llamada periódicamente desde el main loop.
 * Procesa el tick del juego (física, colisiones, dibujo) solo cuando
 * TIMER2 ha generado una interrupción (game_tick_flag == 1).
 * 
 * Flujo:
 * 1. Verificar si hay tick pendiente (por ISR o polling)
 * 2. Actualizar estado del botón
 * 3. Si no iniciado, esperar pulsación del usuario
 * 4. Si jugando: procesar salto, física, colisiones, animación, dibujo
 * 5. Si game over: mostrar mensaje y esperar reinicio
 */
void dino_game_run(void) {
    /* Polling adicional del flag de TIMER2 por si la ISR no se ejecuta.
       Esto previene que el juego se bloquee si NVIC está deshabilitado. */
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
        game_tick_flag = 1;
    }

    /* Salir si no hay tick pendiente */
    if (!game_tick_flag) return;
    game_tick_flag = 0;

    /* Actualizar estado del botón en cada tick */
    update_button_state();

    /* Si el juego aún no fue iniciado por el usuario, esperar pulsación para
       arrancar. Esto evita que el código borre la pantalla inesperadamente al
       arrancar y permite ver el texto inicial. */
    if (!game_started) {
        if (button_pressed_edge()) {
            game_started = 1;
            dino_game_restart();
            lcd_borrarPantalla();
            /* dibujar primer frame inmediatamente */
            draw_score_display();
            draw_game_screen();
        } else {
            return; /* esperar a que el usuario pulse */
        }
    }

    /* Leer botón: si se presiona y está en el suelo -> iniciar salto */
    if (!game_over) {
        /* Iniciar salto si hay solicitud pendiente y está en el suelo */
        if (jump_requested && dino_vpos == 0) {
            /* Duración del salto ajustada según velocidad del juego:
               - A velocidad inicial (move_interval=6): 20 ticks
               - A velocidad máxima (move_interval=2): 12 ticks
               Esto hace que el salto sea más rápido cuando el juego acelera */
            int jump_duration = 10 + (move_interval * 2); /* 12 a 22 ticks */
            dino_vpos = jump_duration;
            jump_requested = 0; /* Limpiar flag después de usar */
            
            // Reproducir efecto de sonido de salto
            melodias_iniciar(melodia_salto);
        }

        /* Actualizar física del juego (movimiento, colisiones, spawns) */
        game_tick_update();
        
        /* Actualizar animación del dinosaurio (ciclo de frames) */
        update_dino_animation();
        
        /* Actualizar sistema de melodías DAC (no bloqueante) */
        melodias_actualizar();
        
        /* Dibujar todo el frame (optimizado, sin parpadeo) */
        draw_game_screen();
        draw_score_display();
    } else {
        /* Game over: mostrar mensaje y esperar reinicio por pulsación */
        lcd_setCursor(1, 0);
        lcd_escribir("  GAME OVER   ");
        if (button_pressed_edge()) {
            /* Reiniciar al detectar pulsación */
            dino_game_restart();
            lcd_borrarPantalla();
        }
    }
}

/**
 * @brief Actualiza la animación del dinosaurio (solo cuando está en el suelo).
 * 
 * Alterna entre frame 0 y frame 1 cada 6 ticks (~300ms) para crear
 * efecto de caminata. La animación se pausa durante el salto.
 */
static void update_dino_animation(void) {
    /* Solo animar cuando está en el suelo */
    if (dino_vpos == 0) {
        animation_counter++;
        if (animation_counter >= 6) { /* Cambiar frame cada 6 ticks (~300ms) */
            current_frame = (current_frame + 1) % 2;
            animation_counter = 0;
        }
    }
}
