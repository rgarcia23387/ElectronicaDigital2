/*
 * Display.h
 *
 * Created: 9/07/2026 18:17:54
 * Author: Rodrigo García
 * Description: Libreria del Display, archivo .h
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <avr/io.h>

void display_init(void);
void display_mostrarDigito(uint8_t digito);
void display_apagar(void);


#endif /* DISPLAY_H_ */