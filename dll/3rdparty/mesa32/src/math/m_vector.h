
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


#ifndef _M_VECTOR_H_
#define _M_VECTOR_H_

#include "glheader.h"
#include "mtypes.h"		/* hack for GLchan */


#define VEC_DIRTY_0        0x1
#define VEC_DIRTY_1        0x2
#define VEC_DIRTY_2        0x4
#define VEC_DIRTY_3        0x8
#define VEC_MALLOC         0x10 /* storage field points to self-allocated mem*/
#define VEC_NOT_WRITEABLE  0x40	/* writable elements to hold clipped data */
#define VEC_BAD_STRIDE     0x100 /* matches tnl's prefered stride */


#define VEC_SIZE_1   VEC_DIRTY_0
#define VEC_SIZE_2   (VEC_DIRTY_0|VEC_DIRTY_1)
#define VEC_SIZE_3   (VEC_DIRTY_0|VEC_DIRTY_1|VEC_DIRTY_2)
#define VEC_SIZE_4   (VEC_DIRTY_0|VEC_DIRTY_1|VEC_DIRTY_2|VEC_DIRTY_3)



/* Wrap all the information about vectors up in a struct.  Has
 * additional fields compared to the other vectors to help us track of
 * different vertex sizes, and whether we need to clean columns out
 * because they contain non-(0,0,0,1) values.
 *
 * The start field is used to reserve data for copied vertices at the
 * end of _mesa_transform_vb, and avoids the need for a multiplication in
 * the transformation routines.
 */
typedef struct {
   GLfloat (*data)[4];	/* may be malloc'd or point to client data */
   GLfloat *start;	/* points somewhere inside of <data> */
   GLuint count;	/* size of the vector (in elements) */
   GLuint stride;	/* stride from one element to the next (in bytes) */
   GLuint size;		/* 2-4 for vertices and 1-4 for texcoords */
   GLuint flags;	/* which columns are dirty */
   void *storage;	/* self-allocated storage */
} GLvector4f;


extern void _mesa_vector4f_init( GLvector4f *v, GLuint flags,
			      GLfloat (*storage)[4] );
extern void _mesa_vector4f_alloc( GLvector4f *v, GLuint flags,
			       GLuint count, GLuint alignment );
extern void _mesa_vector4f_free( GLvector4f *v );
extern void _mesa_vector4f_print( GLvector4f *v, GLubyte *, GLboolean );
extern void _mesa_vector4f_clean_elem( GLvector4f *vec, GLuint nr, GLuint elt );





/*
 * Given vector <v>, return a pointer (cast to <type *> to the <i>-th element.
 *
 * End up doing a lot of slow imuls if not careful.
 */
#define VEC_ELT( v, type, i ) \
       ( (type *)  ( ((GLbyte *) ((v)->data)) + (i) * (v)->stride) )


#endif
