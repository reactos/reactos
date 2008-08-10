
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


#include "glheader.h"
#include "imports.h"
#include "macros.h"
#include "imports.h"

#include "m_vector.h"



/*
 * Given a vector [count][4] of floats, set all the [][elt] values
 * to 0 (if elt = 0, 1, 2) or 1.0 (if elt = 3).
 */
void _mesa_vector4f_clean_elem( GLvector4f *vec, GLuint count, GLuint elt )
{
   static const GLubyte elem_bits[4] = {
      VEC_DIRTY_0,
      VEC_DIRTY_1,
      VEC_DIRTY_2,
      VEC_DIRTY_3
   };
   static const GLfloat clean[4] = { 0, 0, 0, 1 };
   const GLfloat v = clean[elt];
   GLfloat (*data)[4] = (GLfloat (*)[4])vec->start;
   GLuint i;

   for (i = 0 ; i < count ; i++)
      data[i][elt] = v;

   vec->flags &= ~elem_bits[elt];
}

static const GLubyte size_bits[5] = {
   0,
   VEC_SIZE_1,
   VEC_SIZE_2,
   VEC_SIZE_3,
   VEC_SIZE_4,
};



/*
 * Initialize GLvector objects.
 * Input: v - the vector object to initialize.
 *        flags - bitwise-OR of VEC_* flags
 *        storage - pointer to storage for the vector's data
 */


void _mesa_vector4f_init( GLvector4f *v, GLuint flags, GLfloat (*storage)[4] )
{
   v->stride = 4 * sizeof(GLfloat);
   v->size = 2;   /* may change: 2-4 for vertices and 1-4 for texcoords */
   v->data = storage;
   v->start = (GLfloat *) storage;
   v->count = 0;
   v->flags = size_bits[4] | flags ;
}




/*
 * Initialize GLvector objects and allocate storage.
 * Input: v - the vector object
 *        sz - unused????
 *        flags - bitwise-OR of VEC_* flags
 *        count - number of elements to allocate in vector
 *        alignment - desired memory alignment for the data (in bytes)
 */


void _mesa_vector4f_alloc( GLvector4f *v, GLuint flags, GLuint count,
			GLuint alignment )
{
   v->stride = 4 * sizeof(GLfloat);
   v->size = 2;
   v->storage = ALIGN_MALLOC( count * 4 * sizeof(GLfloat), alignment );
   v->start = (GLfloat *) v->storage;
   v->data = (GLfloat (*)[4]) v->storage;
   v->count = 0;
   v->flags = size_bits[4] | flags | VEC_MALLOC ;
}




/*
 * Vector deallocation.  Free whatever memory is pointed to by the
 * vector's storage field if the VEC_MALLOC flag is set.
 * DO NOT free the GLvector object itself, though.
 */


void _mesa_vector4f_free( GLvector4f *v )
{
   if (v->flags & VEC_MALLOC) {
      ALIGN_FREE( v->storage );
      v->data = NULL;
      v->start = NULL;
      v->storage = NULL;
      v->flags &= ~VEC_MALLOC;
   }
}


/*
 * For debugging
 */
void _mesa_vector4f_print( GLvector4f *v, GLubyte *cullmask, GLboolean culling )
{
   GLfloat c[4] = { 0, 0, 0, 1 };
   const char *templates[5] = {
      "%d:\t0, 0, 0, 1\n",
      "%d:\t%f, 0, 0, 1\n",
      "%d:\t%f, %f, 0, 1\n",
      "%d:\t%f, %f, %f, 1\n",
      "%d:\t%f, %f, %f, %f\n"
   };

   const char *t = templates[v->size];
   GLfloat *d = (GLfloat *)v->data;
   GLuint j, i = 0, count;

   _mesa_printf("data-start\n");
   for ( ; d != v->start ; STRIDE_F(d, v->stride), i++)
      _mesa_printf(t, i, d[0], d[1], d[2], d[3]);

   _mesa_printf("start-count(%u)\n", v->count);
   count = i + v->count;

   if (culling) {
      for ( ; i < count ; STRIDE_F(d, v->stride), i++)
	 if (cullmask[i])
	    _mesa_printf(t, i, d[0], d[1], d[2], d[3]);
   }
   else {
      for ( ; i < count ; STRIDE_F(d, v->stride), i++)
	 _mesa_printf(t, i, d[0], d[1], d[2], d[3]);
   }

   for (j = v->size ; j < 4; j++) {
      if ((v->flags & (1<<j)) == 0) {

	 _mesa_printf("checking col %u is clean as advertised ", j);

	 for (i = 0, d = (GLfloat *) v->data ;
	      i < count && d[j] == c[j] ;
	      i++, STRIDE_F(d, v->stride)) {};

	 if (i == count)
	    _mesa_printf(" --> ok\n");
	 else
	    _mesa_printf(" --> Failed at %u ******\n", i);
      }
   }
}


