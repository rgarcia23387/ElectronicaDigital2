/* game.h - logica del juego (apuntado, vuelo, poderes, colisiones, puntaje) */
#ifndef INC_GAME_H_
#define INC_GAME_H_

#include <stdint.h>

#define GAME_FRAME_MS 33            /* ~30 cuadros por segundo */

typedef enum { GAME_RUNNING = 0, GAME_WIN, GAME_LOSE, GAME_QUIT } GameResult;

/* Entradas que recibe el juego en cada cuadro */
typedef struct {
    int8_t  jx, jy;        /* joystick -100..100 (+ derecha, + arriba)           */
    uint8_t connected;     /* 1 si el mando esta enlazado                        */
    uint8_t ev;            /* boton B1 de la Nucleo: 0 nada, 1 corto, 2 largo    */
    uint8_t fire;          /* 1 = se presiono DISPARO (mando)                    */
    uint8_t power;         /* 1 = se presiono PODER (mando)                      */
    uint8_t quit;          /* 1 = clic / Start: salir al menu                    */
} GameInput;

void       game_start(uint8_t level);          /* dibuja la escena e inicia      */
GameResult game_update(const GameInput *in);   /* llamar cada GAME_FRAME_MS      */
uint16_t   game_score(void);
uint8_t    game_stars(void);

/* Estado para enviar al mando (retroalimentacion) */
uint8_t    game_status_state(void);            /* 1 apuntando, 2 vuelo, 3 derrumbe */
uint8_t    game_status_flags(void);            /* bit0 = poder disponible          */

#endif /* INC_GAME_H_ */
