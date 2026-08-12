/*
 * Ultrasonico.c
 *
 * Mide el ancho del pulso de Echo contando esperas de 2 us,
 * sin ocupar ningun Timer del chip.
 */

#include "Ultrasonico.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <util/delay.h>

#define TRIG_PIN PD2
#define ECHO_PIN PD4

void Ultrasonico_Init(void) {
	DDRD |= (1 << TRIG_PIN);      // TRIG como salida
	DDRD &= ~(1 << ECHO_PIN);     // ECHO como entrada
	PORTD &= ~(1 << TRIG_PIN);
}

uint16_t Ultrasonico_LeerCM(void) {
	uint16_t espera = 0, conteo = 0;

	PORTD |= (1 << TRIG_PIN);     // pulso de disparo de 10 us
	_delay_us(10);
	PORTD &= ~(1 << TRIG_PIN);

	while (!(PIND & (1 << ECHO_PIN))) {    // espera a que suba el eco
		_delay_us(2);
		if (++espera > 15000) return 0;    // el sensor no respondio
	}

	while (PIND & (1 << ECHO_PIN)) {       // mide cuanto dura en alto
		_delay_us(2);
		if (++conteo > 15000) return 0;    // objeto fuera de rango
	}

	return (uint16_t)(((uint32_t)conteo * 2UL) / 58UL);  // de us a cm
}
