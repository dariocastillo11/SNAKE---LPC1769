/**
 * @file melodias_dac.c
 * @brief Implementación del sistema de reproducción de melodías con DAC + DMA
 * @details Genera señales triangulares usando el DAC con DMA y Timer0 para reproducir
 *          melodías musicales en segundo plano sin bloquear el programa.
 *
 * @date Noviembre 2025
 */

#include "melodias_dac.h"
#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "dino_game.h"

/* ==================== CONFIGURACIÓN INTERNA =============================== */

#define NUMERO_MUESTRAS            16    // Doble de rápido (16 muestras)
#define MAXIMO_VALOR_DAC           1023
#define MICROSEGUNDOS_POR_SEGUNDO  1000000
#define PAUSA_ARTICULACION_MS      30
#define LED_ALTERNAR_INTERRUPCIONES  500

#define PORT_CERO                  0
#define PIN_22                     ((uint32_t)(1<<22))

/* === CONFIGURACIÓN DMA === */
#define MELODIAS_DMA_CH       1  // Canal DMA 1
#define MELODIAS_CONEXION_DMA    GPDMA_DAC

/* ========================== TABLA DE ONDA ================================= */

const uint16_t TABLA_TRIANGULAR[NUMERO_MUESTRAS] = {
    0,    128,  256,  384,  512,  640,  768,  896,
    1023, 896,  768,  640,  512,  384,  256,  128
};

/* === VARIABLES DMA === */
static volatile uint8_t dma_melodias_enabled = 0;  // Flag de DMA activo
static volatile uint16_t dma_melodias_index = 0;   // Índice para LLI

/* ============================= MELODÍAS =================================== */

const Nota melodia_happy_birthday[] = {
    {DO_4, NEGRA}, {DO_4, CORCHEA}, {RE_4, BLANCA},
    {DO_4, BLANCA}, {FA_4, BLANCA}, {MI_4, REDONDA},
    {DO_4, NEGRA}, {DO_4, CORCHEA}, {RE_4, BLANCA},
    {DO_4, BLANCA}, {SOL_4, BLANCA}, {FA_4, REDONDA},
    {SILENCIO, 0}
};

const Nota melodia_mario[] = {
    {MI_5, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA}, {MI_5, CORCHEA},
    {SILENCIO, CORCHEA}, {DO_5, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA},
    {SOL_5, NEGRA}, {SILENCIO, NEGRA}, {SOL_4, NEGRA}, {SILENCIO, NEGRA},
    {SILENCIO, 0}
};

const Nota melodia_tetris[] = {
    {MI_4, NEGRA}, {SI_3, CORCHEA}, {DO_4, CORCHEA}, {RE_4, NEGRA},
    {DO_4, CORCHEA}, {SI_3, CORCHEA}, {LA_3, NEGRA}, {LA_3, CORCHEA},
    {DO_4, CORCHEA}, {MI_4, NEGRA}, {RE_4, CORCHEA}, {DO_4, CORCHEA},
    {SI_3, NEGRA+CORCHEA}, {DO_4, CORCHEA}, {RE_4, NEGRA}, {MI_4, NEGRA},
    {SILENCIO, 0}
};

const Nota melodia_nokia[] = {
    {MI_5, CORCHEA}, {RE_5, CORCHEA}, {FA_S4, NEGRA}, {SOL_S4, NEGRA},
    {DO_S5, CORCHEA}, {SI_4, CORCHEA}, {RE_4, NEGRA}, {MI_4, NEGRA},
    {SI_4, CORCHEA}, {LA_4, CORCHEA}, {DO_S4, NEGRA}, {MI_4, NEGRA},
    {LA_4, BLANCA},
    {SILENCIO, 0}
};

// Melodía corta para game over
const Nota melodia_game_over[] = {
    {DO_4, CORCHEA}, {SOL_3, CORCHEA}, {MI_3, NEGRA},
    {LA_3, CORCHEA}, {SI_3, CORCHEA}, {LA_3, CORCHEA}, {SOL_S3, CORCHEA},
    {LA_S3, BLANCA}, {SOL_S3, BLANCA},
    {SILENCIO, 0}
};

// Efecto de sonido corto para salto
const Nota melodia_salto[] = {
    {DO_5, SEMICORCHEA}, {MI_5, SEMICORCHEA}, {SOL_5, SEMICORCHEA},
    {SILENCIO, 0}
};

// Melodía de fondo tipo Mario Bros (música ambiente del juego)
const Nota melodia_fondo[] = {
    // Intro
    {MI_5, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA}, {MI_5, CORCHEA},
    {SILENCIO, CORCHEA}, {DO_5, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA},
    {SOL_5, NEGRA}, {SILENCIO, NEGRA},
    {SOL_4, NEGRA}, {SILENCIO, NEGRA},

    // Sección principal
    {DO_5, NEGRA}, {SILENCIO, CORCHEA}, {SOL_4, NEGRA}, {SILENCIO, CORCHEA},
    {MI_4, NEGRA}, {SILENCIO, CORCHEA}, {LA_4, CORCHEA}, {SILENCIO, CORCHEA},
    {SI_4, CORCHEA}, {SILENCIO, CORCHEA}, {LA_S4, CORCHEA}, {LA_4, CORCHEA},

    {SOL_4, NEGRA}, {MI_5, NEGRA}, {SOL_5, NEGRA},
    {LA_5, CORCHEA}, {SILENCIO, CORCHEA}, {FA_5, CORCHEA}, {SOL_5, CORCHEA},
    {SILENCIO, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA}, {DO_5, CORCHEA},
    {RE_5, CORCHEA}, {SI_4, CORCHEA}, {SILENCIO, NEGRA},

    // Repetir variación
    {DO_5, NEGRA}, {SILENCIO, CORCHEA}, {SOL_4, NEGRA}, {SILENCIO, CORCHEA},
    {MI_4, NEGRA}, {SILENCIO, CORCHEA}, {LA_4, CORCHEA}, {SILENCIO, CORCHEA},
    {SI_4, CORCHEA}, {SILENCIO, CORCHEA}, {LA_S4, CORCHEA}, {LA_4, CORCHEA},

    {SOL_4, NEGRA}, {MI_5, NEGRA}, {SOL_5, NEGRA},
    {LA_5, CORCHEA}, {SILENCIO, CORCHEA}, {FA_5, CORCHEA}, {SOL_5, CORCHEA},
    {SILENCIO, CORCHEA}, {MI_5, CORCHEA}, {SILENCIO, CORCHEA}, {DO_5, CORCHEA},
    {RE_5, CORCHEA}, {SI_4, CORCHEA}, {SILENCIO, NEGRA},

    {SILENCIO, 0}
};

/* ========================== VARIABLES INTERNAS ============================ */

static volatile uint8_t indice_tabla_onda = 0;
static volatile uint16_t frecuencia_actual = 0;
static volatile uint8_t reproduciendo = 0;
static volatile uint32_t tiempo_transcurrido_ms = 0;
static volatile uint8_t volumen_porcentaje = 100;

static const Nota *melodia_actual = NULL;
static volatile uint16_t indice_nota_actual = 0;
static volatile uint32_t tiempo_inicio_nota = 0;
static volatile uint8_t modo_loop = 0;  // 1 = loop continuo, 0 = una sola vez

// Sistema de prioridades para melodías
static const Nota *melodia_fondo_guardada = NULL;  // Melodía de fondo pausada
static volatile uint16_t indice_fondo_guardado = 0;
static volatile uint32_t tiempo_fondo_guardado = 0;

/* Forward declaration for static function */
static void melodias_dma_restart_transfer(void);

/**
 * @brief Callback de DMA para Melodías - llamado desde dma_handlers.c
 * 
 * Se dispara cuando una transferencia DMA completa (terminal count).
 * Reinicia la transferencia para la siguiente muestra.
 */
void melodias_dma_on_transfer_complete(void) {
    if (reproduciendo && frecuencia_actual > 0) {
        dma_melodias_index = (dma_melodias_index + 1) % NUMERO_MUESTRAS;
        melodias_dma_restart_transfer();
    }
}

/**
 * @brief Inicializa el controlador GPDMA para melodías
 */
static void melodias_dma_init(void) {
    GPDMA_Init();
    NVIC_EnableIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, 1);
}

/**
 * @brief Configura y lanza la primera transferencia DMA para DAC
 */
static void melodias_dma_start_transfer(void) {
    GPDMA_Channel_CFG_Type dma_cfg;
    
    dma_cfg.channelNum = MELODIAS_DMA_CH;
    dma_cfg.transferSize = 1;                              // 1 muestra por transferencia
    dma_cfg.transferWidth = GPDMA_HALFWORD;                // 16 bits (uint16_t)
    dma_cfg.srcMemAddr = (uint32_t)&TABLA_TRIANGULAR[0];  // Fuente: tabla en RAM
    dma_cfg.dstMemAddr = 0;                                // No aplica (destino es DAC)
    dma_cfg.transferType = GPDMA_M2P;                      // Memoria a Periférico
    dma_cfg.srcConn = 0;                                   // No aplica
    dma_cfg.dstConn = MELODIAS_CONEXION_DMA;             // GPDMA_DAC
    dma_cfg.linkedList = 0;                                // Sin linked list
    
    GPDMA_Setup(&dma_cfg);
    GPDMA_ChannelCmd(MELODIAS_DMA_CH, ENABLE);
    
    dma_melodias_enabled = 1;
}

/**
 * @brief Reinicia la transferencia DMA (llamada desde ISR DMA)
 */
static void melodias_dma_restart_transfer(void) {
    GPDMA_ChannelCmd(MELODIAS_DMA_CH, DISABLE);
    
    GPDMA_Channel_CFG_Type dma_cfg;
    
    dma_cfg.channelNum = MELODIAS_DMA_CH;
    dma_cfg.transferSize = 1;
    dma_cfg.transferWidth = GPDMA_HALFWORD;
    dma_cfg.srcMemAddr = (uint32_t)&TABLA_TRIANGULAR[dma_melodias_index];
    dma_cfg.dstMemAddr = 0;
    dma_cfg.transferType = GPDMA_M2P;
    dma_cfg.srcConn = 0;
    dma_cfg.dstConn = MELODIAS_CONEXION_DMA;
    dma_cfg.linkedList = 0;
    
    GPDMA_Setup(&dma_cfg);
    GPDMA_ChannelCmd(MELODIAS_DMA_CH, ENABLE);
}

/**
 * @brief ISR del Timer0 - Generación de audio con DMA
 * Match 0: Genera la forma de onda triangular punto por punto
 */
void TIMER0_IRQHandler(void) {
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

        if (reproduciendo && frecuencia_actual > 0) {
            uint16_t valor_dac = TABLA_TRIANGULAR[indice_tabla_onda];

            // Aplicar volumen
            if (volumen_porcentaje < 100) {
                valor_dac = (valor_dac * volumen_porcentaje) / 100;
            }

            DAC_UpdateValue(valor_dac);

            indice_tabla_onda++;
            if(indice_tabla_onda >= NUMERO_MUESTRAS) {
                indice_tabla_onda = 0;
            }
        } else {
            DAC_UpdateValue(0);
        }

        // LED indicador de actividad
        static uint16_t led_counter = 0;
        led_counter++;
        if(led_counter >= LED_ALTERNAR_INTERRUPCIONES) {
            led_counter = 0;
            if(GPIO_ReadValue(PORT_CERO) & PIN_22) {
                GPIO_ClearPins(PORT_CERO, PIN_22);
            } else {
                GPIO_SetPins(PORT_CERO, PIN_22);
            }
        }
    }
}

/**
 * @brief ISR del Timer1 - Contador de tiempo (1ms)
 */
void TIMER1_IRQHandler(void) {
    if(TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        tiempo_transcurrido_ms++;
    }
}

/* ==================== FUNCIONES PRIVADAS ================================== */

/**
 * @brief Configura la frecuencia de reproducción
 */
static void set_frecuencia(uint16_t frecuencia_hz) {
    if (frecuencia_hz == 0 || frecuencia_hz == SILENCIO) {
        reproduciendo = 0;
        DAC_UpdateValue(0);
        frecuencia_actual = 0;
        indice_tabla_onda = 0;
        dma_melodias_index = 0;
        return;
    }

    if (frecuencia_hz < 50 || frecuencia_hz > 5000) {
        return;
    }

    uint32_t periodo_completo_us = MICROSEGUNDOS_POR_SEGUNDO / frecuencia_hz;
    uint32_t tiempo_entre_muestras_us = periodo_completo_us / NUMERO_MUESTRAS;

    if (tiempo_entre_muestras_us < 5) {
        tiempo_entre_muestras_us = 5;  // Reducido de 10 para más velocidad
    }

    TIM_Cmd(LPC_TIM0, DISABLE);
    TIM_ResetCounter(LPC_TIM0);
    TIM_UpdateMatchValue(LPC_TIM0, TIM_MATCH_CHANNEL_0, tiempo_entre_muestras_us);

    indice_tabla_onda = 0;
    dma_melodias_index = 0;
    frecuencia_actual = frecuencia_hz;
    reproduciendo = 1;
    
    /* Iniciar DMA si no está activo */
    if (!dma_melodias_enabled) {
        melodias_dma_start_transfer();
    }

    TIM_Cmd(LPC_TIM0, ENABLE);
}

/**
 * @brief Configura GPIO (LED indicador)
 */
static void config_gpio(void) {
    PINSEL_CFG_Type pin_cfg;

    pin_cfg.portNum = PINSEL_PORT_0;
    pin_cfg.pinNum = PINSEL_PIN_22;
    pin_cfg.funcNum = PINSEL_FUNC_0;
    pin_cfg.pinMode = PINSEL_PULLUP;
    pin_cfg.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    GPIO_SetDir(PORT_CERO, PIN_22, 1);
}

/**
 * @brief Configura DAC (P0.26)
 */
static void config_dac(void) {
    PINSEL_CFG_Type pin_cfg;

    pin_cfg.portNum = PINSEL_PORT_0;
    pin_cfg.pinNum = PINSEL_PIN_26;
    pin_cfg.funcNum = PINSEL_FUNC_2;  // AOUT
    pin_cfg.pinMode = PINSEL_TRISTATE;
    pin_cfg.openDrain = PINSEL_OD_NORMAL;
    PINSEL_ConfigPin(&pin_cfg);

    DAC_Init();
    DAC_SetBias(0);
    DAC_UpdateValue(0);
}

/**
 * @brief Configura Timer0 (audio) y Timer1 (tiempo)
 */
static void config_timer(void) {
    TIM_TIMERCFG_Type cfgtimer;
    TIM_MATCHCFG_Type cfgmatch;

    // Timer0 - Audio
    cfgtimer.prescaleOption = TIM_USVAL;
    cfgtimer.prescaleValue = 1;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgtimer);

    cfgmatch.matchChannel = 0;
    cfgmatch.intOnMatch = ENABLE;
    cfgmatch.resetOnMatch = ENABLE;
    cfgmatch.stopOnMatch = DISABLE;
    cfgmatch.extMatchOutputType = TIM_NOTHING;
    cfgmatch.matchValue = 100;
    TIM_ConfigMatch(LPC_TIM0, &cfgmatch);

    NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_SetPriority(TIMER0_IRQn, 1);
    TIM_Cmd(LPC_TIM0, ENABLE);

    // Timer1 - Tiempo (1ms)
    cfgtimer.prescaleOption = TIM_USVAL;
    cfgtimer.prescaleValue = 1;
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &cfgtimer);

    cfgmatch.matchChannel = 0;
    cfgmatch.intOnMatch = ENABLE;
    cfgmatch.resetOnMatch = ENABLE;
    cfgmatch.stopOnMatch = DISABLE;
    cfgmatch.extMatchOutputType = TIM_NOTHING;
    cfgmatch.matchValue = 1000;
    TIM_ConfigMatch(LPC_TIM1, &cfgmatch);

    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_SetPriority(TIMER1_IRQn, 2);
    TIM_Cmd(LPC_TIM1, ENABLE);
}

/* ==================== FUNCIONES PÚBLICAS ================================== */

void melodias_inicializar(void) {
    config_gpio();
    config_dac();
    config_timer();
    melodias_dma_init();  /* Inicializar DMA */
    GPIO_SetPins(PORT_CERO, PIN_22);
}

void melodias_iniciar(const Nota *melodia) {
    if (melodia == NULL) return;

    // Si hay música de fondo en loop, guardarla para reanudarla después
    if (modo_loop && melodia_actual != NULL) {
        melodia_fondo_guardada = melodia_actual;
        indice_fondo_guardado = indice_nota_actual;
        tiempo_fondo_guardado = tiempo_inicio_nota;
    }

    melodia_actual = melodia;
    indice_nota_actual = 0;
    tiempo_inicio_nota = tiempo_transcurrido_ms;
    modo_loop = 0;  // Modo normal (una sola reproducción)
    set_frecuencia(melodia[0].frecuencia);
}

void melodias_iniciar_loop(const Nota *melodia) {
    if (melodia == NULL) return;

    // No guardar nada si iniciamos un loop nuevo
    melodia_fondo_guardada = NULL;

    melodia_actual = melodia;
    indice_nota_actual = 0;
    tiempo_inicio_nota = tiempo_transcurrido_ms;
    modo_loop = 1;  // Modo loop (repetir al terminar)
    set_frecuencia(melodia[0].frecuencia);
}

void melodias_detener(void) {
    melodia_actual = NULL;
    melodia_fondo_guardada = NULL;
    indice_nota_actual = 0;
    modo_loop = 0;
    set_frecuencia(0);
    DAC_UpdateValue(0);
}

void melodias_actualizar(void) {
    if (melodia_actual == NULL) return;

    uint32_t tiempo_actual = tiempo_transcurrido_ms;
    uint32_t duracion_nota = melodia_actual[indice_nota_actual].duracion;
    uint32_t tiempo_transcurrido_nota = tiempo_actual - tiempo_inicio_nota;

    if (tiempo_transcurrido_nota >= duracion_nota) {
        uint32_t tiempo_total_con_pausa = duracion_nota + PAUSA_ARTICULACION_MS;

        if (tiempo_transcurrido_nota < tiempo_total_con_pausa) {
            set_frecuencia(0);
            return;
        }

        indice_nota_actual++;

        // Verificar si llegamos al final de la melodía
        if (melodia_actual[indice_nota_actual].frecuencia == SILENCIO &&
            melodia_actual[indice_nota_actual].duracion == 0) {

            // Si está en modo loop, reiniciar desde el principio
            if (modo_loop) {
                indice_nota_actual = 0;
                set_frecuencia(melodia_actual[0].frecuencia);
                tiempo_inicio_nota = tiempo_actual;
            } else {
                // Si hay música de fondo guardada, reanudarla
                if (melodia_fondo_guardada != NULL) {
                    melodia_actual = melodia_fondo_guardada;
                    indice_nota_actual = indice_fondo_guardado;
                    tiempo_inicio_nota = tiempo_actual - (tiempo_fondo_guardado - tiempo_actual);
                    modo_loop = 1;  // Restaurar modo loop
                    melodia_fondo_guardada = NULL;
                    set_frecuencia(melodia_actual[indice_nota_actual].frecuencia);
                } else {
                    // Si no hay música guardada, detener
                    melodias_detener();
                }
            }
            return;
        }

        set_frecuencia(melodia_actual[indice_nota_actual].frecuencia);
        tiempo_inicio_nota = tiempo_actual;
    }
}

uint8_t melodias_esta_sonando(void) {
    return (melodia_actual != NULL);
}

uint32_t melodias_obtener_tiempo_ms(void) {
    return tiempo_transcurrido_ms;
}

void melodias_establecer_volumen(uint8_t volumen_porcentaje) {
    if (volumen_porcentaje > 100) volumen_porcentaje = 100;
    volumen_porcentaje = volumen_porcentaje;
}
