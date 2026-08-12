/*
 * LCD.c
 *
 * Created: 16/07/2026 18:54:19
 *  Author: Rodrigo García
 */ 

/*
 * LCD.c
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Implementación de la librería LCD 16x2 (HD44780) en modo de 8 bits.
 */

#include "LCD.h"

// Escribe un byte completo en el bus de datos del LCD.
static void LCD_write_byte(uint8_t data)
{
    // D0-D1 -> PD6, PD7 (bits altos del puerto D)
    LCD_DATA_LOW_PORT  = (LCD_DATA_LOW_PORT & 0x3F) | ((data & 0x03) << 6);
    // D2-D7 -> PB0-PB5
    LCD_DATA_HIGH_PORT = (LCD_DATA_HIGH_PORT & 0xC0) | ((data >> 2) & 0x3F);
}

// Genera el pulso de Enable para que el LCD tome el dato/comando
static void LCD_pulse_enable(void)
{
    LCD_CTRL_PORT |= (1 << LCD_E);
    _delay_us(1);
    LCD_CTRL_PORT &= ~(1 << LCD_E);
    _delay_us(1);
}

void LCD_command(uint8_t cmd)
{
    LCD_CTRL_PORT &= ~(1 << LCD_RS); // RS = 0 - comando
    LCD_write_byte(cmd);
    LCD_pulse_enable();
    // Clear display y Cursor home necesitan mas tiempo
    if (cmd == 0x01 || cmd == 0x02)
        _delay_ms(2);
    else
        _delay_us(50);
}

void LCD_data(uint8_t data)
{
    LCD_CTRL_PORT |= (1 << LCD_RS);  // RS = 1 - dato
    LCD_write_byte(data);
    LCD_pulse_enable();
    _delay_us(50);
}

void LCD_string(const char *str)
{
    while (*str)
    {
        LCD_data((uint8_t)*str);
        str++;
    }
}

void LCD_gotoxy(uint8_t col, uint8_t row)
{
    uint8_t address = (row == 0) ? col : (0x40 + col);
    LCD_command(0x80 | address);
}

void LCD_clear(void)
{
    LCD_command(0x01);
}

void LCD_init(void)
{
    // Configurar todos los pines de datos y control como salida
    LCD_DATA_LOW_DDR  |= 0xC0; // PD6-PD7
    LCD_DATA_HIGH_DDR |= 0x3F; // PB0-PB5
    LCD_CTRL_DDR |= (1 << LCD_RS) | (1 << LCD_E);
    LCD_CTRL_PORT &= ~(1 << LCD_E);

    // Espera de encendido
    _delay_ms(20);

    // Secuencia de inicializacion para modo de 8 bits
    LCD_write_byte(0x30);
    LCD_CTRL_PORT &= ~(1 << LCD_RS);
    LCD_pulse_enable();
    _delay_ms(5);

    LCD_write_byte(0x30);
    LCD_pulse_enable();
    _delay_us(200);

    LCD_write_byte(0x30);
    LCD_pulse_enable();
    _delay_us(200);

    // Function set: 8 bits, 2 lineas, fuente 5x8
    LCD_command(0x38);
    // Display off
    LCD_command(0x08);
    // Clear display
    LCD_command(0x01);
    // Entry mode set: incremento, sin desplazamiento
    LCD_command(0x06);
    // Display on, cursor off, blink off
    LCD_command(0x0C);
}