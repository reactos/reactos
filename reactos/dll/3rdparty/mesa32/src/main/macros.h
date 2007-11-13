/**
 * \file macros.h
 * A collection of useful macros.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef MACROS_H
#define MACROS_H

#include "imports.h"


/**
 * \name Integer / float conversion for colors, normals, etc.
 */
/*@{*/

/** Convert GLubyte in [0,255] to GLfloat in [0.0,1.0] */
extern GLfloat _mesa_ubyte_to_float_color_tab[256];
#define UBYTE_TO_FLOAT(u) _mesa_ubyte_to_float_color_tab[(unsigned int)(u)]

/** Convert GLfloat in [0.0,1.0] to GLubyte in [0,255] */
#define FLOAT_TO_UBYTE(X)   ((GLubyte) (GLint) ((X) * 255.0F))


/** Convert GLbyte in [-128,127] to GLfloat in [-1.0,1.0] */
#define BYTE_TO_FLOAT(B)    ((2.0F * (B) + 1.0F) * (1.0F/255.0F))

/** Convert GLfloat in [-1.0,1.0] to GLbyte in [-128,127] */
#define FLOAT_TO_BYTE(X)    ( (((GLint) (255.0F * (X))) - 1) / 2 )


/** Convert GLushort in [0,65536] to GLfloat in [0.0,1.0] */
#define USHORT_TO_FLOAT(S)  ((GLfloat) (S) * (1.0F / 65535.0F))

/** Convert GLshort in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)   ((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

/** Convert GLfloat in [0.0,1.0] to GLshort in [-32768,32767] */
#define FLOAT_TO_SHORT(X)   ( (((GLint) (65535.0F * (X))) - 1) / 2 )


/** Convert GLuint in [0,4294967295] to GLfloat in [0.0,1.0] */
#define UINT_TO_FLOAT(U)    ((GLfloat) (U) * (1.0F / 4294967295.0F))

/** Convert GLfloat in [0.0,1.0] to GLuint in [0,4294967295] */
#define FLOAT_TO_UINT(X)    ((GLuint) ((X) * 4294967295.0))


/** Convert GLint in [-2147483648,2147483647] to GLfloat in [-1.0,1.0] */
#define INT_TO_FLOAT(I)     ((2.0F * (I) + 1.0F) * (1.0F/4294967294.0F))

/** Convert GLfloat in [-1.0,1.0] to GLint in [-2147483648,2147483647] */
/* causes overflow:
#define FLOAT_TO_INT(X)     ( (((GLint) (4294967294.0F * (X))) - 1) / 2 )
*/
/* a close approximation: */
#define FLOAT_TO_INT(X)     ( (GLint) (2147483647.0 * (X)) )


#define BYTE_TO_UBYTE(b)   ((GLubyte) ((b) < 0 ? 0 : (GLubyte) (b)))
#define SHORT_TO_UBYTE(s)  ((GLubyte) ((s) < 0 ? 0 : (GLubyte) ((s) >> 7)))
#define USHORT_TO_UBYTE(s) ((GLubyte) ((s) >> 8))
#define INT_TO_UBYTE(i)    ((GLubyte) ((i) < 0 ? 0 : (GLubyte) ((i) >> 23)))
#define UINT_TO_UBYTE(i)   ((GLubyte) ((i) >> 24))


#define BYTE_TO_USHORT(b)  ((b) < 0 ? 0 : ((GLushort) (((b) * 65535) / 255)))
#define UBYTE_TO_USHORT(b) (((GLushort) (b) << 8) | (GLushort) (b))
#define SHORT_TO_USHORT(s) ((s) < 0 ? 0 : ((GLushort) (((s) * 65535 / 32767))))
#define INT_TO_USHORT(i)   ((i) < 0 ? 0 : ((GLushort) ((i) >> 15)))
#define UINT_TO_USHORT(i)  ((i) < 0 ? 0 : ((GLushort) ((i) >> 16)))
#define UNCLAMPED_FLOAT_TO_USHORT(us, f)  \
        us = ( (GLushort) IROUND( CLAMP((f), 0.0, 1.0) * 65535.0F) )
#define CLAMPED_FLOAT_TO_USHORT(us, f)  \
        us = ( (GLushort) IROUND( (f) * 65535.0F) )

/*@}*/


/** Stepping a GLfloat pointer by a byte stride */
#define STRIDE_F(p, i)  (p = (GLfloat *)((GLubyte *)p + i))
/** Stepping a GLuint pointer by a byte stride */
#define STRIDE_UI(p, i)  (p = (GLuint *)((GLubyte *)p + i))
/** Stepping a GLubyte[4] pointer by a byte stride */
#define STRIDE_4UB(p, i)  (p = (GLubyte (*)[4])((GLubyte *)p + i))
/** Stepping a GLfloat[4] pointer by a byte stride */
#define STRIDE_4F(p, i)  (p = (GLfloat (*)[4])((GLubyte *)p + i))
/** Stepping a GLchan[4] pointer by a byte stride */
#define STRIDE_4CHAN(p, i)  (p = (GLchan (*)[4])((GLubyte *)p + i))
/** Stepping a GLchan pointer by a byte stride */
#define STRIDE_CHAN(p, i)  (p = (GLchan *)((GLubyte *)p + i))
/** Stepping a \p t pointer by a byte stride */
#define STRIDE_T(p, t, i)  (p = (t)((GLubyte *)p + i))


/**********************************************************************/
/** \name 4-element vector operations */
/*@{*/

/** Zero */
#define ZERO_4V( DST )  (DST)[0] = (DST)[1] = (DST)[2] = (DST)[3] = 0

/** Test for equality */
#define TEST_EQ_4V(a,b)  ((a)[0] == (b)[0] &&   \
              (a)[1] == (b)[1] &&   \
              (a)[2] == (b)[2] &&   \
              (a)[3] == (b)[3])

/** Test for equality (unsigned bytes) */
#if defined(__i386__)
#define TEST_EQ_4UBV(DST, SRC) *((GLuint*)(DST)) == *((GLuint*)(SRC))
#else
#define TEST_EQ_4UBV(DST, SRC) TEST_EQ_4V(DST, SRC)
#endif

/** Copy a 4-element vector */
#define COPY_4V( DST, SRC )         \
do {                                \
   (DST)[0] = (SRC)[0];             \
   (DST)[1] = (SRC)[1];             \
   (DST)[2] = (SRC)[2];             \
   (DST)[3] = (SRC)[3];             \
} while (0)

/** Copy a 4-element vector with cast */
#define COPY_4V_CAST( DST, SRC, CAST )  \
do {                                    \
   (DST)[0] = (CAST)(SRC)[0];           \
   (DST)[1] = (CAST)(SRC)[1];           \
   (DST)[2] = (CAST)(SRC)[2];           \
   (DST)[3] = (CAST)(SRC)[3];           \
} while (0)

/** Copy a 4-element unsigned byte vector */
#if defined(__i386__)
#define COPY_4UBV(DST, SRC)                 \
do {                                        \
   *((GLuint*)(DST)) = *((GLuint*)(SRC));   \
} while (0)
#else
/* The GLuint cast might fail if DST or SRC are not dword-aligned (RISC) */
#define COPY_4UBV(DST, SRC)         \
do {                                \
   (DST)[0] = (SRC)[0];             \
   (DST)[1] = (SRC)[1];             \
   (DST)[2] = (SRC)[2];             \
   (DST)[3] = (SRC)[3];             \
} while (0)
#endif

/**
 * Copy a 4-element float vector (avoid using FPU registers)
 * XXX Could use two 64-bit moves on 64-bit systems
 */
#define COPY_4FV( DST, SRC )                  \
do {                                          \
   const GLuint *_s = (const GLuint *) (SRC); \
   GLuint *_d = (GLuint *) (DST);             \
   _d[0] = _s[0];                             \
   _d[1] = _s[1];                             \
   _d[2] = _s[2];                             \
   _d[3] = _s[3];                             \
} while (0)

/** Copy \p SZ elements into a 4-element vector */
#define COPY_SZ_4V(DST, SZ, SRC)  \
do {                              \
   switch (SZ) {                  \
   case 4: (DST)[3] = (SRC)[3];   \
   case 3: (DST)[2] = (SRC)[2];   \
   case 2: (DST)[1] = (SRC)[1];   \
   case 1: (DST)[0] = (SRC)[0];   \
   }                              \
} while(0)

/** Copy \p SZ elements into a homegeneous (4-element) vector, giving
 * default values to the remaining */
#define COPY_CLEAN_4V(DST, SZ, SRC)  \
do {                                 \
      ASSIGN_4V( DST, 0, 0, 0, 1 );  \
      COPY_SZ_4V( DST, SZ, SRC );    \
} while (0)

/** Subtraction */
#define SUB_4V( DST, SRCA, SRCB )           \
do {                                        \
      (DST)[0] = (SRCA)[0] - (SRCB)[0];     \
      (DST)[1] = (SRCA)[1] - (SRCB)[1];     \
      (DST)[2] = (SRCA)[2] - (SRCB)[2];     \
      (DST)[3] = (SRCA)[3] - (SRCB)[3];     \
} while (0)

/** Addition */
#define ADD_4V( DST, SRCA, SRCB )           \
do {                                        \
      (DST)[0] = (SRCA)[0] + (SRCB)[0];     \
      (DST)[1] = (SRCA)[1] + (SRCB)[1];     \
      (DST)[2] = (SRCA)[2] + (SRCB)[2];     \
      (DST)[3] = (SRCA)[3] + (SRCB)[3];     \
} while (0)

/** Element-wise multiplication */
#define SCALE_4V( DST, SRCA, SRCB )         \
do {                                        \
      (DST)[0] = (SRCA)[0] * (SRCB)[0];     \
      (DST)[1] = (SRCA)[1] * (SRCB)[1];     \
      (DST)[2] = (SRCA)[2] * (SRCB)[2];     \
      (DST)[3] = (SRCA)[3] * (SRCB)[3];     \
} while (0)

/** In-place addition */
#define ACC_4V( DST, SRC )          \
do {                                \
      (DST)[0] += (SRC)[0];         \
      (DST)[1] += (SRC)[1];         \
      (DST)[2] += (SRC)[2];         \
      (DST)[3] += (SRC)[3];         \
} while (0)

/** Element-wise multiplication and addition */
#define ACC_SCALE_4V( DST, SRCA, SRCB )     \
do {                                        \
      (DST)[0] += (SRCA)[0] * (SRCB)[0];    \
      (DST)[1] += (SRCA)[1] * (SRCB)[1];    \
      (DST)[2] += (SRCA)[2] * (SRCB)[2];    \
      (DST)[3] += (SRCA)[3] * (SRCB)[3];    \
} while (0)

/** In-place scalar multiplication and addition */
#define ACC_SCALE_SCALAR_4V( DST, S, SRCB ) \
do {                                        \
      (DST)[0] += S * (SRCB)[0];            \
      (DST)[1] += S * (SRCB)[1];            \
      (DST)[2] += S * (SRCB)[2];            \
      (DST)[3] += S * (SRCB)[3];            \
} while (0)

/** Scalar multiplication */
#define SCALE_SCALAR_4V( DST, S, SRCB ) \
do {                                    \
      (DST)[0] = S * (SRCB)[0];         \
      (DST)[1] = S * (SRCB)[1];         \
      (DST)[2] = S * (SRCB)[2];         \
      (DST)[3] = S * (SRCB)[3];         \
} while (0)

/** In-place scalar multiplication */
#define SELF_SCALE_SCALAR_4V( DST, S ) \
do {                                   \
      (DST)[0] *= S;                   \
      (DST)[1] *= S;                   \
      (DST)[2] *= S;                   \
      (DST)[3] *= S;                   \
} while (0)

/** Assignment */
#define ASSIGN_4V( V, V0, V1, V2, V3 )  \
do {                                    \
    V[0] = V0;                          \
    V[1] = V1;                          \
    V[2] = V2;                          \
    V[3] = V3;                          \
} while(0)

/*@}*/


/**********************************************************************/
/** \name 3-element vector operations*/
/*@{*/

/** Zero */
#define ZERO_3V( DST )  (DST)[0] = (DST)[1] = (DST)[2] = 0

/** Test for equality */
#define TEST_EQ_3V(a,b)  \
   ((a)[0] == (b)[0] &&  \
    (a)[1] == (b)[1] &&  \
    (a)[2] == (b)[2])

/** Copy a 3-element vector */
#define COPY_3V( DST, SRC )         \
do {                                \
   (DST)[0] = (SRC)[0];             \
   (DST)[1] = (SRC)[1];             \
   (DST)[2] = (SRC)[2];             \
} while (0)

/** Copy a 3-element vector with cast */
#define COPY_3V_CAST( DST, SRC, CAST )  \
do {                                    \
   (DST)[0] = (CAST)(SRC)[0];           \
   (DST)[1] = (CAST)(SRC)[1];           \
   (DST)[2] = (CAST)(SRC)[2];           \
} while (0)

/** Copy a 3-element float vector */
#define COPY_3FV( DST, SRC )        \
do {                                \
   const GLfloat *_tmp = (SRC);     \
   (DST)[0] = _tmp[0];              \
   (DST)[1] = _tmp[1];              \
   (DST)[2] = _tmp[2];              \
} while (0)

/** Subtraction */
#define SUB_3V( DST, SRCA, SRCB )        \
do {                                     \
      (DST)[0] = (SRCA)[0] - (SRCB)[0];  \
      (DST)[1] = (SRCA)[1] - (SRCB)[1];  \
      (DST)[2] = (SRCA)[2] - (SRCB)[2];  \
} while (0)

/** Addition */
#define ADD_3V( DST, SRCA, SRCB )       \
do {                                    \
      (DST)[0] = (SRCA)[0] + (SRCB)[0]; \
      (DST)[1] = (SRCA)[1] + (SRCB)[1]; \
      (DST)[2] = (SRCA)[2] + (SRCB)[2]; \
} while (0)

/** In-place scalar multiplication */
#define SCALE_3V( DST, SRCA, SRCB )     \
do {                                    \
      (DST)[0] = (SRCA)[0] * (SRCB)[0]; \
      (DST)[1] = (SRCA)[1] * (SRCB)[1]; \
      (DST)[2] = (SRCA)[2] * (SRCB)[2]; \
} while (0)

/** In-place element-wise multiplication */
#define SELF_SCALE_3V( DST, SRC )   \
do {                                \
      (DST)[0] *= (SRC)[0];         \
      (DST)[1] *= (SRC)[1];         \
      (DST)[2] *= (SRC)[2];         \
} while (0)

/** In-place addition */
#define ACC_3V( DST, SRC )          \
do {                                \
      (DST)[0] += (SRC)[0];         \
      (DST)[1] += (SRC)[1];         \
      (DST)[2] += (SRC)[2];         \
} while (0)

/** Element-wise multiplication and addition */
#define ACC_SCALE_3V( DST, SRCA, SRCB )     \
do {                                        \
      (DST)[0] += (SRCA)[0] * (SRCB)[0];    \
      (DST)[1] += (SRCA)[1] * (SRCB)[1];    \
      (DST)[2] += (SRCA)[2] * (SRCB)[2];    \
} while (0)

/** Scalar multiplication */
#define SCALE_SCALAR_3V( DST, S, SRCB ) \
do {                                    \
      (DST)[0] = S * (SRCB)[0];         \
      (DST)[1] = S * (SRCB)[1];         \
      (DST)[2] = S * (SRCB)[2];         \
} while (0)

/** In-place scalar multiplication and addition */
#define ACC_SCALE_SCALAR_3V( DST, S, SRCB ) \
do {                                        \
      (DST)[0] += S * (SRCB)[0];            \
      (DST)[1] += S * (SRCB)[1];            \
      (DST)[2] += S * (SRCB)[2];            \
} while (0)

/** In-place scalar multiplication */
#define SELF_SCALE_SCALAR_3V( DST, S ) \
do {                                   \
      (DST)[0] *= S;                   \
      (DST)[1] *= S;                   \
      (DST)[2] *= S;                   \
} while (0)

/** In-place scalar addition */
#define ACC_SCALAR_3V( DST, S )     \
do {                                \
      (DST)[0] += S;                \
      (DST)[1] += S;                \
      (DST)[2] += S;                \
} while (0)

/** Assignment */
#define ASSIGN_3V( V, V0, V1, V2 )  \
do {                                \
    V[0] = V0;                      \
    V[1] = V1;                      \
    V[2] = V2;                      \
} while(0)

/*@}*/


/**********************************************************************/
/** \name 2-element vector operations*/
/*@{*/

/** Zero */
#define ZERO_2V( DST )  (DST)[0] = (DST)[1] = 0

/** Copy a 2-element vector */
#define COPY_2V( DST, SRC )         \
do {                        \
   (DST)[0] = (SRC)[0];             \
   (DST)[1] = (SRC)[1];             \
} while (0)

/** Copy a 2-element vector with cast */
#define COPY_2V_CAST( DST, SRC, CAST )      \
do {                        \
   (DST)[0] = (CAST)(SRC)[0];           \
   (DST)[1] = (CAST)(SRC)[1];           \
} while (0)

/** Copy a 2-element float vector */
#define COPY_2FV( DST, SRC )            \
do {                        \
   const GLfloat *_tmp = (SRC);         \
   (DST)[0] = _tmp[0];              \
   (DST)[1] = _tmp[1];              \
} while (0)

/** Subtraction */
#define SUB_2V( DST, SRCA, SRCB )       \
do {                        \
      (DST)[0] = (SRCA)[0] - (SRCB)[0];     \
      (DST)[1] = (SRCA)[1] - (SRCB)[1];     \
} while (0)

/** Addition */
#define ADD_2V( DST, SRCA, SRCB )       \
do {                        \
      (DST)[0] = (SRCA)[0] + (SRCB)[0];     \
      (DST)[1] = (SRCA)[1] + (SRCB)[1];     \
} while (0)

/** In-place scalar multiplication */
#define SCALE_2V( DST, SRCA, SRCB )     \
do {                        \
      (DST)[0] = (SRCA)[0] * (SRCB)[0];     \
      (DST)[1] = (SRCA)[1] * (SRCB)[1];     \
} while (0)

/** In-place addition */
#define ACC_2V( DST, SRC )          \
do {                        \
      (DST)[0] += (SRC)[0];         \
      (DST)[1] += (SRC)[1];         \
} while (0)

/** Element-wise multiplication and addition */
#define ACC_SCALE_2V( DST, SRCA, SRCB )     \
do {                        \
      (DST)[0] += (SRCA)[0] * (SRCB)[0];    \
      (DST)[1] += (SRCA)[1] * (SRCB)[1];    \
} while (0)

/** Scalar multiplication */
#define SCALE_SCALAR_2V( DST, S, SRCB )     \
do {                        \
      (DST)[0] = S * (SRCB)[0];         \
      (DST)[1] = S * (SRCB)[1];         \
} while (0)

/** In-place scalar multiplication and addition */
#define ACC_SCALE_SCALAR_2V( DST, S, SRCB ) \
do {                        \
      (DST)[0] += S * (SRCB)[0];        \
      (DST)[1] += S * (SRCB)[1];        \
} while (0)

/** In-place scalar multiplication */
#define SELF_SCALE_SCALAR_2V( DST, S )      \
do {                        \
      (DST)[0] *= S;                \
      (DST)[1] *= S;                \
} while (0)

/** In-place scalar addition */
#define ACC_SCALAR_2V( DST, S )         \
do {                        \
      (DST)[0] += S;                \
      (DST)[1] += S;                \
} while (0)

/** Assign scalers to short vectors */
#define ASSIGN_2V( V, V0, V1 )	\
do {				\
    V[0] = V0;			\
    V[1] = V1;			\
} while(0)

/*@}*/


/** \name Linear interpolation macros */
/*@{*/

/**
 * Linear interpolation
 *
 * \note \p OUT argument is evaluated twice!
 * \note Be wary of using *coord++ as an argument to any of these macros!
 */
#define LINTERP(T, OUT, IN) ((OUT) + (T) * ((IN) - (OUT)))

/* Can do better with integer math
 */
#define INTERP_UB( t, dstub, outub, inub )  \
do {                        \
   GLfloat inf = UBYTE_TO_FLOAT( inub );    \
   GLfloat outf = UBYTE_TO_FLOAT( outub );  \
   GLfloat dstf = LINTERP( t, outf, inf );  \
   UNCLAMPED_FLOAT_TO_UBYTE( dstub, dstf ); \
} while (0)

#define INTERP_CHAN( t, dstc, outc, inc )   \
do {                        \
   GLfloat inf = CHAN_TO_FLOAT( inc );      \
   GLfloat outf = CHAN_TO_FLOAT( outc );    \
   GLfloat dstf = LINTERP( t, outf, inf );  \
   UNCLAMPED_FLOAT_TO_CHAN( dstc, dstf );   \
} while (0)

#define INTERP_UI( t, dstui, outui, inui )  \
   dstui = (GLuint) (GLint) LINTERP( (t), (GLfloat) (outui), (GLfloat) (inui) )

#define INTERP_F( t, dstf, outf, inf )      \
   dstf = LINTERP( t, outf, inf )

#define INTERP_4F( t, dst, out, in )        \
do {                        \
   dst[0] = LINTERP( (t), (out)[0], (in)[0] );  \
   dst[1] = LINTERP( (t), (out)[1], (in)[1] );  \
   dst[2] = LINTERP( (t), (out)[2], (in)[2] );  \
   dst[3] = LINTERP( (t), (out)[3], (in)[3] );  \
} while (0)

#define INTERP_3F( t, dst, out, in )        \
do {                        \
   dst[0] = LINTERP( (t), (out)[0], (in)[0] );  \
   dst[1] = LINTERP( (t), (out)[1], (in)[1] );  \
   dst[2] = LINTERP( (t), (out)[2], (in)[2] );  \
} while (0)

#define INTERP_4CHAN( t, dst, out, in )         \
do {                            \
   INTERP_CHAN( (t), (dst)[0], (out)[0], (in)[0] ); \
   INTERP_CHAN( (t), (dst)[1], (out)[1], (in)[1] ); \
   INTERP_CHAN( (t), (dst)[2], (out)[2], (in)[2] ); \
   INTERP_CHAN( (t), (dst)[3], (out)[3], (in)[3] ); \
} while (0)

#define INTERP_3CHAN( t, dst, out, in )         \
do {                            \
   INTERP_CHAN( (t), (dst)[0], (out)[0], (in)[0] ); \
   INTERP_CHAN( (t), (dst)[1], (out)[1], (in)[1] ); \
   INTERP_CHAN( (t), (dst)[2], (out)[2], (in)[2] ); \
} while (0)

#define INTERP_SZ( t, vec, to, out, in, sz )                \
do {                                    \
   switch (sz) {                            \
   case 4: vec[to][3] = LINTERP( (t), (vec)[out][3], (vec)[in][3] );    \
   case 3: vec[to][2] = LINTERP( (t), (vec)[out][2], (vec)[in][2] );    \
   case 2: vec[to][1] = LINTERP( (t), (vec)[out][1], (vec)[in][1] );    \
   case 1: vec[to][0] = LINTERP( (t), (vec)[out][0], (vec)[in][0] );    \
   }                                    \
} while(0)

/*@}*/



/** Clamp X to [MIN,MAX] */
#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )

/** Assign X to CLAMP(X, MIN, MAX) */
#define CLAMP_SELF(x, mn, mx)  \
   ( (x)<(mn) ? ((x) = (mn)) : ((x)>(mx) ? ((x)=(mx)) : (x)) )



/** Minimum of two values: */
#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )

/** Maximum of two values: */
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

/** Dot product of two 2-element vectors */
#define DOT2( a, b )  ( (a)[0]*(b)[0] + (a)[1]*(b)[1] )

/** Dot product of two 3-element vectors */
#define DOT3( a, b )  ( (a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] )

/** Dot product of two 4-element vectors */
#define DOT4( a, b )  ( (a)[0]*(b)[0] + (a)[1]*(b)[1] + \
            (a)[2]*(b)[2] + (a)[3]*(b)[3] )

/** Dot product of two 4-element vectors */
#define DOT4V(v,a,b,c,d) (v[0]*(a) + v[1]*(b) + v[2]*(c) + v[3]*(d))


/** Cross product of two 3-element vectors */
#define CROSS3(n, u, v)             \
do {                        \
   (n)[0] = (u)[1]*(v)[2] - (u)[2]*(v)[1];  \
   (n)[1] = (u)[2]*(v)[0] - (u)[0]*(v)[2];  \
   (n)[2] = (u)[0]*(v)[1] - (u)[1]*(v)[0];  \
} while (0)


/* Normalize a 3-element vector to unit length. */
#define NORMALIZE_3FV( V )          \
do {                        \
   GLfloat len = (GLfloat) LEN_SQUARED_3FV(V);  \
   if (len) {                   \
      len = INV_SQRTF(len);         \
      (V)[0] = (GLfloat) ((V)[0] * len);    \
      (V)[1] = (GLfloat) ((V)[1] * len);    \
      (V)[2] = (GLfloat) ((V)[2] * len);    \
   }                        \
} while(0)

#define LEN_3FV( V ) (SQRTF((V)[0]*(V)[0]+(V)[1]*(V)[1]+(V)[2]*(V)[2]))
#define LEN_2FV( V ) (SQRTF((V)[0]*(V)[0]+(V)[1]*(V)[1]))

#define LEN_SQUARED_3FV( V ) ((V)[0]*(V)[0]+(V)[1]*(V)[1]+(V)[2]*(V)[2])
#define LEN_SQUARED_2FV( V ) ((V)[0]*(V)[0]+(V)[1]*(V)[1])


#endif
