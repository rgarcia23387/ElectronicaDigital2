/*
 * Laboratorio1Digital2.c
 *
 * Created: 9/07/2026 17:32:30
 * Author: Rodrigo García
 * Description: Juego de carreras - Parte 1: boton de inicio con cuenta regresiva.
 *									Parte 2: contadores de decada de los jugadores (antirrebote).
 *									Parte 3: deteccion del ganador (LEDs + display).
 */
/****************************************/
// Encabezado (Libraries)
/****************************************/
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "display.h"

// Defines

// Botones (PORTC)
#define BTN_INICIO      (1 << PC2)  // Pushbutton inicio de carrera
#define BTN_J1          (1 << PC3)  // Pushbutton jugador 1
#define BTN_J2          (1 << PC4)  // Pushbutton jugador 2
#define DEBOUNCE_COUNT  2           // Muestras estables para validar (10 ms)

// LEDs jugador 1 (D8-D11 = PB0-PB3)
#define LED_J1_MASK     0x0F

// LEDs jugador 2 (D12,D13 = PB4,PB5 / A0,A1 = PC0,PC1)
#define LED_J2_B_MASK   0x30   // PB4,PB5 (bits bajos del contador)
#define LED_J2_C_MASK   0x03   // PC0,PC1 (bits altos del contador)

// Meta de la carrera (4 LEDs por jugador = 4 pulsaciones)
#define META            4

// Timers
#define TIMER1_OCR      15624       // CTC, prescaler 1024 -> 1 Hz (segundero)
#define TIMER0_OCR      77          // CTC, prescaler 1024 -> ~5 ms (muestreo botones)

typedef enum { ESPERA = 0, CUENTA_REGRESIVA, CARRERA, FIN } estado_t;

typedef struct {
    uint8_t acumulador;
    uint8_t estable;
    uint8_t previo;
} boton_t;

/****************************************/
// Function prototypes
/****************************************/

// Inicializacion
void    initPuertos(void);
void    initTimer0(void);
void    initTimer1(void);

// LEDs
void    actualizarLEDsJ1(uint8_t valor);
void    actualizarLEDsJ2(uint8_t valor);

// Antirrebote
void    actualizarDebounce(volatile boton_t *b, uint8_t crudoPresionado);
uint8_t flancoPresion(volatile boton_t *b);

// Ganador
void    verificarGanador(void);

/****************************************/
// Variables
/****************************************/
volatile estado_t estado            = ESPERA;
volatile uint8_t  contadorRegresivo = 5;
volatile uint8_t  contJ1            = 0;
volatile uint8_t  contJ2            = 0;

volatile boton_t btnInicio = {0, 0, 0};
volatile boton_t btnJ1     = {0, 0, 0};
volatile boton_t btnJ2     = {0, 0, 0};

/****************************************/
// Main Function
/****************************************/
int main(void)
{
    initPuertos();
    display_init();
    initTimer0();
    initTimer1();

    sei();

    while (1)
    {
        // Logica manejada por interrupciones (Timer0 y Timer1)
    }
}

/****************************************/
// NON-Interrupt subroutines
/****************************************/

// Puertos
void initPuertos(void)
{
    DDRC  &= ~(BTN_INICIO | BTN_J1 | BTN_J2);   // botones como entrada
    PORTC |=  (BTN_INICIO | BTN_J1 | BTN_J2);   // pull-up interno

    DDRB  |= (LED_J1_MASK | LED_J2_B_MASK);     // PB0-PB5 - LEDs
    DDRC  |= LED_J2_C_MASK;                     // PC0-PC1 - LEDs (J2)

    PORTB &= ~(LED_J1_MASK | LED_J2_B_MASK);    // LEDs apagados
    PORTC &= ~LED_J2_C_MASK;
}

// Configuracion Timer1 
void initTimer1(void)
{
    TCCR1A = 0x00;
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC, prescaler 1024
    OCR1A  = TIMER1_OCR;
    TIMSK1 = (1 << OCIE1A);
}

// Configuracion Timer0 (muestreo de botones, 5 ms)
void initTimer0(void)
{
    TCCR0A = (1 << WGM01);                // CTC
    TCCR0B = (1 << CS02) | (1 << CS00);   // prescaler 1024
    OCR0A  = TIMER0_OCR;
    TIMSK0 = (1 << OCIE0A);
}

// Enciende LEDs del jugador 1 sin apagar los anteriores
void actualizarLEDsJ1(uint8_t valor)
{
    uint8_t patron = (1 << valor) - 1;   // valor=2 - 0b0011, valor=4 -> 0b1111
    PORTB = (PORTB & ~LED_J1_MASK) | (patron & LED_J1_MASK);
}

// Enciende LEDs del jugador 2 sin apagar los anteriores 
void actualizarLEDsJ2(uint8_t valor)
{
    uint8_t patron = (1 << valor) - 1;
    PORTB = (PORTB & ~LED_J2_B_MASK) | ((patron & 0x03) << 4);
    PORTC = (PORTC & ~LED_J2_C_MASK) | ((patron >> 2) & 0x03);
}

// Antirrebote por integrador: sube si esta presionado, baja si no
void actualizarDebounce(volatile boton_t *b, uint8_t crudoPresionado)
{
    if (crudoPresionado) {
        if (b->acumulador < DEBOUNCE_COUNT) b->acumulador++;
    } else {
        if (b->acumulador > 0) b->acumulador--;
    }

    if (b->acumulador >= DEBOUNCE_COUNT) b->estable = 1;
    else if (b->acumulador == 0)         b->estable = 0;
}

// Detecta flanco de liberado - presionado 
uint8_t flancoPresion(volatile boton_t *b)
{
    uint8_t flanco = (b->estable == 1 && b->previo == 0);
    b->previo = b->estable;
    return flanco;
}

// Revisa si algun jugador llego a la meta y declara ganador
void verificarGanador(void)
{
    if (contJ1 >= META) {
        estado = FIN;
        actualizarLEDsJ1(0x0F);
        actualizarLEDsJ2(0x00);
        display_mostrarDigito(1);
    } else if (contJ2 >= META) {
        estado = FIN;
        actualizarLEDsJ2(0x0F);
        actualizarLEDsJ1(0x00);
        display_mostrarDigito(2);
    }
}

/****************************************/
// Interrupt routines
/****************************************/

// ISR - Timer1 Compare Match A, cada 1 s: cuenta regresiva
ISR(TIMER1_COMPA_vect)
{
    if (estado == CUENTA_REGRESIVA) {
        display_mostrarDigito(contadorRegresivo);

        if (contadorRegresivo == 0) estado = CARRERA;
        else contadorRegresivo--;
    }
}

// ISR - Timer0 Compare Match A cada 5 ms: muestreo, antirrebote y logica de juego
ISR(TIMER0_COMPA_vect)
{
    uint8_t crudoInicio = !(PINC & BTN_INICIO);
    uint8_t crudoJ1     = !(PINC & BTN_J1);
    uint8_t crudoJ2     = !(PINC & BTN_J2);

    actualizarDebounce(&btnInicio, crudoInicio);
    actualizarDebounce(&btnJ1,     crudoJ1);
    actualizarDebounce(&btnJ2,     crudoJ2);

    uint8_t flancoInicio = flancoPresion(&btnInicio);
    uint8_t flancoJ1     = flancoPresion(&btnJ1);
    uint8_t flancoJ2     = flancoPresion(&btnJ2);

    // Boton de inicio: comportamiento depende del estado actual
    if (flancoInicio) {
        if (estado == FIN) {
            // Primera pulsacion tras terminar: solo reinicia todo
            contJ1 = 0;
            contJ2 = 0;
            actualizarLEDsJ1(0);
            actualizarLEDsJ2(0);
            display_apagar();
            estado = ESPERA;
        } else if (estado == ESPERA) {
            // Segunda pulsacion: arranca la cuenta regresiva
            contadorRegresivo = 5;
            contJ1 = 0;
            contJ2 = 0;
            actualizarLEDsJ1(0);
            actualizarLEDsJ2(0);
            estado = CUENTA_REGRESIVA;

            TCNT1 = 0;                       // reinicia el segundero
            display_mostrarDigito(5);       // muestra el 5
            contadorRegresivo = 4;          // ya se mostro el 5, el primer tick mostrara el 4
        }
    }

    // Botones de jugadores: solo cuentan durante la carrera
    if (estado == CARRERA) {
        if (flancoJ1) {
            contJ1++;
            actualizarLEDsJ1(contJ1);
        }
        if (flancoJ2) {
            contJ2++;
            actualizarLEDsJ2(contJ2);
        }
        verificarGanador();
    }
}