/* $Id: tcsrchr.inc 49591 2010-11-15 01:29:12Z tkreuzer $
 */

#include "tchar.h"
#include <asm.inc>

PUBLIC _tcsrchr
.code

_tcsrchr:
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

END
/* EOF */
