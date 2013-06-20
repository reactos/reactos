/**************************************************************************
 *
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#ifndef CSO_CONTEXT_H
#define CSO_CONTEXT_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"


#ifdef	__cplusplus
extern "C" {
#endif

struct cso_context;

struct cso_context *cso_create_context( struct pipe_context *pipe );

void cso_release_all( struct cso_context *ctx );

void cso_destroy_context( struct cso_context *cso );



enum pipe_error cso_set_blend( struct cso_context *cso,
                               const struct pipe_blend_state *blend );
void cso_save_blend(struct cso_context *cso);
void cso_restore_blend(struct cso_context *cso);



enum pipe_error cso_set_depth_stencil_alpha( struct cso_context *cso,
                                             const struct pipe_depth_stencil_alpha_state *dsa );
void cso_save_depth_stencil_alpha(struct cso_context *cso);
void cso_restore_depth_stencil_alpha(struct cso_context *cso);



enum pipe_error cso_set_rasterizer( struct cso_context *cso,
                                    const struct pipe_rasterizer_state *rasterizer );
void cso_save_rasterizer(struct cso_context *cso);
void cso_restore_rasterizer(struct cso_context *cso);



enum pipe_error cso_set_samplers( struct cso_context *cso,
                                  unsigned count,
                                  const struct pipe_sampler_state **states );
void cso_save_samplers(struct cso_context *cso);
void cso_restore_samplers(struct cso_context *cso);

/* Alternate interface to support state trackers that like to modify
 * samplers one at a time:
 */
enum pipe_error cso_single_sampler( struct cso_context *cso,
                                    unsigned nr,
                                    const struct pipe_sampler_state *states );

void cso_single_sampler_done( struct cso_context *cso );

enum pipe_error cso_set_vertex_samplers(struct cso_context *cso,
                                        unsigned count,
                                        const struct pipe_sampler_state **states);

void
cso_save_vertex_samplers(struct cso_context *cso);

void
cso_restore_vertex_samplers(struct cso_context *cso);

enum pipe_error
cso_single_vertex_sampler(struct cso_context *cso,
                          unsigned nr,
                          const struct pipe_sampler_state *states);

void
cso_single_vertex_sampler_done(struct cso_context *cso);


enum pipe_error cso_set_vertex_elements(struct cso_context *ctx,
                                        unsigned count,
                                        const struct pipe_vertex_element *states);
void cso_save_vertex_elements(struct cso_context *ctx);
void cso_restore_vertex_elements(struct cso_context *ctx);


void cso_set_vertex_buffers(struct cso_context *ctx,
                            unsigned count,
                            const struct pipe_vertex_buffer *buffers);
void cso_save_vertex_buffers(struct cso_context *ctx);
void cso_restore_vertex_buffers(struct cso_context *ctx);


void cso_set_stream_outputs(struct cso_context *ctx,
                            unsigned num_targets,
                            struct pipe_stream_output_target **targets,
                            unsigned append_bitmask);
void cso_save_stream_outputs(struct cso_context *ctx);
void cso_restore_stream_outputs(struct cso_context *ctx);


/* These aren't really sensible -- most of the time the api provides
 * object semantics for shaders anyway, and the cases where it doesn't
 * (eg mesa's internall-generated texenv programs), it will be up to
 * the state tracker to implement their own specialized caching.
 */
enum pipe_error cso_set_fragment_shader_handle(struct cso_context *ctx,
                                               void *handle );
void cso_delete_fragment_shader(struct cso_context *ctx, void *handle );
/*
enum pipe_error cso_set_fragment_shader( struct cso_context *cso,
                                         const struct pipe_shader_state *shader );
*/
void cso_save_fragment_shader(struct cso_context *cso);
void cso_restore_fragment_shader(struct cso_context *cso);


enum pipe_error cso_set_vertex_shader_handle(struct cso_context *ctx,
                                             void *handle );
void cso_delete_vertex_shader(struct cso_context *ctx, void *handle );
/*
enum pipe_error cso_set_vertex_shader( struct cso_context *cso,
                                       const struct pipe_shader_state *shader );
*/
void cso_save_vertex_shader(struct cso_context *cso);
void cso_restore_vertex_shader(struct cso_context *cso);


enum pipe_error cso_set_geometry_shader_handle(struct cso_context *ctx,
                                               void *handle);
void cso_delete_geometry_shader(struct cso_context *ctx, void *handle);
void cso_save_geometry_shader(struct cso_context *cso);
void cso_restore_geometry_shader(struct cso_context *cso);


enum pipe_error cso_set_framebuffer(struct cso_context *cso,
                                    const struct pipe_framebuffer_state *fb);
void cso_save_framebuffer(struct cso_context *cso);
void cso_restore_framebuffer(struct cso_context *cso);


enum pipe_error cso_set_viewport(struct cso_context *cso,
                                 const struct pipe_viewport_state *vp);
void cso_save_viewport(struct cso_context *cso);
void cso_restore_viewport(struct cso_context *cso);


enum pipe_error cso_set_blend_color(struct cso_context *cso,
                                    const struct pipe_blend_color *bc);

enum pipe_error cso_set_sample_mask(struct cso_context *cso,
                                    unsigned stencil_mask);

enum pipe_error cso_set_stencil_ref(struct cso_context *cso,
                                    const struct pipe_stencil_ref *sr);
void cso_save_stencil_ref(struct cso_context *cso);
void cso_restore_stencil_ref(struct cso_context *cso);


/* clip state */

void
cso_set_clip(struct cso_context *cso,
             const struct pipe_clip_state *clip);

void
cso_save_clip(struct cso_context *cso);

void
cso_restore_clip(struct cso_context *cso);


/* fragment sampler view state */

void
cso_set_fragment_sampler_views(struct cso_context *cso,
                               uint count,
                               struct pipe_sampler_view **views);

void
cso_save_fragment_sampler_views(struct cso_context *cso);

void
cso_restore_fragment_sampler_views(struct cso_context *cso);


/* vertex sampler view state */

void
cso_set_vertex_sampler_views(struct cso_context *cso,
                             uint count,
                             struct pipe_sampler_view **views);

void
cso_save_vertex_sampler_views(struct cso_context *cso);

void
cso_restore_vertex_sampler_views(struct cso_context *cso);


#ifdef	__cplusplus
}
#endif

#endif
