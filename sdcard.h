/*
 * SD-kortin ajuri
 */
#ifndef _SDCARD_H_
#define _SDCARD_H_

int SD_card_init();
int SD_read_lba(void *buff, unsigned long lba);

#endif
