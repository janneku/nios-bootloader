#ifndef _FONTS_H_
#define _FONTS_H_

#include "graphic.h"

void draw_glyph(struct IMAGE *image, int x, int y, int c, uint16_t color);
void draw_string(struct IMAGE *image, int x, int y, const char *s, uint16_t c);
void draw_string_shadow(struct IMAGE *image, int x, int y, const char *s);
unsigned int get_text_width(const char *s);
unsigned int get_text_height();

#endif
