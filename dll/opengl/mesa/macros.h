/* $Id: macros.h,v 1.12 1998/01/06 01:42:29 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: macros.h,v $
 * Revision 1.12  1998/01/06 01:42:29  brianp
 * small patch for BeOS
 *
 * Revision 1.11  1997/06/23 00:23:23  brianp
 * changed preprocessor test for Macintosh
 *
 * Revision 1.10  1997/06/06 03:47:31  brianp
 * added COPY_4UBV()
 *
 * Revision 1.9  1997/05/01 01:39:15  brianp
 * moved NORMALIZE_3V macro to mmath.h and renamed NORMALIZE_3FV
 *
 * Revision 1.8  1997/04/24 00:28:45  brianp
 * added COPY_2V macro
 *
 * Revision 1.7  1997/04/07 02:59:46  brianp
 * added ASSIGN_2V macro
 *
 * Revision 1.6  1997/04/01 04:08:16  brianp
 * changed DEFARRAY, UNDEFARRAY for Mac, per Miklos Fazekas
 * added DEG2RAD macro
 *
 * Revision 1.5  1997/03/21 02:01:17  brianp
 * added ASSERT() macro
 *
 * Revision 1.4  1997/02/03 20:31:26  brianp
 * added DEFARRAY and UNDEFARRAY macros for BeOS
 *
 * Revision 1.3  1996/10/31 01:12:00  brianp
 * removed (GLint) from FLOAT_TO_UINT() macro
 *
 * Revision 1.2  1996/09/15 01:49:44  brianp
 * added #define NULL 0
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * A collection of useful macros.
 */


#ifndef MACROS_H
#define MACROS_H


#include <math.h>
#include <string.h>


#ifdef DEBUG
#  include <assert.h>
#  define ASSERT(X)   assert(X)
#else
#  define ASSERT(X)
#endif


/* Limits: */
#define MAX_GLUSHORT	0xffff
#define MAX_GLUINT	0xffffffff



/* Copy short vectors: */

#define COPY_2V( DST, SRC )	DST[0] = SRC[0];	\
				DST[1] = SRC[1];

#define COPY_3V( DST, SRC )	DST[0] = SRC[0];	\
				DST[1] = SRC[1];	\
				DST[2] = SRC[2];

#define COPY_4V( DST, SRC )	DST[0] = SRC[0];	\
				DST[1] = SRC[1];	\
				DST[2] = SRC[2];	\
				DST[3] = SRC[3];

/*
 * Copy a vector of 4 GLubytes from SRC to DST.
 */
#define COPY_4UBV(DST, SRC)			\
   if (sizeof(GLuint)==4*sizeof(GLubyte)) {	\
      *((GLuint*)(DST)) = *((GLuint*)(SRC));	\
   }						\
   else {					\
      (DST)[0] = (SRC)[0];			\
      (DST)[1] = (SRC)[1];			\
      (DST)[2] = (SRC)[2];			\
      (DST)[3] = (SRC)[3];			\
   }



/* Assign scalers to short vectors: */
#define ASSIGN_2V( V, V0, V1 )  V[0] = V0;  V[1] = V1;

#define ASSIGN_3V( V, V0, V1, V2 )  V[0] = V0;  V[1] = V1;  V[2] = V2;

#define ASSIGN_4V( V, V0, V1, V2, V3 )  V[0] = V0;	\
				        V[1] = V1;	\
				        V[2] = V2;	\
				        V[3] = V3;


/* Test if we're inside a glBegin / glEnd pair: */
#define INSIDE_BEGIN_END(CTX)  ((CTX)->Primitive!=GL_BITMAP)



/* Absolute value (for Int, Float, Double): */
#define ABSI(X)  ((X) < 0 ? -(X) : (X))
#define ABSF(X)  ((X) < 0.0F ? -(X) : (X))
#define ABSD(X)  ((X) < 0.0 ? -(X) : (X))



/* Round a floating-point value to the nearest integer: */
#define ROUNDF(X)  ( (X)<0.0F ? ((GLint) ((X)-0.5F)) : ((GLint) ((X)+0.5F)) )


/* Compute ceiling of integer quotient of A divided by B: */
#define CEILING( A, B )  ( (A) % (B) == 0 ? (A)/(B) : (A)/(B)+1 )


/* Clamp X to [MIN,MAX]: */
#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )


/* Min of two values: */
#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )


/* MAX of two values: */
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )


/* Dot product of two 3-element vectors */
#define DOT3( a, b )  ( a[0]*b[0] + a[1]*b[1] + a[2]*b[2] )


/* Dot product of two 4-element vectors */
#define DOT4( a, b )  ( a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3] )



/*
 * Integer / float conversion for colors, normals, etc.
 */

/* Convert GLubyte in [0,255] to GLfloat in [0.0,1.0] */
#define UBYTE_TO_FLOAT(B)	((GLfloat) (B) * (1.0F / 255.0F))

/* Convert GLfloat in [0.0,1.0] to GLubyte in [0,255] */
#define FLOAT_TO_UBYTE(X)	((GLubyte) (GLint) (((X)) * 255.0F))


/* Convert GLbyte in [-128,127] to GLfloat in [-1.0,1.0] */
#define BYTE_TO_FLOAT(B)	((2.0F * (B) + 1.0F) * (1.0F/255.0F))

/* Convert GLfloat in [-1.0,1.0] to GLbyte in [-128,127] */
#define FLOAT_TO_BYTE(X)	( (((GLint) (255.0F * (X))) - 1) / 2 )


/* Convert GLushort in [0,65536] to GLfloat in [0.0,1.0] */
#define USHORT_TO_FLOAT(S)	((GLfloat) (S) * (1.0F / 65535.0F))

/* Convert GLfloat in [0.0,1.0] to GLushort in [0,65536] */
#define FLOAT_TO_USHORT(X)	((GLushort) (GLint) ((X) * 65535.0F))


/* Convert GLshort in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)	((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

/* Convert GLfloat in [0.0,1.0] to GLshort in [-32768,32767] */
#define FLOAT_TO_SHORT(X)	( (((GLint) (65535.0F * (X))) - 1) / 2 )


/* Convert GLuint in [0,4294967295] to GLfloat in [0.0,1.0] */
#define UINT_TO_FLOAT(U)	((GLfloat) (U) * (1.0F / 4294967295.0F))

/* Convert GLfloat in [0.0,1.0] to GLuint in [0,4294967295] */
#define FLOAT_TO_UINT(X)	((GLuint) ((X) * 4294967295.0))


/* Convert GLint in [-2147483648,2147483647] to GLfloat in [-1.0,1.0] */
#define INT_TO_FLOAT(I)		((2.0F * (I) + 1.0F) * (1.0F/4294967294.0F))

/* Convert GLfloat in [-1.0,1.0] to GLint in [-2147483648,2147483647] */
/* causes overflow:
#define FLOAT_TO_INT(X)		( (((GLint) (4294967294.0F * (X))) - 1) / 2 )
*/
/* a close approximation: */
#define FLOAT_TO_INT(X)		( (GLint) (2147483647.0 * (X)) )



/* Memory copy: */
#ifdef SUNOS4
#define MEMCPY( DST, SRC, BYTES) \
	memcpy( (char *) (DST), (char *) (SRC), (int) (BYTES) )
#else
#define MEMCPY( DST, SRC, BYTES) \
	memcpy( (void *) (DST), (void *) (SRC), (size_t) (BYTES) )
#endif


/* Memory set: */
#ifdef SUNOS4
#define MEMSET( DST, VAL, N ) \
	memset( (char *) (DST), (int) (VAL), (int) (N) )
#else
#define MEMSET( DST, VAL, N ) \
	memset( (void *) (DST), (int) (VAL), (size_t) (N) )
#endif


/* MACs and BeOS don't support static larger than 32kb, so... */
#if defined(macintosh) && !defined(__MRC__)
  extern char *AGLAlloc(int size);
  extern void AGLFree(char* ptr);
#  define DEFARRAY(TYPE,NAME,SIZE)  TYPE *NAME = (TYPE*)AGLAlloc(sizeof(TYPE)*(SIZE))
#  define UNDEFARRAY(NAME)          AGLFree((char*)NAME)
#elif defined(__BEOS__)
#  define DEFARRAY(TYPE,NAME,SIZE)  TYPE *NAME = (TYPE*)malloc(sizeof(TYPE)*(SIZE))
#  define UNDEFARRAY(NAME)          free(NAME)
#else
#  define DEFARRAY(TYPE,NAME,SIZE)  TYPE NAME[SIZE]
#  define UNDEFARRAY(NAME)
#endif


/* Pi */
#ifndef M_PI
#define M_PI (3.1415926)
#endif


/* Degrees to radians conversion: */
#define DEG2RAD (M_PI/180.0)


#ifndef NULL
#define NULL 0
#endif



#endif /*MACROS_H*/
