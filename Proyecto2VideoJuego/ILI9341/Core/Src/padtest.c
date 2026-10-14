/* padtest.c - pantalla de prueba del mando inalambrico */
#include <stdio.h>
#include "padtest.h"
#include "gfx.h"
#include "ili9341.h"
#include "main.h"

#define COL_BG     COL_SKY
#define COL_BOX    COL_PANEL
#define COL_OFF    0x94B2                 /* gris: boton suelto */

/* Recuadro del joystick: 120x120 px, centro en (BOX_CX, BOX_CY) */
#define BOX_X      16
#define BOX_Y      56
#define BOX_S      120
#define BOX_CX     (BOX_X + BOX_S / 2)
#define BOX_CY     (BOX_Y + BOX_S / 2)
#define DOT        8                      /* lado del punto que sigue a la palanca */
#define RANGE      ((BOX_S - DOT) / 2 - 2)

/* Columna de textos */
#define TXT_X      152

/* Botones */
#define BTN_Y      196
#define BTN_W      96
#define BTN_H      28

static int   dot_x = -1, dot_y = -1;      /* posicion dibujada del punto (-1 = ninguna) */
static int8_t  last_jx = 127, last_jy = 127;
static uint8_t last_btn = 0xFF, last_conn = 0xFF;
static uint32_t last_rate_ms = 0, last_frames = 0;
static uint16_t rate = 0, last_rate = 0xFFFF;

static void draw_cross(void)
{
    FillRect(BOX_X + 2, BOX_CY, BOX_S - 4, 1, COL_WOOD_D);
    FillRect(BOX_CX, BOX_Y + 2, 1, BOX_S - 4, COL_WOOD_D);
}

static void draw_button(uint8_t i, const char *name, uint8_t on)
{
    int x = 8 + i * (BTN_W + 8);
    int fill = on ? COL_GREEN : COL_OFF;
    FillRect(x, BTN_Y, BTN_W, BTN_H, fill);
    Rect(x, BTN_Y, BTN_W - 1, BTN_H - 1, COL_BLACK);
    gfx_text_in(name, x, BTN_W, BTN_Y + 8, 1, COL_BLACK, fill);
}

void padtest_full(void)
{
    LCD_Clear(COL_BG);
    FillRect(0, 0, SCREEN_W, 40, COL_RED_D);
    gfx_text_center("PRUEBA DEL MANDO", 12, 2, COL_WHITE, COL_RED_D);

    FillRect(BOX_X, BOX_Y, BOX_S, BOX_S, COL_BOX);
    Rect(BOX_X, BOX_Y, BOX_S - 1, BOX_S - 1, COL_BLACK);
    draw_cross();

    LCD_Print("CLIC/B1 LARGO: SALIR", TXT_X, 176, 1, COL_BLACK, COL_BG);

    /* fuerza a redibujar todo en el siguiente padtest_update */
    dot_x = dot_y = -1;
    last_jx = last_jy = 127;
    last_btn = last_conn = 0xFF;
    last_rate = 0xFFFF;
}

void padtest_update(const PadState *p)
{
    char buf[24];
    uint32_t now = HAL_GetTick();

    /* tramas por segundo (deberian ser ~50) */
    if (now - last_rate_ms >= 1000) {
        rate = (uint16_t)(p->frames - last_frames);
        last_frames = p->frames;
        last_rate_ms = now;
    }

    if (p->connected != last_conn) {
        last_conn = p->connected;
        if (p->connected) LCD_Print("ENLACE: OK", TXT_X, 56, 2, COL_GREEN_T, COL_BG);
        else              LCD_Print("ENLACE: NO", TXT_X, 56, 2, COL_RED,     COL_BG);
    }

    if (p->jx != last_jx || p->jy != last_jy) {
        last_jx = p->jx; last_jy = p->jy;

        snprintf(buf, sizeof(buf), "X:%+4d", p->jx);
        LCD_Print(buf, TXT_X, 84, 2, COL_BLACK, COL_BG);
        snprintf(buf, sizeof(buf), "Y:%+4d", p->jy);
        LCD_Print(buf, TXT_X, 106, 2, COL_BLACK, COL_BG);

        /* +X a la derecha, +Y hacia arriba (en pantalla y crece hacia abajo) */
        int nx = BOX_CX - DOT / 2 + (p->jx * RANGE) / 100;
        int ny = BOX_CY - DOT / 2 - (p->jy * RANGE) / 100;
        if (nx != dot_x || ny != dot_y) {
            if (dot_x >= 0) FillRect(dot_x, dot_y, DOT, DOT, COL_BOX);   /* borra */
            draw_cross();
            FillRect(nx, ny, DOT, DOT, COL_RED);
            dot_x = nx; dot_y = ny;
        }
    }

    if (rate != last_rate) {
        last_rate = rate;
        snprintf(buf, sizeof(buf), "TRAMAS/S:%3u", (unsigned)rate);
        LCD_Print(buf, TXT_X, 134, 1, COL_BLACK, COL_BG);
        snprintf(buf, sizeof(buf), "ERRORES :%3u", (unsigned)(p->bad_frames % 1000));
        LCD_Print(buf, TXT_X, 150, 1, COL_BLACK, COL_BG);
    }

    if (p->btn != last_btn) {
        last_btn = p->btn;
        draw_button(0, "DISPARO", (p->btn & PAD_BTN_FIRE)  != 0);
        draw_button(1, "PODER",   (p->btn & PAD_BTN_POWER) != 0);
        draw_button(2, "CLIC",    (p->btn & PAD_BTN_MENU)  != 0);
    }
}
