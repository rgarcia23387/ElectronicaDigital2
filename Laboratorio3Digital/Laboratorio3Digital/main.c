/*
 * Laboratorio3Digital.c
 *
 * Created: 23/07/2026 18:06:12
 * Author : Rodrigo García  
 * Description: Laboratorio 3 Esclavo: lee dos potenciometros
 *              mediante ADC y transmite las lecturas al maestro via SPI.
 * Trabajo en parejas con Monserrat Samayoa - 23431
 */

/****************************************/
// Encabezado (Libraries)
/****************************************/
#define F_CPU 16000000UL

#include <avr/io.h>
#include "ADC.h"
#include "SPI.h"

/****************************************/
// Function prototypes
/****************************************/
/****************************************/
// Main Function
/****************************************/
int main(void) {
    ADC_Init();
    SPI_InitEsclavo();

    uint8_t pot1, pot2;

    while (1) {
        // Lectura de los dos potenciometros
        pot1 = ADC_Read(6); // Pot 1 conectado en A6
        pot2 = ADC_Read(7); // Pot 2 conectado en A7

        // El esclavo solo responde cuando el maestro genera el reloj.
        // Primer byte pedido por el maestro - valor de Pot1
        SPI_Transfer(pot1);

        // Segundo byte pedido por el maestro - valor de Pot2
        SPI_Transfer(pot2);
    }

    return 0;
}

/****************************************/
// NON-Interrupt subroutines
/****************************************/
/****************************************/
// Interrupt routines
/****************************************/
