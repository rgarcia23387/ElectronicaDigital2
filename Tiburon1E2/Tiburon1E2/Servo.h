/*
 * Servo.h
 *
 * Created: 10/08/2026 02:10:21
 *  Author: dinog
 */ 


#ifndef SERVO_H_
#define SERVO_H_

/*
 * Servo.h
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Generación de señal PWM para servomotor usando Timer1
 *              en modo Fast PWM (ICR1 como TOP). Señal en D9 (OC1A).
 */


#include <avr/io.h>

void Servo_init(void);
void Servo_write_angle(uint8_t angulo);



#endif /* SERVO_H_ */