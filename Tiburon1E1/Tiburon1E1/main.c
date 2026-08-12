/*
 * Tiburon1E1.c
 *
 * Created: 6/08/2026 18:41:31
 * Author : dinog
 */ 
/*
 * Esclavo2_Parqueo.c
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Programa principal del Periferico 2 del sistema de parqueo
 *              inteligente. Lee el sensor ultrasonico HC-SR04 (ocupacion
 *              del espacio) y responde al Maestro por I2C (direccion 0x09)
 *              con el estado y la distancia medida.
 */
/****************************************/
// Encabezado (Libraries)
/****************************************/
#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>
#include "Ultrasonico.h"

#define I2C_SLAVE_ADDRESS 0x09
#define I2C_BUFFER_LEN     2

/****************************************/
// Function prototypes
/****************************************/
void GPIO_init(void);
void I2C_Slave_init(uint8_t direccion);

/****************************************/
// Variables globales
/****************************************/
// i2c_tx_buffer[0] = estado (0 = libre, 1 = ocupado)
// i2c_tx_buffer[1] = distancia medida en cm
volatile uint8_t i2c_tx_buffer[I2C_BUFFER_LEN] = {0, 0};
static volatile uint8_t tx_index = 0;

/****************************************/
// Main Function
/****************************************/
int main(void)
{
    GPIO_init();
    Ultrasonico_init();
    I2C_Slave_init(I2C_SLAVE_ADDRESS);

    while (1)
    {
        uint16_t distancia = Ultrasonico_leer_cm();

        // Umbral de ocupacion: ajustar segun la altura de montaje del sensor
        uint8_t ocupado = (distancia > 0 && distancia < 15) ? 1 : 0;

        // Actualiza el buffer que el Maestro leera por I2C
        i2c_tx_buffer[0] = ocupado;
        i2c_tx_buffer[1] = (uint8_t)distancia;

        _delay_ms(200);
    }
}

/****************************************/
// NON-Interrupt subroutines
/****************************************/
void GPIO_init(void)
{
    // Reservado para configuracion adicional de pines del periferico
}

void I2C_Slave_init(uint8_t direccion)
{
    TWAR = (direccion << 1);
    TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
    sei();
}

/****************************************/
// Interrupt routines
/****************************************/
ISR(TWI_vect)
{
    switch (TWSR & 0xF8)
    {
        case 0x60: // SLA+W recibido, ACK enviado
        case 0x68:
            tx_index = 0;
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;

        case 0xA8: // SLA+R recibido, ACK enviado -> inicia transmision
        case 0xB8: // Byte enviado, ACK recibido -> continua transmision
            if (tx_index < I2C_BUFFER_LEN)
            {
                TWDR = i2c_tx_buffer[tx_index++];
            }
            else
            {
                TWDR = 0xFF;
            }
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;

        case 0xC0: // Byte enviado, NACK recibido (ultimo byte)
        case 0xC8:
            tx_index = 0;
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;

        default:
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;
    }
}
