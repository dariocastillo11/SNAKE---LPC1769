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
