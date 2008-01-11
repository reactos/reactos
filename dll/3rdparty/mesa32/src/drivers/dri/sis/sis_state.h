/**************************************************************************

Copyright 2003 Eric Anholt
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
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86$ */

/*
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#ifndef __SIS_STATE_H__
#define __SIS_STATE_H__

#include "sis_context.h"

/* sis6326_clear.c */
extern void sis6326DDClear( GLcontext *ctx, GLbitfield mask );
extern void sis6326DDClearColor( GLcontext * ctx, const GLfloat color[4] );
extern void sis6326DDClearDepth( GLcontext * ctx, GLclampd d );
extern void sis6326UpdateZPattern(sisContextPtr smesa, GLclampd z);

/* sis_clear.c */
extern void sisDDClear( GLcontext *ctx, GLbitfield mask );
extern void sisDDClearColor( GLcontext * ctx, const GLfloat color[4] );
extern void sisDDClearDepth( GLcontext * ctx, GLclampd d );
extern void sisDDClearStencil( GLcontext * ctx, GLint s );
extern void sisUpdateZStencilPattern( sisContextPtr smesa, GLclampd z,
				      int stencil );

/* sis_fog.c */
extern void sisDDFogfv( GLcontext * ctx, GLenum pname, const GLfloat * params );

/* sis6326_state.c */
extern void sis6326DDInitState( sisContextPtr smesa );
extern void sis6326DDInitStateFuncs( GLcontext *ctx );
extern void sis6326UpdateClipping( GLcontext * gc );
extern void sis6326DDDrawBuffer( GLcontext *ctx, GLenum mode );
extern void sis6326UpdateHWState( GLcontext *ctx );

/* sis_state.c */
extern void sisDDInitState( sisContextPtr smesa );
extern void sisDDInitStateFuncs( GLcontext *ctx );
extern void sisDDDepthMask( GLcontext * ctx, GLboolean flag );
extern void sisUpdateClipping( GLcontext * gc );
extern void sisDDDrawBuffer( GLcontext *ctx, GLenum mode );
extern void sisUpdateHWState( GLcontext *ctx );

#endif
