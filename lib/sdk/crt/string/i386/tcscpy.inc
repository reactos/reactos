/* $Id$
 */

#include "tchar.h"
#include <reactos/asm.h>

PUBLIC _tcscpy
.code

_tcscpy:
    push esi
    push edi
    mov edi, [esp + 12]
    mov esi, [esp + 16]
    cld

.L1:
    _tlods
    _tstos
    test _treg(a), _treg(a)
    jnz .L1

    mov eax, [esp + 12]

    pop edi
    pop esi
    ret

END
/* EOF */
