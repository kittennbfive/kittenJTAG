#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "pindir_ident.h"
#include "4067.h"
#include "adc.h"
#include "avr_pinfuncs.h"
#include "channels.h"
#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define DEBUG_OUTPUT 0

extern channeltype_t channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
extern uint8_t nb_channels; //declared in channels.c
extern devicemode_t device_mode; //declared in channels.c
extern bool override_inputs; //declared in channels.c
extern bool override_unknown; //declared in channels.c

//really basic algorithm but we only have a few samples to sort
static void sort_u16(uint16_t * const samples, const uint8_t nb_samples)
{
	uint16_t temp;
	uint8_t i=0;
	while(i<nb_samples-1)
	{
		if(samples[i]>samples[i+1])
		{
			temp=samples[i];
			samples[i]=samples[i+1];
			samples[i+1]=temp;
			i=0;
		}
		else
			i++;
	}
}

void identify_pin_directions(void)
{
	uint8_t ch;
	uint8_t i;
	uint16_t samples[PIN_DIR_DETECT_NB_SAMPLES];
	uint16_t sample_low, sample_high;
	for(ch=0; ch<nb_channels; ch++)
	{
		set_avr_pin_input(ch, false);
		
		select_channel_4067_activate(ch, true);
		_delay_ms(PIN_DIR_DETECT_DELAY_MS); //this seems to be needed!?
		
		for(i=0; i<PIN_DIR_DETECT_NB_SAMPLES; i++)
		{
			samples[i]=ADC_sample();
			_delay_ms(PIN_DIR_DETECT_DELAY_MS);
		}
#if DEBUG_OUTPUT
		printf_P(PSTR("ch %u samples %u %u %u\n"), ch, samples[0], samples[1], samples[2]);
#endif
		select_channel_4067_activate(ch, false);
		
		set_avr_pin_input(ch, false);
		
		sort_u16(samples, PIN_DIR_DETECT_NB_SAMPLES); //low to high
		
		if(samples[PIN_DIR_DETECT_NB_SAMPLES-1]-samples[0]>PIN_DIR_DETECT_DELTA_MAX) //to much deviation?
		{
#if DEBUG_OUTPUT
			printf_P(PSTR("ch %u changing voltage, ignoring\n"), ch);
#endif
			channels[ch]=PIN_CHANGING_VOLTAGE;
			continue;
		}
		else if(samples[0]>ADC_MAX/2-PIN_DIR_DETECT_HALF_VCC_DELTA_MAX && samples[PIN_DIR_DETECT_NB_SAMPLES-1]<ADC_MAX/2+PIN_DIR_DETECT_HALF_VCC_DELTA_MAX) //all samples near Vcc/2?
		{
#if DEBUG_OUTPUT
			printf_P(PSTR("ch %u floating input\n"), ch);
#endif
			channels[ch]=PIN_INPUT_FLOATING;
		}
		else
		{
			set_avr_pin_output(ch, 0);
			sample_low=ADC_sample();
			set_avr_pin_output(ch, 1);
			sample_high=ADC_sample();
			set_avr_pin_input(ch, false);
			
#if DEBUG_OUTPUT
			printf_P(PSTR("ch %u sample_low %u sample_high %u\n"), ch, sample_low, sample_high);
#endif
			
			if(sample_high>=sample_low && sample_low>PIN_DIR_DETECT_VCC_ADC_MIN)
			{
#if DEBUG_OUTPUT
				printf_P(PSTR("-->ch %u output high or VCC\n"), ch);
#endif
				channels[ch]=PIN_OUTPUT_OR_VCC_GND;
			}
			else if(sample_high<=sample_low && sample_low<PIN_DIR_DETECT_GND_ADC_MAX)
			{
#if DEBUG_OUTPUT
				printf_P(PSTR("-->ch %u output low or GND\n"), ch);
#endif
				channels[ch]=PIN_OUTPUT_OR_VCC_GND;
			}
			else if(sample_high>PIN_DIR_DETECT_PULLUP_ADC_MIN)
			{
#if DEBUG_OUTPUT
				printf_P(PSTR("-->ch %u input_pullup\n"), ch);
#endif
				channels[ch]=PIN_INPUT_PULLUP;
			}
			else if(sample_low<PIN_DIR_DETECT_PULLDOWN_ADC_MAX)
			{
#if DEBUG_OUTPUT
				printf_P(PSTR("-->ch %u input_pulldown\n"), ch);
#endif
				channels[ch]=PIN_INPUT_PULLDOWN;
			}
			else
			{
#if DEBUG_OUTPUT
				printf_P(PSTR("WARNING: could not identify ch %u\n"), ch);
#endif
				channels[ch]=PIN_IDENT_FAILED;
			}
		}
		
	}
}

void print_pin_summary(void)
{
	if(device_mode==MODE_NORMAL)
	{
		printf_P(PSTR("channel state:\n"));
		
		uint8_t i;
		for(i=0; i<NB_CHANNELS_AVAILABLE; i++)
			printf_P(PSTR("%-2u "), i);
		printf_P(PSTR("\n"));
		for(i=0; i<NB_CHANNELS_AVAILABLE; i++)
		{
			switch(channels[i])
			{
				case PIN_NOT_PROBED: printf_P(PSTR("--")); break;
				case PIN_IDENT_FAILED: printf_P(PSTR("??")); break;
				case PIN_CHANGING_VOLTAGE: printf_P(PSTR("a ")); break;
				case PIN_DISABLED: printf_P(PSTR("d ")); break;
				case PIN_INPUT_FLOATING: printf_P(PSTR("I ")); break;
				case PIN_INPUT_PULLUP: printf_P(PSTR("PU")); break;
				case PIN_INPUT_PULLDOWN: printf_P(PSTR("PD")); break;
				case PIN_OUTPUT_OR_VCC_GND: printf_P(PSTR("O ")); break;
			}
			printf_P(PSTR(" "));
		}
		if(override_inputs)
			printf_P(PSTR("\nChannels detected as inputs can be treated as outputs.\n"));
		else
			printf_P(PSTR("\nChannels detected as inputs will always be treated as inputs.\n"));
		if(override_unknown)
			printf_P(PSTR("Channels with unidentified direction can be overridden. Potentially dangerous!\n"));
		else
			printf_P(PSTR("Channels with unidentified direction will not be overriden.\n"));
	}
	else
		printf_P(PSTR("levelshifter mode - configured for %u channels\n"), nb_channels); 
}

void cmd_ident(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(device_mode==MODE_LEVELSHIFTER)
	{
		printf_P(PSTR("error: command unavailable in levelshifter mode\n"));
		return;
	}
	
	if(nb_channels==0)
	{
		printf_P(PSTR("error: number of channels is 0\n"));
		return;
	}
	
	identify_pin_directions();
	print_pin_summary();
}

void cmd_chstate(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	print_pin_summary();	
}

