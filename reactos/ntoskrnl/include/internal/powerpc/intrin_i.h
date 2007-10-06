#ifndef _INTRIN_INTERNAL_
#define _INTRIN_INTERNAL_

#define Ke386SaveFlags(x) __asm__ __volatile__("mfmsr %0" : "=r" (x) :)

#endif

/* EOF */
