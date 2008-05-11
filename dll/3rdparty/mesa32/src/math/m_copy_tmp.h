
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

/*
 * New (3.1) transformation code written by Keith Whitwell.
 */


#define COPY_FUNC( BITS )						\
static void TAG2(copy, BITS)( GLvector4f *to, const GLvector4f *f )	\
{									\
   GLfloat (*t)[4] = (GLfloat (*)[4])to->start;				\
   GLfloat *from = f->start;						\
   GLuint stride = f->stride;				        	\
   GLuint count = to->count;						\
   GLuint i;								\
									\
   if (BITS)								\
      STRIDE_LOOP {							\
	 if (BITS&1) t[i][0] = from[0];					\
	 if (BITS&2) t[i][1] = from[1];					\
	 if (BITS&4) t[i][2] = from[2];					\
	 if (BITS&8) t[i][3] = from[3];					\
      }									\
}

/* We got them all here:
 */
COPY_FUNC( 0x0 )		/* noop */
COPY_FUNC( 0x1 )
COPY_FUNC( 0x2 )
COPY_FUNC( 0x3 )
COPY_FUNC( 0x4 )
COPY_FUNC( 0x5 )
COPY_FUNC( 0x6 )
COPY_FUNC( 0x7 )
COPY_FUNC( 0x8 )
COPY_FUNC( 0x9 )
COPY_FUNC( 0xa )
COPY_FUNC( 0xb )
COPY_FUNC( 0xc )
COPY_FUNC( 0xd )
COPY_FUNC( 0xe )
COPY_FUNC( 0xf )

static void TAG2(init_copy, 0)( void )
{
   _mesa_copy_tab[0x0] = TAG2(copy, 0x0);
   _mesa_copy_tab[0x1] = TAG2(copy, 0x1);
   _mesa_copy_tab[0x2] = TAG2(copy, 0x2);
   _mesa_copy_tab[0x3] = TAG2(copy, 0x3);
   _mesa_copy_tab[0x4] = TAG2(copy, 0x4);
   _mesa_copy_tab[0x5] = TAG2(copy, 0x5);
   _mesa_copy_tab[0x6] = TAG2(copy, 0x6);
   _mesa_copy_tab[0x7] = TAG2(copy, 0x7);
   _mesa_copy_tab[0x8] = TAG2(copy, 0x8);
   _mesa_copy_tab[0x9] = TAG2(copy, 0x9);
   _mesa_copy_tab[0xa] = TAG2(copy, 0xa);
   _mesa_copy_tab[0xb] = TAG2(copy, 0xb);
   _mesa_copy_tab[0xc] = TAG2(copy, 0xc);
   _mesa_copy_tab[0xd] = TAG2(copy, 0xd);
   _mesa_copy_tab[0xe] = TAG2(copy, 0xe);
   _mesa_copy_tab[0xf] = TAG2(copy, 0xf);
}
