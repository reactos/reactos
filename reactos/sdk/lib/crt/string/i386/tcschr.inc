
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcschr
.code

FUNC _tcschr
    FPO 0, 2, 1, 1, 0, FRAME_FPO
    push esi
    mov esi, [esp + 8]
    mov edx, [esp + 12]
    cld

.L1:
    _tlods
    cmp _treg(d), _treg(a)
    je .L2
    test _treg(a), _treg(a)
    jnz .L1
    mov esi, _tsize

.L2:
    mov eax, esi
    _tdec(eax)

    pop esi
    ret
ENDFUNC

END
/* EOF */
