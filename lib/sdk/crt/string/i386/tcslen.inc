
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcslen
.code

FUNC _tcslen
    FPO 0, 1, 1, 1, 0, FRAME_FPO
    push edi
    mov edi, [esp + 8]
    xor eax, eax
    test edi, edi
    jz _tcslen_end

    mov ecx, -1
    cld

    repne _tscas

    not ecx
    dec ecx

    mov eax, ecx

_tcslen_end:
    pop edi
    ret
ENDFUNC

END
/* EOF */
