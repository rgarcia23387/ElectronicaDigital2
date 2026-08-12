/*
 * I2C_Master.h 
 */

#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include <avr/io.h>
#include <stdint.h>

void    I2C_Init(void);                  // 100 kHz
void    I2C_Start(void);
void    I2C_Stop(void);
uint8_t I2C_Write(uint8_t dato);         // devuelve el codigo de estado del TWI
uint8_t I2C_Read(uint8_t ack);           // ack=1 pide mas bytes, 0 = ultimo
uint8_t I2C_LeerEsclavo(uint8_t dir);    // lee 1 byte de estado, 0 si no responde

#endif /* I2C_MASTER_H_ */
