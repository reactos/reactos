/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_state.h,v 1.5 2002/11/05 17:46:09 tsi Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __RADEON_STATE_H__
#define __RADEON_STATE_H__

#include "radeon_context.h"

extern void radeonInitState( radeonContextPtr rmesa );
extern void radeonInitStateFuncs( GLcontext *ctx );

extern void radeonUpdateMaterial( GLcontext *ctx );

extern void radeonSetCliprects( radeonContextPtr rmesa );
extern void radeonRecalcScissorRects( radeonContextPtr rmesa );
extern void radeonUpdateViewportOffset( GLcontext *ctx );
extern void radeonUpdateWindow( GLcontext *ctx );
extern void radeonUpdateDrawBuffer( GLcontext *ctx );
extern void radeonUploadTexMatrix( radeonContextPtr rmesa,
				   int unit, GLboolean swapcols );

extern void radeonValidateState( GLcontext *ctx );

extern void radeonPrintDirty( radeonContextPtr rmesa,
			      const char *msg );


extern void radeonFallback( GLcontext *ctx, GLuint bit, GLboolean mode );
#define FALLBACK( rmesa, bit, mode ) do {				\
   if ( 0 ) fprintf( stderr, "FALLBACK in %s: #%d=%d\n",		\
		     __FUNCTION__, bit, mode );				\
   radeonFallback( rmesa->glCtx, bit, mode );				\
} while (0)


#define MODEL_PROJ 0
#define MODEL      1
#define MODEL_IT   2
#define TEXMAT_0   3
#define TEXMAT_1   4
#define TEXMAT_2   5

#endif
