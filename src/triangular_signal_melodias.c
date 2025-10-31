#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_dac.h"

#define SALIDA          (uint8_t) 1
#define ENTRADA         (uint8_t) 0

#define PIN_22          ((uint32_t)(1<<22))

#define PORT_CERO       (uint8_t) 0
#define PORT_UNO        (uint8_t) 1
#define PORT_DOS        (uint8_t) 2
#define PORT_TRES       (uint8_t) 3

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

/* ========================== ESTRUCTURA DE NOTA ============================ */

typedef struct {
    uint16_t frecuencia;
    uint16_t duracion;
} Nota;

/* ============================= MELODÍAS ===================================== */

// Melodía 1: "Happy Birthday"
const Nota melodia_happy_birthday[] = {
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
const Nota melodia_mario[] = {
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
const Nota melodia_tetris[] = {
    {MI_4, NEGRA}, {SI_3, CORCHEA}, {DO_4, CORCHEA}, {RE_4, NEGRA},
    {DO_4, CORCHEA}, {SI_3, CORCHEA}, {LA_3, NEGRA}, {LA_3, CORCHEA},
    {DO_4, CORCHEA}, {MI_4, NEGRA}, {RE_4, CORCHEA}, {DO_4, CORCHEA},
    {SI_3, NEGRA+CORCHEA}, {DO_4, CORCHEA}, {RE_4, NEGRA}, {MI_4, NEGRA},
    {DO_4, NEGRA}, {LA_3, NEGRA}, {LA_3, NEGRA}, {SILENCIO, CORCHEA},
    {SILENCIO, 0}
};

// Melodía 4: "Nokia Tune"
const Nota melodia_nokia[] = {
    {MI_5, CORCHEA}, {RE_5, CORCHEA}, {FA_S4, NEGRA}, {SOL_S4, NEGRA},
    {DO_S5, CORCHEA}, {SI_4, CORCHEA}, {RE_4, NEGRA}, {MI_4, NEGRA},
    {SI_4, CORCHEA}, {LA_4, CORCHEA}, {DO_S4, NEGRA}, {MI_4, NEGRA},
    {LA_4, BLANCA},
    {SILENCIO, 0}
};

// Configuración para generación de ondas
#define NUMERO_MUESTRAS    64           // Número de muestras en la tabla (optimizado)
#define MAXIMO_VALOR_DAC   1023        // 10-bit DAC (0-1023)

// Tabla de onda triangular (64 muestras para mejor rendimiento)
const uint16_t TABLA_TRIANGULAR[NUMERO_MUESTRAS] = {
    0,    32,   64,   96,   128,  160,  192,  224,  256,  288,  320,  352,  384,  416,  448,  480,
    512,  544,  576,  608,  640,  672,  704,  736,  768,  800,  832,  864,  896,  928,  960,  992,
    1023, 992,  960,  928,  896,  864,  832,  800,  768,  736,  704,  672,  640,  608,  576,  544,
    512,  480,  448,  416,  384,  352,  320,  288,  256,  224,  192,  160,  128,  96,   64,   32
};

/* ========================== VARIABLES GLOBALES ============================ */

volatile uint8_t INDICE_TABLA_DE_ONDA = 0;     // Índice actual en la tabla
volatile uint32_t nota_duracion_ms = 0;         // Duración restante de la nota
volatile uint16_t frecuencia_actual = 0;        // Frecuencia que se está reproduciendo
volatile uint8_t reproduciendo = 0;             // Flag de reproducción activa

/* ========================== DECLARACIONES DE FUNCIONES ==================== */

void config_GPIO(void);
void config_timer(void);
void config_DAC(void);
void set_frecuencia(uint16_t frecuencia_hz);
void generar_nota(uint16_t frecuencia, uint16_t duracion);
void reproducir_melodia(const Nota *melodia);
void delay_ms(uint32_t ms);

/* ===================== MANEJADOR DE INTERRUPCIONES ======================= */

/**
 * @brief Manejador de interrupción del Timer0
 * @note Match 0: Actualiza DAC con tabla de onda (genera frecuencia)
 *       Match 1: Contador de milisegundos (controla duración)
 */
void TIMER0_IRQHandler(void){
    // Match 0: Generar forma de onda
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

        if (reproduciendo) {
            // Escribir valor actual de la onda al DAC
            DAC_UpdateValue(TABLA_TRIANGULAR[INDICE_TABLA_DE_ONDA]);

            // Avanzar al siguiente punto de la onda
            INDICE_TABLA_DE_ONDA++;
            if(INDICE_TABLA_DE_ONDA >= NUMERO_MUESTRAS) {
                INDICE_TABLA_DE_ONDA = 0;  // Reiniciar al principio de la onda
            }
        }

        // PARPADEAR LED (indicador de actividad)
        static uint16_t led_counter = 0;
        if(++led_counter >= 500) {  // Toggle cada 500 interrupciones
            led_counter = 0;
            if(GPIO_ReadValue(PORT_CERO) & PIN_22) {
                GPIO_ClearPins(PORT_CERO, PIN_22);
            } else {
                GPIO_SetPins(PORT_CERO, PIN_22);
            }
        }
    }
    
    // Match 1: Contador de milisegundos para duración de notas
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
        
        if (nota_duracion_ms > 0) {
            nota_duracion_ms--;
        }
    }
    
    return;
}

/* ======================= FUNCIONES DE UTILIDAD ============================ */

/**
 * @brief Retardo en milisegundos
 * @param ms: cantidad de milisegundos a esperar
 */
void delay_ms(uint32_t ms) {
    nota_duracion_ms = ms;
    while (nota_duracion_ms > 0) {
        __NOP();
    }
}

/**
 * @brief Configura la frecuencia del timer para una nota específica
 * @param frecuencia_hz: frecuencia de la nota en Hz (0 = silencio)
 */
void set_frecuencia(uint16_t frecuencia_hz) {
    if (frecuencia_hz == 0 || frecuencia_hz == SILENCIO) {
        // Silencio: detener generación
        TIM_Cmd(LPC_TIM0, DISABLE);
        DAC_UpdateValue(0);
        reproduciendo = 0;
        frecuencia_actual = 0;
        return;
    }
    
    // Calcular período de muestreo para la frecuencia deseada
    // T_muestreo = Período_nota / N_muestras
    // T_muestreo (μs) = (1000000 / frecuencia) / NUMERO_MUESTRAS
    uint32_t periodo_us = 1000000 / frecuencia_hz;           // Período en microsegundos
    uint32_t match_value = periodo_us / NUMERO_MUESTRAS;    // Tiempo por muestra
    
    // Actualizar Match 0 con el nuevo valor
    TIM_UpdateMatchValue(LPC_TIM0, TIM_MATCH_0, match_value);
    
    // Reiniciar contador y estado
    TIM_ResetCounter(LPC_TIM0);
    INDICE_TABLA_DE_ONDA = 0;
    frecuencia_actual = frecuencia_hz;
    reproduciendo = 1;
    
    // Iniciar timer
    TIM_Cmd(LPC_TIM0, ENABLE);
}

/**
 * @brief Genera una nota musical con onda triangular
 * @param frecuencia: frecuencia de la nota en Hz (o 0 para silencio)
 * @param duracion: duración de la nota en milisegundos
 */
void generar_nota(uint16_t frecuencia, uint16_t duracion) {
    if (frecuencia == 0 || frecuencia == SILENCIO) {
        // Silencio
        set_frecuencia(0);
        delay_ms(duracion);
        return;
    }
    
    // Configurar frecuencia
    set_frecuencia(frecuencia);
    
    // Esperar la duración de la nota
    delay_ms(duracion);
    
    // Detener sonido al finalizar la nota
    set_frecuencia(0);
}

/**
 * @brief Reproduce una melodía completa
 * @param melodia: puntero al arreglo de notas (debe terminar con {SILENCIO, 0})
 */
void reproducir_melodia(const Nota *melodia) {
    uint16_t i = 0;
    
    while (melodia[i].frecuencia != 0 || melodia[i].duracion != 0) {
        generar_nota(melodia[i].frecuencia, melodia[i].duracion);
        
        // Pequeña pausa entre notas para mejor articulación
        DAC_UpdateValue(0);
        delay_ms(30);
        
        i++;
    }
    
    // Silencio al final
    set_frecuencia(0);
    DAC_UpdateValue(0);
}

/* ============================== MAIN ====================================== */

int main(void) {
    // Configurar hardware
    config_GPIO();
    config_DAC();
    config_timer();

    // Inicializar LED (indicador de actividad)
    GPIO_SetPins(PORT_CERO, PIN_22);
    
    delay_ms(1000);  // Esperar 1 segundo antes de comenzar
    
    // Reproducir melodías en secuencia
    while(1) {
        // Melodía 1: Super Mario Bros
        reproducir_melodia(melodia_mario);
        delay_ms(2000);
        
        // Melodía 2: Nokia Tune
        reproducir_melodia(melodia_nokia);
        delay_ms(2000);
        
        // Melodía 3: Tetris
        reproducir_melodia(melodia_tetris);
        delay_ms(2000);
        
        // Melodía 4: Happy Birthday
        reproducir_melodia(melodia_happy_birthday);
        delay_ms(3000);
        
        // Escala musical de prueba
        generar_nota(DO_4, NEGRA);
        generar_nota(RE_4, NEGRA);
        generar_nota(MI_4, NEGRA);
        generar_nota(FA_4, NEGRA);
        generar_nota(SOL_4, NEGRA);
        generar_nota(LA_4, NEGRA);
        generar_nota(SI_4, NEGRA);
        generar_nota(DO_5, NEGRA);
        delay_ms(3000);
    }

    return 0;
}

void config_GPIO(){
    PINSEL_CFG_Type pin_configuration;

    // Configurar P0.22 como GPIO (LED)
    pin_configuration.portNum   = PINSEL_PORT_0;
    pin_configuration.pinNum    = PINSEL_PIN_22;
    pin_configuration.pinMode   = PINSEL_PULLUP;
    pin_configuration.funcNum   = PINSEL_FUNC_0;  // GPIO
    pin_configuration.openDrain = PINSEL_OD_NORMAL;

    PINSEL_ConfigPin(&pin_configuration);

    //  Configurar P0.26 como AOUT (salida DAC)
    pin_configuration.portNum   = PINSEL_PORT_0;
    pin_configuration.pinNum    = PINSEL_PIN_26;  // Pin del DAC
    pin_configuration.funcNum   = PINSEL_FUNC_2;  // AOUT función
    pin_configuration.pinMode   = PINSEL_TRISTATE;
    pin_configuration.openDrain = PINSEL_OD_NORMAL;

    PINSEL_ConfigPin(&pin_configuration);

    GPIO_SetDir(PORT_CERO, PIN_22, SALIDA);
    return;
}

void config_DAC(void) {
    DAC_Init();
    // Configurar para máxima velocidad
    DAC_SetBias(0);

    // Valor inicial
    DAC_UpdateValue(0);
}

/**
 * @brief Configura el Timer0 para generación de frecuencias musicales
 * @note Usa Match 0 para frecuencia (variable) y Match 1 para contador de 1ms
 */
void config_timer(){
    TIM_TIMERCFG_Type cfgtimer;
    TIM_MATCHCFG_Type cfgmatch;

    // Configuración del Timer0: modo timer, prescaler = 1μs
    cfgtimer.prescaleOption = TIM_USVAL;
    cfgtimer.prescaleValue  = 1;  // 1 microsegundo por tick

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgtimer);

    // Configurar Match 0 para frecuencia de muestreo (se actualiza dinámicamente)
    cfgmatch.matchChannel        = 0;
    cfgmatch.intOnMatch          = ENABLE;
    cfgmatch.resetOnMatch        = ENABLE;   // Reset contador en match
    cfgmatch.stopOnMatch         = DISABLE;
    cfgmatch.extMatchOutputType  = TIM_NOTHING;
    cfgmatch.matchValue          = 1000;     // Valor inicial (se actualiza después)

    TIM_ConfigMatch(LPC_TIM0, &cfgmatch);
    
    // Configurar Match 1 para contador de milisegundos (1ms = 1000μs)
    cfgmatch.matchChannel        = 1;
    cfgmatch.intOnMatch          = ENABLE;
    cfgmatch.resetOnMatch        = DISABLE;  // NO reset contador (cuenta continuo)
    cfgmatch.stopOnMatch         = DISABLE;
    cfgmatch.extMatchOutputType  = TIM_NOTHING;
    cfgmatch.matchValue          = 1000;     // 1000μs = 1ms
    
    TIM_ConfigMatch(LPC_TIM0, &cfgmatch);

    // Habilitar interrupción del Timer0
    NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_SetPriority(TIMER0_IRQn, 1);
    
    // NO iniciar el timer aún (se inicia al reproducir primera nota)
    // TIM_Cmd(LPC_TIM0, ENABLE);

    return;
}