/*
 * COPYRIGHT:         GNU GPL - See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           
 * FILE:              lib/rtl/i386/res_asm.s
 * PROGRAMER:         
 */

#include <asm.inc>
#include <ks386.inc>

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
