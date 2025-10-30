/**
 * @file    DacMelodias.c
 * @brief   Generador de melodías mediante DAC para buzzer
 * @details Este archivo contiene funciones para generar diferentes melodías
 *          usando el DAC del LPC1769 conectado a un buzzer
 * @date    Octubre 2025
 */

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_systick.h"
#include "lpc17xx_timer.h"

/* ========================== DEFINICIONES DE NOTAS ========================= */

// Frecuencias de notas musicales (en Hz) - Escala temperada
#define DO_3    131
#define DO_S3   139
#define RE_3    147
#define RE_S3   156
#define MI_3    165
#define FA_3    175
#define FA_S3   185
#define SOL_3   196
#define SOL_S3  208
#define LA_3    220
#define LA_S3   233
#define SI_3    247

#define DO_4    262
#define DO_S4   277
#define RE_4    294
#define RE_S4   311
#define MI_4    330
#define FA_4    349
#define FA_S4   370
#define SOL_4   392
#define SOL_S4  415
#define LA_4    440
#define LA_S4   466
#define SI_4    494

#define DO_5    523
#define DO_S5   554
#define RE_5    587
#define RE_S5   622
#define MI_5    659
#define FA_5    698
#define FA_S5   740
#define SOL_5   784
#define SOL_S5  831
#define LA_5    880
#define LA_S5   932
#define SI_5    988

#define SILENCIO 0

// Duraciones de notas (en milisegundos)
#define REDONDA      2000
#define BLANCA       1000
#define NEGRA        500
#define CORCHEA      250
#define SEMICORCHEA  125

/* ========================== VARIABLES GLOBALES ============================ */

static volatile uint32_t dac_toggle_state = 0;  // Estado de la onda (0 o 1)
static volatile uint32_t nota_duracion_ms = 0;   // Duración restante de la nota
static volatile uint8_t reproduciendo = 0;        // Flag de reproducción activa

/* ========================== ESTRUCTURA DE NOTA ============================ */

typedef struct {
    uint16_t frecuencia;
    uint16_t duracion;
} Nota;

/* ============================= MELODÍAS ===================================== */

// Melodía 1: "Happy Birthday"
Nota melodia_happy_birthday[] = {
    {DO_4, NEGRA}, {DO_4, CORCHEA}, {RE_4, BLANCA},
    {DO_4, BLANCA}, {FA_4, BLANCA}, {MI_4, REDONDA},
    {DO_4, NEGRA}, {DO_4, CORCHEA}, {RE_4, BLANCA},
    {DO_4, BLANCA}, {SOL_4, BLANCA}, {FA_4, REDONDA},
    {DO_4, NEGRA}, {DO_4, CORCHEA}, {DO_5, BLANCA},
    {LA_4, BLANCA}, {FA_4, BLANCA}, {MI_4, BLANCA}, {RE_4, BLANCA},
    {SI_4, NEGRA}, {SI_4, CORCHEA}, {LA_4, BLANCA},
    {FA_4, BLANCA}, {SOL_4, BLANCA}, {FA_4, REDONDA},
    {SILENCIO, 0}
};

// Melodía 2: "Super Mario Bros Theme"
Nota melodia_mario[] = {
    {MI_5, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA}, {MI_5, CORCHEA},
    {SILENCIO, CORCHEA}, {DO_5, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA},
    {SOL_5, NEGRA}, {SILENCIO, NEGRA}, {SOL_4, NEGRA}, {SILENCIO, NEGRA},
    
    {DO_5, CORCHEA+SEMICORCHEA}, {SILENCIO, SEMICORCHEA}, {SOL_4, CORCHEA+SEMICORCHEA},
    {SILENCIO, SEMICORCHEA}, {MI_4, CORCHEA+SEMICORCHEA}, {SILENCIO, SEMICORCHEA},
    {LA_4, CORCHEA}, {SILENCIO, CORCHEA}, {SI_4, CORCHEA}, {SILENCIO, CORCHEA},
    {LA_S4, CORCHEA}, {LA_4, CORCHEA},
    
    {SILENCIO, 0}
};

// Melodía 3: "Tetris Theme"
Nota melodia_tetris[] = {
    {MI_4, NEGRA}, {SI_3, CORCHEA}, {DO_4, CORCHEA}, {RE_4, NEGRA},
    {DO_4, CORCHEA}, {SI_3, CORCHEA}, {LA_3, NEGRA}, {LA_3, CORCHEA},
    {DO_4, CORCHEA}, {MI_4, NEGRA}, {RE_4, CORCHEA}, {DO_4, CORCHEA},
    {SI_3, NEGRA+CORCHEA}, {DO_4, CORCHEA}, {RE_4, NEGRA}, {MI_4, NEGRA},
    {DO_4, NEGRA}, {LA_3, NEGRA}, {LA_3, NEGRA}, {SILENCIO, CORCHEA},
    {SILENCIO, 0}
};

// Melodía 4: "Star Wars Theme"
Nota melodia_starwars[] = {
    {LA_3, CORCHEA}, {LA_3, CORCHEA}, {LA_3, CORCHEA},
    {FA_3, CORCHEA+SEMICORCHEA}, {DO_4, SEMICORCHEA},
    {LA_3, CORCHEA}, {FA_3, CORCHEA+SEMICORCHEA}, {DO_4, SEMICORCHEA},
    {LA_3, NEGRA},
    
    {MI_4, CORCHEA}, {MI_4, CORCHEA}, {MI_4, CORCHEA},
    {FA_4, CORCHEA+SEMICORCHEA}, {DO_4, SEMICORCHEA},
    {SOL_S3, CORCHEA}, {FA_3, CORCHEA+SEMICORCHEA}, {DO_4, SEMICORCHEA},
    {LA_3, NEGRA},
    {SILENCIO, 0}
};

// Melodía 5: "Nokia Tune"
Nota melodia_nokia[] = {
    {MI_5, CORCHEA}, {RE_5, CORCHEA}, {FA_S4, NEGRA}, {SOL_S4, NEGRA},
    {DO_S5, CORCHEA}, {SI_4, CORCHEA}, {RE_4, NEGRA}, {MI_4, NEGRA},
    {SI_4, CORCHEA}, {LA_4, CORCHEA}, {DO_S4, NEGRA}, {MI_4, NEGRA},
    {LA_4, BLANCA},
    {SILENCIO, 0}
};

// Melodía 6: "Jingle Bells"
Nota melodia_jingle[] = {
    {MI_4, NEGRA}, {MI_4, NEGRA}, {MI_4, BLANCA},
    {MI_4, NEGRA}, {MI_4, NEGRA}, {MI_4, BLANCA},
    {MI_4, NEGRA}, {SOL_4, NEGRA}, {DO_4, NEGRA+CORCHEA}, {RE_4, CORCHEA},
    {MI_4, REDONDA},
    {FA_4, NEGRA}, {FA_4, NEGRA}, {FA_4, NEGRA+CORCHEA}, {FA_4, CORCHEA},
    {FA_4, NEGRA}, {MI_4, NEGRA}, {MI_4, NEGRA}, {MI_4, CORCHEA}, {MI_4, CORCHEA},
    {MI_4, NEGRA}, {RE_4, NEGRA}, {RE_4, NEGRA}, {MI_4, NEGRA},
    {RE_4, BLANCA}, {SOL_4, BLANCA},
    {SILENCIO, 0}
};

/* ===================== MANEJADOR DE INTERRUPCIONES ======================= */

/**
 * @brief Manejador de interrupción del Timer1 - Match 0 y Match 1
 * @note Esta función se ejecuta en cada match del timer
 */
void TIMER1_IRQHandler(void) {
    // Verificar si la interrupción fue por Match 0 (generación de onda)
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)) {
        // Limpiar flag de interrupción
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        
        if (reproduciendo) {
            if (modo_seno) {
                // Modo senoidal: usar tabla de valores
                Timer1_SenoHandler();
            } else {
                // Modo onda cuadrada: alternar entre alto y bajo
                if (dac_toggle_state == 0) {
                    DAC_UpdateValue(800);  // Valor alto (ajustable)
                    dac_toggle_state = 1;
                } else {
                    DAC_UpdateValue(0);     // Valor bajo
                    dac_toggle_state = 0;
                }
            }
        }
    }
    
    // Verificar si la interrupción fue por Match 1 (contador de milisegundos)
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR1_INT)) {
        // Limpiar flag de interrupción
        TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
        
        // Actualizar Match 1 para el próximo milisegundo
        // Match 1 se incrementa en 1000us cada vez
        uint32_t current_match = TIM_GetCaptureValue(LPC_TIM1, TIM_MATCH_1);
        TIM_UpdateMatchValue(LPC_TIM1, TIM_MATCH_1, current_match + 1000);
        
        // Decrementar duración de nota
        if (nota_duracion_ms > 0) {
            nota_duracion_ms--;
        }
    }
}

/* ======================= FUNCIONES DE RETARDO ============================ */

/**
 * @brief Retardo en milisegundos usando variable de duración del timer
 * @param ms: cantidad de milisegundos a esperar
 */
void delay_ms(uint32_t ms) {
    nota_duracion_ms = ms;
    while (nota_duracion_ms > 0) {
        __NOP();
    }
}

/* ===================== FUNCIONES DE CONFIGURACIÓN ======================== */

/**
 * @brief Inicializa el Timer1 para generación de frecuencias
 * @note Configura Match 0 para frecuencia y Match 1 para contador de 1ms
 */
void Timer1_Init_Audio(void) {
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct;
    
    // Configuración del Timer1: modo timer, prescaler = 1us
    TIM_ConfigStruct.prescaleOption = TIM_USVAL;
    TIM_ConfigStruct.prescaleValue = 1;  // 1 microsegundo por tick
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &TIM_ConfigStruct);
    
    // Configurar Match 0 para generar frecuencia de la nota (se actualiza dinámicamente)
    TIM_MatchConfigStruct.matchChannel = TIM_MATCH_0;
    TIM_MatchConfigStruct.intOnMatch = ENABLE;
    TIM_MatchConfigStruct.resetOnMatch = ENABLE;  // Reset contador en match
    TIM_MatchConfigStruct.stopOnMatch = DISABLE;
    TIM_MatchConfigStruct.extMatchOutputType = TIM_NOTHING;
    TIM_MatchConfigStruct.matchValue = 1000;  // Valor inicial (se actualiza después)
    TIM_ConfigMatch(LPC_TIM1, &TIM_MatchConfigStruct);
    
    // Configurar Match 1 para contador de milisegundos (1ms = 1000us)
    TIM_MatchConfigStruct.matchChannel = TIM_MATCH_1;
    TIM_MatchConfigStruct.intOnMatch = ENABLE;
    TIM_MatchConfigStruct.resetOnMatch = DISABLE;  // NO reset contador
    TIM_MatchConfigStruct.stopOnMatch = DISABLE;
    TIM_MatchConfigStruct.extMatchOutputType = TIM_NOTHING;
    TIM_MatchConfigStruct.matchValue = 1000;  // 1000us = 1ms
    TIM_ConfigMatch(LPC_TIM1, &TIM_MatchConfigStruct);
    
    // Habilitar interrupción del Timer1
    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_SetPriority(TIMER1_IRQn, 1);
    
    // NO iniciar el timer aún (se inicia al reproducir nota)
}

/**
 * @brief Inicializa el DAC en el pin P0.26
 */
void DAC_Init_Melodia(void) {
    PINSEL_CFG_Type PinCfg;
    
    // Configurar P0.26 como salida del DAC (AOUT)
    PinCfg.portNum = PINSEL_PORT_0;
    PinCfg.pinNum = PINSEL_PIN_26;
    PinCfg.funcNum = PINSEL_FUNC_2;
    PinCfg.pinMode = PINSEL_TRISTATE;
    PinCfg.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&PinCfg);
    
    // Inicializar DAC
    DAC_Init();
    
    // Configurar para máxima velocidad (700uA)
    DAC_SetBias(DAC_700uA);
    
    // Inicializar Timer1 para generación de audio
    Timer1_Init_Audio();
}

/**
 * @brief Configura la frecuencia del Timer1 para una nota específica
 * @param frecuencia: frecuencia de la nota en Hz
 */
void Timer1_SetFrequency(uint16_t frecuencia) {
    uint32_t match_value;
    
    if (frecuencia == 0) {
        // Detener timer y silencio
        TIM_Cmd(LPC_TIM1, DISABLE);
        DAC_UpdateValue(0);
        reproduciendo = 0;
        return;
    }
    
    // Calcular valor de match para medio período (onda cuadrada)
    // match_value = (1000000 / frecuencia) / 2
    // Esto da el tiempo en microsegundos para medio ciclo
    match_value = (500000 / frecuencia);  // 500000 = 1000000/2
    
    // Actualizar Match 0 con el nuevo valor
    TIM_UpdateMatchValue(LPC_TIM1, TIM_MATCH_0, match_value);
    
    // Reiniciar contador y estado
    TIM_ResetCounter(LPC_TIM1);
    dac_toggle_state = 0;
    reproduciendo = 1;
    
    // Iniciar timer
    TIM_Cmd(LPC_TIM1, ENABLE);
}

/**
 * @brief Genera una onda cuadrada con el DAC usando Timer1
 * @param frecuencia: frecuencia de la nota en Hz
 * @param duracion: duración de la nota en milisegundos
 */
void DAC_GenerarNota(uint16_t frecuencia, uint16_t duracion) {
    if (frecuencia == SILENCIO || frecuencia == 0) {
        // Silencio - detener timer y poner DAC en 0
        Timer1_SetFrequency(0);
        delay_ms(duracion);
        return;
    }
    
    // Configurar frecuencia en el timer
    Timer1_SetFrequency(frecuencia);
    
    // Esperar la duración de la nota
    delay_ms(duracion);
    
    // Detener sonido al finalizar la nota
    Timer1_SetFrequency(0);
}

/* =================== VARIABLES PARA ONDA SENOIDAL ======================== */

// Tabla de valores para onda senoidal (32 muestras)
static const uint16_t tabla_seno[32] = {
    512, 612, 707, 792, 863, 917, 953, 970,
    970, 953, 917, 863, 792, 707, 612, 512,
    412, 317, 232, 161, 107,  71,  54,  37,
     37,  54,  71, 107, 161, 232, 317, 412
};

static volatile uint8_t seno_index = 0;      // Índice actual en tabla seno
static volatile uint8_t modo_seno = 0;        // 0 = cuadrada, 1 = senoidal

/**
 * @brief Manejador alternativo para onda senoidal (llamado desde IRQHandler)
 */
void Timer1_SenoHandler(void) {
    // Actualizar DAC con valor de tabla senoidal
    DAC_UpdateValue(tabla_seno[seno_index]);
    
    // Avanzar índice
    seno_index++;
    if (seno_index >= 32) {
        seno_index = 0;
    }
}

/**
 * @brief Configura Timer1 para generar onda senoidal
 * @param frecuencia: frecuencia de la nota en Hz
 */
void Timer1_SetFrequency_Seno(uint16_t frecuencia) {
    uint32_t match_value;
    
    if (frecuencia == 0) {
        TIM_Cmd(LPC_TIM1, DISABLE);
        DAC_UpdateValue(512);  // Valor medio
        reproduciendo = 0;
        modo_seno = 0;
        return;
    }
    
    // Calcular valor de match para cada muestra (32 muestras por período)
    match_value = (1000000 / frecuencia) / 32;
    
    // Actualizar Match 0
    TIM_UpdateMatchValue(LPC_TIM1, TIM_MATCH_0, match_value);
    
    // Reiniciar
    TIM_ResetCounter(LPC_TIM1);
    seno_index = 0;
    reproduciendo = 1;
    modo_seno = 1;
    
    // Iniciar timer
    TIM_Cmd(LPC_TIM1, ENABLE);
}

/**
 * @brief Genera una onda sinusoidal aproximada usando tabla de valores
 * @param frecuencia: frecuencia de la nota en Hz
 * @param duracion: duración de la nota en milisegundos
 * @note Esta función genera un sonido más suave que la onda cuadrada
 */
void DAC_GenerarNota_Seno(uint16_t frecuencia, uint16_t duracion) {
    if (frecuencia == SILENCIO || frecuencia == 0) {
        Timer1_SetFrequency_Seno(0);
        delay_ms(duracion);
        return;
    }
    
    // Configurar frecuencia en modo senoidal
    Timer1_SetFrequency_Seno(frecuencia);
    
    // Esperar la duración de la nota
    delay_ms(duracion);
    
    // Detener sonido
    Timer1_SetFrequency_Seno(0);
}

/* ====================== FUNCIONES DE REPRODUCCIÓN ======================== */

/**
 * @brief Reproduce una melodía completa
 * @param melodia: puntero al arreglo de notas
 * @param usar_seno: 1 para onda senoidal, 0 para onda cuadrada
 */
void DAC_ReproducirMelodia(Nota *melodia, uint8_t usar_seno) {
    uint16_t i = 0;
    
    while (melodia[i].frecuencia != 0 || melodia[i].duracion != 0) {
        if (usar_seno) {
            DAC_GenerarNota_Seno(melodia[i].frecuencia, melodia[i].duracion);
        } else {
            DAC_GenerarNota(melodia[i].frecuencia, melodia[i].duracion);
        }
        
        // Pequeña pausa entre notas
        DAC_UpdateValue(0);
        delay_ms(30);
        
        i++;
    }
    
    // Silencio al final
    DAC_UpdateValue(0);
}

/**
 * @brief Reproduce Happy Birthday
 */
void DAC_ReproducirHappyBirthday(void) {
    DAC_ReproducirMelodia(melodia_happy_birthday, 0);
}

/**
 * @brief Reproduce Super Mario Bros Theme
 */
void DAC_ReproducirMario(void) {
    DAC_ReproducirMelodia(melodia_mario, 0);
}

/**
 * @brief Reproduce Tetris Theme
 */
void DAC_ReproducirTetris(void) {
    DAC_ReproducirMelodia(melodia_tetris, 0);
}

/**
 * @brief Reproduce Star Wars Theme
 */
void DAC_ReproducirStarWars(void) {
    DAC_ReproducirMelodia(melodia_starwars, 0);
}

/**
 * @brief Reproduce Nokia Tune
 */
void DAC_ReproducirNokia(void) {
    DAC_ReproducirMelodia(melodia_nokia, 0);
}

/**
 * @brief Reproduce Jingle Bells
 */
void DAC_ReproducirJingleBells(void) {
    DAC_ReproducirMelodia(melodia_jingle, 0);
}

/**
 * @brief Genera un tono de prueba simple
 * @param frecuencia: frecuencia en Hz
 * @param duracion_ms: duración en milisegundos
 */
void DAC_TonoSimple(uint16_t frecuencia, uint16_t duracion_ms) {
    DAC_GenerarNota(frecuencia, duracion_ms);
}

/**
 * @brief Genera una escala musical ascendente
 */
void DAC_EscalaMusical(void) {
    uint16_t escala[] = {DO_4, RE_4, MI_4, FA_4, SOL_4, LA_4, SI_4, DO_5};
    uint8_t i;
    
    for (i = 0; i < 8; i++) {
        DAC_GenerarNota(escala[i], NEGRA);
        delay_ms(50);
    }
}

/**
 * @brief Genera un efecto de sirena
 * @param duracion_total_ms: duración total del efecto en ms
 */
void DAC_EfectoSirena(uint16_t duracion_total_ms) {
    uint16_t freq;
    uint16_t tiempo = 0;
    
    while (tiempo < duracion_total_ms) {
        // Subir frecuencia
        for (freq = 400; freq < 800; freq += 20) {
            DAC_GenerarNota(freq, 50);
            tiempo += 50;
            if (tiempo >= duracion_total_ms) return;
        }
        
        // Bajar frecuencia
        for (freq = 800; freq > 400; freq -= 20) {
            DAC_GenerarNota(freq, 50);
            tiempo += 50;
            if (tiempo >= duracion_total_ms) return;
        }
    }
}

/**
 * @brief Genera un efecto de láser/disparo
 */
void DAC_EfectoLaser(void) {
    uint16_t freq;
    
    for (freq = 1200; freq > 100; freq -= 50) {
        DAC_GenerarNota(freq, 20);
    }
    
    DAC_UpdateValue(0);
}

/**
 * @brief Genera efecto de "Game Over"
 */
void DAC_EfectoGameOver(void) {
    DAC_GenerarNota(DO_4, NEGRA);
    DAC_GenerarNota(SI_3, NEGRA);
    DAC_GenerarNota(LA_S3, NEGRA);
    DAC_GenerarNota(LA_3, BLANCA);
    delay_ms(100);
    DAC_GenerarNota(MI_3, NEGRA);
    DAC_GenerarNota(FA_3, BLANCA);
}

/**
 * @brief Genera efecto de "Victoria"
 */
void DAC_EfectoVictoria(void) {
    DAC_GenerarNota(SOL_4, CORCHEA);
    DAC_GenerarNota(DO_5, CORCHEA);
    DAC_GenerarNota(MI_5, CORCHEA);
    DAC_GenerarNota(SOL_5, NEGRA);
    DAC_GenerarNota(MI_5, CORCHEA);
    DAC_GenerarNota(SOL_5, BLANCA);
}

/* ========================== FUNCIÓN DE EJEMPLO ============================ */

/**
 * @brief Función de ejemplo que reproduce todas las melodías
 * @note Llamar esta función desde main() para probar
 */
void DAC_DemoMelodias(void) {
    // Inicializar DAC
    DAC_Init_Melodia();
    
    delay_ms(1000);
    
    // Reproducir todas las melodías
    DAC_ReproducirMario();
    delay_ms(1000);
    
    DAC_ReproducirTetris();
    delay_ms(1000);
    
    DAC_ReproducirNokia();
    delay_ms(1000);
    
    DAC_ReproducirHappyBirthday();
    delay_ms(1000);
    
    DAC_ReproducirStarWars();
    delay_ms(1000);
    
    DAC_ReproducirJingleBells();
    delay_ms(1000);
    
    // Efectos de sonido
    DAC_EfectoSirena(3000);
    delay_ms(1000);
    
    DAC_EfectoLaser();
    delay_ms(500);
    
    DAC_EfectoGameOver();
    delay_ms(1000);
    
    DAC_EfectoVictoria();
}
