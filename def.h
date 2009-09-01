#ifndef _DEF_H_
#define _DEF_H_

#include "system.h"

#define NIOS2_ICACHE_SIZE 4096
#define NIOS2_DCACHE_SIZE 4096
#define NIOS2_ICACHE_LINE_SIZE 32
#define NIOS2_DCACHE_LINE_SIZE 32

/* Pino ja väliaikaismuisti */
#define BSS_ADDR	(RAM + 0x700000)

/* Ladattavan ohjelman sijainti */
#define PROGRAM_ADDR	RAM

#define EXCP_VECTOR	RAM

#endif
