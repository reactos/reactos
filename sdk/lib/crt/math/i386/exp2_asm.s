#include <asm.inc>

PUBLIC _exp2

/* FUNCTIONS ***************************************************************/
.code

_exp2:
    push ebp
    mov ebp, esp

    fld qword ptr [ebp + 8]
    fxam
    fstsw ax
    fwait
    sahf
    jnp .not_inf
    jnc .not_inf
    test ah, 2
    jz .done
    fstp st
    fldz
    jmp .done
.not_inf:
    fst st(1)
    frndint
    fxch st(1)
    fsub st, st(1)
    f2xm1
    fld1
    faddp st(1), st
    fscale
    fstp st(1)
.done:
    pop ebp
    ret

END
