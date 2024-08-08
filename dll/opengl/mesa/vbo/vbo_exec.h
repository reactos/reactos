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

#ifndef __VBO_EXEC_H__
#define __VBO_EXEC_H__

#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "vbo.h"
#include "vbo_attrib.h"


/**
 * Max number of primitives (number of glBegin/End pairs) per VBO.
 */
#define VBO_MAX_PRIM 64


/**
 * Size of the VBO to use for glBegin/glVertex/glEnd-style rendering.
 */
#define VBO_VERT_BUFFER_SIZE (1024*64)	/* bytes */


/** Current vertex program mode */
enum vp_mode {
   VP_NONE,   /**< fixed function */
   VP_NV,     /**< NV vertex program */
   VP_ARB     /**< ARB vertex program or GLSL vertex shader */
};


struct vbo_exec_eval1_map {
   struct gl_1d_map *map;
   GLuint sz;
};

struct vbo_exec_eval2_map {
   struct gl_2d_map *map;
   GLuint sz;
};



struct vbo_exec_copied_vtx {
   GLfloat buffer[VBO_ATTRIB_MAX * 4 * VBO_MAX_COPIED_VERTS];
   GLuint nr;
};


/** Used to signal when transitioning from one kind of drawing method
 * to another.
 */
enum draw_method
{
   DRAW_NONE,          /**< Initial value only */
   DRAW_BEGIN_END,
   DRAW_DISPLAY_LIST,
   DRAW_ARRAYS
};


struct vbo_exec_context
{
   struct gl_context *ctx;   
   GLvertexformat vtxfmt;
   GLvertexformat vtxfmt_noop;

   enum draw_method last_draw_method;

   struct {
      struct gl_buffer_object *bufferobj;

      GLuint vertex_size;       /* in dwords */

      struct _mesa_prim prim[VBO_MAX_PRIM];
      GLuint prim_count;

      GLfloat *buffer_map;
      GLfloat *buffer_ptr;              /* cursor, points into buffer */
      GLuint   buffer_used;             /* in bytes */
      GLfloat vertex[VBO_ATTRIB_MAX*4]; /* current vertex */

      GLuint vert_count;
      GLuint max_vert;
      struct vbo_exec_copied_vtx copied;

      GLubyte attrsz[VBO_ATTRIB_MAX];
      GLubyte active_sz[VBO_ATTRIB_MAX];

      GLfloat *attrptr[VBO_ATTRIB_MAX]; 
      struct gl_client_array arrays[VBO_ATTRIB_MAX];

      /* According to program mode, the values above plus current
       * values are squashed down to the 32 attributes passed to the
       * vertex program below:
       */
      const struct gl_client_array *inputs[VBO_ATTRIB_MAX];
   } vtx;

   
   struct {
      GLboolean recalculate_maps;
      struct vbo_exec_eval1_map map1[VERT_ATTRIB_MAX];
      struct vbo_exec_eval2_map map2[VERT_ATTRIB_MAX];
   } eval;

   struct {
      /* Arrays and current values manipulated according to program
       * mode, etc.  These are the attributes as seen by vertex
       * programs:
       */
      const struct gl_client_array *inputs[VBO_ATTRIB_MAX];
   } array;

   /* Which flags to set in vbo_exec_BeginVertices() */
   GLbitfield begin_vertices_flags;

#ifdef DEBUG
   GLint flush_call_depth;
#endif
};



/* External API:
 */
void vbo_exec_init( struct gl_context *ctx );
void vbo_exec_destroy( struct gl_context *ctx );
void vbo_exec_invalidate_state( struct gl_context *ctx, GLuint new_state );

void vbo_exec_BeginVertices( struct gl_context *ctx );
void vbo_exec_FlushVertices( struct gl_context *ctx, GLuint flags );


/* Internal functions:
 */
void vbo_exec_array_init( struct vbo_exec_context *exec );
void vbo_exec_array_destroy( struct vbo_exec_context *exec );


void vbo_exec_vtx_init( struct vbo_exec_context *exec );
void vbo_exec_vtx_destroy( struct vbo_exec_context *exec );


/**
 * This is called by glBegin, glDrawArrays and glDrawElements (and
 * variations of those calls).  When we transition from immediate mode
 * drawing to array drawing we need to invalidate the array state.
 *
 * glBegin/End builds vertex arrays.  Those arrays may look identical
 * to glDrawArrays arrays except that the position of the elements may
 * be different.  For example, arrays of (position3v, normal3f) vs. arrays
 * of (normal3f, position3f).  So we need to make sure we notify drivers
 * that arrays may be changing.
 */
static inline void
vbo_draw_method(struct vbo_exec_context *exec, enum draw_method method)
{
   if (exec->last_draw_method != method) {
      exec->ctx->NewState |= _NEW_ARRAY;
      exec->last_draw_method = method;
   }
}


#if FEATURE_beginend

void vbo_exec_vtx_flush( struct vbo_exec_context *exec, GLboolean unmap );
void vbo_exec_vtx_map( struct vbo_exec_context *exec );

#else /* FEATURE_beginend */

static inline void
vbo_exec_vtx_flush( struct vbo_exec_context *exec, GLboolean unmap )
{
}

static inline void
vbo_exec_vtx_map( struct vbo_exec_context *exec )
{
}

#endif /* FEATURE_beginend */

void vbo_exec_vtx_wrap( struct vbo_exec_context *exec );

void vbo_exec_eval_update( struct vbo_exec_context *exec );

void vbo_exec_do_EvalCoord2f( struct vbo_exec_context *exec, 
				     GLfloat u, GLfloat v );

void vbo_exec_do_EvalCoord1f( struct vbo_exec_context *exec,
				     GLfloat u);

#endif
