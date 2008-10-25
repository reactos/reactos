/*
 * GLX Hardware Device Driver for Intel tdfx
 * Copyright (C) 1999 Keith Whitwell
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
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_vb.h,v 1.2 2002/02/22 21:45:04 dawes Exp $ */

#ifndef TDFXVB_INC
#define TDFXVB_INC

#include "mtypes.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "math/m_xform.h"

#define TDFX_XYZ_BIT        0x1
#define TDFX_W_BIT          0x2
#define TDFX_RGBA_BIT       0x4
#define TDFX_TEX1_BIT       0x8
#define TDFX_TEX0_BIT       0x10	
#define TDFX_PTEX_BIT       0x20
#define TDFX_FOGC_BIT       0x40
#define TDFX_MAX_SETUP      0x80

#define _TDFX_NEW_RASTERSETUP (_NEW_TEXTURE |			\
			       _DD_NEW_SEPARATE_SPECULAR |	\
			       _DD_NEW_TRI_UNFILLED |		\
			       _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			       _NEW_FOG)


extern void tdfxValidateBuildProjVerts(GLcontext *ctx,
				       GLuint start, GLuint count,
				       GLuint newinputs );

extern void tdfxPrintSetupFlags(char *msg, GLuint flags );

extern void tdfxInitVB( GLcontext *ctx );

extern void tdfxFreeVB( GLcontext *ctx );

extern void tdfxCheckTexSizes( GLcontext *ctx );

extern void tdfxChooseVertexState( GLcontext *ctx );

extern void tdfxBuildVertices( GLcontext *ctx, GLuint start, GLuint end,
                               GLuint newinputs );

#endif
