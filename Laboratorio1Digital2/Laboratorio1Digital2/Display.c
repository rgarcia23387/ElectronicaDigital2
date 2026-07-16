/*
 * Display.c
 *
 * Created: 9/07/2026 18:18:10
 * Author: Rodrigo García
 * Description: Libreria del Display, archivo .c
 */

/****************************************/
// Encabezado (Libraries)
/****************************************/
#include "Display.h"

// Tabla de segmentos 0-F
static const uint8_t tabla7seg[16] = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111, // 0, 1, 2, 3
    0b01100110, 0b01101101, 0b01111101, 0b00000111, // 4, 5, 6, 7
    0b01111111, 0b01101111, 0b01110111, 0b01111100, // 8, 9, A, b
    0b00111001, 0b01011110, 0b01111001, 0b01110001  // C, d, E, F
};

/****************************************/
// Funciones
/****************************************/

// Configura PORTD como salida y apaga el display
void display_init(void)
{
    DDRD  = 0xFF;
    PORTD = 0x00;
}

// Muestra un digito en el display
void display_mostrarDigito(uint8_t digito)
{
    if (digito > 15) return;
    PORTD = tabla7seg[digito];
}

// Apaga todos los segmentos
void display_apagar(void)
{
    PORTD = 0x00;
}