#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARCH_PS_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARCH_PS_H

#ifdef _M_IX86
#define KiArchContextSwitch          KiSwapContext
#define KiArchInitThreadWithContext  Ke386InitThreadWithContext
#else
#error "Unknown processor"
#endif

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_ARCH_PS_H */

/* EOF */
