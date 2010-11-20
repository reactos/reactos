
#include <asm.inc>

.code

PUBLIC ___readcr4
___readcr4:
    mov eax, cr4
    ret

END

