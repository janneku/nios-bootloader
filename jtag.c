/*
 * JTAG UART ajuri
 */
#include "jtag.h"
#include "io.h"
#include "system.h"

#define JTAG_UART_CONTROL_WSPACE_MSK        0xFFFF0000ul

/*
 * UART rekisterit
 * L‰hde: www.altera.com/literature/hb/nios2/n2cpu_nii51009.pdf
 */
#define JTAG_UART_DATA		0
#define JTAG_UART_CONTROL	1

/*
 * Tulostaa merkin UART kautta. Pit‰‰ toimia ilman keskeytyksi‰!
 */
int putchar(int ch)
{
	int i = 100000;

	/* odotetaan kunnes FIFO mahtuu lis‰‰ */
	while (!(IORD(JTAG_UART, JTAG_UART_CONTROL) &
	       JTAG_UART_CONTROL_WSPACE_MSK)) {
		/* timeout */
		i--;
		if (!i)
			return -1;
	}
	IOWR(JTAG_UART, JTAG_UART_DATA, ch);
	return 0;
}

/*
 * Tulostaa merkkijonon UARTiin
 */
int put_string(const char *s)
{
	while (*s) {
		if (putchar(*s++))
			return -1;
	}
	return 0;
}
