/**
 * \file colormac.h
 * Color-related macros
 */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 * \name Generic color packing macros
 *
 * \todo We may move these into texutil.h at some point.
 */
/*@{*/

#define PACK_COLOR_8888( a, b, c, d )					\
   (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define PACK_COLOR_888( a, b, c )					\
   (((a) << 16) | ((b) << 8) | (c))

#define PACK_COLOR_565( a, b, c )					\
   ((((a) & 0xf8) << 8) | (((b) & 0xfc) << 3) | (((c) & 0xf8) >> 3))

#define PACK_COLOR_1555( a, b, c, d )					\
   ((((b) & 0xf8) << 7) | (((c) & 0xf8) << 2) | (((d) & 0xf8) >> 3) |	\
    ((a) ? 0x8000 : 0))

#define PACK_COLOR_4444( a, b, c, d )					\
   ((((a) & 0xf0) << 8) | (((b) & 0xf0) << 4) | ((c) & 0xf0) | ((d) >> 4))

#define PACK_COLOR_88( a, b )						\
   (((a) << 8) | (b))

#define PACK_COLOR_332( a, b, c )					\
   (((a) & 0xe0) | (((b) & 0xe0) >> 3) | (((c) & 0xc0) >> 6))


#ifdef MESA_BIG_ENDIAN

#define PACK_COLOR_8888_LE( a, b, c, d )	PACK_COLOR_8888( d, c, b, a )

#define PACK_COLOR_565_LE( a, b, c )					\
   (((a) & 0xf8) | (((b) & 0xe0) >> 5) | (((b) & 0x1c) << 11) |		\
   (((c) & 0xf8) << 5))

#define PACK_COLOR_1555_LE( a, b, c, d )				\
   ((((b) & 0xf8) >> 1) | (((c) & 0xc0) >> 6) | (((c) & 0x38) << 10) |	\
    (((d) & 0xf8) << 5) | ((a) ? 0x80 : 0))

#define PACK_COLOR_4444_LE( a, b, c, d )	PACK_COLOR_4444( c, d, a, b )

#define PACK_COLOR_88_LE( a, b )		PACK_COLOR_88( b, a )

#else	/* little endian */

#define PACK_COLOR_8888_LE( a, b, c, d )	PACK_COLOR_8888( a, b, c, d )

#define PACK_COLOR_565_LE( a, b, c )		PACK_COLOR_565( a, b, c )

#define PACK_COLOR_1555_LE( a, b, c, d )	PACK_COLOR_1555( a, b, c, d )

#define PACK_COLOR_4444_LE( a, b, c, d )	PACK_COLOR_4444( a, b, c, d )

#define PACK_COLOR_88_LE( a, b )		PACK_COLOR_88( a, b )

#endif	/* endianness */

/*@}*/


#endif /* COLORMAC_H */
