/*
 * I2C_Slave.c
 */

#include "I2C_Slave.h"
#include <avr/interrupt.h>

volatile uint8_t i2c_estado = 0;

void I2C_Slave_Init(uint8_t direccion) {
	TWAR = (direccion << 1);                          // direccion propia
	TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);   // responde con ACK
}

ISR(TWI_vect) {
	switch (TWSR & 0xF8) {
		case 0xA8:                       // el Maestro nos quiere leer
		case 0xB8:
			TWDR = i2c_estado;
			TWCR |= (1 << TWEA) | (1 << TWINT);
			break;

		default:                         // cualquier otro caso: solo continuar
			TWCR |= (1 << TWEA) | (1 << TWINT);
			break;
	}
}
