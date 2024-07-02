/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

#include <precomp.h>

#if FEATURE_dlist


/**
 * After playback, copy everything but the position from the
 * last vertex to the saved state
 */
static void
_playback_copy_to_current(struct gl_context *ctx,
                          const struct vbo_save_vertex_list *node)
{
   struct vbo_context *vbo = vbo_context(ctx);
   GLfloat vertex[VBO_ATTRIB_MAX * 4];
   GLfloat *data;
   GLuint i, offset;

   if (node->current_size == 0)
      return;

   if (node->current_data) {
      data = node->current_data;
   }
   else {
      data = vertex;

      if (node->count)
         offset = (node->buffer_offset + 
                   (node->count-1) * node->vertex_size * sizeof(GLfloat));
      else
         offset = node->buffer_offset;

      ctx->Driver.GetBufferSubData( ctx, offset,
                                    node->vertex_size * sizeof(GLfloat), 
                                    data, node->vertex_store->bufferobj );

      data += node->attrsz[0]; /* skip vertex position */
   }

   for (i = VBO_ATTRIB_POS+1 ; i < VBO_ATTRIB_MAX ; i++) {
      if (node->attrsz[i]) {
	 GLfloat *current = (GLfloat *)vbo->currval[i].Ptr;
         GLfloat tmp[4];

         COPY_CLEAN_4V(tmp, 
                       node->attrsz[i], 
                       data);
         
         if (memcmp(current, tmp, 4 * sizeof(GLfloat)) != 0) {
            memcpy(current, tmp, 4 * sizeof(GLfloat));

            vbo->currval[i].Size = node->attrsz[i];
            assert(vbo->currval[i].Type == GL_FLOAT);
            vbo->currval[i]._ElementSize = vbo->currval[i].Size * sizeof(GLfloat);

            if (i >= VBO_ATTRIB_FIRST_MATERIAL &&
                i <= VBO_ATTRIB_LAST_MATERIAL)
               ctx->NewState |= _NEW_LIGHT;

            ctx->NewState |= _NEW_CURRENT_ATTRIB;
         }

	 data += node->attrsz[i];
      }
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material(ctx, ctx->Current.Attrib[VBO_ATTRIB_COLOR]);
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



/**
 * Treat the vertex storage as a VBO, define vertex arrays pointing
 * into it:
 */
static void vbo_bind_vertex_list(struct gl_context *ctx,
                                 const struct vbo_save_vertex_list *node)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_save_context *save = &vbo->save;
   struct gl_client_array *arrays = save->arrays;
   GLuint buffer_offset = node->buffer_offset;
   GLuint attr;
   GLubyte node_attrsz[VBO_ATTRIB_MAX];  /* copy of node->attrsz[] */
   GLbitfield64 varying_inputs = 0x0;

   memcpy(node_attrsz, node->attrsz, sizeof(node->attrsz));

   /* Install the default (ie Current) attributes first, then overlay
    * all active ones.
    */
   for (attr = 0; attr < VBO_ATTRIB_MAX; attr++) {
      save->inputs[attr] = &vbo->currval[attr];
   }

   for (attr = 0; attr < VBO_ATTRIB_MAX; attr++) {

      if (node_attrsz[attr]) {
         /* override the default array set above */
         save->inputs[attr] = &arrays[attr];

	 arrays[attr].Ptr = (const GLubyte *) NULL + buffer_offset;
	 arrays[attr].Size = node_attrsz[attr];
	 arrays[attr].StrideB = node->vertex_size * sizeof(GLfloat);
	 arrays[attr].Stride = node->vertex_size * sizeof(GLfloat);
	 arrays[attr].Type = GL_FLOAT;
	 arrays[attr].Enabled = 1;
         arrays[attr]._ElementSize = arrays[attr].Size * sizeof(GLfloat);
         _mesa_reference_buffer_object(ctx,
                                       &arrays[attr].BufferObj,
                                       node->vertex_store->bufferobj);
	 arrays[attr]._MaxElement = node->count; /* ??? */
	 
	 assert(arrays[attr].BufferObj->Name);

	 buffer_offset += node_attrsz[attr] * sizeof(GLfloat);
         varying_inputs |= VERT_BIT(attr);
         ctx->NewState |= _NEW_ARRAY;
      }
   }
}


static void
vbo_save_loopback_vertex_list(struct gl_context *ctx,
                              const struct vbo_save_vertex_list *list)
{
   const char *buffer =
      ctx->Driver.MapBufferRange(ctx, 0,
				 list->vertex_store->bufferobj->Size,
				 GL_MAP_READ_BIT, /* ? */
				 list->vertex_store->bufferobj);

   vbo_loopback_vertex_list(ctx,
                            (const GLfloat *)(buffer + list->buffer_offset),
                            list->attrsz,
                            list->prim,
                            list->prim_count,
                            list->wrap_count,
                            list->vertex_size);

   ctx->Driver.UnmapBuffer(ctx, list->vertex_store->bufferobj);
}


/**
 * Execute the buffer and save copied verts.
 * This is called from the display list code when executing
 * a drawing command.
 */
void
vbo_save_playback_vertex_list(struct gl_context *ctx, void *data)
{
   const struct vbo_save_vertex_list *node =
      (const struct vbo_save_vertex_list *) data;
   struct vbo_save_context *save = &vbo_context(ctx)->save;
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   FLUSH_CURRENT(ctx, 0);

   if (node->prim_count > 0) {

      if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END &&
	  node->prim[0].begin) {

	 /* Degenerate case: list is called inside begin/end pair and
	  * includes operations such as glBegin or glDrawArrays.
	  */
	 if (0)
	    printf("displaylist recursive begin");

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

      vbo_bind_vertex_list( ctx, node );

      vbo_draw_method(exec, DRAW_DISPLAY_LIST);

      /* Again...
       */
      if (ctx->NewState)
	 _mesa_update_state( ctx );

      if (node->count > 0) {
         vbo_context(ctx)->draw_prims(ctx, 
                                      save->inputs, 
                                      node->prim, 
                                      node->prim_count,
                                      NULL,
                                      GL_TRUE,
                                      0,    /* Node is a VBO, so this is ok */
                                      node->count - 1);
      }
   }

   /* Copy to current?
    */
   _playback_copy_to_current( ctx, node );
}


#endif /* FEATURE_dlist */
