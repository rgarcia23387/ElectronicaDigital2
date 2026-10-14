/* storage.h - archivos del juego en la SD: graficos, fondo, niveles y records */
#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include <stdint.h>
#include "screens.h"

#define BK_W 320
#define BK_H 208                               /* hasta el pasto (GROUND_Y) */

/* Fondo cargado de la SD: 2 pixeles por byte (indice de paleta, nibble alto primero) */
extern uint8_t  bk_ok;                         /* 1 = fondo cargado */
extern uint16_t bk_pal[16];
extern uint8_t  bk_px[BK_W * BK_H / 2];

uint8_t storage_init(void);                    /* monta la SD y carga el fondo; 1 = SD OK */
uint8_t storage_ok(void);
/* Codigo de error de la SD para el menu: resultado de f_mount * 10 + paso */
uint16_t storage_error(void);

int  storage_load_level(uint8_t id, LevelDef *out);          /* 1 = leido de la SD */
void storage_load_records(uint16_t *best, uint8_t n);
void storage_save_records(const uint16_t *best, uint8_t n);

int  storage_load_sprites(void);                 /* SPRITES.BIN -> RAM; 1 = ok        */
int  storage_show_image(const char *name);       /* dibuja una imagen .BIN; 1 = ok   */
int  storage_load_text(const char *name, char *buf, int size);   /* 1 = ok          */

typedef struct {
    uint16_t partidas, victorias, derrotas;
    uint32_t puntos;                             /* suma de todos los puntajes */
} GameStats;
void storage_load_stats(GameStats *s);
void storage_save_stats(const GameStats *s);

#endif /* INC_STORAGE_H_ */
