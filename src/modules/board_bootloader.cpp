extern "C" {
#include "can2040.h"
#include "pico/stdlib.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
}

#include "board_bootloader.h"

#include <boot/picobin.h>
#include <pico/bootrom.h>
#include <hardware/flash.h>
#include <pico/assert.h>

/*
 * Information given by get_partition_table_info with flag
 * PT_INFO_PARTITION_LOCATION_AND_FLAGS in first word. Second word
 * contains partition flag data, but haven't needed that yet so
 * didn't bother making that easier to query.
 */
struct partition_location_info {
    /*
     * The bootrom internally refers to BOOTSEL mode as NSBOOT, because the USB
     * and UART bootloaders run in the Non-secure state under Arm.
     */
    bool nsboot_writable; // ie. BOOTSEL
    bool nsboot_readable;
    bool nonsecure_writable;
    bool nonsecure_readable;
    bool secure_writable;
    bool secure_readable;
    uint32_t first_sector; // NOTE: SECTOR NOT ADDRESS. Uses 4kiB sectors
    uint32_t last_sector;
};

constexpr uint32_t status_blink_pin = 2;

constexpr uint32_t status_blink_sleep_time_ms = 250;
constexpr uint32_t status_error_blink_sleep_time_ms = 25;

constexpr bool startup_blink_sequence[] = {
    0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
    0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
};
constexpr size_t startup_blink_sequence_len = sizeof startup_blink_sequence / sizeof *startup_blink_sequence;

// load_partition_table requires a user provided work area. Currently requires
// 3kiB of space, using 4kiB just in case.
constexpr size_t pt_workarea_size = 4096;
static uint8_t pt_workarea[pt_workarea_size];

void blink_startup_status();
void blink_error_status(int32_t error_code);

partition_location_info get_partition_location_info(int32_t part_number);

void board_bootloader_loop()
{
    int32_t error_code = BOOTROM_OK;

    gpio_init(status_blink_pin);
    gpio_set_function(status_blink_pin, GPIO_FUNC_SIO);

    gpio_set_dir(status_blink_pin, GPIO_OUT);

    puts("Hello bootloader");

    blink_startup_status();
    blink_error_status();

    error_code = rom_load_partition_table(pt_workarea, pt_workarea_size, false);

    hard_assert(error_code == BOOTROM_OK);
    puts("Loaded partition table");

    uint32_t pt_info_data[64];

    int32_t pt_info_size = rom_get_partition_table_info(pt_info_data,
                                                        64,
                                                        PT_INFO_PT_INFO);

    // PT_INFO_PT_INFO should give 4 words, one for flags, one for partition count
    // and presence, two for unpartitioned space info
    hard_assert(pt_info_size == 4);
    puts("Got partition table info");

    // First word in buffer filled by get_partition_table_info always contains the subset
    // of flags given supported by the API. Should match what we pass
    hard_assert(pt_info_data[0] == PT_INFO_PT_INFO);

    uint32_t partition_count = pt_info_data[1] & 0xFF; // Low 8 bits partition count
    bool partition_table_present = pt_info_data[1] & 0x100; // Bit 8 contains flag for PT present

    printf("Partition count: %" PRIu32 "\n", partition_count);
    printf("Partition table present: %s\n", partition_table_present ? "yes" : "no");

    if (!partition_table_present) {
        puts("Device does not contain a partition table! Aborting");
        return;
    }

    if (partition_count != 2) {
        // Actually could probably put bootloader outside partition table and then
        // have program inside partition table?? Maybe useful for A/B????
        puts("Partition table does not contain 2 partitions. Aborting");
        return;
    }

    puts("------");

    // ASSUMPTION: Device contains two partitions:
    // Partition 0 contains the bootloader
    // Partition 1 contains the program to boot into

    partition_location_info part0_info = get_partition_location_info(0);
    partition_location_info part1_info = get_partition_location_info(1);

    printf("Partition 0 first sector: 0x%" PRIx32 "\n", part0_info.first_sector);
    printf("Partition 1 first sector: 0x%" PRIx32 "\n", part1_info.first_sector);

    uint32_t part0_start_addr = part0_info.first_sector * FLASH_SECTOR_SIZE;
    uint32_t part0_end_addr = (part0_info.last_sector + 1) * FLASH_SECTOR_SIZE;
    uint32_t part1_start_addr = part1_info.first_sector * FLASH_SECTOR_SIZE;
    uint32_t part1_end_addr = (part1_info.last_sector + 1) * FLASH_SECTOR_SIZE;

    printf("Partition 0 start: 0x%" PRIx32 "\n", part0_start_addr);
    printf("Partition 0 end: 0x%" PRIx32 "\n", part0_end_addr);
    printf("Partition 1 start: 0x%" PRIx32"\n", part1_start_addr);
    printf("Partition 1 end: 0x%" PRIx32 "\n", part1_end_addr);

    printf("Partition 0 nsboot R/W Perms: %d%d\n", part0_info.nsboot_readable, part0_info.nsboot_writable);
    printf("Partition 0 secure R/W Perms: %d%d\n", part0_info.secure_readable, part0_info.secure_writable);
    printf("Partition 0 non-secure R/W Perms: %d%d\n", part0_info.nonsecure_readable, part0_info.nonsecure_writable);

    printf("Partition 1 nsboot R/W Perms: %d%d\n", part1_info.nsboot_readable, part1_info.nsboot_writable);
    printf("Partition 1 secure R/W Perms: %d%d\n", part1_info.secure_readable, part1_info.secure_writable);
    printf("Partition 1 non-secure R/W Perms: %d%d\n", part1_info.nonsecure_readable, part1_info.nonsecure_writable);

    blink_error_status(BOOTROM_OK);
    blink_error_status(0);
}

// Also gets permission information
partition_location_info get_partition_location_info(int32_t part_number) {
    int32_t pt_info_flags = PT_INFO_SINGLE_PARTITION | PT_INFO_PARTITION_LOCATION_AND_FLAGS;
    int32_t pt_info_part_sel = part_number << 24; // Top byte of pt info flags contains partition number to select

    uint32_t pt_locflag_data[64];
    uint32_t pt_locflag_data_cap = sizeof pt_locflag_data / sizeof *pt_locflag_data;

    int32_t pt_locflag_data_len = rom_get_partition_table_info(pt_locflag_data,
                                              pt_locflag_data_cap,
                                              pt_info_flags | pt_info_part_sel);

    // Buffer should contain three words, one for flags supported, one for location, one for partition flags
    hard_assert(pt_locflag_data_len == 3);
    // Checking supported flags match (first word)
    hard_assert(pt_locflag_data[0] == pt_info_flags);

    uint32_t permloc_data = pt_locflag_data[1];

    partition_location_info locinfo = {
        (permloc_data & PICOBIN_PARTITION_PERMISSION_NSBOOT_W_BITS) != 0,
        (permloc_data & PICOBIN_PARTITION_PERMISSION_NSBOOT_R_BITS) != 0,
        (permloc_data & PICOBIN_PARTITION_PERMISSION_NS_W_BITS) != 0,
        (permloc_data & PICOBIN_PARTITION_PERMISSION_NS_R_BITS) != 0,
        (permloc_data & PICOBIN_PARTITION_PERMISSION_S_W_BITS) != 0,
        (permloc_data & PICOBIN_PARTITION_PERMISSION_S_R_BITS) != 0,
        (permloc_data & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB,
        (permloc_data & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB
    };

    return locinfo;
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
