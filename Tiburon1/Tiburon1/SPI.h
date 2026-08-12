/*
 * SPI.h
 *
 * Created: 23/07/2026 18:12:48
 * Author: Rodrigo García  
 * Description: Libreria propia de comunicacion SPI
 */

#ifndef SPI_H_
#define SPI_H_

/****************************************/
// Encabezado (Libraries)
/****************************************/
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************/
// Function prototypes
/****************************************/
// Pines SPI ATmega328P/328PB: MOSI=PB3, MISO=PB4, SCK=PB5, SS=PB2
// Pin de handshake "Ready" (entrada, viene del ESP32): PB1 (D9)

void SPI_InitMaestro(void);
uint8_t SPI_Transfer(uint8_t dato);
void SPI_SS_Low(void);
void SPI_SS_High(void);

// Retorna 1 si el esclavo (ESP32) ya dejo lista su transaccion, 0 si no
uint8_t SPI_EsclavoListo(void);

#ifdef __cplusplus
}
#endif



#endif /* SPI_H_ */