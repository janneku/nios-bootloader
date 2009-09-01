#include "def.h"

.global _start
_start:
	/* Instruction Cache alustus */
	movui r2, NIOS2_ICACHE_SIZE
0:
	addi r2, r2, -NIOS2_ICACHE_LINE_SIZE
	initi r2
	bgt r2, zero, 0b

	/* Data Cache alustus */
	movui r2, NIOS2_DCACHE_SIZE
0:
	addi r2, r2, -NIOS2_DCACHE_LINE_SIZE
	initd 0(r2)
	bgt r2, zero, 0b

	flushp

	/* Alustetaan muisti */
	movui r2, RAM
	movui r3, (RAM+RAM_SIZE)
0:
	stw zero, (r2)
	addi r2, r2, 4
	blt r2, r3, 0b

	/* Asetetaan pino */
	movhi	sp, %hi(stack + (4*1024))
	ori	sp, sp, %lo(stack + (4*1024))
	
	call	main
	
	/* jos mainista palataan... */
afterlife:
	br	afterlife

.end
