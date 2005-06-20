
#ifndef _NTNDK_
#include <ntos/ntdef.h>
#endif

/* Macros expanding to the appropriate inline assembly to raise a breakpoint */
#if defined(_M_IX86)
#define ASM_BREAKPOINT "\nint $3\n"
#elif defined(_M_ALPHA)
#define ASM_BREAKPOINT "\ncall_pal bpt\n"
#elif defined(_M_MIPS)
#define ASM_BREAKPOINT "\nbreak\n"
#else
#error Unsupported architecture.
#endif

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#ifdef DBG
extern VOID FASTCALL CHECK_PAGED_CODE_RTL(char *file, int line);
#define PAGED_CODE_RTL() CHECK_PAGED_CODE_RTL(__FILE__, __LINE__)
#else
#define PAGED_CODE_RTL()
#endif
