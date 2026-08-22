#ifndef __PINDIR_IDENT_H__
#define __PINDIR_IDENT_H__

#include "parser.h"

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

//limits for automated IO-identification - tweak these if needed
#define PIN_DIR_DETECT_DELAY_MS 1 //delay in millseconds between samples for detection of changing voltages on pin
#define PIN_DIR_DETECT_NB_SAMPLES 3 //nb of samples for detection of changing voltages on pin
#define PIN_DIR_DETECT_DELTA_MAX 47 //max deviation (raw ADC-value) allowed for samples to not be classified as changing (analog or whatever) voltage (0,15V @3,3V Vcc; 0,23V @5V Vcc)
#define PIN_DIR_DETECT_HALF_VCC_DELTA_MAX 16 //max deviation (raw ADC-value) around Vcc/2 (ADC_MAX/2) to classify pin as input (0,05V @3,3V Vcc; 0,08V @5V Vcc)
#define PIN_DIR_DETECT_VCC_ADC_MIN 993 //min raw ADC-value to classify pin as output high or Vcc (3,2V @3,3V Vcc; 4,84V @5V Vcc)
#define PIN_DIR_DETECT_GND_ADC_MAX 32 //max raw ADC-value to classifiy pin as output low or GND (0,1V @3,3V Vcc; 0,15V @5V Vcc)
#define PIN_DIR_DETECT_PULLUP_ADC_MIN 1016 //min raw ADC-value for avrpin=1 to classify pin as input_pullup (3,274V @3,3V Vcc; 4,96V @5V Vcc)
#define PIN_DIR_DETECT_PULLDOWN_ADC_MAX 16 //max raw ADC-value for avrpin=0 to classify pin as input_pulldown (0,05V @3,3V Vcc; 0,08V @5V Vcc)

void identify_pin_directions(void);

void print_pin_summary(void);

void cmd_ident(PROTOTYPE_ARGS_HANDLER);
void cmd_chstate(PROTOTYPE_ARGS_HANDLER);

#endif
