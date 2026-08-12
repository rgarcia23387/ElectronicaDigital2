/*
 * LCD8.h - LCD 16x2 en paralelo de 8 bits
 */

#ifndef LCD8_H_
#define LCD8_H_

#include <avr/io.h>
#include <stdint.h>

#define LCD_RS PB3
#define LCD_E  PB2

void LCD_Init(void);
void LCD_Cmd(uint8_t c);                          // envia un comando
void LCD_Dato(uint8_t d);                         // envia un caracter
void LCD_String(const char *s);
void LCD_Gotoxy(uint8_t col, uint8_t fila);
void LCD_Clear(void);
void LCD_CrearChar(uint8_t pos, const uint8_t *mapa); // guarda un caracter propio (0-7)

#endif /* LCD8_H_ */
