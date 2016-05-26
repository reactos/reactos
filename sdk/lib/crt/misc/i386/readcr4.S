
#include <asm.inc>

.code

PUBLIC ____readcr4
____readcr4:
    mov eax, cr4
    ret

END

