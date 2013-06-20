
#include <asm.inc>

PUBLIC _exp
 
/* FUNCTIONS ***************************************************************/
.code

_exp:
    push ebp
    mov ebp, esp

    fld qword ptr [ebp + 8]
    fldl2e
    fmul st, st(1)
    fst st(1)
    frndint
    fxch st(1)
    fsub st, st(1)
    f2xm1
    fld1
    faddp st(1), st
    fscale
    fstp st(1)

    pop ebp
    ret

END
