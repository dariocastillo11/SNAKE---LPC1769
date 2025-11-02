#include "LPC17xx.h"
#include <math.h>
#include <stdint.h>

#define PI 3.14159265

// === CONFIGURACIÓN DEL DAC ===
void DAC_Init(void) {
    LPC_PINCON->PINSEL1 &= ~(0x3 << 20); // Limpiar bits P0.26
    LPC_PINCON->PINSEL1 |=  (0x2 << 20); // Función AOUT en P0.26
}

// === CONFIGURACIÓN DEL TIMER0 ===
void Timer0_Init(uint32_t sample_rate) {
    LPC_SC->PCONP |= (1 << 1);    // Encender Timer0
    LPC_SC->PCLKSEL0 &= ~(0x3 << 2);
    LPC_SC->PCLKSEL0 |=  (0x1 << 2);   // PCLK = CCLK

    LPC_TIM0->MR0 = SystemCoreClock / sample_rate; // match cada muestra
    LPC_TIM0->MCR = (1 << 0) | (1 << 1);           // Interrupción y reset
    NVIC_EnableIRQ(TIMER0_IRQn);
    LPC_TIM0->TCR = 1; // habilitar
}

// === GENERACIÓN DE ONDA SENO ===
#define TABLE_SIZE 100
uint16_t sine_table[TABLE_SIZE];

// Rellena la tabla con una onda seno escalada al rango 10 bits (0-1023)
void fill_sine_table(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        double val = sin(2 * PI * i / TABLE_SIZE);
        sine_table[i] = (uint16_t)((val + 1.0) * 511.5); // rango 0..1023
    }
}

// === REPRODUCCIÓN DE MELODÍA ===
// Frecuencias de notas (Hz)
const float notas[] = {262, 294, 330, 349, 392, 440, 494, 523}; // DO–SI
const int duraciones[] = {400, 400, 400, 400, 400, 400, 400, 800};
const int n_notas = 8;

volatile uint32_t sample_index = 0;
volatile uint32_t samples_per_note = 0;
volatile uint32_t current_note = 0;
volatile uint32_t note_counter = 0;
volatile uint32_t samples_played = 0;
volatile float phase_step = 0;
volatile float phase_acc = 0;

// Configura una nueva nota
void play_note(float freq, uint32_t sample_rate, uint32_t duration_ms) {
    phase_step = (freq * TABLE_SIZE) / sample_rate;
    samples_per_note = (sample_rate * duration_ms) / 1000;
    sample_index = 0;
    samples_played = 0;
}

// === ISR DEL TIMER0 ===
void TIMER0_IRQHandler(void) {
    if (LPC_TIM0->IR & 1) {
        LPC_TIM0->IR = 1; // limpiar flag

        if (current_note < n_notas) {
            uint16_t val = sine_table[(int)phase_acc % TABLE_SIZE];
            phase_acc += phase_step;
            LPC_DAC->DACR = (val << 6); // escribir DAC (bits [15:6])

            samples_played++;
            if (samples_played >= samples_per_note) {
                current_note++;
                if (current_note < n_notas)
                    play_note(notas[current_note], 8000, duraciones[current_note]);
                else
                    LPC_DAC->DACR = 0;
            }
        }
    }
}

// === MAIN ===
int main(void) {
    SystemInit();
    DAC_Init();
    fill_sine_table();

    Timer0_Init(8000); // 8 kHz de muestreo

    current_note = 0;
    play_note(notas[0], 8000, duraciones[0]);

    while (1) {
        __WFI(); // espera interrupciones (ahorra energía)
    }
}
