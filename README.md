<h1 align="center">CANonBow</h1>

## Setup

### Build Environment
To acquire the necessary build dependencies you can use the Dev Container or Nix.

### Using the Dev Container:
1. Install an IDE which supports Dev Containers (VSCode, CLion, etc.)
2. Install `picotool` from your package manager or [the repository](https://github.com/raspberrypi/picotool)
3. Build and reopen the project within the Dev Container. (IDE specific)

### Using Nix:
1. [Install Nix](https://nixos.org/download)
2. Run `nix-shell` to enter an environment with dependencies and `picotool` installed

### Building:

1. Make and enter a build dir: `mkdir build` and `cd build`
2. Generate build files: `cmake ..`
3. Compile: `make`

## Partition Tables

There are three ways the bootrom can boot from flash.

 - Flash Image Boot
 - Flash Partition Boot
 - Partition-table-in-image Boot

When the bootrom attempts to boot from flash, it chooses one of these three methods depending on the contents
of the flash. When programming the flash, the data ends up in blocks linked together into a block loop. Blocks
can be either images or partition tables. Images store code and data. Partition tables divide the flash into
different regions.

### Flash Boot Methods

#### Flash Image Boot

If the bootrom finds a block loop containing an image but no partition table, it boots the image. This is the
type of boot performed when using guppy-embedded without CANonBow (ie. just loading one image block and not
partitioning flash).

#### Flash Partition Boot

If the bootrom finds a block loop containing a partition table but no image, it searches for a bootable partition
inside the partition table and boots that. This is performed when using CANonBow.

#### Partition-Table-in-Image Boot

If the bootrom finds a block loop containing a partition and ALSO an image, it loads the partition table but boots
the image located outside the partition table. In this case, the partition table is considered embedded into the image.
This method is not used currently, but the pico-sdk provides the CMake function `pico_embed_pt_in_binary`, which could
potentially simplify the build and flashing process.

### CANonBow Partition Layout

Currently, when using CANonBow, the flash is partitioned into two sections. The first partition contains the bootloader,
CANonBow. The second partition contains a program to boot into. CANonBow doesn't support A/B versions or any
of the fancy features that partition tables allow yet. 

### Why Use Partition Tables?

Using partition tables allows for flashing into the boot partition using picotool almost exactly as you would
when not using CANonBow. The bootrom also has features such as A/B versioning which require partition tables.
The bootrom API also exposes an interface to easily work with partition tables and their contents.

### Creating Partition Tables

Partition tables are described using a JSON file, the partition layout for CANonBow is in `partition_table.json`. The
[picotool readme](https://github.com/raspberrypi/picotool/blob/master/README.md) contains another example under the
documentation for the `uf2 combine` command.

A UF2 containing the partition layout is created from this description, and then flashed onto the RP2350. The UF2
can be created using `picotool partition create`:

```sh
picotool partition create partition_table.json partition_table.uf2
```

## Flashing

When flashing, first plug the board into your computer. Then, flash the device using `picotool load` with the `-f` flag to force
the device into BOOTSEL mode. Note, flashing the board by treating it as a storage device _cannot be done_ when
using partition tables, use picotool instead.

### Manually Entering BOOTSEL Mode

As a fallback, the RP2350 can manually be put into BOOTSEL mode using the BOOT and RST buttons. Hold down both RST and BOOT, then
release RST and BOOT, in that order. If BOOT is released before RST, the board will not enter BOOTSEL mode. 

### CANonBow Flash Steps

While using CANonBow, the RP2350 needs to be flashed in three steps.

#### Flashing the Partition Table

First the partition table needs to be flashed:
```sh
picotool load -f partition_table.uf2
```

#### Flashing the Bootloader

Next flash the CANonBow into the first partition. The partition loaded into is selected using the `-p` flag.
```sh
picotool load -f -p 0 board_bootloader.uf2
```

#### Flashing Guppy Embedded

The program partition can be flashed using picotool as well:
```sh
picotool load -f -p 1 board_[board_name].uf2
```

Note that currently the bootloader will always overwrite this with a hello world program stored inside the bootloader.

### Overwriting the Partition Table

If you want to stop using CANonBow, you'll need to overwrite the partition table. This can be done by erasing
the flash before flashing, or using the `--ignore-partitions` option while flashing.

```sh
picotool erase -f -a
picotool load -f board_[board_name].uf2
```
or
```sh
picotool load -f --ignore-partitions board_[board_name].uf2
```

## Resources

The [RP2350 datasheet](https://pip-assets.raspberrypi.com/categories/1214-rp2350/documents/RP-008373-DS-3-rp2350-datasheet.pdf) was the primary resource used while developing this bootloader. These sections are particularly relevant:

| Section | Title |
| :--- | :--- |
| 5.1. | Bootrom Concepts |
| 5.1.2. | Partition Tables |
| 5.1.4. | Image Definitions |
| 5.1.5. | Blocks and Block Loops |
| 5.1.12. | Flash Image Boot |
| 5.1.13. | Flash Partition Boot |
| 5.1.14. | Partition-Table-in-Image Boot |
| 5.1.19. | Address Translation |
| 5.2. | Processor-Controlled Boot Sequence |
| 5.2.7. | Flash Boot |
| 5.4.6. | Categorized List of API Functions and ROM Data |
| 5.4.6.1. | Low-Level Flash Access |
| 5.4.6.2. | High-Level Flash Access |
| 5.4.6.4. | Partition Tables |
| 5.4.8. | API Function Listings |
| 7.3. | Chip-Level Resets |
| 12.9. | Watchdog |

### Additional Resources

- The [Katapult](https://github.com/Arksine/katapult) bootloader
- [Picotool](https://github.com/raspberrypi/picotool/tree/master)
- [Pico-SDK](https://github.com/raspberrypi/pico-sdk/tree/master)
- [RP2350 Bootrom](https://github.com/raspberrypi/pico-bootrom-rp2350)
