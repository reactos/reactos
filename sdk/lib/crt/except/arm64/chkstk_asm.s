/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __chkstk for ARM64
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <kxarm64.h>

/* CODE **********************************************************************/

    TEXTAREA

    LEAF_ENTRY __chkstk

    // x17 = page the allocation ends in, page 0 if the size wraps
    mov     x16, sp
    subs    x17, x16, x15, lsl #4
    csel    x17, xzr, x17, lo
    and     x17, x17, #0xFFFFFFFFFFFFF000

    // x16 = page sp is in
    and     x16, x16, #0xFFFFFFFFFFFFF000

LABEL(ChkstkProbePage)
    cmp     x16, x17
    bls     ChkstkDone
    sub     x16, x16, #1, lsl #12
    ldr     xzr, [x16]
    b       ChkstkProbePage

LABEL(ChkstkDone)
    ret

    LEAF_END __chkstk

    END
/* EOF */
