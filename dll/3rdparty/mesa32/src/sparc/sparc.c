/*
 * Mesa 3-D graphics library
 * Version:  6.3
 * 
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 * Sparc assembly code by David S. Miller
 */


#include "sparc.h"

#ifdef USE_SPARC_ASM

#include "context.h"
#include "math/m_xform.h"
#include "tnl/t_context.h"

#ifdef DEBUG
#include "math/m_debug.h"
#endif

#define XFORM_ARGS 	GLvector4f *to_vec, 		\
			const GLfloat m[16], 		\
			const GLvector4f *from_vec

#define DECLARE_XFORM_GROUP(pfx, sz)					   \
 extern void _mesa_##pfx##_transform_points##sz##_general(XFORM_ARGS);     \
 extern void _mesa_##pfx##_transform_points##sz##_identity(XFORM_ARGS);    \
 extern void _mesa_##pfx##_transform_points##sz##_3d_no_rot(XFORM_ARGS);   \
 extern void _mesa_##pfx##_transform_points##sz##_perspective(XFORM_ARGS); \
 extern void _mesa_##pfx##_transform_points##sz##_2d(XFORM_ARGS);          \
 extern void _mesa_##pfx##_transform_points##sz##_2d_no_rot(XFORM_ARGS);   \
 extern void _mesa_##pfx##_transform_points##sz##_3d(XFORM_ARGS);

#define ASSIGN_XFORM_GROUP(pfx, sz)					\
   _mesa_transform_tab[sz][MATRIX_GENERAL] =				\
      _mesa_##pfx##_transform_points##sz##_general;			\
   _mesa_transform_tab[sz][MATRIX_IDENTITY] =				\
      _mesa_##pfx##_transform_points##sz##_identity;			\
   _mesa_transform_tab[sz][MATRIX_3D_NO_ROT] =				\
      _mesa_##pfx##_transform_points##sz##_3d_no_rot;			\
   _mesa_transform_tab[sz][MATRIX_PERSPECTIVE] =			\
      _mesa_##pfx##_transform_points##sz##_perspective;			\
   _mesa_transform_tab[sz][MATRIX_2D] =					\
      _mesa_##pfx##_transform_points##sz##_2d;				\
   _mesa_transform_tab[sz][MATRIX_2D_NO_ROT] =				\
      _mesa_##pfx##_transform_points##sz##_2d_no_rot;			\
   _mesa_transform_tab[sz][MATRIX_3D] =					\
      _mesa_##pfx##_transform_points##sz##_3d;


DECLARE_XFORM_GROUP(sparc, 1)
DECLARE_XFORM_GROUP(sparc, 2)
DECLARE_XFORM_GROUP(sparc, 3)
DECLARE_XFORM_GROUP(sparc, 4)

extern GLvector4f  *_mesa_sparc_cliptest_points4(GLvector4f *clip_vec,
						 GLvector4f *proj_vec,
						 GLubyte clipMask[],
						 GLubyte *orMask,
						 GLubyte *andMask);

extern GLvector4f  *_mesa_sparc_cliptest_points4_np(GLvector4f *clip_vec,
						    GLvector4f *proj_vec,
						    GLubyte clipMask[],
						    GLubyte *orMask,
						    GLubyte *andMask);

#define NORM_ARGS	const GLmatrix *mat,				\
			GLfloat scale,					\
			const GLvector4f *in,				\
			const GLfloat *lengths,				\
			GLvector4f *dest

extern void _mesa_sparc_transform_normalize_normals(NORM_ARGS);
extern void _mesa_sparc_transform_normalize_normals_no_rot(NORM_ARGS);
extern void _mesa_sparc_transform_rescale_normals_no_rot(NORM_ARGS);
extern void _mesa_sparc_transform_rescale_normals(NORM_ARGS);
extern void _mesa_sparc_transform_normals_no_rot(NORM_ARGS);
extern void _mesa_sparc_transform_normals(NORM_ARGS);
extern void _mesa_sparc_normalize_normals(NORM_ARGS);
extern void _mesa_sparc_rescale_normals(NORM_ARGS);



void _mesa_init_all_sparc_transform_asm(void)
{
   ASSIGN_XFORM_GROUP(sparc, 1)
   ASSIGN_XFORM_GROUP(sparc, 2)
   ASSIGN_XFORM_GROUP(sparc, 3)
   ASSIGN_XFORM_GROUP(sparc, 4)

   _mesa_clip_tab[4] = _mesa_sparc_cliptest_points4;
   _mesa_clip_np_tab[4] = _mesa_sparc_cliptest_points4_np;

#if 0
   /* disable these too.  See bug 673938 */
   _mesa_normal_tab[NORM_TRANSFORM | NORM_NORMALIZE] =
	   _mesa_sparc_transform_normalize_normals;
   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_NORMALIZE] =
	   _mesa_sparc_transform_normalize_normals_no_rot;
   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_RESCALE] =
	   _mesa_sparc_transform_rescale_normals_no_rot;
   _mesa_normal_tab[NORM_TRANSFORM | NORM_RESCALE] =
	   _mesa_sparc_transform_rescale_normals;
   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT] =
	   _mesa_sparc_transform_normals_no_rot;
   _mesa_normal_tab[NORM_TRANSFORM] =
	   _mesa_sparc_transform_normals;
   _mesa_normal_tab[NORM_NORMALIZE] =
	   _mesa_sparc_normalize_normals;
   _mesa_normal_tab[NORM_RESCALE] =
	   _mesa_sparc_rescale_normals;
#endif

#ifdef DEBUG_MATH
   _math_test_all_transform_functions("sparc");
   _math_test_all_cliptest_functions("sparc");
   _math_test_all_normal_transform_functions("sparc");
#endif
}

extern unsigned int _mesa_sparc_glapi_begin;
extern unsigned int _mesa_sparc_glapi_end;
extern void __glapi_sparc_icache_flush(unsigned int *);

#endif /* USE_SPARC_ASM */


void _mesa_init_sparc_glapi_relocs(void)
{
#ifdef USE_SPARC_ASM
	unsigned int *insn_ptr, *end_ptr;
	unsigned long disp_addr;

	insn_ptr = &_mesa_sparc_glapi_begin;
	end_ptr = &_mesa_sparc_glapi_end;
	disp_addr = (unsigned long) &_glapi_Dispatch;

	while (insn_ptr < end_ptr) {
#ifdef __arch64__
		insn_ptr[0] |= (disp_addr >> (32 + 10));
		insn_ptr[1] |= ((disp_addr & 0xffffffff) >> 10);
		__glapi_sparc_icache_flush(&insn_ptr[0]);
		insn_ptr[2] |= ((disp_addr >> 32) & ((1 << 10) - 1));
		insn_ptr[3] |= (disp_addr & ((1 << 10) - 1));
		__glapi_sparc_icache_flush(&insn_ptr[2]);
		insn_ptr += 11;
#else
		insn_ptr[0] |= (disp_addr >> 10);
		insn_ptr[1] |= (disp_addr & ((1 << 10) - 1));
		__glapi_sparc_icache_flush(&insn_ptr[0]);
		insn_ptr += 5;
#endif
	}
#endif /* USE_SPARC_ASM */
}
