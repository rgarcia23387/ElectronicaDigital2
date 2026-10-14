/* synth.c - sintetizador de 3 voces para el DAC y el buzzer pasivo */
#include "main.h"
#include "synth.h"
#include "audio_cmd.h"

extern DAC_HandleTypeDef hdac;              /* los genera CubeMX en main.c */
extern TIM_HandleTypeDef htim6;
#if SYNTH_BUZZER
extern TIM_HandleTypeDef htim3;             /* PWM del buzzer pasivo (PA6) */
#endif

/* ---------------------------------------------------------------------- */
/* Notas (Hz)                                                              */
/* ---------------------------------------------------------------------- */
#define A2  110
#define B2b 117
#define C3  131
#define D3  147
#define E3  165
#define F3  175
#define G3  196
#define A3  220
#define B3b 233
#define C4  262
#define C4s 277
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4b 466
#define C5  523
#define D5  587
#define F5  698
#define A5  880
#define B5b 932
#define C6  1047
#define D6  1175
#define R   0

#define NOTE_REST  0
#define NOTE_D4    294
#define NOTE_E4    330
#define NOTE_FS4   370
#define NOTE_G4    392
#define NOTE_A4    440
#define NOTE_AS4   466
#define NOTE_B4    494
#define NOTE_C5    523
#define NOTE_CS5   554
#define NOTE_D5    587
#define NOTE_E5    659

#define T_E   140
#define T_Q   (2 * T_E)
#define T_H   (4 * T_E)

/* Formas de onda */
enum { W_SQ50 = 0, W_SQ25, W_TRI, W_NOISE, W_DEF = 0xFF };

typedef struct { uint16_t f0, f1, ms; uint8_t wave; } Tone;

#define N(f, d)       { (f), 0,   (d), W_DEF }     /* nota con la onda de la pista */
#define S(a, b, d)    { (a), (b), (d), W_DEF }     /* barrido de a hasta b         */
#define NZ(a, b, d)   { (a), (b), (d), W_NOISE }   /* ruido (explosiones, golpes)  */
#define LEN(x)        (sizeof(x) / sizeof((x)[0]))

/* ---------------------------------------------------------------------- */
/* Musica                                                                  */
/* ---------------------------------------------------------------------- */
#include "musica_menu.h"          /* mus_title: las notas que paso el estudiante */

/* Fondo del juego (composicion propia): melodia + bajo */
static const Tone lvl_mel[] = {
    N(D4,T_E), N(R,T_E), N(A4,T_E), N(R,T_E), N(F4,T_E), N(A4,T_E), N(R,T_Q),
    N(C4,T_E), N(R,T_E), N(G4,T_E), N(R,T_E), N(E4,T_E), N(G4,T_E), N(R,T_Q),
    N(B3b,T_E), N(R,T_E), N(F4,T_E), N(R,T_E), N(D4,T_E), N(F4,T_E), N(R,T_Q),
    N(A3,T_E), N(R,T_E), N(E4,T_E), N(C4s,T_E), N(A3,T_Q), N(R,T_Q),
};
static const Tone lvl_bass[] = {
    N(D3,T_Q), N(A2,T_Q), N(D3,T_Q), N(A2,T_Q),
    N(C3,T_Q), N(G3,T_Q), N(C3,T_Q), N(G3,T_Q),
    N(B2b,T_Q), N(F3,T_Q), N(B2b,T_Q), N(F3,T_Q),
    N(A2,T_Q), N(E3,T_Q), N(A2,T_Q), N(E3,T_Q),
};

/* Nivel completo */
static const Tone win_mel[] = {
    N(D5,T_E), N(F5,T_E), N(A5,T_E), N(D6,T_Q), N(R,T_E),
    N(C6,T_E), N(A5,T_E), N(B5b,T_E), N(C6,T_E), N(D6,T_H),
};
static const Tone win_bass[] = {
    N(D3,T_Q), N(A3,T_Q), N(F3,T_Q), N(R,T_E),
    N(F3,T_Q), N(G3,T_Q), N(D3,T_H),
};

/* Game over */
static const Tone lose_mel[] = {
    S(A4, F4, 300), N(R, 60), S(G4, E4, 300), N(R, 60),
    S(F4, C4s, 350), N(R, 80), S(E4, A3, 700),
};
static const Tone lose_bass[] = {
    N(D3, 360), N(C3, 360), N(B2b, 430), N(A2, 700),
};

/* ---------------------------------------------------------------------- */
/* Efectos                                                                 */
/* ---------------------------------------------------------------------- */
static const Tone sfx_move[]   = { N(1200, 30) };
static const Tone sfx_ok[]     = { N(880, 50), N(1320, 90) };
static const Tone sfx_launch[] = { S(300, 1500, 250) };
static const Tone sfx_wood[]   = { NZ(900, 300, 70) };
static const Tone sfx_stone[]  = { N(900, 35), N(R, 15), N(650, 35), NZ(2000, 800, 40) };
static const Tone sfx_pig[]    = { S(600, 1800, 90), N(R, 20), N(1800, 40) };
static const Tone sfx_boom[]   = { NZ(3000, 200, 450) };
static const Tone sfx_power[]  = { S(800, 1600, 80), S(1600, 800, 80) };

static const Tone *const sfx_tab[SFX_COUNT] = {
    sfx_move, sfx_ok, sfx_launch, sfx_wood, sfx_stone, sfx_pig, sfx_boom, sfx_power
};
static const uint8_t sfx_len[SFX_COUNT] = {
    LEN(sfx_move), LEN(sfx_ok), LEN(sfx_launch), LEN(sfx_wood),
    LEN(sfx_stone), LEN(sfx_pig), LEN(sfx_boom), LEN(sfx_power)
};

/* ---------------------------------------------------------------------- */
/* Voces (las lee la interrupcion)                                         */
/* ---------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t inc;      /* incremento de fase (0 = callada) */
    volatile uint16_t amp;      /* amplitud actual 0..600           */
    volatile uint8_t  wave;
    uint32_t phase;
    uint16_t lfsr;              /* generador de ruido */
} Voice;

static Voice voice[3];
static volatile uint16_t master = 100;      /* volumen general 0..100 */
static volatile uint8_t  muted = 0;

static uint32_t freq_to_inc(uint32_t f)
{
    return (uint32_t)(((uint64_t)f << 32) / SYNTH_FS);
}

/* Una muestra: se llama SYNTH_FS veces por segundo desde TIM6 */
void synth_sample_isr(void)
{
    int32_t mix = 0;
    for (int i = 0; i < 3; i++) {
        Voice *v = &voice[i];
        if (!v->inc || !v->amp) continue;
        uint32_t old = v->phase;
        v->phase += v->inc;
        int32_t a = v->amp;
        switch (v->wave) {
        case W_SQ25:
            mix += (v->phase < 0x40000000u) ? a : -a;
            break;
        case W_TRI: {                              /* triangular: sube y baja */
            int32_t t = (int32_t)(v->phase >> 20);   /* 0..4095 */
            t = (t < 2048) ? t : 4095 - t;           /* 0..2047..0 */
            mix += ((t - 1024) * a) / 1024;
            break;
        }
        case W_NOISE:                              /* ruido: LFSR al pasar cada periodo */
            if (v->phase < old) {
                uint16_t bit = ((v->lfsr >> 0) ^ (v->lfsr >> 2) ^ (v->lfsr >> 3) ^ (v->lfsr >> 5)) & 1u;
                v->lfsr = (uint16_t)((v->lfsr >> 1) | (bit << 15));
            }
            mix += (v->lfsr & 1u) ? a : -a;
            break;
        default:                                   /* cuadrada 50 % */
            mix += (v->phase < 0x80000000u) ? a : -a;
            break;
        }
    }
    if (muted) mix = 0;
    int32_t out = 2048 + (mix * (int32_t)master) / 100;
    if (out < 0) out = 0;
    if (out > 4095) out = 4095;
    hdac.Instance->DHR12R1 = (uint32_t)out;        /* escritura directa: rapida */
}

/* ---------------------------------------------------------------------- */
/* Secuenciador (cada 1 ms, en el main)                                    */
/* ---------------------------------------------------------------------- */
typedef struct {
    const Tone *seq;
    uint16_t len, idx;
    uint32_t t0;
    uint8_t  loop, active, wave;
    uint16_t peak;              /* amplitud al empezar cada nota */
} Track;

static Track trk[3];                       /* 0 melodia, 1 bajo, 2 efectos */
static uint16_t cur_f[3];                  /* frecuencia actual de cada voz (para el buzzer) */
static uint8_t cur_music = MUS_NONE;

static void track_start(Track *t, const Tone *seq, uint16_t len, uint8_t loop,
                        uint8_t wave, uint16_t peak)
{
    t->seq = seq; t->len = len; t->idx = 0; t->loop = loop;
    t->wave = wave; t->peak = peak;
    t->active = (seq != 0 && len > 0);
    t->t0 = HAL_GetTick();
}

/* Calcula frecuencia y amplitud de la pista ahora; las escribe en su voz */
static void track_step(Track *t, Voice *v, uint32_t now, uint8_t legato)
{
    while (t->active) {
        const Tone *n = &t->seq[t->idx];
        uint32_t dt = now - t->t0;
        if (dt < n->ms) {
            uint32_t f = n->f0;
            if (n->f1) f = n->f0 + (int32_t)(n->f1 - n->f0) * (int32_t)dt / n->ms;
            /* envolvente: ataque instantaneo, cae al 60 % y se apaga al final */
            uint32_t amp = t->peak;
            uint32_t sus = t->peak * 6 / 10;
            uint32_t dec = (n->ms < 120) ? n->ms : 120;
            amp = (dt < dec) ? t->peak - (t->peak - sus) * dt / dec : sus;
            if (!legato && dt > (uint32_t)n->ms * 85 / 100) amp = 0;   /* separa notas */
            v->wave = (n->wave == W_DEF) ? t->wave : n->wave;
            v->inc  = f ? freq_to_inc(f) : 0;
            v->amp  = f ? (uint16_t)amp : 0;
            cur_f[v - voice] = (f && amp) ? (uint16_t)f : 0;
            return;
        }
        t->t0 += n->ms;
        if (++t->idx >= t->len) {
            if (t->loop) t->idx = 0;
            else         t->active = 0;
        }
    }
    v->amp = 0;
    v->inc = 0;
    cur_f[v - voice] = 0;
}

#if SYNTH_BUZZER
/* Buzzer: onda cuadrada a la frecuencia de la voz principal */
static void buzzer_out(void)
{
    static uint32_t last = 0xFFFFFFFF;
    uint32_t f = cur_f[2] ? cur_f[2] : cur_f[0] ? cur_f[0] : cur_f[1];
    if (f && voice[2].wave == W_NOISE && cur_f[2])      /* ruido: tonos al azar */
        f = 150 + (voice[2].lfsr % 1200);
    if (muted) f = 0;
    if (f == last) return;
    last = f;
    if (f < 40) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
        return;
    }
    uint32_t arr = 1000000UL / f - 1;
    uint32_t ccr = (arr / 2) * master / 100;
    __HAL_TIM_SET_AUTORELOAD(&htim3, arr);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr ? ccr : 1);
}
#endif

void synth_update(void)
{
    static uint32_t last = 0;
    uint32_t now = HAL_GetTick();
    if (now == last) return;
    last = now;
    track_step(&trk[0], &voice[0], now, 0);
    track_step(&trk[1], &voice[1], now, 0);
    track_step(&trk[2], &voice[2], now, 1);
#if SYNTH_BUZZER
    buzzer_out();
#endif
}

static void music(uint8_t m)
{
    if (m == cur_music && (trk[0].active || m == MUS_NONE)) return;
    cur_music = m;
    trk[0].active = trk[1].active = 0;
    switch (m) {
    case MUS_TITLE:
        track_start(&trk[0], mus_title, LEN(mus_title), 1, W_SQ25, 420);
        break;
    case MUS_LEVEL:
        track_start(&trk[0], lvl_mel,  LEN(lvl_mel),  1, W_SQ25, 300);
        track_start(&trk[1], lvl_bass, LEN(lvl_bass), 1, W_TRI,  520);
        break;
    case MUS_WIN:
        track_start(&trk[0], win_mel,  LEN(win_mel),  0, W_SQ25, 420);
        track_start(&trk[1], win_bass, LEN(win_bass), 0, W_TRI,  520);
        break;
    case MUS_LOSE:
        track_start(&trk[0], lose_mel,  LEN(lose_mel),  0, W_SQ50, 380);
        track_start(&trk[1], lose_bass, LEN(lose_bass), 0, W_TRI,  520);
        break;
    default:
        break;
    }
}

void synth_command(uint8_t cmd)
{
    if (cmd >= CMD_SFX_BASE && cmd < CMD_SFX_BASE + SFX_COUNT) {
        uint8_t s = cmd - CMD_SFX_BASE;
        track_start(&trk[2], sfx_tab[s], sfx_len[s], 0, W_SQ50, 480);
    } else if (cmd >= CMD_MUS_BASE && cmd < CMD_MUS_BASE + MUS_COUNT) {
        music(cmd - CMD_MUS_BASE);
    } else if (cmd >= CMD_VOL_BASE && cmd <= CMD_VOL_BASE + 10) {
        master = (uint16_t)(cmd - CMD_VOL_BASE) * 10;
    } else if (cmd == CMD_MUTE_ON) {
        muted = 1;
    } else if (cmd == CMD_MUTE_OFF) {
        muted = 0;
    }
}

void synth_init(void)
{
    for (int i = 0; i < 3; i++) { voice[i].lfsr = 0xACE1u; voice[i].inc = 0; voice[i].amp = 0; }
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048);   /* silencio = mitad */
    HAL_TIM_Base_Start_IT(&htim6);
#if SYNTH_BUZZER
    htim3.Instance->CR1 |= TIM_CR1_ARPE;           /* cambia de nota sin chasquidos */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
#endif
}
