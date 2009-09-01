/*
 * Ajastimen ajuri
 */
#ifndef _TIMER_H_
#define _TIMER_H_

#define HZ		100

void msleep(unsigned int ms);
unsigned long get_ticks();
void timer_irq();
void init_timer();

#endif

