/**
 * @file    adc.c
 * @brief   Sistema de joystick con ADC + DMA + LEDs direccionales
 * @details Lee joystick analógico (X, Y) usando ADC0 canales 0 y 1
 *          DMA guarda 10 muestras por canal en buffers circulares
 *          Calcula promedios y controla 4 LEDs según dirección
 * 
 * Hardware:
 * - P0.23: ADC Canal 0 → Lee VRx (eje horizontal) → Controla HORIZONTAL (Izq/Der)
 * - P0.24: ADC Canal 1 → Lee VRy (eje vertical) → Controla VERTICAL (Arr/Abajo)
 * - P0.0: LED Izquierda  (se enciende cuando VRx bajo)
 * - P0.1: LED Derecha    (se enciende cuando VRx alto)
 * - P0.2: LED Arriba     (se enciende cuando VRy alto)
 * - P0.3: LED Abajo      (se enciende cuando VRy bajo)
 * 
 * @date    Noviembre 2025
 */

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

/* ========================== CONFIGURACIÓN ================================= */

// Canales ADC - MAPEO JOYSTICK KY-023
#define ADC_CHANNEL_X       0    // P0.23 - VRx (potenciómetro horizontal) → IZQUIERDA/DERECHA
#define ADC_CHANNEL_Y       1    // P0.24 - VRy (potenciómetro vertical) → ARRIBA/ABAJO

// PINES DEL ADC
#define ADC_X_PIN           PINSEL_PIN_23  // P0.23 para VRx (eje X)
#define ADC_Y_PIN           PINSEL_PIN_24  // P0.24 para VRy (eje Y)

// Tamaño de buffers (10 muestras por canal)
#define BUFFER_SIZE         10

// Pines de LEDs (Puerto 0)
#define LED_PORT            0

#define LED_IZQUIERDA_PIN   0    // P0.0
#define LED_DERECHA_PIN     1    // P0.1
#define LED_ARRIBA_PIN      2    // P0.2
#define LED_ABAJO_PIN       3    // P0.3

// Umbrales de detección (SE CALIBRAN AUTOMÁTICAMENTE)
#define UMBRAL_MIN          300   // Por debajo = movimiento
#define UMBRAL_MAX          700   // Por encima = movimiento

// Variables de calibración
static uint16_t centro_x = 500;  // Se calibra al inicio
static uint16_t centro_y = 500;  // Se calibra al inicio
static uint16_t deadzone = 30;   // Reducido para valores bajos del ADC

// Frecuencia ADC
#define ADC_FREQUENCY       100000  // 100 kHz

/* ========================== VARIABLES GLOBALES ============================ */

// Buffers DMA para cada canal (el driver ya alinea los datos correctamente)
static volatile uint16_t buffer_x[BUFFER_SIZE];
static volatile uint16_t buffer_y[BUFFER_SIZE];

// LLI structures (DEBEN ser estáticas para que persistan)
static GPDMA_LLI_Type lli_x;
static GPDMA_LLI_Type lli_y;

// Promedios calculados
static uint16_t promedio_x = 2048;
static uint16_t promedio_y = 2048;

// Flag de actualización
static volatile uint8_t datos_listos = 0;

/* ===================== DECLARACIONES DE FUNCIONES ========================= */

void config_gpio_leds(void);
void config_adc(void);
void config_dma(void);
void procesar_joystick(void);
void actualizar_leds(void);
uint16_t calcular_promedio(volatile uint16_t* buffer);

/* ==================== MANEJADOR DE INTERRUPCIONES ========================= */

/**
 * @brief Handler de interrupción del DMA
 * Se ejecuta cuando completa la transferencia de 10 muestras
 * 
 * Canal DMA 0 → Lee ADDR0 (VRx Canal 0 en P0.23) → buffer_x
 * Canal DMA 1 → Lee ADDR1 (VRy Canal 1 en P0.24) → buffer_y
 */
void DMA_IRQHandler(void) {
    // Canal DMA 0 (eje X horizontal) completó transferencia
    if (GPDMA_IntGetStatus(GPDMA_RAW_INTTC, 0)) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, 0);
    }
    
    // Canal DMA 1 (eje Y vertical) completó transferencia
    if (GPDMA_IntGetStatus(GPDMA_RAW_INTTC, 1)) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, 1);
        datos_listos = 1;  // Ambos canales listos
    }
    
    // Limpiar errores si existen
    if (GPDMA_IntGetStatus(GPDMA_RAW_INTERR, 0)) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, 0);
    }
    if (GPDMA_IntGetStatus(GPDMA_RAW_INTERR, 1)) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTERR, 1);
    }
}

/* ===================== IMPLEMENTACIÓN DE FUNCIONES ======================== */

/**
 * @brief Configura GPIOs para los 4 LEDs
 */
void config_gpio_leds(void) {
    PINSEL_CFG_Type pin_cfg;
    
    // Configurar pines como GPIO
    pin_cfg.funcNum = PINSEL_FUNC_0;
    pin_cfg.pinMode = PINSEL_TRISTATE;
    pin_cfg.openDrain = PINSEL_OD_NORMAL;
    pin_cfg.portNum = PINSEL_PORT_0;
    
    // P0.0 - LED Izquierda
    pin_cfg.pinNum = LED_IZQUIERDA_PIN;
    PINSEL_ConfigPin(&pin_cfg);
    
    // P0.1 - LED Derecha
    pin_cfg.pinNum = LED_DERECHA_PIN;
    PINSEL_ConfigPin(&pin_cfg);
    
    // P0.2 - LED Arriba
    pin_cfg.pinNum = LED_ARRIBA_PIN;
    PINSEL_ConfigPin(&pin_cfg);
    
    // P0.3 - LED Abajo
    pin_cfg.pinNum = LED_ABAJO_PIN;
    PINSEL_ConfigPin(&pin_cfg);
    
    // Configurar todos como salida
    GPIO_SetDir(LED_PORT, (1 << LED_IZQUIERDA_PIN) | 
                          (1 << LED_DERECHA_PIN) | 
                          (1 << LED_ARRIBA_PIN) | 
                          (1 << LED_ABAJO_PIN), 1);
    
    // Apagar todos inicialmente (nivel BAJO = apagado, alto = encendido)
    GPIO_ClearPins(LED_PORT, (1 << LED_IZQUIERDA_PIN) | 
                             (1 << LED_DERECHA_PIN) | 
                             (1 << LED_ARRIBA_PIN) | 
                             (1 << LED_ABAJO_PIN));
}

/**
 * @brief Configura ADC para leer 2 canales
 * 
 * MODOS DE OPERACIÓN:
 * - Sin DMA: Modo manual con START_NOW (burst desactivado)
 * - Con DMA: Modo burst automático (se activa en config_dma())
 */
void config_adc(void) {
    // Configurar pines como ADC
    PINSEL_CFG_Type pin_cfg;
    pin_cfg.funcNum = PINSEL_FUNC_1;  // Función 1 = ADC
    pin_cfg.pinMode = PINSEL_TRISTATE;
    pin_cfg.openDrain = PINSEL_OD_NORMAL;
    pin_cfg.portNum = PINSEL_PORT_0;
    
    // P0.23 -> Canal 0 (VRx - eje horizontal)
    pin_cfg.pinNum = ADC_X_PIN;
    PINSEL_ConfigPin(&pin_cfg);
    
    // P0.24 -> Canal 1 (VRy - eje vertical)
    pin_cfg.pinNum = ADC_Y_PIN;
    PINSEL_ConfigPin(&pin_cfg);
    
    // Inicializar ADC a 100kHz
    ADC_Init(ADC_FREQUENCY);
    
    // Habilitar canales 0 (VRx) y 1 (VRy)
    ADC_ChannelCmd(ADC_CHANNEL_X, ENABLE);
    ADC_ChannelCmd(ADC_CHANNEL_Y, ENABLE);
    
    // NO activar burst aquí - se hace en config_dma() si es necesario
    // Modo sin DMA usa START_NOW manual en leer_adc_directo()
}

/**
 * @brief Configura DMA para transferir datos del ADC a buffers
 * Usa 2 canales DMA (uno por cada eje X e Y)
 */
void config_dma(void) {
    GPDMA_Channel_CFG_Type dma_cfg;
    
    // Inicializar DMA
    GPDMA_Init();
    
    /* ===== CANAL DMA 0 - EJE X (HORIZONTAL) ===== */
    // Lee ADC Canal 0 (ADDR0) → P0.23 VRx → Guarda en buffer_x
    
    // Configurar LLI para buffer circular (ESTÁTICA - ya declarada globalmente)
    lli_x.srcAddr = (uint32_t)&(LPC_ADC->ADDR0);  // Canal 0 = VRx (P0.23)
    lli_x.dstAddr = (uint32_t)buffer_x;
    lli_x.nextLLI = (uint32_t)&lli_x;  // Circular
    lli_x.control = GPDMA_DMACCxControl_TransferSize(BUFFER_SIZE)
                  | GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1)
                  | GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1)
                  | GPDMA_DMACCxControl_SWidth(GPDMA_HALFWORD)
                  | GPDMA_DMACCxControl_DWidth(GPDMA_HALFWORD)
                  | GPDMA_DMACCxControl_DI   // Incrementar destino
                  | GPDMA_DMACCxControl_I;   // Terminal count interrupt
    
    // Configurar canal DMA 0
    dma_cfg.channelNum = 0;
    dma_cfg.transferSize = BUFFER_SIZE;
    dma_cfg.transferWidth = GPDMA_HALFWORD;
    dma_cfg.srcMemAddr = (uint32_t)&(LPC_ADC->ADDR0);  // ADDR0 = Canal 0 (VRx)
    dma_cfg.dstMemAddr = (uint32_t)buffer_x;
    dma_cfg.transferType = GPDMA_P2M;
    dma_cfg.srcConn = GPDMA_ADC;
    dma_cfg.dstConn = 0;
    dma_cfg.linkedList = (uint32_t)&lli_x;
    
    GPDMA_Setup(&dma_cfg);
    
    /* ===== CANAL DMA 1 - EJE Y (VERTICAL) ===== */
    // Lee ADC Canal 1 (ADDR1) → P0.24 VRy → Guarda en buffer_y
    
    // Configurar LLI para buffer circular
    lli_y.srcAddr = (uint32_t)&(LPC_ADC->ADDR1);  // Canal 1 = VRy (P0.24)
    lli_y.dstAddr = (uint32_t)buffer_y;
    lli_y.nextLLI = (uint32_t)&lli_y;  // Circular
    lli_y.control = GPDMA_DMACCxControl_TransferSize(BUFFER_SIZE)
                  | GPDMA_DMACCxControl_SBSize(GPDMA_BSIZE_1)
                  | GPDMA_DMACCxControl_DBSize(GPDMA_BSIZE_1)
                  | GPDMA_DMACCxControl_SWidth(GPDMA_HALFWORD)
                  | GPDMA_DMACCxControl_DWidth(GPDMA_HALFWORD)
                  | GPDMA_DMACCxControl_DI   // Incrementar destino
                  | GPDMA_DMACCxControl_I;   // Terminal count interrupt
    
    // Configurar canal DMA 1
    dma_cfg.channelNum = 1;
    dma_cfg.srcMemAddr = (uint32_t)&(LPC_ADC->ADDR1);  // ADDR1 = Canal 1 (VRy)
    dma_cfg.dstMemAddr = (uint32_t)buffer_y;
    dma_cfg.linkedList = (uint32_t)&lli_y;
    
    GPDMA_Setup(&dma_cfg);
    
    // Habilitar interrupciones DMA
    NVIC_EnableIRQ(DMA_IRQn);
    NVIC_SetPriority(DMA_IRQn, 2);
    
    // Activar canales DMA
    GPDMA_ChannelCmd(0, ENABLE);
    GPDMA_ChannelCmd(1, ENABLE);
    
    // CRÍTICO: Activar modo BURST para que el DMA funcione
    // En modo burst, el ADC convierte automáticamente todos los canales habilitados
    // y el DMA transfiere los datos a los buffers sin intervención del CPU
    ADC_BurstCmd(ENABLE);
    ADC_StartCmd(ADC_START_CONTINUOUS);
}

/**
 * @brief Calcula el promedio de un buffer de 10 muestras
 * @param buffer Puntero al buffer con las muestras
 * @return Promedio (0-4095)
 * @note ADC_ChannelGetData() ya devuelve el valor de 12 bits directamente
 */
uint16_t calcular_promedio(volatile uint16_t* buffer) {
    uint32_t suma = 0;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        // El driver ADC ya hace el shift (>> 4) y el mask (& 0xFFF)
        suma += buffer[i];
    }
    
    return (uint16_t)(suma / BUFFER_SIZE);
}

/**
 * @brief Procesa los datos del joystick y actualiza LEDs
 * Calcula promedios de X e Y y determina dirección
 */
void procesar_joystick(void) {
    if (!datos_listos) return;
    
    // Calcular promedios
    promedio_x = calcular_promedio(buffer_x);
    promedio_y = calcular_promedio(buffer_y);
    
    // Actualizar LEDs según posición
    actualizar_leds();
    
    datos_listos = 0;  // Limpiar flag
}

/**
 * @brief Actualiza los LEDs según los promedios de X e Y
 * 
 * MAPEO JOYSTICK KY-023:
 * - VRx (potenciómetro físico horizontal) → P0.23 → Canal 0 → promedio_x → HORIZONTAL (Izq/Der)
 * - VRy (potenciómetro físico vertical) → P0.24 → Canal 1 → promedio_y → VERTICAL (Arr/Abajo)
 * 
 * Lógica:
 * - NEUTRO: Todos apagados (dentro de ±deadzone del centro)
 * - IZQUIERDA: X < (centro_x - deadzone) → P0.0 encendido
 * - DERECHA: X > (centro_x + deadzone) → P0.1 encendido
 * - ARRIBA: Y > (centro_y + deadzone) → P0.2 encendido
 * - ABAJO: Y < (centro_y - deadzone) → P0.3 encendido
 */
void actualizar_leds(void) {
    // Apagar todos los LEDs primero (BAJO = apagado)
    GPIO_ClearPins(LED_PORT, (1 << LED_IZQUIERDA_PIN) | 
                             (1 << LED_DERECHA_PIN) | 
                             (1 << LED_ARRIBA_PIN) | 
                             (1 << LED_ABAJO_PIN));
    
    // HORIZONTAL: Controlado por VRx (Canal 0 en P0.23)
    if (promedio_x < (centro_x - deadzone)) {
        // Joystick a la IZQUIERDA
        GPIO_SetPins(LED_PORT, (1 << LED_IZQUIERDA_PIN));
    }
    else if (promedio_x > (centro_x + deadzone)) {
        // Joystick a la DERECHA
        GPIO_SetPins(LED_PORT, (1 << LED_DERECHA_PIN));
    }
    
    // VERTICAL: Controlado por VRy (Canal 1 en P0.24)
    if (promedio_y > (centro_y + deadzone)) {
        // Joystick ARRIBA
        GPIO_SetPins(LED_PORT, (1 << LED_ARRIBA_PIN));
    }
    else if (promedio_y < (centro_y - deadzone)) {
        // Joystick ABAJO
        GPIO_SetPins(LED_PORT, (1 << LED_ABAJO_PIN));
    }
    
    // Si está en NEUTRO (ambos dentro de deadzone), todos quedan apagados
}

/* ===================== FUNCIÓN DE PRUEBA SIN DMA ========================= */

/**
 * @brief Lee ADC directamente (sin DMA) para pruebas
 * 
 * IMPORTANTE: El ADC con START_NOW solo convierte UN canal por trigger
 * Solución: Habilitar solo el canal deseado, convertir, deshabilitar
 * 
 * @param canal Número de canal ADC (0 o 1)
 * @return Valor ADC de 12 bits (0-4095) o 2048 si timeout
 * 
 * TIMEOUT: Protección contra deadlock si el ADC falla
 * - NO es para dar tiempo al usuario
 * - Previene cuelgues si canal mal configurado, clock deshabilitado, etc.
 * - Con ADC a 100kHz: 1 conversión = 10µs, timeout = 1000 iter (~100µs)
 */
uint16_t leer_adc_directo(uint8_t canal) {
    uint32_t timeout = 1000;  // Suficiente para 100µs (10x margen de seguridad)
    uint16_t valor;
    
    // CRÍTICO: Deshabilitar TODOS los canales primero
    // Evita que START_NOW convierta el canal de menor número por defecto
    ADC_ChannelCmd(ADC_CHANNEL_X, DISABLE);
    ADC_ChannelCmd(ADC_CHANNEL_Y, DISABLE);
    
    // Habilitar SOLO el canal que queremos leer
    ADC_ChannelCmd(canal, ENABLE);
    
    // Iniciar conversión (solo este canal se convertirá)
    ADC_StartCmd(ADC_START_NOW);
    
    // Esperar a que termine la conversión del canal específico
    // Timeout previene deadlock si hay error de configuración
    while (!ADC_ChannelGetStatus(canal, ADC_DATA_DONE) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        // Error: ADC no respondió (posible mal configuración de hardware)
        return 2048;  // Valor neutro (centro del rango 0-4095)
    }
    
    // Leer valor del canal (12 bits: 0-4095)
    valor = ADC_ChannelGetData(canal);
    
    // Re-habilitar ambos canales para próximas lecturas
    ADC_ChannelCmd(ADC_CHANNEL_X, ENABLE);
    ADC_ChannelCmd(ADC_CHANNEL_Y, ENABLE);
    
    return valor;
}

/**
 * @brief Calibra el joystick leyendo posición neutral
 * Toma 10 muestras y calcula el punto de reposo
 */
void calibrar_joystick(void) {
    uint32_t suma_x = 0, suma_y = 0;
    const uint8_t muestras = 10;
    
    // INICIO - Parpadear TODOS 3 veces
    for (int i = 0; i < 3; i++) {
        GPIO_SetPins(LED_PORT, 0x0F);
        for (volatile uint32_t j = 0; j < 500000; j++);
        GPIO_ClearPins(LED_PORT, 0x0F);
        for (volatile uint32_t j = 0; j < 500000; j++);
    }
    
    // Pausa
    for (volatile uint32_t j = 0; j < 2000000; j++);
    
    // Leer 10 muestras para calibrar el centro
    for (int i = 0; i < muestras; i++) {
        suma_x += leer_adc_directo(ADC_CHANNEL_X);
        suma_y += leer_adc_directo(ADC_CHANNEL_Y);
        for (volatile uint32_t j = 0; j < 10000; j++);
    }
    
    centro_x = suma_x / muestras;
    centro_y = suma_y / muestras;
    
    // Ajustar deadzone según valores reales
    // Si los valores son muy altos (>2000), usar deadzone grande
    if (centro_x > 2000 || centro_y > 2000) {
        deadzone = 500;  // Deadzone grande para valores altos
    }
    else if (centro_x > 1000 || centro_y > 1000) {
        deadzone = 250;  // Deadzone media
    }
    else {
        deadzone = 50;   // Deadzone pequeña para valores bajos
    }
    
    // Confirmación final
    for (volatile uint32_t j = 0; j < 1000000; j++);
    GPIO_SetPins(LED_PORT, 0x0F);
    for (volatile uint32_t j = 0; j < 1000000; j++);
    GPIO_ClearPins(LED_PORT, 0x0F);
}

/**
 * @brief Prueba simple sin DMA - MODO CON PROMEDIO DE 10 MUESTRAS
 * 
 * TIMING: 
 * - ADC a 100kHz significa 1 conversión cada 10µs
 * - 10 muestras × 10µs = 100µs por canal
 * - 2 canales × 100µs = 200µs total para completar ciclo
 * - Tasa de actualización: ~5000 lecturas/segundo
 * 
 * IMPORTANTE - LIMITACIÓN DEL HARDWARE:
 * - ADC con START_NOW solo convierte EL PRIMER CANAL HABILITADO (menor número)
 * - Si canal 0 y canal 1 están habilitados, START_NOW solo convierte canal 0
 * - Solución: Leer canales SECUENCIALMENTE con leer_adc_directo()
 *   que habilita/deshabilita canales individualmente
 * 
 * LÓGICA:
 * - Si promedio_x difiere de centro_x → movimiento HORIZONTAL (Izq/Der)
 * - Si promedio_y difiere de centro_y → movimiento VERTICAL (Arr/Abajo)
 * - Movimientos pueden ser simultáneos (diagonales)
 */
void test_sin_dma(void) {
    uint32_t suma_x = 0, suma_y = 0;
    
    // Tomar 10 muestras de CADA canal (secuencialmente)
    // NOTA: leer_adc_directo() ya maneja la habilitación individual de canales
    for (int i = 0; i < 10; i++) {
        // ===== LEER CANAL Y (0) =====
        suma_y += leer_adc_directo(ADC_CHANNEL_Y);
        
        // ===== LEER CANAL X (1) =====
        suma_x += leer_adc_directo(ADC_CHANNEL_X);
        
        // Pequeño delay entre ciclos de muestreo (~10µs)
        for (volatile int j = 0; j < 100; j++);
    }
    
    // Calcular promedios (filtro digital simple)
    promedio_y = suma_y / 10;
    promedio_x = suma_x / 10;
    
    // Apagar todos los LEDs primero
    GPIO_ClearPins(LED_PORT, 0x0F);
    
    // ===== DECISIÓN DE MOVIMIENTO =====
    // Se evalúan AMBOS ejes independientemente
    // Si el joystick se mueve en diagonal, pueden encenderse 2 LEDs
    
    // HORIZONTAL: Controlado por VRx (Canal 0 en P0.23)
    if (promedio_x < (centro_x - deadzone)) {
        GPIO_SetPins(LED_PORT, (1 << LED_IZQUIERDA_PIN));  // P0.0 IZQUIERDA
    }
    else if (promedio_x > (centro_x + deadzone)) {
        GPIO_SetPins(LED_PORT, (1 << LED_DERECHA_PIN));  // P0.1 DERECHA
    }
    
    // VERTICAL: Controlado por VRy (Canal 1 en P0.24)
    if (promedio_y < (centro_y - deadzone)) {
        GPIO_SetPins(LED_PORT, (1 << LED_ABAJO_PIN));  // P0.3 ABAJO
    }
    else if (promedio_y > (centro_y + deadzone)) {
        GPIO_SetPins(LED_PORT, (1 << LED_ARRIBA_PIN));  // P0.2 ARRIBA
    }
    
    // Si ambos promedios están dentro de ±deadzone = NEUTRO (todos apagados)
}

/* ============================== MAIN ====================================== */

int main(void) {
    // Inicializar sistema
    SystemInit();
    
    // Configurar hardware
    config_gpio_leds();
    config_adc();
    
    // CALIBRACIÓN AUTOMÁTICA
    // IMPORTANTE: Mantén el joystick en NEUTRO (centrado) durante los primeros 3 parpadeos
    calibrar_joystick();
    
    // === MODO DE PRUEBA: Descomentar UNA de las siguientes opciones ===
    
    // OPCIÓN 1: Prueba simple SIN DMA (para verificar hardware)
    while (1) {
        test_sin_dma();
        for (volatile uint32_t i = 0; i < 10000; i++);  // Delay para no saturar
    }
    
    /* OPCIÓN 2: Con DMA (comentar el while anterior y descomentar esto)
    config_dma();
    
    while (1) {
        procesar_joystick();
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
    */
    
    return 0;
}
