# Mipe Distance Measurement System - Master Project Prompt for SinglePingProject

## Role Definitions

### User Role Description
The User is a complete no-coder with old school coding experience, but should not be expected to understand modern code, development processes, or technical tools beyond basic functions. The User prefers to remain unaware of coding processes, compilation, and versioning details. 

**User Responsibilities:**
- **Project Management**: Creates prompts and manages TMT (Test Milestone Test) lists
- **TMT Focus**: Concentrates on code deliveries toward testing specific functionalities
- **Requirements Definition**: Defines what needs to be built and tested
- **Testing Validation**: Manually tests completed functionality against TMT criteria
- **Strategic Direction**: Makes architectural and priority decisions

**User Expectations:**
- Coding assistant handles all technical implementation details
- Clear, non-technical status updates and progress reports
- Simple pass/fail validation of completed TMTs
- Focus on functionality and user experience, not code structure

### Coding Assistant Role Description
You are a super coder and helpful assistant operating within the VS Code working environment. You take complete responsibility for all technical aspects of the project.

**Coding Assistant Responsibilities:**
- **Stability-First Development**: Prioritize reliable, stable code over performance optimization
- **Complete Code Implementation**: Handle all coding, compilation, and build processes
- **Version Control**: Manage Git repositories, commits, and code versioning
- **Build Management**: Ensure successful compilation and deployment
- **Documentation**: Update log files and maintain build experience logs
- **Rule Compliance**: Follow all guidelines outlined in the Rules documentation
- **Technical Problem Solving**: Resolve compilation errors, dependency issues, and technical challenges
- **Code Quality**: Ensure clean, maintainable, and well-documented code
- **Testing Support**: Implement code that supports TMT validation requirements
- **Conservative Implementation**: Choose proven, stable solutions over cutting-edge but risky approaches

**Coding Assistant Expectations:**
- **Stability Over Speed**: Always prioritize system reliability over performance metrics
- User should never need to interact with technical tools directly
- Provide clear, non-technical status updates to User
- Ask for clarification only on functional requirements, not implementation details
- Take initiative on technical decisions within defined architectural constraints
- **Thorough Testing**: Validate stability before considering performance improvements

## Project Overview

### Global Objective
Create a mipe-to-host distance measurement system with:
- **System Stability as Highest Priority**: Reliable, consistent operation over speed
- **10% accuracy** in distance measurements with repeatable results
- **Optimal power efficiency** for Mipe device (target: 30+ days battery life)
- **Excellent user experience** through intuitive MotoApp interface
- **Single ping measurement capability** for real-time distance assessment

### Design Philosophy: Stability Over Speed
**Core Principle**: Stability and reliability have absolute priority over performance speed.

**Implementation Guidelines**:
- **Robust over Fast**: Choose stable algorithms over faster but less reliable ones
- **Consistent over Quick**: Prioritize consistent measurements over rapid updates
- **Reliable over Responsive**: Ensure connection stability over quick response times
- **Validated over Optimized**: Thoroughly test functionality before performance optimization
- **Error Recovery**: Comprehensive error handling and graceful degradation
- **Conservative Timing**: Use conservative timeouts and intervals for stability

### System Architecture
```
MotoApp (Android 14+) ←→ BLE ←→ Host (NRF54L15DK) ←→ BLE ←→ Mipe Device
         ↑                           ↑
    Cloud Services              USB-C Power Only
```

**Key Architecture Change**: Host connects **simultaneously** to both MotoApp and Mipe devices, not sequentially.

### Component Roles

#### Host Device (NRF54L15DK)
- **Primary Role**: Central measurement controller and BLE coordinator
- **Key Functions**: 
  - **Simultaneous BLE communication** to both MotoApp AND Mipe devices
  - **RSSI measurement** from Mipe connection with fixed RF parameters
  - **Data forwarding** of RSSI readings to MotoApp for distance calculation
  - Real-time data streaming to MotoApp
- **Power**: USB-C from phone (power-only, no data communication)

#### MotoApp (Android 14+)
- **Primary Role**: Control display, data processing, and cloud link extension of Host
- **Key Functions**:
  - **Distance calculation** from RSSI data received from Host
  - User interface for measurement control
  - Real-time distance visualization
  - Data logging and export
  - Cloud synchronization and remote monitoring
- **Architecture**: MVVM with BLE repository pattern
- **UI Framework**: Jetpack Compose or traditional Views

#### Mipe Device
- **Primary Role**: Power-optimized BLE peripheral for distance measurement
- **Key Functions**:
  - Minimal power consumption (sleep when not measuring)
  - **Stable RSSI signal source** for Host measurement
  - Host-controlled measurement sessions only
  - Consistent transmission power and timing

## LED Status Indicators

### Host and Mipe Device LED Specifications
- All codes will boot up with all 4 LEDs flashing for 200ms. This will indicate to User that the firmware is operational

#### Host Device LED Control
- **Hardware**: Use NRF54L15DK onboard LEDs (LED0-LED3)
- **Software**: Non-blocking LED control with timer-based state machines
- **Priority**: LED updates must not interfere with BLE operations
- **Power**: Not critical

#### Mipe Device LED Control
- **Hardware**: Use available LEDs on Mipe device (LED0-LED1)
- **Software**: None allowed
- **Priority**: LED operations must minimize battery consumption
- **Sleep Integration**: LEDs must work with power management states


## Communication Protocol

### Protocol Design Principles
1. **Stability First**: Reliable operation prioritized over speed or efficiency
2. **Mipe Transmits Minimally**: Only when prompted and absolutely necessary
3. **Host-Controlled Sessions**: Mipe responds, never initiates
4. **Power-First Design**: Every transmission optimized for Mipe battery life
5. **Deterministic Timing**: Consistent response times for accurate measurements
6. **Conservative Timeouts**: Use longer timeouts to ensure reliable connections
7. **Graceful Degradation**: System continues operating even with partial failures

### Communication Flows

#### Host ↔ MotoApp (Standard BLE GATT)
```
Connection: Standard BLE with GATT services
Data Flow: Bidirectional (control commands, RSSI data from Mipe, status)
GATT
Frequency: Near Real-time streaming during measurements
Power Impact: Host powered by USB-C, no power constraints
Key Change: Receives RSSI measurements from Host, calculates distance locally
```

#### Host ↔ Mipe (Power-Optimized RSSI Source)
```
Connection: RF-configured BLE with fixed parameters for stable RSSI
Advertising: Fast advertising for RSSI reads by Host, initially 100Hz. 
Advertising: Mipe will always return to advertising following a disconnect
Data Flow: Host measures RSSI from Mipe connection (minimal Mipe transmission)
GATT for battery voltage reading forwarding to host
Frequency: Continuous RSSI sampling during measurement sessions
Power Impact: Critical - Mipe optimized for minimal transmission
Key Change: Host measures signal strength FROM Mipe, forwards to MotoApp
```
#### UART logging
```
UART connection should be expected for both Mipe and Host
Add as many logging points in the codes as possible to deliver clarity to User about which part of the code is being run and what the BLE status parameters are. 


#### Simultaneous Connection Architecture
```
Host maintains on occuation two concurrent BLE connections:
1. BLE Peripheral → MotoApp (data display and control)
2. BLE Central → Mipe (Brief connection to read battery level, set Mipe parameters)

Data Flow:
Host measures RSSI from Mipe in advetising mode → Forwards RSSI data to MotoApp → MotoApp calculates distance and stores RSSI measurement data
```

### Detailed Data Flow Specifications

#### Host Data Collection Requirements
The Host device must collect and forward the following data to MotoApp for distance calculation:

**Primary RSSI Data (Essential)**
- **Raw RSSI Value**: Signal strength in dBm (e.g., -65 dBm)
- **Timestamp**: Precise measurement time (microsecond resolution)
- **Sample Rate**: Configurable by Mipe advertising frequency (default: 100ms intervals)
- **Data Format**: Signed 8-bit integer for RSSI, 32-bit timestamp

**Connection Quality Data (Important)**
- **Connection Status**: Connected/Disconnected/Reconnecting
- **Link Quality**: BLE connection quality indicator (0-255)
- **Packet Loss**: Count of missed/failed RSSI readings
- **Signal Stability**: RSSI variance over last 10 samples

**Environmental Context Data (Enhanced Accuracy)**
- **TX Power Level**: Mipe transmission power setting to max
- **Channel Information**: BLE channel used for measurement
- **Interference Level**: Background noise measurement
- **Temperature**: Host device temperature (affects RF performance)

**Battery and Power Data (System Health - Critical Priority)**
- **Mipe Battery Level**: voltage (mV)
- **Host Power Status**: USB-powered confirmation, Battery voltage level when powered by battery
- **Charging Status**: If Mipe supports charging, current charging state

#### Host to MotoApp Data Packet Structure

**Standard RSSI Measurement Packet (20 bytes)**
```
Byte 0-1:   Packet Header (0xAA55)
Byte 2:     Packet Type (0x01 = RSSI Data)
Byte 3:     RSSI Value (signed 8-bit, dBm)
Byte 4-7:   Timestamp (32-bit, microseconds)
Byte 8:     Link Quality (0-255)
Byte 9:     TX Power Level (signed 8-bit, dBm)
Byte 10:    Channel Number (0-39)
Byte 11:    Connection Status (bitmask)
Byte 12:    Mipe Battery Level (0-100%)
Byte 13-15: Reserved for future use
Byte 16-17: Packet Sequence Number
Byte 18-19: CRC16 Checksum
```

**Extended Measurement Packet (32 bytes) - Future Use**
```
Standard Packet (20 bytes) +
Byte 20-21: Turn-around Time (microseconds)
Byte 22:    Interference Level (0-255)
Byte 23-24: Extended CRC
```

#### MotoApp Data Processing Requirements
Dedicated Prompt available for MotoApp 

**Near Real-Time RSSI Processing according to Mipe ad frequency**
- **Receive Rate**: Handle 100ms interval packets (10 Hz)
- **Buffer Management**: Maintain rolling buffer of last 300 samples (30 seconds)
- **Data Validation**: Verify packet integrity, detect missing packets
- **Outlier Detection**: Filter unrealistic RSSI values

**Distance Calculation Pipeline**
```
1. RSSI Input: Receive raw RSSI from Host
2. Filtering: Apply statistical filters (moving average, outlier removal)
3. Algorithm Selection: Choose optimal distance calculation method
4. Distance Calculation: Convert RSSI to distance using selected algorithm
5. Confidence Calculation: Estimate measurement uncertainty
6. Display Update: Update UI with calculated distance and confidence
```

**Data Storage and Export**
- **Local Storage**: SQLite database for measurement history
- **Export Format**: CSV with columns: timestamp, rssi_dbm, distance_m, confidence_m, algorithm_used
- **Cloud Sync**: Optional upload to cloud services for analysis

#### BLE GATT Service Structure (Host → MotoApp)

**RSSI Measurement Service (UUID: Custom)**
- **RSSI Data Characteristic**: 
  - Properties: Notify, Read
  - Value: Current RSSI measurement packet
  - Update Rate: 100ms (configurable)
  
- **Measurement Control Characteristic**:
  - Properties: Write, Notify
  - Commands: START_MEASUREMENT, STOP_MEASUREMENT, SET_SAMPLE_RATE
  - Responses: ACK, NACK, STATUS_UPDATE

- **System Status Characteristic**:
  - Properties: Read, Notify
  - Value: Host system status, Mipe connection status, battery levels
  - Update Rate: 1000ms or on status change

- **Configuration Characteristic**:
  - Properties: Read, Write
  - Settings: Sample rate, TX power, measurement parameters
  - Persistence: Settings saved in Host non-volatile memory

#### Data Flow Timing and Synchronization

**Measurement Session Flow**
```
1. MotoApp → Host: START_MEASUREMENT command
2. Host → Mipe: Detect MIPE advertising
3. Host: Begin RSSI sampling from Mipe advertising
4. Host → MotoApp: Stream RSSI packets at configured rate
5. MotoApp: Real-time distance calculation and display
6. MotoApp → Host: STOP_MEASUREMENT command (when needed)
7. Host: Stop RSSI sampling, maintain Mipe connection
```

**Error Handling and Recovery**
```
- Mipe Connection Lost: Host notifies MotoApp, attempts reconnection
- Packet Loss: MotoApp detects missing sequence numbers, requests retransmission
- Invalid RSSI: Host filters impossible values before forwarding
- Buffer Overflow: MotoApp prioritizes recent data, discards oldest samples
```

**Performance Requirements - Stability Prioritized**
- **Reliability**: >99% uptime and connection stability (highest priority)
- **Consistency**: <5% variation in repeated measurements at same distance
- **Error Recovery**: <3 seconds to recover from connection failures
- **Robustness**: System operates reliably across temperature and interference variations
- **Latency**: <200ms from RSSI measurement to MotoApp display (acceptable for stability)
- **Throughput**: 10 Hz RSSI sampling (conservative rate for stable operation)
- **Battery Impact**: Minimal impact on phone battery life
- **Connection Stability**: Maintain connections for hours without intervention

**Stability Over Speed Guidelines**
- **Conservative Sampling**: Use 100ms intervals instead of faster rates for stability
- **Robust Algorithms**: Choose proven algorithms over experimental high-performance ones
- **Extended Timeouts**: Use 30-second connection timeouts instead of aggressive 5-second ones
- **Redundant Error Checking**: Multiple validation layers even if they add latency
- **Graceful Recovery**: Always prioritize maintaining partial functionality over speed

#### Revolving Connection Management

**Critical Requirement**: The system must support continuous connect/disconnect cycles without manual intervention on the Host side.

**Host Behavior - Revolving Discovery Mode**
```
1. Power-On State:
   - Host enters MotoApp discovery mode automatically
   - Begins advertising for MotoApp connection

2. MotoApp Connection Established:
   - Host ready for measurement commands
   - Maintains advertising capability for reconnection

3. MotoApp Disconnection (User initiated or connection lost):
   - Host detects disconnection immediately
   - Host automatically re-enters MotoApp discovery mode
   - Begins advertising again within 1 second
   - No manual reset or button press required

4. Reconnection Ready:
   - Host remains in discovery mode indefinitely
   - Ready to accept new MotoApp connection
   - When MotoApp "Connect" button pressed → immediate connection
```

**MotoApp Behavior - User-Controlled Connection**
```
1. App Launch:
   - Shows "CONNECT TO HOST" button
   - Scans for available Host devices
   - Displays discovered "MIPE_HOST_[MAC]" devices

2. User Presses "Connect":
   - Initiates BLE connection to selected Host
   - Button changes to "CONNECTING..."
   - Establishes GATT services and characteristics
   - Button changes to "DISCONNECT HOST" when connected

3. User Presses "Disconnect" or Connection Lost:
   - Gracefully disconnects from Host
   - Button returns to "CONNECT TO HOST"
   - Host automatically re-enters discovery mode
   - User can immediately reconnect by pressing "Connect" again

4. Automatic Reconnection (Optional):
   - App can optionally attempt automatic reconnection
   - Configurable setting: Auto-reconnect ON/OFF
   - Default: Manual reconnection for user control
```

**Mipe Connection Behavior - Host-Controlled**
```
1. Mipe Discovery/Advertising:
   - Host scans for Mipe devices when measurement needed

2. Mipe Connection:
   - Host connects to Mipe and retrives battery voltage data and delivers other data
   - Independent of MotoApp connection status
   - Connection can be toggled by Host continously
   
3. Mipe Disconnection:
   - Host can re-establish Mipe connection automatically
   - Independent of MotoApp connection status

4. Power Management:
   - Mipe will mainting advertising mode according to data parameters
   - Automatic wake-up and reconnection capability
```

**Connection State Persistence**
```
- Host maintains MotoApp discovery mode across power cycles
- No stored pairing information (fresh connection each time)
- Mipe pairing information stored for automatic reconnection
- User preferences stored in MotoApp (auto-reconnect settings)
```

**User Experience Requirements**
```
- Zero Host-side intervention required
- MotoApp "Connect" button always functional when Host in range
- Seamless reconnection experience
- No complex pairing procedures
- Robust operation across multiple connect/disconnect cycles
```

### Detailed Protocol Specifications

#### Initial Pairing Protocol
```
1. Host Discovery Phase:
   Host: Scans for Mipe devices with specific service UUID
   Mipe: Advertises with device identification and capabilities
   
2. Pairing Establishment:
   Host → Mipe: PAIR_REQUEST with Host identification
   Mipe → Host: PAIR_ACCEPT with Mipe capabilities and power status
   Host → Mipe: PAIR_CONFIRM 
   Mipe: Stores Host identity for future reconnections
```

#### Connection Recovery Protocol
```
Distance measurement systems require robust reconnection due to varying transmission distances.

1. Connection Loss Detection:
   Host: Monitors connection timeout 
   Mipe: Detects connection loss via supervision timeout
   
2. Mipe Listening Mode:
   Upon connection loss, Mipe enters "Listening Mode":
   - Maintains low-power advertising (extended intervals)
   - Listens for specific Host reconnection ping
   - Preserves pairing information and calibration data
   - Timeout: Remains in listening mode for configurable period (default: 5 minutes)
   
3. Host Reconnection Sequence:
   Host → Mipe: RECONNECT_PING with stored Mipe identity
   Mipe → Host: RECONNECT_ACK with current status and battery level
   Host → Mipe: RECONNECT_CONFIRM with measurement session parameters
   Connection re-established with previous calibration data intact
```

#### Power-Optimized Listening Mode
```
Mipe Listening Mode Specifications:
- Advertising Interval:  100ms
```

#### GATT Service Structure (Host ↔ MotoApp)
```
Primary Service: Mipe Host Control Service
├── Measurement Control (Write, Notify) - Start/stop measurements
├── Measurement Data (Read, Notify) - Real-time distance data
├── System Status (Read, Notify) - Connection states, battery, errors
├── Configuration (Read, Write) - Measurement parameters, calibration
└── Device Information (Read) - Firmware version, capabilities
```

### RF Configuration Requirements

#### Host and Mipe RF Parameters 
- **Rx Gain Control**: Fixed setting to eliminate AGC variations (±0.5dB accuracy)
- **Tx Power Control**: Fixed transmission power for stable signal characteristics

#### Standard BLE Parameters (for MotoApp communication)
- **Normal RF settings**: Automatic gain control and standard BLE parameters
- **GATT Services**: Full service discovery and characteristic operations
- **Connection Intervals**: Optimized for real-time data streaming


## Development Guidelines for AI Assistant (Cline)

### Core Principles
1. **Balance Task with Global Architecture**: Always consider how each task fits into the overall system design
2. **Align with Global Objectives**: Every implementation decision should support 10% accuracy, power efficiency, and good UX
3. **Provide Suggestions**: Offer improvements and alternatives based on experience, but stay focused on current task
4. **Granular Focus**: Complete one specific task thoroughly before moving to the next
5. **Integration Awareness**: Consider how current task affects other components and future TMTs

### Task Execution Guidelines
1. **Read Master Prompt**: Always reference this document for context and constraints
2. **Understand TMT Context**: Know which TMT the current task supports and why
3. **Check Dependencies**: Ensure prerequisite tasks are completed before starting
4. **Consider Power Impact**: For Mipe-related tasks, prioritize power efficiency
5. **Plan for Testing**: Implement with manual testing and validation in mind

### Communication Protocol Implementation
1. **Mipe Tasks**: Every transmission must be justified for power efficiency
2. **Host Tasks**: Balance between MotoApp communication and Mipe power optimization
3. **MotoApp Tasks**: Focus on user experience and real-time data presentation
4. **Integration Tasks**: Ensure seamless operation across all components

### Quality Standards
1. **Code Quality**: Clean, maintainable, well-documented code
2. **Error Handling**: Robust error handling and recovery mechanisms
3. **Performance**: Meet timing and accuracy requirements
4. **Power Efficiency**: Minimize power consumption, especially for Mipe
5. **User Experience**: Intuitive and responsive interface design

This Master Project Prompt provides the foundation for all development work. Each task should reference this document and align with its principles and objectives.



