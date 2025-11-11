# 🎮 Control Bluetooth - DinoChrome Arcade

## 📡 Configuración de Hardware

### Conexiones del Módulo HC-05
- **VCC**: 5V o 3.3V (según módulo)
- **GND**: GND
- **TX del HC-05** → **P0.3 (RXD0)** del LPC1769
- **RX del HC-05** → **P0.2 (TXD0)** del LPC1769



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

---

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


---


