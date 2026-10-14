/* audio_cmd.h - comandos de 1 byte que el juego manda a la Nucleo de audio */
#ifndef INC_AUDIO_CMD_H_
#define INC_AUDIO_CMD_H_

#define CMD_SFX_BASE   0x01
#define CMD_MUS_BASE   0x10
#define CMD_VOL_BASE   0x20
#define CMD_MUTE_ON    0x30
#define CMD_MUTE_OFF   0x31

/* Musicas y efectos (mismo orden que en sound.h del juego) */
#ifndef INC_SOUND_H_
enum { MUS_NONE = 0, MUS_TITLE, MUS_LEVEL, MUS_WIN, MUS_LOSE, MUS_COUNT };
enum { SFX_MOVE = 0, SFX_OK, SFX_LAUNCH, SFX_WOOD, SFX_STONE, SFX_PIG, SFX_BOOM, SFX_POWER, SFX_COUNT };
#endif

#endif /* INC_AUDIO_CMD_H_ */
