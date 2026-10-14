/* gfx.c - utilidades de dibujo sobre el driver ILI9341 (8 bits) */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gfx.h"
#include "ili9341.h"
#include "sprites.h"

/* El chip select de la LCD es LCD_CS (definido en main.h por CubeMX) */
#define GFX_CS_L()  (LCD_CS_GPIO_Port->BSRR = ((uint32_t)LCD_CS_Pin << 16))
#define GFX_CS_H()  (LCD_CS_GPIO_Port->BSRR =  (uint32_t)LCD_CS_Pin)

/* Dibuja un sprite enviando solo los tramos de pixeles visibles */
void gfx_sprite(int x, int y, int w, int h, const uint16_t *bmp)
{
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < 0 || py >= SCREEN_H) continue;      /* fila fuera de pantalla */

        const uint16_t *row = bmp + j * w;
        int i = 0;
        while (i < w) {
            /* saltar transparentes y columnas fuera de pantalla */
            while (i < w && (row[i] == SPR_TRANSPARENT || x + i < 0 || x + i >= SCREEN_W)) i++;
            int start = i;
            /* tramo de pixeles visibles */
            while (i < w && row[i] != SPR_TRANSPARENT && x + i >= 0 && x + i < SCREEN_W) i++;

            if (i > start) {
                GFX_CS_L();
                SetWindows(x + start, py, x + i - 1, py);
                for (int k = start; k < i; k++) {
                    LCD_DATA(row[k] >> 8);
                    LCD_DATA(row[k] & 0xFF);
                }
                GFX_CS_H();
            }
        }
    }
}

void gfx_sprite_scaled(int x, int y, int w, int h, const uint16_t *bmp, int s)
{
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            uint16_t c = bmp[j * w + i];
            int px = x + i * s, py = y + j * s;
            if (c == SPR_TRANSPARENT) continue;
            if (px < 0 || py < 0 || px + s > SCREEN_W || py + s > SCREEN_H) continue;
            FillRect(px, py, s, s, c);
        }
    }
}

void gfx_indexed(int x, int y, int w, int h, const uint8_t *px, int img_w, const uint16_t *pal)
{
    if (w <= 0 || h <= 0) return;
    GFX_CS_L();
    SetWindows(x, y, x + w - 1, y + h - 1);          /* una sola ventana para todo */
    for (int j = 0; j < h; j++) {
        const uint8_t *row = px + ((y + j) * img_w) / 2;
        for (int i = 0; i < w; i++) {
            int cx = x + i;
            uint8_t b = row[cx >> 1];
            uint16_t c = pal[(cx & 1) ? (b & 0x0F) : (b >> 4)];
            LCD_DATA(c >> 8);
            LCD_DATA(c & 0xFF);
        }
    }
    GFX_CS_H();
}

void gfx_stream_begin(int x, int y, int w, int h)
{
    GFX_CS_L();
    SetWindows(x, y, x + w - 1, y + h - 1);
}

void gfx_stream_push(const uint16_t *px, int n)
{
    for (int i = 0; i < n; i++) {
        LCD_DATA(px[i] >> 8);
        LCD_DATA(px[i] & 0xFF);
    }
}

void gfx_stream_end(void)
{
    GFX_CS_H();
}

void gfx_fill_circle(int cx, int cy, int r, int color)
{
    for (int dy = -r; dy <= r; dy++) {
        int y = cy + dy;
        if (y < 0 || y >= SCREEN_H) continue;
        int hw = (int)sqrtf((float)(r * r - dy * dy));
        int x0 = cx - hw, x1 = cx + hw;
        if (x0 < 0) x0 = 0;
        if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
        if (x1 < x0) continue;
        FillRect(x0, y, x1 - x0 + 1, 1, color);
    }
}

/* Bresenham: un cuadrito de t x t por cada punto de la linea */
void gfx_line(int x0, int y0, int x1, int y1, int t, int color)
{
    int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        if (x0 >= 0 && y0 >= 0 && x0 + t <= SCREEN_W && y0 + t <= SCREEN_H)
            FillRect(x0, y0, t, t, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_text_in(const char *t, int x0, int w, int y, int size, int color, int bg)
{
    int cw = (size == 1) ? 8 : 16;          /* ancho de caracter del driver */
    int tw = (int)strlen(t) * cw;
    int x = x0 + (w - tw) / 2;
    if (x < 0) x = 0;
    LCD_Print((char *)t, x, y, size, color, bg);
}

void gfx_text_center(const char *t, int y, int size, int color, int bg)
{
    gfx_text_in(t, 0, SCREEN_W, y, size, color, bg);
}
