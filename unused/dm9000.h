/*
 * DM9000 ethernet ajuri
 */
#ifndef _DM9000_H_
#define _DM9000_H_

void dm9000_shutdown();
void dm9000_init();

/*
 * buf pit‰‰ olla 16bit-aligned
 */
int dm9000_rx(char *buf, unsigned int *rxlen);

#endif
