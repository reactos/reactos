
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
 * Intel x86 assembly code by Josh Vanderhoof
 */

#include "main/glheader.h"
#include "main/context.h"
#include "math/m_xform.h"

#include "x86_xform.h"
#include "common_x86_asm.h"

#ifdef USE_X86_ASM
#ifdef USE_3DNOW_ASM
#include "3dnow.h"
#endif
#ifdef USE_SSE_ASM
#include "sse.h"
#endif
#endif

#ifdef DEBUG_MATH
#include "math/m_debug.h"
#endif


#ifdef USE_X86_ASM
DECLARE_XFORM_GROUP( x86, 2 )
DECLARE_XFORM_GROUP( x86, 3 )
DECLARE_XFORM_GROUP( x86, 4 )


extern GLvector4f * _ASMAPI
_mesa_x86_cliptest_points4( GLvector4f *clip_vec,
			    GLvector4f *proj_vec,
			    GLubyte clipMask[],
			    GLubyte *orMask,
			    GLubyte *andMask,
			    GLboolean viewport_z_clip );

extern GLvector4f * _ASMAPI
_mesa_x86_cliptest_points4_np( GLvector4f *clip_vec,
			       GLvector4f *proj_vec,
			       GLubyte clipMask[],
			       GLubyte *orMask,
			       GLubyte *andMask,
			       GLboolean viewport_z_clip );

extern void _ASMAPI
_mesa_v16_x86_cliptest_points4( GLfloat *first_vert,
				GLfloat *last_vert,
				GLubyte *or_mask,
				GLubyte *and_mask,
				GLubyte *clip_mask,
				GLboolean viewport_z_clip );

extern void _ASMAPI
_mesa_v16_x86_general_xform( GLfloat *dest,
			     const GLfloat *m,
			     const GLfloat *src,
			     GLuint src_stride,
			     GLuint count );
#endif


#ifdef USE_X86_ASM
static void _mesa_init_x86_transform_asm( void )
{
   ASSIGN_XFORM_GROUP( x86, 2 );
   ASSIGN_XFORM_GROUP( x86, 3 );
   ASSIGN_XFORM_GROUP( x86, 4 );

   _mesa_clip_tab[4] = _mesa_x86_cliptest_points4;
   _mesa_clip_np_tab[4] = _mesa_x86_cliptest_points4_np;

#ifdef DEBUG_MATH
   _math_test_all_transform_functions( "x86" );
   _math_test_all_cliptest_functions( "x86" );
#endif
}
#endif


void _mesa_init_all_x86_transform_asm( void )
{
   _mesa_get_x86_features();

#ifdef USE_X86_ASM
   if ( _mesa_x86_cpu_features ) {
      _mesa_init_x86_transform_asm();
   }

   if (cpu_has_3dnow) {
      _mesa_init_3dnow_transform_asm();
   }

   if ( cpu_has_xmm ) {
      _mesa_init_sse_transform_asm();
   }

#endif
}
