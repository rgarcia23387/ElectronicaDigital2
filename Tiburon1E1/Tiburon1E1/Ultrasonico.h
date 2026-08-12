/*
 * Ultrasonico.h
 *
 * Created: 6/08/2026 18:50:04
 *  Author: dinog
 */ 

/*
 * Ultrasonico.h
 *
 * Created: 6/08/2026 18:50:04
 *  Author: dinog
 */ 

#ifndef ULTRASONICO_H_
#define ULTRASONICO_H_

/*
 * Ultrasonico.h
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Librería para el sensor ultrasónico HC-SR04, usando el
 *              Timer1 del ATmega328P para medir el ancho del pulso de Echo.
 *              Trig = PD2 (D2), Echo = PD4 (D4)
 */

#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>

// Pines del sensor (en PORTD)
#define TRIG_DDR     DDRD
#define TRIG_PORT    PORTD
#define TRIG_PIN     PD2     // D2 para Trig

#define ECHO_DDR     DDRD
#define ECHO_PIN_REG PIND
#define ECHO_PIN     PD4     // D4 para Echo

void Ultrasonico_init(void);
uint16_t Ultrasonico_leer_cm(void);

#endif /* ULTRASONICO_H_ */