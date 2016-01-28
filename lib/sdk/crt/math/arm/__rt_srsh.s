/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of __rt_srsh
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/

    TEXTAREA

/*
    __int64
    __rt_srsh(
        __int64 value,
        uint32_t shift);

    R0 = loword of value
    R1 = hiword of value
    R2 = shift

*/

    LEAF_ENTRY __rt_srsh

    /* r3 = 32 - r2 */
    rsbs r3, r2, #32

    /* Branch if minus (r2 > 32)  */
    bmi __rt_srsh2

    /* r0 = r0 >> r2 (logical shift!) */
    lsr r0, r0, r2

    /* r3 = r1 << r3 */
    lsl r3, r1, r3

    /* r0 |= r1 << (32 - r2) */
    orr r0, r0, r3

    /* r1 = r1 >> r2 (arithmetic shift!) */
    asr r1, r1, r2

    bx lr

__rt_srsh2

    /* Check if shift is > 64 */
    cmp r2, 64
    bhs __rt_srsh3

    /* r3 = r2 - 32 */
    sub r3, r2, #32

    /* r0 = r1 >> r3 (arithmetic shift!) */
    asr r0, r1, r3

    /* r1 = r1 >> 32 (arithmetic shift!) */
    asr r1, r1, #32

    bx lr

__rt_srsh3

    /* r1 = r1 >> 32 (arithmetic shift!) */
    asr r1, r1, #32

    /* r0 = r1 */
    mov r0, r1

    bx lr

    LEAF_END __rt_srsh

    END
/* EOF */
