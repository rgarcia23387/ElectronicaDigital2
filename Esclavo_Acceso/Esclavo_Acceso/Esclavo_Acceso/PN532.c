/*
 * PN532.c
 */

#include "PN532.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <util/delay.h>

#define PN532_SS  PB2   // D10
#define PN532_RST PC0   // A0

/* ---------- SPI de hardware ---------- */

static void spi_init(void) {
	DDRB |= (1 << PB3) | (1 << PB5) | (1 << PN532_SS);  // MOSI, SCK, SS
	DDRB &= ~(1 << PB4);                                // MISO
	PORTB |= (1 << PN532_SS);                           // SS inactivo

	// DORD=1 porque el PN532 espera los bits LSB primero
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << DORD) | (1 << SPR0);
}

static uint8_t spi_transfer(uint8_t d) {
	SPDR = d;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

/* El PN532 necesita ~2 ms para despertar tras bajar el SS */
static void cs_low(void) {
	PORTB &= ~(1 << PN532_SS);
	_delay_ms(2);
}

static void cs_high(void) {
	_delay_us(10);
	PORTB |= (1 << PN532_SS);
}

/* ---------- Tramas del PN532 ---------- */

static uint8_t wait_ready(uint16_t timeout_ms) {
	uint16_t t = 0;

	while (t < timeout_ms) {
		uint8_t st;
		cs_low();
		spi_transfer(0x02);          // comando SPI: leer estado
		st = spi_transfer(0x00);
		cs_high();

		if (st & 0x01) return 1;     // ya tiene respuesta lista
		_delay_ms(1);
		t++;
	}
	return 0;
}

static void write_frame(const uint8_t *data, uint8_t len) {
	uint8_t checksum = 0;

	cs_low();
	spi_transfer(0x01);              // comando SPI: escribir datos
	spi_transfer(0x00);              // preambulo
	spi_transfer(0x00);
	spi_transfer(0xFF);
	spi_transfer(len);
	spi_transfer((uint8_t)(~len + 1));   // complemento de la longitud

	for (uint8_t i = 0; i < len; i++) {
		spi_transfer(data[i]);
		checksum += data[i];
	}
	spi_transfer((uint8_t)(~checksum + 1));  // complemento del checksum
	spi_transfer(0x00);
	cs_high();
}

static void read_ack(void) {
	cs_low();
	spi_transfer(0x03);              // comando SPI: leer datos
	for (uint8_t i = 0; i < 6; i++) spi_transfer(0x00);
	cs_high();
}

static uint8_t read_response(uint8_t *buf, uint8_t maxlen) {
	uint8_t len;

	cs_low();
	spi_transfer(0x03);
	spi_transfer(0x00);              // preambulo
	spi_transfer(0x00);
	spi_transfer(0x00);
	len = spi_transfer(0x00);        // longitud de la respuesta
	spi_transfer(0x00);              // checksum de la longitud
	spi_transfer(0x00);              // TFI

	if (len == 0) { cs_high(); return 0; }

	for (uint8_t i = 0; i < (len - 1) && i < maxlen; i++)
		buf[i] = spi_transfer(0x00);

	spi_transfer(0x00);              // checksum de los datos
	spi_transfer(0x00);              // postambulo
	cs_high();
	return len - 1;
}

/* ---------- Funciones publicas ---------- */

uint8_t PN532_Init(void) {
	uint8_t cmd = 0x02;              // GetFirmwareVersion
	uint8_t resp[8];
	uint8_t len;

	spi_init();

	DDRC  |= (1 << PN532_RST);       // pulso de reset al modulo
	PORTC &= ~(1 << PN532_RST);
	_delay_ms(100);
	PORTC |= (1 << PN532_RST);
	_delay_ms(200);

	write_frame(&cmd, 1);
	_delay_ms(2);
	if (!wait_ready(200)) return 0;
	read_ack();
	if (!wait_ready(200)) return 0;
	len = read_response(resp, sizeof(resp));

	return (len >= 2 && resp[1] == 0x32);   // 0x32 identifica al PN532
}

uint8_t PN532_SamConfig(void) {
	uint8_t cmd[5] = {0xD4, 0x14, 0x01, 0x14, 0x01};
	uint8_t resp[8];

	write_frame(cmd, 5);
	_delay_ms(2);
	if (!wait_ready(1000)) return 0;
	read_ack();
	if (!wait_ready(1000)) return 0;
	read_response(resp, sizeof(resp));
	return 1;
}

uint8_t PN532_LeerUID(uint8_t *uid, uint8_t *uid_len, uint16_t timeout_ms) {
	uint8_t cmd[4] = {0xD4, 0x4A, 0x01, 0x00};   // busca 1 tarjeta tipo A
	uint8_t resp[20];
	uint8_t len;

	write_frame(cmd, 4);
	_delay_ms(2);
	if (!wait_ready(50)) return 0;
	read_ack();
	if (!wait_ready(timeout_ms)) return 0;       // no hubo tarjeta

	len = read_response(resp, sizeof(resp));
	if (len < 7 || resp[1] == 0) return 0;       // ninguna tarjeta detectada

	*uid_len = resp[6];
	if (*uid_len > 7) *uid_len = 7;
	for (uint8_t i = 0; i < *uid_len; i++) uid[i] = resp[7 + i];
	return 1;
}
