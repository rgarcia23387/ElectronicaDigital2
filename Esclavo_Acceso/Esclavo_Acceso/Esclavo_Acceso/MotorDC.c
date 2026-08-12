/*
 * MotorDC.c
 */

#include "MotorDC.h"
#include "UART.h"

#define MOTOR_ENA PD5
#define MOTOR_IN1 PD6
#define MOTOR_IN2 PD7
#define BTN_SUBIR PD3
#define BTN_BAJAR PD4

static void motor_pwm_on(uint8_t vel) {
	OCR0B = vel;
	TCCR0A |= (1 << COM0B1);            // conecta OC0B al pin
}

static void motor_pwm_off(void) {
	TCCR0A &= ~(1 << COM0B1);           // desconecta OC0B del pin
	PORTD &= ~(1 << MOTOR_ENA);
}

void Motor_Init(void) {
	DDRD |= (1 << MOTOR_ENA) | (1 << MOTOR_IN1) | (1 << MOTOR_IN2);

	DDRD  &= ~((1 << BTN_SUBIR) | (1 << BTN_BAJAR));
	PORTD |=  (1 << BTN_SUBIR) | (1 << BTN_BAJAR);   // pull-up interna

	TCCR0A = (1 << WGM01) | (1 << WGM00);            // Fast PWM
	TCCR0B = (1 << CS01) | (1 << CS00);              // prescaler 64
	OCR0B = 0;

	Motor_Parar();
}

void Motor_Subir(void) {
	PORTD |= (1 << MOTOR_IN1);
	PORTD &= ~(1 << MOTOR_IN2);
	motor_pwm_on(VELOCIDAD_MOTOR);
}

void Motor_Bajar(void) {
	PORTD &= ~(1 << MOTOR_IN1);
	PORTD |= (1 << MOTOR_IN2);
	motor_pwm_on(VELOCIDAD_MOTOR);
}

void Motor_Parar(void) {
	PORTD &= ~((1 << MOTOR_IN1) | (1 << MOTOR_IN2));
	motor_pwm_off();
}

void Motor_RevisarBotones(void) {
	static uint8_t moviendo = 0;
	uint8_t subir = !(PIND & (1 << BTN_SUBIR));   // activos en bajo
	uint8_t bajar = !(PIND & (1 << BTN_BAJAR));

	if (subir && !bajar) {
		if (!moviendo) { UART_Print("Boton SUBIR\r\n"); moviendo = 1; }
		Motor_Subir();
	} else if (bajar && !subir) {
		if (!moviendo) { UART_Print("Boton BAJAR\r\n"); moviendo = 1; }
		Motor_Bajar();
	} else {
		if (moviendo) { UART_Print("Motor detenido\r\n"); moviendo = 0; }
		Motor_Parar();
	}
}
