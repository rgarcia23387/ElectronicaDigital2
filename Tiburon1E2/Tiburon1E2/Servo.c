/*
 * Servo.c
 *
 * Created: 10/08/2026 02:08:49
 *  Author: dinog
 */ 

/*
 * Servo.c
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Implementación de PWM para servo. Prescaler 8 @16MHz
 *              da un reloj de timer de 2MHz -> ICR1=39999 produce un
 *              periodo de 20ms (50Hz). El pulso de 1-2ms corresponde
 *              a valores de OCR1A entre 2000 y 4000 cuentas.
 */

#include "Servo.h"

void Servo_init(void)
{
    DDRB |= (1 << PB1); // D9 (OC1A) como salida

    ICR1 = 39999; // TOP -> periodo de 20ms

    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // Fast PWM, prescaler 8

    OCR1A = 2000; // posicion inicial: 0 grados
}

void Servo_write_angle(uint8_t angulo)
{
    if (angulo > 180) angulo = 180;

    uint16_t pulso = 2000 + ((uint32_t)angulo * 2000UL) / 180UL;
    OCR1A = pulso;
}
