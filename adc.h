#ifndef __ADC_H__
#define __ADC_H__
#include <stdint.h>

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define ADC_MAX 1023 //do not change

void ADC_enable_CH7(void);
void ADC_disable(void);
uint16_t ADC_sample(void);

#endif
