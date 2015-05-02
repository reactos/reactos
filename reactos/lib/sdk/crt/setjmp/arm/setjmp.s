/*
 * COPYRIGHT:         BSD - See COPYING.ARM in the top level directory
 * PROJECT:           ReactOS CRT library
 * PURPOSE:           Implementation of _setjmp / longjmp
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY _setjmp

    mov r1, sp

    /* Store r1 (->Frame) and r4 - r11 */
    stmia r0!, {r1,r4-r11}

    /* Store r1 (->Sp), lr (->Pc), fp (->Fpscr) */
    stmia r0!, {r1,lr,fp}

    /* Store NEON registers */
    vst1.64 {d0}, [r0]!
    vst1.64 {d1}, [r0]!
    vst1.64 {d2}, [r0]!
    vst1.64 {d3}, [r0]!
    vst1.64 {d4}, [r0]!
    vst1.64 {d5}, [r0]!
    vst1.64 {d6}, [r0]!
    vst1.64 {d7}, [r0]!

    /* Return 0 */
    mov r0, #0
    bx lr
    LEAF_END _setjmp

    LEAF_ENTRY longjmp

    ldmia r0!, {r1,r4-r11}
    ldmia r0!, {r1,lr,fp}
    mov sp, r1

    /* Load NEON registers */
    vld1.64 {d0}, [r0]!
    vld1.64 {d1}, [r0]!
    vld1.64 {d2}, [r0]!
    vld1.64 {d3}, [r0]!
    vld1.64 {d4}, [r0]!
    vld1.64 {d5}, [r0]!
    vld1.64 {d6}, [r0]!
    vld1.64 {d7}, [r0]!

    /* Return 1 */
    mov r0, #1
    bx lr
    LEAF_END longjmp

    END
/* EOF */
