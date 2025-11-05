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

/* ==================== CONFIGURACIÓN DE GENERACIÓN DE AUDIO ================ */

// Número de puntos en la tabla de onda (más muestras = mayor calidad)
#define NUMERO_MUESTRAS    64

// Valor máximo del DAC de 10 bits (2^10 - 1 = 1023)
#define MAXIMO_VALOR_DAC   1023

// Conversión de tiempo
#define MICROSEGUNDOS_POR_SEGUNDO  1000000  // 1 segundo = 1,000,000 microsegundos

// Configuración de pausas entre notas
#define PAUSA_ARTICULACION_MS      30       // Pausa de 30ms entre notas para claridad
#define PAUSA_ENTRE_MELODIAS_MS    3000     // Pausa de 3 segundos entre melodías

// Configuración del LED de actividad
#define LED_TOGGLE_INTERRUPCIONES  500      // Toggle LED cada 500 interrupciones

// Tabla de onda triangular (64 muestras para mejor rendimiento)
const uint16_t TABLA_TRIANGULAR[NUMERO_MUESTRAS] = {
    0,    32,   64,   96,   128,  160,  192,  224,  256,  288,  320,  352,  384,  416,  448,  480,
    512,  544,  576,  608,  640,  672,  704,  736,  768,  800,  832,  864,  896,  928,  960,  992,
    1023, 992,  960,  928,  896,  864,  832,  800,  768,  736,  704,  672,  640,  608,  576,  544,
    512,  480,  448,  416,  384,  352,  320,  288,  256,  224,  192,  160,  128,  96,   64,   32
};

/* ========================== VARIABLES GLOBALES ============================ */

/* ─────────────────────────────────────────────────────────────────────────
 * VARIABLES DE GENERACIÓN DE AUDIO (usadas por el IRQHandler)
 * ───────────────────────────────────────────────────────────────────────── */

// Índice actual en TABLA_TRIANGULAR (0-63)
volatile uint8_t INDICE_TABLA_DE_ONDA = 0;

// Duración restante de la nota (legacy - no se usa actualmente)
volatile uint32_t nota_duracion_ms = 0;

// Frecuencia que se está reproduciendo actualmente (en Hz)
volatile uint16_t frecuencia_actual = 0;

// Flag que controla si el DAC genera audio o está en silencio
// - 1: El IRQHandler escribe valores de TABLA_TRIANGULAR al DAC
// - 0: El IRQHandler NO escribe al DAC (silencio)
// CAMBIA EN: set_frecuencia()
volatile uint8_t reproduciendo = 0;

// Contador de tiempo global (incrementado cada 1ms por Match 1)
volatile uint32_t tiempo_transcurrido_ms = 0;

/* ─────────────────────────────────────────────────────────────────────────
 * VARIABLES DE CONTROL DE MELODÍA (usadas por el main loop)
 * ───────────────────────────────────────────────────────────────────────── */

// Puntero a la melodía que se está reproduciendo
// - NULL: No hay melodía activa
// - !NULL: Hay una melodía en curso
// CAMBIA EN: iniciar_melodia() (se asigna), detener_melodia() y actualizar_melodia() (se pone NULL)
const Nota *melodia_actual = NULL;

// Índice de la nota actual dentro de melodia_actual[]
volatile uint16_t indice_nota_actual = 0;

// Timestamp en que comenzó la nota actual (para calcular duración)
volatile uint32_t tiempo_inicio_nota = 0;

/* ========================== DECLARACIONES DE FUNCIONES ==================== */

void config_GPIO(void);
void config_timer(void);
void config_DAC(void);
void set_frecuencia(uint16_t frecuencia_hz);
void iniciar_melodia(const Nota *melodia);
void detener_melodia(void);
void actualizar_melodia(void);  // Función no bloqueante para avanzar melodía
uint8_t esta_reproduciendo(void);
uint32_t obtener_tiempo_ms(void);  // Obtener tiempo global

/* ===================== MANEJADOR DE INTERRUPCIONES ======================= */

/**
 * @brief Manejador de interrupción del Timer0 - GENERACIÓN DE AUDIO
 * 
 * Match 0 (Generación de audio):
 *   - Interrumpe a velocidad variable (según la nota musical)
 *   - Lee un valor de TABLA_TRIANGULAR[] y lo envía al DAC
 *   - Avanza al siguiente punto de la tabla
 *   - Genera la forma de onda triangular
 *   - El contador SE RESETEA automáticamente en cada match (más estable)
 * 
 * Ejemplo: Para tocar DO_4 (262Hz):
 *   - Match 0 interrumpe cada ~60μs (genera onda)
 */
void TIMER0_IRQHandler(void){
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

        // Solo genera onda si hay una nota activa
        if (reproduciendo && frecuencia_actual > 0) {
            // 1. Leer el valor actual de la tabla de onda triangular
            uint16_t valor_dac = TABLA_TRIANGULAR[INDICE_TABLA_DE_ONDA];
            
            // 2. Enviar ese valor al DAC (pin P0.26)
            DAC_UpdateValue(valor_dac);

            // 3. Avanzar al siguiente punto de la tabla
            INDICE_TABLA_DE_ONDA++;
            
            // 4. Si llegamos al final, volver al inicio (ciclo continuo)
            if(INDICE_TABLA_DE_ONDA >= NUMERO_MUESTRAS) {
                INDICE_TABLA_DE_ONDA = 0;
            }
        } else {
            // Si no hay nota activa, asegurar DAC en 0
            DAC_UpdateValue(0);
        }

        // Indicador visual de actividad (LED parpadea)
        static uint16_t led_counter = 0;
        led_counter++;
        
        if(led_counter >= LED_TOGGLE_INTERRUPCIONES) {
            led_counter = 0;
            
            // Toggle del LED en P0.22
            if(GPIO_ReadValue(PORT_CERO) & PIN_22) {
                GPIO_ClearPins(PORT_CERO, PIN_22);
            } else {
                GPIO_SetPins(PORT_CERO, PIN_22);
            }
        }
    }
    
    return;
}

/**
 * @brief Manejador de interrupción del Timer1 - CONTADOR DE TIEMPO
 * 
 * Match 0 del Timer1:
 *   - Interrumpe cada 1 milisegundo (fijo)
 *   - Incrementa contador de tiempo global
 *   - Sirve para medir duraciones de notas
 *   - Útil para el timing del juego
 *   - El contador SE RESETEA automáticamente cada 1ms
 */
void TIMER1_IRQHandler(void){
    if(TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)) {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        
        // Incrementar contador global de milisegundos
        tiempo_transcurrido_ms++;
        
        // Decrementar contador auxiliar
        if (nota_duracion_ms > 0) {
            nota_duracion_ms--;
        }
    }
    
    return;
}

/* ======================= FUNCIONES DE UTILIDAD ============================ */

/**
 * @brief Obtiene el tiempo transcurrido desde el inicio en milisegundos
 * @return Tiempo en milisegundos
 * @note Útil para el loop del juego
 */
uint32_t obtener_tiempo_ms(void) {
    return tiempo_transcurrido_ms;
}

/**
 * @brief Verifica si hay una melodía activa en reproducción
 * 
 * Esta función verifica si HAY UNA MELODÍA EN CURSO, no si está sonando
 * en este instante preciso. Una melodía puede estar activa pero en silencio
 * momentáneo (pausas entre notas, notas de silencio, etc.).
 * 
 * Diferencia entre las banderas:
 * 
 *   melodia_actual (puntero):
 *     - NULL: No hay melodía en curso
 *     - !NULL: Hay una melodía activa (puede estar en pausa/silencio)
 *     - Se actualiza en: iniciar_melodia(), detener_melodia(), actualizar_melodia()
 * 
 *   reproduciendo (flag):
 *     - 0: DAC no está generando audio (silencio)
 *     - 1: DAC está generando audio activamente
 *     - Se actualiza en: set_frecuencia()
 *     - Puede cambiar múltiples veces durante una melodía
 * 
 * Ejemplo de estados durante reproducción:
 *   t=0ms:    melodia_actual=melodia_mario, reproduciendo=1  (DO_4 sonando)
 *   t=500ms:  melodia_actual=melodia_mario, reproduciendo=0  (pausa articulación)
 *   t=530ms:  melodia_actual=melodia_mario, reproduciendo=1  (MI_4 sonando)
 *   t=10000ms: melodia_actual=NULL, reproduciendo=0          (melodía terminó)
 * 
 * @return 1 si hay una melodía activa (aunque esté en silencio momentáneo)
 *         0 si no hay ninguna melodía en curso
 * 
 * @note Esta función verifica melodia_actual, NO reproduciendo
 */
uint8_t esta_reproduciendo(void) {
    return (melodia_actual != NULL);
}

/**
 * @brief Configura la frecuencia de reproducción para una nota musical
 * 
 * VERSIÓN SIMPLIFICADA CON RESET EN MATCH
 * 
 * Esta función calcula la velocidad a la que Match 0 debe interrumpir para
 * generar la frecuencia deseada. Como el contador se RESETEA en cada match,
 * el cálculo es directo y estable.
 * 
 * Proceso:
 * 1. Calcula el período de UN ciclo completo de la nota (en microsegundos)
 * 2. Divide ese período entre 64 muestras
 * 3. Configura Match 0 con ese valor
 * 4. El timer se resetea automáticamente, generando interrupciones estables
 * 
 * Ejemplo para DO_4 (262 Hz):
 *   - Período = 1,000,000 / 262 = 3,816 μs (un ciclo completo)
 *   - Match 0 = 3,816 / 64 = 60 μs (tiempo entre muestras)
 *   - Timer cuenta: 0→60 (reset) 0→60 (reset) ... muy estable
 * 
 * @param frecuencia_hz Frecuencia deseada en Hertz (0 = silencio)
 */
void set_frecuencia(uint16_t frecuencia_hz) {
    // Caso especial: Silencio
    if (frecuencia_hz == 0 || frecuencia_hz == SILENCIO) {
        reproduciendo = 0;              // Desactivar generación de onda
        DAC_UpdateValue(0);             // DAC a 0V
        frecuencia_actual = 0;
        INDICE_TABLA_DE_ONDA = 0;
        return;
    }
    
    // Validar rango de frecuencias
    if (frecuencia_hz < 50 || frecuencia_hz > 5000) {
        return;  // Frecuencia fuera de rango
    }
    
    /* ─────────────────────────────────────────────────────────────────────
     * CÁLCULO DE PERÍODO DE MUESTREO (SIMPLIFICADO)
     * ───────────────────────────────────────────────────────────────────── */
    
    // 1. Período de UN CICLO completo de la nota (en microsegundos)
    uint32_t periodo_completo_us = MICROSEGUNDOS_POR_SEGUNDO / frecuencia_hz;
    
    // 2. Tiempo entre cada muestra de la tabla
    uint32_t tiempo_entre_muestras_us = periodo_completo_us / NUMERO_MUESTRAS;
    
    // Validar que el tiempo no sea demasiado pequeño
    if (tiempo_entre_muestras_us < 10) {
        tiempo_entre_muestras_us = 10;  // Mínimo 10μs
    }
    
    /* ─────────────────────────────────────────────────────────────────────
     * CONFIGURACIÓN DEL TIMER (MODO RESET)
     * ───────────────────────────────────────────────────────────────────── */
    
    // Detener temporalmente el timer para configuración segura
    TIM_Cmd(LPC_TIM0, DISABLE);
    
    // Resetear el contador a 0
    TIM_ResetCounter(LPC_TIM0);
    
    // Configurar el valor de match (el timer se reseteará al llegar aquí)
    TIM_UpdateMatchValue(LPC_TIM0, TIM_MATCH_0, tiempo_entre_muestras_us);
    
    // Reiniciar índice de la tabla
    INDICE_TABLA_DE_ONDA = 0;
    
    // Guardar frecuencia y activar reproducción
    frecuencia_actual = frecuencia_hz;
    reproduciendo = 1;
    
    // Reiniciar el timer
    TIM_Cmd(LPC_TIM0, ENABLE);
}

/**
 * @brief Inicia la reproducción de una melodía de forma no bloqueante
 * 
 * Esta función prepara todo para comenzar a tocar una melodía, pero NO espera
 * a que termine. El avance de la melodía se maneja con actualizar_melodia()
 * que debe llamarse repetidamente desde el loop principal.
 * 
 * Proceso:
 * 1. Guarda el puntero a la melodía
 * 2. Reinicia el índice a la primera nota
 * 3. Marca el tiempo de inicio
 * 4. Configura y empieza a tocar la primera nota
 * 
 * Estado de banderas después de llamar esta función:
 *   melodia_actual = [puntero a la melodía]  (ya NO es NULL)
 *   reproduciendo = 1                         (set_frecuencia() lo activa)
 *   indice_nota_actual = 0                    (primera nota)
 * 
 * @param melodia Puntero al arreglo de notas (debe terminar con {SILENCIO, 0})
 * 
 * @note Esta función retorna inmediatamente. La melodía sigue tocando en
 *       segundo plano gracias a las interrupciones del timer.
 */
void iniciar_melodia(const Nota *melodia) {
    if (melodia == NULL) return;
    
    // Guardar referencia a la melodía (melodia_actual deja de ser NULL)
    melodia_actual = melodia;
    
    // Empezar desde la primera nota
    indice_nota_actual = 0;
    
    // Marcar el tiempo de inicio para control de duración
    tiempo_inicio_nota = tiempo_transcurrido_ms;
    
    // Configurar y empezar a tocar la primera nota
    // (esto pone reproduciendo = 1 internamente)
    set_frecuencia(melodia[0].frecuencia);
}

/**
 * @brief Detiene la reproducción de la melodía actual
 * 
 * Esta función limpia todas las variables de estado y detiene el audio.
 * 
 * Estado de banderas después de llamar esta función:
 *   melodia_actual = NULL       (indica que no hay melodía activa)
 *   reproduciendo = 0            (set_frecuencia(0) lo desactiva)
 *   indice_nota_actual = 0       (reiniciado)
 */
void detener_melodia(void) {
    // Marcar que no hay melodía activa (melodia_actual se pone a NULL)
    melodia_actual = NULL;
    
    // Reiniciar índice
    indice_nota_actual = 0;
    
    // Detener generación de audio (esto pone reproduciendo = 0)
    set_frecuencia(0);
    
    // Asegurar que el DAC está en 0V
    DAC_UpdateValue(0);
}

/**
 * @brief Actualiza el estado de la melodía (función no bloqueante)
 * 
 * Esta función es el "director de orquesta". Debe llamarse repetidamente en el
 * loop principal del juego para que la melodía avance nota por nota.
 * 
 * Funcionamiento:
 * 1. Verifica si la nota actual ya cumplió su duración
 * 2. Si terminó, hace una pausa corta (30ms) para articulación
 * 3. Avanza a la siguiente nota
 * 4. Detecta si llegó al final de la melodía
 * 
 * Ejemplo de timeline:
 *   t=0ms:    DO_4 empieza (500ms de duración)
 *   t=500ms:  actualizar_melodia() detecta que terminó
 *   t=530ms:  Después de pausa, empieza MI_4
 * 
 * @note DEBE llamarse continuamente en el while(1) del main
 * @note NO bloquea la ejecución - retorna inmediatamente
 */
void actualizar_melodia(void) {
    // Si no hay melodía activa, no hacer nada
    if (melodia_actual == NULL) return;
    
    /* ─────────────────────────────────────────────────────────────────────
     * VERIFICAR DURACIÓN DE LA NOTA ACTUAL
     * ───────────────────────────────────────────────────────────────────── */
    
    uint32_t tiempo_actual = tiempo_transcurrido_ms;
    uint32_t duracion_nota = melodia_actual[indice_nota_actual].duracion;
    uint32_t tiempo_transcurrido_nota = tiempo_actual - tiempo_inicio_nota;
    
    // Verificar si la nota actual ya terminó su duración
    if (tiempo_transcurrido_nota >= duracion_nota) {
        
        /* ─────────────────────────────────────────────────────────────────
         * PAUSA DE ARTICULACIÓN
         * ─────────────────────────────────────────────────────────────────
         * Hacer una pausa breve entre notas para que se escuchen separadas.
         * Sin esta pausa, las notas se "pegarían" y sonarían como un tono
         * continuo que cambia de frecuencia.
         */
        uint32_t tiempo_total_con_pausa = duracion_nota + PAUSA_ARTICULACION_MS;
        
        if (tiempo_transcurrido_nota < tiempo_total_con_pausa) {
            set_frecuencia(0);  // Silencio durante la pausa
            return;
        }
        
        /* ─────────────────────────────────────────────────────────────────
         * AVANZAR A LA SIGUIENTE NOTA
         * ───────────────────────────────────────────────────────────────── */
        
        indice_nota_actual++;
        
        // Verificar si llegamos al final de la melodía
        // (el final está marcado con {SILENCIO, 0})
        if (melodia_actual[indice_nota_actual].frecuencia == SILENCIO && 
            melodia_actual[indice_nota_actual].duracion == 0) {
            // AQUÍ SE CAMBIA melodia_actual a NULL
            detener_melodia();
            return;
        }
        
        // Reproducir la siguiente nota
        // AQUÍ SE CAMBIA reproduciendo según la frecuencia
        set_frecuencia(melodia_actual[indice_nota_actual].frecuencia);
        tiempo_inicio_nota = tiempo_actual;
    }
}

/* ============================== MAIN ====================================== */

/**
 * @brief Programa principal - Reproductor de melodías no bloqueante
 * 
 * Este programa demuestra cómo reproducir melodías musicales en segundo plano
 * usando el DAC y Timer0, sin bloquear la ejecución del programa principal.
 * 
 * Arquitectura:
 * 
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │                    TIMER0_IRQHandler                        │
 *   │  (Interrupciones - generan el sonido en segundo plano)      │
 *   │                                                              │
 *   │  Match 0: Lee TABLA_TRIANGULAR → DAC (cada 20-100μs)       │
 *   │  Match 1: Incrementa tiempo_ms (cada 1ms)                   │
 *   └─────────────────────────────────────────────────────────────┘
 *                              ▲
 *                              │
 *                              │ Genera onda
 *                              │
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │                    while(1) - Main Loop                      │
 *   │  (Loop principal - controla la secuencia de notas)          │
 *   │                                                              │
 *   │  - actualizar_melodia() → cambia frecuencia cuando necesita │
 *   │  - Lógica del juego (Snake, etc.)                           │
 *   │  - Control de pausas entre melodías                         │
 *   └─────────────────────────────────────────────────────────────┘
 * 
 * Flujo de ejecución:
 * 1. Inicialización del hardware (GPIO, DAC, Timer)
 * 2. Iniciar primera melodía
 * 3. Loop infinito:
 *    a. actualizar_melodia() avanza nota si es necesario
 *    b. Lógica del juego (no implementada en este demo)
 *    c. Control de cambio de melodías
 */
int main(void) {
    
    /* ═════════════════════════════════════════════════════════════════════
     * INICIALIZACIÓN DEL HARDWARE
     * ═════════════════════════════════════════════════════════════════════ */
    
    config_GPIO();   // Configurar pines (LED y DAC)
    config_DAC();    // Inicializar convertidor digital-analógico
    config_timer();  // Configurar e iniciar Timer0 (Match 0 y Match 1)

    // Encender LED como indicador de que el sistema está listo
    GPIO_SetPins(PORT_CERO, PIN_22);
    
    /* ═════════════════════════════════════════════════════════════════════
     * VARIABLES DE CONTROL
     * ═════════════════════════════════════════════════════════════════════ */
    
    // Tiempo en que terminó la última melodía (para pausas entre melodías)
    uint32_t tiempo_fin_melodia = 0;
    
    // Índice de la melodía actual (0-3)
    uint8_t melodia_index = 0;
    
    // Flag para detectar transición de reproduciendo → terminada
    uint8_t melodia_terminada = 0;
    
    /* ═════════════════════════════════════════════════════════════════════
     * INICIAR REPRODUCCIÓN
     * ═════════════════════════════════════════════════════════════════════ */
    
    iniciar_melodia(melodia_mario);
    
    /* ═════════════════════════════════════════════════════════════════════
     * LOOP PRINCIPAL - NO BLOQUEANTE
     * ═════════════════════════════════════════════════════════════════════ */
    
    while(1) {
        
        /* ─────────────────────────────────────────────────────────────────
         * 1. ACTUALIZAR MELODÍA EN SEGUNDO PLANO
         * ─────────────────────────────────────────────────────────────────
         * Esta función verifica si la nota actual terminó y avanza a la
         * siguiente si es necesario. Es NO BLOQUEANTE - retorna
         * inmediatamente.
         */
        actualizar_melodia();
        
        /* ─────────────────────────────────────────────────────────────────
         * 2. LÓGICA DEL JUEGO
         * ─────────────────────────────────────────────────────────────────
         * AQUÍ VA EL CÓDIGO DE TU JUEGO (Snake, etc.):
         * 
         * - Leer controles (joystick, botones)
         * - Actualizar posición del personaje
         * - Detectar colisiones
         * - Actualizar pantalla LCD
         * - Calcular puntuación
         * - etc.
         * 
         * Todo esto se ejecuta mientras la música suena en segundo plano.
         */
        
        /* ─────────────────────────────────────────────────────────────────
         * 3. DEMO: ROTACIÓN AUTOMÁTICA DE MELODÍAS
         * ─────────────────────────────────────────────────────────────────
         * Este código demuestra cómo cambiar de melodía automáticamente.
         * En un juego real, cambiarías la melodía según eventos:
         * - Menú principal → melodía 1
         * - Jugando → melodía 2
         * - Game Over → melodía 3
         * - Victoria → melodía 4
         */
        
        // Detectar cuando UNA melodía termina (transición de sonando → silencio)
        if (!esta_reproduciendo() && !melodia_terminada) {
            tiempo_fin_melodia = obtener_tiempo_ms();
            melodia_terminada = 1;
        }
        
        // Esperar 3 segundos después de que terminó, luego iniciar la siguiente
        if (melodia_terminada) {
            uint32_t tiempo_actual = obtener_tiempo_ms();
            uint32_t tiempo_desde_fin = tiempo_actual - tiempo_fin_melodia;
            
            if (tiempo_desde_fin > PAUSA_ENTRE_MELODIAS_MS) {
                // Rotar al siguiente índice (0→1→2→3→0...)
                melodia_index = (melodia_index + 1) % 4;
                
                // Seleccionar e iniciar la melodía correspondiente
                switch(melodia_index) {
                    case 0:
                        iniciar_melodia(melodia_mario);
                        break;
                    case 1:
                        iniciar_melodia(melodia_nokia);
                        break;
                    case 2:
                        iniciar_melodia(melodia_tetris);
                        break;
                    case 3:
                        iniciar_melodia(melodia_happy_birthday);
                        break;
                }
                
                melodia_terminada = 0;  // Reset flag para próxima melodía
            }
        }
        
        __WFI();  // Wait For Interrupt
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
 * @brief Configura el Timer0 para generación de audio y base de tiempo
 * 
 * SOLUCIÓN SIMPLIFICADA - Usar Timer0 solo para audio y Timer1 para tiempo
 * 
 * Timer0 - Match 0 (AUDIO con RESET):
 *   - Genera interrupciones a velocidad VARIABLE (según la nota)
 *   - RESETEA el contador en cada match (más estable)
 *   - Cada interrupción actualiza un punto de la onda triangular
 *   - Se reconfigura dinámicamente con set_frecuencia()
 * 
 * Prescaler:
 *   - Configurado a 1 microsegundo por tick
 *   - Permite timing preciso para frecuencias de audio
 */
void config_timer(){
    TIM_TIMERCFG_Type cfgtimer;
    TIM_MATCHCFG_Type cfgmatch;

    /* ═════════════════════════════════════════════════════════════════════
     * CONFIGURACIÓN BÁSICA DEL TIMER0
     * ═════════════════════════════════════════════════════════════════════ */
    
    // Prescaler: 1 tick = 1 microsegundo
    cfgtimer.prescaleOption = TIM_USVAL;  // Unidad: microsegundos
    cfgtimer.prescaleValue  = 1;          // 1 microsegundo por tick

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgtimer);

    /* ═════════════════════════════════════════════════════════════════════
     * MATCH 0: GENERACIÓN DE AUDIO CON RESET
     * ═════════════════════════════════════════════════════════════════════ */
    
    cfgmatch.matchChannel        = 0;
    cfgmatch.intOnMatch          = ENABLE;     // Generar interrupción
    cfgmatch.resetOnMatch        = ENABLE;     // SÍ RESETEAR - más estable
    cfgmatch.stopOnMatch         = DISABLE;    // NO detener timer
    cfgmatch.extMatchOutputType  = TIM_NOTHING;// Sin salida externa
    cfgmatch.matchValue          = 100;        // Valor inicial conservador
    
    TIM_ConfigMatch(LPC_TIM0, &cfgmatch);
    
    /* ═════════════════════════════════════════════════════════════════════
     * CONFIGURAR TIMER1 PARA CONTADOR DE TIEMPO (1ms)
     * ═════════════════════════════════════════════════════════════════════ */
    
    // Timer1 independiente para la base de tiempo
    cfgtimer.prescaleOption = TIM_USVAL;
    cfgtimer.prescaleValue  = 1;
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &cfgtimer);
    
    cfgmatch.matchChannel        = 0;
    cfgmatch.intOnMatch          = ENABLE;
    cfgmatch.resetOnMatch        = ENABLE;     // Reset en cada ms
    cfgmatch.stopOnMatch         = DISABLE;
    cfgmatch.extMatchOutputType  = TIM_NOTHING;
    cfgmatch.matchValue          = 1000;       // 1000μs = 1ms
    
    TIM_ConfigMatch(LPC_TIM1, &cfgmatch);

    /* ═════════════════════════════════════════════════════════════════════
     * HABILITAR INTERRUPCIONES Y ARRANCAR TIMERS
     * ═════════════════════════════════════════════════════════════════════ */
    
    // Timer0 - Audio
    NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_SetPriority(TIMER0_IRQn, 1);
    TIM_Cmd(LPC_TIM0, ENABLE);
    
    // Timer1 - Tiempo
    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_SetPriority(TIMER1_IRQn, 2);  // Menor prioridad que audio
    TIM_Cmd(LPC_TIM1, ENABLE);

    return;
}