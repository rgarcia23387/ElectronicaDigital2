/*
 * Servo.c
 */

#include "Servo.h"

void Servo_Init(void) {
	DDRB |= (1 << PB1);                  // D9 como salida

	ICR1 = 39999;                        // TOP: periodo de 20 ms
	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // Fast PWM, prescaler 8

	OCR1A = 2000;                        // arranca en 0 grados
}

void Servo_Angulo(uint8_t grados) {
	if (grados > 180) grados = 180;
	OCR1A = 2000 + ((uint32_t)grados * 2000UL) / 180UL;  // 1 ms a 2 ms
}
