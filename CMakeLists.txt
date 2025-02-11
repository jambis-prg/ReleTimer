cmake_minimum_required(VERSION 3.22)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
set(PICOTOOL_FETCH_FROM_GIT_PATH "~/picotool")

project(SwitchTimer)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(SwitchTimer
    src/main.cpp
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(SwitchTimer pico_stdlib hardware_i2c)

pico_enable_stdio_usb(SwitchTimer 1)
pico_enable_stdio_uart(SwitchTimer 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(SwitchTimer)