/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_swtcl.h,v 1.3 2003/05/06 23:52:08 daenzer Exp $ */
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
*/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __R200_SWTCL_H__
#define __R200_SWTCL_H__

#include "mtypes.h"
#include "swrast/swrast.h"
#include "r200_context.h"

extern void r200InitSwtcl( GLcontext *ctx );
extern void r200DestroySwtcl( GLcontext *ctx );

extern void r200ChooseRenderState( GLcontext *ctx );
extern void r200ChooseVertexState( GLcontext *ctx );

extern void r200CheckTexSizes( GLcontext *ctx );

extern void r200BuildVertices( GLcontext *ctx, GLuint start, GLuint count,
				 GLuint newinputs );

extern void r200PrintSetupFlags(char *msg, GLuint flags );


extern void r200_emit_indexed_verts( GLcontext *ctx,
				       GLuint start,
				       GLuint count );

extern void r200_translate_vertex( GLcontext *ctx, 
				     const r200Vertex *src, 
				     SWvertex *dst );

extern void r200_print_vertex( GLcontext *ctx, const r200Vertex *v );

extern void r200_import_float_colors( GLcontext *ctx );
extern void r200_import_float_spec_colors( GLcontext *ctx );

extern void r200PointsBitmap( GLcontext *ctx, GLint px, GLint py,
			      GLsizei width, GLsizei height,
			      const struct gl_pixelstore_attrib *unpack,
			      const GLubyte *bitmap );


#endif
