# 🎮 DinoChrome Arcade - Consola Emulada en LPC1769

# prototipo:
🔗 [Ver prototipo en Google Drive](https://drive.google.com/file/d/16qOqM-h5jt_PgI9QWdnAs-lrSv28flxA/view?usp=sharing)
---

### ▶️ Ver demo en YouTube
<p align="center">
  <a href="https://youtu.be/1SkBKUzI0do">
    <img src="https://img.youtube.com/vi/1SkBKUzI0do/0.jpg" width="250" />
  </a>
  <a href="https://youtu.be/fvH5eQaHKKo?si=YbP_Yb0W7B7Uc3JL">
    <img src="https://img.youtube.com/vi/vmjX52HBIpY/0.jpg" width="250" />
  </a>
  <a href="https://youtube.com/shorts/Tz25_OoKHCY?feature=share">
    <img src="https://img.youtube.com/vi/fvH5eQaHKKo/0.jpg" width="250" />
  </a>
</p>
---

### ▶️ Imagen version terminada
<p align="center">
  <img src="https://github.com/user-attachments/assets/474bd065-94ee-44f2-87e9-4ae8bf61a03d" width="200"/>
  <img src="https://github.com/user-attachments/assets/b58cf506-cb85-466f-9cfc-1a206587a7a8" width="200"/>
</p>

[![Estado](https://img.shields.io/badge/estado-Activo-brightgreen)]()
[![Lenguaje](https://img.shields.io/badge/lenguaje-C-blue)]()
[![Hardware](https://img.shields.io/badge/hardware-ARM%20LPC1769-lightgrey)]()


Una plataforma de juegos arcade clásicos implementada en un microcontrolador **LPC1769** con soporte para control vía **joystick analógico** y **Bluetooth**. Incluye dos juegos clásicos: **Snake** y **Dino Chrome**, con audio digital y visualización en pantalla LCD.

---

## ✨ Características Principales

### 🎮 Juegos Incluidos
- **🐍 Snake Clásico:** Navega por la pantalla comiendo comida y evitando chocarte
- **🦖 Dino Chrome:** Esquiva obstáculos en un mundo desértico arcade

### 🎵 Sonido
- **Audio Digital DAC:** Generación de melodías en tiempo real con Timer0
- **Aceleración DMA:** Transferencia automática de muestras de audio
- **Múltiples Melodías:** Diferentes temas para cada pantalla/juego

### 🕹️ Controles Duales
- **Joystick Analógico:** Control físico en tiempo real (ADC de 12 bits)
- **Bluetooth HC-05:** Control inalámbrico desde aplicación móvil
- **Prioridad Automática:** El joystick físico se usa cuando no hay comandos Bluetooth

### 💻 Pantalla LCD
- **Pantalla I2C LCD 16x2:** Visualización clara del juego y estado
- **Interfaz de Menú:** Selección elegante entre juegos

### ⚡ Optimizaciones de Hardware
- **DMA (Direct Memory Access):** Aceleración de comunicación UART y DAC
- **Multitarea:** Manejo simultáneo de joystick, Bluetooth, audio y lógica de juego
- **Interrupciones por Timer:** Sincronización precisa de eventos

---

## 🏗️ Arquitectura del Sistema

### Hardware Utilizado
```
┌─────────────────────────────────────────┐
│       LPC1769 ARM Cortex-M3             │
├─────────────────────────────────────────┤
│  - UART0: Comunicación Bluetooth (P0.2/3)
│  - ADC:   Joystick analógico (P0.23/24)
│  - DAC:   Audio (P0.26 - AOUT)          │
│  - I2C:   Pantalla LCD (P0.0/1)         │
│  - GPIO:  LEDs indicadores (P0.0,6-9)   │
│  - Timer0/1: Sincronización y audio     │
│  - DMA:   Aceleración RX y DAC          │
└─────────────────────────────────────────┘
```

### Periféricos Conectados
- **HC-05 Bluetooth Module** (UART0 @ 9600 bps)
- **Joystick Analógico** (2 ejes X/Y + botón)
- **LCD 16x2 con interfaz I2C** (dirección 0x27)
- **LEDs indicadores** (5 unidades: arriba/abajo/izq/der/botón)

---

## 📦 Estructura del Proyecto

```
SNAKE---LPC1769/
├── include/                          # Headers públicos
│   ├── bluetooth_uart.h             # Driver Bluetooth con DMA
│   ├── melodias_dac.h               # Sistema de melodías
│   ├── joystick_adc.h               # Lectura joystick (ADC)
│   ├── lcd_i2c.h                    # Control pantalla LCD
│   ├── snake_game.h                 # Lógica juego Snake
│   ├── dino_game.h                  # Lógica juego Dino
│   └── menu_juegos.h                # Sistema de menú
│
├── src/                              # Implementaciones
│   ├── bluetooth_uart.c             # [CON DMA] RX automático (canal 0)
│   ├── melodias_dac.c               # [CON DMA] Transferencia samples (canal 1)
│   ├── dma_handlers.c               # [NUEVO] Manejador centralizado DMA
│   ├── joystick_adc.c
│   ├── lcd_i2c.c
│   ├── snake_game.c
│   ├── dino_game.c
│   ├── menu_juegos.c
│   └── main.c                       # Punto de entrada
│
├── CMSISv2p00_LPC17xx/              # Drivers del fabricante
│   ├── inc/                         # Headers CMSIS (core, LPC17xx)
│   ├── Drivers/                     # Drivers periféricos
│   │   └── src/lpc17xx_gpdma.c      # Controlador DMA
│   └── src/                         # Implementación CMSIS
│
├── docs/                             # Documentación
│   ├── BLUETOOTH_README.md
│   ├── JOYSTICK_README.md
│   ├── MENU_SISTEMA_README.md
│   └── I2C.md
│
├── DMA_IMPLEMENTATION.md             # Documentación técnica DMA
├── DMA_BUILD_NOTES.txt               # Notas de compilación
└── README.md                         # Este archivo
```

---

## 🚀 Guía de Inicio Rápido

### Requisitos
- **Compilador ARM:** `arm-none-eabi-gcc`
- **Hardware:** LPC1769 con periféricos mencionados
- **Entorno:** MCUXpresso IDE





### Ejecución
1. Alimentar el LPC1769
2. Seleccionar juego en pantalla LCD con joystick
3. Controlar con **joystick físico** O **comandos Bluetooth**

### Comandos Bluetooth (HC-05 @ 9600 bps)
```
W/w → Arriba
S/s → Abajo
A/a → Izquierda
D/d → Derecha
B/b → Botón
P/p → Pausa/Reintentar
```

---

## 🎯 Funciones Públicas Principales

### Bluetooth
```c
void bt_inicializar(void);                    // Iniciar UART0 + DMA
int bt_leer_caracter_no_bloqueante(void);    // Leer desde buffer DMA
void bt_procesar_comandos(void);              // Procesar comandos Bluetooth
uint16_t bt_obtener_x_simulado(void);        // Eje X simulado (0-4095)
uint16_t bt_obtener_y_simulado(void);        // Eje Y simulado (0-4095)
uint8_t bt_obtener_comando_boton(void);      // Estado botón (0/1)
void bt_escribir_cadena(const char *cadena); // Enviar texto
```

### Joystick
```c
void joystick_inicializar(void);              // Iniciar ADC y LEDs
void joystick_actualizar(void);               // Leer joystick y actualizar
uint16_t joystick_leer_adc(uint8_t canal);   // Lectura ADC cruda
uint8_t joystick_boton_presionado(void);     // Estado botón físico
```

### Melodías
```c
void melodias_inicializar(void);              // Iniciar DAC + Timer + DMA
void melodias_iniciar(const Nota *melodia);  // Reproducir melodía una vez
void melodias_iniciar_loop(const Nota *m);   // Reproducir en loop
void melodias_detener(void);                  // Parar reproducción
void melodias_actualizar(void);               // Avanzar notas (llamar en main loop)
uint8_t melodias_esta_sonando(void);         // ¿Reproduciendo?
```

### Pantalla LCD
```c
void lcd_inicializar(void);                   // Iniciar I2C y LCD
void lcd_escribir_linea(uint8_t linea, const char *texto);
void lcd_borrar_pantalla(void);
void lcd_cursor_inicio(void);
```

---

## 🔧 Implementación DMA

El proyecto incluye **soporte DMA completamente funcional** en dos módulos:

### 📡 Bluetooth RX (Canal DMA 0)
- **Tipo:** P2M (Periférico → Memoria)
- **Conexión:** GPDMA_UART0_Rx
- **Buffer:** 128 bytes ring buffer
- **Ventaja:** Recepción automática sin polling

### 🎵 Melodías DAC (Canal DMA 1)
- **Tipo:** M2P (Memoria → Periférico)
- **Conexión:** GPDMA_DAC
- **Fuente:** Tabla triangular en RAM (64 muestras)
- **Ventaja:** Transferencia automática de samples

### Manejador Centralizado (`dma_handlers.c`)
```c
void GPDMA_IRQHandler(void);  // ISR único que despacha ambos canales
  ├─ bt_dma_on_transfer_complete()      // Callback Bluetooth
  └─ melodias_dma_on_transfer_complete() // Callback Melodías
```



---

## 📊 Características Avanzadas

### Prioridad de Entrada
1. **Bluetooth activo** → Usar valores simulados del joystick
2. **Joystick en centro** → Leer ADC del joystick físico
3. **Automático** → Cambio transparente sin intervención

### LEDs Indicadores
| Pin  | Función |
|------|---------|
| P0.9 | Arriba |
| P0.8 | Abajo |
| P0.7 | Izquierda |
| P0.6 | Derecha |
| P0.0 | Botón/Centro |

### Melodías Predefinidas
- `melodia_mario` - Tema clásico Nintendo
- `melodia_tetris` - Tema Tetris
- `melodia_nokia` - Timbre Nokia retro
- `melodia_game_over` - Sonido derrota
- `melodia_salto` - Efecto salto
- `melodia_fondo` - Música ambiente prolongada

---

## 🔌 Configuración de Pines

### UART0 (Bluetooth)
| Pin | Función | Config |
|-----|---------|--------|
| P0.2 | TXD0 | PINSEL=01 |
| P0.3 | RXD0 | PINSEL=01 |

### ADC (Joystick)
| Pin | Función | Canal |
|-----|---------|-------|
| P0.23 | VRx | AD0.0 |
| P0.24 | VRy | AD0.1 |
| P2.10 | Botón | GPIO INT |

### DAC (Audio)
| Pin | Función |
|-----|---------|
| P0.26 | AOUT (DAC) |

### I2C (LCD)
| Pin | Función |
|-----|---------|
| P0.27 | SCL |
| P0.28 | SDA |

### LEDs
| Pin | Función |
|-----|---------|
| P0.0 | Botón/Centro |
| P0.6 | Derecha |
| P0.7 | Izquierda |
| P0.8 | Abajo |
| P0.9 | Arriba |

---

## 📖 Documentación Adicional

- **`docs/BLUETOOTH_README.md`** - Detalles de integración HC-05
- **`docs/JOYSTICK_README.md`** - Calibración y uso del joystick
- **`docs/MENU_SISTEMA_README.md`** - Sistema de menú
- **`docs/I2C.md`** - Comunicación con LCD


---
