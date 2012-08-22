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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */
 
#include "st_context.h"
#include "st_atom.h"
#include "st_cb_fbo.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "cso_cache/cso_context.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_format.h"


/**
 * When doing GL render to texture, we have to be sure that finalize_texture()
 * didn't yank out the pipe_resource that we earlier created a surface for.
 * Check for that here and create a new surface if needed.
 */
static void
update_renderbuffer_surface(struct st_context *st,
                            struct st_renderbuffer *strb)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_resource *resource = strb->rtt->pt;
   int rtt_width = strb->Base.Width;
   int rtt_height = strb->Base.Height;
   enum pipe_format format = st->ctx->Color.sRGBEnabled ? resource->format : util_format_linear(resource->format);

   if (!strb->surface ||
       strb->surface->format != format ||
       strb->surface->texture != resource ||
       strb->surface->width != rtt_width ||
       strb->surface->height != rtt_height) {
      GLuint level;
      /* find matching mipmap level size */
      for (level = 0; level <= resource->last_level; level++) {
         if (u_minify(resource->width0, level) == rtt_width &&
             u_minify(resource->height0, level) == rtt_height) {
            struct pipe_surface surf_tmpl;
            memset(&surf_tmpl, 0, sizeof(surf_tmpl));
            surf_tmpl.format = format;
            surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
            surf_tmpl.u.tex.level = level;
            surf_tmpl.u.tex.first_layer = strb->rtt_face + strb->rtt_slice;
            surf_tmpl.u.tex.last_layer = strb->rtt_face + strb->rtt_slice;

            pipe_surface_reference(&strb->surface, NULL);

            strb->surface = pipe->create_surface(pipe,
                                                 resource,
                                                 &surf_tmpl);
#if 0
            printf("-- alloc new surface %d x %d into tex %p\n",
                   strb->surface->width, strb->surface->height,
                   texture);
#endif
            break;
         }
      }
   }
}


/**
 * Update framebuffer state (color, depth, stencil, etc. buffers)
 */
static void
update_framebuffer_state( struct st_context *st )
{
   struct pipe_framebuffer_state *framebuffer = &st->state.framebuffer;
   struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   struct st_renderbuffer *strb;
   GLuint i;

   framebuffer->width = fb->Width;
   framebuffer->height = fb->Height;

   /*printf("------ fb size %d x %d\n", fb->Width, fb->Height);*/

   /* Examine Mesa's ctx->DrawBuffer->_ColorDrawBuffers state
    * to determine which surfaces to draw to
    */
   framebuffer->nr_cbufs = 0;
   for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
      strb = st_renderbuffer(fb->_ColorDrawBuffers[i]);

      if (strb) {
         /*printf("--------- framebuffer surface rtt %p\n", strb->rtt);*/
         if (strb->rtt) {
            /* rendering to a GL texture, may have to update surface */
            update_renderbuffer_surface(st, strb);
         }

         if (strb->surface) {
            pipe_surface_reference(&framebuffer->cbufs[framebuffer->nr_cbufs],
                                   strb->surface);
            framebuffer->nr_cbufs++;
         }
         strb->defined = GL_TRUE; /* we'll be drawing something */
      }
   }
   for (i = framebuffer->nr_cbufs; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&framebuffer->cbufs[i], NULL);
   }

   /*
    * Depth/Stencil renderbuffer/surface.
    */
   strb = st_renderbuffer(fb->Attachment[BUFFER_DEPTH].Renderbuffer);
   if (strb) {
      if (strb->rtt) {
         /* rendering to a GL texture, may have to update surface */
         update_renderbuffer_surface(st, strb);
      }
      pipe_surface_reference(&framebuffer->zsbuf, strb->surface);
   }
   else {
      strb = st_renderbuffer(fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      if (strb) {
         assert(strb->surface);
         pipe_surface_reference(&framebuffer->zsbuf, strb->surface);
      }
      else
         pipe_surface_reference(&framebuffer->zsbuf, NULL);
   }

#ifdef DEBUG
   /* Make sure the resource binding flags were set properly */
   for (i = 0; i < framebuffer->nr_cbufs; i++) {
      assert(framebuffer->cbufs[i]->texture->bind & PIPE_BIND_RENDER_TARGET);
   }
   if (framebuffer->zsbuf) {
      assert(framebuffer->zsbuf->texture->bind & PIPE_BIND_DEPTH_STENCIL);
   }
#endif

   cso_set_framebuffer(st->cso_context, framebuffer);
}


const struct st_tracked_state st_update_framebuffer = {
   "st_update_framebuffer",				/* name */
   {							/* dirty */
      _NEW_BUFFERS,					/* mesa */
      ST_NEW_FRAMEBUFFER,				/* st */
   },
   update_framebuffer_state				/* update */
};

