#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#define BAUD 115200
#include <util/setbaud.h>
#include <avr/pgmspace.h>

#include "74xx595.h"
#include "adc.h"
#include "channels.h"
#include "jtag.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!

Please read the fine manual!
*/

int uart_putchar(char c, FILE *stream);

#define SZ_UI_BUFFER 25

#define ECHO_INPUT 0

int main(void)
{
	//UART
	//respect write order!
	UBRRH=(UBRR_VALUE>>8)&0xff;
	UBRRL=UBRR_VALUE&0xff;
	UCSRB=(1<<TXEN)|(1<<RXEN);
	
	FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
	stdout = &uart_output;
	
	//ADC set to ADC7, make this configurable later for OCP DUT
	ADC_enable_CH7();
	
	//SW-SPI
	init_595s();
	
	init_find_jtag();
	
	reset_channels();
	
	printf_P(PSTR("\n\nThis is kittenJTAG version 3\nCopyright (c) 2022-2026 by kittennbfive\nAGPLv3+ and NO WARRANTY - USE AT YOUR OWN RISK!\nPlease read the fine manual.\n\n"));
	
	printf_P(PSTR("command? "));
	
	char cmd[SZ_UI_BUFFER];
	char rx;
	uint8_t pos=0;
	
	while(1)
	{
		while(!(UCSRA&(1<<RXC)));
		
		rx=UDR;
		if(rx=='\r' || rx=='\n')
		{
			cmd[pos]='\0';
#if ECHO_INPUT
			printf_P(PSTR("\n"));
#endif
			parse(cmd);
			pos=0;
			printf_P(PSTR("command? "));
		}
		else if(rx==8) //backspace
		{
			if(pos)
				pos--;
		}
		else if(pos<SZ_UI_BUFFER-1)
		{
			cmd[pos++]=rx;
#if ECHO_INPUT
			uart_putchar(rx, NULL);
#endif
		}
	}
		
	return 0;
}

int uart_putchar(char c, FILE *stream)
{
	(void)stream;
	
	while(!(UCSRA&(1<<UDRE)));
	UDR=c;
	
	return 0;
}
