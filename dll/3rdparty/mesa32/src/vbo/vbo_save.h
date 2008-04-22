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

#ifndef VBO_SAVE_H
#define VBO_SAVE_H

#include "mtypes.h"
#include "vbo.h"
#include "vbo_attrib.h"


struct vbo_save_copied_vtx {
   GLfloat buffer[VBO_ATTRIB_MAX * 4 * VBO_MAX_COPIED_VERTS];
   GLuint nr;
};


/* For display lists, this structure holds a run of vertices of the
 * same format, and a strictly well-formed set of begin/end pairs,
 * starting on the first vertex and ending at the last.  Vertex
 * copying on buffer breaks is precomputed according to these
 * primitives, though there are situations where the copying will need
 * correction at execute-time, perhaps by replaying the list as
 * immediate mode commands.
 *
 * On executing this list, the 'current' values may be updated with
 * the values of the final vertex, and often no fixup of the start of
 * the vertex list is required.
 *
 * Eval and other commands that don't fit into these vertex lists are
 * compiled using the fallback opcode mechanism provided by dlist.c.
 */
struct vbo_save_vertex_list {
   GLubyte attrsz[VBO_ATTRIB_MAX];
   GLuint vertex_size;

   GLuint buffer_offset;
   GLuint count;
   GLuint wrap_count;		/* number of copied vertices at start */
   GLboolean dangling_attr_ref;	/* current attr implicitly referenced 
				   outside the list */

   struct _mesa_prim *prim;
   GLuint prim_count;

   struct vbo_save_vertex_store *vertex_store;
   struct vbo_save_primitive_store *prim_store;
};

/* These buffers should be a reasonable size to support upload to
 * hardware.  Current vbo implementation will re-upload on any
 * changes, so don't make too big or apps which dynamically create
 * dlists and use only a few times will suffer.
 *
 * Consider stategy of uploading regions from the VBO on demand in the
 * case of dynamic vbos.  Then make the dlist code signal that
 * likelyhood as it occurs.  No reason we couldn't change usage
 * internally even though this probably isn't allowed for client VBOs?
 */
#define VBO_SAVE_BUFFER_SIZE (8*1024) /* dwords */
#define VBO_SAVE_PRIM_SIZE   128
#define VBO_SAVE_PRIM_WEAK 0x40

#define VBO_SAVE_FALLBACK    0x10000000

/* Storage to be shared among several vertex_lists.
 */
struct vbo_save_vertex_store {
   struct gl_buffer_object *bufferobj;
   GLfloat *buffer;
   GLuint used;
   GLuint refcount;
};

struct vbo_save_primitive_store {
   struct _mesa_prim buffer[VBO_SAVE_PRIM_SIZE];
   GLuint used;
   GLuint refcount;
};


struct vbo_save_context {
   GLcontext *ctx;
   GLvertexformat vtxfmt;
   struct gl_client_array arrays[VBO_ATTRIB_MAX];
   const struct gl_client_array *inputs[VBO_ATTRIB_MAX];

   GLubyte attrsz[VBO_ATTRIB_MAX];
   GLubyte active_sz[VBO_ATTRIB_MAX];
   GLuint vertex_size;

   GLfloat *buffer;
   GLuint count;
   GLuint wrap_count;
   GLuint replay_flags;

   struct _mesa_prim *prim;
   GLuint prim_count, prim_max;

   struct vbo_save_vertex_store *vertex_store;
   struct vbo_save_primitive_store *prim_store;

   GLfloat *vbptr;		   /* cursor, points into buffer */
   GLfloat vertex[VBO_ATTRIB_MAX*4];	   /* current values */
   GLfloat *attrptr[VBO_ATTRIB_MAX];
   GLuint vert_count;
   GLuint max_vert;
   GLboolean dangling_attr_ref;
   GLboolean have_materials;

   GLuint opcode_vertex_list;

   struct vbo_save_copied_vtx copied;
   
   GLfloat *current[VBO_ATTRIB_MAX]; /* points into ctx->ListState */
   GLubyte *currentsz[VBO_ATTRIB_MAX];
};


void vbo_save_init( GLcontext *ctx );
void vbo_save_destroy( GLcontext *ctx );
void vbo_save_fallback( GLcontext *ctx, GLboolean fallback );

/* save_loopback.c:
 */
void vbo_loopback_vertex_list( GLcontext *ctx,
			       const GLfloat *buffer,
			       const GLubyte *attrsz,
			       const struct _mesa_prim *prim,
			       GLuint prim_count,
			       GLuint wrap_count,
			       GLuint vertex_size);

/* Callbacks:
 */
void vbo_save_EndList( GLcontext *ctx );
void vbo_save_NewList( GLcontext *ctx, GLuint list, GLenum mode );
void vbo_save_EndCallList( GLcontext *ctx );
void vbo_save_BeginCallList( GLcontext *ctx, struct mesa_display_list *list );
void vbo_save_SaveFlushVertices( GLcontext *ctx );
GLboolean vbo_save_NotifyBegin( GLcontext *ctx, GLenum mode );

void vbo_save_playback_vertex_list( GLcontext *ctx, void *data );

void vbo_save_api_init( struct vbo_save_context *save );

#endif
