/*
 * Tiburon1.c
 *
 * Created: 30/07/2026 18:35:00
 * Author : dinog
 */ 
/*
 * Maestro_Prueba_I2C.c
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Programa de prueba del Maestro, SIN el LCD. Solicita por I2C
 *              el estado del espacio de parqueo al Periferico 2 (direccion
 *              0x09) y enciende un LED en D13 (PB5) si el espacio esta
 *              ocupado. Sirve para verificar el sensor ultrasonico y la
 *              comunicacion I2C de forma aislada.
 */
/****************************************/
// Encabezado (Libraries)
/****************************************/
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>

#define DIR_ESCLAVO2 0x09
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_PIN  PB5   // D13 del Nano (LED integrado en la placa)

/****************************************/
// Function prototypes
/****************************************/
void GPIO_init(void);
void I2C_Master_init(void);
void I2C_Master_start(void);
void I2C_Master_stop(void);
void I2C_Master_write_address_read(uint8_t direccion);
uint8_t I2C_Master_read_ack(void);
uint8_t I2C_Master_read_nack(void);
void leer_esclavo2(uint8_t *ocupado, uint8_t *distancia);

/****************************************/
// Main Function
/****************************************/
int main(void)
{
    GPIO_init();
    I2C_Master_init();

    while (1)
    {
        uint8_t ocupado, distancia;
        leer_esclavo2(&ocupado, &distancia);

        if (ocupado)
            LED_PORT |= (1 << LED_PIN);   // LED encendido = "ocupado"
        else
            LED_PORT &= ~(1 << LED_PIN);  // LED apagado = "libre"

        _delay_ms(200);
    }
}

/****************************************/
// NON-Interrupt subroutines
/****************************************/
void GPIO_init(void)
{
    LED_DDR |= (1 << LED_PIN); // LED como salida
}

void I2C_Master_init(void)
{
    TWSR = 0x00; // prescaler = 1
    TWBR = ((F_CPU / 100000UL) - 16) / 2; // velocidad de 100kHz
    TWCR = (1 << TWEN);
}

void I2C_Master_start(void)
{
    TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
    while (!(TWCR & (1 << TWINT)));
}

void I2C_Master_stop(void)
{
    TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
}

void I2C_Master_write_address_read(uint8_t direccion)
{
    TWDR = (direccion << 1) | 0x01; // bit R/W = 1 -> modo lectura
    TWCR = (1 << TWEN) | (1 << TWINT);
    while (!(TWCR & (1 << TWINT)));
}

uint8_t I2C_Master_read_ack(void)
{
    TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t I2C_Master_read_nack(void)
{
    TWCR = (1 << TWEN) | (1 << TWINT);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

void leer_esclavo2(uint8_t *ocupado, uint8_t *distancia)
{
    I2C_Master_start();
    I2C_Master_write_address_read(DIR_ESCLAVO2);
    *ocupado   = I2C_Master_read_ack();
    *distancia = I2C_Master_read_nack();
    I2C_Master_stop();
}

/****************************************/
// Interrupt routines
/****************************************/
// (sin interrupciones por el momento)