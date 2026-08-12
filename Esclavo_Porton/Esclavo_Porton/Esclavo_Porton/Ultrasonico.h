/*
 * Ultrasonico.h - Sensor HC-SR04
*/

#ifndef ULTRASONICO_H_
#define ULTRASONICO_H_

#include <avr/io.h>
#include <stdint.h>

void     Ultrasonico_Init(void);
uint16_t Ultrasonico_LeerCM(void);   // distancia en cm, 0 si no hubo eco

#endif /* ULTRASONICO_H_ */
