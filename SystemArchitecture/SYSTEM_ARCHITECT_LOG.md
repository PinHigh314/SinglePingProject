# System Architect Log

This log documents all major architectural decisions, module boundaries, interface contracts, and the general direction of the project. It serves as a persistent reference for both the system architect and developers, ensuring continuity in case of outages or context loss.

---

## 2025-08-02: Project Kickoff & Workflow Agreement

**Roles & Responsibilities:**
- **System Architect (AI):**
  - Define system structure and main modules.
  - Design and maintain interfaces (header files).
  - Make and document architectural decisions and trade-offs.
  - Guide data flow and inter-module communication.
  - Review code for architectural alignment, stability, and maintainability.
  - Manage build/flash process and incorporate hardware feedback.
- **Developer (User):**
  - Implement module logic and code according to defined interfaces.
  - Provide feedback from hardware testing.

**Workflow:**
- Architect defines structure, interfaces, and direction.
- Developer implements code and reports back.
- Architect reviews, builds, flashes, and updates log as needed.

**Initial Direction:**
- Modular, layered architecture for embedded system.
- Clear separation of concerns: HAL, drivers, communication, application logic.
- Interfaces defined in .h files; implementation in .c files.
- Data flow and inter-module communication to be explicitly documented.

---

## 2025-08-02: MIPE Device Prompt Review & Code Structure Assessment

### Prompt Requirements Summary
- Research-first, stepwise, and evidence-based development.
- Modular structure: main, led_control, button_control, ble_service, connection_manager, battery_monitor, utils.
- Each module with .c (implementation) and .h (interface) files.
- Dedicated directories: src/ (code), include/ (headers), boards/ (overlays), docs/ (documentation).
- Documentation and backup discipline.
- Strict adherence to Nordic best practices and verified APIs.

### Current Code Structure (mipe_device/)
- **src/**: Contains main.c, led_control.c, button_control.c, ble_service.c, connection_manager.c, battery_monitor.c, and subdirectories for ble/, measurement/, utils/.
- **include/**: Contains led_control.h, button_control.h, ble_service.h, connection_manager.h, battery_monitor.h.
- **boards/**: Present (contents not fully listed).
- **prj.conf, CMakeLists.txt**: Present.
- **docs/**: Present at project root (contents not fully listed).

#### Observations
- All major modules specified in the prompt are present as .c/.h pairs.
- Directory structure matches the prompt's recommended organization.
- Subdirectories (e.g., ble/, measurement/, utils/) suggest further modularization, which is positive for maintainability.
- Project is ready for stepwise, modular development as per the prompt.
- Documentation and backup processes need to be verified in practice (docs/ and backup discipline).

#### Recommendations
- Maintain strict separation between interface (.h) and implementation (.c) for all modules.
- Ensure all module interfaces are fully documented and only expose necessary functions/types.
- Use docs/ for research summaries, development logs, and test results as specified.
- Implement and document the backup process at each major milestone.
- Before coding, complete and document the research phase as required by the prompt.

#### Next Steps
- Complete research and document findings in docs/research_summary.md.
- Validate and, if needed, refine module interfaces in include/.
- Begin stepwise development with time-stamped backups and user validation at each step.

---

## 2025-08-02: Build System Configuration & Initial Build Attempt

### Build System Issues Resolved
- **CMakeLists.txt**: Added `target_include_directories(app PRIVATE include)` to properly include header files.
- **Missing Headers**: Copied `battery_monitor.h` from `src/utils/` to `include/` directory.
- **Configuration**: Removed problematic `CONFIG_BT_CONN=y` from `prj.conf` as it's auto-enabled.
- **Include Issues**: Removed non-existent `utils.h` include from `main.c`.

### Current Build Status
- **Headers Found**: Build system now correctly finds and includes project header files.
- **Main Application**: `main.c` compiles successfully with minor warning about `battery_monitor_update()` function.
- **Remaining Issues**: 
  - Picolibc locks compatibility issue (Zephyr/toolchain version mismatch)
  - Missing function declarations in header files (e.g., `battery_monitor_update()`)

### Architectural Observations
- Module structure is sound with proper separation of interface (.h) and implementation (.c).
- Build system configuration needed refinement to properly expose include directories.
- Some function declarations are missing from header files, indicating incomplete interface definitions.

### Recommendations
- Review and complete all header file function declarations.
- Address picolibc compatibility issue (may require Zephyr/toolchain version alignment).
- **Remaining Issues**: 
  - Picolibc locks compatibility issue (Zephyr/toolchain version mismatch)
  - Missing function declarations in header files (e.g., `battery_monitor_update()`)

### Architectural Observations
- Module structure is sound with proper separation of interface (.h) and implementation (.c).
- Build system configuration needed refinement to properly expose include directories.
- Some function declarations are missing from header files, indicating incomplete interface definitions.

### Recommendations
- Review and complete all header file function declarations.
- Address picolibc compatibility issue (may require Zephyr/toolchain version alignment).
- Implement missing functions or remove calls to undefined functions.
- Consider creating minimal stub implementations for missing functions to achieve successful build.

---

## 2025-08-02: Button Interaction Model Update

**Decision:**
- Transition from a simple, polling-based button check (`button_is_pressed()`) to a more robust, event-driven model.
- The `button_control` module will now be responsible for debouncing the raw button input and detecting a "press" event (a rising edge).

**Rationale:**
- **Reliability:** The previous polling model is susceptible to mechanical button bounce, which can register multiple presses for a single physical action. A debouncing state machine solves this.
- **Extensibility:** An event-based system (`button_was_pressed()`) is more versatile. It allows for implementing features like toggling, double-clicks, or long-presses without cluttering `main.c` with state-tracking logic.
- **Encapsulation:** The responsibility of debouncing is now properly encapsulated within the `button_control` module, leading to cleaner code in `main.c`.

**Interface Change:**
- **Removed:** `bool button_is_pressed(void)`
- **Added:**
  - `void button_control_update(void)`: Must be called periodically in the main loop to run the debouncing state machine.
  - `bool button_was_pressed(void)`: Returns `true` for one update cycle after a debounced press is detected.

**Impact:**
- `main.c` is simplified. It no longer needs to track the previous button state. It just calls `button_control_update()` and checks for the press event.
- The system is now ready for more complex user interactions.

---

## 2025-08-02: BLE Advertising and Connection Indication

**Decision:**
- Implement basic BLE advertising to make the MIPE device discoverable.
- Implement connection status callbacks to monitor connection state.
- Rework the `led_control` module to support patterns (e.g., blinking) and link it to the BLE state for visual feedback.

**Rationale:**
- **Core Functionality:** Advertising is the first and most critical step for any BLE peripheral.
- **User Feedback:** Providing immediate visual feedback on the device's state (advertising vs. connected) is crucial for usability and debugging. The LED is the primary tool for this.
- **Modular Interaction:** This change establishes the first link between modules: `ble_service` and `connection_manager` now drive the behavior of `led_control`. This demonstrates the intended modular architecture in action.

**Implementation Details:**
- `ble_service`: Initializes the Zephyr Bluetooth stack and starts advertising a connectable device name.
- `connection_manager`: Uses the `BT_CONN_CB_DEFINE` macro to register static callbacks for `connected` and `disconnected` events.
- `led_control`: Reworked to include a pattern-based state machine (`led_control_update`). It now exposes `led_set_pattern()` which is called by other modules.
- `main.c`: Simplified to remove direct button-to-LED logic. It now just calls the update functions for each module.

---

---

## 2025-08-02: Successful Build Achievement & Missing Function Resolution

**Problem Identified:**
- Build failing due to missing `battery_monitor_init()` and `battery_monitor_update()` functions
- Functions were called in `main.c` but not implemented in `battery_monitor.c`
- `battery_monitor.c` source file was not included in CMakeLists.txt

**Solutions Implemented:**
1. **Interface Completion**: Added missing `battery_monitor_update()` declaration to `battery_monitor.h`
2. **Implementation**: Created complete `battery_monitor.c` with all required functions as stubs
3. **Build System Fix**: Added `src/utils/battery_monitor.c` to CMakeLists.txt target_sources
4. **C Library Fix**: Corrected picolibc configuration from `CONFIG_PICOLIBC_USE_MODULE=n` to `CONFIG_PICOLIBC=n` and `CONFIG_NEWLIB_LIBC=y`

**Build Success Metrics:**
- **Status**: ✅ SUCCESSFUL BUILD
- **Files Compiled**: 91/91 (100%)
- **Memory Usage**: 
  - FLASH: 139,952 bytes (9.57% of 1428 KB available)
  - RAM: 27,268 bytes (14.16% of 188 KB available)
- **Output**: Generated `zephyr.elf` and `merged.hex` ready for flashing

**Current Status:**
- All application modules compile and link successfully
- Architecture is sound with proper modular separation
- Ready for hardware testing (pending board connection/flashing tool issues)

**Flashing Attempts & Issues:**
- **nrfutil**: Fails due to protobuf compatibility issue (Python library version conflict)
- **JLink via west**: Fails with generic error, likely due to device-specific configuration
- **Direct JLink**: Successfully connects, erases device, but fails on memory write ("Writing target memory failed")

**Hardware Connection Status:**
- ✅ Board detected via USB: `/dev/tty.usbmodem0010577046121` and `/dev/tty.usbmodem0010577046123`
- ✅ JLink connection successful: Cortex-M33 r1p0 detected
- ✅ Debug interface working: SWD connection established
- ✅ Power confirmed: VTref=3.300V
- ✅ Security: Secure debug enabled
- ✅ Erase operation: Successful

**Root Cause Analysis:**
The flashing issue appears to be related to:
1. **Device-specific flash programming**: nRF54L15 may require specific flash programming sequences
2. **Memory protection**: Device may have additional security/protection mechanisms
3. **Toolchain compatibility**: JLink device database may need nRF54L15-specific configuration

**Next Steps:**
- ✅ **nRF Connect Programmer**: Launched nRF Connect for Desktop as alternative flashing method
- **Continue Development**: Architecture is complete, ready for implementation
- **Priority Implementation Areas**:
  1. **Battery Monitor**: Implement actual ADC reading for battery voltage
  2. **LED Control**: Enhance pattern system for better visual feedback
  3. **Button Control**: Add debouncing and multi-press detection
  4. **BLE Service**: Implement custom GATT services for ping functionality
  5. **Connection Manager**: Add robust connection handling and reconnection logic

**Development Workflow Established:**
- Build system working perfectly (91/91 files compile successfully)
- Modular architecture allows independent module development
- Incremental testing possible once flashing is resolved
- All interfaces defined and ready for implementation

---

## 2025-08-02: Hardware Testing & Rollback Decision

### Hardware Testing Results
- **Flash Success**: User successfully flashed firmware using nRF Connect Programmer
- **Runtime Issue**: Firmware not working - no LED activity, no BLE advertising
- **Root Cause**: Complex BLE implementation may have caused system instability

### Architectural Decision: Rollback to Stable Base
**Rationale:**
- Hardware testing revealed that complex BLE implementation caused system failure
- Better to have a working foundation than non-functional advanced features
- Incremental development approach: start simple, add complexity gradually

**Rollback Actions Taken:**
1. **BLE Service**: Reverted to simple stub implementation
2. **Configuration**: Removed complex logging and BLE configuration options
3. **Main Loop**: Added simple LED test pattern to verify basic functionality
4. **Build Target**: Pristine rebuild to ensure clean state

**Current Status:**
- **Architecture**: Maintained - all interfaces and modules preserved
- **Functionality**: Simplified to basic LED control for hardware verification
- **Next Steps**: Verify LED blinking, then incrementally add features

**Lessons Learned:**
- Hardware testing is critical for embedded systems
- Complex features should be added incrementally after basic functionality is verified
- System stability takes precedence over feature completeness

---

## 2025-08-03: Re-implementing BLE Advertising and Connection Indication

**Decision:**
- Re-implement basic BLE advertising and connection status indication, building upon the stable, simplified base.
- Refactor the `ble_service` and `connection_manager` modules to use the Zephyr Logging API instead of `printk`.
- The `led_control` module will be enhanced again to support patterns for visual feedback (blinking for advertising, solid for connected).

**Rationale:**
- **Core Functionality:** With a stable base confirmed, implementing discoverability and connection feedback is the next logical step.
- **Best Practices:** Migrating from `printk` to the Zephyr Logging API is crucial for creating maintainable and debuggable firmware.
- **User Feedback:** Linking the BLE state to the LED provides essential, immediate feedback on the device's status, which is vital for both development and end-user experience.

**Implementation Details:**
- `ble_service`: Now uses `LOG_MODULE_REGISTER` and gets the device name from `prj.conf` (`CONFIG_BT_DEVICE_NAME`). It calls `led_set_pattern()` to signal its state.
- `connection_manager`: Implements `connected` and `disconnected` callbacks using `BT_CONN_CB_DEFINE`. These callbacks call `led_set_pattern()` to update the LED based on the connection status.
- `led_control`: The pattern-based state machine (`led_control_update`) is reintroduced to handle different LED states (off, on, blinking).
- `main.c`: The main loop is cleaned up to remove temporary button-to-LED logic, now simply calling the `update()` function for each module.

**Status:**
- The system now has a clean, modular implementation for advertising and connection status, with proper logging and clear visual indication.

---

## 2025-08-03: Adjusting LED Blink Rate for Pairing/Advertising State

**Decision:**
- Modify the LED blink interval for the `LED_PATTERN_ADVERTISING` state from 1000ms to 200ms.

**Rationale:**
- User feedback indicated that the previous 1-second blink interval was too slow to clearly signify the device's advertising/pairing state.
- A faster 200ms interval (200ms on, 200ms off) provides more immediate and noticeable visual feedback, which is standard for devices in a discoverable mode.
- This change improves the user experience and makes the device state easier to diagnose at a glance.

**Implementation Details:**
- The `period_ms` for `LED_PATTERN_ADVERTISING` in `led_control.c` was changed from `1000` to `200`.
- No other modules were affected, demonstrating the benefit of encapsulating the LED pattern logic.

**Status:**
- The LED now blinks at a 2.5Hz frequency (400ms cycle time) while advertising, as per user request.

---

## 2025-08-02: Multi-LED Architecture Implementation & Working Milestone

### Major Architectural Enhancement: Multi-LED System
**Developer Implementation:**
- **Scalable LED Management**: Implemented enum-based LED identification (`LED_ID_HEARTBEAT`, `LED_ID_PAIRING`)
- **Pattern-Based Control**: Added `LED_PATTERN_HEARTBEAT` (500ms) and `LED_PATTERN_ADVERTISING` (200ms)
- **Device Tree Integration**: Professional GPIO handling with `GPIO_DT_SPEC_GET(DT_ALIAS(led0/led1), gpios)`
- **State Machine Architecture**: Individual LED state tracking with efficient update loop

### System Integration Success
**LED Assignment Strategy:**
- **LED0 (Heartbeat)**: System alive indicator - continuous 500ms blink from startup
- **LED1 (Pairing)**: BLE status indicator - fast 200ms blink during advertising
- **Clean Module Boundaries**: Each subsystem controls its own LED without conflicts

### Build & Test Results
**Build Status:** ✅ 100% SUCCESS
- **Memory Efficiency**: FLASH: 166,084 bytes (11.36%), RAM: 31,396 bytes (16.31%)
- **Hardware Verification**: User confirmed MIPE device operating correctly
- **BLE Functionality**: Device advertising as "MIPE" with proper LED indication

### Milestone Backup Created
**Timestamp:** 2025-08-02 09:51:01
**Backup Location:** `mipe_device_backup_20250802_095101_working_multi_led_ble/`
**Files Preserved:** 32 source files (41,079 bytes total)
**Rationale:** Stable working baseline with multi-LED BLE system ready for next development phase

### Architecture Quality Assessment
**Excellent Implementation:**
- ✅ **Scalability**: Easy to add more LEDs and patterns
- ✅ **Maintainability**: Clear enums, proper state management
- ✅ **Efficiency**: Minimal memory footprint with maximum functionality
- ✅ **Professional Standards**: Device tree integration, proper logging, clean interfaces

**Next Development Phase Ready:**
- Foundation established for ping measurement functionality
- Connection management ready for enhancement
- Battery monitoring system in place
- Robust LED feedback system operational

---

Further entries will document:
- Major architectural changes
- Interface updates
- Trade-off decisions
- Data flow diagrams and rationale
- Notable review feedback
- Hardware testing results and rollback decisions
- Milestone backups and development phases
