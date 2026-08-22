#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/cpufunc.h>

#include "4067.h"
#include "74xx595.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define PIN_4067_HALF_VCC 15 //do not change for current hardware!

static uint8_t bytes[2]={0x00, 0x03}; //both 4067 disabled

static inline void write_data(void)
{
	write_data_to_595s(bytes[0], bytes[1]);
}

static void set_enable(const uint8_t nb, const bool enable)
{
	if(enable)
		bytes[1]&=~(1<<(nb-1));
	else
		bytes[1]|=(1<<(nb-1));
}

static uint8_t reverse_bits4(const uint8_t v) //for second 4067, reversed for easier routing...
{
	return ((v&(1<<0))<<3)|((v&(1<<1))<<1)|((v&(1<<2))>>1)|((v&(1<<3))>>3);
}

static void set_pin(const uint8_t nb, const uint8_t pin)
{
	if(nb==1)
	{
		bytes[0]&=0xf0;
		bytes[0]|=(pin&0x0f);
	}
	else
	{
		bytes[0]&=0x0f;
		bytes[0]|=reverse_bits4(pin&0x0f)<<4;
	}
}

void select_channel_4067_activate(const uint8_t ch, const bool half_vcc)
{
	set_enable(1, false);
	set_enable(2, false);

	if(ch<=14)
	{
		set_pin(1, ch);
		set_enable(1, true);
		
		if(half_vcc)
		{
			set_pin(2, PIN_4067_HALF_VCC);
			set_enable(2, true);
		}
	}
	else
	{
		set_pin(2, ch-15);
		set_enable(2, true);
		
		if(half_vcc)
		{
			set_pin(1, PIN_4067_HALF_VCC);
			set_enable(1, true);
		}
	}
	
	write_data();
}
