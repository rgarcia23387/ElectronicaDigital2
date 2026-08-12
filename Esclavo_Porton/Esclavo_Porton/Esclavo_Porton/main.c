/*
 * ESCLAVO PORTON - Ultrasonico HC-SR04 + Motor Stepper
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "Stepper.h"
#include "Ultrasonico.h"
#include "I2C_Slave.h"
#include "UART.h"
#include "Tiempo.h"

#define I2C_DIRECCION 0x09

#define BIT_PORTON_ABIERTO 0x01

#define UMBRAL_CM      15       // menos de esto = hay un vehiculo
#define TIEMPO_ABIERTO 3000UL   // el porton se queda abierto 3 segundos

int main(void) {
	uint8_t porton_abierto = 0;
	uint32_t tiempo_apertura = 0, t_serial = 0;

	Tiempo_Init();
	UART_Init(9600);
	Ultrasonico_Init();
	Stepper_Init();
	I2C_Slave_Init(I2C_DIRECCION);
	sei();

	i2c_estado = 0;
	UART_Print("=== ESCLAVO PORTON ===\r\n");
	UART_Print("Esperando vehiculos...\r\n");

	while (1) {
		uint16_t distancia = Ultrasonico_LeerCM();
		uint8_t vehiculo = (distancia > 0 && distancia < UMBRAL_CM);

		/* ---- Abrir el porton ---- */
		if (vehiculo && !porton_abierto) {
			UART_Print("Vehiculo a ");
			UART_PrintNum(distancia);
			UART_Print(" cm -> ABRIENDO\r\n");

			porton_abierto = 1;                 // se marca antes de mover, asi el
			i2c_estado = BIT_PORTON_ABIERTO;    // Maestro lo ve durante todo el giro

			Stepper_Abrir();
			tiempo_apertura = millis();
			UART_Print("Porton ABIERTO\r\n");
		}

		/* ---- Cerrar a los 3 segundos ---- */
		if (porton_abierto && (millis() - tiempo_apertura >= TIEMPO_ABIERTO)) {
			UART_Print("CERRANDO porton\r\n");
			Stepper_Cerrar();
			porton_abierto = 0;
			i2c_estado = 0;
			UART_Print("Porton CERRADO\r\n");
		}

		/* ---- Monitoreo cada 2 segundos ---- */
		if (millis() - t_serial > 2000) {
			UART_Print("Distancia: ");
			if (distancia > 0) { UART_PrintNum(distancia); UART_Print(" cm"); }
			else UART_Print("sin lectura");
			UART_Print(porton_abierto ? " | Porton: ABIERTO\r\n" : " | Porton: CERRADO\r\n");
			t_serial = millis();
		}

		_delay_ms(50);
	}
}
