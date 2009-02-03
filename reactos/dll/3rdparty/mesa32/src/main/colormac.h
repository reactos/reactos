/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file colormac.h
 * Color-related macros
 */


#ifndef COLORMAC_H
#define COLORMAC_H


#include "imports.h"
#include "config.h"
#include "macros.h"


/** \def BYTE_TO_CHAN
 * Convert from GLbyte to GLchan */

/** \def UBYTE_TO_CHAN
 * Convert from GLubyte to GLchan */

/** \def SHORT_TO_CHAN
 * Convert from GLshort to GLchan */

/** \def USHORT_TO_CHAN
 * Convert from GLushort to GLchan */

/** \def INT_TO_CHAN
 * Convert from GLint to GLchan */

/** \def UINT_TO_CHAN
 * Convert from GLuint to GLchan */

/** \def CHAN_TO_UBYTE
 * Convert from GLchan to GLubyte */

/** \def CHAN_TO_FLOAT
 * Convert from GLchan to GLfloat */

/** \def CLAMPED_FLOAT_TO_CHAN
 * Convert from GLclampf to GLchan */

/** \def UNCLAMPED_FLOAT_TO_CHAN
 * Convert from GLfloat to GLchan */

/** \def COPY_CHAN4
 * Copy a GLchan[4] array */

/** \def CHAN_PRODUCT
 * Scaled product (usually approximated) between two GLchan arguments */

#if CHAN_BITS == 8

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (GLchan) (b))
#define UBYTE_TO_CHAN(b)  (b)
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) ((s) >> 7))
#define USHORT_TO_CHAN(s) ((GLchan) ((s) >> 8))
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 23))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 24))

#define CHAN_TO_UBYTE(c)  (c)
#define CHAN_TO_FLOAT(c)  UBYTE_TO_FLOAT(c)

#define CLAMPED_FLOAT_TO_CHAN(c, f)    CLAMPED_FLOAT_TO_UBYTE(c, f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)  UNCLAMPED_FLOAT_TO_UBYTE(c, f)

#define COPY_CHAN4(DST, SRC)  COPY_4UBV(DST, SRC)

#define CHAN_PRODUCT(a, b)  ((GLubyte) (((GLint)(a) * ((GLint)(b) + 1)) >> 8))

#elif CHAN_BITS == 16

#define BYTE_TO_CHAN(b)   ((b) < 0 ? 0 : (((GLchan) (b)) * 516))
#define UBYTE_TO_CHAN(b)  ((((GLchan) (b)) << 8) | ((GLchan) (b)))
#define SHORT_TO_CHAN(s)  ((s) < 0 ? 0 : (GLchan) (s))
#define USHORT_TO_CHAN(s) (s)
#define INT_TO_CHAN(i)    ((i) < 0 ? 0 : (GLchan) ((i) >> 15))
#define UINT_TO_CHAN(i)   ((GLchan) ((i) >> 16))

#define CHAN_TO_UBYTE(c)  ((c) >> 8)
#define CHAN_TO_FLOAT(c)  ((GLfloat) ((c) * (1.0 / CHAN_MAXF)))

#define CLAMPED_FLOAT_TO_CHAN(c, f)    CLAMPED_FLOAT_TO_USHORT(c, f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)  UNCLAMPED_FLOAT_TO_USHORT(c, f)

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b) ((GLchan) ((((GLuint) (a)) * ((GLuint) (b))) / 65535))

#elif CHAN_BITS == 32

/* XXX floating-point color channels not fully thought-out */
#define BYTE_TO_CHAN(b)   ((GLfloat) ((b) * (1.0F / 127.0F)))
#define UBYTE_TO_CHAN(b)  ((GLfloat) ((b) * (1.0F / 255.0F)))
#define SHORT_TO_CHAN(s)  ((GLfloat) ((s) * (1.0F / 32767.0F)))
#define USHORT_TO_CHAN(s) ((GLfloat) ((s) * (1.0F / 65535.0F)))
#define INT_TO_CHAN(i)    ((GLfloat) ((i) * (1.0F / 2147483647.0F)))
#define UINT_TO_CHAN(i)   ((GLfloat) ((i) * (1.0F / 4294967295.0F)))

#define CHAN_TO_UBYTE(c)  FLOAT_TO_UBYTE(c)
#define CHAN_TO_FLOAT(c)  (c)

#define CLAMPED_FLOAT_TO_CHAN(c, f)  c = (f)
#define UNCLAMPED_FLOAT_TO_CHAN(c, f)      c = (f)

#define COPY_CHAN4(DST, SRC)  COPY_4V(DST, SRC)

#define CHAN_PRODUCT(a, b)    ((a) * (b))

#else

#error unexpected CHAN_BITS size

#endif


/**
 * Convert 3 channels at once.
 *
 * \param dst pointer to destination GLchan[3] array.
 * \param f pointer to source GLfloat[3] array.
 *
 * \sa #UNCLAMPED_FLOAT_TO_CHAN.
 */
#define UNCLAMPED_FLOAT_TO_RGB_CHAN(dst, f)	\
do {						\
   UNCLAMPED_FLOAT_TO_CHAN(dst[0], f[0]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[1], f[1]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[2], f[2]);	\
} while (0)


/**
 * Convert 4 channels at once.
 *
 * \param dst pointer to destination GLchan[4] array.
 * \param f pointer to source GLfloat[4] array.
 *
 * \sa #UNCLAMPED_FLOAT_TO_CHAN.
 */
#define UNCLAMPED_FLOAT_TO_RGBA_CHAN(dst, f)	\
do {						\
   UNCLAMPED_FLOAT_TO_CHAN(dst[0], f[0]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[1], f[1]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[2], f[2]);	\
   UNCLAMPED_FLOAT_TO_CHAN(dst[3], f[3]);	\
} while (0)



/**
 * \name Generic color packing macros.  All inputs should be GLubytes.
 *
 * \todo We may move these into texstore.h at some point.
 */
/*@{*/

#define PACK_COLOR_8888( X, Y, Z, W ) \
   (((X) << 24) | ((Y) << 16) | ((Z) << 8) | (W))

#define PACK_COLOR_8888_REV( X, Y, Z, W ) \
   (((W) << 24) | ((Z) << 16) | ((Y) << 8) | (X))

#define PACK_COLOR_888( X, Y, Z ) \
   (((X) << 16) | ((Y) << 8) | (Z))

#define PACK_COLOR_565( X, Y, Z )                                  \
   ((((X) & 0xf8) << 8) | (((Y) & 0xfc) << 3) | (((Z) & 0xf8) >> 3))

#define PACK_COLOR_565_REV( X, Y, Z ) \
   (((X) & 0xf8) | ((Y) & 0xe0) >> 5 | (((Y) & 0x1c) << 11) | (((Z) & 0xf8) << 5))

#define PACK_COLOR_5551( R, G, B, A )					\
   ((((R) & 0xf8) << 8) | (((G) & 0xf8) << 3) | (((B) & 0xf8) >> 2) |	\
    ((A) ? 1 : 0))

#define PACK_COLOR_1555( A, B, G, R )					\
   ((((B) & 0xf8) << 7) | (((G) & 0xf8) << 2) | (((R) & 0xf8) >> 3) |	\
    ((A) ? 0x8000 : 0))

#define PACK_COLOR_1555_REV( A, B, G, R )					\
   ((((B) & 0xf8) >> 1) | (((G) & 0xc0) >> 6) | (((G) & 0x38) << 10) | (((R) & 0xf8) << 5) |	\
    ((A) ? 0x80 : 0))

#define PACK_COLOR_4444( R, G, B, A )					\
   ((((R) & 0xf0) << 8) | (((G) & 0xf0) << 4) | ((B) & 0xf0) | ((A) >> 4))

#define PACK_COLOR_4444_REV( R, G, B, A )				\
   ((((B) & 0xf0) << 8) | (((A) & 0xf0) << 4) | ((R) & 0xf0) | ((G) >> 4))

#define PACK_COLOR_88( L, A )						\
   (((L) << 8) | (A))

#define PACK_COLOR_88_REV( L, A )					\
   (((A) << 8) | (L))

#define PACK_COLOR_332( R, G, B )					\
   (((R) & 0xe0) | (((G) & 0xe0) >> 3) | (((B) & 0xc0) >> 6))

#define PACK_COLOR_233( B, G, R )					\
   (((B) & 0xc0) | (((G) & 0xe0) >> 2) | (((R) & 0xe0) >> 5))

/*@}*/


#endif /* COLORMAC_H */
