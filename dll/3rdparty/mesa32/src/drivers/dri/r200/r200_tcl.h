/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_tcl.h,v 1.2 2002/12/16 16:18:55 dawes Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __R200_TCL_H__
#define __R200_TCL_H__

#include "r200_context.h"

extern void r200TclPrimitive( GLcontext *ctx, GLenum prim, int hw_prim );
extern void r200EmitEltPrimitive( GLcontext *ctx, GLuint first, GLuint last,
				    GLuint flags );
extern void r200EmitPrimitive( GLcontext *ctx, GLuint first, GLuint last,
				 GLuint flags );

extern void r200TclFallback( GLcontext *ctx, GLuint bit, GLboolean mode );

extern void r200InitStaticFogData( void );

extern float r200ComputeFogBlendFactor( GLcontext *ctx, GLfloat fogcoord );
					      
#define R200_TCL_FALLBACK_RASTER            0x1 /* rasterization */
#define R200_TCL_FALLBACK_UNFILLED          0x2 /* unfilled tris */
#define R200_TCL_FALLBACK_LIGHT_TWOSIDE     0x4 /* twoside tris */
#define R200_TCL_FALLBACK_MATERIAL          0x8 /* material in vb */
#define R200_TCL_FALLBACK_TEXGEN_0          0x10 /* texgen, unit 0 */
#define R200_TCL_FALLBACK_TEXGEN_1          0x20 /* texgen, unit 1 */
#define R200_TCL_FALLBACK_TEXGEN_2          0x40 /* texgen, unit 2 */
#define R200_TCL_FALLBACK_TEXGEN_3          0x80 /* texgen, unit 3 */
#define R200_TCL_FALLBACK_TEXGEN_4          0x100 /* texgen, unit 4 */
#define R200_TCL_FALLBACK_TEXGEN_5          0x200 /* texgen, unit 5 */
#define R200_TCL_FALLBACK_TCL_DISABLE       0x400 /* user disable */
#define R200_TCL_FALLBACK_BITMAP            0x800 /* draw bitmap with points */
#define R200_TCL_FALLBACK_VERTEX_PROGRAM    0x1000/* vertex program active */

#define TCL_FALLBACK( ctx, bit, mode )	r200TclFallback( ctx, bit, mode )

#endif
