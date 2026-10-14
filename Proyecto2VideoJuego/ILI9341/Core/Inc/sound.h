/* sound.h - sonido de 8 bits sin bloquear el juego */
#ifndef INC_SOUND_H_
#define INC_SOUND_H_

#include <stdint.h>

/* 1 = el sonido sale en la Nucleo de audio (UART5), 0 = buzzer local en PC6 */
#define SOUND_REMOTE 1

/* 1 = al encender toca 3 notas (do-mi-sol) para probar el buzzer */
#define SOUND_SELFTEST 1

typedef enum {
    MUS_NONE = 0,
    MUS_TITLE,          /* tema del menu (se repite)            */
    MUS_LEVEL,          /* fondo durante el juego (se repite)   */
    MUS_WIN,            /* nivel completo (una vez)             */
    MUS_LOSE,           /* game over (una vez)                  */
} SoundMusic;

typedef enum {
    SFX_MOVE = 0,       /* mover en el menu        */
    SFX_OK,             /* aceptar                 */
    SFX_LAUNCH,         /* lanzar el pajaro        */
    SFX_WOOD,           /* golpe en madera         */
    SFX_STONE,          /* golpe en piedra         */
    SFX_PIG,            /* cerdo eliminado         */
    SFX_BOOM,           /* explosion               */
    SFX_POWER,          /* poder del pajaro        */
    SFX_COUNT
} SoundSfx;

void sound_init(void);                     /* arranca el PWM (llamar despues de MX_TIM3_Init) */
void sound_update(void);                   /* llamar en cada vuelta del while(1)              */
void sound_music(SoundMusic m);            /* cambia la musica (no reinicia si ya suena)      */
void sound_sfx(SoundSfx s);                /* efecto: interrumpe la musica un momento         */
void sound_volume(uint8_t percent);        /* 0..100 (100 = maximo)                           */
void sound_mute(uint8_t on);

#endif /* INC_SOUND_H_ */
