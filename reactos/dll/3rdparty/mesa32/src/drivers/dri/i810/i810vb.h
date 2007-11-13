/*
 * GLX Hardware Device Driver for Intel i810
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
/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810vb.h,v 1.4 2002/02/22 21:33:04 dawes Exp $ */

#ifndef I810VB_INC
#define I810VB_INC

#include "mtypes.h"
#include "swrast/swrast.h"

#define _I810_NEW_VERTEX (_NEW_TEXTURE |			\
			  _DD_NEW_SEPARATE_SPECULAR |		\
			  _DD_NEW_TRI_UNFILLED |		\
			  _DD_NEW_TRI_LIGHT_TWOSIDE |		\
			  _NEW_FOG)


extern void i810ChooseVertexState( GLcontext *ctx );
extern void i810CheckTexSizes( GLcontext *ctx );
extern void i810BuildVertices( GLcontext *ctx,
			       GLuint start,
			       GLuint count,
			       GLuint newinputs );


extern void *i810_emit_contiguous_verts( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 void *dest );

extern void i810_translate_vertex( GLcontext *ctx,
				   const i810Vertex *src,
				   SWvertex *dst );

extern void i810InitVB( GLcontext *ctx );
extern void i810FreeVB( GLcontext *ctx );

#endif
