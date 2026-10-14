/* gfx.h - utilidades de dibujo sobre el driver ILI9341 */
#ifndef INC_GFX_H_
#define INC_GFX_H_

#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240

/* Colores RGB565 */
#define COL_SKY      0x867D
#define COL_GRASS    0x55A7
#define COL_DIRT     0x8AC5
#define COL_RED      0xD8E3
#define COL_RED_D    0x9061
#define COL_WHITE    0xFFFF
#define COL_BLACK    0x0000
#define COL_WOOD     0xCC68
#define COL_WOOD_D   0x6A23
#define COL_YELLOW   0xFE40
#define COL_PANEL    0xFFBC
#define COL_GREEN_T  0x3D05
#define COL_GREEN    0x07E0
#define COL_ORANGE   0xFBC0

/* Sprite con transparencia (0xF81F), recortado a los bordes de la pantalla */
void gfx_sprite(int x, int y, int w, int h, const uint16_t *bmp);

/* Sprite ampliado s veces (cada pixel = cuadro de s x s). Para pantallas fijas. */
void gfx_sprite_scaled(int x, int y, int w, int h, const uint16_t *bmp, int s);

/* Dibuja una zona de una imagen de 16 colores usando su paleta */
void gfx_indexed(int x, int y, int w, int h, const uint8_t *px, int img_w, const uint16_t *pal);

/* Dibuja por partes una imagen grande que se lee de la SD */
void gfx_stream_begin(int x, int y, int w, int h);
void gfx_stream_push(const uint16_t *px, int n);
void gfx_stream_end(void);

/* Circulo relleno (explosiones). Recorta en los bordes. */
void gfx_fill_circle(int cx, int cy, int r, int color);

/* Linea de grosor t (1 o 2 px), por ejemplo las ligas de la honda. Recorta. */
void gfx_line(int x0, int y0, int x1, int y1, int t, int color);

/* Texto centrado horizontalmente dentro de [x0, x0+w). size = 1 o 2 */
void gfx_text_in(const char *t, int x0, int w, int y, int size, int color, int bg);

/* Texto centrado en toda la pantalla */
void gfx_text_center(const char *t, int y, int size, int color, int bg);

#endif /* INC_GFX_H_ */
