/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of ceil
 * FILE:              lib/sdk/crt/math/amd64/ceil.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

/* CODE **********************************************************************/
.code64

/* ceil(x) = - floor(-x)
 */
PUBLIC ceil
.PROC ceil
    sub rsp, 16
    .ENDPROLOG

    /* Duplicate the bits into rax */
    movd rax, xmm0

    /* Invert the sign bit */
    rol rax, 1
    xor al, 1
    ror rax, 1

    /* Copy back to xmm0 */
    movd xmm0, rax

    /* Truncate xmm0 to integer (double precision) */
    cvttsd2si rcx, xmm0

    /* Shift all bits to the right, keeping the sign bit */
    shr rax, 63

    /* Substract the sign bit from the truncated value, so that
       we get the correct result for negative values. */
    sub rcx, rax

    /* Now compensate for the previous negation */
    neg rcx

    /* Convert the result back to xmm0 (double precision) */
    cvtsi2sd xmm0, rcx

    add rsp, 16
    ret
.ENDP

END
