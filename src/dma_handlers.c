/**
 * @file dma_handlers.c
 * @brief Manejadores centralizados de interrupciones DMA
 * 
 * Centraliza todos los handlers GPDMA para evitar conflictos de múltiples
 * definiciones. Aquí se maneja:
 * - DMA_IRQHandler: El ISR principal que despacha a los handlers específicos
 * - Funciones de callback para cada canal DMA
 * 
 * DMA activo:
 * - Canal 1: Melodías (DAC)
 * - Bluetooth: SIN DMA (solo UART polling)
 * 
 * @date Noviembre 2025
 */

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"

/* === CONFIGURACIÓN DE CANALES === */
#define CANAL_DMA_MELODIAS    1  // Canal DMA 1 para DAC

/* === PROTOTIPOS DE FUNCIONES CALLBACK === */

extern void melodias_dma_on_transfer_complete(void);

/* === MANEJADOR PRINCIPAL DMA === */

/**
 * @brief ISR del DMA - Despacha interrupciones a los módulos correspondientes
 * 
 * Esta es la única definición del handler GPDMA_IRQHandler.
 * Verifica qué canales tienen interrupciones pendientes y llama
 * a las funciones de callback correspondientes.
 */
void GPDMA_IRQHandler(void) {
    /* Verificar canal 1 (Melodías DAC) */
    if (GPDMA_IntGetStatus(GPDMA_INTTC, CANAL_DMA_MELODIAS) == SET) {
        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, CANAL_DMA_MELODIAS);
        melodias_dma_on_transfer_complete();
    }
    
    /* Aquí se pueden agregar más canales si es necesario */
}
