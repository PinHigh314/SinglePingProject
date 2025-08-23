/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * GATT Discovery Implementation for Mipe Device
 * This file implements the GATT service discovery and subscription
 * for receiving RSSI data from Mipe device.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_DECLARE(ble_central, LOG_LEVEL_INF);

/* MIPE Service UUIDs */
static struct bt_uuid_128 mipe_service_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x987654321098)
);

static struct bt_uuid_128 mipe_rssi_char_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x87654322, 0x4321, 0x8765, 0x4321, 0x987654321098)
);

/* Discovery parameters */
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

/* External references */
extern uint8_t notify_cb(struct bt_conn *conn,
                         struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length);

/* Subscribe to RSSI notifications */
static void subscribe_to_rssi(struct bt_conn *conn, uint16_t handle)
{
    int err;

    subscribe_params.notify = notify_cb;
    subscribe_params.value = BT_GATT_CCC_NOTIFY;
    subscribe_params.value_handle = handle;
    subscribe_params.ccc_handle = 0;  /* Will be auto-discovered */
    /* Use default flags - no need to set explicitly */

    err = bt_gatt_subscribe(conn, &subscribe_params);
    if (err && err != -EALREADY) {
        LOG_ERR("Failed to subscribe to RSSI notifications (err %d)", err);
    } else {
        LOG_INF("Subscribed to MIPE RSSI notifications");
    }
}

/* Discovery callback */
uint8_t discover_func(struct bt_conn *conn,
                     const struct bt_gatt_attr *attr,
                     struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr) {
        LOG_INF("Discovery complete");
        memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    LOG_DBG("[ATTRIBUTE] handle %u", attr->handle);

    if (!bt_uuid_cmp(discover_params.uuid, &mipe_service_uuid.uuid)) {
        /* Found MIPE service, now discover RSSI characteristic */
        memset(&discover_params, 0, sizeof(discover_params));
        discover_params.uuid = &mipe_rssi_char_uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.end_handle = 0xffff;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        discover_params.func = discover_func;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_ERR("Failed to discover RSSI characteristic (err %d)", err);
        }
        return BT_GATT_ITER_STOP;
    } else if (!bt_uuid_cmp(discover_params.uuid, &mipe_rssi_char_uuid.uuid)) {
        /* Found RSSI characteristic */
        LOG_INF("Found MIPE RSSI characteristic at handle %u", attr->handle);
        
        /* Get the value handle (next handle after characteristic declaration) */
        uint16_t value_handle = bt_gatt_attr_value_handle(attr);
        
        /* Subscribe to notifications */
        subscribe_to_rssi(conn, value_handle);
        
        memset(&discover_params, 0, sizeof(discover_params));
        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_CONTINUE;
}

/* Start MIPE service discovery */
void discover_mipe_service(struct bt_conn *conn)
{
    int err;

    LOG_INF("Starting MIPE service discovery");

    memset(&discover_params, 0, sizeof(discover_params));
    discover_params.uuid = &mipe_service_uuid.uuid;
    discover_params.func = discover_func;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(conn, &discover_params);
    if (err) {
        LOG_ERR("Failed to start discovery (err %d)", err);
        return;
    }

    LOG_INF("GATT discovery started for MIPE services");
}
