/*
 * Laboratorio2Digital.c
 *
 * Created: 16/07/2026 18:53:58
 * Author : Rodrigo García
 * Description: Lectura de dos potenciometros por ADC, envio de esas lecturas por UART.
 */ 

/****************************************/
// Encabezado (Libraries)
/****************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "LCD.h"

/****************************************/
// Function prototypes
/****************************************/
void ADC_init(void);
uint16_t ADC_read(uint8_t channel);
uint16_t ADC_read_avg(uint8_t channel);
void UART_init(void);
void UART_transmit_char(char data);
void UART_transmit_string(const char *str);

// Columnas de cada campo en el LCD 
#define S1_COL 0
#define S2_COL 6
#define S3_COL 11

// Contador, modificado desde la interrupcion de recepcion UART 
volatile int8_t contador = 0;

/****************************************/
// Main Function
/****************************************/
int main(void)
{
    uint16_t adc1, adc2;
    uint16_t milivolts;
    uint8_t entero, decimal;
    char buf1[8], buf2[8], buf3[8];
    char uart_line[24];

    LCD_init();
    ADC_init();
    UART_init();
    sei(); // habilitar interrupciones globales 

    LCD_gotoxy(0, 0);
    LCD_string("Lab2 - LCD/UART");
    _delay_ms(1000);
    LCD_clear();

    // Etiqueta, se escriben una sola vez en la fila 0
    LCD_gotoxy(S1_COL, 0);
    LCD_string("S1:");
    LCD_gotoxy(S2_COL, 0);
    LCD_string("S2:");
    LCD_gotoxy(S3_COL, 0);
    LCD_string("S3:");

    while (1)
    {
        // Lectura de potenciometros 
        adc1 = ADC_read_avg(0); // Pot1 en A0 
        adc2 = ADC_read_avg(1); // Pot2 en A1 

        // S1: voltaje = adc1 * 5000 / 1023  
        milivolts = (uint32_t)adc1 * 5000UL / 1023UL;
        entero  = milivolts / 1000;
        decimal = (milivolts % 1000) / 10;
        sprintf(buf1, "%u.%02uV ", entero, decimal); 

        // S2: valor crudo del ADC (0-1023)
        sprintf(buf2, "%-4u ", adc2); 

        // S3: contador actualizado por UART
        sprintf(buf3, "%-4d ", contador);

        // Actualizar LCD 
        LCD_gotoxy(S1_COL, 1);
        LCD_string(buf1);
        LCD_gotoxy(S2_COL, 1);
        LCD_string(buf2);
        LCD_gotoxy(S3_COL, 1);
        LCD_string(buf3);

        // Enviar lecturas por UART 
        sprintf(uart_line, "S1:%s S2:%u\r\n", buf1, adc2);
        UART_transmit_string(uart_line);

        _delay_ms(300);
    }

    return 0;
}

/****************************************/
// NON-Interrupt subroutines
/****************************************/
void ADC_init(void)
{
    // Referencia AVcc (5V)
    ADMUX = (1 << REFS0);

    // Habilitar ADC, prescaler de 128 - 16MHz/128 = 125kHz 
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_read(uint8_t channel)
{
    // Seleccionar canal (0-7)
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    // Iniciar conversion 
    ADCSRA |= (1 << ADSC);

    // Esperar a que termine
    while (ADCSRA & (1 << ADSC));

    return ADC;
}

uint16_t ADC_read_avg(uint8_t channel)
{
    uint32_t suma = 0;
    uint8_t i;

    // Promediar 16 muestras para reducir el ruido
    for (i = 0; i < 16; i++)
        suma += ADC_read(channel);

    return (uint16_t)(suma / 16);
}

void UART_init(void)
{
    // Baud rate: 9600 - 16MHz
    uint16_t ubrr = (F_CPU / 16UL / 9600UL) - 1;

    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Habilitar transmisor, receptor e interrupcion de recepcion
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

    // 8 bits de datos, 1 bit de parada, sin paridad
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit_char(char data)
{
    // Esperar a que el buffer de transmision este vacio
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void UART_transmit_string(const char *str)
{
    while (*str)
    {
        UART_transmit_char(*str);
        str++;
    }
}

/****************************************/
// Interrupt routines
/****************************************/
ISR(USART_RX_vect)
{
    char recibido = UDR0;

    if (recibido == '+')
    {
        if (contador < 99)
            contador++;
    }
    else if (recibido == '-')
    {
        if (contador > 0)
            contador--;
    }
}