cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tmt1_mock_data_host)

# Include header directory
target_include_directories(app PRIVATE include)

# Add all source files
target_sources(app PRIVATE
    src/main.c
)
