/*
 * I2C_Slave.h - Bus I2C (TWI) en modo esclavo
*/

#ifndef I2C_SLAVE_H_
#define I2C_SLAVE_H_

#include <avr/io.h>
#include <stdint.h>

extern volatile uint8_t i2c_estado;   // lo que se le reporta al Maestro

void I2C_Slave_Init(uint8_t direccion);

#endif /* I2C_SLAVE_H_ */
