/*
 * Stepper.c
 *
 * Secuencia de 4 pasos: se energiza una bobina a la vez.
 */

#include "Stepper.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <util/delay.h>

#define STEP_IN1 PB0
#define STEP_IN2 PB1
#define STEP_IN3 PB2
#define STEP_IN4 PB3
#define STEP_MASK ((1 << STEP_IN1) | (1 << STEP_IN2) | (1 << STEP_IN3) | (1 << STEP_IN4))

static int8_t paso_actual = 0;

static void stepper_apagar(void) {
	PORTB &= ~STEP_MASK;    // sin corriente en las bobinas, evita calentamiento
}

static void aplicar_paso(int8_t p) {
	uint8_t salida;

	if      (p == 0) salida = (1 << STEP_IN1);
	else if (p == 1) salida = (1 << STEP_IN2);
	else if (p == 2) salida = (1 << STEP_IN3);
	else             salida = (1 << STEP_IN4);

	PORTB = (PORTB & ~STEP_MASK) | salida;
}

static void mover(uint16_t pasos, uint8_t sentido) {
	for (uint16_t i = 0; i < pasos; i++) {
		paso_actual += sentido ? 1 : -1;
		if (paso_actual > 3) paso_actual = 0;   // la secuencia es circular
		if (paso_actual < 0) paso_actual = 3;
		aplicar_paso(paso_actual);
		_delay_ms(VELOCIDAD_PASO);
	}
	stepper_apagar();
}

void Stepper_Init(void) {
	DDRB |= STEP_MASK;
	stepper_apagar();
}

void Stepper_Abrir(void)  { mover(PASOS_PORTON, 1); }
void Stepper_Cerrar(void) { mover(PASOS_PORTON, 0); }
