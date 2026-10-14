/* sprites.h - generado por herramientas/sprites_personajes.py
 * Formato RGB565, transparente = 0xF81F (magenta).
 * Los arreglos viven en RAM: al encender se llenan desde la SD (SPRITES.BIN)
 * o, si no hay SD, desde la copia de respaldo en flash (ver sprites_cfg.h).
 * Pajaros: spr_bird_<tipo>_0 honda, _1 vuelo, _2 choque (_3 bomba al rojo). */
#ifndef INC_SPRITES_H_
#define INC_SPRITES_H_

#include <stdint.h>
#include "sprites_cfg.h"

#define SPR_TRANSPARENT 0xF81F

/* Tabla de sprites (mismo orden que SPRITES.BIN) */
typedef struct { uint16_t *dst; uint16_t n; } SpriteSlot;
extern const SpriteSlot sprite_slots[];
#define SPRITE_SLOTS 19
void sprites_from_flash(void);     /* copia el respaldo de la flash (si existe) */

#define BIRD_RED_0_W 16
#define BIRD_RED_0_H 16
extern uint16_t spr_bird_red_0[256];

#define BIRD_YELLOW_0_W 16
#define BIRD_YELLOW_0_H 16
extern uint16_t spr_bird_yellow_0[256];

#define BIRD_BLACK_0_W 16
#define BIRD_BLACK_0_H 16
extern uint16_t spr_bird_black_0[256];

#define BIRD_BLUE_0_W 12
#define BIRD_BLUE_0_H 12
extern uint16_t spr_bird_blue_0[144];

#define BIRD_BIG_0_W 20
#define BIRD_BIG_0_H 20
extern uint16_t spr_bird_big_0[400];

#define PIG_W 16
#define PIG_H 16
extern uint16_t spr_pig[256];

#define PIG_HURT_W 16
#define PIG_HURT_H 16
extern uint16_t spr_pig_hurt[256];

#define PIG_HELMET_W 16
#define PIG_HELMET_H 16
extern uint16_t spr_pig_helmet[256];

#define PIG_HELMET_DMG_W 16
#define PIG_HELMET_DMG_H 16
extern uint16_t spr_pig_helmet_dmg[256];

#define BLOCK_WOOD_W 16
#define BLOCK_WOOD_H 16
extern uint16_t spr_block_wood[256];

#define BLOCK_WOOD_DMG_W 16
#define BLOCK_WOOD_DMG_H 16
extern uint16_t spr_block_wood_dmg[256];

#define BLOCK_STONE_W 16
#define BLOCK_STONE_H 16
extern uint16_t spr_block_stone[256];

#define BLOCK_STONE_DMG_W 16
#define BLOCK_STONE_DMG_H 16
extern uint16_t spr_block_stone_dmg[256];

#define TILE_GRASS_W 16
#define TILE_GRASS_H 16
extern uint16_t spr_tile_grass[256];

#define TILE_DIRT_W 16
#define TILE_DIRT_H 16
extern uint16_t spr_tile_dirt[256];

#define SLINGSHOT_W 16
#define SLINGSHOT_H 32
extern uint16_t spr_slingshot[512];

#define STAR_FULL_W 16
#define STAR_FULL_H 16
extern uint16_t spr_star_full[256];

#define STAR_EMPTY_W 16
#define STAR_EMPTY_H 16
extern uint16_t spr_star_empty[256];

#define CLOUD_W 32
#define CLOUD_H 16
extern uint16_t spr_cloud[512];

/* Medidas comunes de los cuadros de cada pajaro */
#define BIRD_RED_W 16
#define BIRD_RED_H 16
#define BIRD_RED_FRAMES 3
#define spr_bird_red spr_bird_red_0
#define spr_bird_red_1 spr_bird_red_0
#define spr_bird_red_2 spr_bird_red_0
#define spr_icon_red spr_bird_red_0
#define ICON_RED_W 16
#define ICON_RED_H 16

#define BIRD_YELLOW_W 16
#define BIRD_YELLOW_H 16
#define BIRD_YELLOW_FRAMES 3
#define spr_bird_yellow spr_bird_yellow_0
#define spr_bird_yellow_1 spr_bird_yellow_0
#define spr_bird_yellow_2 spr_bird_yellow_0
#define spr_icon_yellow spr_bird_yellow_0
#define ICON_YELLOW_W 16
#define ICON_YELLOW_H 16

#define BIRD_BLACK_W 16
#define BIRD_BLACK_H 16
#define BIRD_BLACK_FRAMES 4
#define spr_bird_black spr_bird_black_0
#define spr_bird_black_1 spr_bird_black_0
#define spr_bird_black_2 spr_bird_black_0
#define spr_bird_black_3 spr_bird_black_0
#define spr_icon_black spr_bird_black_0
#define ICON_BLACK_W 16
#define ICON_BLACK_H 16

#define BIRD_BLUE_W 12
#define BIRD_BLUE_H 12
#define BIRD_BLUE_FRAMES 3
#define spr_bird_blue spr_bird_blue_0
#define spr_bird_blue_1 spr_bird_blue_0
#define spr_bird_blue_2 spr_bird_blue_0
#define spr_icon_blue spr_bird_blue_0
#define ICON_BLUE_W 12
#define ICON_BLUE_H 12

#define BIRD_BIG_W 20
#define BIRD_BIG_H 20
#define BIRD_BIG_FRAMES 3
#define spr_bird_big spr_bird_big_0
#define spr_bird_big_1 spr_bird_big_0
#define spr_bird_big_2 spr_bird_big_0
#define spr_icon_big spr_bird_big_0
#define ICON_BIG_W 20
#define ICON_BIG_H 20

/* jefe y rey: usan dibujos existentes */
#define spr_pig_mustache spr_pig_helmet
#define PIG_MUSTACHE_W 16
#define PIG_MUSTACHE_H 16
#define spr_pig_king spr_pig
#define PIG_KING_W 16
#define PIG_KING_H 16

#endif /* INC_SPRITES_H_ */
