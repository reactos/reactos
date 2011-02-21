/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of tan
 * FILE:              lib/sdk/crt/math/amd64/ceilf.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

/* CODE **********************************************************************/
.code64

PUBLIC ceilf
ceilf:
    sub rsp, 16

    /* Put parameter on the stack */
    movss [rsp], xmm0
    fld dword ptr [rsp]

    /* Change fpu control word to round up */
    fstcw [rsp + 8]
    mov eax, [rsp + 8]
    or  eax, HEX(00800)
    and eax, HEX(0fbff)
    mov [rsp + 12], eax
    fldcw [rsp + 12]

    /* Round to integer */
    frndint

    /* Restore fpu control word */
    fldcw [rsp + 8]

    fstp dword ptr [rsp]
    movss xmm0, [rsp]

    add rsp, 16
    ret

END
