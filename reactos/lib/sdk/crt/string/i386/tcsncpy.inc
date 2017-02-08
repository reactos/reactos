
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcsncpy
.code

_tcsncpy:
    push esi
    push edi
    mov edi, [esp + 12] /* s1 */
    mov esi, [esp + 16] /* s2 */
    mov ecx, [esp + 20] /* n */

    xor eax, eax
    cld

.L1:
    dec ecx
    js .L2
    _tlods
    _tstos
    test _treg(a), _treg(a)
    jnz .L1
    rep _tstos

.L2:
    mov eax, [esp + 12]

    pop edi
    pop esi
    ret

END
/* EOF */
