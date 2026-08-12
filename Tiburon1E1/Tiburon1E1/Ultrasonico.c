/*
 * Ultrasonico.c
 *
 * Created: 6/08/2026 18:49:44
 *  Author: dinog
 */ 

/*
 * Ultrasonico.c
 *
 * Created: 6/08/2026 18:49:44
 *  Author: dinog
 */ 

/*
 * Ultrasonico.c
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Implementación de lectura de distancia con el sensor HC-SR04.
 *              Trig = PD2 (D2), Echo = PD4 (D4)
 */

#include "Ultrasonico.h"

void Ultrasonico_init(void)
{
    TRIG_DDR |= (1 << TRIG_PIN);   // Trig como salida
    ECHO_DDR &= ~(1 << ECHO_PIN);  // Echo como entrada
    TRIG_PORT &= ~(1 << TRIG_PIN);

    // Timer1 en modo normal, prescaler 8 -> resolucion de 0.5us por tick (16MHz)
    TCCR1A = 0x00;
    TCCR1B = (1 << CS11);
}

uint16_t Ultrasonico_leer_cm(void)
{
    uint16_t ticks;
    uint16_t timeout;

    // Genera el pulso de disparo (10us)
    TRIG_PORT |= (1 << TRIG_PIN);
    _delay_us(10);
    TRIG_PORT &= ~(1 << TRIG_PIN);

    // Espera a que Echo suba (con timeout de seguridad)
    timeout = 0;
    while (!(ECHO_PIN_REG & (1 << ECHO_PIN)))
    {
        timeout++;
        if (timeout > 60000) return 0; // sin respuesta del sensor
    }

    // Reinicia el contador al inicio del pulso de Echo
    TCNT1 = 0;

    // Espera a que Echo baje
    timeout = 0;
    while (ECHO_PIN_REG & (1 << ECHO_PIN))
    {
        timeout++;
        if (timeout > 60000) return 999; // objeto fuera de rango
    }

    ticks = TCNT1;

    // distancia_cm = (ticks * 0.5us) / 58 aprox.
    uint16_t distancia_cm = (uint16_t)((ticks * 0.5) / 58.0);

    return distancia_cm;
}