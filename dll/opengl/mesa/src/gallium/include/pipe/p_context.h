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

#ifndef PIPE_CONTEXT_H
#define PIPE_CONTEXT_H

#include "p_compiler.h"
#include "p_format.h"
#include "p_video_enums.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pipe_blend_color;
struct pipe_blend_state;
struct pipe_box;
struct pipe_clip_state;
struct pipe_depth_stencil_alpha_state;
struct pipe_draw_info;
struct pipe_fence_handle;
struct pipe_framebuffer_state;
struct pipe_index_buffer;
struct pipe_query;
struct pipe_poly_stipple;
struct pipe_rasterizer_state;
struct pipe_resolve_info;
struct pipe_resource;
struct pipe_sampler_state;
struct pipe_sampler_view;
struct pipe_scissor_state;
struct pipe_shader_state;
struct pipe_stencil_ref;
struct pipe_stream_output_target;
struct pipe_surface;
struct pipe_vertex_buffer;
struct pipe_vertex_element;
struct pipe_video_buffer;
struct pipe_video_decoder;
struct pipe_viewport_state;
union pipe_color_union;

/**
 * Gallium rendering context.  Basically:
 *  - state setting functions
 *  - VBO drawing functions
 *  - surface functions
 */
struct pipe_context {
   struct pipe_winsys *winsys;
   struct pipe_screen *screen;

   void *priv;  /**< context private data (for DRI for example) */
   void *draw;  /**< private, for draw module (temporary?) */

   void (*destroy)( struct pipe_context * );

   /**
    * VBO drawing
    */
   /*@{*/
   void (*draw_vbo)( struct pipe_context *pipe,
                     const struct pipe_draw_info *info );
   /*@}*/

   /**
    * Predicate subsequent rendering on occlusion query result
    * \param query  the query predicate, or NULL if no predicate
    * \param mode  one of PIPE_RENDER_COND_x
    */
   void (*render_condition)( struct pipe_context *pipe,
                             struct pipe_query *query,
                             uint mode );

   /**
    * Query objects
    */
   /*@{*/
   struct pipe_query *(*create_query)( struct pipe_context *pipe,
                                       unsigned query_type );

   void (*destroy_query)(struct pipe_context *pipe,
                         struct pipe_query *q);

   void (*begin_query)(struct pipe_context *pipe, struct pipe_query *q);
   void (*end_query)(struct pipe_context *pipe, struct pipe_query *q);

   /**
    * Get results of a query.
    * \param wait  if true, this query will block until the result is ready
    * \return TRUE if results are ready, FALSE otherwise
    */
   boolean (*get_query_result)(struct pipe_context *pipe,
                               struct pipe_query *q,
                               boolean wait,
                               void *result);
   /*@}*/

   /**
    * State functions (create/bind/destroy state objects)
    */
   /*@{*/
   void * (*create_blend_state)(struct pipe_context *,
                                const struct pipe_blend_state *);
   void   (*bind_blend_state)(struct pipe_context *, void *);
   void   (*delete_blend_state)(struct pipe_context *, void  *);

   void * (*create_sampler_state)(struct pipe_context *,
                                  const struct pipe_sampler_state *);
   void   (*bind_fragment_sampler_states)(struct pipe_context *,
                                          unsigned num_samplers,
                                          void **samplers);
   void   (*bind_vertex_sampler_states)(struct pipe_context *,
                                        unsigned num_samplers,
                                        void **samplers);
   void   (*bind_geometry_sampler_states)(struct pipe_context *,
                                          unsigned num_samplers,
                                          void **samplers);
   void   (*delete_sampler_state)(struct pipe_context *, void *);

   void * (*create_rasterizer_state)(struct pipe_context *,
                                     const struct pipe_rasterizer_state *);
   void   (*bind_rasterizer_state)(struct pipe_context *, void *);
   void   (*delete_rasterizer_state)(struct pipe_context *, void *);

   void * (*create_depth_stencil_alpha_state)(struct pipe_context *,
                                        const struct pipe_depth_stencil_alpha_state *);
   void   (*bind_depth_stencil_alpha_state)(struct pipe_context *, void *);
   void   (*delete_depth_stencil_alpha_state)(struct pipe_context *, void *);

   void * (*create_fs_state)(struct pipe_context *,
                             const struct pipe_shader_state *);
   void   (*bind_fs_state)(struct pipe_context *, void *);
   void   (*delete_fs_state)(struct pipe_context *, void *);

   void * (*create_vs_state)(struct pipe_context *,
                             const struct pipe_shader_state *);
   void   (*bind_vs_state)(struct pipe_context *, void *);
   void   (*delete_vs_state)(struct pipe_context *, void *);

   void * (*create_gs_state)(struct pipe_context *,
                             const struct pipe_shader_state *);
   void   (*bind_gs_state)(struct pipe_context *, void *);
   void   (*delete_gs_state)(struct pipe_context *, void *);

   void * (*create_vertex_elements_state)(struct pipe_context *,
                                          unsigned num_elements,
                                          const struct pipe_vertex_element *);
   void   (*bind_vertex_elements_state)(struct pipe_context *, void *);
   void   (*delete_vertex_elements_state)(struct pipe_context *, void *);

   /*@}*/

   /**
    * Parameter-like state (or properties)
    */
   /*@{*/
   void (*set_blend_color)( struct pipe_context *,
                            const struct pipe_blend_color * );

   void (*set_stencil_ref)( struct pipe_context *,
                            const struct pipe_stencil_ref * );

   void (*set_sample_mask)( struct pipe_context *,
                            unsigned sample_mask );

   void (*set_clip_state)( struct pipe_context *,
                            const struct pipe_clip_state * );

   void (*set_constant_buffer)( struct pipe_context *,
                                uint shader, uint index,
                                struct pipe_resource *buf );

   void (*set_framebuffer_state)( struct pipe_context *,
                                  const struct pipe_framebuffer_state * );

   void (*set_polygon_stipple)( struct pipe_context *,
				const struct pipe_poly_stipple * );

   void (*set_scissor_state)( struct pipe_context *,
                              const struct pipe_scissor_state * );

   void (*set_viewport_state)( struct pipe_context *,
                               const struct pipe_viewport_state * );

   void (*set_fragment_sampler_views)(struct pipe_context *,
                                      unsigned num_views,
                                      struct pipe_sampler_view **);

   void (*set_vertex_sampler_views)(struct pipe_context *,
                                    unsigned num_views,
                                    struct pipe_sampler_view **);

   void (*set_geometry_sampler_views)(struct pipe_context *,
                                      unsigned num_views,
                                      struct pipe_sampler_view **);

   void (*set_vertex_buffers)( struct pipe_context *,
                               unsigned num_buffers,
                               const struct pipe_vertex_buffer * );

   void (*set_index_buffer)( struct pipe_context *pipe,
                             const struct pipe_index_buffer * );

   /*@}*/

   /**
    * Stream output functions.
    */
   /*@{*/

   struct pipe_stream_output_target *(*create_stream_output_target)(
                        struct pipe_context *,
                        struct pipe_resource *,
                        unsigned buffer_offset,
                        unsigned buffer_size);

   void (*stream_output_target_destroy)(struct pipe_context *,
                                        struct pipe_stream_output_target *);

   void (*set_stream_output_targets)(struct pipe_context *,
                              unsigned num_targets,
                              struct pipe_stream_output_target **targets,
                              unsigned append_bitmask);

   /*@}*/


   /**
    * Resource functions for blit-like functionality
    *
    * If a driver supports multisampling, resource_resolve must be available.
    */
   /*@{*/

   /**
    * Copy a block of pixels from one resource to another.
    * The resource must be of the same format.
    * Resources with nr_samples > 1 are not allowed.
    */
   void (*resource_copy_region)(struct pipe_context *pipe,
                                struct pipe_resource *dst,
                                unsigned dst_level,
                                unsigned dstx, unsigned dsty, unsigned dstz,
                                struct pipe_resource *src,
                                unsigned src_level,
                                const struct pipe_box *src_box);

   /**
    * Resolve a multisampled resource into a non-multisampled one.
    * Source and destination must be of the same format.
    */
   void (*resource_resolve)(struct pipe_context *pipe,
                            const struct pipe_resolve_info *info);

   /*@}*/

   /**
    * Clear the specified set of currently bound buffers to specified values.
    * The entire buffers are cleared (no scissor, no colormask, etc).
    *
    * \param buffers  bitfield of PIPE_CLEAR_* values.
    * \param color  pointer to a union of fiu array for each of r, g, b, a.
    * \param depth  depth clear value in [0,1].
    * \param stencil  stencil clear value
    */
   void (*clear)(struct pipe_context *pipe,
                 unsigned buffers,
                 const union pipe_color_union *color,
                 double depth,
                 unsigned stencil);

   /**
    * Clear a color rendertarget surface.
    * \param color  pointer to an union of fiu array for each of r, g, b, a.
    */
   void (*clear_render_target)(struct pipe_context *pipe,
                               struct pipe_surface *dst,
                               const union pipe_color_union *color,
                               unsigned dstx, unsigned dsty,
                               unsigned width, unsigned height);

   /**
    * Clear a depth-stencil surface.
    * \param clear_flags  bitfield of PIPE_CLEAR_DEPTH/STENCIL values.
    * \param depth  depth clear value in [0,1].
    * \param stencil  stencil clear value
    */
   void (*clear_depth_stencil)(struct pipe_context *pipe,
                               struct pipe_surface *dst,
                               unsigned clear_flags,
                               double depth,
                               unsigned stencil,
                               unsigned dstx, unsigned dsty,
                               unsigned width, unsigned height);

   /** Flush draw commands
    */
   void (*flush)( struct pipe_context *pipe,
                  struct pipe_fence_handle **fence );

   /**
    * Create a view on a texture to be used by a shader stage.
    */
   struct pipe_sampler_view * (*create_sampler_view)(struct pipe_context *ctx,
                                                     struct pipe_resource *texture,
                                                     const struct pipe_sampler_view *templat);

   void (*sampler_view_destroy)(struct pipe_context *ctx,
                                struct pipe_sampler_view *view);


   /**
    * Get a surface which is a "view" into a resource, used by
    * render target / depth stencil stages.
    * \param usage  bitmaks of PIPE_BIND_* flags
    */
   struct pipe_surface *(*create_surface)(struct pipe_context *ctx,
                                          struct pipe_resource *resource,
                                          const struct pipe_surface *templat);

   void (*surface_destroy)(struct pipe_context *ctx,
                           struct pipe_surface *);

   /**
    * Get a transfer object for transferring data to/from a texture.
    *
    * Transfers are (by default) context-private and allow uploads to be
    * interleaved with
    */
   struct pipe_transfer *(*get_transfer)(struct pipe_context *,
                                         struct pipe_resource *resource,
                                         unsigned level,
                                         unsigned usage,  /* a combination of PIPE_TRANSFER_x */
                                         const struct pipe_box *);

   void (*transfer_destroy)(struct pipe_context *,
                            struct pipe_transfer *);

   void *(*transfer_map)( struct pipe_context *,
                          struct pipe_transfer *transfer );

   /* If transfer was created with WRITE|FLUSH_EXPLICIT, only the
    * regions specified with this call are guaranteed to be written to
    * the resource.
    */
   void (*transfer_flush_region)( struct pipe_context *,
				  struct pipe_transfer *transfer,
				  const struct pipe_box *);

   void (*transfer_unmap)( struct pipe_context *,
                           struct pipe_transfer *transfer );


   /* One-shot transfer operation with data supplied in a user
    * pointer.  XXX: strides??
    */
   void (*transfer_inline_write)( struct pipe_context *,
                                  struct pipe_resource *,
                                  unsigned level,
                                  unsigned usage, /* a combination of PIPE_TRANSFER_x */
                                  const struct pipe_box *,
                                  const void *data,
                                  unsigned stride,
                                  unsigned layer_stride);


   /* Notify a driver that a content of a user buffer has been changed.
    * The changed range is [offset, offset+size-1].
    * The new width0 of the buffer is offset+size. */
   void (*redefine_user_buffer)(struct pipe_context *,
                                struct pipe_resource *,
                                unsigned offset,
                                unsigned size);

   /**
    * Flush any pending framebuffer writes and invalidate texture caches.
    */
   void (*texture_barrier)(struct pipe_context *);
   
   /**
    * Creates a video decoder for a specific video codec/profile
    */
   struct pipe_video_decoder *(*create_video_decoder)( struct pipe_context *context,
                                                       enum pipe_video_profile profile,
                                                       enum pipe_video_entrypoint entrypoint,
                                                       enum pipe_video_chroma_format chroma_format,
                                                       unsigned width, unsigned height, unsigned max_references,
                                                       bool expect_chunked_decode);

   /**
    * Creates a video buffer as decoding target
    */
   struct pipe_video_buffer *(*create_video_buffer)( struct pipe_context *context,
                                                     enum pipe_format buffer_format,
                                                     enum pipe_video_chroma_format chroma_format,
                                                     unsigned width, unsigned height );
};


#ifdef __cplusplus
}
#endif

#endif /* PIPE_CONTEXT_H */
