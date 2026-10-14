/* musica_menu.h - tema del menu (notas proporcionadas por el estudiante).
 * Copiado de Core/Src/sound.c del proyecto del juego. Lo incluye synth.c. */
#ifndef INC_MUSICA_MENU_H_
#define INC_MUSICA_MENU_H_

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

#endif /* INC_MUSICA_MENU_H_ */
