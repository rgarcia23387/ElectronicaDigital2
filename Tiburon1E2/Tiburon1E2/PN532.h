/*
 * PN532.h
 *
 * Created: 10/08/2026 02:19:58
 *  Author: dinog
 */ 


#ifndef PN532_H_
#define PN532_H_


/*
 * PN532.h
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Driver del lector NFC PN532 sobre SPI, a nivel de
 *              registros AVR. Pin RESET = D8.
 */


#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <util/delay.h>
#include "SPI.h"

#define PN532_RESET_DDR   DDRB
#define PN532_RESET_PORT  PORTB
#define PN532_RESET_PIN   PB0  // D8

void PN532_init(void);
uint8_t PN532_sam_config(void);
uint8_t PN532_leer_uid(uint8_t *uid, uint8_t *uid_len, uint16_t timeout_ms);



#endif /* PN532_H_ */