#include "menu.h"
#include "string.h"
#include "graphic.h"
#include "fonts.h"
#include "fat32.h"

static char dirnames[MAX_NAMES][MAX_NAME_LENGTH+1];
static int posx, posy;
static unsigned int width, height, amount;
static uint16_t front, back;
static struct PAL_IMAGE *backimage;
static char ext[4];

#define FOLDER		1
#define MARKED		2

static unsigned int start, offset;

static const char *get_extension(const char *s)
{
	while (*s) {
		if (*(s++) == '.')
			break;
	}
	return s;
}

static void sort_menu()
{
	unsigned int i, j;
	static char tmp[MAX_NAME_LENGTH+1];

	if (!amount)
		return;

	for(i = 0; i < amount - 1; ++i) {
		for(j = i + 1; j < amount; ++j) {
			if(strcasecmp(dirnames[i], dirnames[j]) > 0) {

				strcpy(tmp, dirnames[i]);
				tmp[MAX_NAME_LENGTH]
					= dirnames[i][MAX_NAME_LENGTH];
				strcpy(dirnames[i], dirnames[j]);
				dirnames[i][MAX_NAME_LENGTH]
					= dirnames[j][MAX_NAME_LENGTH];
				strcpy(dirnames[j], tmp);
					dirnames[j][MAX_NAME_LENGTH]
						= tmp[MAX_NAME_LENGTH];
			}
		}
	}

}

void init_menu(struct FAT32_DIR *dir,
			int x_pos,
			int y_pos,
			unsigned int menu_width,
			unsigned int menu_height,
			uint16_t front_colour,
			uint16_t back_colour,
			struct PAL_IMAGE *background,
			const char *extension)
{
	posx = x_pos;
	posy = y_pos;
	width = menu_width;
	height = menu_height;
	front = front_colour;
	back = back_colour;
	backimage = background;
	start = offset = 0;
	strcpy(ext, extension);
	load_menu(dir);
}

void load_menu(struct FAT32_DIR *dir)
{
	unsigned int i = 0;
	int j;

	while (i < MAX_NAMES) {
		j = fat32_readdir(dir, dirnames[i]);
		if (!j) {
			if (!strcasecmp(get_extension(dirnames[i]), ext)) {
				dirnames[i][MAX_NAME_LENGTH] = 0;
				i++;
			}
		}
		else if (j > 0) {
			dirnames[i][MAX_NAME_LENGTH] = FOLDER;
			i++;
		} else
			break;
	}
	amount = i;
	start = offset = 0;
	sort_menu();
	show_menu();
}

void show_menu()
{
	int x = posx, y = posy, txt_h = get_text_height();
	unsigned int i, j;
	unsigned int entries = height/txt_h;
	unsigned int max_chars = width/8;
	uint16_t fcolour;

	if (!backimage)
		fill_rect(&screen, posx, posy, width, height, back);
	else
		copy_pal_image(&screen, posx, posy, width,
					height, backimage, posx, posy);

	fill_rect(&screen, posx, posy + offset*txt_h, width, txt_h, front);

	for (i=start; i<amount; i++) {

		if (i - start == entries)
			break;

		if (i - start == offset)
			fcolour = back;
		/* HACK */
		else if (dirnames[i][MAX_NAME_LENGTH] & MARKED)
			fcolour = RGB(255, 255, 0);
		else
			fcolour = front;

		for (j=0; j<MAX_NAME_LENGTH; j++) {
			if (j == max_chars - 1 || dirnames[i][j] == '\0'
						|| dirnames[i][j] == '.')
				break;

			draw_glyph(&screen, x, y, dirnames[i][j], fcolour);
			x += 8;
		}

		if(dirnames[i][MAX_NAME_LENGTH] & FOLDER)
			draw_glyph(&screen, x, y, '/', fcolour);

		y += get_text_height();
		x = posx;
	}

}

int move_menu(int direction)
{
	int entries = height / get_text_height();
	int newoffset = offset + direction;

	if (newoffset < 0) {
		if (start != 0)
			start += direction;
		else
			direction = 0;
	}

	else if (newoffset >= entries) {
		if (start != amount - entries)
			start += direction;
		else
			direction = 0;
	}

	else if (start + newoffset < amount)
		offset = newoffset;
	else 
		direction = 0;

	show_menu();
	return direction;

}

const char *give_current()
{
	return dirnames[start+offset];
}

void mark_current()
{
	dirnames[start+offset][MAX_NAME_LENGTH] |= MARKED;
}

void remove_marks()
{
	unsigned int i;

	for(i = 0; i < amount; i++) {
		dirnames[i][MAX_NAME_LENGTH] &= ~MARKED;
	}
}

int is_dir()
{
	return dirnames[start+offset][MAX_NAME_LENGTH] & FOLDER;
}
