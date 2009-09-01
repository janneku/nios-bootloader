/*
 * I/O operaatiot
 */
#ifndef _IO_H_
#define _IO_H_

#define __IO_CALC_ADDRESS_NATIVE(BASE, REGNUM) \
  ((void *)( ((unsigned char *)BASE) + ((REGNUM) * (4))) )

#define IORD(BASE, REGNUM) \
  __builtin_ldwio (__IO_CALC_ADDRESS_NATIVE ((BASE), (REGNUM)))
#define IOWR(BASE, REGNUM, DATA) \
  __builtin_stwio (__IO_CALC_ADDRESS_NATIVE ((BASE), (REGNUM)), (DATA))

#endif
