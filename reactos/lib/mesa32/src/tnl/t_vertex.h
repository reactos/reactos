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
   EMIT_3UB_3F_BGR,		/* for specular color */
   EMIT_3UB_3F_RGB,		/* for specular color */
   EMIT_4UB_4F_BGRA,		/* for color */
   EMIT_4UB_4F_RGBA,		/* for color */
   EMIT_4CHAN_4F_RGBA,		/* for swrast color */
   EMIT_MAX
};

struct tnl_attr_map {
   GLuint attrib;			/* _TNL_ATTRIB_ enum */
   enum tnl_attr_format format;
   GLuint offset;
};
   


/* Interpolate between two vertices to produce a third:
 */
extern void _tnl_interp( GLcontext *ctx,
			 GLfloat t,
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary );

/* Copy colors from one vertex to another:
 */
extern void _tnl_copy_pv(  GLcontext *ctx, GLuint edst, GLuint esrc );


/* Extract a named attribute from a hardware vertex.  Will have to
 * reverse any viewport transformation, swizzling or other conversions
 * which may have been applied:
 */
extern void _tnl_get_attr( GLcontext *ctx, const void *vertex, GLenum attrib,
			   GLfloat *dest );

extern void *_tnl_get_vertex( GLcontext *ctx, GLuint nr );


/*
 */
extern GLuint _tnl_install_attrs( GLcontext *ctx,
				  const struct tnl_attr_map *map,
				  GLuint nr, const GLfloat *vp,
				  GLuint unpacked_size );





extern void _tnl_free_vertices( GLcontext *ctx );

extern void _tnl_init_vertices( GLcontext *ctx, 
				GLuint vb_size,
				GLuint max_vertex_size );

extern void *_tnl_emit_vertices_to_buffer( GLcontext *ctx,
					   GLuint start,
					   GLuint count,
					   void *dest );

extern void _tnl_build_vertices( GLcontext *ctx,
				 GLuint start,
				 GLuint count,
				 GLuint newinputs );

extern void _tnl_invalidate_vertices( GLcontext *ctx, GLuint newinputs );

extern void _tnl_invalidate_vertex_state( GLcontext *ctx, GLuint new_state );


#endif
