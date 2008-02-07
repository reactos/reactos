#ifndef _INTRIN_INTERNAL_
#define _INTRIN_INTERNAL_

static __inline__ __attribute__((always_inline)) void KeArchHaltProcessor(void)
{
    //
    // Enter Wait-For-Interrupt Mode
    //
    __asm__ __volatile__
    (
     "mov r1, #0;"
     "mcr p15, 0, r1, c7, c0, 4;"
     );
}

#endif

/* EOF */
