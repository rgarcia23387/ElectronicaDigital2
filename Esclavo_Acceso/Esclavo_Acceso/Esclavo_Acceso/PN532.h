/*
 * PN532.h - Lector RFID/NFC por SPI 
 */

#ifndef PN532_H_
#define PN532_H_

#include <avr/io.h>
#include <stdint.h>

uint8_t PN532_Init(void);         // 1 si el sensor contesta su version de firmware
uint8_t PN532_SamConfig(void);    // lo deja listo para leer tarjetas
uint8_t PN532_LeerUID(uint8_t *uid, uint8_t *uid_len, uint16_t timeout_ms);

#endif /* PN532_H_ */
