#ifndef __JTAG_H__
#define __JTAG_H__
#include <stdint.h>

#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

//TODO: check these values, adjust if needed - beware of variable types (uint8_t mostly)

#define NB_CLK_PULSES_RESET 128 //specifications say 5 clock pulses for reset but there might be several chained TAP!
#define NB_CLK_PULSES_BYPASS_INSTR 128
#define CYCLES_SHIFT_DETECTION_PATTERN 10

#define MAX_INPUT_PINS_TO_WIGGLE 20 //do not change

#define NB_CLK_PULSES_MAX_GET_NB_TAPS 512

#define NB_CLK_PULSES_FILL_IR 32
#define NB_CLK_PULSES_MAX_GET_LENGTH_IR 32

//DR can be quite wide! - variable is uint16_t
#define NB_CLK_PULSES_FILL_DR 512
#define NB_CLK_PULSES_MAX_GET_LENGTH_DR 512

void init_find_jtag(void);

void find_jtag(const bool ignore_if_pullup_missing, const bool dontstop);

void get_nb_taps(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo);

void get_length_ir(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo);

void probe_dr_lengths(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo);

void cmd_jtag(PROTOTYPE_ARGS_HANDLER);
void cmd_taps(PROTOTYPE_ARGS_HANDLER);
void cmd_irlen(PROTOTYPE_ARGS_HANDLER);
void cmd_drlen(PROTOTYPE_ARGS_HANDLER);
void cmd_tdo_pullup(PROTOTYPE_ARGS_HANDLER);

#endif
