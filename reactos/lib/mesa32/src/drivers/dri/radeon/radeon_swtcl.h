/* $XFree86$ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#ifndef __RADEON_TRIS_H__
#define __RADEON_TRIS_H__

#include "mtypes.h"
#include "swrast/swrast.h"
#include "radeon_context.h"

extern void radeonInitSwtcl( GLcontext *ctx );
extern void radeonDestroySwtcl( GLcontext *ctx );

extern void radeonFlushVertices( GLcontext *ctx, GLuint flags );
extern void radeonChooseRenderState( GLcontext *ctx );
extern void radeonChooseVertexState( GLcontext *ctx );

extern void radeonCheckTexSizes( GLcontext *ctx );

extern void radeonBuildVertices( GLcontext *ctx, GLuint start, GLuint count,
				 GLuint newinputs );

extern void radeonPrintSetupFlags(char *msg, GLuint flags );


extern void radeon_emit_indexed_verts( GLcontext *ctx,
				       GLuint start,
				       GLuint count );

extern void radeon_translate_vertex( GLcontext *ctx, 
				     const radeonVertex *src, 
				     SWvertex *dst );

extern void radeon_print_vertex( GLcontext *ctx, const radeonVertex *v );


#endif
