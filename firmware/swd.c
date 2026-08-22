#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "swd.h"
#include "channels.h"
#include "avr_pinfuncs.h"
#include "parser.h"
#include "adc.h"
#include "4067.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

extern channeltype_t channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
extern uint8_t nb_channels; //declared in channels.c
extern devicemode_t device_mode; //declared in channels.c
extern bool direct_clock_drive; //declared in channels.c

#if DEFECTIVE_CHANNELS
extern const bool skip_defective_channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
#endif

static void clk_pulse(const uint8_t clk)
{
	if(direct_clock_drive)
	{
		SWD_CLK_DELAY;
		PORTA|=(1<<PA7);
		SWD_CLK_DELAY;
		PORTA&=~(1<<PA7);
	}
	else
	{
		SWD_CLK_DELAY;
		set_avr_pin_output(clk, 1);
		SWD_CLK_DELAY;
		set_avr_pin_output(clk, 0);
	}
}

static void set_all_io(const bool value, const uint8_t clk_channel)
{
	uint8_t io;
	
	for(io=0; io<nb_channels; io++)
	{
#if DEFECTIVE_CHANNELS
		if(skip_defective_channels[io])
			continue;
#endif

		if(io==clk_channel)
			continue;

		if(device_mode==MODE_NORMAL)
		{
			if(channels[io]!=PIN_INPUT_FLOATING && channels[io]!=PIN_INPUT_PULLUP && channels[io]!=PIN_INPUT_PULLDOWN)
				continue;
		}
		
		set_avr_pin_output(io, value);
	}
}

static void shift_out_u8_all_io(uint8_t value, const uint8_t clk_channel)
{
	uint8_t i;
	for(i=0; i<8; i++)
	{
		set_all_io(value&1, clk_channel);
		clk_pulse(clk_channel);
		value>>=1;
	}
}

static void shift_out_u16_all_io(const uint16_t value, const uint8_t clk_channel)
{
	shift_out_u8_all_io(value&0xff, clk_channel);
	shift_out_u8_all_io((value>>8)&0xff, clk_channel);
}

static void set_avr_pins_io_input(const uint8_t clk_channel)
{
	uint8_t io;
	
	for(io=0; io<nb_channels; io++)
	{
#if DEFECTIVE_CHANNELS
		if(skip_defective_channels[io])
			continue;
#endif

		if(io==clk_channel)
			continue;

		if(device_mode==MODE_NORMAL)
		{
			if(channels[io]==PIN_IDENT_FAILED || channels[io]==PIN_OUTPUT_OR_VCC_GND)
			continue;
		}
		
		set_avr_pin_input(io, false);
	}
}

typedef struct
{
	bool is_valid_ack;
	uint32_t idcode;
	bool idcode_parity;
} io_data_t;

static bool check_idcode_parity(uint32_t idcode, const bool parity)
{
	bool p=0;
	
	uint8_t i;
	for(i=0; i<32; i++)
	{
		p^=(idcode&1);
		idcode>>=1;
	}
	
	return (p==parity);
}

void find_swd(void)
{
	uint8_t i;
	
	uint8_t clk,io;
	
	io_data_t io_data[NB_CHANNELS_AVAILABLE];
	for(i=0; i<NB_CHANNELS_AVAILABLE; i++)
	{
		io_data[i].is_valid_ack=true;
		io_data[i].idcode=0;
		io_data[i].idcode_parity=0;
	}
	
	const bool valid_ack_val[3]={1, 0, 0}; //do not change! by SWD specification!
	
	bool found=false;
	
	printf_P(PSTR("searching...\n"));
	
	if(direct_clock_drive)
	{
		printf_P(PSTR("Using direct clock drive via 4067\n"));
		ADC_disable();
		PORTA&=~(1<<PA7);
		DDRA|=(1<<PA7);
	}
	
	for(clk=0; clk<nb_channels; clk++)
	{
#if DEFECTIVE_CHANNELS
		if(skip_defective_channels[clk])
			continue;
#endif

		if(device_mode==MODE_NORMAL)
		{
			if(channels[clk]!=PIN_INPUT_FLOATING && channels[clk]!=PIN_INPUT_PULLUP && channels[clk]!=PIN_INPUT_PULLDOWN)
				continue;
		}
		
		if(direct_clock_drive)
			select_channel_4067_activate(clk, false);
		
		//reset (min 50 pulses)
		set_all_io(1, clk);
		for(i=0; i<60; i++)
			clk_pulse(clk);
		
		//switch to SWD
		shift_out_u16_all_io(0xe79e, clk);
		
		//reset again (min 50 pulses)
		set_all_io(1, clk);
		for(i=0; i<60; i++)
			clk_pulse(clk);
		
		//idle cycles (min 2? some sources say 12? better safe than sorry...)
		set_all_io(0, clk);
		for(i=0; i<16; i++)
			clk_pulse(clk);
		
		//read IDCODE command
		shift_out_u8_all_io(0xa5, clk);
		
		//turnaround
		set_avr_pins_io_input(clk);
		clk_pulse(clk);
		
		//read ACK and check if OK (0b001, LSb first) somewhere
		for(i=0; i<3; i++)
		{			
			for(io=0; io<nb_channels; io++)
			{
#if DEFECTIVE_CHANNELS
				if(skip_defective_channels[io])
					continue;
#endif

				if(io==clk)
					continue;
				
				if(!io_data[io].is_valid_ack)
					continue;
				
				if(read_avr_pin(io)!=valid_ack_val[i])
					io_data[io].is_valid_ack=false;
			}
			
			clk_pulse(clk);

		}
		
		//read actual IDCODE value
		for(i=0; i<32; i++)
		{
			for(io=0; io<nb_channels; io++)
			{
#if DEFECTIVE_CHANNELS
				if(skip_defective_channels[io])
					continue;
#endif

				if(io==clk)
					continue;
				
				if(!io_data[io].is_valid_ack)
					continue;
				
				io_data[io].idcode>>=1;
				io_data[io].idcode|=((uint32_t)read_avr_pin(io))<<31;
			}
			
			clk_pulse(clk);
		}
		
		//read IDCODE parity
		for(io=0; io<nb_channels; io++)
		{
#if DEFECTIVE_CHANNELS
			if(skip_defective_channels[io])
				continue;
#endif

			if(io==clk)
				continue;
			
			if(!io_data[io].is_valid_ack)
				continue;
			
			io_data[io].idcode_parity=read_avr_pin(io);
		}
		
		clk_pulse(clk);
		
		//prepare for next round
		set_avr_pin_input(clk, false);
		
		for(io=0; io<nb_channels; io++)
		{
#if DEFECTIVE_CHANNELS
			if(skip_defective_channels[io])
				continue;
#endif
			
			if(io==clk)
				continue;

			if(io_data[io].is_valid_ack && check_idcode_parity(io_data[io].idcode, io_data[io].idcode_parity))
			{
				printf_P(PSTR("FOUND SWD: clk %u io %u; IDCODE 0x%08lx\n"), clk, io, io_data[io].idcode);
				found=true;
				//do not break here, print all found SWD interfaces and IDCODE to let human user filter for false positives
			}
		}
	}
	
	if(direct_clock_drive)
	{
		DDRA&=~(1<<PA7);
		ADC_enable_CH7();
	}
	
	if(!found)
		printf_P(PSTR("no SWD-interface found\n"));
}

void cmd_swd(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(nb_channels==0)
	{
		printf_P(PSTR("error: number of channels is 0\n"));
		return;
	}
	
	uint8_t i;
	for(i=0; i<nb_channels; i++)
	{
		if(channels[i]==PIN_NOT_PROBED)
		{
			printf_P(PSTR("error: at least one pin not probed / unknown type\n"));
			return;
		}
	}
	
	find_swd();
}
