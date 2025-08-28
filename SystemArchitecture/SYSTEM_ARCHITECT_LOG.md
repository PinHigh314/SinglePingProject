# System Architect Log & Decision Record

This log documents all major architectural decisions, module boundaries, interface contracts, and the general direction of the project. It serves as a persistent reference for both the system architect and developers, ensuring continuity in case of outages or context loss.

---

## Table of Contents

- [Template for New Architectural Decisions](#template-for-new-architectural-decisions)
- [Log Maintenance Process](#2025-08-21-log-maintenance-process)
- [Project Kickoff & Workflow Agreement](#2025-08-02-project-kickoff--workflow-agreement)
- [MIPE Device Prompt Review & Code Structure Assessment](#2025-08-02-mipe-device-prompt-review--code-structure-assessment)
- [Build System Configuration & Initial Build Attempt](#2025-08-02-build-system-configuration--initial-build-attempt)
- [Button Interaction Model Update](#2025-08-02-button-interaction-model-update)
- [BLE Advertising and Connection Indication](#2025-08-02-ble-advertising-and-connection-indication)
- [Successful Build Achievement & Missing Function Resolution](#2025-08-02-successful-build-achievement--missing-function-resolution)
- [Hardware Testing & Rollback Decision](#2025-08-02-hardware-testing--rollback-decision)
- [Re-implementing BLE Advertising and Connection Indication](#2025-08-03-re-implementing-ble-advertising-and-connection-indication)
- [Adjusting LED Blink Rate for Pairing/Advertising State](#2025-08-03-adjusting-led-blink-rate-for-pairingadvertising-state)
- [Multi-LED Architecture Implementation & Working Milestone](#2025-08-02-multi-led-architecture-implementation--working-milestone)
- [Device Name Change to "MIPE" with Preserved BLE Configuration](#2025-08-26-device-name-change-to-mipe-with-preserved-ble-configuration)
- [Zephyr Interrupt Handler Limitations and Work Queue Pattern](#2025-08-27-zephyr-interrupt-handler-limitations-and-work-queue-pattern)
- [UART Serial Debugging and Monitoring Infrastructure](#2025-08-27-uart-serial-debugging-and-monitoring-infrastructure)
- [GPIO and Zephyr API Usage – nRF54L15DK](#2025-08-27-gpio-and-zephyr-api-usage--nrf54l15dk)
- [Future Entries](#future-entries)

---

## Template for New Architectural Decisions

*(Copy and use this template for all new entries)*

### YYYY-MM-DD: [Title of Decision]

**Decision:**  
A concise statement of the architectural change that was made.

**Rationale:**  
The reasons and trade-offs that led to this decision. What problem was being solved? What alternatives were considered?

**Impact:**  
How this decision affects the codebase, other modules, or future development.

---

## 2025-08-21: Log Maintenance Process

**Decision:**  
To ensure this log is kept up-to-date, the System Architect (AI) will update it at specific, defined trigger points.

**Rationale:**  
An explicit process ensures all major decisions are captured consistently, making the log a trustworthy source of truth.

**Process Triggers:**
1. **Explicit Confirmation:** After a discussion results in a clear architectural decision, the AI will state its intention to update the log.
2. **End-of-Milestone Review:** At the conclusion of each Test Milestone Test (TMT), the AI will review the log for any new lessons learned or decisions made during that phase.
3. **User Request:** The user can request an update to the log at any time.

**Impact:**  
This formalizes the documentation process, making it a reliable and integral part of the development workflow.

---

## 2025-08-02: Project Kickoff & Workflow Agreement

**Roles & Responsibilities:**
- **System Architect (AI):**  
  Defines system structure, interfaces, documents decisions, guides data flow, reviews code, manages build/flash, and incorporates hardware feedback.
- **Developer (User):**  
  Implements module logic and code according to defined interfaces, provides feedback from hardware testing.

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

**Prompt Requirements Summary:**  
Research-first, stepwise, and evidence-based development. Modular structure with dedicated directories and strict documentation/backup discipline.

**Current Code Structure (mipe_device/):**
- **src/**: Contains all major modules and subdirectories for further modularization.
- **include/**: Contains header files for all modules.
- **boards/**, **docs/**: Present for overlays and documentation.
- **prj.conf, CMakeLists.txt**: Present.

**Observations & Recommendations:**
- All major modules specified in the prompt are present as .c/.h pairs.
- Directory structure matches the prompt's recommended organization.
- Maintain strict separation between interface (.h) and implementation (.c).
- Use docs/ for research summaries, development logs, and test results.
- Implement and document the backup process at each major milestone.

---

## 2025-08-02: Build System Configuration & Initial Build Attempt

**Build System Issues Resolved:**
- Properly included header files, fixed missing headers, and resolved configuration issues.
- Build system now correctly finds and includes project header files.
- Remaining issues: Picolibc locks compatibility, missing function declarations.

**Recommendations:**
- Review and complete all header file function declarations.
- Address toolchain compatibility issues.
- Implement missing functions or create stubs for successful build.

---

## 2025-08-02: Button Interaction Model Update

**Decision:**  
Transitioned from polling-based button checks to an event-driven, debounced model.

**Rationale:**  
Improved reliability and extensibility, encapsulated debouncing logic within the button_control module.

**Impact:**  
Simplified main.c, enabled more complex user interactions.

---

## 2025-08-02: BLE Advertising and Connection Indication

**Decision:**  
Implemented basic BLE advertising and connection status callbacks, reworked led_control for pattern-based feedback.

**Rationale:**  
Core BLE functionality and user feedback via LEDs.

**Impact:**  
Established modular interaction between BLE and LED modules.

---

## 2025-08-02: Successful Build Achievement & Missing Function Resolution

**Problem Identified & Solutions:**  
Resolved missing function implementations and build system issues. Achieved successful build and ready for hardware testing.

**Flashing Attempts & Issues:**  
Documented hardware connection status and root cause analysis for flashing failures.

**Next Steps:**  
Continue development, implement priority modules, and maintain modular workflow.

---

## 2025-08-02: Hardware Testing & Rollback Decision

**Testing Results & Rollback:**  
Hardware testing revealed instability with complex BLE features. Rolled back to a stable base and simplified functionality for verification.

**Lessons Learned:**  
Incremental development and hardware testing are critical.

---

## 2025-08-03: Re-implementing BLE Advertising and Connection Indication

**Decision:**  
Re-implemented BLE advertising and connection indication, migrated to Zephyr Logging API, and enhanced LED feedback.

**Status:**  
System now has clean, modular implementation for advertising and connection status.

---

## 2025-08-03: Adjusting LED Blink Rate for Pairing/Advertising State

**Decision:**  
Changed LED blink interval for advertising state from 1000ms to 200ms for improved user feedback.

**Status:**  
LED now blinks at a more noticeable rate during advertising.

---

## 2025-08-02: Multi-LED Architecture Implementation & Working Milestone

**Enhancement:**  
Implemented scalable, pattern-based multi-LED system with device tree integration and state machine architecture.

**Assessment:**  
System is efficient, maintainable, and ready for next development phase.

---

## 2025-08-26: Device Name Change to "MIPE" with Preserved BLE Configuration

**Decision & Changes:**  
Successfully changed device name and preserved critical BLE configuration for RSSI streaming.

**Verification Status:**  
System ready for testing with new device name.

---

## 2025-08-27: Zephyr Interrupt Handler Limitations and Work Queue Pattern

**Decision:**  
Moved LED timing logic out of interrupt handlers into work queues to comply with Zephyr RTOS best practices.

**Lessons Learned:**  
Interrupt handlers should be minimal; timing operations must occur in thread context.

---

## 2025-08-27: UART Serial Debugging and Monitoring Infrastructure

**Decision:**  
Established robust UART serial debugging infrastructure for real-time monitoring and troubleshooting.

**Benefits:**  
Immediate feedback, improved debugging, and verification of system behavior.

---

## 2025-08-27: GPIO and Zephyr API Usage – nRF54L15DK

**Decision:**  
Transitioned from direct, generic GPIO manipulation to Zephyr’s device tree-driven API for all LED and button interactions on nRF54L15DK.

**Rationale:**  
- Zephyr’s device tree abstraction ensures hardware portability, maintainability, and clarity.
- Using `gpio_dt_spec` and device tree aliases (`DT_ALIAS`) allows code to adapt to board changes without rewriting logic.
- Zephyr’s API provides safe, race-free access to GPIO, supports interrupts, and integrates with RTOS features (work queues, callbacks).

**Implementation Details:**  
- **LEDs and Buttons:**  
  - Defined using device tree aliases (`DT_ALIAS(led0)`, `DT_ALIAS(sw1)`, etc.).
  - Accessed via `GPIO_DT_SPEC_GET` for type-safe pin/port configuration.
  - All initialization and control use Zephyr’s `gpio_pin_configure_dt`, `gpio_pin_set_dt`, and interrupt configuration functions.
- **Interrupt Handling:**  
  - Button interrupts are configured with `GPIO_INT_EDGE_FALLING` for event-driven input.
  - Handlers are minimal; deferred actions (e.g., LED flashing) are executed via Zephyr work queues (`k_work`), ensuring compliance with RTOS constraints.
- **Work Queues:**  
  - All time-dependent or blocking operations (like LED flashing) are moved out of interrupt context and into work queue handlers.
  - This pattern avoids issues with sleeping or delays in ISRs and supports future scalability (e.g., multi-threaded event handling).
- **Boot Sequence:**  
  - Uses `all_leds_on()` and `all_leds_off()` for visual feedback, leveraging device tree-driven LED control.
- **BLE Integration:**  
  - Button events can trigger BLE operations (scanning, connecting) using Zephyr’s Bluetooth API, with status feedback via LEDs.

**Architectural Value & Best Practices:**  
- **Device Tree First:**  
  Always define hardware resources in the device tree and use Zephyr’s macros for access. This decouples code from board specifics and supports easy migration.
- **Minimal ISR Logic:**  
  Keep interrupt handlers short; use work queues for anything that may block, sleep, or require significant processing.
- **Explicit Initialization:**  
  Check device readiness (`gpio_is_ready_dt`) before configuring pins. Log and handle failures gracefully.
- **Consistent Logging:**  
  Use Zephyr’s logging API for all status, error, and debug output. This aids troubleshooting and system monitoring.
- **Modular Structure:**  
  Separate hardware abstraction (LED/button control) from application logic (BLE, state machines) for maintainability.
- **Documentation:**  
  Record all architectural decisions, especially those affecting hardware abstraction, concurrency, and event handling.

**Known Pitfalls & Lessons Learned:**  
- Sleeping or blocking in interrupt context leads to unreliable behavior; always use work queues for deferred actions.
- Direct pin manipulation without device tree abstraction risks board incompatibility and maintenance headaches.
- Always verify pin and port readiness before use; hardware initialization failures should be logged and handled.

**Impact:**  
- Codebase is now portable, maintainable, and robust against hardware changes.
- Future developers and agents can extend or migrate the project with minimal risk of repeating past mistakes.
- System is compliant with Zephyr RTOS best practices, supporting reliable operation and easy debugging.

**References:**  
- [Zephyr GPIO API](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- [Zephyr Device Tree Guide](https://docs.zephyrproject.org/latest/guides/dts/index.html)
- [Zephyr Work Queue Pattern](https://docs.zephyrproject.org/latest/kernel/workqueue/index.html)

---

*This entry supersedes any previous logs referencing direct GPIO manipulation. All future code and architectural decisions should follow Zephyr’s device tree and API conventions for hardware abstraction.*

---

## Future Entries

Further entries will document:
- Major architectural changes
- Interface updates
- Trade-off decisions
- Data flow diagrams and rationale
- Notable review feedback
- Hardware testing results and rollback decisions
- Milestone backups and development phases

---

**Tip:**  
Consider adding a "Known Pitfalls" section at the end to help future agents avoid repeat mistakes.
