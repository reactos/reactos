
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcscat
.code

FUNC _tcscat
    FPO 0, 2, 2, 2, 0, FRAME_FPO
    push esi
    push edi
    mov edi, [esp + 12]
    mov esi, [esp + 16]

    xor eax, eax
    mov ecx, -1
    cld

    repne _tscas
    _tdec(edi)

.L1:
    _tlods
    _tstos
    test _treg(a), _treg(a)
    jnz .L1

    mov eax, [esp + 12]
    pop edi
    pop esi
    ret
ENDFUNC

END
/* EOF */
