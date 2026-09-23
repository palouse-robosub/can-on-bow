// This is the main program going on the Guppy microcontrollers
//
// This code listens to id 0x101 for float values, echoes the can frame on id 0x102, and outputs analog output on pin 20
extern "C" {
#include "pico/stdlib.h"
}

#include "modules/board_bootloader.h"
#include <iostream>

int main()
{
    stdio_init_all();

    switch(BOARD_TYPE)
    {
        case -2:
            board_bootloader_loop();
        default: // test
            break;
    }
}