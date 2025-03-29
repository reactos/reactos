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

    LEAF_ENTRY _setjmpex

    /* Store r1 (->Frame) and r4 - r11 */
    stmia r0!, {r1,r4-r11}

    /* Store sp (->Sp), lr (->Pc), fp (->Fpscr) */
    mov r1, sp
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
    LEAF_END _setjmpex

    IMPORT _setjmp, WEAK _setjmpex
    IMPORT setjmp, WEAK _setjmpex

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

    /* Check if r1 is zero */
    cmp r1, #0
    /* if r1 is zero, then set r0 to 1, else r0 = r1 */
    moveq r0, #1
    movne r0, r1
    bx lr
    LEAF_END longjmp

    END
/* EOF */
