/*
 * SPI.h
 *
 * Created: 23/07/2026 18:12:48
 * Author: Rodrigo García  
 * Description: Libreria propia de comunicacion SPI. 
 * Trabajo en parejas con Monserrat Samayoa - 23431
 */

#ifndef SPI_H_
#define SPI_H_

/****************************************/
// Encabezado (Libraries)
/****************************************/
#include <avr/io.h>


/****************************************/
// Function prototypes
/****************************************/

// Inicializacion segun el rol del MCU
void SPI_InitMaestro(void);
void SPI_InitEsclavo(void);

// Transferencia de un byte.
uint8_t SPI_Transfer(uint8_t dato);

// Control manual de la linea SS. 
void SPI_SS_Low(void);
void SPI_SS_High(void);





#endif /* SPI_H_ */