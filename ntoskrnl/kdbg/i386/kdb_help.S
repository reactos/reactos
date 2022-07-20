
#include <asm.inc>
#include <ks386.inc>

.code

PUBLIC _KdbpStackSwitchAndCall@8
_KdbpStackSwitchAndCall@8:
    push ebp
    mov ebp, esp

    mov eax, [esp + 8]         /* New stack */
    mov ecx, [esp + 12]         /* Function to call */
    mov edx, esp              /* Old stack */

    /* Switch stack */
    mov esp, eax
    push edx

    /* Call function */
    call ecx

    /* Switch back to old stack */
    pop esp

    /* Return */
    pop ebp
    ret 8

END
