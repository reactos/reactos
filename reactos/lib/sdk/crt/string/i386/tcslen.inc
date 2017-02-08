
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcslen
.code

FUNC _tcslen
    FPO 0, 1, 1, 1, 0, FRAME_FPO

    /* Save edi and eflags (according to the x86 ABI, we don't need to do that
       but since the native function doesn't change the direction flag, we don't
       either */
    push edi
    pushfd

    /* Load the string pointer into edi */
    mov edi, [esp + 12]

    /* Set eax to 0, since we want to compare with 0 */
    xor eax, eax

    /* Set ecx to -1 (i.e. 0xFFFFFFFF) */
    mov ecx, -1

    /* Clear direction flag */
    cld

    /* Now compare the characters until a 0 is found */
    repne _tscas

    /* Calculate the count. For n characters, we do (n + 1) comparisons. Initial
       value of ecx was -1, so end value of ecx is (-1 - (n + 1)) = -(n + 2).
       => n = -ecx - 2 = ~ecx - 1 */
    not ecx
    lea eax, [ecx - 1]

    /* Restore eflags/edi and return the result */
    popfd
    pop edi
    ret
ENDFUNC

END
/* EOF */
