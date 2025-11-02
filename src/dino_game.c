/**
 * @file dino_game.c
 * @brief Implementación simple del mini-juego "Dinosaurio" usando el LCD I2C.
 *
 * Diseño:
 * - Usa Timer1 para generar ticks de juego (~20 Hz por defecto).
 * - Lee un botón conectado a P0.4 (entrada con pull-up) para saltar.
 * - Actualiza la posición del dinosaurio y obstáculos en cada tick.
 * - Dibuja en el LCD usando `lcd_setCursor` y `lcd_escribir`.
 *
 * Nota: las funciones I2C/LCD se llaman desde la contexto del bucle principal, no
 * desde la ISR. La ISR sólo marca un flag (tick) para evitar bloqueos en la ISR.
 */

#include "dino_game.h"
#include "lcd_i2c.h"
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

#include <stdint.h>
#include <string.h>

/* Configuración -----------------------------------------------------------*/
/* Pin del botón: P0.4 (GPIO input). Cambio sencillo si prefieres otro pin. */
#define DINO_BUTTON_PORT 0
#define DINO_BUTTON_PIN  4

/* Dimensiones del área de juego en caracteres */
#define DINO_COLS 20
#define DINO_ROW_GROUND 3 /* fila inferior (0..3) */
/* Sprite dimensions (characters) - ahora es 1 caracter de ancho */
#define DINO_WIDTH 1
#define DINO_HEIGHT 2

/* Juego: parámetros */
static const uint16_t TICKS_PER_SEC = 20; /* 20 Hz - ajustado para segundos reales */
static const uint16_t TICK_MS = 50; /* 50ms = 20 Hz exacto (1000ms/20 = 50ms) */

/* Estado del juego --------------------------------------------------------*/
static volatile uint8_t game_tick_flag = 0; /* set por la ISR del timer */
static uint8_t obstacles[DINO_COLS]; /* 0 = vacío, 1-3 = tamaño del obstáculo */
static uint8_t dino_col = 2; /* columna fija del dino */
static int8_t dino_vpos = 0; /* 0 = en tierra, >0 = en aire (altura actual) */
static int8_t dino_velocity = 0; /* velocidad vertical: >0 subiendo, <0 cayendo */
static uint8_t spawn_counter = 0;
static uint32_t prng = 0xACE1u; /* LFSR simple */
static uint8_t game_over = 0;
static uint8_t game_started = 0;
static uint32_t score = 0;
static int dino_row = DINO_ROW_GROUND; // Fila inicial del dinosaurio
static uint8_t button_held = 0; /* 1 = botón mantenido presionado */

/* Movement and difficulty tuning */
/* Difficulty tuning constants */
#define INIT_MOVE_INTERVAL 6 /* ticks between moves at start - velocidad inicial moderada */
#define MIN_MOVE_INTERVAL 2 /* velocidad máxima (muy rápido) */
#define MAX_SPAWN_THRESHOLD 80 /* cap for spawn threshold (0..255 scale) */
#define BASE_SPAWN_THRESHOLD 40 /* initial spawn threshold (higher => more spawns) - MÁS ALTO para spawns más frecuentes */

static uint8_t move_interval = INIT_MOVE_INTERVAL; /* number of ticks between obstacle moves */
static uint8_t move_counter = 0;
static uint8_t spawn_threshold = BASE_SPAWN_THRESHOLD; /* 0..255 threshold for RNG; small => rare spawns */
static uint32_t ticks_since_start = 0;
static uint8_t last_obstacle_pos = 0; /* Posición del último obstáculo generado */

/* Debounce */
static uint8_t last_button_state = 1; /* 1 = released (pull-up) */
static uint8_t debounce_cnt = 0;
static uint8_t jump_requested = 0; /* Flag para solicitud de salto pendiente */

/* Animaciones y estado visual del dinosaurio */
/* Define the walking animation frames - 2 caracteres verticales (D arriba + A/I abajo) */
static const char walking_frames[2][2][2] = {
    {   /* Frame 0 - D + A (caminando) */
        {'D', 'A'},  /* columna izquierda: D arriba, A abajo */
        {' ', ' '}   /* columna derecha: vacía */
    },
    {   /* Frame 1 - D + I (caminando) */
        {'D', 'I'},  /* columna izquierda: D arriba, I abajo */
        {' ', ' '}   /* columna derecha: vacía */
    }
};

static int current_frame = 0;       /* cuadro actual de caminar (0 o 1) */
static int animation_counter = 0;   /* contador para reducir velocidad de animación */


/* Helper: lectura del botón (pull-up activo, 0 = presionado) */
static int read_button(void) {
    return (LPC_GPIO0->FIOPIN & (1u << DINO_BUTTON_PIN)) ? 1 : 0;
}

/* PRNG simple (16-bit LFSR) */
static uint16_t lfsr16(void) {
    uint16_t l = (uint16_t)prng;
    uint16_t lsb = l & 1u;
    l >>= 1;
    if (lsb) l ^= 0xB400u;
    prng = l;
    return l;
}

/* Dibuja todo el juego optimizado (sin parpadeo) */
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

/* Comprueba colisión (dino en suelo y obstáculo en su columna) */
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
    }
}

/* Resetea estado del juego */
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

/* Lógica por tick: actualizar posiciones, generar obstáculos, colisiones */
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

/* Inicializa Timer1 con match para TICK_MS */
static void timer1_init(void) {
    TIM_TIMERCFG_Type timer_config;
    TIM_MATCHCFG_Type match_config;

    timer_config.prescaleOption = TIM_USVAL;
    timer_config.prescaleValue = 1000; /* 1 ms base */

    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timer_config);

    match_config.matchChannel = TIM_MATCH_0;
    match_config.intOnMatch = ENABLE;
    match_config.resetOnMatch = ENABLE;
    match_config.stopOnMatch = DISABLE;
    match_config.extMatchOutputType = TIM_NOTHING;
    match_config.matchValue = TICK_MS;

    TIM_ConfigMatch(LPC_TIM1, &match_config);

    NVIC_EnableIRQ(TIMER1_IRQn);
    TIM_Cmd(LPC_TIM1, ENABLE);
}

/* ISR: sólo marca flag para procesar fuera de la ISR */
void TIMER1_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)) {
        game_tick_flag = 1;
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
    }
}

/* Inicializa hardware usado por el juego */
void dino_game_init(void) {
    /* Configurar pin P0.4 como GPIO input (pull-up) */
    PINSEL_CFG_Type pin_cfg;
    pin_cfg.portNum = DINO_BUTTON_PORT;
    pin_cfg.pinNum = DINO_BUTTON_PIN;
    pin_cfg.funcNum = 0; /* GPIO */
    pin_cfg.openDrain = 0;
    pin_cfg.pinMode = 0; /* pull-up */
    PINSEL_ConfigPin(&pin_cfg);

    /* P0.4 como entrada */
    LPC_GPIO0->FIODIR &= ~(1u << DINO_BUTTON_PIN);

    /* Iniciar temporizador de ticks */
    timer1_init();

    dino_game_restart();

    /* Dibujar pantallas iniciales */
    lcd_borrarPantalla();
    lcd_setCursor(0, 0);
    lcd_escribir("DINO: boton salta");
    lcd_setCursor(1, 0);
    lcd_escribir("Presiona para jugar");

    /* Deshabilitar el IRQ de TIMER0 para evitar que otro ISR (main) haga
       desplazamientos en el LCD mientras juegas. */
    NVIC_DisableIRQ(TIMER0_IRQn);
}
/* Debounce simple y detección de estado del botón */
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

/* Detección de borde de pulsación (para iniciar juego y game over) */
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

/* Dibuja los marcadores (fila 0: tiempo y puntuación) */
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

/* Actualiza el juego si hay tick pendiente. Llamar desde main loop. */
void dino_game_run(void) {
    /* Si la ISR no está habilitada por alguna razón, también comprobamos el
       flag del periférico Timer1 aquí (polling) para aceptar ticks aunque la
       ISR no se ejecute. Esto evita depender estrictamente del NVIC. */
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        game_tick_flag = 1;
    }

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
        }

        /* Actualizar juego */
        game_tick_update();
        
        /* Actualizar animación del dinosaurio */
        update_dino_animation();
        
        /* Dibujar todo (optimizado, sin parpadeo) */
        draw_game_screen();
        draw_score_display();
    } else {
        /* Game over: mostrar mensaje y esperar reinicio por pulsación larga */
        lcd_setCursor(1, 0);
        lcd_escribir("  GAME OVER   ");
        if (button_pressed_edge()) {
            /* Reiniciar al detectar pulsación */
            dino_game_restart();
            lcd_borrarPantalla();
        }
    }
}

/* Actualiza la animación del dinosaurio (solo cambia frames de caminar) */
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
