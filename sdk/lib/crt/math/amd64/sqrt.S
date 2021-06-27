/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of sqrt
 * FILE:              lib/sdk/crt/math/amd64/sqrt.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

PUBLIC sqrt
sqrt:

    /* Load the sign bit into rdx */
    mov rdx, HEX(8000000000000000)

    /* Move the lower 64 bits of xmm0 into rax */
    movd rax, xmm0

    /* Test the sign bit */
    test rax, rdx

    /* If it is set, go to the failure path */
    jnz x_is_negative

    /* x is positive, now check if it is NaN by checking if the unsigned
       integer value is larger than the highest valid positive value. */
    mov rcx, HEX(7FF0000000000000)
    cmp rax, rcx
    ja short x_is_nan

    /* All is well, calculate the sqrt */
    sqrtsd xmm0, xmm0
    ret

x_is_negative:
    /* Load failure return value (-1.#IND00) into rcx */
    mov rcx, HEX(0FFF8000000000000)

    /* Check if the parameter was -0.0 */
    cmp rax, rdx

    /* If it was not, load the failure value, otherwise keep -0.0  */
    cmovne  rax, rcx

    /* Move the value back into the return register */
    movd xmm0, rax
    ret

x_is_nan:
    /* Create a 1.#QNAN0 by setting this bit */
    mov rcx, HEX(8000000000000)
    or rax, rcx

    /* Move the value back into the return register */
    movd xmm0, rax
    ret


END
