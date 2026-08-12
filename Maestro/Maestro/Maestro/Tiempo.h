/*
 * Tiempo.h - Base de tiempo de 1 ms con Timer0
 */

#ifndef TIEMPO_H_
#define TIEMPO_H_

#include <avr/io.h>
#include <stdint.h>

void     Tiempo_Init(void);   // arranca el Timer0 en CTC cada 1 ms
uint32_t millis(void);        // milisegundos desde el arranque

#endif /* TIEMPO_H_ */
