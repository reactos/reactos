/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"

#include "sp_context.h"
#include "sp_flush.h"
#include "sp_texture.h"
#include "sp_screen.h"

#include "state_tracker/sw_winsys.h"


/**
 * Conventional allocation path for non-display textures:
 * Use a simple, maximally packed layout.
 */
static boolean
softpipe_resource_layout(struct pipe_screen *screen,
                         struct softpipe_resource *spr)
{
   struct pipe_resource *pt = &spr->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   unsigned buffer_size = 0;

   for (level = 0; level <= pt->last_level; level++) {
      unsigned slices;

      if (pt->target == PIPE_TEXTURE_CUBE)
         slices = 6;
      else if (pt->target == PIPE_TEXTURE_3D)
         slices = depth;
      else
         slices = pt->array_size;

      spr->stride[level] = util_format_get_stride(pt->format, width);

      spr->level_offset[level] = buffer_size;

      buffer_size += (util_format_get_nblocksy(pt->format, height) *
                      slices * spr->stride[level]);

      width  = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }

   spr->data = align_malloc(buffer_size, 16);

   return spr->data != NULL;
}


/**
 * Texture layout for simple color buffers.
 */
static boolean
softpipe_displaytarget_layout(struct pipe_screen *screen,
                              struct softpipe_resource *spr)
{
   struct sw_winsys *winsys = softpipe_screen(screen)->winsys;

   /* Round up the surface size to a multiple of the tile size?
    */
   spr->dt = winsys->displaytarget_create(winsys,
                                          spr->base.bind,
                                          spr->base.format,
                                          spr->base.width0, 
                                          spr->base.height0,
                                          16,
                                          &spr->stride[0] );

   return spr->dt != NULL;
}


/**
 * Create new pipe_resource given the template information.
 */
static struct pipe_resource *
softpipe_resource_create(struct pipe_screen *screen,
                         const struct pipe_resource *templat)
{
   struct softpipe_resource *spr = CALLOC_STRUCT(softpipe_resource);
   if (!spr)
      return NULL;

   assert(templat->format != PIPE_FORMAT_NONE);

   spr->base = *templat;
   pipe_reference_init(&spr->base.reference, 1);
   spr->base.screen = screen;

   spr->pot = (util_is_power_of_two(templat->width0) &&
               util_is_power_of_two(templat->height0) &&
               util_is_power_of_two(templat->depth0));

   if (spr->base.bind & (PIPE_BIND_DISPLAY_TARGET |
			 PIPE_BIND_SCANOUT |
			 PIPE_BIND_SHARED)) {
      if (!softpipe_displaytarget_layout(screen, spr))
         goto fail;
   }
   else {
      if (!softpipe_resource_layout(screen, spr))
         goto fail;
   }
    
   return &spr->base;

 fail:
   FREE(spr);
   return NULL;
}


static void
softpipe_resource_destroy(struct pipe_screen *pscreen,
			  struct pipe_resource *pt)
{
   struct softpipe_screen *screen = softpipe_screen(pscreen);
   struct softpipe_resource *spr = softpipe_resource(pt);

   if (spr->dt) {
      /* display target */
      struct sw_winsys *winsys = screen->winsys;
      winsys->displaytarget_destroy(winsys, spr->dt);
   }
   else if (!spr->userBuffer) {
      /* regular texture */
      align_free(spr->data);
   }

   FREE(spr);
}


static struct pipe_resource *
softpipe_resource_from_handle(struct pipe_screen *screen,
                              const struct pipe_resource *templat,
                              struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = softpipe_screen(screen)->winsys;
   struct softpipe_resource *spr = CALLOC_STRUCT(softpipe_resource);
   if (!spr)
      return NULL;

   spr->base = *templat;
   pipe_reference_init(&spr->base.reference, 1);
   spr->base.screen = screen;

   spr->pot = (util_is_power_of_two(templat->width0) &&
               util_is_power_of_two(templat->height0) &&
               util_is_power_of_two(templat->depth0));

   spr->dt = winsys->displaytarget_from_handle(winsys,
                                               templat,
                                               whandle,
                                               &spr->stride[0]);
   if (!spr->dt)
      goto fail;

   return &spr->base;

 fail:
   FREE(spr);
   return NULL;
}


static boolean
softpipe_resource_get_handle(struct pipe_screen *screen,
                             struct pipe_resource *pt,
                             struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = softpipe_screen(screen)->winsys;
   struct softpipe_resource *spr = softpipe_resource(pt);

   assert(spr->dt);
   if (!spr->dt)
      return FALSE;

   return winsys->displaytarget_get_handle(winsys, spr->dt, whandle);
}


/**
 * Helper function to compute offset (in bytes) for a particular
 * texture level/face/slice from the start of the buffer.
 */
static unsigned
sp_get_tex_image_offset(const struct softpipe_resource *spr,
                        unsigned level, unsigned layer)
{
   const unsigned hgt = u_minify(spr->base.height0, level);
   const unsigned nblocksy = util_format_get_nblocksy(spr->base.format, hgt);
   unsigned offset = spr->level_offset[level];

   if (spr->base.target == PIPE_TEXTURE_CUBE ||
       spr->base.target == PIPE_TEXTURE_3D ||
       spr->base.target == PIPE_TEXTURE_2D_ARRAY) {
      offset += layer * nblocksy * spr->stride[level];
   }
   else if (spr->base.target == PIPE_TEXTURE_1D_ARRAY) {
      offset += layer * spr->stride[level];
   }
   else {
      assert(layer == 0);
   }

   return offset;
}


/**
 * Get a pipe_surface "view" into a texture resource.
 */
static struct pipe_surface *
softpipe_create_surface(struct pipe_context *pipe,
                        struct pipe_resource *pt,
                        const struct pipe_surface *surf_tmpl)
{
   struct pipe_surface *ps;
   unsigned level = surf_tmpl->u.tex.level;

   assert(level <= pt->last_level);
   assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_resource_reference(&ps->texture, pt);
      ps->context = pipe;
      ps->format = surf_tmpl->format;
      ps->width = u_minify(pt->width0, level);
      ps->height = u_minify(pt->height0, level);
      ps->usage = surf_tmpl->usage;

      ps->u.tex.level = level;
      ps->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
      ps->u.tex.last_layer = surf_tmpl->u.tex.last_layer;
   }
   return ps;
}


/**
 * Free a pipe_surface which was created with softpipe_create_surface().
 */
static void 
softpipe_surface_destroy(struct pipe_context *pipe,
                         struct pipe_surface *surf)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For softpipe, nothing to do.
    */
   assert(surf->texture);
   pipe_resource_reference(&surf->texture, NULL);
   FREE(surf);
}


/**
 * Geta pipe_transfer object which is used for moving data in/out of
 * a resource object.
 * \param pipe  rendering context
 * \param resource  the resource to transfer in/out of
 * \param level  which mipmap level
 * \param usage  bitmask of PIPE_TRANSFER_x flags
 * \param box  the 1D/2D/3D region of interest
 */
static struct pipe_transfer *
softpipe_get_transfer(struct pipe_context *pipe,
                      struct pipe_resource *resource,
                      unsigned level,
                      unsigned usage,
                      const struct pipe_box *box)
{
   struct softpipe_resource *spr = softpipe_resource(resource);
   struct softpipe_transfer *spt;

   assert(resource);
   assert(level <= resource->last_level);

   /* make sure the requested region is in the image bounds */
   assert(box->x + box->width <= u_minify(resource->width0, level));
   if (resource->target == PIPE_TEXTURE_1D_ARRAY) {
      assert(box->y + box->height <= resource->array_size);
   }
   else {
      assert(box->y + box->height <= u_minify(resource->height0, level));
      if (resource->target == PIPE_TEXTURE_2D_ARRAY) {
         assert(box->z + box->depth <= resource->array_size);
      }
      else if (resource->target == PIPE_TEXTURE_CUBE) {
         assert(box->z < 6);
      }
      else {
         assert(box->z + box->depth <= (u_minify(resource->depth0, level)));
      }
   }

   /*
    * Transfers, like other pipe operations, must happen in order, so flush the
    * context if necessary.
    */
   if (!(usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
      boolean read_only = !(usage & PIPE_TRANSFER_WRITE);
      boolean do_not_block = !!(usage & PIPE_TRANSFER_DONTBLOCK);
      if (!softpipe_flush_resource(pipe, resource,
                                   level, box->depth > 1 ? -1 : box->z,
                                   0, /* flush_flags */
                                   read_only,
                                   TRUE, /* cpu_access */
                                   do_not_block)) {
         /*
          * It would have blocked, but state tracker requested no to.
          */
         assert(do_not_block);
         return NULL;
      }
   }

   spt = CALLOC_STRUCT(softpipe_transfer);
   if (spt) {
      struct pipe_transfer *pt = &spt->base;
      enum pipe_format format = resource->format;
      const unsigned hgt = u_minify(spr->base.height0, level);
      const unsigned nblocksy = util_format_get_nblocksy(format, hgt);

      pipe_resource_reference(&pt->resource, resource);
      pt->level = level;
      pt->usage = usage;
      pt->box = *box;
      pt->stride = spr->stride[level];
      pt->layer_stride = pt->stride * nblocksy;

      spt->offset = sp_get_tex_image_offset(spr, level, box->z);
 
      spt->offset += 
         box->y / util_format_get_blockheight(format) * spt->base.stride +
         box->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);

      return pt;
   }
   return NULL;
}


/**
 * Free a pipe_transfer object which was created with
 * softpipe_get_transfer().
 */
static void 
softpipe_transfer_destroy(struct pipe_context *pipe,
                          struct pipe_transfer *transfer)
{
   pipe_resource_reference(&transfer->resource, NULL);
   FREE(transfer);
}


/**
 * Create memory mapping for given pipe_transfer object.
 */
static void *
softpipe_transfer_map(struct pipe_context *pipe,
                      struct pipe_transfer *transfer)
{
   struct softpipe_transfer *spt = softpipe_transfer(transfer);
   struct softpipe_resource *spr = softpipe_resource(transfer->resource);
   struct sw_winsys *winsys = softpipe_screen(pipe->screen)->winsys;
   uint8_t *map;
   
   /* resources backed by display target treated specially:
    */
   if (spr->dt) {
      map = winsys->displaytarget_map(winsys, spr->dt, transfer->usage);
   }
   else {
      map = spr->data;
   }

   if (map == NULL)
      return NULL;
   else
      return map + spt->offset;
}


/**
 * Unmap memory mapping for given pipe_transfer object.
 */
static void
softpipe_transfer_unmap(struct pipe_context *pipe,
                        struct pipe_transfer *transfer)
{
   struct softpipe_resource *spr;

   assert(transfer->resource);
   spr = softpipe_resource(transfer->resource);

   if (spr->dt) {
      /* display target */
      struct sw_winsys *winsys = softpipe_screen(pipe->screen)->winsys;
      winsys->displaytarget_unmap(winsys, spr->dt);
   }

   if (transfer->usage & PIPE_TRANSFER_WRITE) {
      /* Mark the texture as dirty to expire the tile caches. */
      spr->timestamp++;
   }
}

/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_resource *
softpipe_user_buffer_create(struct pipe_screen *screen,
                            void *ptr,
                            unsigned bytes,
			    unsigned bind_flags)
{
   struct softpipe_resource *spr;

   spr = CALLOC_STRUCT(softpipe_resource);
   if (!spr)
      return NULL;

   pipe_reference_init(&spr->base.reference, 1);
   spr->base.screen = screen;
   spr->base.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   spr->base.bind = bind_flags;
   spr->base.usage = PIPE_USAGE_IMMUTABLE;
   spr->base.flags = 0;
   spr->base.width0 = bytes;
   spr->base.height0 = 1;
   spr->base.depth0 = 1;
   spr->base.array_size = 1;
   spr->userBuffer = TRUE;
   spr->data = ptr;

   return &spr->base;
}


void
softpipe_init_texture_funcs(struct pipe_context *pipe)
{
   pipe->get_transfer = softpipe_get_transfer;
   pipe->transfer_destroy = softpipe_transfer_destroy;
   pipe->transfer_map = softpipe_transfer_map;
   pipe->transfer_unmap = softpipe_transfer_unmap;

   pipe->transfer_flush_region = u_default_transfer_flush_region;
   pipe->transfer_inline_write = u_default_transfer_inline_write;

   pipe->create_surface = softpipe_create_surface;
   pipe->surface_destroy = softpipe_surface_destroy;
}


void
softpipe_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->resource_create = softpipe_resource_create;
   screen->resource_destroy = softpipe_resource_destroy;
   screen->resource_from_handle = softpipe_resource_from_handle;
   screen->resource_get_handle = softpipe_resource_get_handle;
   screen->user_buffer_create = softpipe_user_buffer_create;

}
