/* $Id: tcsncmp.inc 49591 2010-11-15 01:29:12Z tkreuzer $
 */

#include "tchar.h"
#include <asm.inc>

PUBLIC _tcsncmp
.code

_tcsncmp:
    push esi
    push edi
    mov esi, [esp + 12] /* s1 */
    mov edi, [esp + 16] /* s2 */
    mov ecx, [esp + 20] /* n */

    xor eax, eax
    cld

.L1:
    dec ecx
    js .L2
    _tlods
    _tscas
    jne .L3
    test eax, eax
    jne .L1

.L2:
    xor eax, eax
    jmp .L4

.L3:
    sbb eax, eax
    or al, 1

.L4:
    pop edi
    pop esi
    ret

END
/* EOF */
