/* screens.c - niveles, escena, menu, records y pantallas de fin (solo dibujo) */
#include <stdio.h>
#include <string.h>
#include "screens.h"
#include "gfx.h"
#include "ili9341.h"
#include "sprites.h"
#include "storage.h"

#define TILE 16

/* Cielo: el fondo ilustrado de la SD si se cargo; si no, color plano + nubes */
static void sky_fill(int x, int y, int w, int h)
{
    if (bk_ok) gfx_indexed(x, y, w, h, bk_px, BK_W, bk_pal);
    else       FillRect(x, y, w, h, COL_SKY);
}

/* ---------------------------------------------------------------------- */
/* Niveles (codigos: 0 aire, 1 madera, 2 piedra, 3 cerdo, 4 pasto,         */
/*                   5 tierra, 6 cerdo con casco, 7 cerdo jefe)            */
/* Reglas: las estructuras van en las filas 5..12 (las nubes ocupan arriba)*/
/*         y cada columna debe estar apoyada (sin huecos debajo), si no    */
/*         se derrumba sola despues del primer tiro.                       */
/* ---------------------------------------------------------------------- */
#define GR {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}
#define DI {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}
#define EM {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

static const LevelDef levels[NUM_LEVELS] = {
    /* Generados con herramientas/niveles.py (disenados, simulados y verificados) */
    /* ---------------- Nivel 1 - El campamento ---------------- */
    { {
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        /*  9 */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0},
        /* 10 */ {0,0,0,0,0,0,0,0,3,0,3,0,0,0,3,2,1,2,3,0},
        /* 11 */ {0,0,0,0,0,0,0,0,1,2,1,0,0,0,1,2,1,2,1,0},
        /* 12 */ {0,0,0,0,0,0,0,0,1,2,1,0,0,0,2,2,2,2,2,0},
        GR,
        DI },
      { BT_RED, BT_RED, BT_YELLOW, BT_RED, BT_YELLOW }, 5, 4500, 6300 },
    /* ---------------- Nivel 2 - La colina ---------------- */
    { {
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        /*  7 */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0},
        /*  8 */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,0},
        /*  9 */ {0,0,0,0,0,0,0,0,0,0,3,0,0,0,3,2,6,2,3,0},
        /* 10 */ {0,0,0,0,0,0,0,0,2,2,2,0,0,0,2,2,2,2,2,0},
        /* 11 */ {0,0,0,0,0,0,0,3,2,3,2,0,0,4,4,4,4,4,4,4},
        /* 12 */ {0,0,0,0,0,0,0,2,2,2,2,0,0,5,5,5,5,5,5,5},
        GR,
        DI },
      { BT_RED, BT_YELLOW, BT_BLACK, BT_RED, BT_YELLOW, BT_BLUE }, 6, 5600, 7900 },
    /* ---------------- Nivel 3 - Torres gemelas ---------------- */
    { {
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        /*  6 */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,2,6,0,0},
        /*  7 */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,0},
        /*  8 */ {0,0,0,0,0,0,0,0,0,0,0,0,0,3,2,6,2,3,2,0},
        /*  9 */ {0,0,0,0,0,0,0,2,6,2,0,0,0,2,2,2,2,2,2,3},
        /* 10 */ {0,0,0,0,0,0,3,2,2,2,0,0,4,4,4,4,4,4,4,4},
        /* 11 */ {0,0,0,0,0,0,2,3,2,3,0,0,5,5,5,5,5,5,5,5},
        /* 12 */ {0,0,0,0,0,0,2,2,2,2,0,0,5,5,5,5,5,5,5,5},
        GR,
        DI },
      { BT_BLUE, BT_YELLOW, BT_BLACK, BT_RED, BT_BIG, BT_YELLOW, BT_BLACK }, 7, 7800, 10900 },
    /* ---------------- Nivel 4 - La fortaleza ---------------- */
    { {
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        /*  7 */ {0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0},
        /*  8 */ {0,0,0,0,0,0,0,0,3,0,0,0,2,2,0,0,2,2,2,0},
        /*  9 */ {0,0,0,0,0,0,0,2,2,2,0,4,4,4,0,2,2,3,2,2},
        /* 10 */ {0,0,0,0,0,0,6,2,2,2,0,5,5,5,0,2,6,2,6,2},
        /* 11 */ {0,0,0,0,0,0,2,3,2,6,0,5,5,5,0,3,2,3,2,3},
        /* 12 */ {0,0,0,0,0,0,2,2,2,2,0,5,5,5,0,2,2,2,2,2},
        GR,
        DI },
      { BT_YELLOW, BT_BLACK, BT_BLUE, BT_BIG, BT_RED, BT_BLACK, BT_YELLOW }, 7, 9600, 13400 },
    /* ---------------- Nivel 5 - El castillo del jefe ---------------- */
    { {
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        EM,
        /*  7 */ {0,0,0,0,0,0,0,0,0,0,0,0,3,2,6,0,0,0,0,0},
        /*  8 */ {0,0,0,0,0,0,0,0,6,0,0,0,2,2,2,0,2,7,2,0},
        /*  9 */ {0,0,0,0,0,0,0,2,2,2,0,4,4,4,4,0,2,2,2,2},
        /* 10 */ {0,0,0,0,0,0,3,2,2,2,0,5,5,5,5,0,3,2,3,2},
        /* 11 */ {0,0,0,0,0,0,2,6,2,3,0,5,5,5,5,0,2,6,2,6},
        /* 12 */ {0,0,0,0,0,0,2,2,2,2,0,5,5,5,5,0,2,2,2,2},
        GR,
        DI },
      { BT_RED, BT_YELLOW, BT_BLACK, BT_BLUE, BT_BIG, BT_BLACK, BT_YELLOW, BT_BIG }, 8, 10200, 14300 },
};

uint8_t g_level[LV_ROWS][LV_COLS];
uint8_t g_hp[LV_ROWS][LV_COLS];

/* Vida inicial de cada tipo de tile (madera 1 golpe, piedra 2) */
const uint8_t tile_hp[T_COUNT] = { 0, 1, 2, 1, 0, 0, 2, 3 };

const LevelDef *level_load(uint8_t id)
{
    static LevelDef sd_level;                   /* nivel leido de la SD */
    if (id < 1 || id > NUM_LEVELS) id = 1;
    const LevelDef *d = &levels[id - 1];        /* respaldo: niveles del programa */
    if (storage_load_level(id, &sd_level)) d = &sd_level;
    memcpy(g_level, d->tiles, sizeof(g_level));
    for (int r = 0; r < LV_ROWS; r++)
        for (int c = 0; c < LV_COLS; c++)
            g_hp[r][c] = tile_hp[g_level[r][c]];
    return d;
}

/* ---------------------------------------------------------------------- */
/* Escena                                                                  */
/* ---------------------------------------------------------------------- */
/* (lejos del texto de angulo/fuerza, que ocupa x < 180, y 30..52) */
static const int16_t clouds_x[3] = { 196, 140, 262 };
static const int16_t clouds_y[3] = {  38,  60,  46 };

static void draw_cell(int r, int c)
{
    int x = c * TILE, y = r * TILE;
    int hp = g_hp[r][c];
    switch (g_level[r][c]) {
    case T_WOOD:   LCD_Bitmap(x, y, TILE, TILE, spr_block_wood); break;
    case T_STONE:  LCD_Bitmap(x, y, TILE, TILE, hp >= 2 ? spr_block_stone : spr_block_stone_dmg); break;
    case T_GRASS:  LCD_Bitmap(x, y, TILE, TILE, spr_tile_grass); break;
    case T_DIRT:   LCD_Bitmap(x, y, TILE, TILE, spr_tile_dirt);  break;
    case T_PIG:    gfx_sprite(x, y, PIG_W, PIG_H, spr_pig);      break;
    case T_HELMET: gfx_sprite(x, y, PIG_HELMET_W, PIG_HELMET_H,
                              hp >= 2 ? spr_pig_helmet : spr_pig_helmet_dmg); break;
    case T_BOSS:   gfx_sprite(x, y, PIG_MUSTACHE_W, PIG_MUSTACHE_H,
                              hp >= 2 ? spr_pig_mustache : spr_pig_hurt); break;
    default: break;
    }
}

void scene_restore(int x, int y, int w, int h)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    if (y < GROUND_Y) {
        int sh = (y + h > GROUND_Y) ? (GROUND_Y - y) : h;
        sky_fill(x, y, w, sh);
    }
    for (int i = 0; i < 3 && !bk_ok; i++) {          /* el fondo de la SD ya trae nubes */
        if (clouds_x[i] < x + w && clouds_x[i] + CLOUD_W > x &&
            clouds_y[i] < y + h && clouds_y[i] + CLOUD_H > y)
            gfx_sprite(clouds_x[i], clouds_y[i], CLOUD_W, CLOUD_H, spr_cloud);
    }

    int r0 = y / TILE, r1 = (y + h - 1) / TILE;
    int c0 = x / TILE, c1 = (x + w - 1) / TILE;
    for (int r = r0; r <= r1 && r < LV_ROWS; r++)
        for (int c = c0; c <= c1 && c < LV_COLS; c++)
            draw_cell(r, c);

    if (40 < x + w && 40 + SLINGSHOT_W > x && 176 < y + h && 176 + SLINGSHOT_H > y)
        gfx_sprite(40, 176, SLINGSHOT_W, SLINGSHOT_H, spr_slingshot);
}

/* ---------------------------------------------------------------------- */
/* Piezas comunes                                                          */
/* ---------------------------------------------------------------------- */
static void draw_menu_sky(void)
{
    sky_fill(0, 0, SCREEN_W, GROUND_Y);
    if (!bk_ok) {
        gfx_sprite(20,  104, CLOUD_W, CLOUD_H, spr_cloud);
        gfx_sprite(268, 90,  CLOUD_W, CLOUD_H, spr_cloud);
    }
    for (int c = 0; c < LV_COLS; c++) {
        LCD_Bitmap(c * TILE, GROUND_Y,        TILE, TILE, spr_tile_grass);
        LCD_Bitmap(c * TILE, GROUND_Y + TILE, TILE, TILE, spr_tile_dirt);
    }
}

static void draw_panel(void)
{
    FillRect(40, 50, 240, 140, COL_PANEL);
    /* Rect() del driver dibuja (w+1)x(h+1): restamos 1 para quedar dentro */
    Rect(40, 50, 239, 139, COL_WOOD_D);
    Rect(41, 51, 237, 137, COL_WOOD_D);
    Rect(42, 52, 235, 135, COL_WOOD_D);
}

/* ---------------------------------------------------------------------- */
/* Menu de inicio                                                          */
/* ---------------------------------------------------------------------- */
#define MENU_X   80
#define MENU_W   160
#define MENU_H   25
#define MENU_Y0  90
#define MENU_DY  29

static char menu_label[MENU_ITEMS][12] = { "JUGAR", "NIVEL 1", "RECORDS", "CREDITOS" };

static void menu_draw_option(uint8_t i, uint8_t selected)
{
    int y = MENU_Y0 + i * MENU_DY;
    int fill   = selected ? COL_YELLOW : COL_WOOD;
    int border = selected ? COL_RED    : COL_WOOD_D;
    int text   = selected ? COL_BLACK  : COL_WOOD_D;

    FillRect(MENU_X, y, MENU_W, MENU_H, fill);
    Rect(MENU_X, y, MENU_W - 1, MENU_H - 1, border);
    Rect(MENU_X + 1, y + 1, MENU_W - 3, MENU_H - 3, border);
    gfx_text_in(menu_label[i], MENU_X, MENU_W, y + 5, 2, text, fill);
}

void screens_menu_full(uint8_t sel, uint8_t level)
{
    snprintf(menu_label[1], sizeof(menu_label[1]), "NIVEL %u", (unsigned)level);
    draw_menu_sky();

    FillRect(30, 24, 260, 44, COL_RED_D);
    Rect(30, 24, 259, 43, COL_BLACK);
    Rect(31, 25, 257, 41, COL_BLACK);
    gfx_text_in(GAME_TITLE, 30, 260, 38, 2, COL_WHITE, COL_RED_D);
    gfx_text_center("PROYECTO 2 - ELECTRONICA DIGITAL 2", 76, 1, COL_WHITE, COL_SKY);

    for (uint8_t i = 0; i < MENU_ITEMS; i++) menu_draw_option(i, i == sel);

    /* estado de la memoria SD (esquina superior derecha) */
    if (storage_ok()) LCD_Print(bk_ok ? "SD:OK " : "SD:OK*", 270, 6, 1, COL_GREEN_T, COL_SKY);
    else {                                       /* codigo de error para saber por que */
        char e[12];
        snprintf(e, sizeof(e), "SD E%-3u", (unsigned)storage_error());
        LCD_Print(e, 258, 6, 1, COL_RED, COL_SKY);
    }

    /* decoracion: pajaros a la izquierda, cerdos a la derecha */
    gfx_sprite(6,   GROUND_Y - BIRD_BIG_H,    BIRD_BIG_W,    BIRD_BIG_H,    spr_bird_big);
    gfx_sprite(32,  GROUND_Y - BIRD_RED_H,    BIRD_RED_W,    BIRD_RED_H,    spr_bird_red);
    gfx_sprite(50,  GROUND_Y - BIRD_YELLOW_H, BIRD_YELLOW_W, BIRD_YELLOW_H, spr_bird_yellow);
    gfx_sprite(250, GROUND_Y - PIG_H,         PIG_W,         PIG_H,         spr_pig);
    gfx_sprite(268, GROUND_Y - PIG_HELMET_H,  PIG_HELMET_W,  PIG_HELMET_H,  spr_pig_helmet);
    gfx_sprite(290, GROUND_Y - PIG_KING_H,    PIG_KING_W,    PIG_KING_H,    spr_pig_king);
}

void screens_menu_select(uint8_t old_sel, uint8_t new_sel)
{
    if (old_sel == new_sel) return;
    menu_draw_option(old_sel, 0);
    menu_draw_option(new_sel, 1);
}

void screens_menu_level(uint8_t level, uint8_t sel)
{
    snprintf(menu_label[1], sizeof(menu_label[1]), "NIVEL %u", (unsigned)level);
    menu_draw_option(1, sel == 1);
}

/* ---------------------------------------------------------------------- */
/* Records                                                                 */
/* ---------------------------------------------------------------------- */
void screens_records(const uint16_t *best, uint16_t partidas, uint16_t victorias, uint16_t derrotas)
{
    char buf[32];
    draw_panel();
    gfx_text_in("RECORDS", 40, 240, 58, 2, COL_RED_D, COL_PANEL);
    for (uint8_t i = 0; i < NUM_LEVELS; i++) {
        snprintf(buf, sizeof(buf), "NIVEL %u:  %05u", (unsigned)(i + 1), (unsigned)best[i]);
        gfx_text_in(buf, 40, 240, 80 + i * 13, 1, COL_WOOD_D, COL_PANEL);
    }
    /* estadisticas guardadas en la SD (STATS.TXT) */
    snprintf(buf, sizeof(buf), "PARTIDAS:%u  GANADAS:%u", (unsigned)partidas, (unsigned)victorias);
    gfx_text_in(buf, 40, 240, 148, 1, COL_GREEN_T, COL_PANEL);
    snprintf(buf, sizeof(buf), "PERDIDAS:%u", (unsigned)derrotas);
    gfx_text_in(buf, 40, 240, 160, 1, COL_GREEN_T, COL_PANEL);
    gfx_text_in("PRESIONA UN BOTON", 40, 240, 176, 1, COL_WOOD_D, COL_PANEL);
}

/* ---------------------------------------------------------------------- */
/* Creditos                                                                */
/* ---------------------------------------------------------------------- */
void screens_credits(const char *txt)
{
    if (txt) {                                   /* texto de la SD (CREDITOS.TXT) */
        char line[41];
        int y = 40;
        LCD_Clear(COL_SKY);
        FillRect(0, 0, SCREEN_W, 28, COL_RED_D);
        gfx_text_center("CREDITOS", 6, 2, COL_WHITE, COL_RED_D);
        for (const char *p = txt; *p && y <= 196; y += 16) {
            int n = 0;
            while (*p && *p != '\n' && *p != '\r') {
                if (n < 40 && *p >= 32 && *p <= 126) line[n++] = *p;   /* solo lo que tiene la fuente */
                p++;
            }
            line[n] = '\0';
            if (*p == '\r') p++;
            if (*p == '\n') p++;
            gfx_text_center(line, y, 1, COL_BLACK, COL_SKY);
        }
        FillRect(0, 214, SCREEN_W, 26, COL_RED_D);
        gfx_text_center("CUALQUIER BOTON: VOLVER", 223, 1, COL_WHITE, COL_RED_D);
        return;
    }

    static const char *const lines[] = {
        GAME_TITLE " - PROYECTO 2",
        "IE3054 ELECTRONICA DIGITAL 2",
        "DESARROLLO: " CREDITS_AUTHOR,
        "",
        "JUEGO INSPIRADO EN ANGRY BIRDS",
        "ANGRY BIRDS ES MARCA REGISTRADA",
        "DE ROVIO ENTERTAINMENT CORP.",
        "TEMA MUSICAL: ARI PULKKINEN",
        "",
        "PROYECTO ACADEMICO SIN FINES DE LUCRO",
    };
    LCD_Clear(COL_SKY);
    FillRect(0, 0, SCREEN_W, 28, COL_RED_D);
    gfx_text_center("CREDITOS", 6, 2, COL_WHITE, COL_RED_D);
    for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); i++)
        gfx_text_center(lines[i], 40 + i * 16, 1, (i < 3) ? COL_BLACK : COL_RED_D, COL_SKY);
    FillRect(0, 214, SCREEN_W, 26, COL_RED_D);
    gfx_text_center("CUALQUIER BOTON: VOLVER", 223, 1, COL_WHITE, COL_RED_D);
}

/* ---------------------------------------------------------------------- */
/* Fin de nivel y game over                                                */
/* ---------------------------------------------------------------------- */
void screens_win(uint8_t stars, uint16_t score, uint8_t new_record)
{
    char buf[24];
    draw_panel();
    gfx_text_in("NIVEL COMPLETO", 40, 240, 64, 2, COL_GREEN_T, COL_PANEL);
    for (uint8_t i = 0; i < 3; i++) {
        if (i < stars) gfx_sprite(120 + i * 32, 92, STAR_FULL_W,  STAR_FULL_H,  spr_star_full);
        else           gfx_sprite(120 + i * 32, 92, STAR_EMPTY_W, STAR_EMPTY_H, spr_star_empty);
    }
    snprintf(buf, sizeof(buf), "PUNTOS: %05u", (unsigned)score);
    gfx_text_in(buf, 40, 240, 120, 2, COL_WOOD_D, COL_PANEL);
    if (new_record) gfx_text_in("NUEVO RECORD!", 40, 240, 144, 1, COL_RED, COL_PANEL);
    gfx_text_in("PRESIONA UN BOTON", 40, 240, 168, 1, COL_WOOD_D, COL_PANEL);
}

void screens_gameover(uint16_t score)
{
    char buf[24];
    draw_panel();
    gfx_text_in("GAME OVER", 40, 240, 64, 2, COL_RED, COL_PANEL);
    gfx_sprite(152, 94, PIG_W, PIG_H, spr_pig);
    snprintf(buf, sizeof(buf), "PUNTOS: %05u", (unsigned)score);
    gfx_text_in(buf, 40, 240, 124, 2, COL_WOOD_D, COL_PANEL);
    gfx_text_in("PRESIONA UN BOTON", 40, 240, 164, 1, COL_WOOD_D, COL_PANEL);
}

/* ---------------------------------------------------------------------- */
/* Galeria de personajes (para revisar los sprites en la pantalla real)    */
/* ---------------------------------------------------------------------- */
typedef struct {
    const char     *name;
    const uint16_t *bmp;
    uint8_t         w, h;
} CharInfo;

static void gallery_row(const CharInfo *c, int n, int x0, int dx, int base_y, int name_y)
{
    for (int i = 0; i < n; i++) {
        int cx = x0 + i * dx;                        /* centro de la columna */
        gfx_sprite_scaled(cx - c[i].w, base_y - 2 * c[i].h,
                          c[i].w, c[i].h, c[i].bmp, 2);
        gfx_text_in(c[i].name, cx - dx / 2, dx, name_y, 1, COL_BLACK, COL_SKY);
    }
}

void screens_gallery(void)
{
    static const CharInfo birds[] = {
        { "RED",     spr_bird_red,    BIRD_RED_W,    BIRD_RED_H    },
        { "CHUCK", spr_bird_yellow, BIRD_YELLOW_W, BIRD_YELLOW_H },
        { "BOMB",    spr_bird_black,  BIRD_BLACK_W,  BIRD_BLACK_H  },
        { "BLUE",     spr_bird_blue,   BIRD_BLUE_W,   BIRD_BLUE_H   },
        { "TERRENCE",   spr_bird_big,    BIRD_BIG_W,    BIRD_BIG_H    },
    };
    static const CharInfo pigs[] = {
        { "CERDO",  spr_pig,            PIG_W,            PIG_H            },
        { "HERIDO", spr_pig_hurt,       PIG_HURT_W,       PIG_HURT_H       },
        { "CASCO",  spr_pig_helmet,     PIG_HELMET_W,     PIG_HELMET_H     },
        { "ROTO",   spr_pig_helmet_dmg, PIG_HELMET_DMG_W, PIG_HELMET_DMG_H },
        { "JEFE",   spr_pig_mustache,   PIG_MUSTACHE_W,   PIG_MUSTACHE_H   },
        { "REY",    spr_pig_king,       PIG_KING_W,       PIG_KING_H       },
    };

    LCD_Clear(COL_SKY);
    FillRect(0, 0, SCREEN_W, 24, COL_RED_D);
    gfx_text_center("PERSONAJES", 4, 2, COL_WHITE, COL_RED_D);

    gallery_row(birds, 5, 32, 64, 98, 102);          /* pajaros: base en y = 98  */
    gallery_row(pigs,  6, 27, 53, 194, 198);         /* cerdos:  base en y = 194 */

    FillRect(0, 214, SCREEN_W, 26, COL_RED_D);
    gfx_text_center("CLIC O B1: VOLVER AL MENU", 223, 1, COL_WHITE, COL_RED_D);
}
