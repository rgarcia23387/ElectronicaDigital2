/*
 * SPI.c
 *
 * Created: 23/07/2026 18:12:35
 * Author: Rodrigo García  
 * Description: Libreria propia de comunicacion SPI. 
 * Trabajo en parejas con Monserrat Samayoa - 23431
 */

/****************************************/
// Encabezado (Libraries)
/****************************************/
#include "SPI.h"

// Uso del ATMega328PB
#if defined(__AVR_ATmega328PB__)
    #define SPCR_ SPCR0
    #define SPSR_ SPSR0
    #define SPDR_ SPDR0
#else
    #define SPCR_ SPCR
    #define SPSR_ SPSR
    #define SPDR_ SPDR
#endif

/****************************************/
// Function prototypes
/****************************************/
/****************************************/
// Main Function
/****************************************/
/****************************************/
// NON-Interrupt subroutines
/****************************************/
void SPI_InitMaestro(void) {
    // MOSI, SCK y SS deben ser salidas manejadas por el maestro. 
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);
    DDRB &= ~(1 << PB4);

    SPI_SS_High(); // SS inactivo mientras no se transmite

    // Habilita SPI, modo maestro, reloj = Fosc/16 (SPR0=1)
    SPCR_ = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

void SPI_InitEsclavo(void) {
    // MOSI, SCK y SS son controlados por el maestro, por lo que deben quedar como entradas.
    DDRB |= (1 << PB4);                              // MISO - salida
    DDRB &= ~((1 << PB3) | (1 << PB5) | (1 << PB2));  // MOSI, SCK, SS - entradas

    // Habilita el modulo SPI
    SPCR_ = (1 << SPE);
}

uint8_t SPI_Transfer(uint8_t dato) {
    SPDR_ = dato;                    // en maestro: dispara el reloj / en esclavo: precarga la respuesta
    while (!(SPSR_ & (1 << SPIF)));  // espera a que se completen los 8 bits
    return SPDR_;                    // dato recibido del otro extremo
}

void SPI_SS_Low(void) {
    PORTB &= ~(1 << PB2); // selecciona al esclavo
}

void SPI_SS_High(void) {
    PORTB |= (1 << PB2); // libera al esclavo
}

/****************************************/
// Interrupt routines
/****************************************/