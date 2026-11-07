/*
 * Adapted from fdlibm
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* INCLUDES ******************************************************************/

#include <kxarm64.h>

/* CODE **********************************************************************/

    TEXTAREA

//
// double atan2(double y, double x)
//
    LEAF_ENTRY atan2

    // Either argument NaN gives NaN
    fcmp    d0, d1
    bvs     Atan2Nan

    adr     x15, Atan2Table
    fmov    x9, d0
    fmov    x10, d1
    and     x11, x9, #0x7FFFFFFFFFFFFFFF    // |y| bits
    and     x12, x10, #0x7FFFFFFFFFFFFFFF   // |x| bits

    // x13 bit 0 = y negative, bit 1 = x negative
    lsr     x13, x9, #63
    lsr     x14, x10, #63
    orr     x13, x13, x14, lsl #1

    mov     x16, #0x7FF0000000000000
    cbz     x11, Atan2YZero
    cbz     x12, Atan2PiOver2
    cmp     x12, x16
    beq     Atan2XInf
    cmp     x11, x16
    beq     Atan2PiOver2

    // Exponent gap between |y| and |x|
    sub     x14, x11, x12
    asr     x14, x14, #52
    cmp     x14, #60
    bgt     Atan2Huge
    tbz     x13, #1, Atan2Ratio
    cmn     x14, #60
    blt     Atan2Tiny

LABEL(Atan2Ratio)
    fabs    d2, d0
    fabs    d3, d1
    fdiv    d2, d2, d3

    // x14 = high word of t = |y / x|, x16 = reduction index or -1
    fmov    x14, d2
    lsr     x14, x14, #32
    mov     w17, #0x3FDC0000
    cmp     w14, w17
    bhs     Atan2Reduce
    mov     w17, #0x3E400000
    cmp     w14, w17
    blo     Atan2Quadrant
    mov     x16, #-1
    b       Atan2Poly

LABEL(Atan2Reduce)
    fmov    d4, #1.0
    mov     w17, #0x3FF30000
    cmp     w14, w17
    bhs     Atan2ReduceHigh
    mov     w17, #0x3FE60000
    cmp     w14, w17
    bhs     Atan2ReduceOne

    // t in [7/16, 11/16): atan(1/2) + atan((2t - 1) / (2 + t))
    mov     x16, #0
    fadd    d5, d2, d2
    fsub    d5, d5, d4
    fmov    d6, #2.0
    fadd    d6, d6, d2
    fdiv    d2, d5, d6
    b       Atan2Poly

LABEL(Atan2ReduceOne)
    // t in [11/16, 19/16): atan(1) + atan((t - 1) / (t + 1))
    mov     x16, #1
    fsub    d5, d2, d4
    fadd    d6, d2, d4
    fdiv    d2, d5, d6
    b       Atan2Poly

LABEL(Atan2ReduceHigh)
    movz    w17, #0x4003, lsl #16
    movk    w17, #0x8000
    cmp     w14, w17
    bhs     Atan2ReduceInf

    // t in [19/16, 39/16): atan(3/2) + atan((t - 3/2) / (1 + 3/2 t))
    mov     x16, #2
    fmov    d5, #1.5
    fsub    d6, d2, d5
    fmul    d5, d5, d2
    fadd    d5, d4, d5
    fdiv    d2, d6, d5
    b       Atan2Poly

LABEL(Atan2ReduceInf)
    // t >= 39/16: atan(inf) + atan(-1 / t)
    mov     x16, #3
    fmov    d5, #-1.0
    fdiv    d2, d5, d2

LABEL(Atan2Poly)
    // d3 = z = t^2, d4 = w = z^2
    fmul    d3, d2, d2
    fmul    d4, d3, d3

    // d5 = z * (T0 + w * (T2 + w * (T4 + w * (T6 + w * (T8 + w * T10)))))
    ldr     d5, [x15, #80]
    ldr     d6, [x15, #64]
    fmul    d5, d4, d5
    fadd    d5, d6, d5
    ldr     d6, [x15, #48]
    fmul    d5, d4, d5
    fadd    d5, d6, d5
    ldr     d6, [x15, #32]
    fmul    d5, d4, d5
    fadd    d5, d6, d5
    ldr     d6, [x15, #16]
    fmul    d5, d4, d5
    fadd    d5, d6, d5
    ldr     d6, [x15, #0]
    fmul    d5, d4, d5
    fadd    d5, d6, d5
    fmul    d5, d3, d5

    // d6 = w * (T1 + w * (T3 + w * (T5 + w * (T7 + w * T9))))
    ldr     d6, [x15, #72]
    ldr     d7, [x15, #56]
    fmul    d6, d4, d6
    fadd    d6, d7, d6
    ldr     d7, [x15, #40]
    fmul    d6, d4, d6
    fadd    d6, d7, d6
    ldr     d7, [x15, #24]
    fmul    d6, d4, d6
    fadd    d6, d7, d6
    ldr     d7, [x15, #8]
    fmul    d6, d4, d6
    fadd    d6, d7, d6
    fmul    d6, d4, d6

    // d5 = t * (s1 + s2)
    fadd    d5, d5, d6
    fmul    d5, d2, d5
    tbnz    x16, #63, Atan2PolyDirect

    // atan(t) = hi - ((t * (s1 + s2) - lo) - t)
    add     x17, x15, x16, lsl #3
    ldr     d6, [x17, #120]
    ldr     d7, [x17, #88]
    fsub    d5, d5, d6
    fsub    d5, d5, d2
    fsub    d2, d7, d5
    b       Atan2Quadrant

LABEL(Atan2PolyDirect)
    fsub    d2, d2, d5
    b       Atan2Quadrant

LABEL(Atan2Huge)
    // |y / x| > 2^60, the result is pi/2 with the sign of y
    ldr     d2, [x15, #152]
    ldr     d3, [x15, #168]
    fmov    d4, #0.5
    fmul    d3, d3, d4
    fadd    d2, d2, d3
    and     x13, x13, #1
    b       Atan2Quadrant

LABEL(Atan2Tiny)
    // x < 0 and |y / x| < 2^-60
    fmov    d2, xzr

LABEL(Atan2Quadrant)
    // d2 = atan(|y / x|), place it in the quadrant of (x, y)
    ldr     d6, [x15, #168]
    ldr     d7, [x15, #160]
    cmp     x13, #2
    beq     Atan2Quadrant2
    bhi     Atan2Quadrant3
    fmov    d0, d2
    tbz     x13, #0, Atan2Done
    fneg    d0, d0
    ret

LABEL(Atan2Quadrant2)
    // pi - (z - pi_lo)
    fsub    d2, d2, d6
    fsub    d0, d7, d2
    ret

LABEL(Atan2Quadrant3)
    // (z - pi_lo) - pi
    fsub    d2, d2, d6
    fsub    d0, d2, d7
    ret

LABEL(Atan2YZero)
    // y = +-0 keeps y for x >= +0 and gives +-pi otherwise
    tbz     x13, #1, Atan2Done
    ldr     d0, [x15, #160]
    b       Atan2SignOfY

LABEL(Atan2PiOver2)
    // x = +-0 or y infinite with x finite
    ldr     d0, [x15, #152]
    b       Atan2SignOfY

LABEL(Atan2XInf)
    cmp     x11, x16
    beq     Atan2BothInf
    fmov    d0, xzr
    tbz     x13, #1, Atan2SignOfY
    ldr     d0, [x15, #160]
    b       Atan2SignOfY

LABEL(Atan2BothInf)
    ldr     d0, [x15, #176]
    tbz     x13, #1, Atan2SignOfY
    ldr     d0, [x15, #184]

LABEL(Atan2SignOfY)
    tbz     x13, #0, Atan2Done
    fneg    d0, d0
    ret

LABEL(Atan2Nan)
    fadd    d0, d0, d1

LABEL(Atan2Done)
    ret

    ALIGN 8
LABEL(Atan2Table)
    // atan polynomial T0 to T10
    dcq     0x3FD555555555550D
    dcq     0xBFC999999998EBC4
    dcq     0x3FC24924920083FF
    dcq     0xBFBC71C6FE231671
    dcq     0x3FB745CDC54C206E
    dcq     0xBFB3B0F2AF749A6D
    dcq     0x3FB10D66A0D03D51
    dcq     0xBFADDE2D52DEFD9A
    dcq     0x3FA97B4B24760DEB
    dcq     0xBFA2B4442C6A6C2F
    dcq     0x3F90AD3AE322DA11

    // atan(1/2), atan(1), atan(3/2), atan(inf), high parts
    dcq     0x3FDDAC670561BB4F
    dcq     0x3FE921FB54442D18
    dcq     0x3FEF730BD281F69B
    dcq     0x3FF921FB54442D18

    // and their low parts
    dcq     0x3C7A2B7F222F65E2
    dcq     0x3C81A62633145C07
    dcq     0x3C7007887AF0CBBD
    dcq     0x3C91A62633145C07

    // pi/2, pi, pi_lo, pi/4, 3pi/4
    dcq     0x3FF921FB54442D18
    dcq     0x400921FB54442D18
    dcq     0x3CA1A62633145C07
    dcq     0x3FE921FB54442D18
    dcq     0x4002D97C7F3321D2

    LEAF_END atan2

    END
/* EOF */
