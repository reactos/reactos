/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  *   Michel Dänzer
  */

#include "main/glheader.h"
#include "main/accum.h"
#include "main/formats.h"
#include "main/macros.h"
#include "program/prog_instruction.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_cb_clear.h"
#include "st_cb_fbo.h"
#include "st_format.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_simple_shaders.h"
#include "util/u_draw_quad.h"

#include "cso_cache/cso_context.h"


/**
 * Do per-context initialization for glClear.
 */
void
st_init_clear(struct st_context *st)
{
   struct pipe_screen *pscreen = st->pipe->screen;

   memset(&st->clear, 0, sizeof(st->clear));

   st->clear.raster.gl_rasterization_rules = 1;
   st->clear.raster.depth_clip = 1;
   st->clear.enable_ds_separate = pscreen->get_param(pscreen, PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE);
}


/**
 * Free per-context state for glClear.
 */
void
st_destroy_clear(struct st_context *st)
{
   if (st->clear.fs) {
      cso_delete_fragment_shader(st->cso_context, st->clear.fs);
      st->clear.fs = NULL;
   }
   if (st->clear.vs) {
      cso_delete_vertex_shader(st->cso_context, st->clear.vs);
      st->clear.vs = NULL;
   }
   if (st->clear.vbuf) {
      pipe_resource_reference(&st->clear.vbuf, NULL);
      st->clear.vbuf = NULL;
   }
}


/**
 * Helper function to set the fragment shaders.
 */
static INLINE void
set_fragment_shader(struct st_context *st)
{
   if (!st->clear.fs)
      st->clear.fs = util_make_fragment_passthrough_shader(st->pipe);

   cso_set_fragment_shader_handle(st->cso_context, st->clear.fs);
}


/**
 * Helper function to set the vertex shader.
 */
static INLINE void
set_vertex_shader(struct st_context *st)
{
   /* vertex shader - still required to provide the linkage between
    * fragment shader input semantics and vertex_element/buffers.
    */
   if (!st->clear.vs)
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_COLOR };
      const uint semantic_indexes[] = { 0, 0 };
      st->clear.vs = util_make_vertex_passthrough_shader(st->pipe, 2,
                                                         semantic_names,
                                                         semantic_indexes);
   }

   cso_set_vertex_shader_handle(st->cso_context, st->clear.vs);
}


/**
 * Draw a screen-aligned quadrilateral.
 * Coords are clip coords with y=0=bottom.
 */
static void
draw_quad(struct st_context *st,
          float x0, float y0, float x1, float y1, GLfloat z,
          const union pipe_color_union *color)
{
   struct pipe_context *pipe = st->pipe;

   /* XXX: Need to improve buffer_write to allow NO_WAIT (as well as
    * no_flush) updates to buffers where we know there is no conflict
    * with previous data.  Currently using max_slots > 1 will cause
    * synchronous rendering if the driver flushes its command buffers
    * between one bitmap and the next.  Our flush hook below isn't
    * sufficient to catch this as the driver doesn't tell us when it
    * flushes its own command buffers.  Until this gets fixed, pay the
    * price of allocating a new buffer for each bitmap cache-flush to
    * avoid synchronous rendering.
    */
   const GLuint max_slots = 1; /* 1024 / sizeof(st->clear.vertices); */
   GLuint i;

   if (st->clear.vbuf_slot >= max_slots) {
      pipe_resource_reference(&st->clear.vbuf, NULL);
      st->clear.vbuf_slot = 0;
   }

   if (!st->clear.vbuf) {
      st->clear.vbuf = pipe_buffer_create(pipe->screen,
                                          PIPE_BIND_VERTEX_BUFFER,
                                          PIPE_USAGE_STREAM,
                                          max_slots * sizeof(st->clear.vertices));
   }

   if (!st->clear.vbuf) {
      /* ran out of memory */
      return;
   }

   /* positions */
   st->clear.vertices[0][0][0] = x0;
   st->clear.vertices[0][0][1] = y0;

   st->clear.vertices[1][0][0] = x1;
   st->clear.vertices[1][0][1] = y0;

   st->clear.vertices[2][0][0] = x1;
   st->clear.vertices[2][0][1] = y1;

   st->clear.vertices[3][0][0] = x0;
   st->clear.vertices[3][0][1] = y1;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      st->clear.vertices[i][0][2] = z;
      st->clear.vertices[i][0][3] = 1.0;
      st->clear.vertices[i][1][0] = color->f[0];
      st->clear.vertices[i][1][1] = color->f[1];
      st->clear.vertices[i][1][2] = color->f[2];
      st->clear.vertices[i][1][3] = color->f[3];
   }

   /* put vertex data into vbuf */
   pipe_buffer_write_nooverlap(st->pipe, st->clear.vbuf,
                                           st->clear.vbuf_slot
                                             * sizeof(st->clear.vertices),
                                           sizeof(st->clear.vertices),
                                           st->clear.vertices);

   /* draw */
   util_draw_vertex_buffer(pipe,
                           st->cso_context,
                           st->clear.vbuf, 
                           st->clear.vbuf_slot * sizeof(st->clear.vertices),
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

   /* Increment slot */
   st->clear.vbuf_slot++;
}



/**
 * Do glClear by drawing a quadrilateral.
 * The vertices of the quad will be computed from the
 * ctx->DrawBuffer->_X/Ymin/max fields.
 */
static void
clear_with_quad(struct gl_context *ctx,
                GLboolean color, GLboolean depth, GLboolean stencil)
{
   struct st_context *st = st_context(ctx);
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   const GLfloat fb_width = (GLfloat) fb->Width;
   const GLfloat fb_height = (GLfloat) fb->Height;
   const GLfloat x0 = (GLfloat) ctx->DrawBuffer->_Xmin / fb_width * 2.0f - 1.0f;
   const GLfloat x1 = (GLfloat) ctx->DrawBuffer->_Xmax / fb_width * 2.0f - 1.0f;
   const GLfloat y0 = (GLfloat) ctx->DrawBuffer->_Ymin / fb_height * 2.0f - 1.0f;
   const GLfloat y1 = (GLfloat) ctx->DrawBuffer->_Ymax / fb_height * 2.0f - 1.0f;
   union pipe_color_union clearColor;

   /*
   printf("%s %s%s%s %f,%f %f,%f\n", __FUNCTION__, 
	  color ? "color, " : "",
	  depth ? "depth, " : "",
	  stencil ? "stencil" : "",
	  x0, y0,
	  x1, y1);
   */

   cso_save_blend(st->cso_context);
   cso_save_stencil_ref(st->cso_context);
   cso_save_depth_stencil_alpha(st->cso_context);
   cso_save_rasterizer(st->cso_context);
   cso_save_viewport(st->cso_context);
   cso_save_fragment_shader(st->cso_context);
   cso_save_stream_outputs(st->cso_context);
   cso_save_vertex_shader(st->cso_context);
   cso_save_geometry_shader(st->cso_context);
   cso_save_vertex_elements(st->cso_context);
   cso_save_vertex_buffers(st->cso_context);

   /* blend state: RGBA masking */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      if (color) {
         if (ctx->Color.ColorMask[0][0])
            blend.rt[0].colormask |= PIPE_MASK_R;
         if (ctx->Color.ColorMask[0][1])
            blend.rt[0].colormask |= PIPE_MASK_G;
         if (ctx->Color.ColorMask[0][2])
            blend.rt[0].colormask |= PIPE_MASK_B;
         if (ctx->Color.ColorMask[0][3])
            blend.rt[0].colormask |= PIPE_MASK_A;
         if (st->ctx->Color.DitherFlag)
            blend.dither = 1;
      }
      cso_set_blend(st->cso_context, &blend);
   }

   /* depth_stencil state: always pass/set to ref value */
   {
      struct pipe_depth_stencil_alpha_state depth_stencil;
      memset(&depth_stencil, 0, sizeof(depth_stencil));
      if (depth) {
         depth_stencil.depth.enabled = 1;
         depth_stencil.depth.writemask = 1;
         depth_stencil.depth.func = PIPE_FUNC_ALWAYS;
      }

      if (stencil) {
         struct pipe_stencil_ref stencil_ref;
         memset(&stencil_ref, 0, sizeof(stencil_ref));
         depth_stencil.stencil[0].enabled = 1;
         depth_stencil.stencil[0].func = PIPE_FUNC_ALWAYS;
         depth_stencil.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].valuemask = 0xff;
         depth_stencil.stencil[0].writemask = ctx->Stencil.WriteMask[0] & 0xff;
         stencil_ref.ref_value[0] = ctx->Stencil.Clear;
         cso_set_stencil_ref(st->cso_context, &stencil_ref);
      }

      cso_set_depth_stencil_alpha(st->cso_context, &depth_stencil);
   }

   cso_set_vertex_elements(st->cso_context, 2, st->velems_util_draw);
   cso_set_stream_outputs(st->cso_context, 0, NULL, 0);

   cso_set_rasterizer(st->cso_context, &st->clear.raster);

   /* viewport state: viewport matching window dims */
   {
      const GLboolean invert = (st_fb_orientation(fb) == Y_0_TOP);
      struct pipe_viewport_state vp;
      vp.scale[0] = 0.5f * fb_width;
      vp.scale[1] = fb_height * (invert ? -0.5f : 0.5f);
      vp.scale[2] = 1.0f;
      vp.scale[3] = 1.0f;
      vp.translate[0] = 0.5f * fb_width;
      vp.translate[1] = 0.5f * fb_height;
      vp.translate[2] = 0.0f;
      vp.translate[3] = 0.0f;
      cso_set_viewport(st->cso_context, &vp);
   }

   set_fragment_shader(st);
   set_vertex_shader(st);
   cso_set_geometry_shader_handle(st->cso_context, NULL);

   if (ctx->DrawBuffer->_ColorDrawBuffers[0]) {
      st_translate_color(ctx->Color.ClearColor.f,
                               ctx->DrawBuffer->_ColorDrawBuffers[0]->_BaseFormat,
                               clearColor.f);
   }

   /* draw quad matching scissor rect */
   draw_quad(st, x0, y0, x1, y1, (GLfloat) ctx->Depth.Clear, &clearColor);

   /* Restore pipe state */
   cso_restore_blend(st->cso_context);
   cso_restore_stencil_ref(st->cso_context);
   cso_restore_depth_stencil_alpha(st->cso_context);
   cso_restore_rasterizer(st->cso_context);
   cso_restore_viewport(st->cso_context);
   cso_restore_fragment_shader(st->cso_context);
   cso_restore_vertex_shader(st->cso_context);
   cso_restore_geometry_shader(st->cso_context);
   cso_restore_vertex_elements(st->cso_context);
   cso_restore_vertex_buffers(st->cso_context);
   cso_restore_stream_outputs(st->cso_context);
}


/**
 * Determine if we need to clear the depth buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_color_with_quad(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return GL_TRUE;

   if (!ctx->Color.ColorMask[0][0] ||
       !ctx->Color.ColorMask[0][1] ||
       !ctx->Color.ColorMask[0][2] ||
       !ctx->Color.ColorMask[0][3])
      return GL_TRUE;

   return GL_FALSE;
}


/**
 * Determine if we need to clear the combiend depth/stencil buffer by
 * drawing a quad.
 */
static INLINE GLboolean
check_clear_depth_stencil_with_quad(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   const GLuint stencilMax = 0xff;
   GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   assert(rb->Format == MESA_FORMAT_S8 ||
          rb->Format == MESA_FORMAT_Z24_S8 ||
          rb->Format == MESA_FORMAT_S8_Z24 ||
          rb->Format == MESA_FORMAT_Z32_FLOAT_X24S8);

   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return GL_TRUE;

   if (maskStencil)
      return GL_TRUE;

   return GL_FALSE;
}


/**
 * Determine if we need to clear the depth buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_depth_with_quad(struct gl_context *ctx, struct gl_renderbuffer *rb,
                            boolean ds_separate)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLboolean isDS = util_format_is_depth_and_stencil(strb->surface->format);

   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return GL_TRUE;

   if (!ds_separate && isDS && ctx->DrawBuffer->Visual.stencilBits > 0)
      return GL_TRUE;

   return GL_FALSE;
}


/**
 * Determine if we need to clear the stencil buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_stencil_with_quad(struct gl_context *ctx, struct gl_renderbuffer *rb,
                              boolean ds_separate)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLboolean isDS = util_format_is_depth_and_stencil(strb->surface->format);
   const GLuint stencilMax = 0xff;
   const GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   assert(rb->Format == MESA_FORMAT_S8 ||
          rb->Format == MESA_FORMAT_Z24_S8 ||
          rb->Format == MESA_FORMAT_S8_Z24 ||
          rb->Format == MESA_FORMAT_Z32_FLOAT_X24S8);

   if (maskStencil) 
      return GL_TRUE;

   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return GL_TRUE;

   /* This is correct, but it is necessary to look at the depth clear
    * value held in the surface when it comes time to issue the clear,
    * rather than taking depth and stencil clear values from the
    * current state.
    */
   if (!ds_separate && isDS && ctx->DrawBuffer->Visual.depthBits > 0)
      return GL_TRUE;

   return GL_FALSE;
}



/**
 * Called when we need to flush.
 */
void
st_flush_clear(struct st_context *st)
{
   /* Release vertex buffer to avoid synchronous rendering if we were
    * to map it in the next frame.
    */
   pipe_resource_reference(&st->clear.vbuf, NULL);
   st->clear.vbuf_slot = 0;
}
 


/**
 * Called via ctx->Driver.Clear()
 */
static void
st_Clear(struct gl_context *ctx, GLbitfield mask)
{
   static const GLbitfield BUFFER_BITS_DS
      = (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
   struct st_context *st = st_context(ctx);
   struct gl_renderbuffer *depthRb
      = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb
      = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLbitfield quad_buffers = 0x0;
   GLbitfield clear_buffers = 0x0;
   GLuint i;

   /* This makes sure the pipe has the latest scissor, etc values */
   st_validate_state( st );

   if (mask & BUFFER_BITS_COLOR) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
         GLuint b = ctx->DrawBuffer->_ColorDrawBufferIndexes[i];

         if (mask & (1 << b)) {
            struct gl_renderbuffer *rb
               = ctx->DrawBuffer->Attachment[b].Renderbuffer;
            struct st_renderbuffer *strb = st_renderbuffer(rb);

            if (!strb || !strb->surface)
               continue;

            if (check_clear_color_with_quad( ctx, rb ))
               quad_buffers |= PIPE_CLEAR_COLOR;
            else
               clear_buffers |= PIPE_CLEAR_COLOR;
         }
      }
   }

   if ((mask & BUFFER_BITS_DS) == BUFFER_BITS_DS && depthRb == stencilRb) {
      /* clearing combined depth + stencil */
      struct st_renderbuffer *strb = st_renderbuffer(depthRb);

      if (strb->surface) {
         if (check_clear_depth_stencil_with_quad(ctx, depthRb))
            quad_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
         else
            clear_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
      }
   }
   else {
      /* separate depth/stencil clears */
      /* I don't think truly separate buffers are actually possible in gallium or hw? */
      if (mask & BUFFER_BIT_DEPTH) {
         struct st_renderbuffer *strb = st_renderbuffer(depthRb);

         if (strb->surface) {
            if (check_clear_depth_with_quad(ctx, depthRb,
                                            st->clear.enable_ds_separate))
               quad_buffers |= PIPE_CLEAR_DEPTH;
            else
               clear_buffers |= PIPE_CLEAR_DEPTH;
         }
      }
      if (mask & BUFFER_BIT_STENCIL) {
         struct st_renderbuffer *strb = st_renderbuffer(stencilRb);

         if (strb->surface) {
            if (check_clear_stencil_with_quad(ctx, stencilRb,
                                              st->clear.enable_ds_separate))
               quad_buffers |= PIPE_CLEAR_STENCIL;
            else
               clear_buffers |= PIPE_CLEAR_STENCIL;
         }
      }
   }

   /*
    * If we're going to use clear_with_quad() for any reason, use it for
    * everything possible.
    */
   if (quad_buffers) {
      quad_buffers |= clear_buffers;
      clear_with_quad(ctx,
                      quad_buffers & PIPE_CLEAR_COLOR,
                      quad_buffers & PIPE_CLEAR_DEPTH,
                      quad_buffers & PIPE_CLEAR_STENCIL);
   } else if (clear_buffers) {
      /* driver cannot know it can clear everything if the buffer
       * is a combined depth/stencil buffer but this wasn't actually
       * required from the visual. Hence fix this up to avoid potential
       * read-modify-write in the driver.
       */
      union pipe_color_union clearColor;

      if ((clear_buffers & PIPE_CLEAR_DEPTHSTENCIL) &&
          ((clear_buffers & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
          (depthRb == stencilRb) &&
          (ctx->DrawBuffer->Visual.depthBits == 0 ||
           ctx->DrawBuffer->Visual.stencilBits == 0))
         clear_buffers |= PIPE_CLEAR_DEPTHSTENCIL;

      if (ctx->DrawBuffer->_ColorDrawBuffers[0]) {
         st_translate_color(ctx->Color.ClearColor.f,
			    ctx->DrawBuffer->_ColorDrawBuffers[0]->_BaseFormat,
			    clearColor.f);
      }

      st->pipe->clear(st->pipe, clear_buffers, &clearColor,
                      ctx->Depth.Clear, ctx->Stencil.Clear);
   }
   if (mask & BUFFER_BIT_ACCUM)
      _mesa_clear_accum_buffer(ctx);
}


void
st_init_clear_functions(struct dd_function_table *functions)
{
   functions->Clear = st_Clear;
}
