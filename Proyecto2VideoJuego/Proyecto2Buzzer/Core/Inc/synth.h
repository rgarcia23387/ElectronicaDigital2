/* synth.h - sintetizador de 8 bits (TIM6 a 22 kHz, DAC en PA4) */
#ifndef INC_SYNTH_H_
#define INC_SYNTH_H_

#include <stdint.h>

#define SYNTH_FS  22047u        /* 84 MHz / (TIM6 periodo 3809 + 1) */

/* 1 = tambien suena por el buzzer pasivo en PA6 (TIM3_CH1) */
#define SYNTH_BUZZER 1

void synth_init(void);          /* arranca DAC y TIM6 */
void synth_sample_isr(void);    /* llamar desde la interrupcion de TIM6 */
void synth_update(void);        /* llamar en el while(1): avanza notas cada 1 ms */
void synth_command(uint8_t cmd);/* procesa un comando de audio_cmd.h */

#endif /* INC_SYNTH_H_ */
