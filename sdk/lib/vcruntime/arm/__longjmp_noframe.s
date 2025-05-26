/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __longjmp_noframe for ARM
 * COPYRIGHT:   Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <kxarm.h>

    TEXTAREA

    LEAF_ENTRY __longjmp_noframe

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

    LEAF_END __longjmp_noframe

    END

/* EOF */
