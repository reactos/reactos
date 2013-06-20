/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/imports.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/mfeatures.h"

#include "vbo/vbo.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bufferobjects.h"
#include "st_draw.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "draw/draw_private.h"
#include "draw/draw_context.h"


#if FEATURE_feedback || FEATURE_rastpos

/**
 * Set the (private) draw module's post-transformed vertex format when in
 * GL_SELECT or GL_FEEDBACK mode or for glRasterPos.
 */
static void
set_feedback_vertex_format(struct gl_context *ctx)
{
#if 0
   struct st_context *st = st_context(ctx);
   struct vertex_info vinfo;
   GLuint i;

   memset(&vinfo, 0, sizeof(vinfo));

   if (ctx->RenderMode == GL_SELECT) {
      assert(ctx->RenderMode == GL_SELECT);
      vinfo.num_attribs = 1;
      vinfo.format[0] = FORMAT_4F;
      vinfo.interp_mode[0] = INTERP_LINEAR;
   }
   else {
      /* GL_FEEDBACK, or glRasterPos */
      /* emit all attribs (pos, color, texcoord) as GLfloat[4] */
      vinfo.num_attribs = st->state.vs->cso->state.num_outputs;
      for (i = 0; i < vinfo.num_attribs; i++) {
         vinfo.format[i] = FORMAT_4F;
         vinfo.interp_mode[i] = INTERP_LINEAR;
      }
   }

   draw_set_vertex_info(st->draw, &vinfo);
#endif
}


/**
 * Called by VBO to draw arrays when in selection or feedback mode and
 * to implement glRasterPos.
 * This is very much like the normal draw_vbo() function above.
 * Look at code refactoring some day.
 */
void
st_feedback_draw_vbo(struct gl_context *ctx,
                     const struct gl_client_array **arrays,
                     const struct _mesa_prim *prims,
                     GLuint nr_prims,
                     const struct _mesa_index_buffer *ib,
		     GLboolean index_bounds_valid,
                     GLuint min_index,
                     GLuint max_index,
                     struct gl_transform_feedback_object *tfb_vertcount)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct draw_context *draw = st->draw;
   const struct st_vertex_program *vp;
   const struct pipe_shader_state *vs;
   struct pipe_vertex_buffer vbuffers[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_element velements[PIPE_MAX_ATTRIBS];
   struct pipe_index_buffer ibuffer;
   struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS];
   struct pipe_transfer *ib_transfer = NULL;
   GLuint attr, i;
   const GLubyte *low_addr = NULL;
   const void *mapped_indices = NULL;

   assert(draw);

   st_validate_state(st);

   if (!index_bounds_valid)
      vbo_get_minmax_index(ctx, prims, ib, &min_index, &max_index);

   /* must get these after state validation! */
   vp = st->vp;
   vs = &st->vp_variant->tgsi;

   if (!st->vp_variant->draw_shader) {
      st->vp_variant->draw_shader = draw_create_vertex_shader(draw, vs);
   }

   /*
    * Set up the draw module's state.
    *
    * We'd like to do this less frequently, but the normal state-update
    * code sends state updates to the pipe, not to our private draw module.
    */
   assert(draw);
   draw_set_viewport_state(draw, &st->state.viewport);
   draw_set_clip_state(draw, &st->state.clip);
   draw_set_rasterizer_state(draw, &st->state.rasterizer, NULL);
   draw_bind_vertex_shader(draw, st->vp_variant->draw_shader);
   set_feedback_vertex_format(ctx);

   /* Find the lowest address of the arrays we're drawing */
   if (vp->num_inputs) {
      low_addr = arrays[vp->index_to_input[0]]->Ptr;

      for (attr = 1; attr < vp->num_inputs; attr++) {
         const GLubyte *start = arrays[vp->index_to_input[attr]]->Ptr;
         low_addr = MIN2(low_addr, start);
      }
   }

   /* loop over TGSI shader inputs to determine vertex buffer
    * and attribute info
    */
   for (attr = 0; attr < vp->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      struct gl_buffer_object *bufobj = arrays[mesaAttr]->BufferObj;
      void *map;

      if (bufobj && bufobj->Name) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         assert(stobj->buffer);

         vbuffers[attr].buffer = NULL;
         pipe_resource_reference(&vbuffers[attr].buffer, stobj->buffer);
         vbuffers[attr].buffer_offset = pointer_to_offset(low_addr);
         velements[attr].src_offset = arrays[mesaAttr]->Ptr - low_addr;
      }
      else {
         /* attribute data is in user-space memory, not a VBO */
         uint bytes = (arrays[mesaAttr]->Size
                       * _mesa_sizeof_type(arrays[mesaAttr]->Type)
                       * (max_index + 1));

         /* wrap user data */
         vbuffers[attr].buffer
            = pipe_user_buffer_create(pipe->screen, (void *) arrays[mesaAttr]->Ptr,
                                      bytes,
				      PIPE_BIND_VERTEX_BUFFER);
         vbuffers[attr].buffer_offset = 0;
         velements[attr].src_offset = 0;
      }

      /* common-case setup */
      vbuffers[attr].stride = arrays[mesaAttr]->StrideB; /* in bytes */
      velements[attr].instance_divisor = 0;
      velements[attr].vertex_buffer_index = attr;
      velements[attr].src_format = 
         st_pipe_vertex_format(arrays[mesaAttr]->Type,
                               arrays[mesaAttr]->Size,
                               arrays[mesaAttr]->Format,
                               arrays[mesaAttr]->Normalized,
                               arrays[mesaAttr]->Integer);
      assert(velements[attr].src_format);

      /* tell draw about this attribute */
#if 0
      draw_set_vertex_buffer(draw, attr, &vbuffer[attr]);
#endif

      /* map the attrib buffer */
      map = pipe_buffer_map(pipe, vbuffers[attr].buffer,
                            PIPE_TRANSFER_READ,
			    &vb_transfer[attr]);
      draw_set_mapped_vertex_buffer(draw, attr, map);
   }

   draw_set_vertex_buffers(draw, vp->num_inputs, vbuffers);
   draw_set_vertex_elements(draw, vp->num_inputs, velements);

   memset(&ibuffer, 0, sizeof(ibuffer));
   if (ib) {
      struct gl_buffer_object *bufobj = ib->obj;

      ibuffer.index_size = vbo_sizeof_ib_type(ib->type);
      if (ibuffer.index_size == 0)
         goto out_unref_vertex;

      if (bufobj && bufobj->Name) {
         struct st_buffer_object *stobj = st_buffer_object(bufobj);

         pipe_resource_reference(&ibuffer.buffer, stobj->buffer);
         ibuffer.offset = pointer_to_offset(ib->ptr);

         mapped_indices = pipe_buffer_map(pipe, stobj->buffer,
                                          PIPE_TRANSFER_READ, &ib_transfer);
      }
      else {
         /* skip setting ibuffer.buffer as the draw module does not use it */
         mapped_indices = ib->ptr;
      }

      draw_set_index_buffer(draw, &ibuffer);
      draw_set_mapped_index_buffer(draw, mapped_indices);
   }

   /* set the constant buffer */
   draw_set_mapped_constant_buffer(st->draw, PIPE_SHADER_VERTEX, 0,
                                   st->state.constants[PIPE_SHADER_VERTEX].ptr,
                                   st->state.constants[PIPE_SHADER_VERTEX].size);


   /* draw here */
   for (i = 0; i < nr_prims; i++) {
      draw_arrays(draw, prims[i].mode, prims[i].start, prims[i].count);
   }


   /*
    * unmap vertex/index buffers
    */
   if (ib) {
      draw_set_mapped_index_buffer(draw, NULL);
      draw_set_index_buffer(draw, NULL);

      if (ib_transfer)
         pipe_buffer_unmap(pipe, ib_transfer);
      pipe_resource_reference(&ibuffer.buffer, NULL);
   }

 out_unref_vertex:
   for (attr = 0; attr < vp->num_inputs; attr++) {
      pipe_buffer_unmap(pipe, vb_transfer[attr]);
      draw_set_mapped_vertex_buffer(draw, attr, NULL);
      pipe_resource_reference(&vbuffers[attr].buffer, NULL);
   }
   draw_set_vertex_buffers(draw, 0, NULL);
}

#endif /* FEATURE_feedback || FEATURE_rastpos */

