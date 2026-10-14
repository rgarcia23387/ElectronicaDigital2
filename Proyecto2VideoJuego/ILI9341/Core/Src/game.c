/* game.c - logica del juego: apuntar, volar, chocar, derrumbar y puntuar */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "game.h"
#include "gfx.h"
#include "screens.h"
#include "sprites.h"
#include "ili9341.h"
#include "sound.h"

/* ---------------- Parametros de ajuste (puedes jugar con ellos) --------- */
#define GRAV          0.22f     /* gravedad (px/cuadro^2)                  */
#define V_MIN         3.0f      /* velocidad de lanzamiento con fuerza 0   */
#define V_PER_POWER   0.065f    /* velocidad extra por cada punto de fuerza */
#define ANGLE_MIN     5.0f
#define ANGLE_MAX     80.0f
#define ANGLE_STEP    1.5f
#define POWER_MIN     20.0f
#define POWER_MAX     100.0f
#define POWER_STEP    2.5f
#define V_MAX         14.0f

/* Resistencia de los bloques y fin del vuelo */
#define IMPACT_MIN    1.0f      /* velocidad minima para hacer dano            */
#define IMPACT_STRONG 5.5f      /* golpe fuerte: +1 de dano (rompe la piedra)  */
#define WOOD_SLOW     0.85f     /* velocidad que conserva al atravesar madera  */
#define REST_FRAMES   6         /* cuadros quieto antes de desaparecer (~0.2 s) */
#define MAX_AGE       180       /* vida maxima del pajaro en cuadros (~6 s)     */
#define FUSE_FRAMES   14        /* mecha de la bomba al chocar (~0.5 s)         */

/* 1 = modo honda (se jala hacia atras), 0 = se apunta hacia donde se empuja */
#define STICK_PULL_MODE 1
/* 1 = soltar la palanca de golpe despues de estirar tambien dispara */
#define RELEASE_TO_FIRE 1

#define REST_X        48        /* centro del pajaro en la honda           */
#define REST_Y        170
#define PULL_MAX      24.0f     /* cuanto se estira la liga (px)           */
#define BAND_BACK_X   53        /* punta del poste trasero de la honda     */
#define BAND_BACK_Y   177
#define BAND_FRONT_X  43        /* punta del poste delantero               */
#define BAND_FRONT_Y  179
#define COL_BAND      0x4A02    /* cafe oscuro de la liga                  */

#define AIM_DOTS      9         /* puntos de la trayectoria al apuntar     */
#define MAX_FLY       3         /* el azul se divide en 3                  */
#define TRAIL_MAX     40
#define MAX_PART      40        /* particulas (astillas, polvo, chispas)   */
#define PI_F          3.14159265f

/* Zona alrededor de la honda que se redibuja mientras se apunta */
#define AIM_X 8
#define AIM_Y 128
#define AIM_W 84
#define AIM_H (GROUND_Y + 4 - AIM_Y)     /* +4: el pajaro grande estirado toca el pasto */

/* Texto de angulo/fuerza y barra */
#define INFO_Y 30

/* ---------------- Datos de los pajaros ---------------------------------- */
/* Cuadros de cada pajaro: 0 honda, 1 vuelo, 2 choque, 3 bomba al rojo */
#define FR_SLING 0
#define FR_FLY   1
#define FR_HIT   2
#define FR_HOT   3

typedef struct {
    const uint16_t *fr[4];
    uint8_t w, h, r;                 /* medidas y radio de choque */
    const uint16_t *icon;            /* version chica para la cola del HUD */
    uint8_t iw, ih;
    const char *name;
} BirdInfo;

static const BirdInfo binfo[BT_COUNT] = {
    { { spr_bird_red_0,    spr_bird_red_1,    spr_bird_red_2,    0 },
      BIRD_RED_W,    BIRD_RED_H,    BIRD_RED_W / 2 - 1,    spr_icon_red,    ICON_RED_W,    ICON_RED_H,    "RED"     },
    { { spr_bird_yellow_0, spr_bird_yellow_1, spr_bird_yellow_2, 0 },
      BIRD_YELLOW_W, BIRD_YELLOW_H, BIRD_YELLOW_H / 2 - 1, spr_icon_yellow, ICON_YELLOW_W, ICON_YELLOW_H, "CHUCK" },
    { { spr_bird_black_0,  spr_bird_black_1,  spr_bird_black_2,  spr_bird_black_3 },
      BIRD_BLACK_W,  BIRD_BLACK_H,  BIRD_BLACK_W / 2 - 1,  spr_icon_black,  ICON_BLACK_W,  ICON_BLACK_H,  "BOMB"    },
    { { spr_bird_blue_0,   spr_bird_blue_1,   spr_bird_blue_2,   0 },
      BIRD_BLUE_W,   BIRD_BLUE_H,   BIRD_BLUE_W / 2 - 1,   spr_icon_blue,   ICON_BLUE_W,   ICON_BLUE_H,   "BLUE"     },
    { { spr_bird_big_0,    spr_bird_big_1,    spr_bird_big_2,    0 },
      BIRD_BIG_W,    BIRD_BIG_H,    BIRD_BIG_W / 2 - 1,    spr_icon_big,    ICON_BIG_W,    ICON_BIG_H,    "TERRENCE"   },
};

/* Dano por pajaro segun material: madera, piedra, cerdo, cerdo con casco/jefe */
static const uint8_t dmg_tab[BT_COUNT][4] = {
    { 1, 1, 1, 1 },     /* rojo: normal                          */
    { 2, 1, 1, 1 },     /* amarillo: mejor contra madera         */
    { 1, 1, 1, 1 },     /* bomba: el dano fuerte es la explosion */
    { 1, 1, 1, 1 },     /* azul: se divide en 3                  */
    { 3, 3, 3, 3 },     /* grande: pesado, rompe todo            */
};

/* Puntos al destruir cada tipo de tile */
static const uint16_t tile_pts[T_COUNT] = { 0, 50, 100, 500, 0, 0, 800, 1500 };

typedef enum { PH_ANGLE, PH_POWER, PH_AIM, PH_FLIGHT, PH_SETTLE } Phase;

typedef struct {
    float   x, y, vx, vy;
    uint8_t type, alive, used_power, rest;
    uint16_t age;
    uint8_t fuse;                   /* cuenta regresiva de la bomba (0 = apagada) */
    uint8_t diving;                 /* el grande va en picada                     */
    uint8_t landed;                 /* ya toco el suelo o un bloque               */
    int16_t ox, oy;                 /* ultimo centro dibujado                     */
    uint8_t drawn;
} FlyBird;

typedef struct {
    float    x, y, vx, vy;
    uint16_t col;
    uint8_t  life;                  /* cuadros que le quedan (0 = libre) */
    int16_t  ox, oy;
    uint8_t  drawn;
} Particle;

static struct {
    Phase    phase;
    const LevelDef *def;
    uint8_t  level;
    uint8_t  qi;                    /* indice del pajaro actual en la cola    */
    float    angle, power;
    int8_t   adir, pdir;
    float    snap_angle, snap_power;/* ultimo apuntado con la liga estirada   */
    float    mag_hist[3];           /* magnitud del joystick (3 cuadros)      */
    uint16_t score;
    uint8_t  pigs;
    FlyBird  fly[MAX_FLY];
    Particle part[MAX_PART];
    int16_t  trail[TRAIL_MAX][2];
    uint8_t  ntrail, trail_tick;
    float    hx[4], hy[4];          /* ultimas posiciones del pajaro lider    */
    uint16_t hn;
    int16_t  dots[AIM_DOTS][2];     /* trayectoria dibujada                   */
    uint8_t  ndots;
    int16_t  aim_key;               /* angulo y fuerza dibujados (para no redibujar igual) */
    uint8_t  aim_drawn;
    uint8_t  settle_tick;
    uint16_t settle_idle;
    uint8_t  hud_full, hud_score;
    uint8_t  connected;             /* hay mando enlazado */
    uint8_t  done;                  /* partida terminada (resultado fijo) */
    GameResult result;
} g;

static uint8_t  fall_cnt[LV_ROWS][LV_COLS];  /* filas que ha caido cada tile */
static uint32_t rng = 12345;

static int rnd(int n)                        /* numero pseudoaleatorio 0..n-1 */
{
    rng = rng * 1103515245u + 12345u;
    return (int)((rng >> 16) % (uint32_t)n);
}

static int is_block(uint8_t t)
{
    return t == T_WOOD || t == T_STONE || t == T_PIG || t == T_HELMET || t == T_BOSS;
}

/* Lo que detiene al pajaro: bloques, cerdos y el terreno de las colinas */
static int is_solid(uint8_t t)
{
    return is_block(t) || t == T_GRASS || t == T_DIRT;
}

static int is_pig(uint8_t t)
{
    return t == T_PIG || t == T_HELMET || t == T_BOSS;
}

/* ======================================================================= */
/* HUD                                                                     */
/* ======================================================================= */
static uint8_t cur_type(void) { return g.def->birds[g.qi]; }

static void hud_score_draw(void)
{
    char buf[20];
    snprintf(buf, sizeof(buf), "PUNTOS:%05u", (unsigned)g.score);
    LCD_Print(buf, 6, 3, 1, COL_BLACK, COL_SKY);
    snprintf(buf, sizeof(buf), "CERDOS:%u ", (unsigned)g.pigs);
    LCD_Print(buf, 6, 16, 1, COL_BLACK, COL_SKY);
}

static void hud_draw(void)
{
    char buf[16];
    FillRect(0, 0, SCREEN_W, 28, COL_SKY);
    hud_score_draw();
    snprintf(buf, sizeof(buf), "NIVEL %u", (unsigned)g.level);
    LCD_Print(buf, 120, 16, 1, COL_BLACK, COL_SKY);
    if (g.qi < g.def->nbirds)
        LCD_Print((char *)binfo[cur_type()].name, 120, 3, 1, COL_RED_D, COL_SKY);

    /* pajaros que quedan despues del actual (iconos): el siguiente queda mas a la izquierda */
    int waiting = g.def->nbirds - g.qi - 1, slot = 0;
    for (int i = g.qi + 1; i < g.def->nbirds; i++, slot++) {
        const BirdInfo *bi = &binfo[g.def->birds[i]];
        int cx = 306 - (waiting - 1 - slot) * 22, cy = 14;
        gfx_sprite(cx - bi->iw / 2, cy - bi->ih / 2, bi->iw, bi->ih, bi->icon);
    }
    g.hud_full = g.hud_score = 0;
}

/* Cualquier restauracion que toque la franja del HUD obliga a redibujarlo */
static void restore(int x, int y, int w, int h)
{
    scene_restore(x, y, w, h);
    if (y < 28) g.hud_full = 1;
}

/* ======================================================================= */
/* Particulas (astillas, polvo, chispas de los poderes)                     */
/* ======================================================================= */
static void spawn(float x, float y, int n, uint16_t c1, uint16_t c2, float speed, float up)
{
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < MAX_PART; i++) {
            Particle *p = &g.part[i];
            if (p->life) continue;
            float a = rnd(360) * PI_F / 180.0f;
            float s = speed * (0.4f + rnd(60) / 100.0f);
            p->x = x; p->y = y;
            p->vx = cosf(a) * s;
            p->vy = sinf(a) * s - up;
            p->col = (k & 1) ? c2 : c1;
            p->life = 10 + rnd(12);
            p->drawn = 0;
            break;
        }
    }
}

static void parts_erase(void)
{
    for (int i = 0; i < MAX_PART; i++) {
        Particle *p = &g.part[i];
        if (p->drawn) { restore(p->ox, p->oy, 2, 2); p->drawn = 0; }
    }
}

static void parts_update_draw(void)
{
    for (int i = 0; i < MAX_PART; i++) {
        Particle *p = &g.part[i];
        if (!p->life) continue;
        p->vy += 0.25f;
        p->x += p->vx;
        p->y += p->vy;
        p->life--;
        int x = (int)p->x, y = (int)p->y;
        if (x < 0 || x > SCREEN_W - 2 || y < 30 || y > GROUND_Y - 2) { p->life = 0; continue; }
        if (!p->life) continue;
        FillRect(x, y, 2, 2, p->col);
        p->ox = x; p->oy = y; p->drawn = 1;
    }
}

static int parts_alive(void)
{
    for (int i = 0; i < MAX_PART; i++) if (g.part[i].life || g.part[i].drawn) return 1;
    return 0;
}

/* ======================================================================= */
/* Apuntado: honda estirada, trayectoria y barra de fuerza                  */
/* ======================================================================= */
static void bar_draw(void)
{
    char buf[24];
    uint16_t col = (g.power < 55) ? COL_GREEN : (g.power < 80) ? COL_YELLOW : COL_RED;
    int w = (int)((g.power - POWER_MIN) * 78.0f / (POWER_MAX - POWER_MIN));
    snprintf(buf, sizeof(buf), "ANGULO:%2d  FUERZA:%3d", (int)(g.angle + 0.5f), (int)(g.power + 0.5f));
    LCD_Print(buf, 6, INFO_Y, 1, COL_BLACK, COL_SKY);
    FillRect(6, INFO_Y + 14, 80, 8, COL_BLACK);
    if (w > 0) FillRect(7, INFO_Y + 15, w, 6, col);
    if (w < 78) FillRect(7 + w, INFO_Y + 15, 78 - w, 6, COL_WHITE);
}

static void info_clear(void)
{
    restore(4, INFO_Y - 1, 180, 24);
}

/* Posicion del pajaro en la honda segun angulo y fuerza */
static void aim_bird_pos(int *bx, int *by)
{
    float a = g.angle * PI_F / 180.0f;
    float pull = (g.power - POWER_MIN) / (POWER_MAX - POWER_MIN) * PULL_MAX;
    *bx = REST_X - (int)(cosf(a) * pull);
    *by = REST_Y + (int)(sinf(a) * pull);          /* hacia abajo y atras */
}

/* Calcula la trayectoria con la misma fisica del vuelo (sin choques) */
static void aim_compute_dots(int16_t dots[][2], uint8_t *n)
{
    float a = g.angle * PI_F / 180.0f;
    float v = V_MIN + g.power * V_PER_POWER;
    float x = REST_X, y = REST_Y, vx = v * cosf(a), vy = -v * sinf(a);
    *n = 0;
    for (int f = 1; f <= 60 && *n < AIM_DOTS; f++) {
        vy += GRAV;
        x += vx; y += vy;
        if (y >= GROUND_Y - 3 || x >= SCREEN_W - 3) break;
        int c = (int)x / 16, r = (int)y / 16;
        if (r >= 0 && r < LV_ROWS && c >= 0 && c < LV_COLS && is_solid(g_level[r][c])) break;
        if ((f % 3) == 0 && y > INFO_Y + 26 && x > AIM_X + AIM_W) {
            dots[*n][0] = (int16_t)x;
            dots[*n][1] = (int16_t)y;
            (*n)++;
        }
    }
}

static void aim_erase(void)
{
    if (!g.aim_drawn) return;
    restore(AIM_X, AIM_Y, AIM_W, AIM_H);
    for (int i = 0; i < g.ndots; i++) restore(g.dots[i][0] - 1, g.dots[i][1] - 1, 4, 4);
    g.ndots = 0;
    g.aim_drawn = 0;
}

static void aim_draw(int force)
{
    int16_t key = (int16_t)((int)(g.angle + 0.5f) * 128 + (int)(g.power + 0.5f));
    if (!force && g.aim_drawn && key == g.aim_key) return;   /* nada cambio: sin parpadeo */
    g.aim_key = key;

    const BirdInfo *bi = &binfo[cur_type()];
    int bx, by;
    aim_bird_pos(&bx, &by);
    aim_erase();

    /* liga trasera -> pajaro -> liga delantera (asi el pajaro queda "dentro") */
    gfx_line(BAND_BACK_X, BAND_BACK_Y, bx + 3, by + 2, 2, COL_BAND);
    gfx_sprite(bx - bi->w / 2, by - bi->h / 2, bi->w, bi->h, bi->fr[FR_SLING]);
    gfx_line(BAND_FRONT_X, BAND_FRONT_Y, bx - 3, by + 3, 2, COL_BAND);

    /* trayectoria: cada punto mas pequeno que el anterior */
    aim_compute_dots(g.dots, &g.ndots);
    for (int i = 0; i < g.ndots; i++) {
        int s = (i < 3) ? 3 : 2;
        FillRect(g.dots[i][0], g.dots[i][1], s, s, COL_WHITE);
    }
    bar_draw();
    g.aim_drawn = 1;
}

static void begin_aim(void)
{
    g.phase = g.connected ? PH_AIM : PH_ANGLE;
    g.pdir = 1;
    g.power = POWER_MIN;
    g.mag_hist[0] = g.mag_hist[1] = g.mag_hist[2] = 0;
    aim_draw(1);
}

/* Joystick -> angulo y fuerza; devuelve 1 si se solto la liga */
static int aim_from_stick(int jx, int jy)
{
    float m = sqrtf((float)(jx * jx + jy * jy));
    float mmax = fmaxf(g.mag_hist[0], fmaxf(g.mag_hist[1], g.mag_hist[2]));
    g.mag_hist[2] = g.mag_hist[1]; g.mag_hist[1] = g.mag_hist[0]; g.mag_hist[0] = m;

    if (m < 10.0f) {                                    /* palanca suelta */
#if RELEASE_TO_FIRE && STICK_PULL_MODE
        if (mmax >= 45.0f) {                            /* se solto de golpe: dispara */
            g.angle = g.snap_angle;
            g.power = g.snap_power;
            return 1;
        }
#endif
        g.power += (POWER_MIN - g.power) * 0.5f;        /* la liga regresa */
        return 0;
    }
#if STICK_PULL_MODE
    float dx = (float)-jx, dy = (float)-jy;             /* sale al lado contrario */
#else
    float dx = (float)jx,  dy = (float)jy;
#endif
    float ang = atan2f(dy, dx) * 180.0f / PI_F;
    if (ang > 90.0f) ang = ANGLE_MAX; else if (ang < 0.0f) ang = ANGLE_MIN;
    if (ang < ANGLE_MIN) ang = ANGLE_MIN;
    if (ang > ANGLE_MAX) ang = ANGLE_MAX;
    float pw = POWER_MIN + (fminf(m, 100.0f) / 100.0f) * (POWER_MAX - POWER_MIN);
    g.angle += (ang - g.angle) * 0.5f;                  /* suavizado */
    g.power += (pw  - g.power) * 0.5f;
    if (m >= 35.0f) { g.snap_angle = g.angle; g.snap_power = g.power; }
    return 0;
}

/* ======================================================================= */
/* Dano a tiles, derrumbe                                                  */
/* ======================================================================= */
static void debris(int r, int c, uint8_t t)
{
    float x = c * 16 + 8, y = r * 16 + 8;
    switch (t) {
    case T_WOOD:  spawn(x, y, 6, COL_WOOD,  COL_WOOD_D, 3.0f, 1.5f); break;
    case T_STONE: spawn(x, y, 6, 0x94B2,    0x6B4D,     3.0f, 1.5f); break;
    default:      spawn(x, y, 8, COL_GREEN, COL_WHITE,  3.5f, 2.0f); break;   /* cerdo: puf */
    }
}

/* Devuelve 1 si el tile se destruyo */
static int damage_cell(int r, int c, int dmg)
{
    uint8_t t = g_level[r][c];
    if (!is_block(t) || dmg <= 0) return 0;

    if (g_hp[r][c] <= dmg) {
        g.score += tile_pts[t];
        if (is_pig(t)) { if (g.pigs) g.pigs--; sound_sfx(SFX_PIG); }
        else sound_sfx(t == T_STONE ? SFX_STONE : SFX_WOOD);
        g_level[r][c] = T_EMPTY;
        g_hp[r][c] = 0;
        g.hud_score = 1;
        restore(c * 16, r * 16, 16, 16);
        debris(r, c, t);
        return 1;
    }
    g_hp[r][c] -= dmg;
    sound_sfx(t == T_STONE ? SFX_STONE : SFX_WOOD);
    restore(c * 16, r * 16, 16, 16);                    /* muestra el dano */
    return 0;
}

/* Cada bloque sin apoyo baja una fila; devuelve 1 si algo se movio */
static int settle_step(void)
{
    int moved = 0;
    for (int r = LV_ROWS - 3; r >= 0; r--) {            /* filas 13 y 14 son suelo */
        for (int c = 0; c < LV_COLS; c++) {
            uint8_t t = g_level[r][c];
            if (!is_block(t) || g_level[r + 1][c] != T_EMPTY) continue;

            g_level[r + 1][c] = t;
            g_hp[r + 1][c]    = g_hp[r][c];
            fall_cnt[r + 1][c] = fall_cnt[r][c] + 1;
            g_level[r][c]     = T_EMPTY;
            g_hp[r][c]        = 0;
            fall_cnt[r][c]    = 0;
            moved = 1;
            restore(c * 16, r * 16, 16, 32);
        }
    }
    /* aterrizajes: tiles que cayeron y ya no pueden seguir cayendo */
    for (int r = 0; r < LV_ROWS - 2; r++) {
        for (int c = 0; c < LV_COLS; c++) {
            if (!fall_cnt[r][c] || g_level[r + 1][c] == T_EMPTY) continue;
            uint8_t t = g_level[r][c];
            uint8_t n = fall_cnt[r][c];
            fall_cnt[r][c] = 0;
            if (is_pig(t) && n >= 2)
                damage_cell(r, c, n >= 3 ? 2 : 1);
            else if (n >= 2)
                spawn(c * 16 + 8, r * 16 + 15, 3, 0x8AC5, 0xC5CF, 1.5f, 1.0f);  /* polvo */
        }
    }
    return moved;
}

/* ======================================================================= */
/* Pajaros en vuelo                                                        */
/* ======================================================================= */
static void erase_bird(FlyBird *b)
{
    if (!b->drawn) return;
    const BirdInfo *bi = &binfo[b->type];
    restore(b->ox - bi->w / 2 - 1, b->oy - bi->h / 2 - 1, bi->w + 2, bi->h + 2);
    b->drawn = 0;
}

static void draw_bird(FlyBird *b)
{
    const BirdInfo *bi = &binfo[b->type];
    const uint16_t *spr = bi->fr[b->landed ? FR_HIT : FR_FLY];
    if (b->type == BT_BLACK && b->fuse)                  /* mecha encendida: parpadea */
        spr = bi->fr[(b->fuse & 2) ? FR_HOT : FR_HIT];
    b->ox = (int16_t)b->x;
    b->oy = (int16_t)b->y;
    gfx_sprite(b->ox - bi->w / 2, b->oy - bi->h / 2, bi->w, bi->h, spr);
    b->drawn = 1;
}

/* Dano en area alrededor de (cx, cy) */
static void area_damage(int cx, int cy, float r1, int d1, float r2, int d2)
{
    for (int r = 0; r < LV_ROWS - 2; r++) {
        for (int c = 0; c < LV_COLS; c++) {
            if (!is_block(g_level[r][c])) continue;
            float ddx = (c * 16 + 8) - cx, ddy = (r * 16 + 8) - cy;
            float d = sqrtf(ddx * ddx + ddy * ddy);
            if (d <= r1)      damage_cell(r, c, d1);
            else if (d <= r2) damage_cell(r, c, d2);
        }
    }
}

/* Explosion de la bomba: dano en area + anillos de fuego + chispas */
static void explode(FlyBird *b)
{
    int cx = (int)b->x, cy = (int)b->y;
    b->alive = 0;
    erase_bird(b);
    sound_sfx(SFX_BOOM);

    static const uint8_t rad[5] = { 8, 16, 26, 34, 42 };
    for (int i = 0; i < 5; i++) {
        gfx_fill_circle(cx, cy, rad[i], (i & 1) ? COL_YELLOW : COL_ORANGE);
        if (i == 1) gfx_fill_circle(cx, cy, 6, COL_WHITE);
        HAL_Delay(16);
        sound_update();
    }
    restore(cx - 43, cy - 43, 87, 87);
    area_damage(cx, cy, 26.0f, 3, 42.0f, 2);
    spawn(cx, cy, 12, COL_ORANGE, COL_YELLOW, 5.0f, 2.0f);
}

/* Golpe del pajaro grande en picada: onda expansiva y polvo */
static void dive_impact(FlyBird *b)
{
    b->diving = 0;
    sound_sfx(SFX_BOOM);
    area_damage((int)b->x, (int)b->y + 6, 22.0f, 1, 0.0f, 0);
    spawn(b->x - 6, b->y + 8, 6, 0x8AC5, 0xC5CF, 3.5f, 2.5f);
    spawn(b->x + 6, b->y + 8, 6, 0x8AC5, 0xC5CF, 3.5f, 2.5f);
}

static void kill_or_explode(FlyBird *b)
{
    if (b->type == BT_BLACK) { explode(b); return; }
    b->alive = 0;
    erase_bird(b);
    spawn(b->x, b->y, 5, COL_WHITE, 0xC618, 2.0f, 1.5f);          /* plumas */
}

/* Busca el primer tile solido que toca el pajaro (circulo vs rectangulo) */
static int find_hit(const FlyBird *b, int rr, int *hr, int *hc)
{
    int c0 = (int)floorf((b->x - rr) / 16.0f), c1 = (int)floorf((b->x + rr) / 16.0f);
    int r0 = (int)floorf((b->y - rr) / 16.0f), r1 = (int)floorf((b->y + rr) / 16.0f);
    if (c0 < 0) c0 = 0;
    if (c1 > LV_COLS - 1) c1 = LV_COLS - 1;
    if (r0 < 0) r0 = 0;
    if (r1 > LV_ROWS - 3) r1 = LV_ROWS - 3;

    for (int r = r0; r <= r1; r++) {
        for (int c = c0; c <= c1; c++) {
            if (!is_solid(g_level[r][c])) continue;
            float tl = c * 16.0f, tt = r * 16.0f;
            float qx = fminf(fmaxf(b->x, tl), tl + 16.0f);
            float qy = fminf(fmaxf(b->y, tt), tt + 16.0f);
            float ddx = b->x - qx, ddy = b->y - qy;
            if (ddx * ddx + ddy * ddy < (float)(rr * rr)) { *hr = r; *hc = c; return 1; }
        }
    }
    return 0;
}

static void bird_update(FlyBird *b)
{
    const BirdInfo *bi = &binfo[b->type];
    float rr = bi->r;

    b->age++;
    if (b->fuse && --b->fuse == 0) { explode(b); return; }

    b->vy += GRAV;
    float sp = fmaxf(fabsf(b->vx), fabsf(b->vy));
    if (sp > V_MAX) { float k = V_MAX / sp; b->vx *= k; b->vy *= k; sp = V_MAX; }

    int   n  = (int)(sp / 4.0f) + 1;                    /* subpasos: evita atravesar tiles */
    float sx = b->vx / n, sy = b->vy / n;
    int   ground = 0, touched = 0;

    for (int s = 0; s < n; s++) {
        float px0 = b->x, py0 = b->y;
        b->x += sx;
        b->y += sy;

        if (b->y + rr > GROUND_Y) { b->y = GROUND_Y - rr; ground = 1; }

        int hr, hc;
        if (find_hit(b, (int)rr, &hr, &hc)) {
            float impact = sqrtf(b->vx * b->vx + b->vy * b->vy);
            uint8_t t = g_level[hr][hc];

            /* Madera: se rompe y el pajaro la atraviesa sin rebotar */
            if (t == T_WOOD && impact >= IMPACT_MIN) {
                damage_cell(hr, hc, 99);
                b->vx *= WOOD_SLOW; b->vy *= WOOD_SLOW;
                sx = b->vx / n; sy = b->vy / n;
                continue;
            }

            int mat = (t == T_WOOD) ? 0 : (t == T_STONE) ? 1 : (t == T_PIG) ? 2 : 3;
            int dmg = 0;
            if (impact >= IMPACT_MIN) {
                dmg = dmg_tab[b->type][mat];
                if (impact >= IMPACT_STRONG) dmg++;
            }
            if (b->diving) dive_impact(b);
            int destroyed = damage_cell(hr, hc, dmg);
            touched = 1;
            b->landed = 1;
            if (b->type == BT_BLACK && !b->fuse) b->fuse = FUSE_FRAMES;   /* enciende la mecha */

            if (destroyed) {                                     /* sigue, con menos velocidad */
                float k = (b->type == BT_BIG) ? 0.85f : 0.6f;
                b->vx *= k; b->vy *= k;
            } else {                                             /* rebota */
                float tl = hc * 16.0f, tt = hr * 16.0f;
                int horiz = (px0 + rr <= tl + 0.01f) || (px0 - rr >= tl + 16.0f - 0.01f);
                if (horiz) {
                    b->x  = (px0 < tl) ? tl - rr - 0.5f : tl + 16.0f + rr + 0.5f;
                    b->vx = -b->vx * 0.25f;  b->vy *= 0.8f;
                } else {
                    b->y  = (py0 < tt) ? tt - rr - 0.5f : tt + 16.0f + rr + 0.5f;
                    b->vy = -b->vy * 0.25f;  b->vx *= 0.8f;
                }
            }
            break;
        }
    }

    if (ground) {
        touched = 1;
        b->landed = 1;
        if (b->diving) dive_impact(b);
        if (b->vy > 1.2f) b->vy = -b->vy * 0.3f; else b->vy = 0;
        b->vx *= 0.75f;                                  /* mas friccion: se detiene antes */
        if (b->type == BT_BLACK && !b->fuse) b->fuse = FUSE_FRAMES;
    }
    if (touched && fabsf(b->vx) < 0.6f && fabsf(b->vy) < 1.0f) b->rest++;
    else if (!touched) b->rest = 0;

    /* la mecha encendida echa chispas */
    if (b->fuse && (b->fuse & 1)) spawn(b->x + 2, b->y - bi->h / 2, 1, COL_YELLOW, COL_ORANGE, 1.5f, 1.5f);

    /* fin del vuelo: reposo, demasiado tiempo, o fuera de pantalla */
    if (b->rest >= REST_FRAMES || b->age > MAX_AGE) { kill_or_explode(b); return; }
    if (b->x < -30 || b->x > SCREEN_W + 30 || b->y > SCREEN_H + 30) {
        b->alive = 0; erase_bird(b);
    }
}

/* Poder especial */
static void activate_power(FlyBird *b)
{
    b->used_power = 1;
    switch (b->type) {
    case BT_YELLOW:                                  /* acelera: estela de chispas */
        sound_sfx(SFX_POWER);
        b->vx *= 1.9f; b->vy *= 1.9f;
        spawn(b->x - b->vx, b->y - b->vy, 10, COL_WHITE, COL_YELLOW, 2.5f, 0.0f);
        break;
    case BT_BLACK:                                   /* explota ya */
        explode(b);
        break;
    case BT_BLUE: {                                  /* se divide en 3 */
        static const float ang[2] = { -0.22f, 0.22f };
        sound_sfx(SFX_POWER);
        spawn(b->x, b->y, 8, COL_WHITE, 0x5D7C, 2.5f, 0.5f);
        for (int k = 0; k < 2; k++) {
            for (int i = 0; i < MAX_FLY; i++) {
                if (g.fly[i].alive) continue;
                FlyBird *n = &g.fly[i];
                *n = *b;
                n->drawn = 0;
                n->used_power = 1;
                float ca = cosf(ang[k]), sa = sinf(ang[k]);
                n->vx = b->vx * ca - b->vy * sa;
                n->vy = b->vx * sa + b->vy * ca;
                n->alive = 1;
                break;
            }
        }
        break;
    }
    case BT_BIG:                                     /* se estrella hacia abajo */
        sound_sfx(SFX_POWER);
        b->vx *= 0.2f; b->vy = 9.0f;
        b->diving = 1;
        spawn(b->x, b->y - 8, 6, COL_WHITE, COL_RED, 2.0f, 1.0f);
        break;
    default: break;                                  /* rojo: sin poder */
    }
}

static void launch(void)
{
    float a = g.angle * PI_F / 180.0f;
    float v = V_MIN + g.power * V_PER_POWER;

    aim_erase();                                     /* quita pajaro, ligas y trayectoria */
    info_clear();
    gfx_line(BAND_BACK_X, BAND_BACK_Y, BAND_FRONT_X, BAND_FRONT_Y, 2, COL_BAND);   /* liga suelta */
    sound_sfx(SFX_LAUNCH);

    memset(g.fly, 0, sizeof(g.fly));
    FlyBird *b = &g.fly[0];
    b->x = REST_X; b->y = REST_Y;
    b->vx = v * cosf(a); b->vy = -v * sinf(a);
    b->type = cur_type();
    b->alive = 1;
    g.ntrail = 0; g.trail_tick = 0; g.hn = 0;
    g.phase = PH_FLIGHT;
}

static void flight_update(uint8_t use_power)
{
    int i;
    if (g.fly[0].age == 2) restore(BAND_FRONT_X - 2, BAND_BACK_Y - 2, 16, 8);   /* quita la liga suelta */

    if (use_power) {
        /* 1) si hay un pajaro en el aire con poder disponible, lo usa */
        int done = 0;
        for (i = 0; i < MAX_FLY; i++) {
            FlyBird *b = &g.fly[i];
            if (b->alive && !b->used_power && !b->landed && b->type != BT_RED) {
                activate_power(b);
                done = 1;
                break;
            }
        }
        /* 2) Si todos los pajaros ya cayeron, se quitan y pasa el siguiente */
        if (!done) {
            int alive = 0, all_landed = 1;
            for (i = 0; i < MAX_FLY; i++) {
                if (!g.fly[i].alive) continue;
                alive = 1;
                if (!g.fly[i].landed) all_landed = 0;
            }
            if (alive && all_landed)
                for (i = 0; i < MAX_FLY; i++)
                    if (g.fly[i].alive) kill_or_explode(&g.fly[i]);
        }
    }
    for (i = 0; i < MAX_FLY; i++) if (g.fly[i].alive) erase_bird(&g.fly[i]);
    parts_erase();
    for (i = 0; i < MAX_FLY; i++) if (g.fly[i].alive) bird_update(&g.fly[i]);

    /* Estela: un punto cada 3 cuadros donde estaba el pajaro hace 4 */
    for (i = 0; i < MAX_FLY; i++) {
        FlyBird *b = &g.fly[i];
        if (!b->alive) continue;
        int k = g.hn & 3;
        if (g.hn >= 4 && ++g.trail_tick >= 3 && g.ntrail < TRAIL_MAX) {
            g.trail_tick = 0;
            int tx = (int)g.hx[k], ty = (int)g.hy[k];
            float ddx = g.hx[k] - b->x, ddy = g.hy[k] - b->y;
            float rmin = binfo[b->type].r + 4.0f;
            if (tx >= 2 && tx <= SCREEN_W - 3 && ty >= 30 && ty <= GROUND_Y - 3 &&
                ddx * ddx + ddy * ddy > rmin * rmin) {
                g.trail[g.ntrail][0] = tx;
                g.trail[g.ntrail][1] = ty;
                g.ntrail++;
                FillRect(tx - 1, ty - 1, 2, 2, COL_WHITE);
            }
        }
        g.hx[k] = b->x; g.hy[k] = b->y; g.hn++;
        break;                                       /* solo el primer pajaro vivo */
    }
    parts_update_draw();
    for (i = 0; i < MAX_FLY; i++) if (g.fly[i].alive) draw_bird(&g.fly[i]);

    int any = 0;
    for (i = 0; i < MAX_FLY; i++) if (g.fly[i].alive) any = 1;
    if (!any) {                                      /* termino el vuelo */
        for (i = 0; i < g.ntrail; i++)
            restore(g.trail[i][0] - 2, g.trail[i][1] - 2, 5, 5);
        g.ntrail = 0;
        g.qi++;
        g.settle_tick = 0;
        g.settle_idle = 0;
        g.phase = PH_SETTLE;
    }
}

/* ======================================================================= */
/* API                                                                     */
/* ======================================================================= */
void game_start(uint8_t level)
{
    memset(&g, 0, sizeof(g));
    memset(fall_cnt, 0, sizeof(fall_cnt));
    rng ^= HAL_GetTick();
    g.def = level_load(level);
    g.level = (level >= 1 && level <= NUM_LEVELS) ? level : 1;
    g.angle = 45.0f; g.adir = 1;
    g.power = POWER_MIN; g.pdir = 1;
    g.snap_angle = 45.0f; g.snap_power = POWER_MIN;
    for (int r = 0; r < LV_ROWS; r++)
        for (int c = 0; c < LV_COLS; c++)
            if (is_pig(g_level[r][c])) g.pigs++;

    scene_restore(0, 0, SCREEN_W, SCREEN_H);
    hud_draw();
    begin_aim();
}

GameResult game_update(const GameInput *in)
{
    GameResult res = GAME_RUNNING;
    if (g.done) return g.result;                 /* ya termino: no repetir bonos */
    if (in->quit || in->ev == 2) return GAME_QUIT;

    uint8_t ev = in->ev;                         /* boton B1 de la Nucleo */
    g.connected = in->connected;
    if (in->connected && (g.phase == PH_ANGLE || g.phase == PH_POWER)) g.phase = PH_AIM;
    else if (!in->connected && g.phase == PH_AIM) { g.phase = PH_ANGLE; g.adir = 1; }

    switch (g.phase) {
    case PH_ANGLE:
        g.angle += g.adir * ANGLE_STEP;
        if (g.angle >= ANGLE_MAX) { g.angle = ANGLE_MAX; g.adir = -1; }
        if (g.angle <= ANGLE_MIN) { g.angle = ANGLE_MIN; g.adir =  1; }
        if (ev == 1) { g.phase = PH_POWER; g.power = POWER_MIN; g.pdir = 1; }
        aim_draw(0);
        break;

    case PH_POWER:
        g.power += g.pdir * POWER_STEP;
        if (g.power >= POWER_MAX) { g.power = POWER_MAX; g.pdir = -1; }
        if (g.power <= POWER_MIN) { g.power = POWER_MIN; g.pdir =  1; }
        if (ev == 1) { launch(); break; }
        aim_draw(0);
        break;

    case PH_AIM: {
        int released = aim_from_stick(in->jx, in->jy);
        if (released || in->fire || ev == 1) { launch(); break; }
        aim_draw(0);
        break;
    }

    case PH_FLIGHT:
        flight_update((ev == 1) || in->power || in->fire);
        break;

    case PH_SETTLE:
        parts_erase();
        if (++g.settle_tick >= 3) {                  /* los bloques caen una fila cada 3 cuadros */
            g.settle_tick = 0;
            if (settle_step()) g.settle_idle = 0; else g.settle_idle++;
        }
        parts_update_draw();
        if (g.settle_idle >= 3 && !parts_alive()) {  /* todo quieto (~0.3 s): evaluar */
            if (g.pigs == 0) {
                g.score += 1000 * (uint16_t)(g.def->nbirds - g.qi);   /* bono por pajaros */
                res = GAME_WIN;
            } else if (g.qi >= g.def->nbirds) {
                res = GAME_LOSE;
            } else {
                g.hud_full = 1;
                begin_aim();
            }
        }
        break;
    }

    if (res != GAME_RUNNING) { g.done = 1; g.result = res; }
    if (g.hud_full) hud_draw();
    else if (g.hud_score) { hud_score_draw(); g.hud_score = 0; }
    return res;
}

uint16_t game_score(void) { return g.score; }

/* Estado para el mando: 1 apuntando, 2 en vuelo, 3 derrumbe */
uint8_t game_status_state(void)
{
    switch (g.phase) {
    case PH_FLIGHT: return 2;
    case PH_SETTLE: return 3;
    default:        return 1;
    }
}

/* bit0 = hay un pajaro en vuelo con poder disponible (LED / luz del control) */
uint8_t game_status_flags(void)
{
    if (g.phase != PH_FLIGHT) return 0;
    for (int i = 0; i < MAX_FLY; i++)
        if (g.fly[i].alive && !g.fly[i].used_power && g.fly[i].type != BT_RED) return 1;
    return 0;
}

uint8_t game_stars(void)
{
    if (g.score >= g.def->star3) return 3;
    if (g.score >= g.def->star2) return 2;
    return 1;
}
