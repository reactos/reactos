
#include <asm.inc>

PUBLIC _fmodf

/* FUNCTIONS ***************************************************************/
.code

_fmodf:
    push ebp
    mov ebp, esp

    fld dword ptr [esp + 4]
    fld dword ptr [esp + 8]
    fxch st(1)
l1:
    fprem
    fstsw ax
    sahf
    jp l1
    fstp st(1)

    pop ebp
    ret

END
