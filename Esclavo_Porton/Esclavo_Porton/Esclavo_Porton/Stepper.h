/*
 * Stepper.h - Motor a pasos con driver ULN2003
 */

#ifndef STEPPER_H_
#define STEPPER_H_

#include <avr/io.h>
#include <stdint.h>

#define PASOS_PORTON   1024   // ajustar segun cuanto suba el porton
#define VELOCIDAD_PASO 2      // ms entre pasos (menor = mas rapido)

void Stepper_Init(void);
void Stepper_Abrir(void);
void Stepper_Cerrar(void);

#endif /* STEPPER_H_ */
