#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/cpufunc.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "jtag.h"
#include "channels.h"
#include "avr_pinfuncs.h"
#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define LENGTH_PATTERN 8

static const bool detection_pattern[LENGTH_PATTERN]={1,0,0,1,1,1,1,0};

extern channeltype_t channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
extern uint8_t nb_channels; //declared in channels.c
extern devicemode_t device_mode; //declared in channels.c
extern bool override_inputs; //declared in channels.c
extern bool override_unknown; //declared in channels.c

typedef struct
{
	bool valid_data;
	uint8_t ch_tms;
	uint8_t ch_tdi;
	uint8_t ch_tck;
	uint8_t ch_tdo;
} jtag_pins_t;

static jtag_pins_t jtag_pins;

static uint8_t nb_taps=0;
static uint8_t irlen=0;
static bool tdo_pullup=false;

#if DEFECTIVE_CHANNELS
extern const bool skip_defective_channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
#endif

//TODO: add adjustable delay for slow DUT?
static void clock_pulses(const uint8_t tck, uint16_t nb)
{
	while(nb--)
	{
		set_avr_pin_output(tck, 1);
		_NOP();
		set_avr_pin_output(tck, 0);
		_NOP();
	}
}

static void send_tms_command(const uint8_t tms, const uint8_t tck, char const * const cmd)
{
	uint8_t i=0;
	while(cmd[i])
	{
		set_avr_pin_output(tms, cmd[i]-'0');
		clock_pulses(tck, 1);
		i++;
	}
	
	set_avr_pin_output(tms, 0);
}

static void send_ir(const uint8_t tdi, const uint8_t tms, const uint8_t tck, uint32_t instr, uint8_t length_ir)
{
	while(length_ir)
	{
		if(instr&1)
			set_avr_pin_output(tdi, 1);
		else
			set_avr_pin_output(tdi, 0);
		if(length_ir==1)
			set_avr_pin_output(tms, 1);
		else
			set_avr_pin_output(tms, 0);
		clock_pulses(tck, 1);
		instr>>=1;
		length_ir--;
	}
}

void init_find_jtag(void)
{
	jtag_pins.valid_data=false;
}

void find_jtag(const bool ignore_if_pullup_missing, const bool dontstop)
{
	uint8_t i,j;
	
	uint8_t tdi, tck, tms, tdo;
	
	uint8_t pos_tdo_pattern[NB_CHANNELS_AVAILABLE];
	memset(pos_tdo_pattern, 0, NB_CHANNELS_AVAILABLE*sizeof(uint8_t));
	
	bool found=false;
	
	nb_taps=0;
	irlen=0;
	jtag_pins.valid_data=false;
	
	printf_P(PSTR("searching...\n"));
	
	for(tdi=0; tdi<nb_channels; tdi++)
	{

#if DEFECTIVE_CHANNELS
		if(skip_defective_channels[tdi])
			continue;
#endif
		
		if(device_mode==MODE_NORMAL)
		{
			if(channels[tdi]!=PIN_INPUT_FLOATING && channels[tdi]!=PIN_INPUT_PULLUP && channels[tdi]!=PIN_INPUT_PULLDOWN && !(channels[tdi]==PIN_IDENT_FAILED && override_unknown))
				continue;
		
			if(ignore_if_pullup_missing && channels[tdi]!=PIN_INPUT_PULLUP)
				continue;
		}
		
		set_avr_pin_output(tdi, 1);
		
		for(tck=0; tck<nb_channels; tck++)
		{

#if DEFECTIVE_CHANNELS
			if(skip_defective_channels[tck])
				continue;
#endif

			if(tck==tdi)
				continue;
			
			if(device_mode==MODE_NORMAL)
			{
				if(channels[tck]!=PIN_INPUT_FLOATING && channels[tck]!=PIN_INPUT_PULLUP && channels[tck]!=PIN_INPUT_PULLDOWN && !(channels[tck]==PIN_IDENT_FAILED && override_unknown))
					continue;
			}
			
			set_avr_pin_output(tck, 0);
			
			for(tms=0; tms<nb_channels; tms++)
			{
#if DEFECTIVE_CHANNELS
				if(skip_defective_channels[tms])
					continue;
#endif
				
				if(tms==tck || tms==tdi)
					continue;
				
				if(device_mode==MODE_NORMAL)
				{
					if(channels[tms]!=PIN_INPUT_FLOATING && channels[tms]!=PIN_INPUT_PULLUP && channels[tms]!=PIN_INPUT_PULLDOWN && !(channels[tms]==PIN_IDENT_FAILED && override_unknown))
						continue;
				
					if(ignore_if_pullup_missing && channels[tms]!=PIN_INPUT_PULLUP)
						continue;
				}
				
				set_avr_pin_output(tms, 1);
				
				//reset
				clock_pulses(tck, NB_CLK_PULSES_RESET);
				
				//select IR
				send_tms_command(tms, tck, "01100");
				
				//send bypass-instr = all ones
				set_avr_pin_output(tdi, 1);
				clock_pulses(tck, NB_CLK_PULSES_BYPASS_INSTR);
				
				//go via update_ir to idle
				send_tms_command(tms, tck, "110");				
				
				//select DR
				send_tms_command(tms, tck, "100");
				
				for(tdo=0; tdo<nb_channels; tdo++)
					pos_tdo_pattern[tdo]=0;
				
				//shift detection pattern several times because there might be several TAP in the chain
				for(i=0; i<CYCLES_SHIFT_DETECTION_PATTERN; i++)
				{
					for(j=0; j<LENGTH_PATTERN; j++)
					{
						set_avr_pin_output(tdi, detection_pattern[j]);
						
						clock_pulses(tck, 1);
												
						for(tdo=0; tdo<nb_channels; tdo++)
						{
#if DEFECTIVE_CHANNELS
							if(skip_defective_channels[tdo])
								continue;
#endif
							
							if(tdo==tdi || tdo==tck || tdo==tms)
								continue;
							
							if(device_mode==MODE_NORMAL)
							{
								if(channels[tdo]!=PIN_OUTPUT_OR_VCC_GND && !override_inputs && !override_unknown)
									continue;
							}
							
							set_avr_pin_input(tdo, tdo_pullup);
							
							bool value_tdo=read_avr_pin(tdo);
							
							set_avr_pin_input(tdo, false);
							
							if(value_tdo==detection_pattern[pos_tdo_pattern[tdo]])
							{
								pos_tdo_pattern[tdo]++;
								if(pos_tdo_pattern[tdo]==LENGTH_PATTERN)
								{
									found=true;
									
									printf_P(PSTR("FOUND JTAG: tdi %u tck %u tms %u tdo %u\n"), tdi, tck, tms, tdo);

									jtag_pins.ch_tdi=tdi;
									jtag_pins.ch_tck=tck;
									jtag_pins.ch_tms=tms;
									jtag_pins.ch_tdo=tdo;
									jtag_pins.valid_data=true;
									
									for(i=0; i<nb_channels; i++)
										set_avr_pin_input(i, false);
									
									if(!dontstop)
										return;
								}
							}
							else
								pos_tdo_pattern[tdo]=0;
						}
					}
				}
				set_avr_pin_input(tms, false);
			}
			set_avr_pin_input(tck, false);
		}
		set_avr_pin_input(tdi, false);
	}
	
	if(!found)
		printf_P(PSTR("no JTAG-interface found\n"));
}

void get_nb_taps(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo)
{
	set_avr_pin_input(tdo, false);
	
	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);
	
	//select IR
	send_tms_command(tms, tck, "01100");
	
	//send bypass-instr = all ones
	set_avr_pin_output(tdi, 1);
	clock_pulses(tck, NB_CLK_PULSES_BYPASS_INSTR);
	
	//go via update_ir to idle
	send_tms_command(tms, tck, "110");
	
	//select DR
	send_tms_command(tms, tck, "100");
	
	//clock in a single 0
	set_avr_pin_output(tdi, 0);	
	clock_pulses(tck, 1);
	set_avr_pin_output(tdi, 1);
	
	//wait for this 0
	bool found=false;
	uint16_t i;
	bool pinlevel;
	for(i=0; i<NB_CLK_PULSES_MAX_GET_NB_TAPS; i++)
	{
		set_avr_pin_output(tck, 1);
		_NOP();
		pinlevel=read_avr_pin(tdo);
		set_avr_pin_output(tck, 0);
		_NOP();
		if(!pinlevel)
		{
			found=1;
			break;
		}
	}

	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);

	if(found)
		nb_taps=i+1;
	else
		nb_taps=0;
}

void get_length_ir(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo)
{
	set_avr_pin_input(tdo, false);
	
	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);
	
	//select IR
	send_tms_command(tms, tck, "01100");
	
	//fill the IR with 1
	set_avr_pin_output(tdi, 1);
	clock_pulses(tck, NB_CLK_PULSES_FILL_IR);

	//clock in a single 0
	set_avr_pin_output(tdi, 0);
	clock_pulses(tck, 1);
	
	//wait for this 0
	bool found=false;
	uint8_t i;
	bool pinlevel;
	for(i=0; i<NB_CLK_PULSES_MAX_GET_LENGTH_IR; i++)
	{
		set_avr_pin_output(tck, 1);
		_NOP();
		pinlevel=read_avr_pin(tdo);
		set_avr_pin_output(tck, 0);
		_NOP();
		if(!pinlevel)
		{
			found=1;
			break;
		}
	}
	
	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);

	if(found)
		irlen=i+1;
	else
		irlen=0;
}

void probe_dr_lengths(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo) //DANGEROUS! Read the manual!
{	
	set_avr_pin_input(tdo, false);
	
	//go to idle
	send_tms_command(tms, tck, "0");
	
	uint32_t instr;
	uint16_t i;
	bool found;
	
	for(instr=0x00; instr<(1U<<irlen); instr++)
	{
		printf_P(PSTR("instruction 0x%lx: "), instr);
		
		if(instr==(1U<<irlen)-1)
			printf_P(PSTR("last one, BYPASS, length DR should be 1: "));
		
		//select and write IR
		send_tms_command(tms, tck, "1100");
		send_ir(tdi, tms, tck, instr, irlen);
		
		//select DR
		send_tms_command(tms, tck, "1100");
		
		//fill the current DR with 1
		set_avr_pin_output(tdi, 1);
		clock_pulses(tck, NB_CLK_PULSES_FILL_DR);

		//clock in a single 0
		set_avr_pin_output(tdi, 0);
		clock_pulses(tck, 1);
		
		//wait for this 0
		found=false;
		bool pinlevel;
		for(i=0; i<NB_CLK_PULSES_MAX_GET_LENGTH_DR; i++)
		{
			set_avr_pin_output(tck, 1);
			_NOP();
			pinlevel=read_avr_pin(tdo);
			set_avr_pin_output(tck, 0);
			_NOP();
			if(!pinlevel)
			{
				printf_P(PSTR("length DR is %u\n"), i+1);
				found=true;
				break;
			}
		}
		
		if(!found)
			printf_P(PSTR("length DR not found, too big?\n"));

		//go back to idle
		send_tms_command(tms, tck, "110");
	}
}

void cmd_jtag(PROTOTYPE_ARGS_HANDLER) //0-2 args
{
	(void)cmd;
	
	if(nb_channels<4)
	{
		printf_P(PSTR("error: need at least 4 channels\n"));
		return;
	}
	
	if(device_mode==MODE_NORMAL)
	{
		uint8_t ch;
		bool found_output=false;
		for(ch=0; ch<nb_channels; ch++)
		{
			if(channels[ch]==PIN_NOT_PROBED)
			{
				printf_P(PSTR("error: at least one pin not probed / unknown type\n"));
				return;
			}
			
			if(channels[ch]==PIN_OUTPUT_OR_VCC_GND)
			{
				found_output=true;
				break;
			}
		}
		
		if(!found_output && !override_inputs)
		{
			printf_P(PSTR("error: no outputs on DUT detected, need at least one for TDO\n"));
			return;
		}
	}
	
	bool ignore_if_pullup_missing=false;
	bool dontstop=false;
	
	char arg[SZ_BUFFER_ARGUMENTS];
	
	uint8_t nb=nb_args;
	
	while(nb--)
	{
		strcpy(arg, get_next_argument());
		if(!strcmp_P(arg, PSTR("ignore")))
		{
			if(device_mode==MODE_NORMAL)
			{
				printf_P(PSTR("will ignore inputs without needed pullup\n"));
				ignore_if_pullup_missing=true;
			}
			else
			{
				printf_P(PSTR("warning: \"ignore\" unsupported in levelshifter mode\n"));
			}
		}
		else if(!strcmp_P(arg, PSTR("dontstop")))
		{
			printf_P(PSTR("won't stop once JTAG found\n"));
			dontstop=true;
		}
		else
		{
			printf_P(PSTR("error: invalid argument\n"));
			return;
		}
	}
	
	find_jtag(ignore_if_pullup_missing, dontstop);
}

void cmd_taps(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(!jtag_pins.valid_data)
	{
		printf_P(PSTR("error: no JTAG-interface known\n"));
		return;
	}
	
	get_nb_taps(jtag_pins.ch_tms, jtag_pins.ch_tdi, jtag_pins.ch_tck, jtag_pins.ch_tdo);
	
	printf_P(PSTR("found %u TAP(s)\n"), nb_taps);
}

void cmd_irlen(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(!jtag_pins.valid_data)
	{
		printf_P(PSTR("error: no JTAG-interface known\n"));
		return;
	}
	
	if(nb_taps!=1)
	{
		printf_P(PSTR("error: irlen will only work if there is a single TAP\n"));
		return;
	}
	
	get_length_ir(jtag_pins.ch_tms, jtag_pins.ch_tdi, jtag_pins.ch_tck, jtag_pins.ch_tdo);
	
	if(irlen)
		printf_P(PSTR("length of IR is %u bits\n"), irlen);
	else
		printf_P(PSTR("could not find length of IR, too big?\n"));
}

void cmd_drlen(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(!jtag_pins.valid_data)
	{
		printf_P(PSTR("error: no JTAG-interface known\n"));
		return;
	}
	
	if(nb_taps!=1)
	{
		printf_P(PSTR("error: irlen will only work if there is a single TAP\n"));
		return;
	}
	
	if(irlen==0)
	{
		printf_P(PSTR("error: length of IR is not known\n"));
		return;
	}
	
	probe_dr_lengths(jtag_pins.ch_tms, jtag_pins.ch_tdi, jtag_pins.ch_tck, jtag_pins.ch_tdo);
}


void cmd_tdo_pullup(PROTOTYPE_ARGS_HANDLER) //1 arg
{
	ARGS_HANDLER_UNUSED;
	
	if(device_mode==MODE_LEVELSHIFTER)
	{
		printf_P(PSTR("error: command unavailable in levelshifter mode\n"));
		return;
	}
	
	char yesno[SZ_BUFFER_ARGUMENTS];
	
	strcpy(yesno, get_next_argument());
	
	if(!strcmp_P(yesno, PSTR("yes")))
	{
		tdo_pullup=true;
		printf_P(PSTR("Pullup on TDO enabled\n"));
	}
	else if(!strcmp_P(yesno, PSTR("no")))
	{
		tdo_pullup=false;
		printf_P(PSTR("Pullup on TDO disabled\n"));
	}
	else
		printf_P(PSTR("error: invalid argument (yes/no)\n"));
}
