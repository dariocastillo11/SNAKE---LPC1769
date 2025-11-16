# 🎮 Sistema de Menú Multi-Juegos

## Resumen

Se ha implementado un sistema completo de menú con navegación por joystick que permite seleccionar entre dos juegos:
1. **Dino Chrome** (ya existente)
2. **Snake** (nuevo)

---

## 📁 Archivos Nuevos Creados

### Headers (`include/`)
- **`menu_juegos.h`** - Sistema de menú con navegación
- **`snake_game.h`** - Juego Snake

### Implementaciones (`src/`)
- **`menu_juegos.c`** - Lógica del menú con navegación por joystick
- **`snake_game.c`** - Juego Snake completo con Timer3

### Archivos Modificados
- **`main.c`** - Integración del menú y máquina de estados

---

## 🎮 Controles del Menú

### Navegación
- **Joystick ARRIBA**: Subir en el menú
- **Joystick ABAJO**: Bajar en el menú
- **Botón del Joystick**: Seleccionar juego

### Indicador Visual
- **`>`** Puntero que indica la opción seleccionada

---

## 🐍 Controles del Juego Snake

### Movimiento
- **Joystick ARRIBA**: Mover serpiente hacia arriba
- **Joystick ABAJO**: Mover serpiente hacia abajo
- **Joystick IZQUIERDA**: Mover serpiente hacia la izquierda
- **Joystick DERECHA**: Mover serpiente hacia la derecha

### Pausa
- **Botón del Joystick**: Pausar/Reanudar juego

### Objetivo
- Comer la comida (`*`) para crecer
- Evitar chocar con las paredes o con tu propio cuerpo
- La velocidad aumenta cada 5 comidas

### Game Over
- Al terminar, se muestra la puntuación
- Presiona el **Botón** para volver al menú

---

## 🦖 Controles del Juego Dino


---

## 🔧 Recursos del Sistema

### Timers Utilizados

| Timer | Módulo | Uso |
|-------|--------|-----|
| TIMER0 | `melodias_dac.c` | Generación de audio DAC |
| TIMER1 | `melodias_dac.c` | Contador de tiempo melodías |
| TIMER2 | `dino_game.c` | Motor del juego Dino (50ms ticks) |
| TIMER3 | `snake_game.c` | Motor del juego Snake (50ms ticks) |

### GPIO y Periféricos

| Periférico | Pines | Uso |
|------------|-------|-----|
| ADC | P0.23, P0.24 | Ejes X/Y del joystick |
| GPIO | P0.0, P0.6-P0.9 | LEDs indicadores de dirección |
| GPIO | P2.10 | Botón del joystick (EINT3) |
| I2C0 | P0.27, P0.28 | LCD I2C |
| DAC | P0.26 | Salida de audio |

---



---

## 🎯 Flujo de Usuario

### Al Encender
1. Aparece el **menú de selección**:
   ```
   SELECCIONA JUEGO
   > 1. DINO CHROME
     2. SNAKE
   Arriba/Abajo/Boton
   ```

2. Usa el **joystick** para mover el puntero `>`

3. Presiona el **botón** para seleccionar

### Durante el Juego
- El juego seleccionado se ejecuta normalmente
- Los **LEDs del joystick** siguen indicando direcciones

### Al Terminar (Snake)
- Pantalla de Game Over con puntuación
- Presiona **botón** para volver al menú

### Al Terminar (Dino)
- **TODO**: Agregar detección de game over y opción de volver al menú
- Actualmente el juego se reinicia automáticamente

---

## ⚙️ Configuración y Ajustes

### Velocidad del Snake

En `snake_game.c`, línea ~23:
```c
#define SNAKE_SPEED_TICKS 6  // Cambiar para ajustar velocidad inicial
```
- Valores más bajos = más rápido
- Valores más altos = más lento

### Debounce del Menú

En `menu_juegos.c`, línea ~15:
```c
#define MENU_DEBOUNCE_TICKS 15  // Ajustar sensibilidad de navegación
```

### Tamaño Máximo de la Serpiente

En `snake_game.c`, línea ~21:
```c
#define SNAKE_MAX_LENGTH 50  // Máximo de segmentos
```

---

