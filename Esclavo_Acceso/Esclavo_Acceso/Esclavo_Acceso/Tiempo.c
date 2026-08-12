/*
 * Tiempo.c
 */

#include "Tiempo.h"
#include <avr/interrupt.h>

volatile uint32_t ms_count = 0;

ISR(TIMER2_COMPA_vect) { ms_count++; }   // se dispara cada 1 ms

void Tiempo_Init(void) {
	TCCR2A = (1 << WGM21);               // modo CTC
	OCR2A  = 249;                        // 1 ms con prescaler 64
	TIMSK2 = (1 << OCIE2A);
	TCCR2B = (1 << CS22);                // prescaler 64
}

uint32_t millis(void) {
	uint32_t v;
	cli();                               // lectura atomica de 32 bits
	v = ms_count;
	sei();
	return v;
}
