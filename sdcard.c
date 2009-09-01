/*
 * SD-kortin ajuri
 */
#include "sdcard.h"
#include "types.h"
#include "string.h"
#include "timer.h"
#include "jtag.h"
#include "io.h"
#include "system.h"

#define SECTOR_SIZE		512

/*
 * PIO rekisterit
 * Lähde: www.altera.com/literature/hb/nios2/n2cpu_nii51007.pdf
 */
#define PIO_DATA		0
#define PIO_DIRECTION		1
#define PIO_INTERRUPTMASK	2
#define PIO_EDGECAPTURE		3

//  SD Card Set I/O Direction
#define SD_CMD_IN   IOWR(SD_CMD, PIO_DIRECTION, 0)
#define SD_CMD_OUT  IOWR(SD_CMD, PIO_DIRECTION, 1)
#define SD_DAT_IN   IOWR(SD_DAT, PIO_DIRECTION, 0)
#define SD_DAT_OUT  IOWR(SD_DAT, PIO_DIRECTION, 1)

//  SD Card Output High/Low
#define SD_CMD_LOW  IOWR(SD_CMD, PIO_DATA, 0)
#define SD_CMD_HIGH IOWR(SD_CMD, PIO_DATA, 1)
#define SD_DAT_LOW  IOWR(SD_DAT, PIO_DATA, 0)
#define SD_DAT_HIGH IOWR(SD_DAT, PIO_DATA, 1)
#define SD_CLK_LOW  IOWR(SD_CLK, PIO_DATA, 0)
#define SD_CLK_HIGH IOWR(SD_CLK, PIO_DATA, 1)

//  SD Card Input Read
#define SD_TEST_CMD IORD(SD_CMD, PIO_DATA)
#define SD_TEST_DAT IORD(SD_DAT, PIO_DATA)

/*
 * MMC-käskyt
 * Lähde:
 * www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf
 */
#define MMC_GO_IDLE_STATE         0
#define MMC_ALL_SEND_CID          2
#define MMC_SET_RELATIVE_ADDR     3
#define MMC_SELECT_CARD           7
#define MMC_SEND_CSD              9
#define MMC_SET_BLOCKLEN         16
#define MMC_READ_SINGLE_BLOCK    17
#define MMC_APP_CMD              55
#define SD_APP_OP_COND           41

/*
 * Pieni viive jottei ylitetä 25MHz
 */
static void udelay()
{
	SD_TEST_DAT;
	SD_TEST_DAT;
}

static void sd_delay(unsigned int count)
{
	while (count--) {
		SD_CLK_LOW;
		udelay();
		SD_CLK_HIGH;
	}
}

static uint8_t read_dat_byte()
{
	int j;
	uint8_t c = 0;

	for (j = 0; j < 8; j++) {
		c <<= 1;
		if (SD_TEST_DAT)
			c |= 1;
		SD_CLK_LOW;
		udelay();
		SD_CLK_HIGH;
	}
	return c;
}

static uint8_t read_cmd_byte()
{
	int j;
	uint8_t c = 0;

	for (j = 0; j < 8; j++) {
		c <<= 1;
		if (SD_TEST_CMD)
			c |= 1;
		SD_CLK_LOW;
		udelay();
		SD_CLK_HIGH;
	}
	return c;
}

static void write_cmd_byte(uint8_t c)
{
	int j;

	for (j = 0; j < 8; j++) {
		SD_CLK_LOW;
		if (c & 0x80)
			SD_CMD_HIGH;
		else
			SD_CMD_LOW;
		SD_CLK_HIGH;

		c <<= 1;
	}
}

static uint8_t mmc_crc(uint8_t *buf, size_t len)
{
	uint8_t crc = 0, b;
	int i, j;

	for (i = 0; i < len; i++) {
		b = buf[i];

		for (j = 0; j < 8; j++) {
			crc <<= 1;
			if ((crc ^ b) & 0x80)
				crc ^= 0x09;

			b <<= 1;
		}
	}
	return (crc << 1) | 1;
}

static uint16_t mmc_crc16(uint8_t *buf, size_t len)
{
	uint16_t crc = 0, b;
	int i, j;

	for (i = 0; i < len; i++) {
		b = buf[i];

		crc ^= b << 8;
		for (j = 0; j < 8; j++) {
			if (crc & 0x8000) {
				crc <<= 1;
				crc ^= 0x1021;
			} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

static int execute_command(const uint8_t *cmd, uint8_t reg, uint8_t *resp)
{
	int i, len;

	SD_CMD_OUT;
	for (i = 0; i < 6; i++) {
		write_cmd_byte(cmd[i]);
	}
	SD_CMD_IN;

	if (!reg)
		return 0;

	sd_delay(2);

	/* odotetaan nollabittiä */
	i = 100;
	while (SD_TEST_CMD) {
		/* timeout */
		i--;
		if (!i) {
			put_string("execute_command(): timeout\n");
			return -1;
		}
		SD_CLK_LOW;
		udelay();
		SD_CLK_HIGH;
		udelay();
	}

	if (reg == 2)
		len = 17;
	else
		len = 6;

	for (i = 0; i < len; i++) {
		resp[i] = read_cmd_byte();
	}

	/* tarkistetaan CRC */
	if (reg == 2) {
		if (resp[16] != mmc_crc(resp + 1, 15)) {
			put_string("execute_command(): invalid CRC\n");
			return -1;
		}
	} else if (reg != 3) {
		if (resp[5] != mmc_crc(resp, 5)) {
			put_string("execute_command(): invalid CRC\n");
			return -1;
		}
	}

	sd_delay(8);

	return 0;
}

static int sd_command(uint8_t opcode, uint32_t arg, uint8_t reg,
		     uint8_t *resp, int retries)
{
	uint8_t buf[6];

	buf[0] = opcode | 0x40;
	buf[1] = arg >> 24;
	buf[2] = arg >> 16;
	buf[3] = arg >> 8;
	buf[4] = arg;
	/* CRC loppuun */
	buf[5] = mmc_crc(buf, 5);

	while (retries--) {
		if (!execute_command(buf, reg, resp)) {
			return 0;
		}
		sd_delay(20);
	}
	return -1;
}

static int read_block(uint8_t *buf, unsigned long lba)
{
	int i;
	uint8_t crc[2];

	/* ei lueta replyä, koska data tulee DAT linjalta samaan aikaan! */
	sd_command(MMC_READ_SINGLE_BLOCK, lba * SECTOR_SIZE, 0, NULL, 1);

	sd_delay(2);

	/* odotetaan nollabittiä */
	i = 10000000;
	while (SD_TEST_DAT) {
		/* timeout */
		i--;
		if (!i) {
			put_string("read_block(): timeout\n");
			return -1;
		}
		SD_CLK_LOW;
		udelay();
		SD_CLK_HIGH;
		udelay();
	}

	SD_CLK_LOW;
	udelay();
	SD_CLK_HIGH;

	for (i = 0; i < SECTOR_SIZE; i++) {
		buf[i] = read_dat_byte();
	}

	crc[0] = read_dat_byte();
	crc[1] = read_dat_byte();

	if (((crc[0] << 8) | crc[1]) != mmc_crc16(buf, SECTOR_SIZE)) {
		put_string("read_block(): invalid CRC\n");
		return -1;
	}
	sd_delay(8);

	return 0;
}

int SD_card_init()
{
	uint8_t RCA[2];
	uint8_t buffer[20];
	int i;

	SD_CMD_OUT;
	SD_DAT_IN;
	SD_CLK_HIGH;
	SD_CMD_HIGH;
	SD_DAT_LOW;
	SD_CMD_IN;

	sd_delay(80);

	sd_command(MMC_GO_IDLE_STATE, 0, 0, NULL, 1);

	sd_delay(8);

	/* odotetaan että kortti on valmis */
	i = 30;
	while (1) {
		i--;
		if (!i)
			return -1;

		msleep(1);
		sd_delay(8);

		if (sd_command(MMC_APP_CMD, 0, 1, buffer, 1))
			continue;

		if (!sd_command(SD_APP_OP_COND, 0xF00000, 3, buffer, 1)) {
			if (buffer[1] & 0x80)
				break;
		}
	}

	put_string("SD_card_init(): card ready\n");

	if (sd_command(MMC_ALL_SEND_CID, 0, 2, buffer, 5)) {
		return -1;
	}

	if (sd_command(MMC_SET_RELATIVE_ADDR, 0, 6, buffer, 5))
		return -1;
	RCA[0] = buffer[1];
	RCA[1] = buffer[2];

	if (sd_command(MMC_SEND_CSD, RCA[0] << 24 | RCA[1] << 16, 2, buffer,
			5)) {
		return -1;
	}

	if (sd_command(MMC_SELECT_CARD, RCA[0] << 24 | RCA[1] << 16, 1,
			buffer, 5))
		return -1;

	if (sd_command(MMC_SET_BLOCKLEN, SECTOR_SIZE, 1, buffer, 5))
		return -1;

	return 0;
}

int SD_read_lba(void *buff, unsigned long lba)
{
	int retries = 5;

	sd_delay(8);

	while (retries--) {
		if (!read_block(buff, lba)) {
			return 0;
		}
		sd_delay(20);
	}
	return -1;
}
