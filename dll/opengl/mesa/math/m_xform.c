/*
 * Mesa 3-D graphics library
 * Version:  5.1
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
 * Matrix/vertex/vector transformation stuff
 *
 *
 * NOTES:
 * 1. 4x4 transformation matrices are stored in memory in column major order.
 * 2. Points/vertices are to be thought of as column vectors.
 * 3. Transformation of a point p by a matrix M is: p' = M * p
 */

#include <precomp.h>

#ifdef DEBUG_MATH
#include "m_debug.h"
#endif

#ifdef USE_X86_ASM
#include "x86/common_x86_asm.h"
#endif

#ifdef USE_X86_64_ASM
#include "x86-64/x86-64.h"
#endif

#ifdef USE_SPARC_ASM
#include "sparc/sparc.h"
#endif

#ifdef USE_PPC_ASM
#include "ppc/common_ppc_features.h"
#endif

clip_func _mesa_clip_tab[5];
clip_func _mesa_clip_np_tab[5];
dotprod_func _mesa_dotprod_tab[5];
vec_copy_func _mesa_copy_tab[0x10];
normal_func _mesa_normal_tab[0xf];
transform_func *_mesa_transform_tab[5];


/* Raw data format used for:
 *    - Object-to-eye transform prior to culling, although this too
 *      could be culled under some circumstances.
 *    - Eye-to-clip transform (via the function above).
 *    - Cliptesting
 *    - And everything else too, if culling happens to be disabled.
 *
 * GH: It's used for everything now, as clipping/culling is done
 *     elsewhere (most often by the driver itself).
 */
#define TAG(x) x
#define TAG2(x,y) x##y
#define STRIDE_LOOP for ( i = 0 ; i < count ; i++, STRIDE_F(from, stride) )
#define LOOP for ( i = 0 ; i < n ; i++ )
#define ARGS
#include "m_xform_tmp.h"
#include "m_clip_tmp.h"
#include "m_norm_tmp.h"
#include "m_dotprod_tmp.h"
#include "m_copy_tmp.h"
#undef TAG
#undef TAG2
#undef LOOP
#undef ARGS


/*
 * This is called only once.  It initializes several tables with pointers
 * to optimized transformation functions.  This is where we can test for
 * AMD 3Dnow! capability, Intel SSE, etc. and hook in the right code.
 */
void
_math_init_transformation( void )
{
   init_c_transformations();
   init_c_norm_transform();
   init_c_cliptest();
   init_copy0();
   init_dotprod();

#ifdef DEBUG_MATH
   _math_test_all_transform_functions( "default" );
   _math_test_all_normal_transform_functions( "default" );
   _math_test_all_cliptest_functions( "default" );
#endif

#ifdef USE_X86_ASM
   _mesa_init_all_x86_transform_asm();
#elif defined( USE_SPARC_ASM )
   _mesa_init_all_sparc_transform_asm();
#elif defined( USE_PPC_ASM )
   _mesa_init_all_ppc_transform_asm();
#elif defined( USE_X86_64_ASM )
   _mesa_init_all_x86_64_transform_asm();
#endif
}
