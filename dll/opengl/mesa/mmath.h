/* $Id: mmath.h,v 1.5 1997/12/18 02:54:26 brianp Exp $ */

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
 * $Log: mmath.h,v $
 * Revision 1.5  1997/12/18 02:54:26  brianp
 * added Josh Vanderhoof's float->int x86 asm code
 *
 * Revision 1.4  1997/10/15 00:36:17  brianp
 * renamed the FAST/REGULAR_MATH macros
 *
 * Revision 1.3  1997/09/29 22:23:54  brianp
 * added FAST/REGULAR_MATH macros
 *
 * Revision 1.2  1997/05/03 00:54:31  brianp
 * fixed a comment
 *
 * Revision 1.1  1997/05/01 01:42:38  brianp
 * Initial revision
 *
 */


/*
 * Faster arithmetic functions.  If the FAST_MATH preprocessor symbol is
 * defined on the command line (-DFAST_MATH) then we'll use some (hopefully)
 * faster functions for sqrt(), etc.
 */


#ifndef MMATH_H
#define MMATH_H

#include <math.h>

#define FloatToInt(F) lrintf((F))

extern float gl_sqrt(float x);

#ifdef FAST_MATH
#  define GL_SQRT(X)  gl_sqrt(X)
#else
#  define GL_SQRT(X)  sqrt(X)
#endif


/* Normalize a 3-element vector to unit length */
#define NORMALIZE_3FV( V )				\
{							\
   GLfloat len;						\
   len = GL_SQRT(V[0]*V[0]+V[1]*V[1]+V[2]*V[2]);	\
   if (len>0.0001F) {					\
      len = 1.0F / len;					\
      V[0] *= len;					\
      V[1] *= len;					\
      V[2] *= len;					\
   }							\
}


extern void gl_init_math(void);


#endif
