#ifndef MENU_H
#define MENU_H

#define MAX_NAMES 255

#include "fat32.h"
#include "graphic.h"

void init_menu(struct FAT32_DIR *dir,
			int x_pos,
			int y_pos,
			unsigned int menu_width,
			unsigned int menu_height,
			uint16_t front_colour,
			uint16_t back_colour,
			struct PAL_IMAGE *background,
			const char *extension);

void load_menu(struct FAT32_DIR *dir);

int move_menu(int direction);

void show_menu();

const char *give_current();

void mark_current();

void remove_marks();

int is_dir();

#endif
