/* screens.h - niveles, escena, menu, records y pantallas de fin (solo dibujo) */
#ifndef INC_SCREENS_H_
#define INC_SCREENS_H_

#include <stdint.h>

#define GAME_TITLE     "ANGRY BIRDS"
#define CREDITS_AUTHOR "RODRIGO GARCIA"
#define MENU_ITEMS     4

/* Nivel: cuadricula de 20 columnas x 15 filas, tiles de 16x16 px */
#define LV_COLS    20
#define LV_ROWS    15
#define GROUND_Y   208
#define NUM_LEVELS 5
#define MAX_BIRDS  8

/* Codigos de tile */
enum { T_EMPTY = 0, T_WOOD, T_STONE, T_PIG, T_GRASS, T_DIRT, T_HELMET, T_BOSS, T_COUNT };

/* Tipos de pajaro */
typedef enum { BT_RED = 0, BT_YELLOW, BT_BLACK, BT_BLUE, BT_BIG, BT_COUNT } BirdType;

typedef struct {
    uint8_t  tiles[LV_ROWS][LV_COLS];
    uint8_t  birds[MAX_BIRDS];           /* cola de pajaros */
    uint8_t  nbirds;
    uint16_t star2, star3;               /* puntaje para 2 y 3 estrellas */
} LevelDef;

extern uint8_t g_level[LV_ROWS][LV_COLS];   /* nivel actual (RAM) */
extern uint8_t g_hp[LV_ROWS][LV_COLS];      /* vida de cada tile  */
extern const uint8_t tile_hp[T_COUNT];      /* vida inicial por tipo */

/* Carga el nivel (1..NUM_LEVELS). Luego se puede reemplazar por la SD. */
const LevelDef *level_load(uint8_t id);

/* Redibuja una zona de la escena (cielo, nubes, tiles, honda) */
void scene_restore(int x, int y, int w, int h);

/* Pantalla de creditos: texto de la SD, o NULL para usar el del programa */
void screens_credits(const char *txt);

/* Menu de inicio */
void screens_menu_full(uint8_t sel, uint8_t level);
void screens_menu_select(uint8_t old_sel, uint8_t new_sel);
void screens_menu_level(uint8_t level, uint8_t sel);

/* Records: mejor puntaje de cada nivel + estadisticas guardadas en la SD */
void screens_records(const uint16_t *best, uint16_t partidas, uint16_t victorias, uint16_t derrotas);

/* Fin de nivel / fin de juego */
void screens_win(uint8_t stars, uint16_t score, uint8_t new_record);
void screens_gameover(uint16_t score);

/* Galeria con todos los pajaros y cerdos ampliados x2 */
void screens_gallery(void);

#endif /* INC_SCREENS_H_ */
