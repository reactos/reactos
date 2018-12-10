/* $Id: mmath.c,v 1.3 1997/07/24 01:23:16 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
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
 * $Log: mmath.c,v $
 * Revision 1.3  1997/07/24 01:23:16  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.2  1997/05/28 03:25:43  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.1  1997/05/01 01:41:54  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "GL/gl.h"
#include "mmath.h"
#endif



/*
 * A High Speed, Low Precision Square Root
 * by Paul Lalonde and Robert Dawson
 * from "Graphics Gems", Academic Press, 1990
 */

/*
 * SPARC implementation of a fast square root by table 
 * lookup.
 * SPARC floating point format is as follows:
 *
 * BIT 31 	30 	23 	22 	0
 *     sign	exponent	mantissa
 */
static short sqrttab[0x100];    /* declare table of square roots */

static void init_sqrt(void)
{
#ifdef FAST_MATH
   unsigned short i;
   float f;
   unsigned int *fi = (unsigned int *)&f;
                                /* to access the bits of a float in  */
                                /* C quickly we must misuse pointers */

   for(i=0; i<= 0x7f; i++) {
      *fi = 0;

      /*
       * Build a float with the bit pattern i as mantissa
       * and an exponent of 0, stored as 127
       */

      *fi = (i << 16) | (127 << 23);
      f = sqrt(f);

      /*
       * Take the square root then strip the first 7 bits of
       * the mantissa into the table
       */

      sqrttab[i] = (*fi & 0x7fffff) >> 16;

      /*
       * Repeat the process, this time with an exponent of
       * 1, stored as 128
       */

      *fi = 0;
      *fi = (i << 16) | (128 << 23);
      f = sqrt(f);
      sqrttab[i+0x80] = (*fi & 0x7fffff) >> 16;
   }
#endif /*FAST_MATH*/
}


float gl_sqrt( float x )
{
#ifdef FAST_MATH
   unsigned int *num = (unsigned int *)&x;
                                /* to access the bits of a float in C
                                 * we must misuse pointers */
                                                        
   short e;                     /* the exponent */
   if (x == 0.0F) return 0.0F;  /* check for square root of 0 */
   e = (*num >> 23) - 127;      /* get the exponent - on a SPARC the */
                                /* exponent is stored with 127 added */
   *num &= 0x7fffff;            /* leave only the mantissa */
   if (e & 0x01) *num |= 0x800000;
                                /* the exponent is odd so we have to */
                                /* look it up in the second half of  */
                                /* the lookup table, so we set the   */
                                /* high bit                                */
   e >>= 1;                     /* divide the exponent by two */
                                /* note that in C the shift */
                                /* operators are sign preserving */
                                /* for signed operands */
   /* Do the table lookup, based on the quaternary mantissa,
    * then reconstruct the result back into a float
    */
   *num = ((sqrttab[*num >> 16]) << 16) | ((e + 127) << 23);
   return x;
#else
   return sqrt(x);
#endif
}



/*
 * Initialize tables, etc for fast math functions.
 */
void gl_init_math(void)
{
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      init_sqrt();


      initialized = GL_TRUE;
   }
}
