#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdbool.h>

enum led_id {
	LED_ID_HEARTBEAT,
	LED_ID_PAIRING,
	LED_ID_CONNECTION,  /* Connection status LED */
	LED_ID_DATA,        /* Data transmission LED */
	LED_ID_ERROR,       /* Error/low battery LED */
	LED_ID_COUNT        /* Keep this last */
};

enum led_pattern {
	LED_PATTERN_OFF,
	LED_PATTERN_ON,
	LED_PATTERN_HEARTBEAT,
	LED_PATTERN_ADVERTISING,
	LED_PATTERN_CONNECTED,      /* Solid on when connected */
	LED_PATTERN_DATA_ACTIVE,    /* Blink when data is being transmitted */
	LED_PATTERN_ERROR,          /* Fast blink for error indication */
	LED_PATTERN_SLOW_BLINK,     /* Slow blink for listening mode */
};

void led_control_init(void);
void led_set_state(enum led_id id, bool on);
void led_set_pattern(enum led_id id, enum led_pattern pattern);
void led_control_update(void);

#endif // LED_CONTROL_H
