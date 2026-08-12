/*
 * Tiburon1E2.c
 *
 * Created: 10/08/2026 02:07:51
 * Author : dinog
 */ 
/*
 * Esclavo_Servo_PN532.c
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Programa del Esclavo con lector NFC PN532 (SPI) y servo
 *              (talanquera). Al leer una tarjeta valida, abre el servo
 *              y lo cierra automaticamente 3 segundos despues, quedando
 *              listo para la siguiente lectura. Responde al Maestro por
 *              I2C (direccion 0x08) con el estado de la puerta.
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
#include "PN532.h"
#include "Servo.h"
#include "UART.h"

#define I2C_SLAVE_ADDRESS  0x08
#define I2C_BUFFER_LEN     1
#define TIEMPO_ABIERTA_MS  3000
#define TIMEOUT_LECTURA_MS 100

/****************************************/
// Function prototypes
/****************************************/
void GPIO_init(void);
void Timer0_init(void);
uint32_t millis(void);
void I2C_Slave_init(uint8_t direccion);
void abrir_puerta(void);
void cerrar_puerta(void);

/****************************************/
// Variables globales
/****************************************/
volatile uint32_t millis_count = 0;
volatile uint8_t i2c_tx_buffer[I2C_BUFFER_LEN] = {0};
static volatile uint8_t tx_index = 0;

uint8_t  puerta_abierta = 0;
uint32_t tiempo_apertura = 0;
uint8_t  uid_registrado[7];
uint8_t  uid_registrado_len = 0;
uint8_t  hay_tarjeta_registrada = 0;

/****************************************/
// Main Function
/****************************************/
int main(void)
{
    uint8_t uid[7];
    uint8_t uid_len;

    GPIO_init();
    Timer0_init();
    UART_init(9600);
    PN532_init();
    Servo_init();
    I2C_Slave_init(I2C_SLAVE_ADDRESS);
    sei();

    Servo_write_angle(0);
    puerta_abierta = 0;

    if (PN532_sam_config())
        UART_print("PN532 listo\r\n");
    else
        UART_print("ERROR: PN532 no respondio\r\n");

    while (1)
    {
        if (!puerta_abierta)
        {
            if (PN532_leer_uid(uid, &uid_len, TIMEOUT_LECTURA_MS))
            {
                UART_print("Tarjeta detectada, UID: ");
                for (uint8_t i = 0; i < uid_len; i++)
                    UART_print_hex(uid[i]);
                UART_print("\r\n");

                if (!hay_tarjeta_registrada)
                {
                    // Registra la primera tarjeta leida como la autorizada
                    for (uint8_t i = 0; i < uid_len; i++)
                        uid_registrado[i] = uid[i];
                    uid_registrado_len = uid_len;
                    hay_tarjeta_registrada = 1;
                    UART_print("Tarjeta registrada\r\n");
                    abrir_puerta();
                }
                else
                {
                    uint8_t coincide = (uid_len == uid_registrado_len);
                    if (coincide)
                    {
                        for (uint8_t i = 0; i < uid_len; i++)
                            if (uid[i] != uid_registrado[i]) coincide = 0;
                    }

                    if (coincide)
                    {
                        UART_print("Tarjeta valida\r\n");
                        abrir_puerta();
                    }
                    else
                    {
                        UART_print("Tarjeta NO autorizada\r\n");
                    }
                }
            }
        }

        if (puerta_abierta && (millis() - tiempo_apertura >= TIEMPO_ABIERTA_MS))
        {
            cerrar_puerta();
        }

        i2c_tx_buffer[0] = puerta_abierta;
    }
}

/****************************************/
// NON-Interrupt subroutines
/****************************************/
void GPIO_init(void)
{
    // Reservado para configuracion adicional de pines
}

void Timer0_init(void)
{
    TCCR0A = (1 << WGM01);               // Modo CTC
    OCR0A  = 249;                        // 1ms con prescaler 64 @ 16MHz
    TIMSK0 = (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00);  // prescaler 64
}

uint32_t millis(void)
{
    uint32_t valor;
    cli();
    valor = millis_count;
    sei();
    return valor;
}

void I2C_Slave_init(uint8_t direccion)
{
    TWAR = (direccion << 1);
    TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
}

void abrir_puerta(void)
{
    Servo_write_angle(90);
    puerta_abierta = 1;
    tiempo_apertura = millis();
    UART_print("Puerta ABIERTA\r\n");
}

void cerrar_puerta(void)
{
    Servo_write_angle(0);
    puerta_abierta = 0;
    UART_print("Puerta CERRADA - lista para otra tarjeta\r\n");
}

/****************************************/
// Interrupt routines
/****************************************/
ISR(TIMER0_COMPA_vect)
{
    millis_count++;
}

ISR(TWI_vect)
{
    switch (TWSR & 0xF8)
    {
        case 0x60:
        case 0x68:
            tx_index = 0;
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;

        case 0xA8:
        case 0xB8:
            if (tx_index < I2C_BUFFER_LEN)
                TWDR = i2c_tx_buffer[tx_index++];
            else
                TWDR = 0xFF;
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;

        case 0xC0:
        case 0xC8:
            tx_index = 0;
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;

        default:
            TWCR |= (1 << TWEA) | (1 << TWINT);
            break;
    }
}

