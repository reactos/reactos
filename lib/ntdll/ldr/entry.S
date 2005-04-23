#include <roscfg.h>

.extern ___true_LdrInitializeThunk@16

.globl _LdrInitializeThunk@16
_LdrInitializeThunk@16:
#if defined(_M_IX86)
 nop /* breakin overwrites this with "int 3" */
 jmp ___true_LdrInitializeThunk@16
#elif defined(_M_ALPHA)
 nop /* breakin overwrites this with "call_pal bpt" */
 br ___true_LdrInitializeThunk@16
#elif defined(_M_MIPS)
 nop /* breakin overwrites this with "break" */
 j ___true_LdrInitializeThunk@16
#else
#error Unsupported architecture.
#endif
