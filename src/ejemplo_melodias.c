/**
 * @file    ejemplo_melodias.c
 * @brief   Ejemplo de uso del sistema de generación de melodías con DAC
 * @details Este archivo muestra cómo integrar el generador de melodías
 *          en tu proyecto. Incluye ejemplos simples y avanzados.
 * @date    Octubre 2025
 */

#include "LPC17xx.h"
#include "DacMelodias.h"

/* ======================== EJEMPLO 1: USO BÁSICO =========================== */

/**
 * @brief Ejemplo básico: Inicializar y reproducir una melodía
 */
void ejemplo_basico(void) {
    // 1. Inicializar el sistema de audio (DAC + Timer1)
    DAC_Init_Melodia();
    
    // 2. Reproducir una melodía predefinida
    DAC_ReproducirMario();
    
    // 3. Pequeña pausa entre melodías
    delay_ms(1000);
    
    // 4. Reproducir otra melodía
    DAC_ReproducirTetris();
}

/* ====================== EJEMPLO 2: NOTAS INDIVIDUALES ==================== */

/**
 * @brief Ejemplo de generación de notas individuales
 */
void ejemplo_notas_individuales(void) {
    // Inicializar
    DAC_Init_Melodia();
    
    // Reproducir notas individuales (Do, Re, Mi, Fa, Sol, La, Si, Do)
    DAC_GenerarNota(DO_4, NEGRA);   // Do central, duración negra (500ms)
    DAC_GenerarNota(RE_4, NEGRA);   // Re
    DAC_GenerarNota(MI_4, NEGRA);   // Mi
    DAC_GenerarNota(FA_4, NEGRA);   // Fa
    DAC_GenerarNota(SOL_4, NEGRA);  // Sol
    DAC_GenerarNota(LA_4, NEGRA);   // La
    DAC_GenerarNota(SI_4, NEGRA);   // Si
    DAC_GenerarNota(DO_5, BLANCA);  // Do agudo, duración blanca (1000ms)
    
    // Nota personalizada: 440Hz (La) durante 2 segundos
    DAC_GenerarNota(440, 2000);
}

/* ====================== EJEMPLO 3: EFECTOS DE SONIDO ===================== */

/**
 * @brief Ejemplo de efectos de sonido para videojuegos
 */
void ejemplo_efectos_videojuego(void) {
    DAC_Init_Melodia();
    
    // Efecto de disparo láser
    DAC_EfectoLaser();
    delay_ms(500);
    
    // Efecto de victoria (cuando ganas)
    DAC_EfectoVictoria();
    delay_ms(500);
    
    // Efecto de game over (cuando pierdes)
    DAC_EfectoGameOver();
    delay_ms(500);
    
    // Efecto de sirena (alarma)
    DAC_EfectoSirena(3000);  // 3 segundos de sirena
}

/* ==================== EJEMPLO 4: MELODÍA PERSONALIZADA =================== */

/**
 * @brief Ejemplo de cómo crear tu propia melodía
 */
void ejemplo_melodia_personalizada(void) {
    DAC_Init_Melodia();
    
    // Crear tu propia secuencia de notas
    // Ejemplo: Melodía simple de "Estrellita ¿Dónde Estás?"
    
    DAC_GenerarNota(DO_4, NEGRA);    // Es-
    DAC_GenerarNota(DO_4, NEGRA);    // tre-
    DAC_GenerarNota(SOL_4, NEGRA);   // lli-
    DAC_GenerarNota(SOL_4, NEGRA);   // ta
    
    DAC_GenerarNota(LA_4, NEGRA);    // ¿dón-
    DAC_GenerarNota(LA_4, NEGRA);    // de
    DAC_GenerarNota(SOL_4, BLANCA);  // es-tás?
    
    DAC_GenerarNota(FA_4, NEGRA);    // Me
    DAC_GenerarNota(FA_4, NEGRA);    // pre-
    DAC_GenerarNota(MI_4, NEGRA);    // gun-
    DAC_GenerarNota(MI_4, NEGRA);    // to
    
    DAC_GenerarNota(RE_4, NEGRA);    // ¿quién
    DAC_GenerarNota(RE_4, NEGRA);    // se-
    DAC_GenerarNota(DO_4, BLANCA);   // rás?
}

/* =================== EJEMPLO 5: ONDA SENOIDAL (SUAVE) ==================== */

/**
 * @brief Ejemplo usando onda senoidal para sonido más suave
 */
void ejemplo_onda_senoidal(void) {
    DAC_Init_Melodia();
    
    // Las ondas senoidales suenan más suaves y menos "electrónicas"
    // que las ondas cuadradas
    
    // Nota con onda senoidal
    DAC_GenerarNota_Seno(LA_4, BLANCA);   // La 440Hz
    delay_ms(200);
    DAC_GenerarNota_Seno(DO_5, BLANCA);   // Do
    delay_ms(200);
    DAC_GenerarNota_Seno(MI_5, BLANCA);   // Mi
}

/* ================= EJEMPLO 6: INTEGRACIÓN EN MAIN ======================== */

/**
 * @brief Ejemplo de cómo integrar en tu función main()
 */
void ejemplo_main_integracion(void) {
    // En tu main.c, después de SystemInit():
    
    SystemInit();
    
    // Inicializar el sistema de audio
    DAC_Init_Melodia();
    
    // Reproducir melodía de inicio
    DAC_ReproducirMario();
    
    // Tu código principal...
    while(1) {
        // Aquí puedes llamar a las funciones de melodías
        // según eventos de tu aplicación
        
        // Ejemplo: si presionas un botón, reproduce sonido
        // if (boton_presionado) {
        //     DAC_EfectoLaser();
        // }
        
        // Ejemplo: si ganas el juego
        // if (juego_ganado) {
        //     DAC_EfectoVictoria();
        // }
    }
}

/* ================ EJEMPLO 7: TONOS DE FRECUENCIA LIBRE =================== */

/**
 * @brief Ejemplo de generación de tonos con frecuencias arbitrarias
 */
void ejemplo_tonos_libres(void) {
    DAC_Init_Melodia();
    
    // Puedes usar cualquier frecuencia entre 100Hz y 2000Hz
    
    DAC_TonoSimple(200, 500);   // Tono grave
    delay_ms(100);
    
    DAC_TonoSimple(400, 500);   // Tono medio-grave
    delay_ms(100);
    
    DAC_TonoSimple(800, 500);   // Tono medio
    delay_ms(100);
    
    DAC_TonoSimple(1600, 500);  // Tono agudo
}

/* ================= EJEMPLO 8: DEMO COMPLETA ============================ */

/**
 * @brief Función demo que muestra todas las capacidades
 */
void ejemplo_demo_completa(void) {
    // Esta es la función más simple: reproduce todo automáticamente
    DAC_DemoMelodias();
}

/* ======================= NOTAS DE USO ===================================== */

/**
 * CONEXIÓN DEL HARDWARE:
 * ----------------------
 * Pin P0.26 (AOUT) del LPC1769 → Buzzer pasivo
 * 
 * Esquema simple:
 *   P0.26 ──[Resistor 100Ω]──┬── Buzzer (+)
 *                            │
 *                           GND ── Buzzer (-)
 * 
 * O con amplificador:
 *   P0.26 → Entrada amplificador → Buzzer o Speaker
 * 
 * IMPORTANTE:
 * - El buzzer debe ser PASIVO (no activo)
 * - Si usas un buzzer activo, no funcionará correctamente
 * - Usa un buzzer piezoeléctrico pasivo o un pequeño altavoz
 * 
 * AJUSTES:
 * - Volumen: Cambiar el valor 800 en DAC_UpdateValue(800) en DacMelodias.c
 *   Valores válidos: 0-1023 (recomendado: 600-900)
 * - Si suena distorsionado: reducir el valor
 * - Si no se escucha: aumentar el valor o verificar conexiones
 */

/**
 * CARACTERÍSTICAS TÉCNICAS:
 * -------------------------
 * - DAC de 10 bits (1024 niveles: 0-1023)
 * - Frecuencias soportadas: ~100Hz a ~2000Hz
 * - Precisión de frecuencia: ±0.1% (gracias al Timer1)
 * - Modos de onda: Cuadrada (más fuerte) y Senoidal (más suave)
 * - Timer usado: Timer1 (no interferirá con Timer0, Timer2 o Timer3)
 * - Interrupciones: TIMER1_IRQHandler (prioridad 1)
 */

/**
 * SOLUCIÓN DE PROBLEMAS:
 * ----------------------
 * 1. No se escucha nada:
 *    - Verificar conexión del buzzer
 *    - Verificar que el buzzer sea PASIVO
 *    - Aumentar volumen (DAC_UpdateValue valor)
 *    - Verificar que DAC_Init_Melodia() se llame antes
 * 
 * 2. Sonido distorsionado:
 *    - Reducir volumen del DAC
 *    - Verificar que el buzzer soporte la frecuencia
 *    - Agregar resistor en serie (100-220Ω)
 * 
 * 3. Notas desafinadas:
 *    - El Timer1 debería dar precisión exacta
 *    - Verificar frecuencia del reloj del sistema (SystemCoreClock)
 * 
 * 4. Melodías muy rápidas/lentas:
 *    - Ajustar las constantes NEGRA, BLANCA, etc.
 *    - Verificar que delay_ms() funcione correctamente
 */
