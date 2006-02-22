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

#ifndef __T_SAVE_API_H__
#define __T_SAVE_API_H__

#include "t_context.h"

extern GLboolean _tnl_weak_begin( GLcontext *ctx, GLenum mode );

extern void _tnl_EndList( GLcontext *ctx );
extern void _tnl_NewList( GLcontext *ctx, GLuint list, GLenum mode );

extern void _tnl_EndCallList( GLcontext *ctx );
extern void _tnl_BeginCallList( GLcontext *ctx, struct mesa_display_list *list );

extern void _tnl_SaveFlushVertices( GLcontext *ctx );

extern void _tnl_save_init( GLcontext *ctx );
extern void _tnl_save_destroy( GLcontext *ctx );

extern void _tnl_loopback_vertex_list( GLcontext *ctx,
				       const struct tnl_vertex_list *list );

extern void _tnl_playback_vertex_list( GLcontext *ctx, void *data );

#endif
