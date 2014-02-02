
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcsncat
.code

_tcsncat:
    push esi
    push edi
    mov edi, [esp + 12]
    mov esi, [esp + 16]
    cld

    xor eax, eax
    mov ecx, -1
    repne _tscas
    _tdec(edi)

    mov ecx, [esp + 20]

.L1:
    dec ecx
    js .L2
    _tlods
    _tstos
    test _treg(a), _treg(a)
    jne .L1
    jmp .L3

.L2:
    xor eax, eax
    _tstos

.L3:
    mov eax, [esp + 12]
    pop edi
    pop esi

    ret

END
/* EOF */
