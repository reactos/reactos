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


#include "main/glheader.h"
#include "main/imports.h"
#include "main/macros.h"

#include "m_vector.h"



/**
 * Given a vector [count][4] of floats, set all the [][elt] values
 * to 0 (if elt = 0, 1, 2) or 1.0 (if elt = 3).
 */
void
_mesa_vector4f_clean_elem( GLvector4f *vec, GLuint count, GLuint elt )
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

   for (i = 0; i < count; i++)
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


/**
 * Initialize GLvector objects.
 * \param v  the vector object to initialize.
 * \param flags  bitwise-OR of VEC_* flags
 * \param storage  pointer to storage for the vector's data
 */
void
_mesa_vector4f_init( GLvector4f *v, GLbitfield flags, GLfloat (*storage)[4] )
{
   v->stride = 4 * sizeof(GLfloat);
   v->size = 2;   /* may change: 2-4 for vertices and 1-4 for texcoords */
   v->data = storage;
   v->start = (GLfloat *) storage;
   v->count = 0;
   v->flags = size_bits[4] | flags;
}


/**
 * Initialize GLvector objects and allocate storage.
 * \param v  the vector object
 * \param flags  bitwise-OR of VEC_* flags
 * \param count  number of elements to allocate in vector
 * \param alignment  desired memory alignment for the data (in bytes)
 */
void
_mesa_vector4f_alloc( GLvector4f *v, GLbitfield flags, GLuint count,
                      GLuint alignment )
{
   v->stride = 4 * sizeof(GLfloat);
   v->size = 2;
   v->storage = _mesa_align_malloc( count * 4 * sizeof(GLfloat), alignment );
   v->storage_count = count;
   v->start = (GLfloat *) v->storage;
   v->data = (GLfloat (*)[4]) v->storage;
   v->count = 0;
   v->flags = size_bits[4] | flags | VEC_MALLOC;
}


/**
 * Vector deallocation.  Free whatever memory is pointed to by the
 * vector's storage field if the VEC_MALLOC flag is set.
 * DO NOT free the GLvector object itself, though.
 */
void
_mesa_vector4f_free( GLvector4f *v )
{
   if (v->flags & VEC_MALLOC) {
      _mesa_align_free( v->storage );
      v->data = NULL;
      v->start = NULL;
      v->storage = NULL;
      v->flags &= ~VEC_MALLOC;
   }
}


/**
 * For debugging
 */
void
_mesa_vector4f_print( const GLvector4f *v, const GLubyte *cullmask,
                      GLboolean culling )
{
   static const GLfloat c[4] = { 0, 0, 0, 1 };
   static const char *templates[5] = {
      "%d:\t0, 0, 0, 1\n",
      "%d:\t%f, 0, 0, 1\n",
      "%d:\t%f, %f, 0, 1\n",
      "%d:\t%f, %f, %f, 1\n",
      "%d:\t%f, %f, %f, %f\n"
   };

   const char *t = templates[v->size];
   GLfloat *d = (GLfloat *)v->data;
   GLuint j, i = 0, count;

   printf("data-start\n");
   for (; d != v->start; STRIDE_F(d, v->stride), i++)
      printf(t, i, d[0], d[1], d[2], d[3]);

   printf("start-count(%u)\n", v->count);
   count = i + v->count;

   if (culling) {
      for (; i < count; STRIDE_F(d, v->stride), i++)
	 if (cullmask[i])
	    printf(t, i, d[0], d[1], d[2], d[3]);
   }
   else {
      for (; i < count; STRIDE_F(d, v->stride), i++)
	 printf(t, i, d[0], d[1], d[2], d[3]);
   }

   for (j = v->size; j < 4; j++) {
      if ((v->flags & (1<<j)) == 0) {

	 printf("checking col %u is clean as advertised ", j);

	 for (i = 0, d = (GLfloat *) v->data;
	      i < count && d[j] == c[j];
	      i++, STRIDE_F(d, v->stride)) {
            /* no-op */
         }

	 if (i == count)
	    printf(" --> ok\n");
	 else
	    printf(" --> Failed at %u ******\n", i);
      }
   }
}
