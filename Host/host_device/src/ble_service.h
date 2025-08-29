#ifndef BLE_SERVICE_H_
#define BLE_SERVICE_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

/* Custom Service UUIDs */
#define BT_UUID_CUSTOM_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_CUSTOM_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_CUSTOM_SERVICE_VAL)

/* Measurement Data Characteristic UUID */
#define BT_UUID_MEASUREMENT_DATA_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

#define BT_UUID_MEASUREMENT_DATA_CHAR \
    BT_UUID_DECLARE_128(BT_UUID_MEASUREMENT_DATA_CHAR_VAL)

/* Control Command Characteristic UUID */
#define BT_UUID_CONTROL_COMMAND_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

#define BT_UUID_CONTROL_COMMAND_CHAR \
    BT_UUID_DECLARE_128(BT_UUID_CONTROL_COMMAND_CHAR_VAL)

/* System Status Characteristic UUID */
#define BT_UUID_SYSTEM_STATUS_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)

#define BT_UUID_SYSTEM_STATUS_CHAR \
    BT_UUID_DECLARE_128(BT_UUID_SYSTEM_STATUS_CHAR_VAL)

/* Mipe Status Characteristic UUID */
#define BT_UUID_MIPE_STATUS_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef4)

#define BT_UUID_MIPE_STATUS_CHAR \
    BT_UUID_DECLARE_128(BT_UUID_MIPE_STATUS_CHAR_VAL)

/* Log Data Characteristic UUID */
#define BT_UUID_LOG_DATA_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef5)

#define BT_UUID_LOG_DATA_CHAR \
    BT_UUID_DECLARE_128(BT_UUID_LOG_DATA_CHAR_VAL)

/**
 * @brief Initialize the BLE custom service.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ble_service_init(void);

/**
 * @brief Send measurement data to the connected client.
 *
 * @param data Pointer to the data to send.
 * @param len Length of the data.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ble_service_send_measurement_data(const uint8_t *data, uint16_t len);

/**
 * @brief Set the application connection object.
 *
 * @param conn Connection object.
 */
void ble_service_set_app_conn(struct bt_conn *conn);

#endif /* BLE_SERVICE_H_ */
