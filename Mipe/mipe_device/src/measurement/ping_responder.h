#ifndef PING_RESPONDER_H
#define PING_RESPONDER_H

#include <stdint.h>

int ping_responder_init(void);
void ping_responder_process(void);
int ping_responder_handle_request(const uint8_t *data, uint16_t len);

#endif
