/*
 * Tekstien piirto fontilla
 */
#include "fonts.h"
#include "graphic.h"
#include "string.h"
#include "bug.h"
#include "system.h"

#include "fontti.h"

/*
 * Piirt‰‰ halutun merkin kuvaan
 */
void draw_glyph(struct IMAGE *image, int x, int y, int c, uint16_t color)
{
	static const char glyphs[] =
	    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz*-!/.:";
	uint16_t *d;
	unsigned char bits;
	const unsigned char *src = 0;
	unsigned int i, h;
	if ((uint8_t)c <= ' ')
		return;

#ifdef DEBUG
	if (x < 0 || y < 0 || x + 8 > image->width ||
	    y + fontti_png_height > image->height)
		bug();
#endif
	/* etsit‰‰n merkki taulukosta */
	for (i = 0; glyphs[i]; ++i) {
		if (glyphs[i] == c) {
			src = fontti_png_data + i * fontti_png_height;
			break;
		}
	}
	if (!src)
		return;

	/* piirret‰‰n merkki */
	d = &image->pixels[x + y * image->width];
	for (h = 0; h < fontti_png_height; ++h) {
		bits = *src++;
		for (i = 0; i < 4; ++i) {
			/* kaksi pikseli‰ kerralla */
			if (bits & 1)
				*d = color;
			if (bits & 2)
				d[1] = color;
			d += 2;
			bits >>= 2;
		}
		/* seuraava rivi */
		d += image->width - 8;
	}
}

/*
 * Piirt‰‰ merkkijonon kuvaan annetulla v‰rill‰
 */
void draw_string(struct IMAGE *image, int x, int y, const char *s, uint16_t c)
{
	while (*s) {
		draw_glyph(image, x, y, *s++, c);
		x += 8;
	}
}

void draw_string_shadow(struct IMAGE *image, int x, int y, const char *s)
{
	draw_string(image, x + 1, y + 1, s, BLACK);
	draw_string(image, x, y, s, WHITE);
}

unsigned int get_text_width(const char *s)
{
	return strlen(s) * 8;
}

unsigned int get_text_height()
{
	return fontti_png_height;
}
