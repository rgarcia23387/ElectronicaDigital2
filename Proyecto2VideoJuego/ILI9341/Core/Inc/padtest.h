/* padtest.h - pantalla de prueba del mando (enlace, joystick y botones) */
#ifndef INC_PADTEST_H_
#define INC_PADTEST_H_

#include "controller.h"

void padtest_full(void);                     /* dibuja la pantalla completa      */
void padtest_update(const PadState *p);      /* redibuja solo lo que cambio      */

#endif /* INC_PADTEST_H_ */
