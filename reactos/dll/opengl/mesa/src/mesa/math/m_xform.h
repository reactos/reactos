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


#ifndef _M_XFORM_H
#define _M_XFORM_H


#include "main/compiler.h"
#include "main/glheader.h"
#include "math/m_matrix.h"
#include "math/m_vector.h"

#ifdef USE_X86_ASM
#define _XFORMAPI _ASMAPI
#define _XFORMAPIP _ASMAPIP
#else
#define _XFORMAPI
#define _XFORMAPIP *
#endif


extern void
_math_init_transformation(void);
extern void
init_c_cliptest(void);

/* KW: Clip functions now do projective divide as well.  The projected
 * coordinates are very useful to us because they let us cull
 * backfaces and eliminate vertices from lighting, fogging, etc
 * calculations.  Despite the fact that this divide could be done one
 * day in hardware, we would still have a reason to want to do it here
 * as long as those other calculations remain in software.
 *
 * Clipping is a convenient place to do the divide on x86 as it should be
 * possible to overlap with integer outcode calculations.
 *
 * There are two cases where we wouldn't want to do the divide in cliptest:
 *    - When we aren't clipping.  We still might want to cull backfaces
 *      so the divide should be done elsewhere.  This currently never
 *      happens.
 *
 *    - When culling isn't likely to help us, such as when the GL culling
 *      is disabled and we not lighting or are only lighting
 *      one-sided.  In this situation, backface determination provides
 *      us with no useful information.  A tricky case to detect is when
 *      all input data is already culled, although hopefully the
 *      application wouldn't turn on culling in such cases.
 *
 * We supply a buffer to hold the [x/w,y/w,z/w,1/w] values which
 * are the result of the projection.  This is only used in the
 * 4-vector case - in other cases, we just use the clip coordinates
 * as the projected coordinates - they are identical.
 *
 * This is doubly convenient because it means the Win[] array is now
 * of the same stride as all the others, so I can now turn map_vertices
 * into a straight-forward matrix transformation, with asm acceleration
 * automatically available.
 */

/* Vertex buffer clipping flags
 */
#define CLIP_RIGHT_SHIFT 	0
#define CLIP_LEFT_SHIFT 	1
#define CLIP_TOP_SHIFT  	2
#define CLIP_BOTTOM_SHIFT       3
#define CLIP_NEAR_SHIFT  	4
#define CLIP_FAR_SHIFT  	5

#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
#define CLIP_USER_BIT    0x40
#define CLIP_CULL_BIT    0x80
#define CLIP_FRUSTUM_BITS    0x3f


typedef GLvector4f * (_XFORMAPIP clip_func)( GLvector4f *vClip,
					     GLvector4f *vProj,
					     GLubyte clipMask[],
					     GLubyte *orMask,
					     GLubyte *andMask,
					     GLboolean viewport_z_clip );

typedef void (*dotprod_func)( GLfloat *out,
			      GLuint out_stride,
			      CONST GLvector4f *coord_vec,
			      CONST GLfloat plane[4] );

typedef void (*vec_copy_func)( GLvector4f *to,
			       CONST GLvector4f *from );



/*
 * Functions for transformation of normals in the VB.
 */
typedef void (_NORMAPIP normal_func)( CONST GLmatrix *mat,
				      GLfloat scale,
				      CONST GLvector4f *in,
				      CONST GLfloat lengths[],
				      GLvector4f *dest );


/* Flags for selecting a normal transformation function.
 */
#define NORM_RESCALE   0x1		/* apply the scale factor */
#define NORM_NORMALIZE 0x2		/* normalize */
#define NORM_TRANSFORM 0x4		/* apply the transformation matrix */
#define NORM_TRANSFORM_NO_ROT 0x8	/* apply the transformation matrix */




/* KW: New versions of the transform function allow a mask array
 *     specifying that individual vector transform should be skipped
 *     when the mask byte is zero.  This is always present as a
 *     parameter, to allow a unified interface.
 */
typedef void (_XFORMAPIP transform_func)( GLvector4f *to_vec,
					  CONST GLfloat m[16],
					  CONST GLvector4f *from_vec );


extern dotprod_func  _mesa_dotprod_tab[5];
extern vec_copy_func _mesa_copy_tab[0x10];
extern vec_copy_func _mesa_copy_clean_tab[5];
extern clip_func     _mesa_clip_tab[5];
extern clip_func     _mesa_clip_np_tab[5];
extern normal_func   _mesa_normal_tab[0xf];

/* Use of 2 layers of linked 1-dimensional arrays to reduce
 * cost of lookup.
 */
extern transform_func *_mesa_transform_tab[5];



#define TransformRaw( to, mat, from ) \
   ( _mesa_transform_tab[(from)->size][(mat)->type]( to, (mat)->m, from ), \
     (to) )


#endif
