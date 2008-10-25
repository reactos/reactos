/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _SAVAGE_SPAN_H
#define _SAVAGE_SPAN_H

#include "drirenderbuffer.h"


extern void savageDDInitSpanFuncs( GLcontext *ctx );

extern void
savageSetSpanFunctions(driRenderbuffer *rb, const GLvisual *vis,
                       GLboolean float_depth);


/*
 * Savage 16-bit float depth format with zExpOffset=16:
 *   4 bit unsigned exponent, 12 bit mantissa
 *
 * The meaning of the mantissa is different from IEEE floatint point
 * formats. The same number can't be encoded with different exponents.
 * So no bits are wasted.
 *
 * exponent | range encoded by mantissa | accuracy or mantissa
 * ---------+---------------------------+---------------------
 *       15 | 2^-1 .. 1                 | 2^-13
 *       14 | 2^-2 .. 2^-1              | 2^-14
 *       13 | 2^-3 .. 2^-2              | 2^-15
 *      ... | ...                       |
 *        2 | 2^-14 .. 2^-13            | 2^-27
 *        1 | 2^-15 .. 2^-14            | 2^-27
 *        0 | 2^-16 .. 2^-15            | 2^-28
 *
 * Note that there is no encoding for numbers < 2^-16.
 */
static __inline GLuint savageEncodeFloat16( GLdouble x )
{
    GLint r = (GLint)(x * 0x10000000);
    GLint exp = 0;
    if (r < 0x1000)
	return 0;
    while (r - 0x1000 > 0x0fff) {
	r >>= 1;
	exp++;
    }
    return exp > 0xf ? 0xffff : (r - 0x1000) | (exp << 12);
}
static __inline GLdouble savageDecodeFloat16( GLuint x )
{
    static const GLdouble pow2[16] = {
	1.0/(1<<28), 1.0/(1<<27), 1.0/(1<<26), 1.0/(1<<25),
	1.0/(1<<24), 1.0/(1<<23), 1.0/(1<<22), 1.0/(1<<21),
	1.0/(1<<20), 1.0/(1<<19), 1.0/(1<<18), 1.0/(1<<17),
	1.0/(1<<16), 1.0/(1<<15), 1.0/(1<<14), 1.0/(1<<13)
    };
    static const GLdouble bias[16] = {
	1.0/(1<<16), 1.0/(1<<15), 1.0/(1<<14), 1.0/(1<<13),
	1.0/(1<<12), 1.0/(1<<11), 1.0/(1<<10), 1.0/(1<< 9),
	1.0/(1<< 8), 1.0/(1<< 7), 1.0/(1<< 6), 1.0/(1<< 5),
	1.0/(1<< 4), 1.0/(1<< 3), 1.0/(1<< 2), 1.0/(1<< 1)
    };
    GLuint mant = x & 0x0fff;
    GLuint exp = (x >> 12) & 0xf;
    return bias[exp] + pow2[exp]*mant;
}

/*
 * Savage 24-bit float depth format with zExpOffset=32:
 *   5 bit unsigned exponent, 19 bit mantissa
 *
 * Details analogous to the 16-bit format.
 */
static __inline GLuint savageEncodeFloat24( GLdouble x )
{
    int64_t r = (int64_t)(x * ((int64_t)1 << (19+32)));
    GLint exp = 0;
    if (r < 0x80000)
	return 0;
    while (r - 0x80000 > 0x7ffff) {
	r >>= 1;
	exp++;
    }
    return exp > 0x1f ? 0xffffff : (r - 0x80000) | (exp << 19);
}
#define _1 (int64_t)1
static __inline GLdouble savageDecodeFloat24( GLuint x )
{
    static const GLdouble pow2[32] = {
	1.0/(_1<<51), 1.0/(_1<<50), 1.0/(_1<<49), 1.0/(_1<<48),
	1.0/(_1<<47), 1.0/(_1<<46), 1.0/(_1<<45), 1.0/(_1<<44),
	1.0/(_1<<43), 1.0/(_1<<42), 1.0/(_1<<41), 1.0/(_1<<40),
	1.0/(_1<<39), 1.0/(_1<<38), 1.0/(_1<<37), 1.0/(_1<<36),
	1.0/(_1<<35), 1.0/(_1<<34), 1.0/(_1<<33), 1.0/(_1<<32),
	1.0/(_1<<31), 1.0/(_1<<30), 1.0/(_1<<29), 1.0/(_1<<28),
	1.0/(_1<<27), 1.0/(_1<<26), 1.0/(_1<<25), 1.0/(_1<<24),
	1.0/(_1<<23), 1.0/(_1<<22), 1.0/(_1<<21), 1.0/(_1<<20)
    };
    static const GLdouble bias[32] = {
	1.0/(_1<<32), 1.0/(_1<<31), 1.0/(_1<<30), 1.0/(_1<<29),
	1.0/(_1<<28), 1.0/(_1<<27), 1.0/(_1<<26), 1.0/(_1<<25),
	1.0/(_1<<24), 1.0/(_1<<23), 1.0/(_1<<22), 1.0/(_1<<21),
	1.0/(_1<<20), 1.0/(_1<<19), 1.0/(_1<<18), 1.0/(_1<<17),
	1.0/(_1<<16), 1.0/(_1<<15), 1.0/(_1<<14), 1.0/(_1<<13),
	1.0/(_1<<12), 1.0/(_1<<11), 1.0/(_1<<10), 1.0/(_1<< 9),
	1.0/(_1<< 8), 1.0/(_1<< 7), 1.0/(_1<< 6), 1.0/(_1<< 5),
	1.0/(_1<< 4), 1.0/(_1<< 3), 1.0/(_1<< 2), 1.0/(_1<< 1)
    };
    GLuint mant = x & 0x7ffff;
    GLuint exp = (x >> 19) & 0x1f;
    return bias[exp] + pow2[exp]*mant;
}
#undef _1


#endif
