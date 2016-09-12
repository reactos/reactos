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
FUNC ceilf
    sub rsp, 16
    .ENDPROLOG

    /* Duplicate the bits into eax (zero exteneded to rax) */
    movd eax, xmm0

    /* Invert the sign bit */
    xor eax, HEX(80000000)

    /* Copy back to xmm0 */
    movd xmm0, eax

    /* Truncate xmm0 to integer (single precision) */
    cvttss2si rcx, xmm0

    /* Shift all bits to the right, keeping the sign bit */
    shr rax, 31

    /* Add the sign bit from the truncated value, so that
       we get the correct result for negative values. */
    add rcx, rax

    /* Now compensate for the previous negation */
    neg ecx

    /* Convert the result back to xmm0 (single precision) */
    cvtsi2ss xmm0, rcx

    add rsp, 16
    ret
ENDFUNC

END
