# Sistema de Joystick con ADC + DMA - Flujos de Trabajo

**Archivo:** `src/adc.c`  
**Hardware:** LPC1769 + Joystick KY-023  
**Fecha:** Noviembre 2025

---

## 📋 Índice

1. [Descripción General](#-descripción-general)
2. [Mapeo de Hardware](#-mapeo-de-hardware)
3. [Modo 1: SIN DMA (Polling)](#-modo-1-sin-dma-polling)
4. [Modo 2: CON DMA (Interrupciones)](#-modo-2-con-dma-interrupciones)
5. [Comparación de Modos](#-comparación-de-modos)
6. [Diagramas de Flujo](#-diagramas-de-flujo)
7. [Timing y Performance](#-timing-y-performance)

---

## 🎯 Descripción General

Este sistema lee un joystick analógico KY-023 (2 ejes: VRx y VRy) usando el ADC del LPC1769 y controla 4 LEDs direccionales. Implementa dos modos de operación:

- **Modo SIN DMA:** Lectura manual por software (polling) - Para pruebas y debug
- **Modo CON DMA:** Lectura automática por hardware - Para producción

Ambos modos promedian 10 muestras por eje para reducir ruido y calibran automáticamente el punto neutro del joystick al inicio.

---

## 🔌 Mapeo de Hardware

### Conexiones del Joystick

| Cable Joystick | Pin LPC1769 | Canal ADC | Registro | Canal DMA | Buffer      | Controla     | LEDs       |
|----------------|-------------|-----------|----------|-----------|-------------|--------------|------------|
| **VRx** (horizontal) | P0.23 | Canal 0 | ADDR0 | DMA 0 | `buffer_x` | Izq/Der | P0.0, P0.1 |
| **VRy** (vertical)   | P0.24 | Canal 1 | ADDR1 | DMA 1 | `buffer_y` | Arr/Abajo | P0.2, P0.3 |

### LEDs de Retroalimentación

| Pin   | LED       | Estado        | Condición                          |
|-------|-----------|---------------|------------------------------------|
| P0.0  | Izquierda | ON cuando VRx bajo  | `promedio_x < (centro_x - deadzone)` |
| P0.1  | Derecha   | ON cuando VRx alto  | `promedio_x > (centro_x + deadzone)` |
| P0.2  | Arriba    | ON cuando VRy alto  | `promedio_y > (centro_y + deadzone)` |
| P0.3  | Abajo     | ON cuando VRy bajo  | `promedio_y < (centro_y - deadzone)` |

### Configuración del ADC

- **Frecuencia:** 100 kHz
- **Resolución:** 12 bits (0-4095)
- **Periodo de conversión:** 10 µs por canal
- **Canales habilitados:** 0 (VRx) y 1 (VRy)

---

## 🔄 Modo 1: SIN DMA (Polling)

### Descripción

Modo de operación simple donde el CPU lee el ADC manualmente canal por canal. **Actualmente activo** en el `main()`.

### Flujo de Trabajo Completo

```
┌─────────────────────────────────────────────────────────────────┐
│                        INICIALIZACIÓN                            │
└─────────────────────────────────────────────────────────────────┘
    1. SystemInit()
       └─> Inicializa relojes del sistema
    
    2. config_gpio_leds()
       └─> Configura P0.0-P0.3 como salidas GPIO
       └─> Apaga todos los LEDs inicialmente
    
    3. config_adc()
       └─> Configura P0.23 como ADC0.0 (VRx)
       └─> Configura P0.24 como ADC0.1 (VRy)
       └─> Inicializa ADC a 100 kHz
       └─> Habilita canales 0 y 1
       └─> NO activa burst mode (se usa START_NOW manual)
    
    4. calibrar_joystick()
       └─> [Ver sección CALIBRACIÓN más abajo]

┌─────────────────────────────────────────────────────────────────┐
│                  CALIBRACIÓN AUTOMÁTICA                          │
└─────────────────────────────────────────────────────────────────┘
    
    INDICACIÓN VISUAL:
    ├─> Parpadear 4 LEDs × 3 veces
    │   └─> Usuario debe mantener joystick CENTRADO
    │
    ├─> Pausa de 2 segundos
    │
    LECTURA DE MUESTRAS:
    ├─> FOR i = 0 to 9:  (10 muestras)
    │   │
    │   ├─> suma_x += leer_adc_directo(CANAL_0)  // VRx
    │   │   └─> Deshabilita todos los canales
    │   │   └─> Habilita SOLO canal 0
    │   │   └─> ADC_START_NOW
    │   │   └─> Espera DONE con timeout (1000 iter)
    │   │   └─> Lee valor de ADDR0
    │   │   └─> Re-habilita ambos canales
    │   │
    │   ├─> suma_y += leer_adc_directo(CANAL_1)  // VRy
    │   │   └─> (mismo proceso para canal 1)
    │   │
    │   └─> Delay ~1ms entre muestras
    │
    CÁLCULO DE PARÁMETROS:
    ├─> centro_x = suma_x / 10
    ├─> centro_y = suma_y / 10
    │
    ├─> Ajuste adaptativo de deadzone:
    │   ├─> Si centro > 2000 → deadzone = 500
    │   ├─> Si centro > 1000 → deadzone = 250
    │   └─> Sino            → deadzone = 50
    │
    CONFIRMACIÓN:
    └─> Parpadear 4 LEDs × 1 vez (calibración completa)

┌─────────────────────────────────────────────────────────────────┐
│                    LOOP INFINITO PRINCIPAL                       │
└─────────────────────────────────────────────────────────────────┘
    
    WHILE (1):
        │
        ├─> test_sin_dma()
        │   │
        │   ┌────────────────────────────────────────────┐
        │   │  FASE 1: ADQUISICIÓN DE DATOS              │
        │   └────────────────────────────────────────────┘
        │   │
        │   ├─> Inicializar: suma_x = 0, suma_y = 0
        │   │
        │   ├─> FOR i = 0 to 9:  (10 muestras por canal)
        │   │   │
        │   │   ├─> suma_y += leer_adc_directo(CANAL_1)
        │   │   │   └─> Tiempo: ~10µs
        │   │   │
        │   │   ├─> suma_x += leer_adc_directo(CANAL_0)
        │   │   │   └─> Tiempo: ~10µs
        │   │   │
        │   │   └─> Delay ~1µs (100 iteraciones)
        │   │
        │   │   Total por iteración: ~21µs
        │   │   Total 10 muestras: ~210µs
        │   │
        │   ┌────────────────────────────────────────────┐
        │   │  FASE 2: PROCESAMIENTO                     │
        │   └────────────────────────────────────────────┘
        │   │
        │   ├─> promedio_x = suma_x / 10
        │   ├─> promedio_y = suma_y / 10
        │   │
        │   ┌────────────────────────────────────────────┐
        │   │  FASE 3: CONTROL DE LEDS                   │
        │   └────────────────────────────────────────────┘
        │   │
        │   ├─> Apagar todos los LEDs
        │   │
        │   ├─> DECISIÓN HORIZONTAL (VRx):
        │   │   ├─> IF promedio_x < (centro_x - deadzone):
        │   │   │   └─> Encender LED_IZQUIERDA (P0.0)
        │   │   │
        │   │   └─> ELSE IF promedio_x > (centro_x + deadzone):
        │   │       └─> Encender LED_DERECHA (P0.1)
        │   │
        │   ├─> DECISIÓN VERTICAL (VRy):
        │   │   ├─> IF promedio_y < (centro_y - deadzone):
        │   │   │   └─> Encender LED_ABAJO (P0.3)
        │   │   │
        │   │   └─> ELSE IF promedio_y > (centro_y + deadzone):
        │   │       └─> Encender LED_ARRIBA (P0.2)
        │   │
        │   └─> NOTA: Movimientos diagonales encienden 2 LEDs
        │
        ├─> Delay 10000 iteraciones (~1ms)
        │   └─> Evita saturar el CPU
        │
        └─> REPETIR ↺
```

### Características del Modo SIN DMA

**✅ Ventajas:**
- Simple de entender y debuggear
- No requiere configuración de DMA
- Control total del timing
- Útil para verificar hardware

**❌ Desventajas:**
- Alto uso de CPU (~80%)
- Timing variable (depende del código)
- Baja tasa de actualización (~100 Hz)
- Bloquea el CPU durante lecturas

### Función Crítica: `leer_adc_directo()`

```c
uint16_t leer_adc_directo(uint8_t canal) {
    // PROBLEMA: ADC con START_NOW solo convierte el canal de MENOR número
    // SOLUCIÓN: Habilitar/deshabilitar canales individualmente
    
    1. Deshabilitar TODOS los canales
    2. Habilitar SOLO el canal deseado
    3. ADC_START_NOW (solo este canal se convierte)
    4. Esperar DONE con timeout (protección contra deadlock)
    5. Leer valor del registro ADDR[canal]
    6. Re-habilitar ambos canales para próxima lectura
    
    Timeout = 1000 iteraciones (~100µs)
    └─> NO es para el usuario, es protección contra fallos de hardware
}
```

---

## ⚡ Modo 2: CON DMA (Interrupciones)

### Descripción

Modo avanzado donde el DMA transfiere datos del ADC a memoria automáticamente, sin intervención del CPU. **Actualmente comentado** en el `main()`.

### Flujo de Trabajo Completo

```
┌─────────────────────────────────────────────────────────────────┐
│                        INICIALIZACIÓN                            │
└─────────────────────────────────────────────────────────────────┘
    
    (Igual que modo SIN DMA hasta calibración)
    
    1. SystemInit()
    2. config_gpio_leds()
    3. config_adc()
    4. calibrar_joystick()
    
    5. config_dma()  ← NUEVO
       │
       ┌────────────────────────────────────────────┐
       │  CONFIGURACIÓN CANAL DMA 0 (EJE X)         │
       └────────────────────────────────────────────┘
       │
       ├─> GPDMA_Init()
       │   └─> Inicializa controlador DMA
       │
       ├─> Configurar LLI (Linked List Item) para buffer_x:
       │   ├─> srcAddr  = &LPC_ADC->ADDR0  (Canal 0 - VRx)
       │   ├─> dstAddr  = buffer_x[0]
       │   ├─> nextLLI  = &lli_x  (CIRCULAR)
       │   ├─> size     = 10 muestras (HALFWORD)
       │   └─> control  = Terminal Count Interrupt ENABLE
       │
       ├─> Configurar Canal DMA 0:
       │   ├─> Periférico → Memoria (P2M)
       │   ├─> Fuente: ADDR0
       │   ├─> Destino: buffer_x
       │   ├─> Trigger: ADC (señal de DMA request)
       │   └─> Linked List: lli_x (circular)
       │
       ┌────────────────────────────────────────────┐
       │  CONFIGURACIÓN CANAL DMA 1 (EJE Y)         │
       └────────────────────────────────────────────┘
       │
       ├─> Configurar LLI para buffer_y:
       │   ├─> srcAddr  = &LPC_ADC->ADDR1  (Canal 1 - VRy)
       │   ├─> dstAddr  = buffer_y[0]
       │   ├─> nextLLI  = &lli_y  (CIRCULAR)
       │   ├─> size     = 10 muestras
       │   └─> control  = Terminal Count Interrupt ENABLE
       │
       ├─> Configurar Canal DMA 1:
       │   └─> (mismo que DMA 0, pero con ADDR1)
       │
       ┌────────────────────────────────────────────┐
       │  ACTIVACIÓN DEL SISTEMA                    │
       └────────────────────────────────────────────┘
       │
       ├─> NVIC_EnableIRQ(DMA_IRQn)
       ├─> NVIC_SetPriority(DMA_IRQn, 2)
       │
       ├─> GPDMA_ChannelCmd(0, ENABLE)  // DMA canal 0
       ├─> GPDMA_ChannelCmd(1, ENABLE)  // DMA canal 1
       │
       ├─> ADC_BurstCmd(ENABLE)         // ¡MODO BURST!
       └─> ADC_StartCmd(CONTINUOUS)     // ¡INICIA CONVERSIÓN!
           │
           └─> A partir de aquí, el ADC convierte automáticamente
               y el DMA transfiere datos sin intervención del CPU

┌─────────────────────────────────────────────────────────────────┐
│              HARDWARE EN SEGUNDO PLANO (Automático)              │
└─────────────────────────────────────────────────────────────────┘

    El ADC en modo BURST convierte continuamente ambos canales:
    
    Tiempo 0µs:
        ADC → Convierte Canal 0 (VRx)
        ADDR0 = valor_nuevo
        ADC → Genera señal DMA request
        DMA → Automático: ADDR0 → buffer_x[0]
    
    Tiempo 10µs:
        ADC → Convierte Canal 1 (VRy)
        ADDR1 = valor_nuevo
        ADC → Genera señal DMA request
        DMA → Automático: ADDR1 → buffer_y[0]
    
    Tiempo 20µs:
        ADC → Convierte Canal 0
        DMA → buffer_x[1] = ADDR0
    
    Tiempo 30µs:
        ADC → Convierte Canal 1
        DMA → buffer_y[1] = ADDR1
    
    ... (continúa automáticamente)
    
    Tiempo 180µs:
        ADC → Convierte Canal 0 (muestra #10)
        DMA → buffer_x[9] = ADDR0
        DMA → ¡Completó 10 muestras!
        
        ┌────────────────────────────────────┐
        │  INTERRUPCIÓN: DMA_IRQHandler()    │
        └────────────────────────────────────┘
        │
        ├─> GPDMA_IntGetStatus(GPDMA_RAW_INTTC, 0) = TRUE
        ├─> GPDMA_ClearIntPending(GPDMA_CLR_INTTC, 0)
        └─> canal_x_listo = 1
    
    Tiempo 190µs:
        ADC → Convierte Canal 1 (muestra #10)
        DMA → buffer_y[9] = ADDR1
        DMA → ¡Completó 10 muestras!
        
        ┌────────────────────────────────────┐
        │  INTERRUPCIÓN: DMA_IRQHandler()    │
        └────────────────────────────────────┘
        │
        ├─> GPDMA_IntGetStatus(GPDMA_RAW_INTTC, 1) = TRUE
        ├─> GPDMA_ClearIntPending(GPDMA_CLR_INTTC, 1)
        ├─> canal_y_listo = 1
        │
        ├─> SINCRONIZACIÓN:
        │   IF (canal_x_listo && canal_y_listo):
        │       datos_listos = 1  ← ¡FLAG GLOBAL SETEADO!
        │       canal_x_listo = 0
        │       canal_y_listo = 0
        │
        └─> RETURN (sale de interrupción)
    
    Tiempo 200µs:
        DMA → Reinicio automático (LLI circular)
        DMA → buffer_x[0] se sobrescribe con nueva muestra
        DMA → buffer_y[0] se sobrescribe con nueva muestra
        
        ↺ Ciclo se REPITE infinitamente (cada 200µs)

┌─────────────────────────────────────────────────────────────────┐
│                    LOOP INFINITO PRINCIPAL                       │
└─────────────────────────────────────────────────────────────────┘
    
    WHILE (1):
        │
        ├─> procesar_joystick()
        │   │
        │   ├─> IF (!datos_listos):
        │   │   └─> return  // Esperar próxima interrupción
        │   │
        │   │   (CPU está libre, puede hacer otras tareas)
        │   │
        │   ├─> Cuando datos_listos = 1:
        │   │   │
        │   │   ┌────────────────────────────────────┐
        │   │   │  CÁLCULO DE PROMEDIOS              │
        │   │   └────────────────────────────────────┘
        │   │   │
        │   │   ├─> promedio_x = calcular_promedio(buffer_x)
        │   │   │   └─> Suma buffer_x[0..9] / 10
        │   │   │
        │   │   ├─> promedio_y = calcular_promedio(buffer_y)
        │   │   │   └─> Suma buffer_y[0..9] / 10
        │   │   │
        │   │   ┌────────────────────────────────────┐
        │   │   │  CONTROL DE LEDS                   │
        │   │   └────────────────────────────────────┘
        │   │   │
        │   │   ├─> actualizar_leds()
        │   │   │   └─> (Misma lógica que modo SIN DMA)
        │   │   │
        │   │   └─> datos_listos = 0  // Limpiar flag
        │   │
        │   └─> RETURN
        │
        ├─> Delay 100000 iteraciones (~10ms)
        │   └─> CPU puede hacer otras tareas aquí
        │
        └─> REPETIR ↺
```

### DMA IRQ Handler - Sincronización

```c
void DMA_IRQHandler(void) {
    // PROBLEMA: Canales DMA 0 y 1 completan en momentos ligeramente diferentes
    // SOLUCIÓN: Sincronizar con flags, solo procesar cuando AMBOS terminaron
    
    IF (Canal DMA 0 completó):
        ├─> Limpiar interrupción
        └─> canal_x_listo = 1
    
    IF (Canal DMA 1 completó):
        ├─> Limpiar interrupción
        └─> canal_y_listo = 1
    
    IF (canal_x_listo && canal_y_listo):  // SINCRONIZACIÓN
        ├─> datos_listos = 1        // Señal para procesar
        ├─> canal_x_listo = 0       // Reset para próxima vez
        └─> canal_y_listo = 0
    
    // Limpiar errores si existen
}
```

### Características del Modo CON DMA

**✅ Ventajas:**
- Bajo uso de CPU (~5%)
- Alta tasa de actualización (5000 Hz)
- Timing preciso y constante
- CPU libre para otras tareas
- Ideal para producción

**❌ Desventajas:**
- Configuración más compleja
- Mayor uso de memoria (2 buffers)
- Más difícil de debuggear
- Requiere entender DMA e interrupciones

### Buffer Circular con LLI

```
┌──────────────────────────────────┐
│  buffer_x[10] (Canal 0 - VRx)    │
└──────────────────────────────────┘
     ↓
[0]  ← DMA escribe aquí
[1]  ← luego aquí
[2]
...
[9]  ← última muestra
     ↓
     Interrupción: Terminal Count
     ↓
     LLI.nextLLI = &lli_x (apunta a sí mismo)
     ↓
[0]  ← DMA reinicia automáticamente ↺
```

---

## 📊 Comparación de Modos

| Característica | Modo SIN DMA | Modo CON DMA |
|----------------|--------------|--------------|
| **CPU Usage** | ~80% | ~5% |
| **Tasa de actualización** | ~100 Hz (cada 10ms) | ~5000 Hz (cada 200µs) |
| **Latencia** | Alta (espera activa) | Baja (interrupción) |
| **Jitter** | Alto (timing variable) | Bajo (timing fijo) |
| **Precisión** | Media | Alta |
| **Complejidad código** | Baja | Media-Alta |
| **Uso de memoria** | Mínimo | 40 bytes (2×10 samples) |
| **Debugging** | Fácil | Moderado |
| **Multitarea** | Difícil (CPU ocupado) | Fácil (CPU libre) |
| **Consumo energía** | Alto (CPU siempre activo) | Bajo (CPU puede dormir) |
| **Recomendado para** | Debug/pruebas/prototipo | Producción/juego Snake |

### ¿Cuándo usar cada modo?

#### Usar **SIN DMA** cuando:
- Estás verificando que el hardware funciona
- Debuggeando problemas de conexión
- Aprendiendo cómo funciona el ADC
- No necesitas alta frecuencia de muestreo
- El código es simple y no tienes otras tareas

#### Usar **CON DMA** cuando:
- El hardware ya está verificado
- Necesitas alta tasa de actualización
- El CPU debe hacer otras tareas (renderizar pantalla, calcular física del juego)
- Quieres minimizar consumo de energía
- Es código de producción

---

## 📈 Diagramas de Flujo

### Diagrama de Estados - Modo DMA

```
        ┌──────────────┐
        │   STARTUP    │
        │ (Power-On)   │
        └──────┬───────┘
               │
               ▼
        ┌──────────────┐
        │ CALIBRACIÓN  │
        │ (Joystick    │
        │  centrado)   │
        └──────┬───────┘
               │
               ▼
        ┌──────────────┐
        │ CONFIG_DMA   │
        │ (Inicia      │
        │  burst mode) │
        └──────┬───────┘
               │
               ▼
        ┌──────────────────────────┐
        │  ESTADO: MUESTREANDO     │◄───────┐
        │  ADC + DMA trabajando    │        │
        │  en segundo plano        │        │
        │  (CPU libre)             │        │
        └──────┬───────────────────┘        │
               │                            │
               │ Cada 200µs                 │
               ▼                            │
        ┌──────────────────┐               │
        │ DMA_IRQHandler() │               │
        │ (Interrupción)   │               │
        └──────┬───────────┘               │
               │                            │
               ▼                            │
        ┌──────────────────┐               │
        │ datos_listos=1?  │───NO──────────┤
        └──────┬───────────┘               │
               │ SÍ                         │
               ▼                            │
        ┌──────────────────┐               │
        │ Calcular         │               │
        │ promedios        │               │
        └──────┬───────────┘               │
               │                            │
               ▼                            │
        ┌──────────────────┐               │
        │ Actualizar LEDs  │               │
        └──────┬───────────┘               │
               │                            │
               ▼                            │
        ┌──────────────────┐               │
        │ datos_listos=0   │               │
        └──────┬───────────┘               │
               │                            │
               └────────────────────────────┘
```

### Diagrama Temporal - ADC + DMA

```
Tiempo (µs)    ADC               DMA              Buffer          Estado
──────────────────────────────────────────────────────────────────────────
0              Ch0 convierte     →                buffer_x[0]     
10             Ch1 convierte     →                buffer_y[0]     
20             Ch0 convierte     →                buffer_x[1]     
30             Ch1 convierte     →                buffer_y[1]     
40             Ch0 convierte     →                buffer_x[2]     
50             Ch1 convierte     →                buffer_y[2]     
60             Ch0 convierte     →                buffer_x[3]     
70             Ch1 convierte     →                buffer_y[3]     
80             Ch0 convierte     →                buffer_x[4]     
90             Ch1 convierte     →                buffer_y[4]     
100            Ch0 convierte     →                buffer_x[5]     
110            Ch1 convierte     →                buffer_y[5]     
120            Ch0 convierte     →                buffer_x[6]     
130            Ch1 convierte     →                buffer_y[6]     
140            Ch0 convierte     →                buffer_x[7]     
150            Ch1 convierte     →                buffer_y[7]     
160            Ch0 convierte     →                buffer_x[8]     
170            Ch1 convierte     →                buffer_y[8]     
180            Ch0 convierte     →                buffer_x[9]     DMA0 INT ↑
190            Ch1 convierte     →                buffer_y[9]     DMA1 INT ↑
200            ---               Reinicio (LLI)   buffer_x[0]     datos_listos=1
210            Ch0 convierte     →                buffer_x[0]     ↺ REPITE
220            Ch1 convierte     →                buffer_y[0]
```

### Secuencia de Calibración

```
Tiempo         Acción                              LED Estado
───────────────────────────────────────────────────────────────
0.0s           Inicio                              ────
0.1s           Parpadeo #1 ON                      ████
0.6s           Parpadeo #1 OFF                     ────
1.1s           Parpadeo #2 ON                      ████
1.6s           Parpadeo #2 OFF                     ────
2.1s           Parpadeo #3 ON                      ████
2.6s           Parpadeo #3 OFF                     ────
               ┌──────────────────────────────┐
2.6s           │ MANTENER JOYSTICK CENTRADO   │
               └──────────────────────────────┘
4.6s           Leyendo 10 muestras...          ─  ─
5.0s           Cálculo de centro_x/y           ────
5.5s           Confirmación ON                 ████
6.0s           Confirmación OFF                ────
               ┌──────────────────────────────┐
6.0s           │ CALIBRACIÓN COMPLETA         │
               │ Listo para usar              │
               └──────────────────────────────┘
```

---

## ⏱️ Timing y Performance

### Tiempos de Conversión ADC

| Parámetro | Valor | Cálculo |
|-----------|-------|---------|
| Frecuencia ADC | 100 kHz | Configurado |
| Periodo por conversión | 10 µs | 1 / 100kHz |
| Canales activos | 2 | Canal 0 y 1 |
| Tiempo por ciclo completo | 20 µs | 2 × 10µs |
| Muestras por buffer | 10 | `BUFFER_SIZE` |
| **Tiempo para llenar buffer** | **200 µs** | 10 × 20µs |
| **Frecuencia de interrupción DMA** | **5000 Hz** | 1 / 200µs |
| **Ancho de banda por canal** | **50 kHz** | Nyquist |

### Performance Modo SIN DMA

```
Ciclo de lectura completo:
├─> 10 muestras × 2 canales = 20 lecturas
├─> Cada lectura:
│   ├─> Deshabilitar canales: ~5µs
│   ├─> Habilitar canal: ~5µs
│   ├─> START_NOW + espera DONE: ~10µs
│   ├─> Leer valor: ~5µs
│   ├─> Re-habilitar canales: ~5µs
│   └─> Total: ~30µs por lectura
├─> 20 lecturas × 30µs = 600µs
├─> Cálculo promedios: ~50µs
├─> Actualizar LEDs: ~10µs
└─> Total por ciclo: ~660µs

Tasa máxima sin delay: ~1515 Hz
Tasa real con delay (10000 iter): ~100 Hz
CPU usage: ~80% (lectura activa)
```

### Performance Modo CON DMA

```
Ciclo de muestreo (hardware):
├─> ADC convierte automáticamente: 200µs
├─> DMA transfiere datos: automático (0% CPU)
└─> Interrupción + sincronización: ~5µs

Procesamiento (software):
├─> Esperar datos_listos: 0% CPU (idle)
├─> Cálculo promedios: ~50µs
├─> Actualizar LEDs: ~10µs
└─> Total procesamiento: ~60µs

Tasa de muestreo: 5000 Hz (fija por hardware)
Tasa de procesamiento: ~100 Hz (configurable por delay)
CPU usage: ~5% (solo procesar resultados)
CPU libre: ~95% (para otras tareas)
```

### Comparación de Carga de CPU

```
Modo SIN DMA:
CPU [████████████████████████████████████░░░░░░░░░] 80%
    │                                   │
    └─ Leyendo ADC activamente          └─ Delays

Modo CON DMA:
CPU [██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 5%
    │ │                                
    │ └─ Procesar datos                
    └─ Interrupciones DMA              
    
    ░░░ = CPU libre para:
        - Actualizar display LCD
        - Calcular física del juego
        - Generar música (Timer0)
        - Comunicación serial
        - Modo sleep (ahorro energía)
```

### Latencia de Respuesta

| Modo | Desde movimiento joystick hasta LED | Jitter |
|------|-------------------------------------|--------|
| **SIN DMA** | ~1-10 ms (variable) | Alto |
| **CON DMA** | 200 µs (fijo) | Bajo |

**Conclusión:** Modo DMA tiene **5-50x menos latencia** con timing predecible.

---

## 🔧 Activar Modo DMA

Para cambiar del modo SIN DMA al modo CON DMA, editar `main()`:

```c
int main(void) {
    SystemInit();
    config_gpio_leds();
    config_adc();
    calibrar_joystick();
    
    // COMENTAR ESTO (Modo SIN DMA):
    /*
    while (1) {
        test_sin_dma();
        for (volatile uint32_t i = 0; i < 10000; i++);
    }
    */
    
    // DESCOMENTAR ESTO (Modo CON DMA):
    config_dma();
    
    while (1) {
        procesar_joystick();
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
    
    return 0;
}
```

---

## 📚 Referencias

- **Manual:** LPC17xx User Manual (UM10360)
- **Capítulos relevantes:**
  - Chapter 29: ADC (Analog-to-Digital Converter)
  - Chapter 31: GPDMA (General Purpose DMA)
- **Joystick:** KY-023 Dual Axis XY Joystick Module
- **Código fuente:** `src/adc.c`

---

## 🎮 Integración con Snake Game

El sistema de joystick está diseñado para integrarse con el juego Snake:

- **Modo recomendado:** CON DMA
- **Timer0 Match 0:** Genera música de fondo
- **Timer0 Match 1:** Controla velocidad del juego (1ms tick)
- **DMA:** Lee joystick sin interrumpir música ni juego
- **LCD I2C:** Muestra puntuación (usando tiempo libre del CPU)

**Prioridades de interrupciones:**
1. Timer0 (prioridad 1) - Audio crítico
2. DMA (prioridad 2) - Control del juego
3. I2C (prioridad 3) - Display no crítico

---