/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 */

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "mtypes.h"
#include "macros.h"
#include "light.h"
#include "state.h"

#include "vbo_context.h"


/*
 * After playback, copy everything but the position from the
 * last vertex to the saved state
 */
static void _playback_copy_to_current( GLcontext *ctx,
				       const struct vbo_save_vertex_list *node )
{
   struct vbo_context *vbo = vbo_context(ctx);
   GLfloat vertex[VBO_ATTRIB_MAX * 4], *data = vertex;
   GLuint i, offset;

   if (node->count)
      offset = (node->buffer_offset + 
		(node->count-1) * node->vertex_size * sizeof(GLfloat));
   else
      offset = node->buffer_offset;

   ctx->Driver.GetBufferSubData( ctx, 0, offset, 
				 node->vertex_size * sizeof(GLfloat), 
				 data, node->vertex_store->bufferobj );

   data += node->attrsz[0]; /* skip vertex position */

   for (i = VBO_ATTRIB_POS+1 ; i < VBO_ATTRIB_MAX ; i++) {
      if (node->attrsz[i]) {
	 GLfloat *current = (GLfloat *)vbo->currval[i].Ptr;

	 COPY_CLEAN_4V(current, 
		       node->attrsz[i], 
		       data);

	 vbo->currval[i].Size = node->attrsz[i];

	 data += node->attrsz[i];

	 if (i >= VBO_ATTRIB_FIRST_MATERIAL &&
	     i <= VBO_ATTRIB_LAST_MATERIAL)
	    ctx->NewState |= _NEW_LIGHT;
      }
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material(ctx, ctx->Current.Attrib[VBO_ATTRIB_COLOR0]);
   }

   /* CurrentExecPrimitive
    */
   if (node->prim_count) {
      const struct _mesa_prim *prim = &node->prim[node->prim_count - 1];
      if (prim->end)
	 ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
      else
	 ctx->Driver.CurrentExecPrimitive = prim->mode;
   }
}



/* Treat the vertex storage as a VBO, define vertex arrays pointing
 * into it:
 */
static void vbo_bind_vertex_list( GLcontext *ctx,
                                   const struct vbo_save_vertex_list *node )
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_save_context *save = &vbo->save;
   struct gl_client_array *arrays = save->arrays;
   GLuint data = node->buffer_offset;
   const GLuint *map;
   GLuint attr;

   /* Install the default (ie Current) attributes first, then overlay
    * all active ones.
    */
   switch (get_program_mode(ctx)) {
   case VP_NONE:
      memcpy(arrays,      vbo->legacy_currval, 16 * sizeof(arrays[0]));
      memcpy(arrays + 16, vbo->mat_currval,    MAT_ATTRIB_MAX * sizeof(arrays[0]));
      map = vbo->map_vp_none;
      break;
   case VP_NV:
   case VP_ARB:
      /* The aliasing of attributes for NV vertex programs has already
       * occurred.  NV vertex programs cannot access material values,
       * nor attributes greater than VERT_ATTRIB_TEX7.  
       */
      memcpy(arrays,      vbo->legacy_currval,  16 * sizeof(arrays[0]));
      memcpy(arrays + 16, vbo->generic_currval, 16 * sizeof(arrays[0]));
      map = vbo->map_vp_arb;
      break;
   }

   for (attr = 0; attr < VBO_ATTRIB_MAX; attr++) {
      GLuint src = map[attr];

      if (node->attrsz[src]) {
	 arrays[attr].Ptr = (const GLubyte *)data;
	 arrays[attr].Size = node->attrsz[src];
	 arrays[attr].StrideB = node->vertex_size * sizeof(GLfloat);
	 arrays[attr].Stride = node->vertex_size * sizeof(GLfloat);
	 arrays[attr].Type = GL_FLOAT;
	 arrays[attr].Enabled = 1;
	 arrays[attr].BufferObj = node->vertex_store->bufferobj;
	 arrays[attr]._MaxElement = node->count; /* ??? */
	 
	 assert(arrays[attr].BufferObj->Name);

	 data += node->attrsz[attr] * sizeof(GLfloat);
      }
   }
}

static void vbo_save_loopback_vertex_list( GLcontext *ctx,
					   const struct vbo_save_vertex_list *list )
{
   const char *buffer = ctx->Driver.MapBuffer(ctx, 
					      GL_ARRAY_BUFFER_ARB, 
					      GL_READ_ONLY, /* ? */
					       list->vertex_store->bufferobj);

   vbo_loopback_vertex_list( ctx,
			     (const GLfloat *)(buffer + list->buffer_offset),
			     list->attrsz,
			     list->prim,
			     list->prim_count,
			     list->wrap_count,
			     list->vertex_size);

   ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER_ARB, 
			   list->vertex_store->bufferobj);
}


/**
 * Execute the buffer and save copied verts.
 */
void vbo_save_playback_vertex_list( GLcontext *ctx, void *data )
{
   const struct vbo_save_vertex_list *node = (const struct vbo_save_vertex_list *) data;
   struct vbo_save_context *save = &vbo_context(ctx)->save;

   FLUSH_CURRENT(ctx, 0);

   if (node->prim_count > 0 && node->count > 0) {

      if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END &&
	  node->prim[0].begin) {

	 /* Degenerate case: list is called inside begin/end pair and
	  * includes operations such as glBegin or glDrawArrays.
	  */
	 if (0)
	    _mesa_printf("displaylist recursive begin");

	 vbo_save_loopback_vertex_list( ctx, node );
	 return;
      }
      else if (save->replay_flags) {
	 /* Various degnerate cases: translate into immediate mode
	  * calls rather than trying to execute in place.
	  */
	 vbo_save_loopback_vertex_list( ctx, node );
	 return;
      }
      
      if (ctx->NewState)
	 _mesa_update_state( ctx );

      /* XXX also need to check if shader enabled, but invalid */
      if ((ctx->VertexProgram.Enabled && !ctx->VertexProgram._Enabled) ||
          (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBegin (invalid vertex/fragment program)");
         return;
      }

      vbo_bind_vertex_list( ctx, node );

      vbo_context(ctx)->draw_prims( ctx, 
				    save->inputs, 
				    node->prim, 
				    node->prim_count,
				    NULL,
				    0,	/* Node is a VBO, so this is ok */
				    node->count - 1);
   }

   /* Copy to current?
    */
   _playback_copy_to_current( ctx, node );
}
