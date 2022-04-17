
#include <asm.inc>

PUBLIC _fmod

/* FUNCTIONS ***************************************************************/
.code

_fmod:
    push ebp
    mov ebp, esp

    fld qword ptr [ebp + 8]
    fld qword ptr [ebp + 16]
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
