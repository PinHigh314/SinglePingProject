# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(singleping_mipe)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE
    src/main.c
    src/led_control.c
    src/button_control.c
    src/ble_service.c
    src/connection_manager.c
    src/battery_monitor.c
)
