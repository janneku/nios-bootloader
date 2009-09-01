/*
 * JTAG UART ajuri
 */
#ifndef _JTAG_H_
#define _JTAG_H_

#include "types.h"

int putchar(int ch);
int print_hex(unsigned long ul, unsigned int width);
int put_string(const char *s);

#endif
