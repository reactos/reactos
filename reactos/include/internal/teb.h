/* TEB/PEB parameters */
#ifndef __INCLUDE_INTERNAL_TEB
#define __INCLUDE_INTERNAL_TEB

#include <internal/ps.h>

#define PEB_BASE        (0xb0001000)
#define PEB_STARTUPINFO (0xb0003000)

#define NtCurrentPeb() ((PNT_PEB)PEB_BASE)

#endif /* __INCLUDE_INTERNAL_TEB */
