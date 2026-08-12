/*
 * Tiempo.c
 */

#include "Tiempo.h"
#include <avr/interrupt.h>

volatile uint32_t ms_count = 0;

ISR(TIMER0_COMPA_vect) { ms_count++; }   // se dispara cada 1 ms

void Tiempo_Init(void) {
	TCCR0A = (1 << WGM01);               // modo CTC
	OCR0A  = 249;                        // 1 ms con prescaler 64
	TIMSK0 = (1 << OCIE0A);              // habilita la interrupcion
	TCCR0B = (1 << CS01) | (1 << CS00);  // prescaler 64
}

uint32_t millis(void) {
	uint32_t v;
	cli();                               // lectura atomica de 32 bits
	v = ms_count;
	sei();
	return v;
}
