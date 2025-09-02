/**
 * SinglePing Mipe Device - Power-Optimized BLE Peripheral
 * 
 * SYSTEM ARCHITECTURE:
 * MotoApp (Android) ←→ BLE ←→ Host (NRF54L15DK) ←→ BLE ←→ Mipe Device
 * 
 * MIPE DEVICE ROLE:
 * - Power-optimized BLE peripheral for distance measurement
 * - Minimal power consumption (TARGET: 30+ days battery life)
 * - Host-controlled measurement sessions ONLY
 * - NEVER initiates communication, only responds
 * - Stable signal characteristics for accurate RSSI
 * - Consistent transmission power and timing
 * 
 * POWER-FIRST DESIGN PRINCIPLES:
 * 1. Mipe transmits MINIMALLY - only when prompted and absolutely necessary
 * 2. Every transmission costs battery life - must be justified
 * 3. Sleep mode is the default state
 * 4. Wake only for measurement sessions
 * 5. Single ping response mechanism (no repeated transmissions)
 * 
 * CONNECTION RECOVERY PROTOCOL:
 * - Upon connection loss: Enter "Listening Mode"
 * - Listening Mode: Low-power advertising (1000ms intervals)
 * - Listen for Host reconnection ping
 * - Single response to reconnection ping (no repeats)
 * - Timeout: 5 minutes in listening mode, then deep sleep
 * 
 * CONNECTION STATES:
 * 1. ADVERTISING: Initial pairing mode, high power advertising
 * 2. LISTENING: Post-connection loss, low power advertising
 * 3. CONNECTED: Active connection with Host
 * 4. MEASURING: Responding to measurement session
 * 5. DEEP_SLEEP: Power conservation mode, no advertising
 * 
 * PROTOCOL SPECIFICATIONS:
 * - Packet sizes minimized (2-7 bytes typical)
 * - No acknowledgments unless critical
 * - Immediate sleep after session termination
 * - Fixed RF parameters for measurement consistency
 * 
 * TMT PHASES:
 * - TMT3: Basic Mipe-Host integration
 * - TMT4: Real RSSI measurements
 * - TMT5: RF optimization for accuracy
 * - TMT6: Power optimization for 30+ day battery life
 * 
 * TARGET SPECIFICATIONS:
 * - Distance accuracy: 10%
 * - Battery life: 30+ days
 * - Response time: <100ms
 * - Power consumption in listening mode: <10μA average
 */

#include "led_control.h"
#include "button_control.h"
#include "ble_service.h"
#include "connection_manager.h"
#include "battery_monitor.h"
#include <zephyr/kernel.h>

/**
 * Main entry point for Mipe device - POWER OPTIMIZED
 * Minimal initialization and power-conscious main loop
 */
int main(void) {
    /* POWER OPTIMIZATION: Minimal initialization sequence */
    
    /* Initialize button control for SW3 battery reading
     * SW0 for other functions, SW3 specifically for battery
     */
    button_control_init();
    
    /* Initialize battery monitoring for advertising data
     * Battery voltage will be included in every advertising packet
     */
    battery_monitor_init();
    
    /* Initialize BLE service and start advertising immediately
     * This is the primary function - advertising for RSSI measurement
     */
    ble_service_init();
    
    /* Initialize connection manager for Host communication
     * Handles connection states and recovery protocol
     */
    connection_manager_init();
    
    /* POWER OPTIMIZATION: No LED patterns at startup
     * LEDs consume power - only use when absolutely necessary
     */
    
    printk("========================================\n");
    printk("MIPE DEVICE STARTED\n");
    printk("  BLE advertising: ACTIVE with battery data\n");
    printk("  Battery monitoring: INITIALIZED\n");
    printk("  SW3 button: Manual battery read\n");
    printk("========================================\n");

    /* Main control loop - POWER OPTIMIZED
     * Minimal processing, maximum sleep time
     */
    while (1) {
        /* Check buttons - primary user interface
         * SW3 triggers battery read, SW0 for other functions
         */
        button_control_update();
        
        /* Check if SW3 was pressed for battery reading */
        if (button_sw3_was_pressed()) {
            /* Read battery voltage once and log it */
            battery_monitor_read_once();
        }
        
        /* Process BLE events - essential for operation
         * Handles advertising and Host communication
         */
        ble_service_update();
        
        /* Manage connection states - essential for recovery
         * Handles connection loss and recovery protocol
         */
        connection_manager_update();
        
        /* POWER OPTIMIZATION: Removed unnecessary updates
         * - No LED updates (saves power)
         * - No periodic battery monitoring (only on-demand)
         */

        /* Sleep between updates to conserve power
         * Increased from 10ms to 100ms for better power savings
         * Still responsive enough for button presses and BLE events
         */
        k_msleep(100);
    }

    return 0;
}
