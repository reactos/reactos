/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#ifndef __MACH64_VB_H__
#define __MACH64_VB_H__

#include "mtypes.h"
#include "swrast/swrast.h"
#include "mach64_context.h"

/* premultiply texture coordinates by homogenous coordinate */
#define MACH64_PREMULT_TEXCOORDS

#define _MACH64_NEW_VERTEX_STATE (_DD_NEW_SEPARATE_SPECULAR |          \
                               _DD_NEW_TRI_LIGHT_TWOSIDE |             \
                               _DD_NEW_TRI_UNFILLED |                  \
                               _NEW_TEXTURE |                          \
                               _NEW_FOG)


extern void mach64CheckTexSizes( GLcontext *ctx );
extern void mach64ChooseVertexState( GLcontext *ctx );

extern void mach64BuildVertices( GLcontext *ctx, GLuint start, GLuint count,
				   GLuint newinputs );

extern void mach64PrintSetupFlags(char *msg, GLuint flags );

extern void mach64InitVB( GLcontext *ctx );
extern void mach64FreeVB( GLcontext *ctx );

#if 0
extern void mach64_emit_contiguous_verts( GLcontext *ctx,
					    GLuint start,
					    GLuint count );

extern void mach64_emit_indexed_verts( GLcontext *ctx,
					 GLuint start,
					 GLuint count );
#endif

extern void mach64_translate_vertex( GLcontext *ctx,
				       const mach64Vertex *src,
				       SWvertex *dst );

extern void mach64_print_vertex( GLcontext *ctx, const mach64Vertex *v );


#endif /* __MACH64_VB_H__ */
