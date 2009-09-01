#ifndef FAT32_H
#define FAT32_H

#include "types.h"

#define SECTOR_SIZE 	512

#define MAX_NAME_LENGTH 256

struct FAT32_FILE {
	uint32_t start_cluster;
	uint32_t cur_cluster;
	uint32_t cur_sector;
	uint32_t cur_byte;
	uint32_t bytes_left;
	uint32_t size;
};

struct FAT32_DIR {
	uint32_t start_cluster;
	uint32_t cur_cluster;
	uint32_t cur_sector;
	int position;
};

int fat32_init();

int fat32_fopen(struct FAT32_DIR *dir, const char *name, struct FAT32_FILE *f);

/* Vaihtaa nykyisen kansion sisällä kansiota */
int fat32_chdir(struct FAT32_DIR *dir, const char *name);

/* Aukaisee polun ("/polku/jonnekkin") */
int fat32_opendir(const char *path, struct FAT32_DIR *dir);

/*	Antaa kansiossa olevan kansion/tiedoston nimen
*	Paluuarvot:
*	<0	jos toimintoa ei voitu suorittaa
*	0	jos löydettiin tiedosto
*	>0	jos löydettiin kansio
*/
int fat32_readdir(struct FAT32_DIR *dir, char *name);

size_t fat32_read(void *d, size_t count, struct FAT32_FILE *f);

int fat32_seek(size_t offset, struct FAT32_FILE *f);

#endif
