/*
 * Copyright 2003 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */

#ifndef _TNL_VERTEX_H
#define _TNL_VERTEX_H

#include "main/glheader.h"
#include "t_context.h"

struct gl_context;
struct tnl_clipspace;

/* New mechanism to specify hardware vertices so that tnl can build
 * and manipulate them directly.  
 */


/* It will probably be necessary to allow drivers to specify new
 * emit-styles to cover all the wierd and wacky things out there.
 */
enum tnl_attr_format {
   EMIT_1F,
   EMIT_2F,
   EMIT_3F,
   EMIT_4F,
   EMIT_2F_VIEWPORT,		/* do viewport transform and emit */
   EMIT_3F_VIEWPORT,		/* do viewport transform and emit */
   EMIT_4F_VIEWPORT,		/* do viewport transform and emit */
   EMIT_3F_XYW,			/* for projective texture */
   EMIT_1UB_1F,			/* for fog coordinate */
   EMIT_3UB_3F_RGB,		/* for specular color */
   EMIT_3UB_3F_BGR,		/* for specular color */
   EMIT_4UB_4F_RGBA,		/* for color */
   EMIT_4UB_4F_BGRA,		/* for color */
   EMIT_4UB_4F_ARGB,		/* for color */
   EMIT_4UB_4F_ABGR,		/* for color */
   EMIT_4CHAN_4F_RGBA,		/* for swrast color */
   EMIT_PAD,			/* leave a hole of 'offset' bytes */
   EMIT_MAX
};

struct tnl_attr_map {
   GLuint attrib;			/* _TNL_ATTRIB_ enum */
   enum tnl_attr_format format;
   GLuint offset;
};

struct tnl_format_info {
   const char *name;
   tnl_extract_func extract;
   tnl_insert_func insert[4];
   const GLuint attrsize;
};

extern const struct tnl_format_info _tnl_format_info[EMIT_MAX];


/* Interpolate between two vertices to produce a third:
 */
extern void _tnl_interp( struct gl_context *ctx,
			 GLfloat t,
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary );

/* Copy colors from one vertex to another:
 */
extern void _tnl_copy_pv(  struct gl_context *ctx, GLuint edst, GLuint esrc );


/* Extract a named attribute from a hardware vertex.  Will have to
 * reverse any viewport transformation, swizzling or other conversions
 * which may have been applied:
 */
extern void _tnl_get_attr( struct gl_context *ctx, const void *vertex, GLenum attrib,
			   GLfloat *dest );

/* Complementary to the above.
 */
extern void _tnl_set_attr( struct gl_context *ctx, void *vout, GLenum attrib, 
			   const GLfloat *src );


extern void *_tnl_get_vertex( struct gl_context *ctx, GLuint nr );

extern GLuint _tnl_install_attrs( struct gl_context *ctx,
				  const struct tnl_attr_map *map,
				  GLuint nr, const GLfloat *vp,
				  GLuint unpacked_size );

extern void _tnl_free_vertices( struct gl_context *ctx );

extern void _tnl_init_vertices( struct gl_context *ctx, 
				GLuint vb_size,
				GLuint max_vertex_size );

extern void *_tnl_emit_vertices_to_buffer( struct gl_context *ctx,
					   GLuint start,
					   GLuint end,
					   void *dest );

/* This function isn't optimal. Check out 
 * gallium/auxilary/translate for a more comprehensive implementation of
 * the same functionality.
 */
  
extern void *_tnl_emit_indexed_vertices_to_buffer( struct gl_context *ctx,
						   const GLuint *elts,
						   GLuint start,
						   GLuint end,
						   void *dest );


extern void _tnl_build_vertices( struct gl_context *ctx,
				 GLuint start,
				 GLuint end,
				 GLuint newinputs );

extern void _tnl_invalidate_vertices( struct gl_context *ctx, GLuint newinputs );

extern void _tnl_invalidate_vertex_state( struct gl_context *ctx, GLuint new_state );

extern void _tnl_notify_pipeline_output_change( struct gl_context *ctx );


#define GET_VERTEX_STATE(ctx)  &(TNL_CONTEXT(ctx)->clipspace)

/* Internal function:
 */
void _tnl_register_fastpath( struct tnl_clipspace *vtx,
			     GLboolean match_strides );


/* t_vertex_generic.c -- Internal functions for t_vertex.c
 */
void _tnl_generic_copy_pv_extras( struct gl_context *ctx, 
				  GLuint dst, GLuint src );

void _tnl_generic_interp_extras( struct gl_context *ctx,
				 GLfloat t,
				 GLuint dst, GLuint out, GLuint in,
				 GLboolean force_boundary );

void _tnl_generic_copy_pv( struct gl_context *ctx, GLuint edst, GLuint esrc );

void _tnl_generic_interp( struct gl_context *ctx,
			  GLfloat t,
			  GLuint edst, GLuint eout, GLuint ein,
			  GLboolean force_boundary );

void _tnl_generic_emit( struct gl_context *ctx,
			GLuint count,
			GLubyte *v );

void _tnl_generate_hardwired_emit( struct gl_context *ctx );

/* t_vertex_sse.c -- Internal functions for t_vertex.c
 */
void _tnl_generate_sse_emit( struct gl_context *ctx );

#endif
