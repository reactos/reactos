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
  *   Brian Paul
  */

#include "main/imports.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/mfeatures.h"

#include "st_context.h"
#include "st_texture.h"
#include "st_cb_blit.h"
#include "st_cb_fbo.h"
#include "st_atom.h"

#include "util/u_blit.h"


void
st_init_blit(struct st_context *st)
{
   st->blit = util_create_blit(st->pipe, st->cso_context);
}


void
st_destroy_blit(struct st_context *st)
{
   util_destroy_blit(st->blit);
   st->blit = NULL;
}


#if FEATURE_EXT_framebuffer_blit

static void
st_BlitFramebuffer_resolve(struct gl_context *ctx,
                           GLbitfield mask,
                           struct pipe_resolve_info *info)
{
   const GLbitfield depthStencil = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

   struct st_context *st = st_context(ctx);

   struct st_renderbuffer *srcRb, *dstRb;

   if (mask & GL_COLOR_BUFFER_BIT) {
      srcRb = st_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
      dstRb = st_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0]);

      info->mask = PIPE_MASK_RGBA;

      info->src.res = srcRb->texture;
      info->src.layer = srcRb->surface->u.tex.first_layer;
      info->dst.res = dstRb->texture;
      info->dst.level = dstRb->surface->u.tex.level;
      info->dst.layer = dstRb->surface->u.tex.first_layer;

      st->pipe->resource_resolve(st->pipe, info);
   }

   if (mask & depthStencil) {
      struct gl_renderbuffer_attachment *srcDepth, *srcStencil;
      struct gl_renderbuffer_attachment *dstDepth, *dstStencil;
      boolean combined;

      srcDepth = &ctx->ReadBuffer->Attachment[BUFFER_DEPTH];
      dstDepth = &ctx->DrawBuffer->Attachment[BUFFER_DEPTH];
      srcStencil = &ctx->ReadBuffer->Attachment[BUFFER_STENCIL];
      dstStencil = &ctx->DrawBuffer->Attachment[BUFFER_STENCIL];

      combined =
         st_is_depth_stencil_combined(srcDepth, srcStencil) &&
         st_is_depth_stencil_combined(dstDepth, dstStencil);

      if ((mask & GL_DEPTH_BUFFER_BIT) || combined) {
         /* resolve depth and, if combined and requested, stencil as well */
         srcRb = st_renderbuffer(srcDepth->Renderbuffer);
         dstRb = st_renderbuffer(dstDepth->Renderbuffer);

         info->mask = (mask & GL_DEPTH_BUFFER_BIT) ? PIPE_MASK_Z : 0;
         if (combined && (mask & GL_STENCIL_BUFFER_BIT)) {
            mask &= ~GL_STENCIL_BUFFER_BIT;
            info->mask |= PIPE_MASK_S;
         }

         info->src.res = srcRb->texture;
         info->src.layer = srcRb->surface->u.tex.first_layer;
         info->dst.res = dstRb->texture;
         info->dst.level = dstRb->surface->u.tex.level;
         info->dst.layer = dstRb->surface->u.tex.first_layer;

         st->pipe->resource_resolve(st->pipe, info);
      }

      if (mask & GL_STENCIL_BUFFER_BIT) {
         /* resolve separate stencil buffer */
         srcRb = st_renderbuffer(srcStencil->Renderbuffer);
         dstRb = st_renderbuffer(dstStencil->Renderbuffer);

         info->mask = PIPE_MASK_S;

         info->src.res = srcRb->texture;
         info->src.layer = srcRb->surface->u.tex.first_layer;
         info->dst.res = dstRb->texture;
         info->dst.level = dstRb->surface->u.tex.level;
         info->dst.layer = dstRb->surface->u.tex.first_layer;

         st->pipe->resource_resolve(st->pipe, info);
      }
   }
}

static void
st_BlitFramebuffer(struct gl_context *ctx,
                   GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                   GLbitfield mask, GLenum filter)
{
   const GLbitfield depthStencil = (GL_DEPTH_BUFFER_BIT |
                                    GL_STENCIL_BUFFER_BIT);
   struct st_context *st = st_context(ctx);
   const uint pFilter = ((filter == GL_NEAREST)
                         ? PIPE_TEX_MIPFILTER_NEAREST
                         : PIPE_TEX_MIPFILTER_LINEAR);
   struct gl_framebuffer *readFB = ctx->ReadBuffer;
   struct gl_framebuffer *drawFB = ctx->DrawBuffer;

   st_validate_state(st);

   if (!_mesa_clip_blit(ctx, &srcX0, &srcY0, &srcX1, &srcY1,
                        &dstX0, &dstY0, &dstX1, &dstY1)) {
      return; /* nothing to draw/blit */
   }

   if (st_fb_orientation(drawFB) == Y_0_TOP) {
      /* invert Y for dest */
      dstY0 = drawFB->Height - dstY0;
      dstY1 = drawFB->Height - dstY1;
   }

   if (st_fb_orientation(readFB) == Y_0_TOP) {
      /* invert Y for src */
      srcY0 = readFB->Height - srcY0;
      srcY1 = readFB->Height - srcY1;
   }

   /* Disable conditional rendering. */
   if (st->render_condition) {
      st->pipe->render_condition(st->pipe, NULL, 0);
   }

   if (readFB->Visual.sampleBuffers > drawFB->Visual.sampleBuffers &&
       readFB->Visual.samples > 1) {
      struct pipe_resolve_info info;

      if (dstX0 < dstX1) {
         info.dst.x0 = dstX0;
         info.dst.x1 = dstX1;
         info.src.x0 = srcX0;
         info.src.x1 = srcX1;
      } else {
         info.dst.x0 = dstX1;
         info.dst.x1 = dstX0;
         info.src.x0 = srcX1;
         info.src.x1 = srcX0;
      }
      if (dstY0 < dstY1) {
         info.dst.y0 = dstY0;
         info.dst.y1 = dstY1;
         info.src.y0 = srcY0;
         info.src.y1 = srcY1;
      } else {
         info.dst.y0 = dstY1;
         info.dst.y1 = dstY0;
         info.src.y0 = srcY1;
         info.src.y1 = srcY0;
      }

      st_BlitFramebuffer_resolve(ctx, mask, &info); /* filter doesn't apply */

      goto done;
   }

   if (srcY0 > srcY1 && dstY0 > dstY1) {
      /* Both src and dst are upside down.  Swap Y to make it
       * right-side up to increase odds of using a fast path.
       * Recall that all Gallium raster coords have Y=0=top.
       */
      GLint tmp;
      tmp = srcY0;
      srcY0 = srcY1;
      srcY1 = tmp;
      tmp = dstY0;
      dstY0 = dstY1;
      dstY1 = tmp;
   }

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_renderbuffer_attachment *srcAtt =
         &readFB->Attachment[readFB->_ColorReadBufferIndex];

      if(srcAtt->Type == GL_TEXTURE) {
         struct st_texture_object *srcObj =
            st_texture_object(srcAtt->Texture);
         struct st_renderbuffer *dstRb =
            st_renderbuffer(drawFB->_ColorDrawBuffers[0]);
         struct pipe_surface *dstSurf = dstRb->surface;

         if (!srcObj->pt)
            goto done;

         util_blit_pixels(st->blit, srcObj->pt, srcAtt->TextureLevel,
                          srcX0, srcY0, srcX1, srcY1,
                          srcAtt->Zoffset + srcAtt->CubeMapFace,
                          dstSurf, dstX0, dstY0, dstX1, dstY1,
                          0.0, pFilter);
      }
      else {
         struct st_renderbuffer *srcRb =
            st_renderbuffer(readFB->_ColorReadBuffer);
         struct st_renderbuffer *dstRb =
            st_renderbuffer(drawFB->_ColorDrawBuffers[0]);
         struct pipe_surface *srcSurf = srcRb->surface;
         struct pipe_surface *dstSurf = dstRb->surface;

         util_blit_pixels(st->blit,
                          srcRb->texture, srcSurf->u.tex.level,
                          srcX0, srcY0, srcX1, srcY1,
                          srcSurf->u.tex.first_layer,
                          dstSurf, dstX0, dstY0, dstX1, dstY1,
                          0.0, pFilter);
      }
   }

   if (mask & depthStencil) {
      /* depth and/or stencil blit */

      /* get src/dst depth surfaces */
      struct gl_renderbuffer_attachment *srcDepth =
         &readFB->Attachment[BUFFER_DEPTH];
      struct gl_renderbuffer_attachment *dstDepth =
         &drawFB->Attachment[BUFFER_DEPTH];
      struct gl_renderbuffer_attachment *srcStencil =
         &readFB->Attachment[BUFFER_STENCIL];
      struct gl_renderbuffer_attachment *dstStencil =
         &drawFB->Attachment[BUFFER_STENCIL];

      struct st_renderbuffer *srcDepthRb =
         st_renderbuffer(readFB->Attachment[BUFFER_DEPTH].Renderbuffer);
      struct st_renderbuffer *dstDepthRb = 
         st_renderbuffer(drawFB->Attachment[BUFFER_DEPTH].Renderbuffer);
      struct pipe_surface *dstDepthSurf =
         dstDepthRb ? dstDepthRb->surface : NULL;

      if ((mask & depthStencil) == depthStencil &&
          st_is_depth_stencil_combined(srcDepth, srcStencil) &&
          st_is_depth_stencil_combined(dstDepth, dstStencil)) {

         /* Blitting depth and stencil values between combined
          * depth/stencil buffers.  This is the ideal case for such buffers.
          */
         util_blit_pixels(st->blit,
                          srcDepthRb->texture,
                          srcDepthRb->surface->u.tex.level,
                          srcX0, srcY0, srcX1, srcY1,
                          srcDepthRb->surface->u.tex.first_layer,
                          dstDepthSurf, dstX0, dstY0, dstX1, dstY1,
                          0.0, pFilter);
      }
      else {
         /* blitting depth and stencil separately */

         if (mask & GL_DEPTH_BUFFER_BIT) {
            util_blit_pixels(st->blit, srcDepthRb->texture,
                             srcDepthRb->surface->u.tex.level,
                             srcX0, srcY0, srcX1, srcY1,
                             srcDepthRb->surface->u.tex.first_layer,
                             dstDepthSurf, dstX0, dstY0, dstX1, dstY1,
                             0.0, pFilter);
         }

         if (mask & GL_STENCIL_BUFFER_BIT) {
            /* blit stencil only */
            _mesa_problem(ctx, "st_BlitFramebuffer(STENCIL) not completed");
         }
      }
   }

done:
   /* Restore conditional rendering state. */
   if (st->render_condition) {
      st->pipe->render_condition(st->pipe, st->render_condition,
                                 st->condition_mode);
   }
}


void
st_init_blit_functions(struct dd_function_table *functions)
{
   functions->BlitFramebuffer = st_BlitFramebuffer;
}

#endif /* FEATURE_EXT_framebuffer_blit */
