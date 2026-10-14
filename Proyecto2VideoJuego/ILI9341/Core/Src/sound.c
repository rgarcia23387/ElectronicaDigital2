/* sound.c - musica y efectos de 8 bits (buzzer local o Nucleo de audio) */
#include "main.h"
#include "sound.h"

#if SOUND_REMOTE
/* ====================================================================== */
/* Modo remoto: la Nucleo de audio genera el sonido (DAC + amplificador).  */
/* Aqui solo se manda 1 byte por evento por UART5 (PC12 -> PD2 de la otra). */
/* ====================================================================== */
#include "audio_cmd.h"

extern UART_HandleTypeDef huart5;          /* lo genera CubeMX en main.c */

static SoundMusic cur_music = (SoundMusic)0xFF;

static void send_cmd(uint8_t c)
{
    HAL_UART_Transmit(&huart5, &c, 1, 2);  /* 1 byte = 87 us a 115200 */
}

void sound_init(void)
{
    cur_music = (SoundMusic)0xFF;
    send_cmd(CMD_MUTE_OFF);
    send_cmd(CMD_MUS_BASE + MUS_NONE);
}

void sound_update(void) { }                /* el ritmo lo lleva la otra Nucleo */

void sound_music(SoundMusic m)
{
    if (m == cur_music) return;            /* no reinicia si ya esta sonando */
    cur_music = m;
    send_cmd(CMD_MUS_BASE + (uint8_t)m);
}

void sound_sfx(SoundSfx s)
{
    if (s < SFX_COUNT) send_cmd(CMD_SFX_BASE + (uint8_t)s);
}

void sound_volume(uint8_t percent)
{
    send_cmd(CMD_VOL_BASE + (percent > 100 ? 10 : percent / 10));
}

void sound_mute(uint8_t on)
{
    send_cmd(on ? CMD_MUTE_ON : CMD_MUTE_OFF);
}

#else
/* ====================================================================== */
/* Modo local: buzzer pasivo en esta Nucleo (TIM3_CH1, PC6)                */
/* ====================================================================== */
extern TIM_HandleTypeDef htim3;            /* lo genera CubeMX en main.c */
#define BUZZER_CH  TIM_CHANNEL_1

/* ---------------------------------------------------------------------- */
/* Notas (Hz)                                                              */
/* ---------------------------------------------------------------------- */
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
#define C5s 554
#define D5  587
#define E5  659
#define F5  698
#define G5  784
#define A5  880
#define B5b 932
#define C6  1047
#define D6  1175
#define R   0                               /* silencio */

/* Nombres usados en el tema del menu (frecuencias en Hz) */
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

/* Duraciones (ms) */
#define T_E   140                           /* corchea */
#define T_Q   (2 * T_E)                     /* negra   */
#define T_H   (4 * T_E)                     /* blanca  */

typedef struct { uint16_t f0, f1, ms; } Tone;

#define N(f, d)  { (f), 0, (d) }            /* nota fija            */
#define S(a, b, d) { (a), (b), (d) }        /* barrido de a hasta b */
#define LEN(x)   (sizeof(x) / sizeof((x)[0]))

/* ---------------------------------------------------------------------- */
/* Musica (composiciones propias)                                          */
/* ---------------------------------------------------------------------- */

/* Tema del menu (notas proporcionadas por el estudiante): {frecuencia, ms} */
static const Tone mus_title[] = {
    N(NOTE_D4,   110), N(NOTE_E4,   110),

    N(NOTE_G4,   220), N(NOTE_E4,   220),
    N(NOTE_B4,   220), N(NOTE_E4,   110),
    N(NOTE_FS4,  110), N(NOTE_G4,   220),
    N(NOTE_B4,   220), N(NOTE_A4,   440),

    N(NOTE_A4,   110), N(NOTE_B4,   110),
    N(NOTE_A4,   110), N(NOTE_G4,   110),
    N(NOTE_FS4,  440), N(NOTE_G4,   220),
    N(NOTE_FS4,  110), N(NOTE_E4,   110),
    N(NOTE_REST, 220), N(NOTE_D4,   110),
    N(NOTE_E4,   110),

    N(NOTE_G4,   220), N(NOTE_E4,   220),
    N(NOTE_REST, 220), N(NOTE_G4,   110),
    N(NOTE_A4,   110), N(NOTE_B4,   220),
    N(NOTE_G4,   220), N(NOTE_REST, 220),
    N(NOTE_A4,   110), N(NOTE_AS4,  110),

    N(NOTE_D5,   110), N(NOTE_CS5,  110),
    N(NOTE_D5,   110), N(NOTE_C5,   110),
    N(NOTE_D5,   110), N(NOTE_E5,   110),
    N(NOTE_D5,   110), N(NOTE_C5,   110),

    N(NOTE_B4,   220), N(NOTE_A4,   220),
    N(NOTE_G4,   220), N(NOTE_FS4,  220),

    N(NOTE_G4,   220), N(NOTE_E4,   220),
    N(NOTE_G4,   110), N(NOTE_A4,   110),
    N(NOTE_B4,   220), N(NOTE_G4,   220),

    N(NOTE_A4,   110), N(NOTE_AS4,  110),
    N(NOTE_D5,   110), N(NOTE_CS5,  110),
    N(NOTE_D5,   110), N(NOTE_C5,   110),

    N(NOTE_B4,   220), N(NOTE_A4,   220),
    N(NOTE_G4,   440), N(NOTE_REST, 220),
};

/* Fondo del juego: corto, grave y tranquilo para no cansar */
static const Tone mus_level[] = {
    N(D4,T_E), N(R,T_E), N(A4,T_E), N(R,T_E), N(F4,T_E), N(A4,T_E), N(R,T_Q),
    N(C4,T_E), N(R,T_E), N(G4,T_E), N(R,T_E), N(E4,T_E), N(G4,T_E), N(R,T_Q),
    N(B3b,T_E), N(R,T_E), N(F4,T_E), N(R,T_E), N(D4,T_E), N(F4,T_E), N(R,T_Q),
    N(A3,T_E), N(R,T_E), N(E4,T_E), N(C4s,T_E), N(A3,T_Q), N(R,T_Q),
};

/* Nivel completo: arpegio hacia arriba */
static const Tone mus_win[] = {
    N(D5,T_E), N(F5,T_E), N(A5,T_E), N(D6,T_Q), N(R,T_E),
    N(C6,T_E), N(A5,T_E), N(B5b,T_E), N(C6,T_E), N(D6,T_H),
};

/* Game over: bajada triste con barridos */
static const Tone mus_lose[] = {
    S(A4, F4, 300), N(R, 60), S(G4, E4, 300), N(R, 60),
    S(F4, C4s, 350), N(R, 80), S(E4, A3, 700),
};

/* ---------------------------------------------------------------------- */
/* Efectos                                                                 */
/* ---------------------------------------------------------------------- */
static const Tone sfx_move[]   = { N(1200, 30) };
static const Tone sfx_ok[]     = { N(880, 50), N(1320, 90) };
static const Tone sfx_launch[] = { S(300, 1500, 250) };
static const Tone sfx_wood[]   = { S(220, 120, 60) };
static const Tone sfx_stone[]  = { N(900, 35), N(R, 15), N(650, 35) };
static const Tone sfx_pig[]    = { S(600, 1800, 90), N(R, 20), N(1800, 40) };
static const Tone sfx_boom[]   = { N(150, 30), N(90, 30), N(180, 30), N(70, 30), S(120, 40, 250) };
static const Tone sfx_power[]  = { S(800, 1600, 80), S(1600, 800, 80) };

static const Tone *const sfx_tab[SFX_COUNT] = {
    sfx_move, sfx_ok, sfx_launch, sfx_wood, sfx_stone, sfx_pig, sfx_boom, sfx_power
};
static const uint8_t sfx_len[SFX_COUNT] = {
    LEN(sfx_move), LEN(sfx_ok), LEN(sfx_launch), LEN(sfx_wood),
    LEN(sfx_stone), LEN(sfx_pig), LEN(sfx_boom), LEN(sfx_power)
};

/* ---------------------------------------------------------------------- */
/* Reproductor                                                             */
/* ---------------------------------------------------------------------- */
typedef struct {
    const Tone *seq;
    uint16_t    len, idx;
    uint32_t    t0;            /* inicio de la nota actual */
    uint8_t     loop, active;
} Track;

static Track    music, sfx;
static SoundMusic cur_music = MUS_NONE;
static uint8_t  volume = 100, muted = 0;
static uint32_t last_freq = 0;

/* Pone el buzzer a una frecuencia (0 = silencio) */
static void hw_tone(uint32_t f)
{
    if (f == last_freq) return;
    last_freq = f;
    if (f == 0 || muted || volume == 0) {
        __HAL_TIM_SET_COMPARE(&htim3, BUZZER_CH, 0);
        return;
    }
    uint32_t arr = 1000000UL / f - 1;
    /* volumen: 100 % -> ciclo de 50 %; menos volumen -> pulso mas angosto */
    uint32_t ccr = (arr / 2) * volume / 100;
    if (ccr == 0) ccr = 1;
    __HAL_TIM_SET_AUTORELOAD(&htim3, arr);
    __HAL_TIM_SET_COMPARE(&htim3, BUZZER_CH, ccr);
}

static void track_start(Track *t, const Tone *seq, uint16_t len, uint8_t loop)
{
    t->seq = seq; t->len = len; t->idx = 0;
    t->loop = loop; t->active = (seq != 0 && len > 0);
    t->t0 = HAL_GetTick();
}

/* Avanza la pista y devuelve la frecuencia que debe sonar (0 = silencio) */
static uint32_t track_step(Track *t, uint32_t now, uint8_t legato)
{
    while (t->active) {
        const Tone *n = &t->seq[t->idx];
        uint32_t dt = now - t->t0;
        if (dt < n->ms) {
            if (n->f0 == 0) return 0;
            if (!legato && dt > (uint32_t)n->ms * 85 / 100) return 0;   /* separa notas */
            if (n->f1 == 0) return n->f0;
            return n->f0 + (int32_t)(n->f1 - n->f0) * (int32_t)dt / n->ms;  /* barrido */
        }
        t->t0 += n->ms;                     /* siguiente nota */
        if (++t->idx >= t->len) {
            if (t->loop) t->idx = 0;
            else         t->active = 0;
        }
    }
    return 0;
}

void sound_init(void)
{
    htim3.Instance->CR1 |= TIM_CR1_ARPE;   /* el cambio de periodo espera al fin del ciclo: sin chasquidos */
    __HAL_TIM_SET_COMPARE(&htim3, BUZZER_CH, 0);
    HAL_TIM_PWM_Start(&htim3, BUZZER_CH);
    music.active = sfx.active = 0;

#if SOUND_SELFTEST
    /* Prueba del buzzer: do - mi - sol, 0.3 s cada una (bloquea ~1 s al encender) */
    static const uint16_t test[3] = { 523, 659, 784 };
    for (int i = 0; i < 3; i++) {
        uint32_t arr = 1000000UL / test[i] - 1;
        __HAL_TIM_SET_AUTORELOAD(&htim3, arr);
        __HAL_TIM_SET_COMPARE(&htim3, BUZZER_CH, arr / 2);
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        HAL_Delay(300);
    }
    __HAL_TIM_SET_COMPARE(&htim3, BUZZER_CH, 0);
    last_freq = 0;
#endif
}

void sound_update(void)
{
    uint32_t now = HAL_GetTick();
    if (sfx.active) {
        uint32_t f = track_step(&sfx, now, 1);
        if (sfx.active) { hw_tone(f); return; }
        music.t0 = now;                     /* termino el efecto: la musica sigue donde iba */
    }
    hw_tone(track_step(&music, now, 0));
}

void sound_music(SoundMusic m)
{
    if (m == cur_music && (music.active || m == MUS_NONE)) return;
    cur_music = m;
    switch (m) {
    case MUS_TITLE: track_start(&music, mus_title, LEN(mus_title), 1); break;
    case MUS_LEVEL: track_start(&music, mus_level, LEN(mus_level), 1); break;
    case MUS_WIN:   track_start(&music, mus_win,   LEN(mus_win),   0); break;
    case MUS_LOSE:  track_start(&music, mus_lose,  LEN(mus_lose),  0); break;
    default:        music.active = 0; break;
    }
}

void sound_sfx(SoundSfx s)
{
    if (s >= SFX_COUNT) return;
    track_start(&sfx, sfx_tab[s], sfx_len[s], 0);
}

void sound_volume(uint8_t percent)
{
    volume = (percent > 100) ? 100 : percent;
    last_freq = 0xFFFFFFFF;                 /* fuerza a reaplicar */
}

void sound_mute(uint8_t on)
{
    muted = on;
    last_freq = 0xFFFFFFFF;
}

#endif /* SOUND_REMOTE */
