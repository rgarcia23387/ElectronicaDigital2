/* controller.c - recepcion del mando por UART (USART3: PC10 TX, PC11 RX) */
#include "main.h"
#include "controller.h"

extern UART_HandleTypeDef huart3;           /* lo genera CubeMX en main.c */

#define RB_SIZE 256                         /* buffer circular: cabe ~0.7 s de tramas si el main se ocupa */

static volatile uint8_t  rb[RB_SIZE];
static volatile uint16_t rb_head = 0, rb_tail = 0;
static uint8_t           rx_byte;

static PadState pad;
static uint8_t  prev_btn = 0, press_latch = 0;
static uint32_t last_frame_ms = 0;
static uint8_t  nav_armed = 1;

/* ---------------------------------------------------------------------- */
/* Interrupciones de UART (se ejecuta por cada byte recibido)              */
/* ---------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        uint16_t next = (rb_head + 1) % RB_SIZE;
        if (next != rb_tail) {              /* si el buffer se llena se descarta el byte */
            rb[rb_head] = rx_byte;
            rb_head = next;
        }
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }
}

/* Si hay un error (ej. desbordamiento o ruido al conectar cables) se rearma la recepcion */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }
}

void pad_init(void)
{
    pad.jx = pad.jy = 0; pad.btn = 0; pad.connected = 0;
    pad.seq = 0; pad.frames = 0; pad.bad_frames = 0;
    HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
}

/* ---------------------------------------------------------------------- */
/* Analisis de tramas: 0xAA 0x55 jx jy botones seq chk                     */
/* ---------------------------------------------------------------------- */
static void parse_byte(uint8_t b)
{
    static uint8_t st = 0;
    static uint8_t f[5];                    /* jx jy btn seq chk */

    switch (st) {
    case 0: if (b == 0xAA) st = 1; break;
    case 1:
        if (b == 0x55)      st = 2;
        else if (b != 0xAA) st = 0;         /* si llega otro 0xAA se queda esperando 0x55 */
        break;
    default:                                /* st = 2..6 : datos y checksum */
        f[st - 2] = b;
        if (++st == 7) {
            st = 0;
            if ((uint8_t)(f[0] ^ f[1] ^ f[2] ^ f[3]) == f[4]) {   /* trama valida */
                pad.jx  = (int8_t)f[0];
                pad.jy  = (int8_t)f[1];
                pad.btn = f[2];
                pad.seq = f[3];
                pad.frames++;
                press_latch |= (uint8_t)(pad.btn & ~prev_btn);    /* flancos de subida */
                prev_btn = pad.btn;
                pad.connected = 1;
                last_frame_ms = HAL_GetTick();
            } else {
                pad.bad_frames++;
            }
        }
        break;
    }
}

void pad_poll(void)
{
    while (rb_tail != rb_head) {
        uint8_t b = rb[rb_tail];
        rb_tail = (rb_tail + 1) % RB_SIZE;
        parse_byte(b);
    }
    if (pad.connected && (HAL_GetTick() - last_frame_ms) > PAD_TIMEOUT_MS) {
        pad.connected = 0;                  /* se perdio el enlace: se sueltan los controles */
        pad.jx = pad.jy = 0;
        pad.btn = 0;
        prev_btn = 0;
    }
}

const PadState *pad_get(void) { return &pad; }

uint8_t pad_take_presses(void)
{
    uint8_t p = press_latch;
    press_latch = 0;
    return p;
}

/* Navegacion de menu con el joystick: un evento por cada vez que se empuja */
int8_t pad_nav_y(void)
{
    if (!pad.connected) return 0;
    if (nav_armed) {
        if (pad.jy >  50) { nav_armed = 0; return -1; }     /* arriba */
        if (pad.jy < -50) { nav_armed = 0; return  1; }     /* abajo  */
    } else if (pad.jy > -25 && pad.jy < 25) {
        nav_armed = 1;                                      /* volvio al centro */
    }
    return 0;
}

void pad_send_status(uint8_t state, uint8_t flags)
{
    uint8_t fr[5] = { 0xAA, 0x56, state, flags, (uint8_t)(state ^ flags) };
    HAL_UART_Transmit(&huart3, fr, sizeof(fr), 10);
}
