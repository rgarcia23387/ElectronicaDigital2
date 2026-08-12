/*
 * Servo.h - Servo por PWM de hardware con Timer1
 */

#ifndef SERVO_H_
#define SERVO_H_

#include <avr/io.h>
#include <stdint.h>

void Servo_Init(void);
void Servo_Angulo(uint8_t grados);   // 0 a 180

#endif /* SERVO_H_ */
