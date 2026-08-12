/*
 * VL53L0X.h - Sensor de distancia por tiempo de vuelo
 */

#ifndef VL53L0X_H_
#define VL53L0X_H_

#include <avr/io.h>
#include <stdint.h>

#define VL53_ADDR 0x29

uint8_t  VL53L0X_Init(void);      // 1 si el sensor respondio y calibro
uint16_t VL53L0X_LeerMM(void);    // distancia en mm, 0xFFFF si hay timeout

#endif /* VL53L0X_H_ */
