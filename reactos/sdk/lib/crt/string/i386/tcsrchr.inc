
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcsrchr
.code

FUNC _tcsrchr
    FPO 0, 2, 1, 1, 0, FRAME_FPO
    push esi
    mov esi, [esp + 8]
    mov edx, [esp + 12]

    cld
    mov ecx, _tsize

.L1:
    _tlods
    cmp _treg(d), _treg(a)
    jne .L2
    mov ecx, esi

.L2:
    test _treg(a), _treg(a)
    jnz .L1

    mov eax, ecx
    _tdec(eax)
    pop esi
    ret
ENDFUNC

END
/* EOF */
