#ifndef __CHANNELS_H__
#define __CHANNELS_H__
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define NB_CHANNELS_AVAILABLE 24 //do not change for current hardware

//Workaround for crappy levelshifters with defective channels
//NOTE TO SELF: CHANGE THIS BEFORE RELEASE!
#define DEFECTIVE_CHANNELS 0

typedef enum
{
	MODE_NORMAL,
	MODE_LEVELSHIFTER
} devicemode_t;

typedef enum
{
	PIN_NOT_PROBED=0,
	PIN_IDENT_FAILED, //these pins are ignored but can be manually set to some type
	PIN_CHANGING_VOLTAGE, //ignore these
	PIN_DISABLED, //can be set manually
	PIN_INPUT_FLOATING,
	PIN_INPUT_PULLUP,
	PIN_INPUT_PULLDOWN,
	PIN_OUTPUT_OR_VCC_GND
} channeltype_t;

void reset_channels(void);

void cmd_reset(PROTOTYPE_ARGS_HANDLER);
void cmd_devmode(PROTOTYPE_ARGS_HANDLER);
void cmd_channels(PROTOTYPE_ARGS_HANDLER);
void cmd_setmode(PROTOTYPE_ARGS_HANDLER);
void cmd_override(PROTOTYPE_ARGS_HANDLER);
void cmd_direct_clock_drive(PROTOTYPE_ARGS_HANDLER);

#endif
