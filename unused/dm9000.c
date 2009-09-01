/*
 * DM9000 ethernet ajuri
 */
/*
 * Koodin pohja:
 *      Davicom DM9000 Fast Ethernet driver for Linux.
 * 	Copyright (C) 1997  Sten Wang
 *
 * 	This program is free software; you can redistribute it and/or
 * 	modify it under the terms of the GNU General Public License
 * 	as published by the Free Software Foundation; either version 2
 * 	of the License, or (at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * (C) Copyright 1997-1998 DAVICOM Semiconductor,Inc. All Rights Reserved.
 *
 * Additional updates, Copyright:
 *	Ben Dooks <ben@simtec.co.uk>
 *	Sascha Hauer <s.hauer@pengutronix.de>
 */
#include "dm9000.h"
#include "dm9000_regs.h"
#include "mii.h"
#include "my_boot.h"
#include "system.h"
#include "io.h"
#include "jtag.h"

#define IO_DELAY	20

#define IO_addr     0
#define IO_data     1

#define DM9000_PHY	0x40	/* PHY address 0x01 */

/*
 * I/O kirjoitus controllerin rekisteriin
 */
static void iow(unsigned int reg, unsigned int data)
{
	IOWR(DM9000A, IO_addr, reg);
	usleep(IO_DELAY);
	
	IOWR(DM9000A, IO_data, data);
	usleep(IO_DELAY);
}

/*
 * I/O luku controllerin rekisteristä
 */
static unsigned int ior(unsigned int reg)
{
	unsigned int val;
	
	IOWR(DM9000A, IO_addr, reg);
	usleep(IO_DELAY);
	
	val = IORD(DM9000A, IO_data);
	usleep(IO_DELAY);
	return val;
}

/*
 * Kirjoitus PHYlle
 */
static void dm9000_phy_write(int phyaddr_unused, unsigned int reg,
		unsigned int value)
{
	/* Fill the phyxcer register into REG_0C */
	iow(DM9000_EPAR, DM9000_PHY | reg);

	/* Fill the written data into REG_0D & REG_0E */
	iow(DM9000_EPDRL, value);
	iow(DM9000_EPDRH, value >> 8);

	iow(DM9000_EPCR, EPCR_EPOS | EPCR_ERPRW);	/* Issue phyxcer write command */

	msleep(1);		/* Wait write complete */

	iow(DM9000_EPCR, 0x0);	/* Clear phyxcer write command */
}

/*
 * Luku PHYstä
 */
static unsigned int dm9000_phy_read(int phy_reg_unused, unsigned int reg)
{
	unsigned int ret;

	/* Fill the phyxcer register into REG_0C */
	iow(DM9000_EPAR, DM9000_PHY | reg);

	iow(DM9000_EPCR, EPCR_ERPRR | EPCR_EPOS);	/* Issue phyxcer read command */

	msleep(1);		/* Wait read complete */

	iow(DM9000_EPCR, 0x0);	/* Clear phyxcer read command */

	/* The read data keeps on REG_0D & REG_0E */
	ret = (ior(DM9000_EPDRH) << 8) | ior(DM9000_EPDRL);

	return ret;
}

void dm9000_shutdown()
{
	puts("dm9000_shutdown()");

	/* RESET device */
	dm9000_phy_write(0, MII_BMCR, BMCR_RESET);	/* PHY RESET */
	iow(DM9000_GPR, 0x01);	/* Power-Down PHY */
	iow(DM9000_IMR, IMR_PAR);
	iow(DM9000_RCR, 0x00);	/* Disable RX */
}

void dm9000_init()
{
	puts("dm9000_init()");

	/* RESET device */
	iow(DM9000_NCR, NCR_RST);
	msleep(10);

	iow(DM9000_GPR, 0);	/* REG_1F bit0 activate phyxcer */
	iow(DM9000_GPCR, GPCR_GEP_CNTL);	/* Let GPIO0 output */
	iow(DM9000_GPR, 0x01);	/* Power-Down PHY */
	msleep(1);
	iow(DM9000_GPR, 0);	/* Enable PHY */
	msleep(10);

	iow(DM9000_NCR, NCR_EXT_PHY);

	dm9000_phy_write(0, MII_BMCR, BMCR_RESET);	/* PHY RESET */
	msleep(10);
	dm9000_phy_write(0, 16, 0x404); /* ??? */
	dm9000_phy_write(0, MII_ADVERTISE, ADVERTISE_100FULL | ADVERTISE_10FULL
			| ADVERTISE_CSMA);
	dm9000_phy_write(0, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);
	
	/* Program operating register */
	iow(DM9000_TCR, 0);	        /* TX Polling clear */
	iow(DM9000_BPTR, 0x3f);	/* Less 3Kb, 200us */
	iow(DM9000_FCR, 0xff);	/* Flow Control */
	iow(DM9000_SMCR, 0);        /* Special Mode */
	/* clear TX status */
	iow(DM9000_NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END);
	iow(DM9000_ISR, ISR_CLR_STATUS); /* Clear interrupt status */
	iow(0x2D, 0x80);      /* Switch LED to mode 1  ????? */
	
	iow(DM9000_RCR, RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN | RCR_ALL);
	iow(DM9000_IMR, IMR_PAR | IMR_PRM);
	
	msleep(10);
	
	puts("waiting link...");
	while (1) {
		dm9000_phy_read(0, MII_BMSR);
		if (dm9000_phy_read(0, MII_BMSR) & BMSR_LSTATUS) {
			puts("OK");
			break;
		}
		msleep(10);
	}
	
	puts("waiting autoneg...");
	while (1) {
		dm9000_phy_read(0, MII_BMSR);
		if (dm9000_phy_read(0, MII_BMSR) & BMSR_ANEGCOMPLETE) {
			puts("OK");
			break;
		}
		msleep(10);
	}
}

/*
 * buf pitää olla 16bit-aligned
 */

int dm9000_rx(char *buf, unsigned int *rxlen)
{
	unsigned int status, len;
	
	status = ior(DM9000_ISR); /* Check for rx interrupt */
	
	if (!(status & ISR_PRS))
		return 0;
	iow(DM9000_ISR, ISR_CLR_STATUS); /* Clear interrupt status */
	
	ior(DM9000_MRCMDX);	/* Dummy read */

	/* Get most updated data */
	status = IORD(DM9000A, IO_data) & 0xff;
	usleep(IO_DELAY);

	/* Status check: this byte must be 0 or 1 */
	if (status > DM9000_PKT_RDY) {
		puts("dm9000 error");
		while (1) ;
	}
	
	if (status != DM9000_PKT_RDY)
		return 0;
	
	IOWR(DM9000A,IO_addr, DM9000_MRCMD);
	usleep(IO_DELAY);
	status =  IORD(DM9000A, IO_data);
	usleep(IO_DELAY);
	len =  IORD(DM9000A, IO_data);
	usleep(IO_DELAY);
	
	if (status & (RSR_FOE | RSR_CE | RSR_AE |
				      RSR_PLE | RSR_RWTO |
				      RSR_LCS | RSR_RF) << 8) {
		puts("dm9000 rx error");
		/* Tyhjennetään pakettipuskuri */
		len = (len + 1) >> 1;
		while (len--) {
			IORD(DM9000A, IO_data);
			usleep(IO_DELAY);
		}
		return 0;
	}
	*rxlen = len;
	
	/* Luetaan paketti puskurista */
	len = (len + 1) >> 1;
	while (len--) {
		*((unsigned short *)buf) = IORD(DM9000A, IO_data);
		usleep(IO_DELAY);
		buf += 2;
	}
	
	return 1;
}
