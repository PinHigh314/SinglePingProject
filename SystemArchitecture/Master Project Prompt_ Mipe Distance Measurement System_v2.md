# Mipe Distance Measurement System - Master Project Prompt

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
- **Verification**: LED status indicators (LED0=heartbeat, LED1=MotoApp, LED2=Mipe)

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

### Host Device LED Specifications

#### LED0 - System Heartbeat
- **Function**: System operational indicator
- **Pattern**: Continuous flashing at 500ms intervals
- **Duty Cycle**: 50% (250ms ON, 250ms OFF)
- **Behavior**: Always active when Host is powered and operational
- **Purpose**: Confirms main system loop is running

#### LED1 - MotoApp Connection Status
- **Pairing Mode**: Flashing at 200ms intervals (100ms ON, 100ms OFF)
- **Connected State**: Fully lit (solid ON)
- **Disconnected State**: OFF
- **Behavior**:
  - Starts flashing when Host enters MotoApp pairing mode
  - Switches to solid ON when MotoApp successfully connects
  - Returns to flashing if connection lost and attempting reconnection
  - Turns OFF when not in pairing mode and not connected

#### LED2 - Mipe Connection Status
- **Connected State**: Fully lit (solid ON)
- **Disconnected State**: OFF
- **Behavior**:
  - Turns ON when Mipe device successfully connects to Host
  - Remains ON during entire Mipe connection session
  - Turns OFF immediately when Mipe connection is lost
  - No flashing pattern (Mipe connection is Host-initiated)

#### LED3 - RSSI Data Streaming Status
- **Streaming Active**: Fully lit (solid ON)
- **Streaming Inactive**: OFF
- **Behavior**:
  - Turns ON when Host begins RSSI sampling from Mipe
  - Remains ON during entire measurement session
  - Turns OFF when RSSI streaming stops (STOP_MEASUREMENT command)
  - Independent of connection status (can be OFF even when connected)

### Mipe Device LED Specifications

#### LED0 - System Heartbeat
- **Function**: System operational indicator
- **Pattern**: Continuous flashing at 1000ms intervals (power-optimized)
- **Duty Cycle**: 10% (100ms ON, 900ms OFF)
- **Behavior**: Always active when Mipe is powered and operational
- **Purpose**: Confirms Mipe is alive and responsive (minimal power consumption)

#### LED1 - Host Connection Status
- **Pairing Mode**: Flashing at 200ms intervals (100ms ON, 100ms OFF)
- **Connected State**: Fully lit (solid ON)
- **Disconnected State**: OFF
- **Behavior**:
  - Starts flashing when Mipe enters pairing/advertising mode
  - Switches to solid ON when Host successfully connects
  - Returns to flashing if connection lost and re-entering pairing mode
  - Turns OFF when in deep sleep or power-save mode

### LED Status Interpretation Guide

#### Host Device Status Combinations
- **LED0 Flashing, LED1 OFF, LED2 OFF, LED3 OFF**: System running, no connections
- **LED0 Flashing, LED1 Flashing, LED2 OFF, LED3 OFF**: Searching for MotoApp
- **LED0 Flashing, LED1 ON, LED2 OFF, LED3 OFF**: Connected to MotoApp only
- **LED0 Flashing, LED1 ON, LED2 ON, LED3 OFF**: Connected to both, no measurement
- **LED0 Flashing, LED1 ON, LED2 ON, LED3 ON**: Full system operational, measuring
- **LED0 OFF**: System failure or power loss

#### Mipe Device Status Combinations
- **LED0 Flashing, LED1 OFF**: Powered but not advertising (sleep mode)
- **LED0 Flashing, LED1 Flashing**: Advertising, waiting for Host connection
- **LED0 Flashing, LED1 ON**: Connected to Host, ready for measurements
- **LED0 OFF**: Deep sleep or power loss

### LED Implementation Requirements

#### Host Device LED Control
- **Hardware**: Use NRF54L15DK onboard LEDs (LED0-LED3)
- **Software**: Non-blocking LED control with timer-based state machines
- **Priority**: LED updates must not interfere with BLE operations
- **Power**: Minimal impact on USB power consumption

#### Mipe Device LED Control
- **Hardware**: Use available LEDs on Mipe device (LED0-LED1)
- **Software**: Power-optimized LED control with minimal current draw
- **Priority**: LED operations must minimize battery consumption
- **Sleep Integration**: LEDs must work with power management states

#### LED Timing Specifications
- **Heartbeat Timing**: Must be precise and consistent
- **Flash Timing**: 200ms flash must be accurate for user recognition
- **State Transitions**: LED changes must be immediate (<10ms delay)
- **Synchronization**: No requirement for LED synchronization between devices

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
Frequency: Real-time streaming during measurements
Power Impact: Host powered by USB-C, no power constraints
Key Change: Receives RSSI measurements from Host, calculates distance locally
```

#### Host ↔ Mipe (Power-Optimized RSSI Source)
```
Connection: RF-configured BLE with fixed parameters for stable RSSI
Data Flow: Host measures RSSI from Mipe connection (minimal Mipe transmission)
Frequency: Continuous RSSI sampling during measurement sessions
Power Impact: Critical - Mipe optimized for minimal transmission
Key Change: Host measures signal strength FROM Mipe, forwards to MotoApp
```

#### Simultaneous Connection Architecture
```
Host maintains two concurrent BLE connections:
1. BLE Peripheral → MotoApp (data display and control)
2. BLE Central → Mipe (RSSI measurement source)

Data Flow:
Host measures RSSI from Mipe → Forwards RSSI to MotoApp → MotoApp calculates distance
```

### Detailed Data Flow Specifications

#### Host Data Collection Requirements
The Host device must collect and forward the following data to MotoApp for distance calculation:

**Primary RSSI Data (Essential)**
- **Raw RSSI Value**: Signal strength in dBm (e.g., -65 dBm)
- **Timestamp**: Precise measurement time (microsecond resolution)
- **Sample Rate**: Configurable (default: 100ms intervals)
- **Data Format**: Signed 8-bit integer for RSSI, 32-bit timestamp

**Connection Quality Data (Important)**
- **Connection Status**: Connected/Disconnected/Reconnecting
- **Link Quality**: BLE connection quality indicator (0-255)
- **Packet Loss**: Count of missed/failed RSSI readings
- **Signal Stability**: RSSI variance over last 10 samples

**Environmental Context Data (Enhanced Accuracy)**
- **TX Power Level**: Mipe transmission power setting
- **Channel Information**: BLE channel used for measurement
- **Interference Level**: Background noise measurement
- **Temperature**: Host device temperature (affects RF performance)

**Battery and Power Data (System Health - Critical Priority)**
- **Mipe Battery Level**: Percentage (0-100%) and voltage (mV)
- **Mipe Battery Health**: Charge cycles, degradation status, estimated remaining life
- **Mipe Power Rating**: Battery capacity (mAh), chemistry type, voltage range
- **Host Power Status**: USB-powered confirmation, input voltage/current
- **Low Battery Alerts**: Critical battery warnings with thresholds
- **Power Optimization Status**: Current power saving mode and efficiency metrics
- **Battery Temperature**: Mipe battery temperature for safety monitoring
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
Byte 22:    Temperature (signed 8-bit, Celsius)
Byte 23:    Interference Level (0-255)
Byte 24-27: GPS Coordinates (optional)
Byte 28-29: Altitude (optional)
Byte 30-31: Extended CRC
```

#### MotoApp Data Processing Requirements

**Real-Time RSSI Processing**
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
2. Host → Mipe: Establish/verify connection
3. Host: Begin RSSI sampling from Mipe connection
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
   - LED1 starts flashing (200ms intervals)
   - Begins advertising for MotoApp connection

2. MotoApp Connection Established:
   - LED1 switches to solid ON
   - Host ready for measurement commands
   - Maintains advertising capability for reconnection

3. MotoApp Disconnection (User initiated or connection lost):
   - Host detects disconnection immediately
   - LED1 switches from solid to flashing (200ms intervals)
   - Host automatically re-enters MotoApp discovery mode
   - Begins advertising again within 1 second
   - No manual reset or button press required

4. Reconnection Ready:
   - Host remains in discovery mode indefinitely
   - Ready to accept new MotoApp connection
   - When MotoApp "Connect" button pressed → immediate connection
   - LED1 returns to solid ON when reconnected
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
1. Mipe Discovery:
   - Host scans for Mipe devices when measurement needed
   - LED2 turns ON when Mipe connected
   - Maintains connection during measurement sessions

2. Mipe Disconnection:
   - LED2 turns OFF when connection lost
   - Host can re-establish Mipe connection automatically
   - Independent of MotoApp connection status

3. Power Management:
   - Mipe can enter sleep mode between measurements
   - Host re-establishes connection when needed
   - Automatic wake-up and reconnection capability
```

**Connection State Persistence**
```
- Host maintains MotoApp discovery mode across power cycles
- No stored pairing information (fresh connection each time)
- Mipe pairing information stored for automatic reconnection
- User preferences stored in MotoApp (auto-reconnect settings)
```

**LED Behavior During Revolving Connections**
```
Host LED1 Pattern:
- Flashing: Always in discovery mode (ready for MotoApp)
- Solid: Connected to MotoApp
- Immediate transition: <1 second from disconnect to discovery mode

Mipe LED1 Pattern:
- Flashing: Advertising for Host connection
- Solid: Connected to Host
- Automatic re-advertising after disconnection
```

**User Experience Requirements**
```
- Zero Host-side intervention required
- MotoApp "Connect" button always functional when Host in range
- Seamless reconnection experience
- Clear visual feedback via LEDs
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
   Host → Mipe: PAIR_CONFIRM with measurement parameters
   Mipe: Stores Host identity for future reconnections
   
3. Initial Calibration:
   Host: Performs baseline RSSI measurements at known distance
   Mipe: Maintains stable signal during calibration phase
   Host: Establishes distance calculation baseline
```

#### Connection Recovery Protocol
```
Distance measurement systems require robust reconnection due to varying transmission distances.

1. Connection Loss Detection:
   Host: Monitors connection timeout and RSSI fade
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
   
4. Single Ping Response:
   When Mipe receives RECONNECT_PING:
   - Immediately responds with RECONNECT_ACK (single response)
   - Transitions to active connection mode
   - No repeated responses to avoid power waste
   - If no RECONNECT_CONFIRM received within 5 seconds, returns to listening mode
```

#### Power-Optimized Listening Mode
```
Mipe Listening Mode Specifications:
- Advertising Interval: 1000ms (vs 100ms during initial pairing)
- Advertising Duration: 100ms every 1000ms (10% duty cycle)
- Connection Window: 50ms response window for reconnection pings
- Power Consumption: <10μA average during listening mode
- Maximum Listening Duration: 5 minutes before deep sleep
- Deep Sleep Recovery: Button press or scheduled wake (if implemented)
```

#### Mipe Power-Optimized Protocol
```
1. Session Initiation:
   Host → Mipe: WAKE_REQUEST (4 bytes)
   Mipe → Host: WAKE_ACK (2 bytes)

2. Measurement Phase:
   Host: Controls RSSI sampling timing
   Mipe: Maintains stable signal, responds to queries only
   Duration: Minimal (< 1 second per measurement)

3. Session Termination:
   Host → Mipe: SLEEP_COMMAND (2 bytes)
   Mipe: Immediate sleep mode (no acknowledgment needed)
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

#### Host RF Parameters (for Mipe communication)
- **Rx Gain Control**: Fixed setting to eliminate AGC variations (±0.5dB accuracy)
- **Tx Power Control**: Fixed transmission power for stable signal characteristics
- **Channel Selection**: Single channel operation to avoid hopping variations
- **Connection Intervals**: Optimized for measurement accuracy vs power consumption

#### Standard BLE Parameters (for MotoApp communication)
- **Normal RF settings**: Automatic gain control and standard BLE parameters
- **GATT Services**: Full service discovery and characteristic operations
- **Connection Intervals**: Optimized for real-time data streaming

## Test Milestone Tests (TMT) Methodology

### TMT Execution Principles
1. **Strictly Sequential**: Complete one TMT before starting the next
2. **Single Task Focus**: One task at a time within each TMT
3. **Manual Validation**: User conducts all integration tests
4. **Granular Tasks**: Prevent AI assistant from wandering into multiple improvements
5. **Flexible Addition**: Can add tasks mid-TMT or revisit completed sections

### TMT Success Criteria
Each TMT must demonstrate:
- **Functional Completeness**: All planned features working
- **Integration Success**: Components communicate correctly
- **Performance Targets**: Meet accuracy, power, or UX requirements
- **Stability**: Reliable operation under test conditions

### Bug Fixing and Iteration Process
1. **Bug Discovery**: During TMT validation testing
2. **Task Addition**: Add specific bug fix tasks to current TMT
3. **Focus Shift**: Complete bug fixes before proceeding
4. **Re-validation**: Repeat TMT test after fixes
5. **TMT Completion**: Only when all criteria met

## Proposed TMT Structure

### TMT1: MotoApp Foundation
**Objective**: Build complete MotoApp user interface and functionality (no BLE connectivity)

**Success Criteria**:
- MotoApp UI fully implemented with all planned screens and elements
- All buttons, graphs, and status displays functional (with mock data)
- Connection status indicators implemented (showing disconnected state)
- Distance measurement display with simulated data visualization
- Settings and configuration screens operational
- APK builds and installs successfully on Android 14+
- Navigation between screens working smoothly
- User interface meets design requirements

**Key Deliverables**:
- Complete MotoApp UI implementation
- Main screen with measurement display and controls
- Connection status screen with Host device indicators
- Real-time graph display for distance measurements
- Settings screen for configuration options
- Mock data generation for UI testing
- Professional UI/UX design implementation

**UI Elements to Implement**:
- **Main Screen**: Distance display, measurement controls, connection status
- **Connection Screen**: Host device discovery status, connection indicators
- **Measurement Screen**: Real-time distance graph, measurement history
- **Settings Screen**: BLE configuration, measurement parameters, calibration options
- **Status Indicators**: Connection state, battery levels, error messages
- **Control Buttons**: Start/stop measurement, connect/disconnect, settings access

### TMT2: App-Host BLE Integration
**Objective**: Establish BLE communication between MotoApp and Host with simulated test data

**Success Criteria**:
- MotoApp can discover Host device by name "MIPE_HOST_[MAC]"
- Stable BLE connection establishment and maintenance
- GATT service discovery and characteristic access working
- Host streams simulated measurement data to MotoApp
- Real-time data display in MotoApp graphs and indicators
- Connection status correctly reflected in UI
- LED1 on Host correctly shows MotoApp connection status
- Automatic reconnection working after connection loss

**Key Deliverables**:
- Host BLE advertising with correct device identification
- MotoApp BLE Central implementation with Host discovery
- GATT services implemented on Host (with simulated data)
- Real-time data streaming from Host to MotoApp
- Connection management on both Host and MotoApp
- UI updates with real BLE connection status
- Error handling for connection failures

**Host Simulated Data**:
- Distance measurements (1-15 meters with realistic variations)
- RSSI values (-40 to -80 dBm range)
- System status (connected, measuring, idle)
- Battery status simulation
- Measurement timing data

### TMT3: Mipe Device Integration
**Objective**: Add Mipe device and establish Host-Mipe communication with battery status monitoring

**Success Criteria**:
- Host can discover and connect to Mipe device
- Sequential BLE mode switching (MotoApp → Mipe → MotoApp) operational
- Basic RSSI measurements between Host and Mipe working
- Mipe battery status transmission to Host and display in MotoApp
- Power-optimized protocol implementation functional
- LED2 correctly indicates Mipe connection status
- Connection recovery protocol working (listening mode, reconnection ping)
- Pairing and re-pairing functionality operational

**Key Deliverables**:
- Mipe BLE peripheral implementation with power optimization
- Host BLE Central functionality for Mipe communication
- Sequential communication state machine (Host switches between MotoApp and Mipe)
- Basic RSSI sampling capability between Host and Mipe
- Battery status monitoring and transmission from Mipe
- Connection recovery protocol (listening mode, single ping response)
- MotoApp display of Mipe connection status and battery level

**Battery Status Integration**:
- Mipe transmits battery percentage to Host
- Host forwards battery status to MotoApp via GATT
- MotoApp displays Mipe battery level in real-time
- Low battery warnings and notifications
- Battery status included in connection recovery protocol

### TMT4: Distance Calculation with Real Data
**Objective**: Implement advanced distance calculation using real RSSI and timing data from Mipe

**Success Criteria**:
- Multiple distance calculation algorithms implemented and tested
- Real RSSI data from Host-Mipe communication used (no simulated data)
- Turn-around time measurement system operational
- Data clustering and pattern recognition working
- 10% accuracy achieved at test distances (1m, 3m, 5m, 10m, 15m)
- Real-time distance calculation with <100ms latency
- MotoApp displays real calculated distances instead of simulated data
- Algorithm performance comparison and selection complete

**Key Deliverables**:
- Replace all simulated data with real measurements from Mipe
- Multi-modal distance calculation system (RSSI + Turn-around time)
- Mathematical model library and algorithm iteration framework
- Real-time distance streaming to MotoApp with confidence intervals
- Data collection and analysis system for algorithm optimization
- Performance benchmarking and model selection

**Real Data Integration**:
- Host collects actual RSSI measurements from Mipe communication
- Turn-around time measurement from Host request to Mipe response
- Environmental data collection for compensation algorithms
- Statistical analysis of real measurement variations
- Algorithm training and optimization using collected real data

### TMT5: RF Optimization & Turn-Around Time
**Objective**: Implement RF parameter control and precise timing measurement for enhanced accuracy

**Success Criteria**:
- Fixed Rx gain control operational (±0.5dB stability)
- Fixed Tx power control implemented
- RSSI measurements show improved consistency with real data
- Turn-around time measurement system operational with sub-millisecond precision
- Consistent response timing achieved (<1ms variation)
- RF parameter validation and monitoring working
- Improved distance calculation accuracy with optimized RF parameters

**Key Deliverables**:
- RF configuration APIs and control systems
- RSSI sampling with fixed parameters for consistent measurements
- Turn-around time measurement system (Host request → Mipe response timing)
- Response time consistency validation and optimization
- Combined RSSI + timing data collection and analysis
- RF parameter monitoring and adjustment system

### TMT6: Power Optimization
**Objective**: Minimize Mipe power consumption for extended battery life while maintaining accuracy

**Success Criteria**:
- Mipe sleep/wake cycle optimized for minimal power consumption
- Power consumption measured and validated using eval board measurement system
- 30+ day battery life projection achieved and verified
- Minimal transmission protocol fully implemented and tested
- Power monitoring and reporting operational in real-time
- Battery life optimization without compromising measurement accuracy

**Key Deliverables**:
- Power-optimized Mipe firmware with intelligent sleep/wake management
- Sleep/wake state management with measurement session coordination
- Power consumption measurement system using eval board capabilities
- Battery life projection validation with real usage patterns
- Optimized communication protocol minimizing Mipe transmissions
- Real-time power monitoring and battery status reporting

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



#### Connection State Management
```
Host Connection States:
1. DISCONNECTED: No Mipe connection, LED2 off
2. DISCOVERING: Scanning for Mipe devices, LED2 fast flash (100ms)
3. PAIRING: Initial pairing in progress, LED2 medium flash (200ms)
4. CONNECTED: Active connection established, LED2 solid on
5. MEASURING: Measurement session active, LED2 solid on
6. RECONNECTING: Attempting reconnection after loss, LED2 slow flash (500ms)

Mipe Connection States:
1. ADVERTISING: Initial pairing mode, high power advertising
2. LISTENING: Post-connection loss, low power advertising
3. CONNECTED: Active connection with Host
4. MEASURING: Responding to measurement session
5. DEEP_SLEEP: Power conservation mode, no advertising
```

#### Error Handling and Recovery
```
Connection Timeout Scenarios:

1. Pairing Timeout (30 seconds):
   Host: Returns to DISCONNECTED state, stops scanning
   Mipe: Returns to ADVERTISING mode for 2 minutes, then DEEP_SLEEP
   
2. Measurement Session Timeout (10 seconds):
   Host: Aborts session, attempts reconnection
   Mipe: Returns to LISTENING mode, preserves pairing data
   
3. Reconnection Failure (3 attempts):
   Host: Reports connection lost to MotoApp, stops attempts
   Mipe: Enters DEEP_SLEEP mode to conserve battery
   
4. Battery Low (Mipe <10%):
   Mipe → Host: LOW_BATTERY_WARNING before entering DEEP_SLEEP
   Host → MotoApp: Battery status notification
   
5. Range Exceeded (RSSI < -80dBm):
   Host: Warns user via MotoApp, continues attempting connection
   Mipe: Maintains LISTENING mode until timeout
```

#### Protocol Packet Structures
```
RECONNECT_PING (Host → Mipe):
- Byte 0: Command ID (0x01)
- Bytes 1-4: Host Identity (stored during pairing)
- Byte 5: Sequence Number
- Byte 6: Checksum
Total: 7 bytes

RECONNECT_ACK (Mipe → Host):
- Byte 0: Response ID (0x81)
- Byte 1: Mipe Status (connected/battery level)
- Byte 2: Signal Strength Indicator
- Byte 3: Sequence Number (echo)
- Byte 4: Checksum
Total: 5 bytes

RECONNECT_CONFIRM (Host → Mipe):
- Byte 0: Command ID (0x02)
- Byte 1: Measurement Parameters
- Byte 2: Session Timeout
- Byte 3: Checksum
Total: 4 bytes
```

