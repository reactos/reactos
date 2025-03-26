
 /*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:
 * FILE:              lib/sdk/crt/math/i386/log10_asm.s
 * PROGRAMER:         Magnus Olsen (magnus@greatlord.com)
 *
 */

#include <asm.inc>

PUBLIC _log10

/* FUNCTIONS ***************************************************************/
.code

_log10:

    push    ebp
    mov     ebp,esp
    fld     qword ptr [ebp+8]       // Load real from stack
    fldlg2                          // Load log base 10 of 2
    fxch    st(1)                   // Exchange st, st(1)
    fyl2x                           // Compute the log base 10(x)
    pop     ebp
    ret

END
