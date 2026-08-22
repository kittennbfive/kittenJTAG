#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/cpufunc.h>

#include "avr_pinfuncs.h"
#include "channels.h"
#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

extern channeltype_t channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
extern uint8_t nb_channels; //declared in channels.c

typedef struct
{
	volatile uint8_t * ddr;
	volatile uint8_t * port;
	volatile uint8_t * pin;
	uint8_t bitmask;
} channelpin_t;

static const channelpin_t channel_to_avr_pin[NB_CHANNELS_AVAILABLE]={ {&DDRA, &PORTA, &PINA, (1<<0)}, {&DDRA, &PORTA, &PINA, (1<<1)}, {&DDRA, &PORTA, &PINA, (1<<2)}, {&DDRA, &PORTA, &PINA, (1<<3)}, {&DDRA, &PORTA, &PINA, (1<<4)}, {&DDRA, &PORTA, &PINA, (1<<5)}, {&DDRD, &PORTD, &PIND, (1<<6)}, {&DDRD, &PORTD, &PIND, (1<<7)}, {&DDRB, &PORTB, &PINB, (1<<0)}, {&DDRB, &PORTB, &PINB, (1<<1)}, {&DDRB, &PORTB, &PINB, (1<<2)}, {&DDRB, &PORTB, &PINB, (1<<3)}, {&DDRB, &PORTB, &PINB, (1<<4)}, {&DDRB, &PORTB, &PINB, (1<<5)}, {&DDRB, &PORTB, &PINB, (1<<6)}, {&DDRB, &PORTB, &PINB, (1<<7)}, {&DDRC, &PORTC, &PINC, (1<<0)}, {&DDRC, &PORTC, &PINC, (1<<1)}, {&DDRC, &PORTC, &PINC, (1<<2)}, {&DDRC, &PORTC, &PINC, (1<<3)}, {&DDRC, &PORTC, &PINC, (1<<4)}, {&DDRC, &PORTC, &PINC, (1<<5)}, {&DDRC, &PORTC, &PINC, (1<<6)}, {&DDRC, &PORTC, &PINC, (1<<7)} };

//always inline these for speed during search!

inline __attribute__((__always_inline__)) void set_avr_pin_input(const uint8_t ch, const bool pullup)
{
	if(pullup)
		(*channel_to_avr_pin[ch].port)|=channel_to_avr_pin[ch].bitmask;
	else
		(*channel_to_avr_pin[ch].port)&=~channel_to_avr_pin[ch].bitmask;
	(*channel_to_avr_pin[ch].ddr)&=~channel_to_avr_pin[ch].bitmask;
}

inline __attribute__((__always_inline__)) void set_avr_pin_output(const uint8_t ch, const bool value)
{
	(*channel_to_avr_pin[ch].ddr)|=channel_to_avr_pin[ch].bitmask;
	if(value)
		(*channel_to_avr_pin[ch].port)|=channel_to_avr_pin[ch].bitmask;
	else
		(*channel_to_avr_pin[ch].port)&=~channel_to_avr_pin[ch].bitmask;
}

inline __attribute__((__always_inline__)) bool read_avr_pin(const uint8_t ch)
{
	return !!((*channel_to_avr_pin[ch].pin)&channel_to_avr_pin[ch].bitmask);
}

void cmd_allinp(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	uint8_t i;
	for(i=0; i<NB_CHANNELS_AVAILABLE; i++)
		set_avr_pin_input(i, false);
	
	printf_P(PSTR("all channels set to input on AVR\n"));
}
