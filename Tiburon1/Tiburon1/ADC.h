/*
 * ADC.h
 *
 * Created: 23/07/2026 18:10:57
 * Author: Rodrigo García
 * Description: Libreria propia para el manejo del modulo ADC (lectura de potenciometros)
 * Trabajo en parejas con Monserrat Samayoa - 23431
 */

#ifndef ADC_H_
#define ADC_H_


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
void ADC_Init(void);
uint8_t ADC_Read(uint8_t canal);

#ifdef __cplusplus
}
#endif


#endif /* ADC_H_ */