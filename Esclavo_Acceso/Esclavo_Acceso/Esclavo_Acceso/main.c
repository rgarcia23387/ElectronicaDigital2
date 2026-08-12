/*
 * ESCLAVO ACCESO - PN532 + Servo + Motor DC (L298N)
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "PN532.h"
#include "Servo.h"
#include "MotorDC.h"
#include "I2C_Slave.h"
#include "UART.h"
#include "Tiempo.h"

#define I2C_DIRECCION 0x08

/* ---------- Bits que se le reportan al Maestro ---------- */
#define BIT_SERVO_ABIERTO 0x01
#define BIT_PERMITIDO     0x02
#define BIT_DENEGADO      0x04

/* ---------- Ajustes ---------- */
#define ANGULO_ABIERTO  90
#define ANGULO_CERRADO  0
#define TIEMPO_ABIERTO  3000UL   // el servo se queda abierto 3 segundos
#define TIEMPO_AVISO    2500UL   // cuanto dura el aviso hacia el Maestro
#define GAP_LECTURAS    800UL    // evita releer la misma tarjeta seguido

// ---------- Tarjetas autorizadas ----------

#define AUTO_REGISTRO 1

#define NUM_TARJETAS 2
const uint8_t tarjetas[NUM_TARJETAS][7] = {
	{0xDE, 0xAD, 0xBE, 0xEF, 0, 0, 0},   // cambiar por un UID real
	{0x12, 0x34, 0x56, 0x78, 0, 0, 0}    // cambiar por un UID real
};
const uint8_t tarjetas_len[NUM_TARJETAS] = {4, 4};

uint8_t uid_registrado[7];
uint8_t uid_registrado_len = 0;
uint8_t hay_registro = 0;

static uint8_t es_autorizada(const uint8_t *uid, uint8_t len) {
#if AUTO_REGISTRO
	if (!hay_registro || len != uid_registrado_len) return 0;
	for (uint8_t i = 0; i < len; i++)
		if (uid[i] != uid_registrado[i]) return 0;
	return 1;
#else
	for (uint8_t t = 0; t < NUM_TARJETAS; t++) {
		if (tarjetas_len[t] != len) continue;
		uint8_t igual = 1;
		for (uint8_t i = 0; i < len; i++)
			if (tarjetas[t][i] != uid[i]) { igual = 0; break; }
		if (igual) return 1;
	}
	return 0;
#endif
}

/* ---------- Estado ---------- */
uint8_t  servo_abierto = 0;
uint32_t tiempo_apertura = 0;
uint8_t  aviso_permitido = 0, aviso_denegado = 0;
uint32_t tiempo_aviso = 0;
uint32_t ultima_lectura = 0;

static void abrir_talanquera(void) {
	UART_Print(">> Acceso PERMITIDO, servo a 90\r\n");
	Servo_Angulo(ANGULO_ABIERTO);
	servo_abierto = 1;
	tiempo_apertura = millis();

	aviso_permitido = 1;
	aviso_denegado = 0;
	tiempo_aviso = millis();
}

int main(void) {
	uint8_t uid[7], uid_len;

	Tiempo_Init();
	UART_Init(9600);
	Servo_Init();
	Motor_Init();
	I2C_Slave_Init(I2C_DIRECCION);
	sei();

	Servo_Angulo(ANGULO_CERRADO);

	UART_Print("=== ESCLAVO ACCESO ===\r\n");

	/* Prueba del motor: si aqui no se mueve, el problema es de cableado */
	UART_Print("Probando motor...\r\n");
	Motor_Subir(); _delay_ms(1000);
	Motor_Bajar(); _delay_ms(1000);
	Motor_Parar();

	if (PN532_Init()) {
		UART_Print("PN532 OK\r\n");
		PN532_SamConfig();
	} else {
		UART_Print("ERROR: PN532 no encontrado (switch en SPI? cableado?)\r\n");
	}

	UART_Print("Sistema listo\r\n");

	while (1) {
		Motor_RevisarBotones();          // primero, para que respondan rapido

		/* ---- Lectura de tarjeta ---- */
		if (millis() - ultima_lectura >= GAP_LECTURAS) {
			if (PN532_LeerUID(uid, &uid_len, 50)) {
				ultima_lectura = millis();

				UART_Print("Tarjeta: ");
				for (uint8_t i = 0; i < uid_len; i++) UART_PrintHex(uid[i]);
				UART_Print("\r\n");

#if AUTO_REGISTRO
				if (!hay_registro) {     // la primera tarjeta queda guardada
					for (uint8_t i = 0; i < uid_len; i++) uid_registrado[i] = uid[i];
					uid_registrado_len = uid_len;
					hay_registro = 1;
					UART_Print("Tarjeta REGISTRADA\r\n");
					abrir_talanquera();
				} else
#endif
				if (es_autorizada(uid, uid_len)) {
					abrir_talanquera();
				} else {
					UART_Print(">> Acceso DENEGADO\r\n");
					aviso_denegado = 1;
					aviso_permitido = 0;
					tiempo_aviso = millis();
				}
			}
		}

		/* ---- El servo regresa a 0 a los 3 segundos ---- */
		if (servo_abierto && (millis() - tiempo_apertura >= TIEMPO_ABIERTO)) {
			UART_Print(">> Servo de regreso a 0\r\n");
			Servo_Angulo(ANGULO_CERRADO);
			servo_abierto = 0;
		}

		/* ---- Se apagan los avisos al Maestro ---- */
		if ((aviso_permitido || aviso_denegado) && (millis() - tiempo_aviso >= TIEMPO_AVISO)) {
			aviso_permitido = 0;
			aviso_denegado = 0;
		}

		/* ---- Estado que lee el Maestro ---- */
		i2c_estado = (servo_abierto   ? BIT_SERVO_ABIERTO : 0) |
		             (aviso_permitido ? BIT_PERMITIDO     : 0) |
		             (aviso_denegado  ? BIT_DENEGADO      : 0);
	}
}
