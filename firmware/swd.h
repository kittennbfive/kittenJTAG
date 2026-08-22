#ifndef __SWD_H__
#define __SWD_H__

#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

//clock frequency is slow to "compensate" for "slow" rising/falling edges due to protection resistors. Sadly there is no public specification from ARM for clock timing / min frequency / max rise/fall time... With DCD (see README) and/or depending on the value of the protection resistors maybe we can decrease this value, to be tried.
#define SWD_CLK_DELAY _delay_us(100) //~5kHz

void find_swd(void);

void cmd_swd(PROTOTYPE_ARGS_HANDLER);

#endif
