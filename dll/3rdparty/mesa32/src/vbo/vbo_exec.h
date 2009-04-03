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

#include "main/mtypes.h"
#include "vbo.h"
#include "vbo_attrib.h"


#define VBO_MAX_PRIM 64

/* Wierd implementation stuff:
 */
#define VBO_VERT_BUFFER_SIZE (1024*16)	/* dwords == 64k */
#define VBO_MAX_ATTR_CODEGEN 16 
#define ERROR_ATTRIB 16




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


typedef void (*vbo_attrfv_func)( const GLfloat * );


struct vbo_exec_context
{
   GLcontext *ctx;   
   GLvertexformat vtxfmt;

   struct {
      struct gl_buffer_object *bufferobj;
      GLubyte *buffer_map;

      GLuint vertex_size;

      struct _mesa_prim prim[VBO_MAX_PRIM];
      GLuint prim_count;

      GLfloat *vbptr;		     /* cursor, points into buffer */
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
      GLuint program_mode;
      GLuint enabled_flags;
      const struct gl_client_array *inputs[VERT_ATTRIB_MAX];
   } vtx;

   
   struct {
      GLboolean recalculate_maps;
      struct vbo_exec_eval1_map map1[VERT_ATTRIB_MAX];
      struct vbo_exec_eval2_map map2[VERT_ATTRIB_MAX];
   } eval;

   struct {
      GLuint program_mode;
      GLuint enabled_flags;
      GLuint array_obj;

      /* These just mirror the current arrayobj (todo: make arrayobj
       * look like this and remove the mirror):
       */
      const struct gl_client_array *legacy_array[16];
      const struct gl_client_array *generic_array[16];

      /* Arrays and current values manipulated according to program
       * mode, etc.  These are the attributes as seen by vertex
       * programs:
       */
      const struct gl_client_array *inputs[VERT_ATTRIB_MAX];
   } array;
};



/* External API:
 */
void vbo_exec_init( GLcontext *ctx );
void vbo_exec_destroy( GLcontext *ctx );
void vbo_exec_invalidate_state( GLcontext *ctx, GLuint new_state );
void vbo_exec_FlushVertices( GLcontext *ctx, GLuint flags );


/* Internal functions:
 */
void vbo_exec_array_init( struct vbo_exec_context *exec );
void vbo_exec_array_destroy( struct vbo_exec_context *exec );


void vbo_exec_vtx_init( struct vbo_exec_context *exec );
void vbo_exec_vtx_destroy( struct vbo_exec_context *exec );
void vbo_exec_vtx_flush( struct vbo_exec_context *exec );
void vbo_exec_vtx_wrap( struct vbo_exec_context *exec );

void vbo_exec_eval_update( struct vbo_exec_context *exec );

void vbo_exec_do_EvalCoord2f( struct vbo_exec_context *exec, 
				     GLfloat u, GLfloat v );

void vbo_exec_do_EvalCoord1f( struct vbo_exec_context *exec,
				     GLfloat u);

extern GLboolean 
vbo_validate_shaders(GLcontext *ctx);

#endif
