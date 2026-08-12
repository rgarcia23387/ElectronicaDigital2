/*
 * UART.c
 */

#include "UART.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

void UART_Init(uint32_t baud) {
	uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits, sin paridad, 1 stop
}

void UART_Putc(char c) {
	while (!(UCSR0A & (1 << UDRE0)));   // espera a que el buffer este libre
	UDR0 = c;
}

void UART_Print(const char *s) {
	while (*s) UART_Putc(*s++);
}

void UART_PrintNum(uint16_t n) {
	char buf[6];
	uint8_t i = 0;

	if (n == 0) { UART_Putc('0'); return; }
	while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
	while (i > 0) UART_Putc(buf[--i]);  // se imprime al reves
}

void UART_PrintHex(uint8_t v) {
	const char *h = "0123456789ABCDEF";
	UART_Putc(h[(v >> 4) & 0x0F]);
	UART_Putc(h[v & 0x0F]);
	UART_Putc(' ');
}
