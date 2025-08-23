#ifndef BLE_PERIPHERAL_H
#define BLE_PERIPHERAL_H

#include <stdbool.h>

typedef void (*connection_status_cb_t)(bool connected);

int ble_peripheral_init(connection_status_cb_t conn_cb);
int ble_peripheral_start_advertising(void);
int ble_peripheral_stop_advertising(void);
bool ble_peripheral_is_connected(void);

#endif
