/* controller.h - recibe el mando por USART3 y le devuelve el estado del juego */
#ifndef INC_CONTROLLER_H_
#define INC_CONTROLLER_H_

#include <stdint.h>

#define PAD_BTN_FIRE   0x01     /* boton DISPARO                  */
#define PAD_BTN_POWER  0x02     /* boton PODER del pajaro         */
#define PAD_BTN_MENU   0x04     /* clic del joystick (salir)      */

#define PAD_TIMEOUT_MS 300      /* sin tramas por este tiempo = mando desconectado */

typedef struct {
    int8_t   jx, jy;            /* -100..100 (+ derecha, + arriba) */
    uint8_t  btn;               /* botones presionados ahora       */
    uint8_t  connected;         /* 1 = llegan tramas validas       */
    uint8_t  seq;               /* contador del ultimo paquete     */
    uint32_t frames;            /* tramas validas recibidas (para depurar) */
    uint32_t bad_frames;        /* tramas con checksum malo        */
} PadState;

void            pad_init(void);              /* inicia la recepcion por USART3  */
void            pad_poll(void);              /* llamar en cada vuelta del main  */
const PadState *pad_get(void);
uint8_t         pad_take_presses(void);      /* botones que se presionaron desde la ultima llamada */
int8_t          pad_nav_y(void);             /* menu: -1 arriba, +1 abajo, 0 nada (por flanco)     */
void            pad_send_status(uint8_t state, uint8_t flags);   /* retroalimentacion al mando */

#endif /* INC_CONTROLLER_H_ */
