/* $XFree86$ */
/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

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
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#ifndef __T_VTX_API_H__
#define __T_VTX_API_H__

#include "t_context.h"

extern void _tnl_vtx_init( GLcontext *ctx );
extern void _tnl_vtx_destroy( GLcontext *ctx );

extern void _tnl_FlushVertices( GLcontext *ctx, GLuint flags );
extern void _tnl_flush_vtx( GLcontext *ctx );

extern void _tnl_do_EvalCoord2f( GLcontext* ctx, GLfloat u, GLfloat v );
extern void _tnl_do_EvalCoord1f(GLcontext* ctx, GLfloat u);
extern void _tnl_update_eval( GLcontext *ctx );

extern GLboolean *_tnl_translate_edgeflag( GLcontext *ctx, 
					   const GLfloat *data, 
					   GLuint count,
					   GLuint stride );

extern GLboolean *_tnl_import_current_edgeflag( GLcontext *ctx,
						GLuint count );

#endif
