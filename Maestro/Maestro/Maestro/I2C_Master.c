/*
 * I2C_Master.c
 */

#include "I2C_Master.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

void I2C_Init(void) {
	TWSR = 0x00;                          // prescaler 1
	TWBR = ((F_CPU / 100000UL) - 16) / 2; // 100 kHz
	TWCR = (1 << TWEN);
}

void I2C_Start(void) {
	TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));
}

void I2C_Stop(void) {
	TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
	while (TWCR & (1 << TWSTO));          // espera a que el bus quede libre
}

uint8_t I2C_Write(uint8_t dato) {
	TWDR = dato;
	TWCR = (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));
	return (TWSR & 0xF8);
}

uint8_t I2C_Read(uint8_t ack) {
	TWCR = (1 << TWEN) | (1 << TWINT) | (ack ? (1 << TWEA) : 0);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

uint8_t I2C_LeerEsclavo(uint8_t dir) {
	uint8_t valor;

	I2C_Start();
	I2C_Write((dir << 1) | 1);            // SLA+R
	if ((TWSR & 0xF8) != 0x40) {          // el esclavo no dio ACK
		I2C_Stop();
		return 0;
	}
	valor = I2C_Read(0);                  // ultimo byte, se responde NACK
	I2C_Stop();
	return valor;
}
