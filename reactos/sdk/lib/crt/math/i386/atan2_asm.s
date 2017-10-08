
#include <asm.inc>

PUBLIC _atan2

.code
_atan2:
    push ebp
    mov ebp, esp

    fld qword ptr [ebp + 8]
    fld qword ptr [ebp + 16]
    fpatan

    pop ebp
    ret

END
