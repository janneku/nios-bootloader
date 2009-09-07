/*
 * Grafiikan apufunktiot
 */
#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

#include "types.h"

#define SCR_WIDTH	640
#define SCR_HEIGHT	400

#define WHITE		0xFFFF
#define BLACK		0

/*
 * Luo 16bit värin. (r,g,b) oltava välillä 0-255
 */
#define RGB(r, g, b)		\
	((b) >> 3 | (uint16_t)((g) >> 2) << 5 | (uint16_t)((r) >> 3) << 11)

/*
 * 16bit RGB värin apumakroja, r:g:b suhde 5:6:5
 */
#define CREATE_RGB(r, g, b)		\
	((b) | (uint16_t)(g) << 5 | (uint16_t)(r) << 11)

#define GET_R(c)		\
	((uint16_t)(c) >> 11)
#define GET_G(c)		\
	(((uint16_t)(c) >> 5) & 63)
#define GET_B(c)		\
	((uint16_t)(c) & 31)

struct IMAGE {
	uint16_t *pixels;
	unsigned int width, height;
};

struct PAL_IMAGE {
	uint8_t *pixels;
	const uint16_t *palette;
	unsigned int width, height;
};

extern struct IMAGE screen;

void plot(struct IMAGE *image, int x, int y, uint16_t c);
void fill_rect(struct IMAGE *image, int x, int y, unsigned int width,
	       unsigned int height, uint16_t c);
void draw_image(struct IMAGE *target, int x, int y, const struct IMAGE *image);
void draw_pal_image(struct IMAGE *target, int x, int y,
		    const struct PAL_IMAGE *image);
void copy_pal_image(struct IMAGE *target, int x, int y, unsigned int width,
                    unsigned int height, const struct PAL_IMAGE *image,
                    int ix, int iy);
void draw_pal_image_double(struct IMAGE *target, int x, int y,
			   const struct PAL_IMAGE *image);
void shade_palette(uint16_t *palette, const uint16_t *from, int shade);

void extract_image(struct PAL_IMAGE *image, const unsigned char *data);
void init_graphics(int highres);

#endif
