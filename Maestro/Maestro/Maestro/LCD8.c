/*
 * LCD8.c
 */

#include "LCD8.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <util/delay.h>

static void lcd_pulso_enable(void) {
	PORTB |= (1 << LCD_E);
	_delay_us(1);
	PORTB &= ~(1 << LCD_E);
	_delay_us(50);                                  // el LCD necesita este respiro
}

static void lcd_enviar(uint8_t dato, uint8_t es_dato) {
	if (es_dato) PORTB |= (1 << LCD_RS);
	else         PORTB &= ~(1 << LCD_RS);

	PORTD = (PORTD & 0x03) | (dato << 2);           // conserva PD0/PD1 (UART)
	PORTB = (PORTB & ~0x03) | ((dato >> 6) & 0x03); // bits D6 y D7 del dato
	lcd_pulso_enable();
}

void LCD_Cmd(uint8_t c) {
	lcd_enviar(c, 0);
	if (c == 0x01 || c == 0x02) _delay_ms(2);       // clear y home son lentos
}

void LCD_Dato(uint8_t d) { lcd_enviar(d, 1); }

void LCD_String(const char *s) {
	while (*s) LCD_Dato((uint8_t)*s++);
}

void LCD_Gotoxy(uint8_t col, uint8_t fila) {
	LCD_Cmd(0x80 | (fila ? (0x40 + col) : col));    // 0x40 = inicio de la 2a linea
}

void LCD_Clear(void) { LCD_Cmd(0x01); }

void LCD_CrearChar(uint8_t pos, const uint8_t *mapa) {
	LCD_Cmd(0x40 | (pos << 3));                     // direccion en la CGRAM
	for (uint8_t i = 0; i < 8; i++) LCD_Dato(mapa[i]);
	LCD_Cmd(0x80);                                  // regresa a la DDRAM
}

void LCD_Init(void) {
	DDRB |= (1 << LCD_RS) | (1 << LCD_E) | (1 << PB0) | (1 << PB1);
	DDRD |= 0xFC;                                   // PD2..PD7 salida, PD0/PD1 para el UART

	_delay_ms(50);                                  // espera de encendido del LCD
	LCD_Cmd(0x30);
	_delay_ms(5);
	LCD_Cmd(0x30);
	_delay_us(150);
	LCD_Cmd(0x30);
	_delay_us(150);

	LCD_Cmd(0x38);   // 8 bits, 2 lineas, fuente 5x8
	LCD_Cmd(0x08);   // display apagado
	LCD_Cmd(0x01);   // limpiar
	_delay_ms(2);
	LCD_Cmd(0x06);   // el cursor avanza a la derecha
	LCD_Cmd(0x0C);   // display encendido, sin cursor
}
