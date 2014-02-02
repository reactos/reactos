
/*
 * Mesa 3-D graphics library
 * Version:  5.0.1
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
 * 3DNow! optimizations contributed by
 * Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 */

#include "main/glheader.h"
#include "main/context.h"
#include "math/m_xform.h"
#include "tnl/t_context.h"

#include "3dnow.h"
#include "x86_xform.h"

#ifdef DEBUG_MATH
#include "math/m_debug.h"
#endif


#ifdef USE_3DNOW_ASM
DECLARE_XFORM_GROUP( 3dnow, 2 )
DECLARE_XFORM_GROUP( 3dnow, 3 )
DECLARE_XFORM_GROUP( 3dnow, 4 )

DECLARE_NORM_GROUP( 3dnow )


extern void _ASMAPI
_mesa_v16_3dnow_general_xform( GLfloat *first_vert,
			       const GLfloat *m,
			       const GLfloat *src,
			       GLuint src_stride,
			       GLuint count );

extern void _ASMAPI
_mesa_3dnow_project_vertices( GLfloat *first,
			      GLfloat *last,
			      const GLfloat *m,
			      GLuint stride );

extern void _ASMAPI
_mesa_3dnow_project_clipped_vertices( GLfloat *first,
				      GLfloat *last,
				      const GLfloat *m,
				      GLuint stride,
				      const GLubyte *clipmask );
#endif


void _mesa_init_3dnow_transform_asm( void )
{
#ifdef USE_3DNOW_ASM
   ASSIGN_XFORM_GROUP( 3dnow, 2 );
   ASSIGN_XFORM_GROUP( 3dnow, 3 );
   ASSIGN_XFORM_GROUP( 3dnow, 4 );

   /* There's a bug somewhere in the 3dnow_normal.S file that causes
    * bad shading.  Disable for now.
   ASSIGN_NORM_GROUP( 3dnow );
   */

#ifdef DEBUG_MATH
   _math_test_all_transform_functions( "3DNow!" );
   _math_test_all_normal_transform_functions( "3DNow!" );
#endif
#endif
}
