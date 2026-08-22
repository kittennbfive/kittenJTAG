#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <avr/pgmspace.h>

#include "channels.h"
#include "avr_pinfuncs.h"
#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

channeltype_t channels[NB_CHANNELS_AVAILABLE];
uint8_t nb_channels=0;

//NOTE TO SELF: CHANGE THIS BEFORE RELEASE!
#if DEFECTIVE_CHANNELS
const bool skip_defective_channels[NB_CHANNELS_AVAILABLE]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //from 0 to NB_CHANNELS_AVAILABLE-1, 1 means defective
#endif

//yes, global variables are "bad style" but we are really limited in FLASH memory and global vars need less space than "static" global vars and get()-functions everywhere
devicemode_t device_mode=MODE_NORMAL;
bool override_inputs=false;
bool override_unknown=false;
bool direct_clock_drive=false;

void reset_channels(void)
{
	uint8_t i;
	for(i=0; i<NB_CHANNELS_AVAILABLE; i++)
	{
		channels[i]=PIN_NOT_PROBED;
		set_avr_pin_input(i, false);
	}
	
	nb_channels=0;
	override_inputs=false;
	override_unknown=false;
	direct_clock_drive=false;
}

void cmd_reset(PROTOTYPE_ARGS_HANDLER) //0 args //reset everything
{
	ARGS_HANDLER_UNUSED;
	
	reset_channels();
	
	printf_P(PSTR("reset ok\n"));
}

void cmd_devmode(PROTOTYPE_ARGS_HANDLER)
{
	ARGS_HANDLER_UNUSED;
	
	char str_mode[SZ_BUFFER_ARGUMENTS];
	strcpy(str_mode, get_next_argument());
	
	if(!strcmp(str_mode, "normal"))
	{
		device_mode=MODE_NORMAL;
		printf_P(PSTR("device mode set to normal\n"));
	}
	else if(!strcmp(str_mode, "shift"))
	{
		device_mode=MODE_LEVELSHIFTER;
		printf_P(PSTR("device mode set to levelshifter mode, some functions are unavailable\n"));
	}
	else
		printf_P(PSTR("error: invalid mode\n"));
}

void cmd_channels(PROTOTYPE_ARGS_HANDLER) //1 arg //set number of channels
{
	ARGS_HANDLER_UNUSED;
	
	nb_channels=atoi(get_next_argument()); //if argument is NaN atoi() will return 0 but that's fine here, would be catched below
	
	if(nb_channels<2 || nb_channels>24)
	{
		printf_P(PSTR("error: invalid number of channels (2-24)\n"));
		nb_channels=0;
		return;
	}
	
	printf_P(PSTR("number of channels set to %u\n"), nb_channels);

#if DEFECTIVE_CHANNELS
	printf_P(PSTR("scanning for defective channels...\n")); 
	uint8_t ch;
	bool found=false;
	for(ch=0; ch<nb_channels; ch++)
	{
		if(skip_defective_channels[ch])
		{
			found=true;
			printf_P(PSTR("WARNING: channel %u marked as defective, will not be used!\n"), ch);
		}
	}
	if(!found)
		printf_P(PSTR("all used channels ok\n"));
#endif
}

void cmd_setmode(PROTOTYPE_ARGS_HANDLER) //2 args //set channel state / override automatically detected state
{
	ARGS_HANDLER_UNUSED;

	if(device_mode==MODE_LEVELSHIFTER)
	{
		printf_P(PSTR("error: command unavailable in levelshifter mode\n"));
		return;
	}
	
	char ch_str[SZ_BUFFER_ARGUMENTS];
	strcpy(ch_str, get_next_argument());
	
	uint8_t ch;
	
	//we need to make sur ch_str only contains digits; otherwise atoi() will return 0 and channel 0 will be modified by mistake
	uint8_t i;
	for(i=0; i<strlen(ch_str); i++)
	{
		if(!isdigit(ch_str[i]))
		{
			printf_P(PSTR("error: invalid channel (NaN)\n"));
			return;
		}
	}
	
	ch=atoi(ch_str);
	
	if(ch>23)
	{
		printf_P(PSTR("error: invalid channel (0-23)\n"));
		return;
	}
	
	char state[SZ_BUFFER_ARGUMENTS];
	strcpy(state, get_next_argument());
	
	if(!strcmp_P(state, PSTR("disabled")))
	{
		channels[ch]=PIN_DISABLED;
		printf_P(PSTR("channel %u disabled\n"), ch);
	}
	else if(!strcmp_P(state, PSTR("input")))
	{
		channels[ch]=PIN_INPUT_FLOATING;
		printf_P(PSTR("channel %u defined as floating input\n"), ch);
	}
	else if(!strcmp_P(state, PSTR("inputPU")))
	{
		channels[ch]=PIN_INPUT_PULLUP;
		printf_P(PSTR("channel %u defined as input with pullup\n"), ch);
	}
	else if(!strcmp_P(state, PSTR("inputPD")))
	{
		channels[ch]=PIN_INPUT_PULLDOWN;
		printf_P(PSTR("channel %u defined as input with pulldown\n"), ch);
	}
	else if(!strcmp_P(state, PSTR("output")))
	{
		channels[ch]=PIN_OUTPUT_OR_VCC_GND;
		printf_P(PSTR("channel %u defined as output\n"), ch);
	}
	else
		printf_P(PSTR("error: invalid state specified\n"));
}

void cmd_override(PROTOTYPE_ARGS_HANDLER) //2 args
{
	ARGS_HANDLER_UNUSED;
	
	if(device_mode==MODE_LEVELSHIFTER)
	{
		printf_P(PSTR("error: command unavailable in levelshifter mode\n"));
		return;
	}
	
	char type[SZ_BUFFER_ARGUMENTS];
	char yesno[SZ_BUFFER_ARGUMENTS];
	
	strcpy(type, get_next_argument());
	strcpy(yesno, get_next_argument());
	
	if(!strcmp_P(type, PSTR("inputs")))
	{
		if(!strcmp_P(yesno, PSTR("yes")))
		{
			override_inputs=true;
			printf_P(PSTR("Will override channels detected as inputs\n"));
		}
		else if(!strcmp_P(yesno, PSTR("no")))
		{
			override_inputs=false;
			printf_P(PSTR("override input channels set to false\n"));
		}
		else
			printf_P(PSTR("error: invalid argument (yes/no)\n"));
	}
	else if(!strcmp_P(type, PSTR("unknown")))
	{
		if(!strcmp_P(yesno, PSTR("yes")))
		{
			override_unknown=true;
			printf_P(PSTR("Will override channels that couldn't be identified.\nWARNING: This function is potentially dangerous!\n"));
		}
		else if(!strcmp_P(yesno, PSTR("no")))
		{
			override_unknown=false;
			printf_P(PSTR("override unknown channels set to false\n"));
		}
		else
			printf_P(PSTR("error: invalid argument (yes/no)\n"));
	}
	else
		printf_P(PSTR("error: invalid argument (inputs/unknown)\n"));
}

void cmd_direct_clock_drive(PROTOTYPE_ARGS_HANDLER) //1 arg
{
	ARGS_HANDLER_UNUSED;
	
	if(device_mode==MODE_LEVELSHIFTER)
	{
		//the output of the AVR could/would fight against the levelshifter output (unless the later can be set to and is in tristate mode)
		printf_P(PSTR("error: command unavailable in levelshifter mode\n"));
		return;
	}
	
	char yesno[SZ_BUFFER_ARGUMENTS];
	
	strcpy(yesno, get_next_argument());
	
	if(!strcmp_P(yesno, PSTR("yes")))
	{
		direct_clock_drive=true;
		printf_P(PSTR("Direct clock drive enabled - USE AT YOUR OWN RISK!\n"));
	}
	else if(!strcmp_P(yesno, PSTR("no")))
	{
		direct_clock_drive=false;
		printf_P(PSTR("Direct clock drive disabled\n"));
	}
	else
		printf_P(PSTR("error: invalid argument (yes/no)\n"));
}
