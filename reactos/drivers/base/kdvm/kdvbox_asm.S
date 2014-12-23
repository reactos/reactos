
#include <asm.inc>

.code

#ifdef _M_IX86
PUBLIC @KdVmExchange@8
FUNC @KdVmExchange@8
    xchg eax, ecx
    mov edx, HEX(5659)
    out dx, eax
    ret
ENDFUNC
#else
PUBLIC KdVmExchange
FUNC KdVmExchange
    xchg rax, rcx
    mov edx, HEX(5659)
    out dx, rax
    ret
ENDFUNC
#endif

END
