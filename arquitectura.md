# ARQUITECTURA DE HARDWARE - DINOCHROME ARCADE
## Microcontrolador: LPC1769 (ARM Cortex-M3, 100 MHz)

---

## 1️⃣ TIMERS (Temporizadores)

### Timer0
- **Uso**: Generación de melodías (DAC) - Notas musicales
- **Modo**: Match Control (PWM o interrupción)
- **Match0 (MAT0.0)**: P0.28 (no usado - reservado)
- **Frecuencia**: Variable según melodía (200 Hz - 2 kHz)
- **Interrupción**: TIMER0_IRQn

### Timer1
- **Uso**: Soporte adicional para melodías (si se necesita mayor resolución)
- **Modo**: Match Control
- **Frecuencia**: Similar a Timer0
- **Interrupción**: TIMER1_IRQn

### Timer2
- **Uso**: CRÍTICO - Tick principal del juego (50 ms = 20 Hz)
- **Modo**: Interrupción por Match
- **Match0 (MAT2.0)**: P0.6 (no usado - solo interrupción)
- **Período**: 50 ms (PCLK / PRESCALER = 1 tick)
- **Interrupción**: TIMER2_IRQn
- **Variable Global**: `bandera_tick_juego` (actualizada en ISR)

### Timer3
- **Uso**: CRÍTICO - Tick del juego Snake (50 ms = 20 Hz)
- **Modo**: Interrupción por Match
- **Match0 (MAT3.0)**: P0.10 (no usado - solo interrupción)
- **Período**: 50 ms (Prescaler 1ms, Match value 50)
- **Configuración**:
  ```c
  Prescaler: 1000 (1ms por tick)
  Match value: 50 (50 ticks = 50ms)
  Reset automático en match
  ```
- **Interrupción**: TIMER3_IRQn
- **Variable Global**: `tick_flag` (actualizada en ISR)
- **Función**:
  - Se ejecuta cada 50ms en la función `config_timer()`
  - Handler: `TIMER3_IRQHandler()` setea `tick_flag = 1`
  - Sincroniza la velocidad de movimiento de la serpiente
  - Frecuencia resultante: 20 Hz (20 ticks por segundo)

---

## 2️⃣ ADC (Conversor Analógico-Digital)

### Configuración General
- **Periférico**: ADC0 (LPC_ADC)
- **Frecuencia de Muestreo**: PCLK / (CLKDIV + 1) = 24 MHz / 25 = 960 kHz
- **Resolución**: 12 bits (0 - 4095)
- **Modo**: Lectura manual (sin DMA)

### Canales ADC Utilizados

| Canal | Pin | Función | Rango |
|-------|-----|---------|-------|
| **AD0.0** | P0.23 | Eje X Joystick | 0-4095 (6 a 4095) |
| **AD0.1** | P0.24 | Eje Y Joystick | 0-4095 (7 a 4095) |
| - | - | Restantes | No utilizados |

### Configuración de Pines
```
P0.23 (AD0.0) - Joystick Eje X
  ├─ PINSEL1[14:15] = 01 (Función ADC)
  └─ Entrada analógica (sin GPIO)

P0.24 (AD0.1) - Joystick Eje Y
  ├─ PINSEL1[16:17] = 01 (Función ADC)
  └─ Entrada analógica (sin GPIO)
```

### Lectura del ADC
```c
// Promediado de 4 muestras por canal
// Filtro digital: Zona muerta (Deadzone) ±500 alrededor del centro (2048)
```

---

## 3️⃣ DMA (Acceso Directo a Memoria)

### DMA General
- **Periférico**: GPDMA (General Purpose DMA)
- **Canales**: 8 disponibles (LPC1769)
- **Modo**: Linked List para transferencias continuas

### Canal DMA 0 - Bluetooth UART
- **Número de Canal**: 0
- **Fuente**: UART0 RBR (Receptor)
- **Destino**: Buffer circular `buffer_rx_bt[256]`
- **Tipo de Transferencia**: 
  - **Ancho**: 8 bits por transferencia
  - **Ráfaga**: 4 bytes por solicitud
  - **Longitud**: 256 bytes
  - **Modo**: Circular (reinicia al final)
- **Solicitud DMA**: UART0_RX
- **Interrupción**: `GPDMA_IRQHandler` → `GPDMA_IntGetStatus(GPDMA_INTTC, 0)`

### Canal DMA 1 - Melodías (DAC)
- **Número de Canal**: 1
- **Fuente**: Buffer de notas `buffer_notas[MAX_NOTAS]`
- **Destino**: DAC0 (P0.26)
- **Tipo de Transferencia**:
  - **Ancho**: 32 bits (incluye bit de espera)
  - **Longitud**: MAX_NOTAS transferencias
  - **Modo**: Circular (loop de melodía)
- **Solicitud DMA**: DAC_TX
- **Interrupción**: `GPDMA_IRQHandler` → `GPDMA_IntGetStatus(GPDMA_INTTC, 1)`

---

## 4️⃣ UART (Comunicación Serial)

### UART0 - Bluetooth
- **Puerto**: P0.2 (TX) / P0.3 (RX)
- **Configuración PINSEL0**:
  ```
  P0.2 (TX):  PINSEL0[5:4]   = 01 (Función UART0 TXD0)
  P0.3 (RX):  PINSEL0[7:6]   = 01 (Función UART0 RXD0)
  ```
- **Velocidad**: 9600 baud
- **Formato**: 8 bits, 1 stop, sin paridad
- **Modo**: DMA para RX, Poll para TX
- **Periférico**: LPC_UART0
- **Interrupciones**: 
  - UART0_IRQn (no habilitada, usa DMA)
  - GPDMA_IRQn (para fin de transferencia DMA)

### Protocolo Bluetooth
```
Comandos recibidos:
  'W' → Arriba
  'S' → Abajo
  'A' → Izquierda
  'D' → Derecha
  'B' → Botón
```

---

## 5️⃣ I2C (Bus Inter-Integrated Circuit)

### I2C0 - Display LCD
- **Pines**: P0.27 (SDA0) / P0.28 (SCL0)
- **Configuración PINSEL0**:
  ```
  P0.27 (SDA0): PINSEL0[55:54] = 01 (Función I2C0 SDA)
  P0.28 (SCL0): PINSEL0[57:56] = 01 (Función I2C0 SCL)
  ```
- **Velocidad**: 100 kHz (Standard I2C)
- **Dirección LCD**: 0x27 (7 bits) / 0x4E (8 bits)
- **Tipo LCD**: 20x4 caracteres
- **Periférico**: LPC_I2C0
- **Modo**: Polling (bloqueante, bajo tráfico)

---

## 6️⃣ DAC (Conversor Digital-Analógico)

### DAC0 - Audio Melodías
- **Pin**: P0.26
- **Configuración PINSEL1**:
  ```
  P0.26 (DAC_OUT): PINSEL1[21:20] = 10 (Función DAC)
  ```
- **Resolución**: 10 bits (0 - 1023)
- **Voltaje Salida**: 0 - 3.3V
- **Fuente de Datos**: DMA (Canal 1)
- **Periférico**: LPC_DAC
- **Frecuencia de Actualización**: Variable (controlada por Timer0/Timer1)

---

## 7️⃣ GPIO (Entrada/Salida Digital)

### Joystick Button
| Pin | Puerto | Función | Configuración |
|-----|--------|---------|---------------|
| **P2.10** | 2 | Botón Joystick | EINT3 (Interrupción) |

- **Configuración**:
  ```
  PINSEL4[21:20] = 00 (GPIO)
  FIODIR[10] = 0 (Entrada)
  IO2IntEnF = 1 (Flanco de bajada)
  ```
- **Interrupción**: EINT3_IRQn
- **Lógica**: Activo bajo (presionado = 0)

### LEDs Indicadores Joystick
| Pin | Nombre | Función | Lógica |
|-----|--------|---------|--------|
| **P0.0** | LED_BOTON | Centro/Botón | Activo Alto (1=ON) |
| **P0.6** | LED_DERECHA | Joystick Derecha | Activo Alto |
| **P0.7** | LED_IZQUIERDA | Joystick Izquierda | Activo Alto |
| **P0.8** | LED_ABAJO | Joystick Abajo | Activo Alto |
| **P0.9** | LED_ARRIBA | Joystick Arriba | Activo Alto |

- **Configuración PINSEL0**:
  ```
  P0.0:  PINSEL0[1:0]     = 00 (GPIO)
  P0.6:  PINSEL0[13:12]   = 00 (GPIO)
  P0.7:  PINSEL0[15:14]   = 00 (GPIO)
  P0.8:  PINSEL0[17:16]   = 00 (GPIO)
  P0.9:  PINSEL0[19:18]   = 00 (GPIO)
  
  FIODIR: Todos como salida (1)
  ```

### Botón GPIO
| Pin | Función | Configuración |
|-----|---------|---------------|
| **P0.4** | Botón P0.4 | GPIO entrada con Pull-up |

- **Configuración PINSEL0**:
  ```
  P0.4: PINSEL0[9:8] = 00 (GPIO)
  FIODIR[4] = 0 (Entrada)
  PINMODE[9:8] = 10 (Pull-up habilitado)
  ```

---

## 8️⃣ RELOJ (Clock Distribution)

### Fuentes de Reloj
```
Oscillator: 12 MHz (externo)
     ↓
PLL0 (Phase Lock Loop)
     ↓
CCLK (CPU Clock): 100 MHz
     ↓
┌────────────────────────────────┐
├─ PCLK (Periféricos): 25 MHz    │ (CCLK / 4)
├─ Timer0/1/2/3: 25 MHz          │
├─ ADC: 25 MHz (CLKDIV=24)        │
├─ UART0: 25 MHz                 │
├─ I2C0: 25 MHz                  │
├─ DMA: 100 MHz                  │
└─ DAC: 25 MHz                   │
```

### Power Control (PCONP)
```c
PCONP |= (1 << 12);  // ADC0 encendido
PCONP |= (1 << 0);   // Timer0 encendido
PCONP |= (1 << 2);   // UART0 encendido
PCONP |= (1 << 7);   // I2C0 encendido
PCONP |= (1 << 29);  // DAC encendido
PCONP |= (1 << 6);   // DMA encendido
```

---

## 📊 **TABLA RESUMEN DE PINES**

| Pin | Puerto | Función | Periférico | PINSEL | Estado |
|-----|--------|---------|------------|--------|--------|
| P0.0 | 0 | LED Centro | GPIO | 00 | OUT |
| P0.2 | 0 | UART0 TX | UART | 01 | TX |
| P0.3 | 0 | UART0 RX | UART | 01 | RX |
| P0.4 | 0 | Botón | GPIO | 00 | IN |
| P0.6 | 0 | LED Derecha | GPIO | 00 | OUT |
| P0.7 | 0 | LED Izq | GPIO | 00 | OUT |
| P0.8 | 0 | LED Abajo | GPIO | 00 | OUT |
| P0.9 | 0 | LED Arriba | GPIO | 00 | OUT |
| P0.23 | 0 | ADC Eje X | ADC | 01 | IN |
| P0.24 | 0 | ADC Eje Y | ADC | 01 | IN |
| P0.26 | 0 | DAC Out | DAC | 10 | OUT |
| P0.27 | 0 | I2C SDA | I2C | 01 | OPEN-DRAIN |
| P0.28 | 0 | I2C SCL | I2C | 01 | OPEN-DRAIN |
| P2.10 | 2 | Botón Joystick | EINT3 | 00 | IN |

---

## 🔄 **DIAGRAMA DE FLUJO - INTERRUPCIONES**

```
┌─────────────────────────────────────────────────────┐
│           SISTIO DE INTERRUPCIONES                  │
├─────────────────────────────────────────────────────┤
│                                                      │
│  TIMER2_IRQHandler (20 Hz)                          │
│  ├─ Actualiza juego (joystick, lógica)             │
│  ├─ Redibuja pantalla LCD                          │
│  └─ Genera bandera_tick_juego                       │
│                                                      │
│  TIMER3_IRQHandler (20 Hz)                          │
│  ├─ Sincroniza movimiento de la serpiente          │
│  └─ Actualiza tick_flag para el juego              │
│                                                      │
│  GPDMA_IRQHandler                                   │
│  ├─ Canal 0: UART0 DMA completo (Bluetooth)        │
│  │  └─ Procesa comandos Bluetooth recibidos        │
│  └─ Canal 1: DAC DMA completo (Melodías)           │
│     └─ Reinicia buffer de notas (loop)             │
│                                                      │
│  EINT3_IRQHandler                                   │
│  ├─ P2.10: Botón joystick presionado               │
│  └─ Alterna estado de pausa/menú                    │
│                                                      │
│  UART0_IRQHandler (DESHABILITADO)                   │
│  └─ RX manejado por DMA automáticamente            │
│                                                      │
└─────────────────────────────────────────────────────┘
```

---

## 🎯 **MAPEO DE RECURSOS**

### Recursos Utilizados
```
✅ Timer0     - Melodías (precisión)
✅ Timer1     - Melodías (respaldo)
✅ Timer2     - Tick principal (Dino Game)
✅ Timer3     - Tick del juego Snake

✅ ADC0 (Canal 0) - Joystick X
✅ ADC0 (Canal 1) - Joystick Y
⭕ ADC0 (Canales 2-7) - Disponibles

✅ DMA Canal 0 - Bluetooth RX
✅ DMA Canal 1 - DAC/Melodías
⭕ DMA (Canales 2-7) - Disponibles

✅ UART0 - Bluetooth
⭕ UART1-3 - Disponibles

✅ I2C0 - LCD
⭕ I2C1-2 - Disponibles

✅ DAC0 - Melodías
⭕ DAC1 - Disponible

✅ 5 LEDs GPIO (P0.0, P0.6-9)
✅ 2 Botones GPIO (P0.4, P2.10)
⭕ Pines GPIO restantes - Disponibles
```

---

## 📌 **NOTAS IMPORTANTES**

1. **Timer2** es crítico: controla la frecuencia de actualización del juego (20 Hz = 50 ms)
2. **Timer3** sincroniza el movimiento de la serpiente en el juego Snake
3. **DMA** es no-bloqueante: permite que el CPU siga ejecutando mientras se transfieren datos
4. **ADC** usa promediado de 4 muestras + filtro de zona muerta para reducir ruido
5. **I2C** es bloqueante (polling): bajo tráfico, no requiere DMA
6. **Bluetooth** recibe automáticamente via DMA sin interferir con el juego
7. **Melodías** se generan en paralelo sin bloquear el juego
