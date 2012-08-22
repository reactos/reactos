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

#ifndef U_INLINES_H
#define U_INLINES_H

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_screen.h"
#include "util/u_debug.h"
#include "util/u_debug_describe.h"
#include "util/u_debug_refcnt.h"
#include "util/u_atomic.h"
#include "util/u_box.h"
#include "util/u_math.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Reference counting helper functions.
 */


static INLINE void
pipe_reference_init(struct pipe_reference *reference, unsigned count)
{
   p_atomic_set(&reference->count, count);
}

static INLINE boolean
pipe_is_referenced(struct pipe_reference *reference)
{
   return p_atomic_read(&reference->count) != 0;
}

/**
 * Update reference counting.
 * The old thing pointed to, if any, will be unreferenced.
 * Both 'ptr' and 'reference' may be NULL.
 * \return TRUE if the object's refcount hits zero and should be destroyed.
 */
static INLINE boolean
pipe_reference_described(struct pipe_reference *ptr, 
                         struct pipe_reference *reference, 
                         debug_reference_descriptor get_desc)
{
   boolean destroy = FALSE;

   if(ptr != reference) {
      /* bump the reference.count first */
      if (reference) {
         assert(pipe_is_referenced(reference));
         p_atomic_inc(&reference->count);
         debug_reference(reference, get_desc, 1);
      }

      if (ptr) {
         assert(pipe_is_referenced(ptr));
         if (p_atomic_dec_zero(&ptr->count)) {
            destroy = TRUE;
         }
         debug_reference(ptr, get_desc, -1);
      }
   }

   return destroy;
}

static INLINE boolean
pipe_reference(struct pipe_reference *ptr, struct pipe_reference *reference)
{
   return pipe_reference_described(ptr, reference, 
                                   (debug_reference_descriptor)debug_describe_reference);
}

static INLINE void
pipe_surface_reference(struct pipe_surface **ptr, struct pipe_surface *surf)
{
   struct pipe_surface *old_surf = *ptr;

   if (pipe_reference_described(&(*ptr)->reference, &surf->reference, 
                                (debug_reference_descriptor)debug_describe_surface))
      old_surf->context->surface_destroy(old_surf->context, old_surf);
   *ptr = surf;
}

static INLINE void
pipe_resource_reference(struct pipe_resource **ptr, struct pipe_resource *tex)
{
   struct pipe_resource *old_tex = *ptr;

   if (pipe_reference_described(&(*ptr)->reference, &tex->reference, 
                                (debug_reference_descriptor)debug_describe_resource))
      old_tex->screen->resource_destroy(old_tex->screen, old_tex);
   *ptr = tex;
}

static INLINE void
pipe_sampler_view_reference(struct pipe_sampler_view **ptr, struct pipe_sampler_view *view)
{
   struct pipe_sampler_view *old_view = *ptr;

   if (pipe_reference_described(&(*ptr)->reference, &view->reference,
                                (debug_reference_descriptor)debug_describe_sampler_view))
      old_view->context->sampler_view_destroy(old_view->context, old_view);
   *ptr = view;
}

static INLINE void
pipe_so_target_reference(struct pipe_stream_output_target **ptr,
                         struct pipe_stream_output_target *target)
{
   struct pipe_stream_output_target *old = *ptr;

   if (pipe_reference_described(&(*ptr)->reference, &target->reference,
                     (debug_reference_descriptor)debug_describe_so_target))
      old->context->stream_output_target_destroy(old->context, old);
   *ptr = target;
}

static INLINE void
pipe_surface_reset(struct pipe_context *ctx, struct pipe_surface* ps,
                   struct pipe_resource *pt, unsigned level, unsigned layer,
                   unsigned flags)
{
   pipe_resource_reference(&ps->texture, pt);
   ps->format = pt->format;
   ps->width = u_minify(pt->width0, level);
   ps->height = u_minify(pt->height0, level);
   ps->usage = flags;
   ps->u.tex.level = level;
   ps->u.tex.first_layer = ps->u.tex.last_layer = layer;
   ps->context = ctx;
}

static INLINE void
pipe_surface_init(struct pipe_context *ctx, struct pipe_surface* ps,
                  struct pipe_resource *pt, unsigned level, unsigned layer,
                  unsigned flags)
{
   ps->texture = 0;
   pipe_reference_init(&ps->reference, 1);
   pipe_surface_reset(ctx, ps, pt, level, layer, flags);
}

/* Return true if the surfaces are equal. */
static INLINE boolean
pipe_surface_equal(struct pipe_surface *s1, struct pipe_surface *s2)
{
   return s1->texture == s2->texture &&
          s1->format == s2->format &&
          (s1->texture->target != PIPE_BUFFER ||
           (s1->u.buf.first_element == s2->u.buf.first_element &&
            s1->u.buf.last_element == s2->u.buf.last_element)) &&
          (s1->texture->target == PIPE_BUFFER ||
           (s1->u.tex.level == s2->u.tex.level &&
            s1->u.tex.first_layer == s2->u.tex.first_layer &&
            s1->u.tex.last_layer == s2->u.tex.last_layer));
}

/*
 * Convenience wrappers for screen buffer functions.
 */

static INLINE struct pipe_resource *
pipe_buffer_create( struct pipe_screen *screen,
		    unsigned bind,
		    unsigned usage,
		    unsigned size )
{
   struct pipe_resource buffer;
   memset(&buffer, 0, sizeof buffer);
   buffer.target = PIPE_BUFFER;
   buffer.format = PIPE_FORMAT_R8_UNORM; /* want TYPELESS or similar */
   buffer.bind = bind;
   buffer.usage = usage;
   buffer.flags = 0;
   buffer.width0 = size;
   buffer.height0 = 1;
   buffer.depth0 = 1;
   buffer.array_size = 1;
   return screen->resource_create(screen, &buffer);
}


static INLINE struct pipe_resource *
pipe_user_buffer_create( struct pipe_screen *screen, void *ptr, unsigned size,
			 unsigned usage )
{
   return screen->user_buffer_create(screen, ptr, size, usage);
}

static INLINE void *
pipe_buffer_map_range(struct pipe_context *pipe,
		      struct pipe_resource *buffer,
		      unsigned offset,
		      unsigned length,
		      unsigned usage,
		      struct pipe_transfer **transfer)
{
   struct pipe_box box;
   void *map;

   assert(offset < buffer->width0);
   assert(offset + length <= buffer->width0);
   assert(length);

   u_box_1d(offset, length, &box);

   *transfer = pipe->get_transfer( pipe,
                                   buffer,
                                   0,
                                   usage,
                                   &box);

   if (*transfer == NULL)
      return NULL;

   map = pipe->transfer_map( pipe, *transfer );
   if (map == NULL) {
      pipe->transfer_destroy( pipe, *transfer );
      *transfer = NULL;
      return NULL;
   }

   return map;
}


static INLINE void *
pipe_buffer_map(struct pipe_context *pipe,
                struct pipe_resource *buffer,
                unsigned usage,
                struct pipe_transfer **transfer)
{
   return pipe_buffer_map_range(pipe, buffer, 0, buffer->width0, usage, transfer);
}


static INLINE void
pipe_buffer_unmap(struct pipe_context *pipe,
                  struct pipe_transfer *transfer)
{
   if (transfer) {
      pipe->transfer_unmap(pipe, transfer);
      pipe->transfer_destroy(pipe, transfer);
   }
}

static INLINE void
pipe_buffer_flush_mapped_range(struct pipe_context *pipe,
                               struct pipe_transfer *transfer,
                               unsigned offset,
                               unsigned length)
{
   struct pipe_box box;
   int transfer_offset;

   assert(length);
   assert(transfer->box.x <= offset);
   assert(offset + length <= transfer->box.x + transfer->box.width);

   /* Match old screen->buffer_flush_mapped_range() behaviour, where
    * offset parameter is relative to the start of the buffer, not the
    * mapped range.
    */
   transfer_offset = offset - transfer->box.x;

   u_box_1d(transfer_offset, length, &box);

   pipe->transfer_flush_region(pipe, transfer, &box);
}

static INLINE void
pipe_buffer_write(struct pipe_context *pipe,
                  struct pipe_resource *buf,
                  unsigned offset,
                  unsigned size,
                  const void *data)
{
   struct pipe_box box;
   unsigned usage = PIPE_TRANSFER_WRITE;

   if (offset == 0 && size == buf->width0) {
      usage |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;
   } else {
      usage |= PIPE_TRANSFER_DISCARD_RANGE;
   }

   u_box_1d(offset, size, &box);

   pipe->transfer_inline_write( pipe,
                                buf,
                                0,
                                usage,
                                &box,
                                data,
                                size,
                                0);
}

/**
 * Special case for writing non-overlapping ranges.
 *
 * We can avoid GPU/CPU synchronization when writing range that has never
 * been written before.
 */
static INLINE void
pipe_buffer_write_nooverlap(struct pipe_context *pipe,
                            struct pipe_resource *buf,
                            unsigned offset, unsigned size,
                            const void *data)
{
   struct pipe_box box;

   u_box_1d(offset, size, &box);

   pipe->transfer_inline_write(pipe,
                               buf,
                               0,
                               (PIPE_TRANSFER_WRITE |
                                PIPE_TRANSFER_NOOVERWRITE),
                               &box,
                               data,
                               0, 0);
}

static INLINE void
pipe_buffer_read(struct pipe_context *pipe,
                 struct pipe_resource *buf,
                 unsigned offset,
                 unsigned size,
                 void *data)
{
   struct pipe_transfer *src_transfer;
   ubyte *map;

   map = (ubyte *) pipe_buffer_map_range(pipe,
					 buf,
					 offset, size,
					 PIPE_TRANSFER_READ,
					 &src_transfer);

   if (map)
      memcpy(data, map, size);

   pipe_buffer_unmap(pipe, src_transfer);
}

static INLINE struct pipe_transfer *
pipe_get_transfer( struct pipe_context *context,
                   struct pipe_resource *resource,
                   unsigned level, unsigned layer,
                   enum pipe_transfer_usage usage,
                   unsigned x, unsigned y,
                   unsigned w, unsigned h)
{
   struct pipe_box box;
   u_box_2d_zslice( x, y, layer, w, h, &box );
   return context->get_transfer( context,
                                 resource,
                                 level,
                                 usage,
                                 &box );
}

static INLINE void *
pipe_transfer_map( struct pipe_context *context,
                   struct pipe_transfer *transfer )
{
   return context->transfer_map( context, transfer );
}

static INLINE void
pipe_transfer_unmap( struct pipe_context *context,
                     struct pipe_transfer *transfer )
{
   context->transfer_unmap( context, transfer );
}


static INLINE void
pipe_transfer_destroy( struct pipe_context *context, 
                       struct pipe_transfer *transfer )
{
   context->transfer_destroy(context, transfer);
}


static INLINE boolean util_get_offset( 
   const struct pipe_rasterizer_state *templ,
   unsigned fill_mode)
{
   switch(fill_mode) {
   case PIPE_POLYGON_MODE_POINT:
      return templ->offset_point;
   case PIPE_POLYGON_MODE_LINE:
      return templ->offset_line;
   case PIPE_POLYGON_MODE_FILL:
      return templ->offset_tri;
   default:
      assert(0);
      return FALSE;
   }
}

/**
 * This function is used to copy an array of pipe_vertex_buffer structures,
 * while properly referencing the pipe_vertex_buffer::buffer member.
 *
 * \sa util_copy_framebuffer_state
 */
static INLINE void util_copy_vertex_buffers(struct pipe_vertex_buffer *dst,
                                            unsigned *dst_count,
                                            const struct pipe_vertex_buffer *src,
                                            unsigned src_count)
{
   unsigned i;

   /* Reference the buffers of 'src' in 'dst'. */
   for (i = 0; i < src_count; i++) {
      pipe_resource_reference(&dst[i].buffer, src[i].buffer);
   }
   /* Unreference the rest of the buffers in 'dst'. */
   for (; i < *dst_count; i++) {
      pipe_resource_reference(&dst[i].buffer, NULL);
   }

   /* Update the size of 'dst' and copy over the other members
    * of pipe_vertex_buffer. */
   *dst_count = src_count;
   memcpy(dst, src, src_count * sizeof(struct pipe_vertex_buffer));
}

#ifdef __cplusplus
}
#endif

#endif /* U_INLINES_H */
