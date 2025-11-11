# Integración del Módulo Joystick ADC en DinoChrome

## Resumen de Cambios

Se ha integrado exitosamente el módulo de control de joystick analógico mediante ADC en el proyecto DinoChrome, manteniendo toda la funcionalidad existente del juego.

## Archivos Creados

### 1. `include/joystick_adc.h`
Header del módulo con prototipos de funciones públicas:
- `joystick_init()` - Inicializa ADC, GPIO y interrupciones
- `joystick_update()` - Actualiza lecturas y LEDs (llamar en el loop principal)
- `joystick_leer_adc(canal)` - Lee valor ADC de un canal
- `joystick_boton_presionado()` - Retorna estado del botón

### 2. `src/joystick_adc.c`
Implementación completa del módulo con:
- Configuración del ADC para canales 0 y 1
- Control de 5 LEDs indicadores de dirección
- Manejo de interrupción del botón
- Detección de direcciones basada en umbrales

## Archivos Modificados

### `src/main.c`
- **Línea 14**: Agregado `#include "joystick_adc.h"`
- **Línea 34**: Agregado `joystick_init();` después de `melodias_init()`
- **Línea 48**: Agregado `joystick_update();` en el loop principal

## Configuración de Hardware

### Joystick Analógico
- **P0.23 (AD0.0)**: Eje X del joystick
- **P0.24 (AD0.1)**: Eje Y del joystick
- **P2.10**: Botón del joystick (interrupción en flanco de bajada)

### LEDs Indicadores
- **P0.9**: LED ARRIBA (joystick Y < 100)
- **P0.8**: LED ABAJO (joystick Y > 4000)
- **P0.7**: LED IZQUIERDA (joystick X < 100)
- **P0.6**: LED DERECHA (joystick X > 4000)
- **P0.0**: LED CENTRO/BOTÓN (botón presionado)

### Configuración de Lógica de LEDs
Los LEDs están configurados como **activo alto** (LED_ACTIVE_LOW = 0):
- Escribir 1 → LED encendido
- Escribir 0 → LED apagado

Si tu hardware usa lógica inversa, cambia `LED_ACTIVE_LOW` a `1` en `joystick_adc.c` línea 28.

## Umbrales del ADC

Los valores del ADC son de 12 bits (0-4095):

| Posición | Eje X | Eje Y | LED Encendido |
|----------|-------|-------|---------------|
| Centro/Reposo | ~3500 | ~3500 | Ninguno |
| Arriba | ~3500 | ~7 | P0.9 |
| Abajo | ~3500 | ~4095 | P0.8 |
| Izquierda | ~6 | ~3350 | P0.7 |
| Derecha | ~4095 | ~3340 | P0.6 |
| Botón presionado | - | - | P0.0 |

### Ajuste de Umbrales
Si necesitas ajustar la sensibilidad, modifica en `joystick_adc.c`:
```c
#define ADC_MIN         100     // Umbral inferior (default: 100)
#define ADC_MAX         4000    // Umbral superior (default: 4000)
```

## Recursos del Sistema Utilizados

### Periféricos
- **ADC**: Canales 0 y 1 configurados
- **GPIO Port 0**: Pines 0, 6, 7, 8, 9 (salidas para LEDs)
- **GPIO Port 2**: Pin 10 (entrada con interrupción)
- **NVIC**: Interrupción EINT3 habilitada

### Interrupciones
- **EINT3_IRQHandler**: Maneja el botón del joystick en P2.10

### Timers (NO utilizados por el joystick)
El módulo joystick NO usa timers, por lo que NO hay conflictos con:
- TIMER0: Usado por `melodias_dac.c` (generación de audio)
- TIMER1: Usado por `melodias_dac.c` (contador de tiempo)
- TIMER2: Usado por `dino_game.c` (motor del juego)

## Compatibilidad con el Proyecto Existente

✅ **SIN CONFLICTOS**:
- No hay solapamiento de pines con I2C, LCD, DAC o botón del juego
- No hay conflicto de timers
- El handler EINT3 no estaba implementado previamente
- La función `joystick_update()` es no bloqueante

⚠️ **ADVERTENCIA**: El pin **P0.4** usado originalmente para el botón del juego DinoChrome permanece igual. El botón del joystick (P2.10) es independiente.

## Compilación

Para compilar el proyecto actualizado:

1. Asegúrate de que `joystick_adc.c` esté incluido en tu Makefile o configuración del IDE
2. Los headers de CMSIS deben estar en el include path
3. Compila normalmente con tu toolchain ARM GCC

### En MCUXpresso IDE:
1. Proyecto → Properties → C/C++ Build → Settings
2. Verifica que `src/joystick_adc.c` esté en la lista de fuentes
3. Build → Clean → Build Project

## Funcionamiento

Al ejecutar el firmware:
1. El sistema inicializa todos los periféricos (I2C, ADC, DAC, LCD, GPIO)
2. El juego DinoChrome arranca automáticamente en el LCD
3. **Simultáneamente**, el joystick lee constantemente las posiciones X/Y
4. Los LEDs se encienden según la dirección del joystick:
   - Un solo LED encendido a la vez
   - Prioridad: Botón > Eje Y > Eje X
   - Si el joystick está en centro, todos los LEDs apagados

## Resolución del Error Original de Compilación

**Problema original**: El proyecto `tp` en OneDrive tenía definiciones múltiples de:
- `main()` en `dac_buffer.c` y `main.c`
- `TIMER0_IRQHandler()` en `dac_buffer.c` y `melodias_dac.c`
- `DAC_Init()` en `dac_buffer.c` y librería CMSIS

**Solución recomendada** (para el proyecto `tp`):
```powershell
# Desde C:\Users\dario\OneDrive\Documentos\tp
New-Item -Path .\backup_src -ItemType Directory -ErrorAction SilentlyContinue
Move-Item -Path .\src\dac_buffer.c -Destination .\backup_src\dac_buffer.c
make -r -j8 all
```

Este módulo en DinoChrome evita esos errores al no tener definiciones duplicadas.

## Testing

Para probar el módulo:
1. Mueve el joystick hacia arriba → LED P0.9 enciende
2. Mueve el joystick hacia abajo → LED P0.8 enciende
3. Mueve el joystick hacia izquierda → LED P0.7 enciende
4. Mueve el joystick hacia derecha → LED P0.6 enciende
5. Presiona el botón del joystick → LED P0.0 enciende (se mantiene hasta presionar de nuevo)
6. El juego DinoChrome debe seguir funcionando normalmente en el LCD

## Próximos Pasos (Opcional)

Si deseas usar el joystick para controlar el juego DinoChrome:
1. Modifica `dino_game.c` para leer `joystick_boton_presionado()` o valores ADC
2. Reemplaza o complementa el botón actual (P0.4) con detección de movimiento del joystick
3. Ejemplo: saltar al mover hacia arriba, agacharse al mover hacia abajo

---

**Fecha de integración**: Noviembre 2025  
**Versión**: 1.0  
**Compatibilidad**: LPC1769, MCUXpresso IDE, ARM GCC
