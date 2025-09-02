#ifndef BUTTON_CONTROL_H
#define BUTTON_CONTROL_H

#include <stdbool.h>

void button_control_init(void);
void button_control_update(void);
bool button_was_pressed(void);
bool button_sw3_was_pressed(void);  /* SW3 for battery read */

#endif // BUTTON_CONTROL_H
