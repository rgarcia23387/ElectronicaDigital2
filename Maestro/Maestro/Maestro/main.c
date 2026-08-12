/*
 * Maestro.c
 *
 * Author : Rodrigo García  
 * Description: Parqueo Inteligente
 */


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "LCD8.h"
#include "I2C_Master.h"
#include "VL53L0X.h"
#include "UART.h"
#include "Tiempo.h"

/* ---------- Direcciones de los esclavos ---------- */
#define DIR_ACCESO  0x08
#define DIR_PORTON  0x09

/* ---------- Bits de estado que reportan los esclavos ---------- */
#define ACC_PERMITIDO  0x02
#define ACC_DENEGADO   0x04
#define PORT_ABIERTO   0x01

/* ---------- Ajustes ---------- */
#define UMBRAL_CARRO_MM  300     // menos de esto = hay un carro
#define GAP_CONTEO_MS    1500UL  // evita contar dos veces el mismo carro

/* ---------- Dibujo del check para S2 ---------- */
const uint8_t CHAR_CHECK[8] = {
	0b00000, 0b00001, 0b00011, 0b10110,
	0b11100, 0b01000, 0b00000, 0b00000
};

uint16_t contador_carros = 0;
uint8_t  ultimo_acceso = 0;    // 0 sin lecturas, 1 permitido, 2 denegado
uint8_t  porton_abierto = 0;

static void lcd_encabezado(void) {
	LCD_Gotoxy(0, 0);
	LCD_String("S1   S2   S3    ");
}

static void lcd_actualizar(void) {
	uint16_t n = contador_carros;

	LCD_Gotoxy(0, 1);                          // S1: contador de 3 digitos
	LCD_Dato('0' + ((n / 100) % 10));
	LCD_Dato('0' + ((n / 10) % 10));
	LCD_Dato('0' + (n % 10));
	LCD_String("  ");

	LCD_Gotoxy(5, 1);                          // S2: resultado del acceso
	if (ultimo_acceso == 1)      LCD_Dato(0);  // caracter de check
	else if (ultimo_acceso == 2) LCD_Dato('X');
	else                         LCD_Dato('-');
	LCD_String("    ");

	LCD_Gotoxy(10, 1);                         // S3: estado del porton
	LCD_String(porton_abierto ? "O     " : "C     ");
}

int main(void) {
	uint8_t vl53_ok;
	uint8_t carro_anterior = 0, acceso_anterior = 0;
	uint32_t ultimo_conteo = 0;

	Tiempo_Init();
	UART_Init(9600);
	I2C_Init();
	LCD_Init();
	sei();

	LCD_CrearChar(0, CHAR_CHECK);

	LCD_Gotoxy(0, 0);
	LCD_String("Estacionamiento");
	LCD_Gotoxy(0, 1);
	LCD_String("Iniciando...");
	_delay_ms(1500);

	vl53_ok = VL53L0X_Init();

	LCD_Clear();
	if (!vl53_ok) {                    // avisa del error pero no se traba
		LCD_Gotoxy(0, 0);
		LCD_String("ERROR VL53L0X");
		LCD_Gotoxy(0, 1);
		LCD_String("Revisa SDA/SCL");
		_delay_ms(2000);
		LCD_Clear();
	}
	lcd_encabezado();

	while (1) {
		/* ---- S1: contar carros con el VL53L0X ---- */
		if (vl53_ok) {
			uint16_t mm = VL53L0X_LeerMM();
			uint8_t hay_carro = (mm != 0xFFFF && mm < UMBRAL_CARRO_MM);

			// suma solo cuando aparece un carro que antes no estaba
			if (hay_carro && !carro_anterior && (millis() - ultimo_conteo > GAP_CONTEO_MS)) {
				contador_carros++;
				ultimo_conteo = millis();
				UART_Print("CARRO:");
				UART_PrintNum(contador_carros);
				UART_Print("\n");
			}
			carro_anterior = hay_carro;
		}

		/* ---- S2: resultado del PN532 (Esclavo Acceso) ---- */
		uint8_t est_acceso = I2C_LeerEsclavo(DIR_ACCESO);

		if ((est_acceso & ACC_PERMITIDO) && !(acceso_anterior & ACC_PERMITIDO)) {
			ultimo_acceso = 1;
			UART_Print("ACCESO:OK\n");
		}
		if ((est_acceso & ACC_DENEGADO) && !(acceso_anterior & ACC_DENEGADO)) {
			ultimo_acceso = 2;
			UART_Print("ACCESO:X\n");
		}
		acceso_anterior = est_acceso;

		/* ---- S3: estado del porton (Esclavo Porton) ---- */
		porton_abierto = (I2C_LeerEsclavo(DIR_PORTON) & PORT_ABIERTO) ? 1 : 0;

		lcd_actualizar();
		_delay_ms(120);
	}
}
