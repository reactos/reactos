/* $Id$
*/

#include "tchar.h"
#include <reactos/asm.h>

PUBLIC _tcslen
.code

_tcslen:
    push edi
    mov edi, [esp + 8]
    xor eax, eax
    test edi, edi
    jz _tcslen_end

    mov ecx, -1
    cld

    repne _tscas

    not ecx
    dec ecx

    mov eax, ecx

_tcslen_end:
    pop edi
    ret

END
/* EOF */
