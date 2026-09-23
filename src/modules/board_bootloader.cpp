extern "C" {
#include "can2040.h"
#include "pico/stdlib.h"
#include <stddef.h>
#include <stdlib.h>
}

#include "board_bootloader.h"

#include <pico/bootrom.h>

constexpr uint32_t status_blink_pin = 2;

constexpr uint32_t status_blink_sleep_time_ms = 250;
constexpr uint32_t status_error_blink_sleep_time_ms = 25;

constexpr bool startup_blink_sequence[] = {
    0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
    0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
};
constexpr size_t startup_blink_sequence_len = sizeof startup_blink_sequence / sizeof *startup_blink_sequence;

void blink_startup_status();
void blink_error_status(int32_t error_code);

void board_bootloader_loop()
{
    gpio_init(status_blink_pin);
    gpio_set_function(status_blink_pin, GPIO_FUNC_SIO);

    gpio_set_dir(status_blink_pin, GPIO_OUT);

    blink_startup_status();
    blink_error_status();

    blink_error_status(BOOTROM_OK);
}

void blink_startup_status()
{
    for (size_t i = 0; i < startup_blink_sequence_len; i++)
    {
        gpio_put(status_blink_pin, startup_blink_sequence[i]);
        sleep_ms(status_blink_sleep_time_ms);
    }
}

void blink_error_status(int32_t error_code)
{
    bool status = false;
    size_t blink_count = abs(error_code) * 2;

    for (size_t i = 0; i < 40; ++i)
    {
        gpio_put(status_blink_pin, status);
        sleep_ms(status_error_blink_sleep_time_ms);

        status = !status;
    }

    gpio_put(status_blink_pin, false);
    sleep_ms(1000);

    for (size_t i = 0; i < blink_count; ++i)
    {
        gpio_put(status_blink_pin, status);
        sleep_ms(status_blink_sleep_time_ms);

        status = !status;
    }

    gpio_put(status_blink_pin, false);
    sleep_ms(1000);

    while (true)
    {
        gpio_put(status_blink_pin, status);
        sleep_ms(status_error_blink_sleep_time_ms);

        status = !status;
    }
}
