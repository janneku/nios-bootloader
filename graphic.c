/*
 * Grafiikan apufunktiot
 */
#include "graphic.h"
#include "string.h"
#include "utils.h"
#include "bug.h"
#include "io.h"
#include "def.h"
#include "system.h"

struct IMAGE screen;

void plot(struct IMAGE *image, int x, int y, uint16_t c)
{
#ifdef DEBUG
	if (x < 0 || y < 0 || x >= (int)image->width || y >= (int)image->height)
		bug();
#endif
	image->pixels[x + y * image->width] = c;
}

/*
 * T‰ytt‰‰ suorakulmion annetulla v‰rill‰
 */
void fill_rect(struct IMAGE *image, int x, int y, unsigned int width,
	       unsigned int height, uint16_t c)
{
	uint16_t *d;

#ifdef DEBUG
	if (x < 0 || y < 0 || x + width > image->width ||
	    y + height > image->height)
		bug();
#endif
	d = &image->pixels[x + y * image->width];
	while (height--) {
		memset16(d, c, width);
		d += image->width;
	}
}

/*
 * Piirt‰‰ kuvan annettuun sijaintiin
 */
void draw_image(struct IMAGE *target, int x, int y, const struct IMAGE *image)
{
	uint16_t *d;
	const uint16_t *src;
	unsigned int h = image->height;

#ifdef DEBUG
	if (x < 0 || y < 0 || x + image->width > target->width ||
	    y + image->height > target->height)
		bug();
#endif
	d = &target->pixels[x + y * target->width];
	src = image->pixels;
	while (h--) {
		memcpy16(d, src, image->width);
		d += target->width;
		src += image->width;
	}
}

/*
 * Piirt‰‰ kuvan annettuun sijaintiin
 */
void draw_pal_image(struct IMAGE *target, int x, int y,
		    const struct PAL_IMAGE *image)
{
	uint16_t *d;
	const uint8_t *src;
	unsigned int h = image->height, i;

#ifdef DEBUG
	if (x < 0 || y < 0 || x + image->width > target->width ||
	    y + image->height > target->height)
		bug();
#endif
	d = &target->pixels[x + y * target->width];
	src = image->pixels;
	while (h--) {
		i = image->width;
		while (i--) {
			*d++ = image->palette[*src++];
		}
		d += target->width - image->width;
	}
}

/*
 * Piirt‰‰ osan kuvasta annettuun sijaintiin
 */
void copy_pal_image(struct IMAGE *target, int x, int y, unsigned int width,
		    unsigned int height, const struct PAL_IMAGE *image,
		    int ix, int iy)
{
	uint16_t *d;
	const uint8_t *src;
	unsigned int i;

#ifdef DEBUG
	if (x < 0 || y < 0 || x + width > target->width ||
	    y + height > target->height)
		bug();
	if (ix < 0 || iy < 0 || ix + width > image->width ||
	    iy + height > image->height)
		bug();
#endif
	d = &target->pixels[x + y * target->width];
	src = &image->pixels[ix + iy * image->width];
	while (height--) {
		i = width;
		while (i--) {
			*d++ = image->palette[*src++];
		}
		d += target->width - width;
		src += image->width - width;
	}
}

void shade_palette(uint16_t *palette, const uint16_t *from, int shade)
{
	static unsigned char table[64];
	unsigned int i;

	for (i = 0; i < 64; ++i) {
		table[i] = i * shade / 32;
	}
	i = 256;
	while (i--) {
		*palette++ = CREATE_RGB(table[GET_R(*from)],
					table[GET_G(*from)],
					table[GET_B(*from)]);
		from++;
	}
}

void draw_pal_image_double(struct IMAGE *target, int x, int y,
			   const struct PAL_IMAGE *image)
{
	uint16_t *d, *d2;
	const uint8_t *src;
	unsigned int h = image->height, i;

#ifdef DEBUG
	if (x < 0 || y < 0 || x + 2*image->width > target->width ||
	    y + 2*image->height > target->height)
		bug();
#endif
	d = &target->pixels[x + y * target->width];
	d2 = d + target->width;
	src = image->pixels;
	while (h--) {
		i = image->width;
		while (i--) {
			*d = d[1] = *d2 = d2[1] = image->palette[*src++];
			d  += 2;
			d2 += 2;
		}
		d  += target->width*2 - image->width*2;
		d2 += target->width*2 - image->width*2;
	}
}

void extract_image(struct PAL_IMAGE *image, const unsigned char *data)
{
	unsigned int count;
	uint8_t val, *d = image->pixels;
	size_t size = image->width * image->height;

	while (size) {
		if (*data >= 0x80) {
			/* yksitt‰isi‰ pikseleit‰ */

			count = *data++ - 0x80 + 1;
			if (count > size)
				bug();
			size -= count;
			while (count--) {
				*d++ = *data++;
			}
		} else {
			/* toistuva pikseli */

			count = *data++ + 1;
			val = *data++;
			if (count > size)
				bug();
			size -= count;
			while (count--) {
				*d++ = val;
			}
		}
	}
}

static uint16_t framebuffer[SCR_WIDTH * SCR_HEIGHT];

void flip_page()
{
	int i;

	/* Varmistetaan ett‰ kirjoitukset n‰kyy muistissa */
	for (i = 0; i < NIOS2_DCACHE_SIZE; i += NIOS2_DCACHE_LINE_SIZE) {
		flush_dcache(i);
	}

	if (screen.width != SCR_WIDTH / 2)
		return;

	IOWR(VGA_MODE, 1, (unsigned long)screen.pixels);

	screen.pixels += SCR_WIDTH * SCR_HEIGHT / 4;
	if (screen.pixels == &framebuffer[SCR_WIDTH * SCR_HEIGHT]) {
		screen.pixels = framebuffer;
	}
}

void init_graphics(int highres)
{
	if (highres) {
		screen.width = SCR_WIDTH;
		screen.height = SCR_HEIGHT;
		IOWR(VGA_MODE, 0, 0);
	} else {
		screen.width = SCR_WIDTH / 2;
		screen.height = SCR_HEIGHT / 2;
		IOWR(VGA_MODE, 0, 1);
	}
	screen.pixels = framebuffer;

	IOWR(VGA_MODE, 1, (unsigned long)framebuffer);
}
