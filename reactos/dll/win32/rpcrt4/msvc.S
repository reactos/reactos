
#include <asm.inc>

.code32

EXTERN _ObjectStubless@4:PROC

PUBLIC _call_stubless_func
_call_stubless_func:

    push esp        /* pointer to index */
    call _ObjectStubless@4
    pop edx         /* args size */
    mov ecx, [esp]  /* return address */
    add esp, edx
    jmp ecx

END
