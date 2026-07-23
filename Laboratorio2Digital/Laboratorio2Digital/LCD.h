/*
 * LCD.h
 *
 * Created: 16/07/2026 18:54:33
 *  Author: Rodrigo García
 */ 


#ifndef LCD_H_
#define LCD_H_


#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <util/delay.h>

//Pines de control (en PORTB)
#define LCD_CTRL_DDR   DDRB
#define LCD_CTRL_PORT  PORTB
#define LCD_E          PB2
#define LCD_RS         PB3

// Pines de datos 
#define LCD_DATA_LOW_DDR   DDRD
#define LCD_DATA_LOW_PORT  PORTD
#define LCD_DATA_HIGH_DDR  DDRB
#define LCD_DATA_HIGH_PORT PORTB

void LCD_init(void);
void LCD_command(uint8_t cmd);
void LCD_data(uint8_t data);
void LCD_string(const char *str);
void LCD_gotoxy(uint8_t col, uint8_t row);
void LCD_clear(void);

#endif /* LCD_H_ */