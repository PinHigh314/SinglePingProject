/**
 * Mipe Device Connection Manager - P003 Implementation
 * 
 * Handles BLE connection state management for Mipe device
 * Implements power-optimized connection handling from Host
 * 
 * Connection States:
 * - IDLE: No connection, power-optimized advertising
 * - ADVERTISING: Active advertising for Host discovery
 * - CONNECTED: Active connection with Host device
 * - LISTENING: Post-disconnection recovery mode
 */

#include "connection_manager.h"
#include "led_control.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

LOG_MODULE_REGISTER(connection_manager, LOG_LEVEL_INF);

/* Connection state machine */
typedef enum {
    CONN_STATE_IDLE,
    CONN_STATE_ADVERTISING,
    CONN_STATE_CONNECTED,
    CONN_STATE_LISTENING,
} conn_state_t;

/* Connection manager context */
static struct {
    conn_state_t state;
    struct bt_conn *conn;
    uint32_t disconnect_time;
    bool auto_reconnect;
    uint8_t reconnect_attempts;
} conn_ctx = {
    .state = CONN_STATE_IDLE,
    .conn = NULL,
    .disconnect_time = 0,
    .auto_reconnect = true,
    .reconnect_attempts = 0,
};

/* Connection parameters optimized for power */
static struct bt_le_conn_param conn_params = {
    .interval_min = BT_GAP_INIT_CONN_INT_MIN,  /* 30ms min */
    .interval_max = BT_GAP_INIT_CONN_INT_MAX,  /* 50ms max */
    .latency = 4,                              /* Allow 4 connection events to be skipped */
    .timeout = 400,                             /* 4 seconds supervision timeout */
};

/* Forward declarations */
static void update_connection_state(conn_state_t new_state);
static void handle_listening_mode(void);

/**
 * Connection established callback
 * Called when Host successfully connects to Mipe
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
    if (err) {
        LOG_ERR("Connection failed to %s (err 0x%02x)", addr, err);
        update_connection_state(CONN_STATE_ADVERTISING);
        return;
    }
    
    LOG_INF("Connected to Host: %s", addr);
    
    /* Store connection reference */
    if (conn_ctx.conn) {
        bt_conn_unref(conn_ctx.conn);
    }
    conn_ctx.conn = bt_conn_ref(conn);
    conn_ctx.reconnect_attempts = 0;
    
    /* Update connection parameters for power optimization */
    int ret = bt_conn_le_param_update(conn, &conn_params);
    if (ret) {
        LOG_WRN("Failed to request connection parameter update (err %d)", ret);
    } else {
        LOG_INF("Connection parameters update requested");
    }
    
    /* Update state */
    update_connection_state(CONN_STATE_CONNECTED);
}

/**
 * Connection lost callback
 * Triggers listening mode for reconnection
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected from %s (reason 0x%02x)", addr, reason);
    
    /* Clean up connection reference */
    if (conn_ctx.conn) {
        bt_conn_unref(conn_ctx.conn);
        conn_ctx.conn = NULL;
    }
    
    /* Record disconnect time for timeout handling */
    conn_ctx.disconnect_time = k_uptime_get();
    
    /* Enter listening mode for reconnection */
    if (conn_ctx.auto_reconnect) {
        LOG_INF("Entering listening mode for reconnection");
        update_connection_state(CONN_STATE_LISTENING);
    } else {
        update_connection_state(CONN_STATE_IDLE);
    }
}

/**
 * Connection parameter update callback
 * Logs parameter changes for monitoring
 */
static void conn_param_updated(struct bt_conn *conn, uint16_t interval,
                               uint16_t latency, uint16_t timeout)
{
    LOG_INF("Connection parameters updated: interval=%d, latency=%d, timeout=%d",
            interval, latency, timeout);
}

/**
 * Connection parameter update request callback
 * Accepts Host's parameter update requests
 */
static bool conn_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
    LOG_INF("Connection parameter update requested by Host");
    LOG_INF("Requested: interval=[%d,%d], latency=%d, timeout=%d",
            param->interval_min, param->interval_max,
            param->latency, param->timeout);
    
    /* Accept Host's parameters for optimal coordination */
    return true;
}

/* Connection callbacks structure */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .le_param_updated = conn_param_updated,
    .le_param_req = conn_param_req,
};

/**
 * Update connection state machine
 */
static void update_connection_state(conn_state_t new_state)
{
    if (conn_ctx.state == new_state) {
        return;
    }
    
    LOG_INF("Connection state: %d -> %d", conn_ctx.state, new_state);
    conn_ctx.state = new_state;
    
    switch (new_state) {
    case CONN_STATE_IDLE:
        /* Stop any advertising */
        bt_le_adv_stop();
        break;
        
    case CONN_STATE_ADVERTISING:
        /* Handled by ble_service */
        break;
        
    case CONN_STATE_CONNECTED:
        /* Connection established */
        break;
        
    case CONN_STATE_LISTENING:
        /* Start low-power advertising for reconnection */
        handle_listening_mode();
        break;
    }
}

/**
 * Handle listening mode for power-optimized reconnection
 */
static void handle_listening_mode(void)
{
    /* Advertising parameters for listening mode - slower for power saving */
    static const struct bt_le_adv_param listening_adv_param = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_NAME,
        .interval_min = BT_GAP_ADV_SLOW_INT_MIN,  /* 1000ms min */
        .interval_max = BT_GAP_ADV_SLOW_INT_MAX,  /* 1200ms max */
        .peer = NULL,
    };
    
    /* Create advertising data */
    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    };
    
    int err = bt_le_adv_start(&listening_adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Failed to start listening mode advertising (err %d)", err);
        update_connection_state(CONN_STATE_IDLE);
    } else {
        LOG_INF("Listening mode advertising started (low power)");
        conn_ctx.reconnect_attempts++;
    }
}

/**
 * Initialize connection manager
 */
void connection_manager_init(void)
{
    LOG_INF("Initializing connection manager for P003");
    
    /* Register connection callbacks */
    bt_conn_cb_register(&conn_callbacks);
    
    /* Set initial state */
    conn_ctx.state = CONN_STATE_ADVERTISING;
    conn_ctx.auto_reconnect = true;
    
    LOG_INF("Connection manager initialized - ready for Host connections");
}

/**
 * Update connection manager
 * Handles timeouts and state transitions
 */
void connection_manager_update(void)
{
    static uint32_t last_check = 0;
    uint32_t now = k_uptime_get();
    
    /* Check every second */
    if (now - last_check < 1000) {
        return;
    }
    last_check = now;
    
    switch (conn_ctx.state) {
    case CONN_STATE_LISTENING:
        /* Check for listening mode timeout (5 minutes) */
        if (now - conn_ctx.disconnect_time > (5 * 60 * 1000)) {
            LOG_INF("Listening mode timeout - entering idle state");
            update_connection_state(CONN_STATE_IDLE);
        }
        break;
        
    case CONN_STATE_IDLE:
        /* In idle state, check if we should restart advertising */
        if (conn_ctx.auto_reconnect && conn_ctx.reconnect_attempts < 3) {
            LOG_INF("Retrying advertising (attempt %d/3)", conn_ctx.reconnect_attempts + 1);
            update_connection_state(CONN_STATE_ADVERTISING);
        }
        break;
        
    default:
        break;
    }
}

/**
 * Get current connection state
 */
bool connection_manager_is_connected(void)
{
    return (conn_ctx.state == CONN_STATE_CONNECTED) && (conn_ctx.conn != NULL);
}

/**
 * Enable/disable auto-reconnect feature
 */
void connection_manager_set_auto_reconnect(bool enable)
{
    conn_ctx.auto_reconnect = enable;
    LOG_INF("Auto-reconnect %s", enable ? "enabled" : "disabled");
}

/**
 * Request connection parameter update
 * Used to optimize power consumption during different operation modes
 */
int connection_manager_update_params(uint16_t interval_min, uint16_t interval_max,
                                     uint16_t latency, uint16_t timeout)
{
    if (!conn_ctx.conn) {
        return -ENOTCONN;
    }
    
    struct bt_le_conn_param params = {
        .interval_min = interval_min,
        .interval_max = interval_max,
        .latency = latency,
        .timeout = timeout,
    };
    
    int ret = bt_conn_le_param_update(conn_ctx.conn, &params);
    if (ret) {
        LOG_ERR("Failed to update connection parameters (err %d)", ret);
    } else {
        LOG_INF("Connection parameter update requested");
    }
    
    return ret;
}
