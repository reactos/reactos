/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef _TNL_H
#define _TNL_H

#include "main/glheader.h"

struct gl_client_array;
struct gl_context;
struct gl_program;


/* These are the public-access functions exported from tnl.  (A few
 * more are currently hooked into dispatch directly by the module
 * itself.)
 */
extern GLboolean
_tnl_CreateContext( struct gl_context *ctx );

extern void
_tnl_DestroyContext( struct gl_context *ctx );

extern void
_tnl_InvalidateState( struct gl_context *ctx, GLuint new_state );

/* Functions to revive the tnl module after being unhooked from
 * dispatch and/or driver callbacks.
 */

extern void
_tnl_wakeup( struct gl_context *ctx );

/* Driver configuration options:
 */
extern void
_tnl_need_projected_coords( struct gl_context *ctx, GLboolean flag );


/* Control whether T&L does per-vertex fog
 */
extern void
_tnl_allow_vertex_fog( struct gl_context *ctx, GLboolean value );

extern void
_tnl_allow_pixel_fog( struct gl_context *ctx, GLboolean value );

struct _mesa_prim;
struct _mesa_index_buffer;

void
_tnl_draw_prims( struct gl_context *ctx,
		 const struct gl_client_array *arrays[],
		 const struct _mesa_prim *prim,
		 GLuint nr_prims,
		 const struct _mesa_index_buffer *ib,
		 GLuint min_index,
		 GLuint max_index);

void
_tnl_vbo_draw_prims( struct gl_context *ctx,
		     const struct gl_client_array *arrays[],
		     const struct _mesa_prim *prim,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     GLboolean index_bounds_valid,
		     GLuint min_index,
		     GLuint max_index);

extern void
_mesa_load_tracked_matrices(struct gl_context *ctx);

extern void
_tnl_RasterPos(struct gl_context *ctx, const GLfloat vObj[4]);

#endif
