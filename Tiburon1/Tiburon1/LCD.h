/*
 * LCD.h
 *
 * Created: 16/07/2026 18:54:33
 *  Author: Rodrigo García
 */ 
/*
 * LCD.h
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Librería para el manejo de LCD 16x2 (HD44780) en modo de 8 bits, para el microcontrolador Maestro del sistema de parqueo inteligente.
 */

#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>

// Pines de control (en PORTD) -> RS = D4, E = D5
#define LCD_CTRL_DDR   DDRD
#define LCD_CTRL_PORT  PORTD
#define LCD_RS         PD4
#define LCD_E          PD5

// Pines de datos
// D0-D1 en PORTD (PD6, PD7)  -> corresponden a los pines Arduino D6, D7
#define LCD_DATA_LOW_DDR   DDRD
#define LCD_DATA_LOW_PORT  PORTD

// D2-D7 en PORTB (PB0-PB5)   -> corresponden a los pines Arduino D8 a D13
#define LCD_DATA_HIGH_DDR  DDRB
#define LCD_DATA_HIGH_PORT PORTB

void LCD_init(void);
void LCD_command(uint8_t cmd);
void LCD_data(uint8_t data);
void LCD_string(const char *str);
void LCD_gotoxy(uint8_t col, uint8_t row);
void LCD_clear(void);

#endif /* LCD_H_ */