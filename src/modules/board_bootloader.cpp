extern "C" {
    #include "can2040.h"
    #include "pico/stdlib.h"
    #include <stdio.h>
    #include <stddef.h>
}

#include "board_bootloader.h"

#include <pico/bootrom.h>

constexpr uint32_t status_blink_pin = 2;
constexpr uint32_t status_blink_sleep_time_ms = 250;

constexpr bool startup_blink_sequence[] = {
    0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
    0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
};
constexpr size_t startup_blink_sequence_len = sizeof startup_blink_sequence / sizeof *startup_blink_sequence;

void board_bootloader_loop()
{
    gpio_init(status_blink_pin);
    gpio_set_function(status_blink_pin, GPIO_FUNC_SIO);

    gpio_set_dir(status_blink_pin, GPIO_OUT);

    for (size_t i = 0; i < startup_blink_sequence_len; i++)
    {
        gpio_put(status_blink_pin, startup_blink_sequence[i]);
        sleep_ms(status_blink_sleep_time_ms);
    }
}
