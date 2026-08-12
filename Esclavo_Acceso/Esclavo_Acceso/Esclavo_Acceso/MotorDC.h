/*
 * MotorDC.h - Motor DC por L298N + botones
 */

#ifndef MOTORDC_H_
#define MOTORDC_H_

#include <avr/io.h>
#include <stdint.h>

#define VELOCIDAD_MOTOR 200   // 0 a 255

void Motor_Init(void);
void Motor_Subir(void);
void Motor_Bajar(void);
void Motor_Parar(void);
void Motor_RevisarBotones(void);   // llamar seguido dentro del bucle principal

#endif /* MOTORDC_H_ */
