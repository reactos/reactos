
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
 * x86-64 optimizations shamelessy converted from x86/sse/3dnow assembly by
 * Mikko Tiihonen
 */

#ifdef USE_X86_64_ASM

#include "main/glheader.h"
#include "main/context.h"
#include "math/m_xform.h"
#include "tnl/t_context.h"
#include "x86-64.h"
#include "../x86/x86_xform.h"

#ifdef DEBUG
#include "math/m_debug.h"
#endif

extern void _mesa_x86_64_cpuid(unsigned int *regs);

DECLARE_XFORM_GROUP( x86_64, 4 )
DECLARE_XFORM_GROUP( 3dnow, 4 )

#else
/* just to silence warning below */
#include "x86-64.h"
#endif

/*
extern void _mesa_x86_64_transform_points4_general( XFORM_ARGS );
extern void _mesa_x86_64_transform_points4_identity( XFORM_ARGS );
extern void _mesa_x86_64_transform_points4_perspective( XFORM_ARGS );
extern void _mesa_x86_64_transform_points4_3d( XFORM_ARGS );
extern void _mesa_x86_64_transform_points4_3d_no_rot( XFORM_ARGS );
extern void _mesa_x86_64_transform_points4_2d_no_rot( XFORM_ARGS );
extern void _mesa_x86_64_transform_points4_2d( XFORM_ARGS );
*/

#ifdef USE_X86_64_ASM
static void message( const char *msg )
{
   GLboolean debug;
#ifdef DEBUG
   debug = GL_TRUE;
#else
   if ( _mesa_getenv( "MESA_DEBUG" ) ) {
      debug = GL_TRUE;
   } else {
      debug = GL_FALSE;
   }
#endif
   if ( debug ) {
      _mesa_debug( NULL, "%s", msg );
   }
}
#endif


void _mesa_init_all_x86_64_transform_asm(void)
{
#ifdef USE_X86_64_ASM
   unsigned int regs[4];

   if ( _mesa_getenv( "MESA_NO_ASM" ) ) {
     return;
   }

   message("Initializing x86-64 optimizations\n");


   _mesa_transform_tab[4][MATRIX_GENERAL] =
      _mesa_x86_64_transform_points4_general;
   _mesa_transform_tab[4][MATRIX_IDENTITY] =
      _mesa_x86_64_transform_points4_identity;
   _mesa_transform_tab[4][MATRIX_3D] =
      _mesa_x86_64_transform_points4_3d;

   regs[0] = 0x80000001;
   regs[1] = 0x00000000;
   regs[2] = 0x00000000;
   regs[3] = 0x00000000;
   _mesa_x86_64_cpuid(regs);
   if (regs[3] & (1U << 31)) {
      message("3Dnow! detected\n");
      _mesa_transform_tab[4][MATRIX_3D_NO_ROT] =
	  _mesa_3dnow_transform_points4_3d_no_rot;
      _mesa_transform_tab[4][MATRIX_PERSPECTIVE] =
	  _mesa_3dnow_transform_points4_perspective;
      _mesa_transform_tab[4][MATRIX_2D_NO_ROT] =
	  _mesa_3dnow_transform_points4_2d_no_rot;
      _mesa_transform_tab[4][MATRIX_2D] =
	  _mesa_3dnow_transform_points4_2d;

   }

   
#ifdef DEBUG_MATH
   _math_test_all_transform_functions("x86_64");
   _math_test_all_cliptest_functions("x86_64");
   _math_test_all_normal_transform_functions("x86_64");
#endif

#endif
}
