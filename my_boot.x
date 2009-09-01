/*
 * Muistikartta
 */
#include "def.h"
OUTPUT_FORMAT( "elf32-littlenios2" )
OUTPUT_ARCH( nios2 )
ENTRY( _start )

SECTIONS
{
	/* Koodi ja RO-data flashissa */
	.flash ONBOARD_FLASH :
	{
		*(.text)
		*(.rodata)
		*(.rodata.*)
	}

	/* Pino ja väliaikaismuisti */
	.ram BSS_ADDR :
	{
		*(.bss)
		*(COMMON)
	}
	
	/* Ei-sallitut */
	/DISCARD/ : {
		*(.comment)
		*(.data)
	}
}
