# 📘 Documentación Técnica: Generador de Señal Triangular con DAC

**Proyecto**: SNAKE---LPC1769  
**Archivo**: `src/triangular_signal.c`  
**Microcontrolador**: LPC1769 (ARM Cortex-M3)  
**Fecha**: 31 de Octubre, 2025

---

## 📑 Tabla de Contenidos

1. [Descripción General](#descripción-general)
2. [Parámetros de Configuración](#parámetros-de-configuración)
3. [Configuración de Hardware](#configuración-de-hardware)
4. [Análisis de Funcionamiento](#análisis-de-funcionamiento)
5. [Diagramas y Tablas](#diagramas-y-tablas)
6. [Cálculos y Fórmulas](#cálculos-y-fórmulas)
7. [Troubleshooting](#troubleshooting)

---

## 📋 Descripción General

Este programa genera una **onda triangular analógica de 100 Hz** utilizando el DAC (Conversor Digital-Analógico) del microcontrolador LPC1769. La señal se construye mediante una tabla de 128 valores que se actualizan periódicamente mediante interrupciones del TIMER0.

### Características Principales

- ✅ Generación de onda triangular pura
- ✅ Frecuencia de señal: **100 Hz**
- ✅ Resolución: **10 bits (1024 niveles)**
- ✅ Frecuencia de muestreo: **12.82 kHz**
- ✅ LED indicador de actividad
- ✅ Sistema basado en interrupciones

### Aplicaciones

- Generación de señales de prueba
- Sistemas de audio
- Control de motores
- Calibración de equipos
- Enseñanza de procesamiento de señales

---

## ⚙️ Parámetros de Configuración

### Constantes Definidas

```c
#define NUMERO_MUESTRAS            128      // Muestras por ciclo completo
#define MAXIMO_VALOR_DAC          1023      // Valor máximo del DAC (10 bits)
#define FRECUENCIA_DE_MUESTREO_US   78      // Período entre muestras (μs)
```

### Tabla de Especificaciones

| Parámetro | Valor | Unidad | Descripción |
|-----------|-------|--------|-------------|
| **Señal de Salida** ||||
| Frecuencia | 100 | Hz | Frecuencia de la onda triangular |
| Período | 10 | ms | Duración de un ciclo completo |
| Amplitud | 0 - 3.3 | V | Rango de voltaje de salida |
| Forma | Triangular | - | Simétrica ascendente/descendente |
| **Muestreo** ||||
| Muestras/ciclo | 128 | - | Puntos de la tabla |
| Tiempo entre muestras | 78 | μs | Período de muestreo |
| Frecuencia de muestreo | 12,820 | Hz | 1 / 78μs |
| **DAC** ||||
| Resolución | 10 | bits | 0 a 1023 |
| Voltaje por paso | 3.22 | mV | 3.3V / 1024 |
| Tiempo de establecimiento | ~1 | μs | Con BIAS=0 |
| Corriente de consumo | ~700 | μA | Modo alta velocidad |
| **Timer** ||||
| Timer usado | TIMER0 | - | Timer/Counter 0 |
| Prescaler | 1 | μs | Incremento cada microsegundo |
| Match Value | 78 | - | Genera interrupción |
| **Indicadores** ||||
| LED toggle | 39 | ms | Cada 500 interrupciones |
| Pin LED | P0.22 | - | GPIO de salida |

---

## 🔧 Configuración de Hardware

### 1️⃣ GPIO - Función `config_GPIO()`

#### Pin P0.22 - LED Indicador

```c
pin_configuration.portNum   = PINSEL_PORT_0;
pin_configuration.pinNum    = PINSEL_PIN_22;
pin_configuration.funcNum   = PINSEL_FUNC_0;    // Función GPIO
pin_configuration.pinMode   = PINSEL_PULLUP;     // Resistencia pull-up
pin_configuration.openDrain = PINSEL_OD_NORMAL;  // Push-pull
```

**Configuración:**
- **Puerto**: 0
- **Pin**: 22
- **Función**: GPIO (FUNC_0)
- **Modo**: Pull-up habilitado
- **Dirección**: Salida

**Razón de Configuración:**
- Se usa como indicador visual de actividad del sistema
- Pull-up para estado definido cuando no se maneja
- Push-pull para mejor capacidad de corriente
- Parpadea cada 500 interrupciones (~39 ms)

#### Pin P0.26 - Salida Analógica DAC (AOUT)

```c
pin_configuration.portNum   = PINSEL_PORT_0;
pin_configuration.pinNum    = PINSEL_PIN_26;
pin_configuration.funcNum   = PINSEL_FUNC_2;    // Función AOUT (DAC)
pin_configuration.pinMode   = PINSEL_TRISTATE;  // Alta impedancia
pin_configuration.openDrain = PINSEL_OD_NORMAL;
```

**Configuración:**
- **Puerto**: 0
- **Pin**: 26 (único pin con DAC)
- **Función**: AOUT (FUNC_2)
- **Modo**: Tristate (sin pull-up/down)
- **Tipo**: Salida analógica

**Razón de Configuración:**
- `PINSEL_FUNC_2` habilita la función especial del DAC
- `PINSEL_TRISTATE` evita interferencias de resistencias pull en la señal analógica
- Este es el único pin del LPC1769 con capacidad DAC
- Salida analógica de 0 a 3.3V

---

### 2️⃣ DAC - Función `config_DAC()`

```c
DAC_Init();              // Inicializa el periférico DAC
DAC_SetBias(0);          // BIAS = 0 → Máxima velocidad
DAC_UpdateValue(0);      // Valor inicial: 0V
```

#### Configuración del BIAS

El bit BIAS del registro DACCTRL controla el compromiso velocidad/consumo:

| BIAS | Settling Time | Corriente | Uso |
|------|---------------|-----------|-----|
| 0 | ~1 μs | 700 μA | **Alta velocidad** ← Usado aquí |
| 1 | ~2.5 μs | 350 μA | Bajo consumo |

**Razón de Configuración:**
- Se eligió `BIAS=0` para máxima velocidad
- Con muestreo cada 78 μs, se requiere respuesta rápida
- Settling time de 1 μs permite transiciones limpias
- Necesario para mantener fidelidad de la señal a 12.82 kHz

#### Características del DAC

- **Resolución**: 10 bits (1024 niveles)
- **Rango**: 0 a 3.3V
- **Ecuación**: $V_{out} = \frac{VALUE}{1024} \times 3.3V$
- **Mínimo paso**: $\frac{3.3V}{1024} \approx 3.22mV$

**Ejemplo de Valores:**

| Valor Digital | Voltaje Analógico |
|---------------|-------------------|
| 0 | 0.00 V |
| 256 | 0.83 V |
| 512 | 1.65 V |
| 768 | 2.48 V |
| 1023 | 3.30 V |

---

### 3️⃣ TIMER0 - Función `config_timer()`

```c
// Configuración del Timer
cfgtimer.prescaleOption = TIM_USVAL;    // Base de tiempo: microsegundos
cfgtimer.prescaleValue  = 1;            // Incrementa cada 1 μs

// Configuración del Match
cfgmatch.matchChannel        = 0;       // Usa canal MATCH0
cfgmatch.intOnMatch          = ENABLE;  // Genera interrupción
cfgmatch.resetOnMatch        = ENABLE;  // Reinicia contador
cfgmatch.stopOnMatch         = DISABLE; // Continúa corriendo
cfgmatch.extMatchOutputType  = TIM_NOTHING;
cfgmatch.matchValue          = 78;      // Coincide cada 78 μs
```

#### Flujo de Operación del Timer

```
┌─────────────────────────────────────────────────────┐
│  Timer Counter: 0 → 1 → 2 → ... → 78 → 0 (reset)  │
│                                    ↑                │
│                              Genera IRQ             │
│                           Ejecuta Handler           │
└─────────────────────────────────────────────────────┘
```

**Secuencia:**
1. Timer incrementa cada 1 μs (prescaler)
2. Cuando llega a 78, genera interrupción (MATCH0)
3. Se ejecuta `TIMER0_IRQHandler()`
4. Timer se resetea automáticamente
5. Ciclo se repite indefinidamente

**Razón de Configuración:**
- Proporciona base de tiempo precisa para muestreo
- Frecuencia de 12.82 kHz es adecuada para señal de 100 Hz
- Cumple con teorema de Nyquist (fs >> 2×f_señal)
- Reset automático simplifica el código

---

## 🔄 Análisis de Funcionamiento

### Función Principal `main()`

```c
int main(void) {
    config_GPIO();      // 1. Configura pines
    config_DAC();       // 2. Inicializa DAC
    config_timer();     // 3. Inicia timer e interrupciones
    
    GPIO_SetPins(PORT_CERO, PIN_22);  // 4. Enciende LED
    
    while(1) {          // 5. Loop infinito (todo por interrupciones)
    }
    
    return 0;
}
```

**Flujo de Inicialización:**
1. Configura hardware (GPIO, DAC, Timer)
2. Habilita interrupciones
3. Entra en loop infinito
4. Todo el trabajo se hace en la ISR

---

### Manejador de Interrupción `TIMER0_IRQHandler()`

```c
void TIMER0_IRQHandler(void){
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        
        // ① LIMPIAR FLAG DE INTERRUPCIÓN
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        
        // ② ACTUALIZAR DAC CON VALOR DE LA TABLA
        DAC_UpdateValue(TABLA_DE_LA_SEÑAL_TRIANGULAR[INDICE_TABLA_DE_ONDA]);
        
        // ③ AVANZAR AL SIGUIENTE ÍNDICE
        INDICE_TABLA_DE_ONDA++;
        if(INDICE_TABLA_DE_ONDA >= NUMERO_MUESTRAS) {
            INDICE_TABLA_DE_ONDA = 0;  // Wrap around
        }
        
        // ④ CONTROL DEL LED (DEBUG)
        static uint16_t led_counter = 0;
        if(++led_counter >= 500) {
            led_counter = 0;
            // Toggle LED
            if(GPIO_ReadValue(PORT_CERO) & PIN_22) {
                GPIO_ClearPins(PORT_CERO, PIN_22);
            } else {
                GPIO_SetPins(PORT_CERO, PIN_22);
            }
        }
    }
    return;
}
```

#### Diagrama de Flujo de la ISR

```
         TIMER0 IRQ
              ↓
    ¿MATCH0 generó IRQ?
         ↓ [SÍ]
    Limpiar Flag
         ↓
    DAC ← Tabla[índice]
         ↓
    índice++
         ↓
    ¿índice >= 128?
    ↓ [SÍ]    ↓ [NO]
índice=0    Continuar
    ↓           ↓
    led_counter++
         ↓
    ¿led_counter >= 500?
         ↓ [SÍ]
    Toggle LED
    led_counter=0
         ↓
      Return
```

#### Paso a Paso de la Ejecución

**① Verificación y Limpieza (Líneas 1-3)**
- Verifica que la interrupción sea del MATCH0
- Limpia el flag para permitir siguiente interrupción
- **Crítico**: Si no se limpia, la ISR se ejecutaría continuamente

**② Actualización del DAC (Línea 6)**
```c
DAC_UpdateValue(TABLA_DE_LA_SEÑAL_TRIANGULAR[INDICE_TABLA_DE_ONDA]);
```
- Lee valor de la tabla en posición actual
- Escribe al registro del DAC
- El DAC convierte inmediatamente a voltaje analógico
- **Tiempo de ejecución**: < 1 μs (crucial para no perder muestras)

**③ Avance del Índice (Líneas 9-12)**
```c
INDICE_TABLA_DE_ONDA++;
if(INDICE_TABLA_DE_ONDA >= NUMERO_MUESTRAS) {
    INDICE_TABLA_DE_ONDA = 0;  // Circular buffer
}
```
- Incrementa índice para siguiente muestra
- Implementa buffer circular (wrap-around)
- Al llegar a 128, vuelve a 0 (ciclo continuo)

**④ Indicador LED (Líneas 15-24)**
```c
static uint16_t led_counter = 0;
if(++led_counter >= 500) {
    led_counter = 0;
    // Toggle LED
}
```
- Contador estático (mantiene valor entre llamadas)
- Toggle cada 500 interrupciones
- **Frecuencia del LED**: 500 × 78μs = 39ms → ~25 Hz
- Proporciona feedback visual sin osciloscopio

---

## 📊 Diagramas y Tablas

### Tabla de la Señal Triangular

La tabla contiene 128 valores que forman una onda triangular simétrica:

```
TABLA_DE_LA_SEÑAL_TRIANGULAR[128] = {
    // Rampa ascendente (índices 0-63): 0 → 1023
    0, 16, 32, 48, ..., 992, 1008, 1023,
    
    // Rampa descendente (índices 64-127): 1023 → 0
    1023, 1008, 992, ..., 48, 32, 16
}
```

#### Estructura de la Tabla

| Sección | Índices | Valores | Descripción |
|---------|---------|---------|-------------|
| **Rampa Ascendente** | 0 - 63 | 0 → 1023 | Primera mitad del ciclo |
| **Pico** | 64 | 1023 | Valor máximo |
| **Rampa Descendente** | 65 - 127 | 1023 → 16 | Segunda mitad |
| **Reinicio** | → 0 | 0 | Cierra el ciclo |

#### Características de la Tabla

- **Incremento**: 1023 / 64 ≈ **16 pasos** por muestra
- **Simetría**: Perfectamente simétrica (ascendente = descendente invertido)
- **Continuidad**: Sin discontinuidades (valor[127]=16, valor[0]=0)
- **Linealidad**: Cambio constante entre muestras

### Visualización de la Señal

```
Voltaje (V)
3.3V ┤           ╱╲
     │          ╱  ╲
     │         ╱    ╲
1.65V┤        ╱      ╲
     │       ╱        ╲
     │      ╱          ╲
0.0V ┤─────╱            ╲─────
     └────────────────────────→ Tiempo
     0    2.5ms  5ms   7.5ms  10ms
     |←─── Período = 10ms ────→|
```

### Diagrama de Bloques del Sistema

```
┌──────────────┐
│   TIMER0     │
│  (78 μs)     │
└──────┬───────┘
       │ IRQ cada 78μs
       ↓
┌──────────────────────────┐
│   TIMER0_IRQHandler()    │
│  - Lee tabla[índice]     │
│  - Escribe a DAC         │
│  - índice++              │
└──────┬───────────────────┘
       ↓
┌──────────────┐      ┌──────────────┐
│     DAC      │      │   GPIO LED   │
│   (P0.26)    │      │   (P0.22)    │
└──────┬───────┘      └──────┬───────┘
       │                     │
       ↓                     ↓
  Onda Triangular       Parpadeo
    100 Hz               ~25 Hz
    0-3.3V
```

---

## 🔬 Cálculos y Fórmulas

### Frecuencia de la Señal

**Fórmula base:**
$$f_{señal} = \frac{1}{T_{período}}$$

**Período de la señal:**
$$T_{período} = N_{muestras} \times T_{muestreo}$$

**Sustituyendo valores:**
$$T_{período} = 128 \times 78\mu s = 9,984\mu s \approx 10ms$$

**Frecuencia resultante:**
$$f_{señal} = \frac{1}{10ms} = 100Hz$$

---

### Frecuencia de Muestreo

**Fórmula:**
$$f_{muestreo} = \frac{1}{T_{muestreo}}$$

**Cálculo:**
$$f_{muestreo} = \frac{1}{78\mu s} = 12,820.5Hz \approx 12.82kHz$$

**Ratio de muestreo:**
$$\frac{f_{muestreo}}{f_{señal}} = \frac{12,820Hz}{100Hz} = 128.2$$

> ✅ **Muy superior al criterio de Nyquist** (mínimo 2×f_señal = 200 Hz)

---

### Resolución del DAC

**Voltaje por paso:**
$$V_{paso} = \frac{V_{max}}{2^{bits}} = \frac{3.3V}{2^{10}} = \frac{3.3V}{1024} = 3.22mV$$

**Voltaje de salida para cualquier valor:**
$$V_{out} = \frac{VALUE}{1024} \times 3.3V$$

**Ejemplos:**
- Valor 512: $V_{out} = \frac{512}{1024} \times 3.3 = 1.65V$
- Valor 768: $V_{out} = \frac{768}{1024} \times 3.3 = 2.48V$

---

### Distorsión Armónica Total (THD)

Para una onda triangular con 128 muestras:

**THD estimado:**
$$THD \approx \frac{1}{N_{muestras}} \times 100\% = \frac{1}{128} \times 100\% \approx 0.78\%$$

> ✅ **Excelente calidad** - THD < 1% se considera alta fidelidad

---

### Frecuencia del LED

**Período del toggle:**
$$T_{LED} = N_{interrupciones} \times T_{muestreo} = 500 \times 78\mu s = 39ms$$

**Frecuencia de parpadeo:**
$$f_{LED} = \frac{1}{2 \times T_{LED}} = \frac{1}{78ms} \approx 12.8Hz$$

(Dividido por 2 porque el LED tiene dos estados: ON y OFF)

---

### Tiempo de Ejecución de la ISR

**Operaciones en la ISR:**
1. Verificación de flag: ~2 ciclos
2. Limpieza de flag: ~5 ciclos
3. Lectura de tabla: ~3 ciclos
4. Escritura a DAC: ~10 ciclos
5. Incremento de índice: ~3 ciclos
6. Comparación: ~2 ciclos
7. LED (ocasional): ~20 ciclos

**Total estimado:** ~25-45 ciclos de reloj

**Con CPU a 100 MHz:**
$$T_{ISR} = \frac{45 ciclos}{100MHz} = 0.45\mu s$$

**Carga del CPU:**
$$Carga = \frac{T_{ISR}}{T_{muestreo}} = \frac{0.45\mu s}{78\mu s} \times 100\% \approx 0.58\%$$

> ✅ **Carga mínima** - Deja 99.4% del CPU libre para otras tareas

---

## 🛠️ Troubleshooting

### Problemas Comunes y Soluciones

#### 1. No hay señal en la salida DAC

**Síntomas:**
- Voltaje constante en P0.26
- No se observa onda triangular

**Causas posibles:**
- ✗ Pin mal configurado
- ✗ DAC no inicializado
- ✗ Interrupciones deshabilitadas

**Solución:**
```c
// Verificar configuración del pin
PINSEL_ConfigPin(&pin_configuration);  // FUNC_2 para AOUT

// Verificar inicialización del DAC
DAC_Init();
DAC_SetBias(0);

// Verificar habilitación de interrupciones
NVIC_EnableIRQ(TIMER0_IRQn);
```

---

#### 2. Frecuencia incorrecta

**Síntomas:**
- Señal más rápida o lenta de lo esperado

**Causas posibles:**
- ✗ `FRECUENCIA_DE_MUESTREO_US` incorrecto
- ✗ Prescaler del timer mal configurado

**Solución:**
```c
// Para 100 Hz con 128 muestras:
#define FRECUENCIA_DE_MUESTREO_US  78  // (10ms / 128 = 78.125μs)

// Verificar prescaler
cfgtimer.prescaleOption = TIM_USVAL;  // DEBE ser microsegundos
cfgtimer.prescaleValue  = 1;           // Incrementa cada 1μs
```

**Fórmula para calcular:**
$$T_{muestreo} = \frac{T_{deseado}}{N_{muestras}} = \frac{10ms}{128} = 78.125\mu s$$

---

#### 3. Señal distorsionada

**Síntomas:**
- Forma de onda irregular
- Glitches o picos

**Causas posibles:**
- ✗ BIAS del DAC en modo lento
- ✗ ISR tarda demasiado
- ✗ Tabla mal generada

**Solución:**
```c
// Usar modo rápido del DAC
DAC_SetBias(0);  // DEBE ser 0 para alta velocidad

// Minimizar código en la ISR
// Evitar operaciones pesadas dentro del handler

// Verificar tabla de valores
// Asegurar continuidad entre último y primer valor
```

---

#### 4. LED no parpadea

**Síntomas:**
- LED siempre encendido o apagado
- No hay indicación visual

**Causas posibles:**
- ✗ Pin LED mal configurado
- ✗ Contador del LED no incrementa
- ✗ Timer no genera interrupciones

**Solución:**
```c
// Verificar configuración GPIO
GPIO_SetDir(PORT_CERO, PIN_22, SALIDA);

// Verificar inicialización del LED
GPIO_SetPins(PORT_CERO, PIN_22);  // Estado inicial

// Verificar que la ISR se ejecuta
// Usar debugger o cambiar contador a valor menor para test
```

---

#### 5. Sistema se cuelga o resetea

**Síntomas:**
- Microcontrolador se resetea constantemente
- Código no ejecuta correctamente

**Causas posibles:**
- ✗ Stack overflow por ISR recursiva
- ✗ Flag de interrupción no se limpia
- ✗ Watchdog timer activo

**Solución:**
```c
// SIEMPRE limpiar flag de interrupción
TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);  // ¡CRÍTICO!

// Verificar que ISR termina rápido
// No usar delays o loops largos dentro de la ISR

// Verificar tamaño del stack
// Aumentar si es necesario en el linker script
```

---

### Mediciones y Verificación

#### Con Osciloscopio

**Mediciones esperadas en P0.26:**
- **Frecuencia**: 100 Hz ± 1%
- **Amplitud**: 0 a 3.3V
- **Forma**: Triangular simétrica
- **Duty cycle**: 50% (rampa up = rampa down)

**Mediciones esperadas en P0.22 (LED):**
- **Frecuencia**: ~12.8 Hz
- **Duty cycle**: 50%

---

#### Sin Osciloscopio

**Verificación con multímetro:**
- Voltaje DC promedio en P0.26: ~1.65V (3.3V / 2)
- Si es muy diferente, hay problema

**Verificación con LED:**
- Debe parpadear visiblemente (~13 veces/segundo)
- Si parpadea muy rápido o muy lento, revisar configuración

---

### Modificaciones Comunes

#### Cambiar la Frecuencia de la Señal

Para generar una señal de **frecuencia diferente**:

**Opción 1: Cambiar período de muestreo**
```c
// Para 200 Hz (5ms de período):
#define FRECUENCIA_DE_MUESTREO_US  39  // 5ms / 128 = 39.06μs
```

**Opción 2: Cambiar número de muestras**
```c
// Para 100 Hz con 64 muestras:
#define NUMERO_MUESTRAS  64
#define FRECUENCIA_DE_MUESTREO_US  156  // 10ms / 64 = 156.25μs
// Regenerar la tabla con 64 valores
```

**Fórmula general:**
$$T_{muestreo}(\mu s) = \frac{T_{período\_deseado}(ms) \times 1000}{N_{muestras}}$$

---

#### Cambiar Amplitud de la Señal

Para generar amplitud **reducida** (ej: 0-1.65V en vez de 0-3.3V):

```c
// Opción 1: Escalar todos los valores en la tabla
const uint16_t TABLA_DE_LA_SEÑAL_TRIANGULAR[NUMERO_MUESTRAS] = {
    0, 8, 16, 24, ...  // Dividir todos los valores entre 2
};

// Opción 2: Usar offset en el DAC
DAC_UpdateValue(TABLA_DE_LA_SEÑAL_TRIANGULAR[INDICE_TABLA_DE_ONDA] / 2);
```

---

#### Generar Otras Formas de Onda

**Onda Senoidal:**
```c
// Generar tabla con:
// valor[i] = 512 + 511 * sin(2π * i / 128)
const uint16_t TABLA_SENOIDAL[128] = { /* ... */ };
```

**Onda Cuadrada:**
```c
// Primera mitad: 1023, segunda mitad: 0
const uint16_t TABLA_CUADRADA[128] = {
    1023, 1023, ..., 1023,  // 64 valores
    0, 0, ..., 0             // 64 valores
};
```

**Onda Diente de Sierra:**
```c
// Rampa ascendente completa
const uint16_t TABLA_DIENTE_SIERRA[128] = {
    0, 8, 16, 24, ..., 1016, 1023  // Ascendente solamente
};
```

---

## 📚 Referencias

### Documentos Técnicos

- **LPC17xx User Manual (UM10360)**: Capítulos 27 (DAC), 21 (Timer), 8 (GPIO)
- **CMSIS Documentation**: Core peripheral access layer
- **ARM Cortex-M3 Technical Reference Manual**: Interrupt handling

### Teoría de Señales

- **Teorema de Nyquist**: $f_{muestreo} \geq 2 \times f_{señal\_max}$
- **Resolución efectiva**: Bits efectivos vs nominales
- **THD (Total Harmonic Distortion)**: Medida de calidad de señal

### Herramientas de Desarrollo

- **MCUXpresso / Keil**: IDEs recomendados
- **Flash Magic**: Para programación del LPC1769
- **Osciloscopio**: Verificación de señales analógicas

---

## 📝 Notas Finales

### Ventajas de este Diseño

✅ **Eficiente**: Carga de CPU < 1%  
✅ **Preciso**: Timer por hardware, no delays por software  
✅ **Escalable**: Fácil cambiar frecuencia o forma de onda  
✅ **Robusto**: Sistema basado en interrupciones  
✅ **Documentado**: Código bien comentado  

### Limitaciones

⚠️ **Frecuencia fija**: Requiere recompilar para cambiar  
⚠️ **Una señal a la vez**: Solo un DAC disponible  
⚠️ **Resolución limitada**: 10 bits (3.22mV/paso)  
⚠️ **Sin control de volumen**: Siempre 0-3.3V  

### Mejoras Posibles

🔧 **Frecuencia variable**: Calcular `matchValue` dinámicamente  
🔧 **Múltiples ondas**: Selector por botón o UART  
🔧 **Amplitud ajustable**: Escalar valores de la tabla  
🔧 **DMA**: Transferir tabla al DAC sin CPU  
🔧 **Filtro anti-aliasing**: Hardware externo para mejorar señal 
