#ifndef _UTILS_H_
#define _UTILS_H_

#include "types.h"

void write_irq_mask(uint32_t mask);
void enable_irqs();
void disable_irqs();
void flush_icache(uint32_t addr);
void flush_dcache(uint32_t addr);

void exception_handler(uint32_t irqs_pending);

extern char excp_wrapper; /* osoitetta varten */

#endif
