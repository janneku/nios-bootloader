/*
 * Merkkijonofunktiot ja muistink‰sittely
 */
#include "string.h"
#include "bug.h"

void *memcpy(void *d_, const void *src_, size_t l)
{
	uint8_t *d = d_;
	const uint8_t *src = src_;

	/* alignointi */
	while (((unsigned long)d & 3) && l) {
		*d++ = *src++;
		l--;
	}
	if (l == 0)
		return d_;

	if (((unsigned long)src & 3) == 0) {
		/* voidaan kopioida 32bit sanoja */
		while (l >= 4) {
			*(uint32_t *) d = *(const uint32_t *)src;
			d += 4;
			src += 4;
			l -= 4;
		}
	} else if (((unsigned long)src & 1) == 0) {
		/* voidaan kopioida 16bit sanoja */
		while (l >= 2) {
			*(uint16_t *) d = *(const uint16_t *)src;
			d += 2;
			src += 2;
			l -= 2;
		}
	}

	/* loppuosa */
	while (l--) {
		*d++ = *src++;
	}
	return d_;
}

void *memset(void *d_, int ch, size_t l)
{
	uint8_t *d = d_;
	unsigned int j;

	/* alignointi */
	while (((unsigned long)d & 3) && l) {
		*d++ = ch;
		l--;
	}
	if (l == 0)
		return d_;

	/* t‰ytet‰‰n 32bit sanoilla */
	uint8_t b = ch;
	uint32_t val = b | (b << 8) | (b << 16) | (b << 24);

	while (l >= 4 * 4) {
		for (j = 0; j < 4; ++j) {
			((uint32_t *) d)[j] = val;
		}
		d += 4 * 4;
		l -= 4 * 4;
	}
	while (l >= 4) {
		*(uint32_t *) d = val;
		d += 4;
		l -= 4;
	}

	/* loppuosa */
	while (l--) {
		*d++ = ch;
	}
	return d_;
}

int memcmp(const void *a, const void *b, size_t l)
{
	while (l--) {
		if (*(uint8_t *)a != *(uint8_t *)b)
			return 1;
		a++;
		b++;
	}
	return 0;
}

size_t strlen(const char *s)
{
	size_t l = 0;
	while (*s++)
		l++;
	return l;
}

char *strcpy(char *dst, const char *src)
{
	while (*src) {
		*dst++ = *src++;
	}
	*dst = 0;
	return NULL;
}

char *strcat(char *dst, const char *src)
{
	while (*dst)
		dst++;
	while (*src) {
		*dst++ = *src++;
	}
	*dst = 0;
	return NULL;
}

/*
 * K‰ytett‰v‰ framebufferin kirjoitukseen!
 */
void *memcpy16(void *d_, const void *src_, size_t l)
{
	uint16_t *d = d_;
	const uint16_t *src = src_;

	/* alignointi */
	if (((unsigned long)d & 3) && l) {
		*d++ = *src++;
		l--;
	}
	if (l == 0)
		return d_;

	if (((unsigned long)src & 3) == 0) {
		/* voidaan kopioida 32bit sanoja */
		while (l >= 2) {
			*(uint32_t *) d = *(const uint32_t *)src;
			d += 2;
			src += 2;
			l -= 2;
		}
	}

	/* loppuosa */
	while (l--) {
		*d++ = *src++;
	}
	return d_;
}

/*
 * K‰ytett‰v‰ framebufferin kirjoitukseen!
 */
void *memset16(void *d_, uint16_t v, size_t l)
{
	uint16_t *d = d_;
	unsigned int j;

	/* alignointi */
	if (((unsigned long)d & 3) && l) {
		*d++ = v;
		l--;
	}
	if (l == 0)
		return d_;

	/* t‰ytet‰‰n 32bit sanoilla */
	uint32_t val = v | (v << 16);

	while (l >= 2 * 4) {
		for (j = 0; j < 4; ++j) {
			((uint32_t *) d)[j] = val;
		}
		d += 2 * 4;
		l -= 2 * 4;
	}
	while (l >= 2) {
		*(uint32_t *) d = val;
		d += 2;
		l -= 2;
	}

	/* loppuosa */
	while (l--) {
		*d++ = v;
	}
	return d_;
}

char *strncpy(char *dst, const char *src, size_t l)
{
	while (*src && l) {
		*dst++ = *src++;
		l--;
	}
	while (l--) {
		*dst++ = 0;
	}
	return NULL;
}

int toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		return (uint8_t)c - 'a' + 'A';
	return c;
}

int tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
		return (uint8_t)c - 'A' + 'a';
	return c;
}

int strcmp(const char *a, const char *b)
{
	while (*a == *b) {
		if (!*a)
			return 0;
		a++;
		b++;
	}
	return 1;
}

int strcasecmp(const char *a, const char *b)
{
	while (toupper(*a) == toupper(*b)) {
		if (!*a)
			return 0;
		a++;
		b++;
	}
	return toupper(*a) - toupper(*b);
}

int strncasecmp(const char *a, const char *b, size_t l)
{
	while (l--) {
		if (toupper(*a) != toupper(*b))
			return 1;
		if (!*a)
			return 0;
		a++;
		b++;
	}
	return 0;
}

static const char digits[] = "0123456789abcdef";

void number_str(char *buf, unsigned long val, unsigned int radix)
{
	static char tmp[64];
	char *p = tmp;

#ifdef DEBUG
	if (radix >= sizeof(digits))
		bug();
#endif
	do {
		*p++ = digits[val % radix];
		val /= radix;
	} while (val);

	while (p > tmp)
		*buf++ = *--p;
	*buf = 0;
}

/*
 * v‰h‰n kuin C-kirjaston sprintf(), format voi sis‰lt‰‰ yhden %d tai %i.
 */
void format_number(char *buf, const char *format, unsigned long val,
		   unsigned int radix)
{
	static char tmp[64];
	char *p = tmp;

#ifdef DEBUG
	if (radix >= sizeof(digits))
		bug();
#endif
	while (*format && *format != '%') {
		*buf++ = *format++;
	}

	if (*format != '%') {
		*buf = 0;
		return;
	}
	format++;
	if (*format != 'd' && *format != 'i') {
		*buf = 0;
		return;
	}
	format++;

	do {
		*p++ = digits[val % radix];
		val /= radix;
	} while (val);

	while (p > tmp)
		*buf++ = *--p;

	while (*format) {
		*buf++ = *format++;
	}
	*buf = 0;
}

int atoi(const char *s)
{
	int val = 0;
	
	while (*s >= '0' && *s <= '9') {
		val *= 10;
		val += *s++ - '0';
	}
	return val;
}
