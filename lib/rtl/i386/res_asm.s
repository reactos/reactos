
#include <reactos/asm.h>
#include <ndk/asm.h>

EXTERN _LdrpAccessResource@16:PROC

/*
 * On x86, Shrinker, an executable compressor, depends on the
 * "call access_resource" instruction being there.
 */
.code
PUBLIC _LdrAccessResource@16
_LdrAccessResource@16:
    push ebp
    mov ebp, esp
    sub esp, 4
    push [ebp + 24]
    push [ebp + 20]
    push [ebp + 16]
    push [ebp + 12]
    push [ebp + 8]
    call _LdrpAccessResource@16
    leave
    ret 16

END
