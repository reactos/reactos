/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_state.h,v 1.2 2002/11/05 17:46:08 tsi Exp $ */
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

#ifndef __R200_STATE_H__
#define __R200_STATE_H__

#include "r200_context.h"

extern void r200InitState( r200ContextPtr rmesa );
extern void r200InitStateFuncs( struct dd_function_table *functions );
extern void r200InitTnlFuncs( GLcontext *ctx );

extern void r200UpdateMaterial( GLcontext *ctx );

extern void r200SetCliprects( r200ContextPtr rmesa );
extern void r200RecalcScissorRects( r200ContextPtr rmesa );
extern void r200UpdateViewportOffset( GLcontext *ctx );
extern void r200UpdateWindow( GLcontext *ctx );
extern void r200UpdateDrawBuffer(GLcontext *ctx);

extern void r200ValidateState( GLcontext *ctx );

extern void r200PrintDirty( r200ContextPtr rmesa,
			      const char *msg );


extern void r200Fallback( GLcontext *ctx, GLuint bit, GLboolean mode );
#define FALLBACK( rmesa, bit, mode ) do {				\
   if ( 0 ) fprintf( stderr, "FALLBACK in %s: #%d=%d\n",		\
		     __FUNCTION__, bit, mode );				\
   r200Fallback( rmesa->glCtx, bit, mode );				\
} while (0)

extern void r200LightingSpaceChange( GLcontext *ctx );

#endif
