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
#include "bluetooth_uart.h" // Comandos Bluetooth
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

#include <stdint.h>
#include <string.h>

/* ========================== CONFIGURACIÓN ================================= */

/* Pin del botón de salto */
#define PUERTO_BOTON_DINO 0
#define PIN_BOTON_DINO  4

/* Dimensiones del área de juego en caracteres LCD (20x4) */
#define COLUMNAS_DINO 20          // Ancho del LCD en caracteres
#define FILA_SUELO_DINO 3     // Fila inferior (0=marcadores, 1-3=juego)

/* Dimensiones del sprite del dinosaurio */
#define ANCHO_DINO 1          // Ancho en caracteres (optimizado)
#define ALTO_DINO 2         // Alto en caracteres (2 filas)

/* Configuración de timing del juego */
#define TICKS_POR_SEGUNDO 20      // Frecuencia de actualización (20 Hz)
#define TICK_MS 50            // Período de tick (50ms = 1000ms/20)

/* Configuración de dificultad del juego */
#define INTERVALO_MOVIMIENTO_INICIAL 2       // Intervalo inicial entre movimientos (ticks) - MUY rápido
#define INTERVALO_MOVIMIENTO_MINIMO 1        // Intervalo mínimo (velocidad máxima)
#define UMBRAL_SPAWN_BASE 40    // Umbral inicial de spawn de obstáculos (0-255)
#define UMBRAL_SPAWN_MAXIMO 80     // Umbral máximo de spawn

/* ========================== VARIABLES DE ESTADO ========================== */

/* Flag de tick del timer (seteado por TIMER2_IRQHandler cada 50ms) */
static volatile uint8_t bandera_tick_juego = 0;

/* Array de obstáculos (0 = vacío, 1-3 = tamaño del obstáculo) */
static uint8_t obstaculos[COLUMNAS_DINO];

/* Posición del dinosaurio */
static uint8_t columna_dino = 2;          /* Columna fija del dinosaurio */
static int8_t posicion_vertical_dino = 0;          /* Posición vertical (0=suelo, >0=aire) */
static int8_t velocidad_dino = 0;      /* Velocidad vertical (sin usar actualmente) */

/* Generación de obstáculos */
static uint8_t contador_spawn = 0;
static uint32_t semilla_rng = 0xACE1u;       /* Semilla para PRNG (LFSR de 16 bits) */
static uint8_t posicion_ultimo_obstaculo = 0; /* Posición del último obstáculo generado */

/* Estado del juego */
static uint8_t juego_terminado = 0;         /* 1 = juego terminado */
static uint8_t juego_iniciado = 0;      /* 1 = juego iniciado por el usuario */
static uint32_t puntuacion = 0;            /* Puntuación actual */
static uint32_t ticks_desde_inicio = 0; /* Ticks desde inicio del juego */

/* Control del botón */
static uint8_t boton_presionado = 0;       /* 1 = botón presionado */
static uint8_t estado_boton_anterior = 0; /* Estado anterior del botón (0=no presionado, activo BAJO) */
static uint8_t contador_debounce = 0;      /* Contador de debounce */
static uint8_t salto_solicitado = 0;    /* 1 = salto pendiente */
static uint8_t ultimo_estado_confirma = 0;         /* Último estado confirmado de boton_presionado (para edge detection) */

/* Ajuste dinámico de dificultad */
static uint8_t intervalo_movimiento = INTERVALO_MOVIMIENTO_INICIAL; /* Ticks entre movimientos */
static uint8_t contador_movimiento = 0;                    /* Contador para intervalo_movimiento */
static uint8_t umbral_spawn = UMBRAL_SPAWN_BASE; /* Umbral de spawn (0-255) */

/* Animación del dinosaurio */
static int frame_actual = 0;        /* Frame actual de animación (0 o 1) */
static int contador_animacion = 0;    /* Contador para velocidad de animación */

/* ========================== DECLARACIONES FORWARD ======================== */

static void actualizar_animacion_dino(void);
static void dibujar_marcadores(void);
static void dibujar_pantalla_juego(void);

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
 * @brief Lee el estado del botón desde el GPIO o Bluetooth.
 *
 * El botón físico está conectado a GND con pull-up interno (activo BAJO):
 * - Pin LOW (0) = presionado → retorna 1
 * - Pin HIGH (1) = no presionado → retorna 0
 *
 * También verifica si hay comando 'B' desde Bluetooth.
 *
 * @return 1 si está presionado (físico o Bluetooth), 0 si no
 */
static int leer_boton(void) {
    /* Leer botón físico P0.4 */
    int physical = (LPC_GPIO0->FIOPIN & (1u << PIN_BOTON_DINO)) ? 0 : 1;
    
    /* Leer comando Bluetooth */
    int bt_button = bt_obtener_comando_boton();
    
    /* Si cualquiera está presionado, retornar 1 */
    if (bt_button) {
        bt_limpiar_comando_boton();  // Limpiar después de leer
        return 1;
    }
    
    return physical;
}

/**
 * @brief Generador de números pseudo-aleatorios (PRNG) usando LFSR de 16 bits.
 *
 * Implementa un Linear Feedback Shift Register para generar secuencia
 * pseudo-aleatoria. Usado para spawning de obstáculos.
 *
 * @return Número pseudo-aleatorio de 16 bits
 */
static uint16_t generar_aleatorio_lfsr16(void) {
    uint16_t l = (uint16_t)semilla_rng;
    uint16_t lsb = l & 1u;
    l >>= 1;
    if (lsb) l ^= 0xB400u;
    semilla_rng = l;
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
static void dibujar_pantalla_juego(void) {
    /* Calcular altura del salto del dinosaurio
       Altura ajustada dinámicamente según la duración del salto actual
       y la velocidad de caída */
    int height = 0;

    /* Umbrales más conservadores para evitar parpadeos */
    if (posicion_vertical_dino >= 10) {
        height = 2;      /* muy alto */
    } else if (posicion_vertical_dino >= 5) {
        height = 1;      /* medio */
    } else {
        height = 0;      /* en el suelo */
    }

    int fila_inferior_dino = FILA_SUELO_DINO - height;

    /* Dibujar fila por fila (filas 1-3, la 0 es para marcadores) */
    for (int row = 1; row <= FILA_SUELO_DINO; row++) {
        lcd_establecer_cursor(row, 0);

        for (int col = 0; col < COLUMNAS_DINO; col++) {
            char ch = ' '; /* por defecto vacío */

            /* ¿Hay dinosaurio en esta posición? (ahora solo 1 columna) */
            if (col == columna_dino) {
                /* Verificar si el dino ocupa esta fila */
                if (row >= fila_inferior_dino - 1 && row <= fila_inferior_dino) {
                    /* Índice de fila relativo al sprite (0=arriba, 1=abajo) */
                    int sprite_row = row - (fila_inferior_dino - 1);
                    /* Solo usamos la primera columna del frame (índice 0) */
                    ch = walking_frames[frame_actual][0][sprite_row];
                }
            }

            /* Si no hay dino, verificar obstáculo en fila inferior
               Los obstáculos ahora pueden ser múltiples '#' consecutivos */
            if (ch == ' ' && row == FILA_SUELO_DINO) {
                /* Verificar si esta columna tiene obstáculo */
                if (obstaculos[col] > 0) {
                    ch = '#';
                } else {
                    /* Verificar si es parte de un obstáculo que empezó antes */
                    for (int back = 1; back < 3; back++) {
                        int check_col = col - back;
                        if (check_col >= 0 && obstaculos[check_col] > back) {
                            ch = '#';
                            break;
                        }
                    }
                }
            }

            lcd_escribir_byte(ch);
        }
    }
}

/**
 * @brief Comprueba si hay colisión entre el dinosaurio y un obstáculo.
 *
 * Detecta colisión cuando:
 * 1. El dinosaurio está cerca del suelo (posicion_vertical_dino bajo)
 * 2. Hay un obstáculo en la columna del dinosaurio
 *
 * El umbral de altura segura se ajusta dinámicamente según la velocidad
 * del juego para evitar falsas colisiones durante saltos rápidos.
 *
 * Si detecta colisión:
 * - Setea juego_terminado = 1
 * - Reproduce melodía de game over
 */
static void verificar_colision(void) {
    /* Colisión solo cuando el dinosaurio está en el suelo (no saltando)
       y hay un obstáculo en su columna (ahora solo ocupa 1 columna). */

    /* Umbral dinámico según velocidad del juego:
       - Velocidad lenta: necesita estar muy bajo (> 4)
       - Velocidad rápida: puede estar un poco más alto (> 2)
       Esto evita colisiones falsas cuando acelera */
    int altura_segura = 2 + (intervalo_movimiento / 2); /* 2 a 5 según velocidad */

    if (posicion_vertical_dino > altura_segura) {
        return; /* saltó sobre el obstáculo */
    }

    /* Verificar si hay obstáculo en la columna del dinosaurio */
    if (obstaculos[columna_dino]) {
        juego_terminado = 1;
        /* Detener música de fondo antes de reproducir melodía de game over */
        melodias_detener();
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
void juego_dinosaurio_reiniciar(void) {
    memset(obstaculos, 0, sizeof(obstaculos));
    posicion_vertical_dino = 0;
    velocidad_dino = 0;
    boton_presionado = 0;
    salto_solicitado = 0;
    estado_boton_anterior = 0;  // 0 = no presionado (activo BAJO)
    contador_debounce = 0;
    contador_spawn = 0;
    juego_terminado = 0;
    juego_iniciado = 0;  // Resetear para que pida "presiona para jugar" nuevamente
    puntuacion = 0;
    ticks_desde_inicio = 0;
    frame_actual = 0;
    contador_animacion = 0;
    intervalo_movimiento = INTERVALO_MOVIMIENTO_INICIAL;
    contador_movimiento = 0;
    umbral_spawn = UMBRAL_SPAWN_BASE;
    posicion_ultimo_obstaculo = 0;
    /* Asegurar que la detección de flancos reinicie correctamente */
    ultimo_estado_confirma = 0;
}

/**
 * @brief Actualiza la física del juego en cada tick.
 *
 * Procesamiento en orden:
 * 1. Actualiza física del salto (caída gravitacional)
 * 2. Mueve obstáculos hacia la izquierda según intervalo_movimiento
 * 3. Genera nuevos obstáculos aleatoriamente (tamaño 1-3 caracteres)
 * 4. Ajusta dificultad dinámicamente (velocidad y spawn rate)
 * 5. Detecta colisiones
 * 6. Actualiza puntuación al pasar obstáculos
 *
 * La velocidad de caída se adapta a intervalo_movimiento para mantener
 * jugabilidad consistente a diferentes velocidades de juego.
 */
static void actualizar_tick_juego(void) {
    if (juego_terminado) return;

    /* Contador global de ticks (para ajustar dificultad con el tiempo) */
    ticks_desde_inicio++;

    /* Manejo de salto con velocidad adaptativa:
       - A velocidad lenta: cae 1 tick por frame (suave)
       - A velocidad rápida: cae 2 ticks por frame (rápido)
       Esto hace que la caída sea proporcional a la velocidad del juego */
    if (posicion_vertical_dino > 0) {
        /* Calcular velocidad de caída según intervalo_movimiento:
           - intervalo_movimiento 6 (lento): decrement = 1
           - intervalo_movimiento 4 (medio): decrement = 1
           - intervalo_movimiento 2 (rápido): decrement = 2 */
        int velocidad_caida = (intervalo_movimiento <= 3) ? 2 : 1;
        posicion_vertical_dino -= velocidad_caida;
        if (posicion_vertical_dino < 0) posicion_vertical_dino = 0; /* no bajar de 0 */
    }

    /* Incrementar contador de movimiento y mover obstáculos sólo cuando
       alcanzamos el intervalo. Esto permite ajustar velocidad inicial más
       lenta y aumentarla con el tiempo. */
    contador_movimiento++;
    if (contador_movimiento < intervalo_movimiento) {
        return; /* no mover obstáculos aún */
    }
    contador_movimiento = 0;
    /* Mover obstáculos hacia la izquierda */
    /* Guardar si había un obstáculo en la columna del dino antes del movimiento
       para poder contar cuando haya pasado con éxito. */
    uint8_t habia_en_dino = obstaculos[columna_dino];
    for (int i = 0; i < COLUMNAS_DINO - 1; i++) {
        obstaculos[i] = obstaculos[i + 1];
    }

    /* Actualizar posición del último obstáculo (se mueve a la izquierda) */
    if (posicion_ultimo_obstaculo > 0) {
        posicion_ultimo_obstaculo--;
    }

    /* Generar spawn usando un umbral sobre 0..255; umbral_spawn pequeño al
       inicio provoca pocos obstáculos, y lo aumentamos con el tiempo.
       Ahora los obstáculos pueden tener tamaño 1, 2 o 3 caracteres.
       IMPORTANTE: Solo generar si hay al menos 4 espacios desde el último obstáculo */
    uint16_t r = generar_aleatorio_lfsr16() & 0xFFu;
    if (r < umbral_spawn && posicion_ultimo_obstaculo == 0) {
        /* Generar tamaño aleatorio del obstáculo: 1, 2 o 3 */
        uint8_t tamaño = (generar_aleatorio_lfsr16() % 3) + 1; /* 1, 2 o 3 */
        obstaculos[COLUMNAS_DINO - 1] = tamaño;
        /* Marcar que generamos obstáculo: mínimo 4 espacios de separación
           (3 vacíos + 1 del obstáculo) */
        posicion_ultimo_obstaculo = 4 + tamaño; /* separación + tamaño del obstáculo */
    } else {
        obstaculos[COLUMNAS_DINO - 1] = 0;
    }

     /* Ajustar dificultad dinámicamente según puntuación (cada obstáculo pasado).
         - intervalo_movimiento baja gradualmente (obstáculos se mueven más rápido).
         - umbral_spawn sube gradualmente (más probabilidad de spawn).
         Esto produce un aumento suave de la dificultad conforme avanza. */
     /* Cada 5 puntos, aumentar velocidad */
     int nivel_velocidad = puntuacion / 5;
     int nuevo_intervalo = (int)INTERVALO_MOVIMIENTO_INICIAL - nivel_velocidad;
     if (nuevo_intervalo < INTERVALO_MOVIMIENTO_MINIMO) nuevo_intervalo = INTERVALO_MOVIMIENTO_MINIMO;
     intervalo_movimiento = (uint8_t)nuevo_intervalo;

     /* Cada 3 puntos, aumentar probabilidad de spawn; limitar a UMBRAL_SPAWN_MAXIMO */
     int nivel_spawn = puntuacion / 3;
     int nuevo_spawn = (int)UMBRAL_SPAWN_BASE + (nivel_spawn * 2);
     if (nuevo_spawn > UMBRAL_SPAWN_MAXIMO) nuevo_spawn = UMBRAL_SPAWN_MAXIMO;
     umbral_spawn = (uint8_t)nuevo_spawn;

    verificar_colision();

    /* Si antes había un obstáculo en la columna del dino y ahora
       no está ocupada -> +1 punto (pasó correctamente). */
    int ahora_en_dino = obstaculos[columna_dino];
    if (habia_en_dino && !ahora_en_dino && !juego_terminado) {
        puntuacion++;
    }
}

/**
 * @brief Inicializa TIMER2 para generar ticks de juego cada 50ms.
 *
 * Configura TIMER2 en modo contador con prescaler de 1ms y match en 50ms.
 * Genera interrupción periódica que setea bandera_tick_juego para actualización
 * del juego en el main loop.
 */
static void inicializar_timer2(void) {
    TIM_TIMERCFG_Type configuracion_timer;
    TIM_MATCHCFG_Type configuracion_match;

    /* Configurar prescaler para base de tiempo de 1ms */
    configuracion_timer.prescaleOption = TIM_USVAL;
    configuracion_timer.prescaleValue = 1000; /* 1ms base time */

    TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &configuracion_timer);

    /* Configurar match para período de tick del juego (50ms) */
    configuracion_match.matchChannel = TIM_MATCH_CHANNEL_0;
    configuracion_match.intOnMatch = ENABLE;        /* Generar interrupción */
    configuracion_match.resetOnMatch = ENABLE;      /* Reset automático en match */
    configuracion_match.stopOnMatch = DISABLE;      /* Continuar después del match */
    configuracion_match.extMatchOutputType = TIM_NOTHING;
    configuracion_match.matchValue = TICK_MS;       /* 50ms tick rate */

    TIM_ConfigMatch(LPC_TIM2, &configuracion_match);

    /* Habilitar interrupción TIMER2 en NVIC */
    NVIC_EnableIRQ(TIMER2_IRQn);

    /* Iniciar TIMER2 */
    TIM_Cmd(LPC_TIM2, ENABLE);
}

/**
 * @brief Handler de interrupción de TIMER2.
 *
 * Se ejecuta cada 50ms (20 Hz) cuando TIMER2 alcanza el valor de match.
 * Setea la bandera bandera_tick_juego para indicar al main loop que debe
 * procesar un tick de juego. Mantiene la ISR lo más breve posible.
 */
void TIMER2_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)) {
        bandera_tick_juego = 1;
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
 * Nota: Deshabilita TIMER0_IRQn para evitar conflictos con otras ISRs
 * que puedan escribir en el LCD simultáneamente.
 */
void juego_dinosaurio_inicializar(void) {
    /* NOTA: P0.4 YA está configurado como GPIO input con PULL-UP en main.c
       No reconfigurar aquí para no sobrescribir la configuración global */
    
    /* Asegurar que P0.4 es entrada (sin tocar el pinMode) */
    LPC_GPIO0->FIODIR &= ~(1u << PIN_BOTON_DINO);

    /* Deshabilitar IRQ de TIMER2 por si estaba activo */
    NVIC_DisableIRQ(TIMER2_IRQn);
    
    /* Limpiar flag de tick pendiente */
    bandera_tick_juego = 0;
    
    /* Iniciar TIMER2 para ticks del juego (TIM_Init ya resetea el timer) */
    inicializar_timer2();

    /* Resetear estado del juego */
    juego_dinosaurio_reiniciar();

    /* Iniciar juego inmediatamente (como Snake) */
    juego_iniciado = 1;

    /* SIEMPRE dibujar pantalla inicial limpia */
    lcd_borrarPantalla();
    dibujar_marcadores();
    dibujar_pantalla_juego();

    /* NOTA: TIMER0_IRQn debe estar HABILITADO para que funcione el DAC/audio.
       El sistema de melodías no interfiere con el LCD porque usa DMA y
       no hace llamadas bloqueantes desde la ISR. */
}
/**
 * @brief Debounce del botón y actualización de estado.
 *
 * Implementa debounce simple:
 * - Detecta cambios de estado del botón
 * - Espera 2 ticks de estabilidad antes de confirmar
 * - Setea salto_solicitado inmediatamente al presionar (para respuesta rápida)
 * - Actualiza boton_presionado después de confirmar estado estable
 */
static void actualizar_estado_boton(void) {
    int raw = leer_boton();
    int presionado = (raw == 1);  // Botón activo BAJO: 1 = presionado

    if (presionado != estado_boton_anterior) {
        contador_debounce = 0;
        estado_boton_anterior = presionado;

        /* Si cambió a presionado, marcar solicitud de salto inmediatamente */
        if (presionado) {
            salto_solicitado = 1;
        }
        return;
    }

    if (contador_debounce < 3) {
        contador_debounce++;
        return;
    }

    /* Estado estable: actualizar boton_presionado */
    boton_presionado = presionado ? 1 : 0;
}

/**
 * @brief Detecta flanco de pulsación del botón (edge detection).
 *
 * Detecta transición de no presionado → presionado (rising edge).
 * Usado para iniciar juego y reiniciar después de game over.
 *
 * @return 1 si detectó flanco ascendente, 0 en caso contrario
 */
static int flanco_boton_presionado(void) {
    int flanco = 0;

    if (boton_presionado && !ultimo_estado_confirma) {
        flanco = 1; /* flanco ascendente */
    }

    ultimo_estado_confirma = boton_presionado;
    return flanco;
}

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
static void dibujar_marcadores(void) {
    /* Mostrar etiqueta DINO en esquina superior izquierda */
    lcd_establecer_cursor(0, 0);
    lcd_escribir("DINO");

    /* Mostrar tiempo transcurrido (segundos) en el centro */
    int tiempo_s = (int)(ticks_desde_inicio / TICKS_POR_SEGUNDO);
    char buffer_tiempo[4];
    buffer_tiempo[3] = '\0';
    int ts = tiempo_s;
    buffer_tiempo[2] = (ts % 10) + '0'; ts /= 10;
    buffer_tiempo[1] = (ts % 10) + '0'; ts /= 10;
    buffer_tiempo[0] = (ts % 10) + '0';
    int columna_tiempo = (COLUMNAS_DINO - 3) / 2;
    lcd_establecer_cursor(0, columna_tiempo);
    lcd_escribir(buffer_tiempo);

    /* Mostrar puntuación en esquina superior derecha */
    char texto_puntuacion[4];
    int sc = (int)puntuacion;
    texto_puntuacion[3] = '\0';
    texto_puntuacion[2] = (sc % 10) + '0'; sc /= 10;
    texto_puntuacion[1] = (sc % 10) + '0'; sc /= 10;
    texto_puntuacion[0] = (sc % 10) + '0';
    lcd_establecer_cursor(0, COLUMNAS_DINO - 3);
    lcd_escribir(texto_puntuacion);
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
void juego_dinosaurio_ejecutar(void) {
    /* Polling adicional del flag de TIMER2 por si la ISR no se ejecuta.
       Esto previene que el juego se bloquee si NVIC está deshabilitado. */
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
        bandera_tick_juego = 1;
    }

    /* Salir si no hay tick pendiente */
    if (!bandera_tick_juego) return;
    bandera_tick_juego = 0;

    /* Actualizar estado del botón en cada tick */
    actualizar_estado_boton();

    /* Si el juego aún no fue iniciado por el usuario, esperar pulsación para
       arrancar. Esto evita que el código borre la pantalla inesperadamente al
       arrancar y permite ver el texto inicial. */
    if (!juego_iniciado) {
        /* Usar salto_solicitado que se setea inmediatamente al detectar el botón */
        if (salto_solicitado) {
            juego_iniciado = 1;
            salto_solicitado = 0; /* Limpiar para evitar salto inmediato */
            juego_dinosaurio_reiniciar();

            /* Iniciar música de fondo en loop continuo */

            lcd_borrarPantalla();
            /* dibujar primer frame inmediatamente */
            dibujar_marcadores();
            dibujar_pantalla_juego();
        } else {
            return; /* esperar a que el usuario pulse */
        }
    }

    /* Leer botón: si se presiona y está en el suelo -> iniciar salto */
    if (!juego_terminado) {
        /* Iniciar salto si hay solicitud pendiente y está en el suelo */
        if (salto_solicitado && posicion_vertical_dino == 0) {
            /* Duración del salto ajustada según velocidad del juego:
               - A velocidad inicial (intervalo_movimiento=6): 20 ticks
               - A velocidad máxima (intervalo_movimiento=2): 12 ticks
               Esto hace que el salto sea más rápido cuando el juego acelera */
            int duracion_salto = 10 + (intervalo_movimiento * 2); /* 12 a 22 ticks */
            posicion_vertical_dino = duracion_salto;
            salto_solicitado = 0; /* Limpiar flag después de usar */

            // NO reproducir efecto (interrumpe música de fondo)
            // melodias_iniciar(melodia_salto);
        }

        /* Actualizar física del juego (movimiento, colisiones, spawns) */
        actualizar_tick_juego();

        /* Actualizar animación del dinosaurio (ciclo de frames) */
        actualizar_animacion_dino();

        /* Actualizar sistema de melodías DAC (no bloqueante) */
        melodias_actualizar();

        /* Dibujar todo el frame (optimizado, sin parpadeo) */
        dibujar_pantalla_juego();
        dibujar_marcadores();
    } else {
        /* Game over: mostrar mensaje y esperar botón para volver al menú */
        static uint8_t game_over_mostrado = 0;
        
        if (!game_over_mostrado) {
            lcd_establecer_cursor(1, 0);
            lcd_escribir("  GAME OVER   ");
            lcd_establecer_cursor(3, 0);
            lcd_escribir("Boton:Volver al menu");
            game_over_mostrado = 1;
        }
        
        if (flanco_boton_presionado()) {
            /* Usuario quiere volver al menú */
            juego_terminado = 2;  // Estado especial: volver al menú solicitado
            game_over_mostrado = 0;
        }
    }
}

/**
 * @brief Actualiza la animación del dinosaurio (solo cuando está en el suelo).
 *
 * Alterna entre frame 0 y frame 1 cada 6 ticks (~300ms) para crear
 * efecto de caminata. La animación se pausa durante el salto.
 */
static void actualizar_animacion_dino(void) {
    /* Solo animar cuando está en el suelo */
    if (posicion_vertical_dino == 0) {
        contador_animacion++;
        if (contador_animacion >= 6) { /* Cambiar frame cada 6 ticks (~300ms) */
            frame_actual = (frame_actual + 1) % 2;
            contador_animacion = 0;
        }
    }
}

/**
 * @brief Retorna el estado del juego
 * @return 0 = jugando, 1 = game over (pantalla mostrada), 2 = volver al menú
 */
uint8_t juego_dinosaurio_ha_terminado(void) {
    return juego_terminado;
}
