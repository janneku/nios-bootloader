/*
 * Ajastimen ajuri
 */
#include "timer.h"
#include "utils.h"
#include "io.h"
#include "system.h"

/*
 * Timer rekisterit
 * L‰hde: www.altera.com/literature/hb/nios2/n2cpu_nii51008.pdf
 */
#define TIMER_STATUS		0
#define TIMER_CONTROL		1
#define TIMER_PERIOD		2

static volatile unsigned long ticks;

/*
 * Pys‰ytt‰‰ suorituksen, parametrin‰ aika millisekunneissa.
 */
void msleep(unsigned int ms)
{
	unsigned long end = ticks + ms / (1000 / HZ) + 1;
	
	/* odotetaan loppumista */
	while (ticks != end)
		;
}

unsigned long get_ticks()
{
	return ticks;
}

void timer_irq()
{
	ticks++;
	
	/* kirjoitus nollaa keskeytyksen */
	IOWR(TIMER, TIMER_STATUS, 0);
}

void init_timer()
{
	ticks = 0;
	
	/* ajastimen keskeytys p‰‰lle */
	IOWR(TIMER, TIMER_CONTROL, 1);
	write_irq_mask(1u << TIMER_IRQ);
}
