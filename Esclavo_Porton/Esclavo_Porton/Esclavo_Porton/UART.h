/*
 * UART.h - Comunicacion serial 
 */

#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>

void UART_Init(uint32_t baud);          // arranca el UART al baudaje dado
void UART_Putc(char c);                 // envia un caracter
void UART_Print(const char *s);         // envia una cadena
void UART_PrintNum(uint16_t n);         // envia un numero en decimal
void UART_PrintHex(uint8_t v);          // envia un byte en hexadecimal

#endif /* UART_H_ */
