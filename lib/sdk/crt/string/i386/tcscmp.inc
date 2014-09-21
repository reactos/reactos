
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcscmp
.code

_tcscmp:
    push esi
    push edi
    mov esi, [esp + 12]
    mov edi, [esp + 16]
    xor eax, eax
    cld

.L1:
    _tlods
    _tscas
    jne .L2
    test eax, eax
    jne .L1
    xor eax, eax
    jmp .L3

.L2:
    sbb eax, eax
    or al, 1

.L3:
    pop edi
    pop esi
    ret

END
/* EOF */
