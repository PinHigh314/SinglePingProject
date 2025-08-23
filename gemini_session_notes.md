# Gemini Session Notes

## Last Updated: 2025-08-21

## My Rules of Conduct

I will adhere to the following rules as specified in `rules/geminiRules.txt`:
1.  **First Actions:** Always read `PROJECT_CONTEXT_MEMORY.md` then `TMT_Structured_Tasks.json` at the start of every session.
2.  **Research First:** I will conduct exhaustive research before any implementation.
3.  **Step-by-step:** I will work on one task at a time and wait for confirmation before proceeding.
4.  **Versioning:** I will follow the specified naming conventions for all build artifacts.
5.  **Logging:** I will maintain and update the project's intelligence logs.
6.  **Forbidden Topics:** I will not discuss USB CDC or UART over USB.
7.  **TMT Workflow:** I will strictly follow the task management workflow defined in the rules.

## Project Understanding

I understand that this project is to create a 3-piece distance measurement system (MotoApp, Host, Mipe) with a strong emphasis on stability. The Host device (nRF54L15DK) will connect to both the MotoApp (Android) and the Mipe device simultaneously. The Host will measure the RSSI from the Mipe's BLE signal and send this data to the MotoApp, which will then calculate and display the distance.

I have reviewed the following documents to gain this understanding:
- `SystemArchitecture/Master Project Prompt_ Mipe Distance Measurement System_v2.md`
- `SystemArchitecture/MotoApp TMT1 Focused UI Specification.md`
- `SystemArchitecture/NRF54L15DK_DEVELOPMENT_INTELLIGENCE.md`
- `SystemArchitecture/SYSTEM_ARCHITECT_LOG.md`
- `New_SinglePing_nRF54L15DK/PROJECT_CONTEXT_MEMORY.md`
- `BuildExperience.md`
- `rules/geminiRules.txt`

## Last Completed Task

**Task M001: Android Project Setup**
- **Status:** Complete.
- **Details:** I have successfully configured the `MotoApp` project to build without errors.

## Next Planned Task

**TMT1: MotoApp Foundation (with Host mock data)**
- **Objective:** Build the MotoApp UI and a corresponding Host firmware that provides mock data.
- **Plan:**
    1.  **Host Firmware (TMT1 version):**
        -   Create a simple Host firmware that advertises as a BLE peripheral.
        -   Implement the GATT services as defined in the `Master Project Prompt`.
        -   The "Measurement Data Characteristic" will notify with *fake* RSSI data.
        -   Implement the LED indicators for connection status.
    2.  **MotoApp (TMT1 version):**
        -   Implement the single-screen UI as specified in the `MotoApp TMT1 Focused UI Specification.md`.
        -   Implement the BLE communication logic to connect to the Host device.
        -   Receive the fake RSSI data from the Host and display it on the graph.
        -   Calculate and display the distance based on the fake RSSI data.
    3.  **Task Breakdown:**
        -   I will start with **Task M003: Connection Status Screen** in the `TMT_Structured_Tasks.json` file, which involves implementing the connection logic in the MotoApp.
        -   In parallel, I will need to work on a new task for the Host firmware to provide the mock data. I will propose a new task for this.
