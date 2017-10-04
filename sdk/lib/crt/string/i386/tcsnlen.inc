
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcsnlen
.code

FUNC _tcsnlen
    FPO 0, 2, 1, 1, 0, FRAME_FPO
    push edi
    mov edi, [esp + 8]
    mov ecx, [esp + 12]
    xor eax, eax
    test ecx, ecx
    jz .L1
    mov edx, ecx

    cld

    repne _tscas

    sete al
    sub edx, ecx
    sub edx, eax
    mov eax, edx

.L1:
    pop edi
    ret
ENDFUNC

END
/* EOF */
