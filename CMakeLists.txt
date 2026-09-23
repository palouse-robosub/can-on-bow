cmake_minimum_required(VERSION 3.13)

# Pico SDK
set(PICO_PLATFORM rp2350)
include(pico_sdk_import.cmake)

project(guppy_embedded)
set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

# Add can2040 lib
include(FetchContent)

FetchContent_Declare(
    can2040
    GIT_REPOSITORY https://github.com/KevinOConnor/can2040.git
    GIT_TAG master
)

FetchContent_MakeAvailable(can2040)

add_library(can2040 STATIC
    ${can2040_SOURCE_DIR}/src/can2040.c
)

target_include_directories(can2040 PUBLIC
    ${can2040_SOURCE_DIR}/src
)

target_link_libraries(can2040
    hardware_pio
)

# add stuff from modules folder
file(GLOB MODULE_SOURCES
    src/modules/*.c
    src/modules/*.cpp
)

function(add_board BOARD_NAME BOARD_TYPE)

    add_executable(${BOARD_NAME}
        src/main.cpp
        ${MODULE_SOURCES}
    )

    pico_set_binary_type(${BOARD_NAME} copy_to_ram)
    target_link_libraries(${BOARD_NAME}
        pico_stdlib
        can2040
    )

    target_compile_definitions(${BOARD_NAME} PRIVATE BOARD_TYPE=${BOARD_TYPE})

    pico_enable_stdio_usb(${BOARD_NAME} 1)
    pico_enable_stdio_uart(${BOARD_NAME} 0)

endfunction()

add_board(board_bootloader -2)

pico_add_extra_outputs(board_bootloader)
