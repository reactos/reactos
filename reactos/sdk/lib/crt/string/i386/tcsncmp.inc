
#include "tchar.h"
#include <asm.inc>

PUBLIC _tcsncmp
.code

FUNC _tcsncmp
    FPO 0, 3, 2, 2, 0, FRAME_FPO
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
ENDFUNC

END
/* EOF */
