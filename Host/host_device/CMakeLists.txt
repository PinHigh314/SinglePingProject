cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(host_device_test_v9)

# Main application source - v9 with LED1 timing fix
target_sources(app PRIVATE
    src/main_test_fixed_rssi_v9.c
    src/ble/ble_peripheral_v8.c
    src/ble/ble_central_real.c
)

# Include directories
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
