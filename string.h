/*
 * Merkkijonofunktiot ja muistinkäsittely
 */
#ifndef _STRING_H_
#define _STRING_H_

#include "types.h"

#define NULL		0

void *memcpy(void *d, const void *src, size_t l);
void *memset(void *d, int ch, size_t l);
int memcmp(const void *a, const void *b, size_t l);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
void *memset16(void *d, uint16_t v, size_t l);
void *memcpy16(void *d, const void *src, size_t l);
char *strncpy(char *dst, const char *src, size_t l);
int strcmp(const char *a, const char *b);
int strcasecmp(const char *a, const char *b);
int strncasecmp(const char *a, const char *b, size_t l);
void number_str(char *buf, unsigned long val, unsigned int radix);
void format_number(char *buf, const char *format, unsigned long val,
		unsigned int radix);
int atoi(const char *s);

#endif
