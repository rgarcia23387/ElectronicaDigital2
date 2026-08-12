/*
 * ADC.c
 *
 * Created: 23/07/2026 18:11:24
 * Author: Rodrigo García
 * Description: Libreria propia para el manejo del modulo ADC 
 * Trabajo en parejas con Monserrat Samayoa - 23431
 */

/****************************************/
// Encabezado (Libraries)
/****************************************/
#include "ADC.h"

/****************************************/
// Function prototypes
/****************************************/

/****************************************/
// Main Function
/****************************************/

/****************************************/
// NON-Interrupt subroutines
/****************************************/
void ADC_Init(void) {
    // AVcc como referencia
    // Solo se usan los 8 bits altos 
    ADMUX = (1 << REFS0) | (1 << ADLAR);

    // Habilita el ADC  con prescaler de 128 - 16MHz/128 = 125kHz 
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint8_t ADC_Read(uint8_t canal) {
    canal &= 0x07; // solo canales 0-7 son validos

    // Selecciona el canal sin perder la configuracion
    ADMUX = (ADMUX & 0xF0) | canal;

    ADCSRA |= (1 << ADSC);          // inicia la conversion
    while (ADCSRA & (1 << ADSC));   // espera a que termine 

    return ADCH;                    // retorna los 8 bits mas significativos del resultado
}

/****************************************/
// Interrupt routines
/****************************************/
