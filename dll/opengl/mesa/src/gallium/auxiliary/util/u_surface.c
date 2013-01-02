/**************************************************************************
 *
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Surface utility functions.
 *  
 * @author Brian Paul
 */


#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_rect.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"


/**
 * Initialize a pipe_surface object.  'view' is considered to have
 * uninitialized contents.
 */
void
u_surface_default_template(struct pipe_surface *surf,
                           const struct pipe_resource *texture,
                           unsigned bind)
{
   memset(surf, 0, sizeof(*surf));

   surf->format = texture->format;
   /* XXX should filter out all non-rt/ds bind flags ? */
   surf->usage = bind;
}

/**
 * Helper to quickly create an RGBA rendering surface of a certain size.
 * \param textureOut  returns the new texture
 * \param surfaceOut  returns the new surface
 * \return TRUE for success, FALSE if failure
 */
boolean
util_create_rgba_surface(struct pipe_context *pipe,
                         uint width, uint height,
                         uint bind,
                         struct pipe_resource **textureOut,
                         struct pipe_surface **surfaceOut)
{
   static const enum pipe_format rgbaFormats[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_A8B8G8R8_UNORM,
      PIPE_FORMAT_NONE
   };
   const uint target = PIPE_TEXTURE_2D;
   enum pipe_format format = PIPE_FORMAT_NONE;
   struct pipe_resource templ;
   struct pipe_surface surf_templ;
   struct pipe_screen *screen = pipe->screen;
   uint i;

   /* Choose surface format */
   for (i = 0; rgbaFormats[i]; i++) {
      if (screen->is_format_supported(screen, rgbaFormats[i],
                                      target, 0, bind)) {
         format = rgbaFormats[i];
         break;
      }
   }
   if (format == PIPE_FORMAT_NONE)
      return FALSE;  /* unable to get an rgba format!?! */

   /* create texture */
   memset(&templ, 0, sizeof(templ));
   templ.target = target;
   templ.format = format;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind = bind;

   *textureOut = screen->resource_create(screen, &templ);
   if (!*textureOut)
      return FALSE;

   /* create surface */
   u_surface_default_template(&surf_templ, *textureOut, bind);
   /* create surface / view into texture */
   *surfaceOut = pipe->create_surface(pipe,
                                      *textureOut,
                                      &surf_templ);
   if (!*surfaceOut) {
      pipe_resource_reference(textureOut, NULL);
      return FALSE;
   }

   return TRUE;
}


/**
 * Release the surface and texture from util_create_rgba_surface().
 */
void
util_destroy_rgba_surface(struct pipe_resource *texture,
                          struct pipe_surface *surface)
{
   pipe_surface_reference(&surface, NULL);
   pipe_resource_reference(&texture, NULL);
}



/**
 * Fallback function for pipe->resource_copy_region().
 * Note: (X,Y)=(0,0) is always the upper-left corner.
 */
void
util_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst,
                          unsigned dst_level,
                          unsigned dst_x, unsigned dst_y, unsigned dst_z,
                          struct pipe_resource *src,
                          unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct pipe_transfer *src_trans, *dst_trans;
   void *dst_map;
   const void *src_map;
   enum pipe_format src_format, dst_format;
   unsigned w = src_box->width;
   unsigned h = src_box->height;

   assert(src && dst);
   assert((src->target == PIPE_BUFFER && dst->target == PIPE_BUFFER) ||
          (src->target != PIPE_BUFFER && dst->target != PIPE_BUFFER));

   if (!src || !dst)
      return;

   src_format = src->format;
   dst_format = dst->format;

   src_trans = pipe_get_transfer(pipe,
                                 src,
                                 src_level,
                                 src_box->z,
                                 PIPE_TRANSFER_READ,
                                 src_box->x, src_box->y, w, h);

   dst_trans = pipe_get_transfer(pipe,
                                 dst,
                                 dst_level,
                                 dst_z,
                                 PIPE_TRANSFER_WRITE,
                                 dst_x, dst_y, w, h);

   assert(util_format_get_blocksize(dst_format) == util_format_get_blocksize(src_format));
   assert(util_format_get_blockwidth(dst_format) == util_format_get_blockwidth(src_format));
   assert(util_format_get_blockheight(dst_format) == util_format_get_blockheight(src_format));

   src_map = pipe->transfer_map(pipe, src_trans);
   dst_map = pipe->transfer_map(pipe, dst_trans);

   assert(src_map);
   assert(dst_map);

   if (src_map && dst_map) {
      if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
         memcpy(dst_map, src_map, w);
      } else {
         util_copy_rect(dst_map,
                        dst_format,
                        dst_trans->stride,
                        0, 0,
                        w, h,
                        src_map,
                        src_trans->stride,
                        0,
                        0);
      }
   }

   pipe->transfer_unmap(pipe, src_trans);
   pipe->transfer_unmap(pipe, dst_trans);

   pipe->transfer_destroy(pipe, src_trans);
   pipe->transfer_destroy(pipe, dst_trans);
}



#define UBYTE_TO_USHORT(B) ((B) | ((B) << 8))


/**
 * Fallback for pipe->clear_render_target() function.
 * XXX this looks too hackish to be really useful.
 * cpp > 4 looks like a gross hack at best...
 * Plus can't use these transfer fallbacks when clearing
 * multisampled surfaces for instance.
 */
void
util_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         const union pipe_color_union *color,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct pipe_transfer *dst_trans;
   void *dst_map;
   union util_color uc;

   assert(dst->texture);
   if (!dst->texture)
      return;
   /* XXX: should handle multiple layers */
   dst_trans = pipe_get_transfer(pipe,
                                 dst->texture,
                                 dst->u.tex.level,
                                 dst->u.tex.first_layer,
                                 PIPE_TRANSFER_WRITE,
                                 dstx, dsty, width, height);

   dst_map = pipe->transfer_map(pipe, dst_trans);

   assert(dst_map);

   if (dst_map) {
      assert(dst_trans->stride > 0);

      util_pack_color(color->f, dst->texture->format, &uc);
      util_fill_rect(dst_map, dst->texture->format,
                     dst_trans->stride,
                     0, 0, width, height, &uc);
   }

   pipe->transfer_unmap(pipe, dst_trans);
   pipe->transfer_destroy(pipe, dst_trans);
}

/**
 * Fallback for pipe->clear_stencil() function.
 * sw fallback doesn't look terribly useful here.
 * Plus can't use these transfer fallbacks when clearing
 * multisampled surfaces for instance.
 */
void
util_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct pipe_transfer *dst_trans;
   ubyte *dst_map;
   boolean need_rmw = FALSE;

   if ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) &&
       ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
       util_format_is_depth_and_stencil(dst->format))
      need_rmw = TRUE;

   assert(dst->texture);
   if (!dst->texture)
      return;
   dst_trans = pipe_get_transfer(pipe,
                                 dst->texture,
                                 dst->u.tex.level,
                                 dst->u.tex.first_layer,
                                 (need_rmw ? PIPE_TRANSFER_READ_WRITE :
                                     PIPE_TRANSFER_WRITE),
                                 dstx, dsty, width, height);

   dst_map = pipe->transfer_map(pipe, dst_trans);

   assert(dst_map);

   if (dst_map) {
      unsigned dst_stride = dst_trans->stride;
      unsigned zstencil = util_pack_z_stencil(dst->texture->format, depth, stencil);
      unsigned i, j;
      assert(dst_trans->stride > 0);

      switch (util_format_get_blocksize(dst->format)) {
      case 1:
         assert(dst->format == PIPE_FORMAT_S8_UINT);
         if(dst_stride == width)
            memset(dst_map, (ubyte) zstencil, height * width);
         else {
            for (i = 0; i < height; i++) {
               memset(dst_map, (ubyte) zstencil, width);
               dst_map += dst_stride;
            }
         }
         break;
      case 2:
         assert(dst->format == PIPE_FORMAT_Z16_UNORM);
         for (i = 0; i < height; i++) {
            uint16_t *row = (uint16_t *)dst_map;
            for (j = 0; j < width; j++)
               *row++ = (uint16_t) zstencil;
            dst_map += dst_stride;
            }
         break;
      case 4:
         if (!need_rmw) {
            for (i = 0; i < height; i++) {
               uint32_t *row = (uint32_t *)dst_map;
               for (j = 0; j < width; j++)
                  *row++ = zstencil;
               dst_map += dst_stride;
            }
         }
         else {
            uint32_t dst_mask;
            if (dst->format == PIPE_FORMAT_Z24_UNORM_S8_UINT)
               dst_mask = 0xffffff00;
            else {
               assert(dst->format == PIPE_FORMAT_S8_UINT_Z24_UNORM);
               dst_mask = 0xffffff;
            }
            if (clear_flags & PIPE_CLEAR_DEPTH)
               dst_mask = ~dst_mask;
            for (i = 0; i < height; i++) {
               uint32_t *row = (uint32_t *)dst_map;
               for (j = 0; j < width; j++) {
                  uint32_t tmp = *row & dst_mask;
                  *row++ = tmp | (zstencil & ~dst_mask);
               }
               dst_map += dst_stride;
            }
         }
         break;
      case 8:
      {
         uint64_t zstencil = util_pack64_z_stencil(dst->texture->format,
                                                   depth, stencil);

         assert(dst->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT);

         if (!need_rmw) {
            for (i = 0; i < height; i++) {
               uint64_t *row = (uint64_t *)dst_map;
               for (j = 0; j < width; j++)
                  *row++ = zstencil;
               dst_map += dst_stride;
            }
         }
         else {
            uint64_t src_mask;

            if (clear_flags & PIPE_CLEAR_DEPTH)
               src_mask = 0x00000000ffffffffull;
            else
               src_mask = 0x000000ff00000000ull;

            for (i = 0; i < height; i++) {
               uint64_t *row = (uint64_t *)dst_map;
               for (j = 0; j < width; j++) {
                  uint64_t tmp = *row & ~src_mask;
                  *row++ = tmp | (zstencil & src_mask);
               }
               dst_map += dst_stride;
            }
         }
         break;
      }
      default:
         assert(0);
         break;
      }
   }

   pipe->transfer_unmap(pipe, dst_trans);
   pipe->transfer_destroy(pipe, dst_trans);
}
