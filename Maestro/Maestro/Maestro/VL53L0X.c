/*
 * VL53L0X.c
 *
 * Secuencia de arranque y calibracion equivalente a
 * DataInit + StaticInit + PerformRefCalibration de la API de ST.
 */

#include "VL53L0X.h"
#include "I2C_Master.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <util/delay.h>

static uint8_t stop_var = 0;   // valor interno que pide el sensor en cada lectura

/* ---------- Acceso a registros ---------- */

static void vl_w8(uint8_t reg, uint8_t val) {
	I2C_Start();
	I2C_Write(VL53_ADDR << 1);
	I2C_Write(reg);
	I2C_Write(val);
	I2C_Stop();
}

static uint8_t vl_r8(uint8_t reg) {
	uint8_t v;
	I2C_Start();
	I2C_Write(VL53_ADDR << 1);
	I2C_Write(reg);
	I2C_Start();                     // repeated start para pasar a lectura
	I2C_Write((VL53_ADDR << 1) | 1);
	v = I2C_Read(0);
	I2C_Stop();
	return v;
}

static void vl_w16(uint8_t reg, uint16_t val) {
	I2C_Start();
	I2C_Write(VL53_ADDR << 1);
	I2C_Write(reg);
	I2C_Write((uint8_t)(val >> 8));
	I2C_Write((uint8_t)(val & 0xFF));
	I2C_Stop();
}

static uint16_t vl_r16(uint8_t reg) {
	uint16_t alto, bajo;
	I2C_Start();
	I2C_Write(VL53_ADDR << 1);
	I2C_Write(reg);
	I2C_Start();
	I2C_Write((VL53_ADDR << 1) | 1);
	alto = I2C_Read(1);
	bajo = I2C_Read(0);
	I2C_Stop();
	return (alto << 8) | bajo;
}

static void vl_rmulti(uint8_t reg, uint8_t *buf, uint8_t len) {
	I2C_Start();
	I2C_Write(VL53_ADDR << 1);
	I2C_Write(reg);
	I2C_Start();
	I2C_Write((VL53_ADDR << 1) | 1);
	for (uint8_t i = 0; i < len; i++) buf[i] = I2C_Read(i < (len - 1));
	I2C_Stop();
}

static void vl_wmulti(uint8_t reg, const uint8_t *buf, uint8_t len) {
	I2C_Start();
	I2C_Write(VL53_ADDR << 1);
	I2C_Write(reg);
	for (uint8_t i = 0; i < len; i++) I2C_Write(buf[i]);
	I2C_Stop();
}

/* ---------- Informacion de SPAD de fabrica ---------- */

static uint8_t vl_spad_info(uint8_t *count, uint8_t *aperture) {
	uint16_t t = 0;
	uint8_t tmp;

	vl_w8(0x80, 0x01); vl_w8(0xFF, 0x01); vl_w8(0x00, 0x00);
	vl_w8(0xFF, 0x06); vl_w8(0x83, vl_r8(0x83) | 0x04);
	vl_w8(0xFF, 0x07); vl_w8(0x81, 0x01); vl_w8(0x80, 0x01);
	vl_w8(0x94, 0x6B); vl_w8(0x83, 0x00);

	while (vl_r8(0x83) == 0x00) { _delay_ms(1); if (++t > 200) return 0; }

	vl_w8(0x83, 0x01);
	tmp = vl_r8(0x92);
	*count = tmp & 0x7F;
	*aperture = (tmp >> 7) & 0x01;

	vl_w8(0x81, 0x00); vl_w8(0xFF, 0x06);
	vl_w8(0x83, vl_r8(0x83) & ~0x04);
	vl_w8(0xFF, 0x01); vl_w8(0x00, 0x01);
	vl_w8(0xFF, 0x00); vl_w8(0x80, 0x00);
	return 1;
}

/* ---------- Calibracion de referencia (VHV y fase) ---------- */

static uint8_t vl_calib(uint8_t vhv) {
	uint16_t t = 0;

	vl_w8(0x00, 0x01 | vhv);
	while ((vl_r8(0x13) & 0x07) == 0) { _delay_ms(1); if (++t > 200) return 0; }
	vl_w8(0x0B, 0x01);
	vl_w8(0x00, 0x00);
	return 1;
}

/* ---------- Inicializacion ---------- */

uint8_t VL53L0X_Init(void) {
	uint8_t spad_count, spad_aperture, spad_map[6], primer, activos = 0;

	DDRC  |= (1 << PC1);      // XSHUT en A1
	PORTC &= ~(1 << PC1);     // apaga el sensor
	_delay_ms(10);
	PORTC |= (1 << PC1);      // lo enciende
	_delay_ms(10);

	if (vl_r8(0xC0) != 0xEE) return 0;   // no responde o no es un VL53L0X

	vl_w8(0x88, 0x00); vl_w8(0x80, 0x01); vl_w8(0xFF, 0x01); vl_w8(0x00, 0x00);
	stop_var = vl_r8(0x91);
	vl_w8(0x00, 0x01); vl_w8(0xFF, 0x00); vl_w8(0x80, 0x00);

	vl_w8(0x60, vl_r8(0x60) | 0x12);       // desactiva limites del MSRC
	vl_w16(0x44, (uint16_t)(0.25 * 128));  // limite de senal 0.25 MCPS
	vl_w8(0x01, 0xFF);

	if (!vl_spad_info(&spad_count, &spad_aperture)) return 0;
	vl_rmulti(0xB0, spad_map, 6);

	vl_w8(0xFF, 0x01); vl_w8(0x4F, 0x00); vl_w8(0x4E, 0x2C);
	vl_w8(0xFF, 0x00); vl_w8(0xB6, 0xB4);

	primer = spad_aperture ? 12 : 0;
	for (uint8_t i = 0; i < 48; i++) {      // deja activos solo los SPAD correctos
		uint8_t bi = i / 8, bit = i % 8;
		if (i < primer || activos == spad_count) spad_map[bi] &= ~(1 << bit);
		else if ((spad_map[bi] >> bit) & 0x01) activos++;
	}
	vl_wmulti(0xB0, spad_map, 6);

	vl_w8(0x0A, 0x04);
	vl_w8(0x84, vl_r8(0x84) & ~0x10);
	vl_w8(0x0B, 0x01);
	vl_w8(0x01, 0xE8);

	vl_w8(0x01, 0x01); if (!vl_calib(0x40)) return 0;
	vl_w8(0x01, 0x02); if (!vl_calib(0x00)) return 0;
	vl_w8(0x01, 0xE8);                     // restaura la secuencia normal
	return 1;
}

/* ---------- Lectura de un solo disparo ---------- */

uint16_t VL53L0X_LeerMM(void) {
	uint16_t t = 0, d;

	vl_w8(0x80, 0x01); vl_w8(0xFF, 0x01); vl_w8(0x00, 0x00);
	vl_w8(0x91, stop_var);
	vl_w8(0x00, 0x01); vl_w8(0xFF, 0x00); vl_w8(0x80, 0x00);

	vl_w8(0x00, 0x01);                     // arranca la medicion
	while (vl_r8(0x00) & 0x01) { _delay_ms(1); if (++t > 100) return 0xFFFF; }

	t = 0;
	while ((vl_r8(0x13) & 0x07) == 0) { _delay_ms(1); if (++t > 100) return 0xFFFF; }

	d = vl_r16(0x14 + 10);
	vl_w8(0x0B, 0x01);                     // limpia la bandera de interrupcion
	return d;
}
