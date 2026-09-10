/*
 * Laboratorio6Nano.c
 *
 * Created: 3/09/2026 19:14:59
 * Author : Rodrigo García
 */ 


#define F_CPU 16000000UL
#define UART_BAUD 9600UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define UART_UBRR \
    ((F_CPU / (16UL * UART_BAUD)) - 1UL)


// Botones conectados en PD2-PD7.

#define BOTON_ARRIBA       PD2
#define BOTON_ABAJO        PD3
#define BOTON_DERECHA      PD4
#define BOTON_IZQUIERDA    PD5
#define BOTON_ACCION_A     PD6
#define BOTON_ACCION_B     PD7

#define MASCARA_BOTONES ( \
    (1U << BOTON_ARRIBA)    | \
    (1U << BOTON_ABAJO)     | \
    (1U << BOTON_DERECHA)   | \
    (1U << BOTON_IZQUIERDA) | \
    (1U << BOTON_ACCION_A)  | \
    (1U << BOTON_ACCION_B)    \
)

static void UART0_Inicializar(void);
static void UART0_EnviarCaracter(char caracter);
static void UART0_EnviarCadena(const char *cadena);

static void Botones_Inicializar(void);
static void EnviarAcciones(uint8_t botones);


static void UART0_Inicializar(void)
{
    uint16_t divisor = (uint16_t)UART_UBRR;


    UCSR0A = 0;
    UBRR0H = (uint8_t)(divisor >> 8);
    UBRR0L = (uint8_t)divisor;
    UCSR0B = (1U << TXEN0);
    UCSR0C =
        (1U << UCSZ01) |
        (1U << UCSZ00);
}

static void UART0_EnviarCaracter(char caracter)
{
    while ((UCSR0A & (1U << UDRE0)) == 0)
    {

    }

    UDR0 = caracter;
}


static void UART0_EnviarCadena(const char *cadena)
{
    while (*cadena != '\0')
    {
        UART0_EnviarCaracter(*cadena);
        cadena++;
    }
}

// Configura PD2-PD7 como entradas con pull-up.

static void Botones_Inicializar(void)
{

// PD2-PD7 como entradas.

    DDRD &= (uint8_t)~MASCARA_BOTONES;
    PORTD |= MASCARA_BOTONES;
}

static void EnviarAcciones(uint8_t botones)
{
    if (botones & (1U << BOTON_ARRIBA))
    {
        UART0_EnviarCadena("U - ARRIBA\r\n");
    }

    if (botones & (1U << BOTON_ABAJO))
    {
        UART0_EnviarCadena("D - ABAJO\r\n");
    }

    if (botones & (1U << BOTON_DERECHA))
    {
        UART0_EnviarCadena("R - DERECHA\r\n");
    }

    if (botones & (1U << BOTON_IZQUIERDA))
    {
        UART0_EnviarCadena("L - IZQUIERDA\r\n");
    }

    if (botones & (1U << BOTON_ACCION_A))
    {
        UART0_EnviarCadena("A - ACCION A\r\n");
    }

    if (botones & (1U << BOTON_ACCION_B))
    {
        UART0_EnviarCadena("B - ACCION B\r\n");
    }
}

// Programa Principal
int main(void)
{
    uint8_t estado_anterior;
    uint8_t estado_actual;
    uint8_t botones_detectados;
    uint8_t botones_confirmados;

    UART0_Inicializar();
    Botones_Inicializar();

    estado_anterior =
        PIND & MASCARA_BOTONES;

    UART0_EnviarCadena("\r\n");
    UART0_EnviarCadena("============================\r\n");
    UART0_EnviarCadena("Esperando botones...\r\n");
    UART0_EnviarCadena("============================\r\n\r\n");

    while (1)
    {
// Leer Botones
	        estado_actual =
            PIND & MASCARA_BOTONES;
			
        botones_detectados =
            estado_anterior &
            (uint8_t)(~estado_actual) &
            MASCARA_BOTONES;

        if (botones_detectados != 0)
        {
            _delay_ms(30);

            estado_actual =
                PIND & MASCARA_BOTONES;
				
            botones_confirmados =
                botones_detectados &
                (uint8_t)(~estado_actual) &
                MASCARA_BOTONES;

            EnviarAcciones(botones_confirmados);
        }


        estado_anterior = estado_actual;

        _delay_ms(5);
    }

    return 0;
}