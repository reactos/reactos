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


#include "config.h"
#include "macros.h"
#include "mtypes.h"


/**
 * Convert four float values in [0,1] to ubytes in [0,255] with clamping.
 */
static inline void
_mesa_unclamped_float_rgba_to_ubyte(GLubyte dst[4], const GLfloat src[4])
{
   int i;
   for (i = 0; i < 4; i++)
      UNCLAMPED_FLOAT_TO_UBYTE(dst[i], src[i]);
}


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
    ((A) >> 7))

#define PACK_COLOR_1555( A, B, G, R )					\
   ((((B) & 0xf8) << 7) | (((G) & 0xf8) << 2) | (((R) & 0xf8) >> 3) |	\
    (((A) & 0x80) << 8))

#define PACK_COLOR_1555_REV( A, B, G, R )					\
   ((((B) & 0xf8) >> 1) | (((G) & 0xc0) >> 6) | (((G) & 0x38) << 10) | (((R) & 0xf8) << 5) |	\
    ((A) ? 0x80 : 0))

#define PACK_COLOR_2101010_UB( A, B, G, R )					\
   (((B) << 22) | ((G) << 12) | ((R) << 2) |	\
    (((A) & 0xc0) << 24))

#define PACK_COLOR_2101010_US( A, B, G, R )					\
   ((((B) >> 6) << 20) | (((G) >> 6) << 10) | ((R) >> 6) |	\
    (((A) >> 14) << 30))

#define PACK_COLOR_4444( R, G, B, A )					\
   ((((R) & 0xf0) << 8) | (((G) & 0xf0) << 4) | ((B) & 0xf0) | ((A) >> 4))

#define PACK_COLOR_4444_REV( R, G, B, A )				\
   ((((B) & 0xf0) << 8) | (((A) & 0xf0) << 4) | ((R) & 0xf0) | ((G) >> 4))

#define PACK_COLOR_44( L, A )						\
   (((L) & 0xf0) | (((A) & 0xf0) >> 4))

#define PACK_COLOR_88( L, A )						\
   (((L) << 8) | (A))

#define PACK_COLOR_88_REV( L, A )					\
   (((A) << 8) | (L))

#define PACK_COLOR_1616( L, A )						\
   (((L) << 16) | (A))

#define PACK_COLOR_1616_REV( L, A )					\
   (((A) << 16) | (L))

#define PACK_COLOR_332( R, G, B )					\
   (((R) & 0xe0) | (((G) & 0xe0) >> 3) | (((B) & 0xc0) >> 6))

#define PACK_COLOR_233( B, G, R )					\
   (((B) & 0xc0) | (((G) & 0xe0) >> 2) | (((R) & 0xe0) >> 5))

/*@}*/


#endif /* COLORMAC_H */
