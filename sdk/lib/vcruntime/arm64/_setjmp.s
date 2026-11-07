/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _setjmp for ARM64
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

    TEXTAREA

//
// int _setjmpex(_JUMP_BUFFER* _Env, void* _Frame);
//
    LEAF_ENTRY _setjmpex

    // Save the Frame passed in x1
    str     x1, [x0, #JbFrame]

    // Save the non-volatile registers
    stp     x19, x20, [x0, #JbX19]
    stp     x21, x22, [x0, #JbX21]
    stp     x23, x24, [x0, #JbX23]
    stp     x25, x26, [x0, #JbX25]
    stp     x27, x28, [x0, #JbX27]
    stp     fp, lr, [x0, #JbFp]
    mov     x2, sp
    str     x2, [x0, #JbSp]

    // Save floating point state
    mrs     x2, fpcr
    str     w2, [x0, #JbFpcr]
    mrs     x2, fpsr
    str     w2, [x0, #JbFpsr]
    stp     d8, d9, [x0, #JbD8]
    stp     d10, d11, [x0, #JbD10]
    stp     d12, d13, [x0, #JbD12]
    stp     d14, d15, [x0, #JbD14]

    // Return 0
    mov     w0, #0
    ret

    LEAF_END _setjmpex

    IMPORT __intrinsic_setjmpex, WEAK _setjmpex
    IMPORT _setjmp, WEAK _setjmpex
    IMPORT setjmp, WEAK _setjmpex

    END

/* EOF */
