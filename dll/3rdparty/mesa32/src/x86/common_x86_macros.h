/* $Id: common_x86_macros.h,v 1.3 2002/10/29 20:28:58 brianp Exp $ */

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
 *
 * Authors:
 *    Gareth Hughes
 */

#ifndef __COMMON_X86_MACROS_H__
#define __COMMON_X86_MACROS_H__


/* =============================================================
 * Transformation function declarations:
 */

#define XFORM_ARGS	GLvector4f *to_vec,				\
			const GLfloat m[16],				\
			const GLvector4f *from_vec

#define DECLARE_XFORM_GROUP( pfx, sz ) \
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_general( XFORM_ARGS );		\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_identity( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_3d_no_rot( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_perspective( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_2d( XFORM_ARGS );		\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_2d_no_rot( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_3d( XFORM_ARGS );

#define ASSIGN_XFORM_GROUP( pfx, sz )					\
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


/* =============================================================
 * Normal transformation function declarations:
 */

#define NORM_ARGS	const GLmatrix *mat,				\
			GLfloat scale,					\
			const GLvector4f *in,				\
			const GLfloat *lengths,				\
			GLvector4f *dest

#define DECLARE_NORM_GROUP( pfx ) \
extern void _ASMAPI _mesa_##pfx##_rescale_normals( NORM_ARGS );				\
extern void _ASMAPI _mesa_##pfx##_normalize_normals( NORM_ARGS );			\
extern void _ASMAPI _mesa_##pfx##_transform_normals( NORM_ARGS );			\
extern void _ASMAPI _mesa_##pfx##_transform_normals_no_rot( NORM_ARGS );		\
extern void _ASMAPI _mesa_##pfx##_transform_rescale_normals( NORM_ARGS );		\
extern void _ASMAPI _mesa_##pfx##_transform_rescale_normals_no_rot( NORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_normalize_normals( NORM_ARGS );		\
extern void _ASMAPI _mesa_##pfx##_transform_normalize_normals_no_rot( NORM_ARGS );

#define ASSIGN_NORM_GROUP( pfx )					\
   _mesa_normal_tab[NORM_RESCALE] =					\
      _mesa_##pfx##_rescale_normals;					\
   _mesa_normal_tab[NORM_NORMALIZE] =					\
      _mesa_##pfx##_normalize_normals;					\
   _mesa_normal_tab[NORM_TRANSFORM] =					\
      _mesa_##pfx##_transform_normals;					\
   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT] =				\
      _mesa_##pfx##_transform_normals_no_rot;				\
   _mesa_normal_tab[NORM_TRANSFORM | NORM_RESCALE] =			\
      _mesa_##pfx##_transform_rescale_normals;				\
   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_RESCALE] =		\
      _mesa_##pfx##_transform_rescale_normals_no_rot;			\
   _mesa_normal_tab[NORM_TRANSFORM | NORM_NORMALIZE] =			\
      _mesa_##pfx##_transform_normalize_normals;			\
   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_NORMALIZE] =		\
      _mesa_##pfx##_transform_normalize_normals_no_rot;


#endif
