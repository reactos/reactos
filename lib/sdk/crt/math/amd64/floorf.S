/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of tan
 * FILE:              lib/sdk/crt/math/amd64/floorf.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

/* CODE **********************************************************************/
.code64

PUBLIC floorf
floorf:
    sub rsp, 16

    /* Put parameter on the stack */
    movss [rsp], xmm0
    fld   dword ptr [rsp]

    /* Change fpu control word to round down */
    fstcw [rsp]
    mov   eax, [rsp]
    or    eax, 0x00400
    and   eax, 0x0f7ff
    mov   [rsp + 8], eax
    fldcw [rsp + 8]

    /* Round to integer */
    frndint

    /* Restore fpu control word */
    fldcw [rsp]

    fstp  dword ptr [rsp]
    movss xmm0, [rsp]
    add rsp, 16
    ret

END
