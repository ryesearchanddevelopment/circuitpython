// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2025 Cooper Dalrymple
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"

#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/fourwire/FourWire.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"
#include "shared-bindings/board/__init__.h"


uint8_t display_init_sequence[] = {
    /*
    0, 0xae, // display off
    0xd5, 0, 0x80, // set display clock div
    0, 0xd3, 0, 0x00, // set display offset
    0, 0x40, // set start line
    0, 0xa4, // display all on, resume
    0, 0xa6, // normal display
    0, 0x8d, 0, 0x14, // charge pump
    0, 0x20, 0, 0x00, // memory mode
    0, 0xa0, // segremap
    0, 0xc0, // com scan increment
    0, 0x81, 0, 0xff, // set contrast
    0, 0xd9, 0, 0xf1, // set precharge
    0, 0xd8, 0, 0x20, // set v com detect
    0, 0xa8, 0, 40-1, // set multiplex
    0, 0xda, 0, 0x12, // set com pins
    0, 0xad, 0, 0x30,
    0, 0xaf, // on
    */

    0xae, 0, // sleep
    0xd5, 1, 0x80, // fOsc divide by 2
    0xd3, 1, 0x00, // set display offset
    0x40, 1, 0x00, // set start line
    0xa4, 0, // display all on, resume
    0xa6, 0, // normal display
    0x8d, 1, 0x14, // charge pump
    0x20, 1, 0x00, // memory mode
    0xa0, 0, // segremap
    0xc0, 0, // com scan increment
    0x81, 1, 0xff, // set contrast
    0xd9, 1, 0xf1, // set precharge
    0xd8, 1, 0x20, // set v com detect
    0xa8, 1, 40-1, // set multiplex
    0xda, 1, 0x12, // set com pins
    0xad, 1, 0x30,
    0xaf, 0, // on
};

void board_init(void) {
    busio_spi_obj_t *spi = common_hal_board_create_spi(0);
    fourwire_fourwire_obj_t *bus = &allocate_display_bus()->fourwire_bus;
    bus->base.type = &fourwire_fourwire_type;
    common_hal_fourwire_fourwire_construct(bus,
        spi,
        CIRCUITPY_BOARD_OLED_DC, // Command or data
        CIRCUITPY_BOARD_OLED_CS, // Chip select
        CIRCUITPY_BOARD_OLED_RESET, // Reset
        10000000, // Baudrate
        0, // Polarity
        0); // Phase

    busdisplay_busdisplay_obj_t *display = &allocate_display()->display;
    display->base.type = &busdisplay_busdisplay_type;
    common_hal_busdisplay_busdisplay_construct(
        display,
        bus,
        72, // Width (after rotation)
        40, // Height (after rotation)
        28, // column start
        0, // row start
        0, // rotation
        1, // Color depth
        true, // grayscale
        false, // pixels in byte share row. only used for depth < 8
        1, // bytes per cell. Only valid for depths < 8
        false, // reverse_pixels_in_byte. Only valid for depths < 8
        true, // reverse_pixels_in_word
        0, // Set column command
        0, // Set row command
        0, // Write memory command
        display_init_sequence,
        sizeof(display_init_sequence),
        NULL,  // backlight pin
        0x81,
        1.0f, // brightness
        true, // single_byte_bounds
        true, // data_as_commands
        true, // auto_refresh
        60, // native_frames_per_second
        true, // backlight_on_high
        true, // SH1107_addressing
        50000); // backlight pwm frequency
}

void reset_board(void) {
}
