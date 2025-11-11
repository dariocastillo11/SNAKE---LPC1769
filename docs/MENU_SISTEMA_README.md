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

(Sin cambios respecto a la versión anterior)

---

## 🏗️ Arquitectura del Sistema

```
INICIO
  │
  ├─ SystemInit()
  ├─ Configurar periféricos (I2C, DAC, ADC, GPIO)
  ├─ menu_init() → Mostrar menú
  │
  └─ LOOP PRINCIPAL
      │
      ├─ Estado: EN MENÚ
      │   ├─ menu_run() → Procesar entrada joystick
      │   └─ Si se selecciona juego → Cambiar estado
      │
      ├─ Estado: JUEGO ACTIVO (Dino o Snake)
      │   ├─ Inicializar juego (si no está inicializado)
      │   ├─ juego_run() → Ejecutar lógica del juego
      │   └─ Si termina → Volver al menú
      │
      ├─ melodias_actualizar() → Audio (siempre activo)
      └─ joystick_update() → LEDs indicadores (siempre activo)
```

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

## 🚀 Compilación y Uso

### 1. Copiar archivos al proyecto `tp`

```powershell
cd "C:\Users\dario\OneDrive\Documentos\tp"

# Copiar headers
Copy-Item "C:\Users\dario\Downloads\DinoChrome\DinoChrome\include\menu_juegos.h" ".\include\" -Force
Copy-Item "C:\Users\dario\Downloads\DinoChrome\DinoChrome\include\snake_game.h" ".\include\" -Force

# Copiar implementaciones
Copy-Item "C:\Users\dario\Downloads\DinoChrome\DinoChrome\src\menu_juegos.c" ".\src\" -Force
Copy-Item "C:\Users\dario\Downloads\DinoChrome\DinoChrome\src\snake_game.c" ".\src\" -Force
Copy-Item "C:\Users\dario\Downloads\DinoChrome\DinoChrome\src\main.c" ".\src\" -Force

# Recompilar
make -r -j8 all
```

### 2. Asegúrate de que los nuevos archivos se compilen

Si usas Makefile manual, agrega:
```makefile
SRCS += src/menu_juegos.c
SRCS += src/snake_game.c
```

Si usas MCUXpresso IDE, el proyecto detectará automáticamente los nuevos archivos.

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

## 🐛 Solución de Problemas

### El menú no aparece al inicio
- Verifica que `menu_init()` se llame después de `lcd_init()`
- Comprueba que el LCD funcione correctamente

### Snake no se mueve
- Verifica que TIMER3 no esté siendo usado por otro módulo
- Revisa que `TIMER3_IRQHandler` esté correctamente implementado

### El juego no responde al joystick
- Confirma que `joystick_init()` se ejecutó correctamente
- Verifica las conexiones de hardware (P0.23, P0.24)

### No vuelve al menú después de Snake
- El juego detecta game over automáticamente
- Presiona el botón del joystick en la pantalla de Game Over

---

## 🎨 Mejoras Futuras Sugeridas

### Para el Sistema de Menú
- [ ] Agregar más juegos
- [ ] Animaciones en el menú
- [ ] Música de fondo en el menú
- [ ] Guardar puntuaciones máximas en EEPROM

### Para el Juego Snake
- [ ] Niveles con obstáculos
- [ ] Diferentes tipos de comida (bonus)
- [ ] Modo multijugador
- [ ] Efectos de sonido (usando DAC)

### Para el Juego Dino
- [ ] Agregar detección de game over y volver al menú
- [ ] Función `dino_game_is_over()` para integración con el menú
- [ ] Opción de reintentar o volver al menú

---

## 📊 Memoria y Performance

### Uso de RAM
- **Menu**: ~100 bytes (opciones y estado)
- **Snake**: ~250 bytes (array de serpiente + buffer LCD)
- **Total adicional**: ~350 bytes

### Uso de Flash
- **Menu**: ~2KB
- **Snake**: ~4KB
- **Total adicional**: ~6KB

### CPU
- Ambos juegos usan ticks de 50ms (20 Hz)
- Carga de CPU: <5% en bucle principal

---

## ✅ Testing

### Checklist de Pruebas

- [ ] El menú aparece al encender
- [ ] El joystick navega correctamente (arriba/abajo)
- [ ] El puntero `>` se mueve
- [ ] Al presionar botón se selecciona el juego
- [ ] Dino Chrome funciona correctamente
- [ ] Snake funciona correctamente
- [ ] Snake detecta colisiones
- [ ] Snake crece al comer
- [ ] La puntuación se muestra correctamente
- [ ] Volver al menú desde Snake funciona
- [ ] Los LEDs del joystick siguen funcionando

---

**Fecha de implementación**: Noviembre 2025  
**Versión**: 2.0  
**Plataforma**: LPC1769, MCUXpresso IDE
