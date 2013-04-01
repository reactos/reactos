/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of floorf
 * FILE:              lib/sdk/crt/math/amd64/floorf.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

PUBLIC floorf
FUNC floorf
    sub rsp, 16
    .ENDPROLOG

    /* Truncate xmm0 to integer (single precision) */
    cvttss2si rcx, xmm0

    /* Duplicate the bits into rax */
    movd eax, xmm0

    /* Shift all bits to the right, keeping the sign bit */
    shr rax, 31

    /* Substract the sign bit from the truncated value, so that
       we get the correct result for negative values. */
    sub rcx, rax

    /* Convert the result back to xmm0 (single precision) */
    cvtsi2ss xmm0, rcx

    add rsp, 16
    ret
ENDFUNC

END
