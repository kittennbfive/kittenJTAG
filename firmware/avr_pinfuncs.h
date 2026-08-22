#ifndef __AVR_PINFUNCS_H__
#define __AVR_PINFUNCS_H__
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

void set_avr_pin_input(const uint8_t ch, const bool pullup);
void set_avr_pin_output(const uint8_t ch, const bool value);
bool read_avr_pin(const uint8_t ch);

void cmd_allinp(PROTOTYPE_ARGS_HANDLER);

#endif
