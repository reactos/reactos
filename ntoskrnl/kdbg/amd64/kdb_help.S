
#include <asm.inc>

#include <ksamd64.inc>

.code64

PUBLIC KdbpStackSwitchAndCall
KdbpStackSwitchAndCall:

    /* Save old stack */
    mov rax, rsp

    /* Set new stack */
    mov rsp, rcx

    /* Save old stack on new stack */
    push rax

    /* Call function */
    call rdx

    /* Restore old stack */
    pop rax
    mov rsp, rax

    /* Return */
    ret

END
