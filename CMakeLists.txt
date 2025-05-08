cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
set(PICOTOOL_FETCH_FROM_GIT_PATH "~/picotool")

project(ReleTimer)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(ReleTimer
    src/main.cpp
    src/ssd1306.cpp
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(ReleTimer pico_stdlib hardware_i2c)

pico_enable_stdio_usb(ReleTimer 1)
pico_enable_stdio_uart(ReleTimer 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(ReleTimer)