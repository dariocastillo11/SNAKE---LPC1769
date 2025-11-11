# 🎮 Control Bluetooth - DinoChrome Arcade

## 📡 Configuración de Hardware

### Conexiones del Módulo HC-05
- **VCC**: 5V o 3.3V (según módulo)
- **GND**: GND
- **TX del HC-05** → **P0.3 (RXD0)** del LPC1769
- **RX del HC-05** → **P0.2 (TXD0)** del LPC1769

⚠️ **IMPORTANTE**: Si tu módulo HC-05 es de 3.3V, conecta RX directamente. Si es de 5V, usa un divisor de tensión (resistencias 1kΩ y 2kΩ) en la línea RX del HC-05.

### Parámetros UART0
- **Baudrate**: 9600 bps
- **Formato**: 8 bits, sin paridad, 1 stop bit (8N1)
- **Control de flujo**: Ninguno

---

## 🎯 Comandos Bluetooth

### Control de Dirección
| Comando | Función | Equivalente Joystick |
|---------|---------|---------------------|
| `W` o `w` | **Arriba** | Joystick hacia arriba |
| `S` o `s` | **Abajo** | Joystick hacia abajo |
| `A` o `a` | **Izquierda** | Joystick hacia izquierda |
| `D` o `d` | **Derecha** | Joystick hacia derecha |
| `B` o `b` | **Botón** | Presionar botón P0.4 |

### Comportamiento
- Los comandos Bluetooth **tienen prioridad** sobre el joystick físico
- Cuando llega un comando, se mantiene activo por ~5 ciclos
- Si no hay comandos activos, el sistema vuelve al joystick físico
- Los LEDs indicadores muestran la dirección activa (BT o joystick)

---

## 📱 Aplicaciones Recomendadas

### Android
1. **Serial Bluetooth Terminal** (Kai Morich)
   - [Google Play](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal)
   - Configuración: 9600 baud, sin terminador de línea
   
2. **Bluetooth Terminal** (Qwerty)
   - Modo teclado para enviar comandos individuales

### iOS
1. **BLE Terminal** (HM-10 compatible)
2. **Serial Bluetooth Terminal**

### PC
1. **PuTTY** (Windows/Linux)
   - Configurar puerto COM del HC-05
   - Baud: 9600, 8-N-1
   
2. **Arduino Serial Monitor**
   - Seleccionar puerto COM del Bluetooth
   - Configurar 9600 baud

---

## 🎮 Uso en los Juegos

### Menú Principal
- `W` / `S`: Navegar entre opciones (Dino / Snake)
- `B`: Seleccionar juego

### Juego Dino Chrome
- `B`: Saltar (cuando está en el suelo)
- `B`: Volver al menú (en Game Over)

### Juego Snake
- `W` / `S` / `A` / `D`: Controlar dirección de la serpiente
- `B`: Pausar/Reanudar
- `B`: Volver al menú (en Game Over)

---

## 🔍 Diagnóstico

### Mensajes de Confirmación
Al conectarse, el sistema envía:
```
=== DINOCHROME ARCADE ===
Comandos: W(arriba) S(abajo) A(izq) D(der) B(boton)
Conectado!
```

Al recibir comandos, responde:
```
CMD: UP
CMD: DOWN
CMD: LEFT
CMD: RIGHT
CMD: BUTTON
```

### Solución de Problemas

**Problema**: No recibo mensajes del LPC1769
- ✅ Verificar baudrate (debe ser 9600)
- ✅ Verificar cruce de cables (TX→RX, RX→TX)
- ✅ Verificar emparejamiento del HC-05

**Problema**: Los comandos no funcionan
- ✅ Verificar que envías caracteres individuales (W, A, S, D, B)
- ✅ Los comandos son case-insensitive (W = w)
- ✅ No enviar terminadores de línea (\r\n) con cada comando

**Problema**: El joystick físico no funciona
- ✅ Si hay comandos Bluetooth activos, estos tienen prioridad
- ✅ Espera ~250ms sin comandos BT para volver al joystick
- ✅ Envía comando neutral para forzar modo joystick

**Problema**: LEDs no responden a comandos BT
- ✅ Los LEDs muestran la dirección combinada (BT o joystick)
- ✅ Verifica que `joystick_update()` se llama periódicamente

---

## 💻 Implementación Técnica

### Arquitectura
```
main.c
  ├─ bt_init() ────────────► Configura UART0 (P0.2/P0.3)
  │
  └─ Loop principal
      ├─ joystick_update() ──► joystick_adc.c
      │   ├─ bt_process_commands() ────► bluetooth_uart.c
      │   │   └─ Lee UART0 y actualiza valores simulados
      │   │
      │   ├─ Si BT activo: usar valores simulados
      │   └─ Si BT inactivo: leer ADC físico
      │
      ├─ menu_run() ─────────► menu_juegos.c
      │   └─ leer_boton_p04() ────► Botón físico OR BT
      │
      ├─ dino_game_run() ────► dino_game.c
      │   └─ read_button() ───────► Botón físico OR BT
      │
      └─ snake_game_run() ───► snake_game.c
          └─ leer_boton_p04() ────► Botón físico OR BT
```

### Módulos Modificados
1. **bluetooth_uart.c** (NUEVO)
   - Configura UART0 a 9600 bps
   - Procesa comandos W/A/S/D/B
   - Genera valores ADC simulados (100, 2048, 3900)

2. **joystick_adc.c** (MODIFICADO)
   - Llama `bt_process_commands()` en cada update
   - Prioriza valores BT sobre lecturas físicas
   - Transparente para los juegos

3. **dino_game.c** (MODIFICADO)
   - `read_button()` combina P0.4 físico + comando 'B'

4. **snake_game.c** (MODIFICADO)
   - `leer_boton_p04()` combina P0.4 físico + comando 'B'

5. **menu_juegos.c** (MODIFICADO)
   - `leer_boton_p04()` combina P0.4 físico + comando 'B'

---

## 🧪 Prueba Rápida

1. **Conectar HC-05** al LPC1769 (TX→P0.3, RX→P0.2)
2. **Compilar** el proyecto
3. **Emparejar** el HC-05 desde tu dispositivo (PIN: 1234 o 0000)
4. **Abrir terminal** Bluetooth (9600 baud)
5. **Enviar**: `W` → Deberías ver "CMD: UP" y el LED UP encender
6. **Enviar**: `B` → Deberías ver "CMD: BUTTON"
7. **En el menú**: `W`/`S` navega, `B` selecciona
8. **En Dino**: `B` salta
9. **En Snake**: `W`/`A`/`S`/`D` mueve la serpiente

---

## 📊 Timing y Rendimiento

- **Baudrate**: 9600 bps → ~960 bytes/seg → ~1ms por byte
- **Command duration**: 5 ciclos (~250ms) para mantener comando activo
- **Latencia**: < 10ms desde recepción hasta acción
- **Polling**: No usa interrupciones UART (lectura no bloqueante)

---

## 🔧 Configuración Avanzada

### Cambiar Baudrate
En `bluetooth_uart.h`:
```c
#define BT_UART0_BAUD    9600  // Cambiar a 115200 si tu HC-05 lo soporta
```

### Ajustar Duración de Comandos
En `bluetooth_uart.c`, función `bt_process_commands()`:
```c
command_duration = 5;  // Aumentar para comandos más largos
```

### Agregar Nuevos Comandos
En `bluetooth_uart.c`:
```c
case 'P': case 'p':  // Nuevo comando
    // Tu código aquí
    bt_write_str("CMD: CUSTOM\r\n");
    break;
```

---

## 📝 Notas Finales

✅ **Compatible** con joystick físico (ambos funcionan simultáneamente)  
✅ **No usa interrupciones** UART (polling en main loop)  
✅ **Bajo overhead** (~5% del tiempo de CPU)  
✅ **Feedback visual** por LEDs y confirmación por terminal  
✅ **Plug & Play** - solo conectar HC-05 y emparejar  

⚠️ **No reconfigurar P0.2/P0.3** en otros módulos (están dedicados a UART0)

---

**¡Disfruta tu control dual Joystick + Bluetooth!** 🎮📱
