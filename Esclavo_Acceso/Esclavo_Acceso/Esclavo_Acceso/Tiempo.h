/*
 * Tiempo.h - Base de tiempo de 1 ms con Timer2
 */

#ifndef TIEMPO_H_
#define TIEMPO_H_

#include <avr/io.h>
#include <stdint.h>

void     Tiempo_Init(void);
uint32_t millis(void);

#endif /* TIEMPO_H_ */
