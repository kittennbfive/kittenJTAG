#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/cpufunc.h>

#include "74xx595.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

//if you modify the hardware beware of hardcoded pins here!

static void clock_out_byte(uint8_t byte)
{
	uint8_t i;
	for(i=0; i<8; i++)
	{
		if(byte&(1<<7))
			PORTD|=(1<<PD2);
		else
			PORTD&=~(1<<PD2);
		byte<<=1;
		PORTD|=(1<<PD4);
		_NOP();
		PORTD&=~(1<<PD4);
	}
}

void write_data_to_595s(const uint8_t data0, const uint8_t data1)
{
	clock_out_byte(data1);
	clock_out_byte(data0);
	
	PORTD|=(1<<PD5);
	_NOP();
	PORTD&=~(1<<PD5);
}

void init_595s(void)
{
	DDRD|=(1<<PD2)|(1<<PD4)|(1<<PD5);
	write_data_to_595s(0x00, 0x03); //disable both 4067
}
