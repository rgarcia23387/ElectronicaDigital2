/*
 * SPI.c
 *
 * Created: 23/07/2026 18:12:35
 * Author: Rodrigo García  
 * Description: Libreria propia de comunicacion SPI. 
 */
/****************************************/
// Encabezado (Libraries)
/****************************************/
#include "SPI.h"

// El ATmega328PB renombra los registros del SPI agregando el sufijo "0".
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
    // MOSI, SCK y SS son salidas manejadas por el maestro. MISO es entrada.
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);
    DDRB &= ~(1 << PB4);

    // Pin de handshake "Ready" (PB1 / D9) como entrada, viene del ESP32
    DDRB &= ~(1 << PB1);

    SPI_SS_High(); // SS inactivo (en alto) mientras no se transmite

    // Habilita SPI (SPE=1), modo maestro (MSTR=1), reloj = Fosc/16 (SPR0=1)
    // Se usa un reloj moderado (1MHz con 16MHz de Fosc) para mayor confiabilidad con el ESP32
    SPCR_ = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

uint8_t SPI_Transfer(uint8_t dato) {
    SPDR_ = dato;                    // escribir SPDR inicia la transferencia (genera el reloj)
    while (!(SPSR_ & (1 << SPIF)));  // espera a que se completen los 8 bits
    return SPDR_;                    // dato recibido desde el esclavo
}

void SPI_SS_Low(void) {
    PORTB &= ~(1 << PB2); // selecciona al esclavo (activo en bajo)
}

void SPI_SS_High(void) {
    PORTB |= (1 << PB2); // libera al esclavo
}

uint8_t SPI_EsclavoListo(void) {
    return (PINB & (1 << PB1)) ? 1 : 0;
}

/****************************************/
// Interrupt routines
/****************************************/
