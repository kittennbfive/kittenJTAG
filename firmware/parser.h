#ifndef __PARSER_H__
#define __PARSER_H__
#include <stdint.h>

/*
This file is part of kittenJTAG version 3, previously known as curious-kitten-JTAG.

Copyright (c) 2022-2026 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

//should be fine
#define SZ_BUFFER_COMMAND 15
#define SZ_BUFFER_ARGUMENTS 15
#define NB_ARGUMENTS_MAX 3

#define PROTOTYPE_ARGS_HANDLER char const * const cmd, const uint8_t nb_args

#define ARGS_HANDLER_UNUSED (void)cmd; (void)nb_args

void parse(char * const inp);

char * get_next_argument(void);
char * peek_next_argument(void);
uint8_t get_number_remaining_args(void);

#endif
