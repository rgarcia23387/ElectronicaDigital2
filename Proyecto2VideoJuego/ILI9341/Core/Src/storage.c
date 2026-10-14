/* storage.c - lectura y escritura de los archivos del juego en la SD (FatFs) */
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "storage.h"
#include "sprites.h"
#include "gfx.h"

uint8_t  bk_ok = 0;
uint16_t bk_pal[16];
uint8_t  bk_px[BK_W * BK_H / 2];

#if !__has_include("fatfs.h")
/* ---------------------------------------------------------------------- */
/* FATFS todavia no esta activado en CubeMX: el juego funciona sin SD      */
/* (graficos de la flash, cielo plano, niveles del programa).              */
/* ---------------------------------------------------------------------- */
uint8_t storage_init(void) { return 0; }
uint8_t storage_ok(void) { return 0; }
uint16_t storage_error(void) { return 999; }        /* FATFS no activado */
int  storage_load_level(uint8_t id, LevelDef *out) { (void)id; (void)out; return 0; }
void storage_load_records(uint16_t *best, uint8_t n) { (void)best; (void)n; }
void storage_save_records(const uint16_t *best, uint8_t n) { (void)best; (void)n; }
int  storage_load_sprites(void) { return 0; }
int  storage_show_image(const char *name) { (void)name; return 0; }
int  storage_load_text(const char *name, char *buf, int size) { (void)name; (void)buf; (void)size; return 0; }
void storage_load_stats(GameStats *s) { (void)s; }
void storage_save_stats(const GameStats *s) { (void)s; }

#else
#include "fatfs.h"

static uint8_t sd_ok = 0;
static uint8_t mount_res = 0;                       /* FRESULT de f_mount */
extern uint8_t sd_init_step;                        /* paso donde fallo la SD (sd_spi.c) */

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

uint16_t storage_error(void) { return (uint16_t)(mount_res * 10u + sd_init_step); }

static int load_backdrop(void)
{
    FIL f;
    UINT n;
    uint8_t hdr[8 + 32];
    if (f_open(&f, "FONDO.BIN", FA_READ) != FR_OK) return 0;

    int ok = (f_read(&f, hdr, sizeof(hdr), &n) == FR_OK && n == sizeof(hdr) &&
              memcmp(hdr, "BK01", 4) == 0 && rd16(hdr + 4) == BK_W && rd16(hdr + 6) == BK_H);
    if (ok) {
        for (int i = 0; i < 16; i++) bk_pal[i] = rd16(hdr + 8 + 2 * i);
        ok = (f_read(&f, bk_px, sizeof(bk_px), &n) == FR_OK && n == sizeof(bk_px));
    }
    f_close(&f);
    return ok;
}

uint8_t storage_init(void)
{
    FRESULT r = f_mount(&USERFatFS, USERPath, 1);
    if (r != FR_OK) {                                /* un segundo intento (tarjetas lentas) */
        HAL_Delay(100);
        r = f_mount(&USERFatFS, USERPath, 1);
    }
    mount_res = (uint8_t)r;
    sd_ok = (r == FR_OK);
    bk_ok = sd_ok ? (uint8_t)load_backdrop() : 0;
    return sd_ok;
}

uint8_t storage_ok(void) { return sd_ok; }

/* ---------------------------------------------------------------------- */
/* Graficos                                                                */
/* ---------------------------------------------------------------------- */
int storage_load_sprites(void)
{
    FIL f;
    UINT n;
    uint8_t hdr[6];
    if (!sd_ok || f_open(&f, "SPRITES.BIN", FA_READ) != FR_OK) return 0;

    int ok = (f_read(&f, hdr, 6, &n) == FR_OK && n == 6 &&
              memcmp(hdr, "SP01", 4) == 0 && rd16(hdr + 4) == SPRITE_SLOTS);
    for (int i = 0; ok && i < SPRITE_SLOTS; i++) {
        uint8_t c[2];
        UINT bytes = sprite_slots[i].n * 2u;
        ok = (f_read(&f, c, 2, &n) == FR_OK && n == 2 && rd16(c) == sprite_slots[i].n);
        /* el STM32 es little-endian igual que el archivo: se lee directo al arreglo */
        if (ok) ok = (f_read(&f, sprite_slots[i].dst, bytes, &n) == FR_OK && n == bytes);
    }
    f_close(&f);
    return ok;
}

int storage_show_image(const char *name)
{
    FIL f;
    UINT n;
    uint8_t hdr[8];
    static uint16_t buf[256];                    /* 512 bytes = un sector de la SD */
    if (!sd_ok || f_open(&f, name, FA_READ) != FR_OK) return 0;

    int ok = (f_read(&f, hdr, 8, &n) == FR_OK && n == 8 && memcmp(hdr, "IM01", 4) == 0);
    int w = rd16(hdr + 4), h = rd16(hdr + 6);
    if (ok && (w < 1 || h < 1 || w > SCREEN_W || h > SCREEN_H)) ok = 0;
    if (ok) {
        uint32_t left = (uint32_t)w * h;
        gfx_stream_begin((SCREEN_W - w) / 2, (SCREEN_H - h) / 2, w, h);
        while (left) {
            UINT want = (left > 256 ? 256 : left) * 2u;
            if (f_read(&f, buf, want, &n) != FR_OK || n != want) { ok = 0; break; }
            gfx_stream_push(buf, (int)(n / 2));
            left -= n / 2;
        }
        gfx_stream_end();
    }
    f_close(&f);
    return ok;
}

int storage_load_text(const char *name, char *buf, int size)
{
    FIL f;
    UINT len;
    if (!sd_ok || size < 2 || f_open(&f, name, FA_READ) != FR_OK) return 0;
    if (f_read(&f, buf, (UINT)(size - 1), &len) != FR_OK) len = 0;
    f_close(&f);
    buf[len] = '\0';
    return len > 0;
}

/* ---------------------------------------------------------------------- */
/* Niveles                                                                 */
/* ---------------------------------------------------------------------- */
int storage_load_level(uint8_t id, LevelDef *out)
{
    FIL f;
    UINT n;
    char name[16];
    uint8_t buf[4 + 1 + 8 + 4 + LV_ROWS * LV_COLS];
    if (!sd_ok) return 0;

    snprintf(name, sizeof(name), "NIVEL%u.BIN", (unsigned)id);
    if (f_open(&f, name, FA_READ) != FR_OK) return 0;
    int ok = (f_read(&f, buf, sizeof(buf), &n) == FR_OK && n == sizeof(buf) &&
              memcmp(buf, "LV01", 4) == 0 && buf[4] >= 1 && buf[4] <= MAX_BIRDS);
    f_close(&f);
    if (!ok) return 0;

    /* valida los codigos antes de usarlos */
    for (int i = 0; i < 8; i++)            if (buf[5 + i] >= BT_COUNT)          return 0;
    for (int i = 0; i < LV_ROWS * LV_COLS; i++) if (buf[17 + i] >= T_COUNT)    return 0;

    out->nbirds = buf[4];
    memcpy(out->birds, buf + 5, MAX_BIRDS);
    out->star2 = rd16(buf + 13);
    out->star3 = rd16(buf + 15);
    memcpy(out->tiles, buf + 17, LV_ROWS * LV_COLS);
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Records y estadisticas (archivos de texto: se pueden abrir en la PC)    */
/* ---------------------------------------------------------------------- */
static void write_text(const char *name, const char *txt)
{
    FIL f;
    UINT w;
    if (!sd_ok || f_open(&f, name, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) return;
    f_write(&f, txt, (UINT)strlen(txt), &w);
    f_close(&f);                             /* cerrar = se escribe de verdad en la SD */
}

void storage_load_records(uint16_t *best, uint8_t n)
{
    char txt[256];
    if (!storage_load_text("RECORDS.TXT", txt, sizeof(txt))) return;
    for (char *p = txt; p && *p; ) {             /* una linea a la vez */
        unsigned lv, pts;
        if (sscanf(p, "NIVEL%u=%u", &lv, &pts) == 2 && lv >= 1 && lv <= n)
            best[lv - 1] = (uint16_t)(pts > 65535 ? 65535 : pts);
        p = strchr(p, '\n');
        if (p) p++;
    }
}

void storage_save_records(const uint16_t *best, uint8_t n)
{
    char txt[256];
    int len = 0;
    for (uint8_t i = 0; i < n && len < (int)sizeof(txt) - 20; i++)
        len += snprintf(txt + len, sizeof(txt) - len, "NIVEL%u=%05u\r\n",
                        (unsigned)(i + 1), (unsigned)best[i]);
    write_text("RECORDS.TXT", txt);
}

void storage_load_stats(GameStats *s)
{
    char txt[160];
    if (!storage_load_text("STATS.TXT", txt, sizeof(txt))) return;
    for (char *p = txt; p && *p; ) {
        unsigned long v;
        if      (sscanf(p, "PARTIDAS=%lu",  &v) == 1) s->partidas  = (uint16_t)v;
        else if (sscanf(p, "VICTORIAS=%lu", &v) == 1) s->victorias = (uint16_t)v;
        else if (sscanf(p, "DERROTAS=%lu",  &v) == 1) s->derrotas  = (uint16_t)v;
        else if (sscanf(p, "PUNTOS=%lu",    &v) == 1) s->puntos    = (uint32_t)v;
        p = strchr(p, '\n');
        if (p) p++;
    }
}

void storage_save_stats(const GameStats *s)
{
    char txt[160];
    snprintf(txt, sizeof(txt), "PARTIDAS=%u\r\nVICTORIAS=%u\r\nDERROTAS=%u\r\nPUNTOS=%lu\r\n",
             (unsigned)s->partidas, (unsigned)s->victorias, (unsigned)s->derrotas,
             (unsigned long)s->puntos);
    write_text("STATS.TXT", txt);
}

#endif /* fatfs.h */
