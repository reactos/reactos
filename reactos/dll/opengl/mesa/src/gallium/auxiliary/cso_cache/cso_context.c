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

 /**
  * @file
  * 
  * Wrap the cso cache & hash mechanisms in a simplified
  * pipe-driver-specific interface.
  *
  * @author Zack Rusin <zack@tungstengraphics.com>
  * @author Keith Whitwell <keith@tungstengraphics.com>
  */

#include "pipe/p_state.h"
#include "util/u_framebuffer.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"

#include "cso_cache/cso_context.h"
#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_hash.h"
#include "cso_context.h"


/**
 * Info related to samplers and sampler views.
 * We have one of these for fragment samplers and another for vertex samplers.
 */
struct sampler_info
{
   struct {
      void *samplers[PIPE_MAX_SAMPLERS];
      unsigned nr_samplers;
   } hw;

   void *samplers[PIPE_MAX_SAMPLERS];
   unsigned nr_samplers;

   void *samplers_saved[PIPE_MAX_SAMPLERS];
   unsigned nr_samplers_saved;

   struct pipe_sampler_view *views[PIPE_MAX_SAMPLERS];
   unsigned nr_views;

   struct pipe_sampler_view *views_saved[PIPE_MAX_SAMPLERS];
   unsigned nr_views_saved;
};



struct cso_context {
   struct pipe_context *pipe;
   struct cso_cache *cache;

   boolean has_geometry_shader;
   boolean has_streamout;

   struct sampler_info fragment_samplers;
   struct sampler_info vertex_samplers;

   uint nr_vertex_buffers;
   struct pipe_vertex_buffer vertex_buffers[PIPE_MAX_ATTRIBS];

   uint nr_vertex_buffers_saved;
   struct pipe_vertex_buffer vertex_buffers_saved[PIPE_MAX_ATTRIBS];

   unsigned nr_so_targets;
   struct pipe_stream_output_target *so_targets[PIPE_MAX_SO_BUFFERS];

   unsigned nr_so_targets_saved;
   struct pipe_stream_output_target *so_targets_saved[PIPE_MAX_SO_BUFFERS];

   /** Current and saved state.
    * The saved state is used as a 1-deep stack.
    */
   void *blend, *blend_saved;
   void *depth_stencil, *depth_stencil_saved;
   void *rasterizer, *rasterizer_saved;
   void *fragment_shader, *fragment_shader_saved, *geometry_shader;
   void *vertex_shader, *vertex_shader_saved, *geometry_shader_saved;
   void *velements, *velements_saved;

   struct pipe_clip_state clip;
   struct pipe_clip_state clip_saved;

   struct pipe_framebuffer_state fb, fb_saved;
   struct pipe_viewport_state vp, vp_saved;
   struct pipe_blend_color blend_color;
   unsigned sample_mask;
   struct pipe_stencil_ref stencil_ref, stencil_ref_saved;
};


static boolean delete_blend_state(struct cso_context *ctx, void *state)
{
   struct cso_blend *cso = (struct cso_blend *)state;

   if (ctx->blend == cso->data)
      return FALSE;

   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_depth_stencil_state(struct cso_context *ctx, void *state)
{
   struct cso_depth_stencil_alpha *cso = (struct cso_depth_stencil_alpha *)state;

   if (ctx->depth_stencil == cso->data)
      return FALSE;

   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);

   return TRUE;
}

static boolean delete_sampler_state(struct cso_context *ctx, void *state)
{
   struct cso_sampler *cso = (struct cso_sampler *)state;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_rasterizer_state(struct cso_context *ctx, void *state)
{
   struct cso_rasterizer *cso = (struct cso_rasterizer *)state;

   if (ctx->rasterizer == cso->data)
      return FALSE;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_fs_state(struct cso_context *ctx, void *state)
{
   struct cso_fragment_shader *cso = (struct cso_fragment_shader *)state;
   if (ctx->fragment_shader == cso->data)
      return FALSE;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}

static boolean delete_vs_state(struct cso_context *ctx, void *state)
{
   struct cso_vertex_shader *cso = (struct cso_vertex_shader *)state;
   if (ctx->vertex_shader == cso->data)
      return TRUE;
   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return FALSE;
}

static boolean delete_vertex_elements(struct cso_context *ctx,
                                      void *state)
{
   struct cso_velements *cso = (struct cso_velements *)state;

   if (ctx->velements == cso->data)
      return FALSE;

   if (cso->delete_state)
      cso->delete_state(cso->context, cso->data);
   FREE(state);
   return TRUE;
}


static INLINE boolean delete_cso(struct cso_context *ctx,
                                 void *state, enum cso_cache_type type)
{
   switch (type) {
   case CSO_BLEND:
      return delete_blend_state(ctx, state);
      break;
   case CSO_SAMPLER:
      return delete_sampler_state(ctx, state);
      break;
   case CSO_DEPTH_STENCIL_ALPHA:
      return delete_depth_stencil_state(ctx, state);
      break;
   case CSO_RASTERIZER:
      return delete_rasterizer_state(ctx, state);
      break;
   case CSO_FRAGMENT_SHADER:
      return delete_fs_state(ctx, state);
      break;
   case CSO_VERTEX_SHADER:
      return delete_vs_state(ctx, state);
      break;
   case CSO_VELEMENTS:
      return delete_vertex_elements(ctx, state);
      break;
   default:
      assert(0);
      FREE(state);
   }
   return FALSE;
}

static INLINE void sanitize_hash(struct cso_hash *hash, enum cso_cache_type type,
                                 int max_size, void *user_data)
{
   struct cso_context *ctx = (struct cso_context *)user_data;
   /* if we're approach the maximum size, remove fourth of the entries
    * otherwise every subsequent call will go through the same */
   int hash_size = cso_hash_size(hash);
   int max_entries = (max_size > hash_size) ? max_size : hash_size;
   int to_remove =  (max_size < max_entries) * max_entries/4;
   struct cso_hash_iter iter = cso_hash_first_node(hash);
   if (hash_size > max_size)
      to_remove += hash_size - max_size;
   while (to_remove) {
      /*remove elements until we're good */
      /*fixme: currently we pick the nodes to remove at random*/
      void *cso = cso_hash_iter_data(iter);
      if (delete_cso(ctx, cso, type)) {
         iter = cso_hash_erase(hash, iter);
         --to_remove;
      } else
         iter = cso_hash_iter_next(iter);
   }
}


struct cso_context *cso_create_context( struct pipe_context *pipe )
{
   struct cso_context *ctx = CALLOC_STRUCT(cso_context);
   if (ctx == NULL)
      goto out;

   assert(PIPE_MAX_SAMPLERS == PIPE_MAX_VERTEX_SAMPLERS);

   ctx->cache = cso_cache_create();
   if (ctx->cache == NULL)
      goto out;
   cso_cache_set_sanitize_callback(ctx->cache,
                                   sanitize_hash,
                                   ctx);

   ctx->pipe = pipe;

   /* Enable for testing: */
   if (0) cso_set_maximum_cache_size( ctx->cache, 4 );

   if (pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_GEOMETRY,
                                PIPE_SHADER_CAP_MAX_INSTRUCTIONS) > 0) {
      ctx->has_geometry_shader = TRUE;
   }
   if (pipe->screen->get_param(pipe->screen,
                               PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS) != 0) {
      ctx->has_streamout = TRUE;
   }

   return ctx;

out:
   cso_destroy_context( ctx );      
   return NULL;
}


/**
 * Prior to context destruction, this function unbinds all state objects.
 */
void cso_release_all( struct cso_context *ctx )
{
   unsigned i;
   struct sampler_info *info;

   if (ctx->pipe) {
      ctx->pipe->bind_blend_state( ctx->pipe, NULL );
      ctx->pipe->bind_rasterizer_state( ctx->pipe, NULL );
      ctx->pipe->bind_fragment_sampler_states( ctx->pipe, 0, NULL );
      if (ctx->pipe->bind_vertex_sampler_states)
         ctx->pipe->bind_vertex_sampler_states(ctx->pipe, 0, NULL);
      ctx->pipe->bind_depth_stencil_alpha_state( ctx->pipe, NULL );
      ctx->pipe->bind_fs_state( ctx->pipe, NULL );
      ctx->pipe->bind_vs_state( ctx->pipe, NULL );
      ctx->pipe->bind_vertex_elements_state( ctx->pipe, NULL );
      ctx->pipe->set_fragment_sampler_views(ctx->pipe, 0, NULL);
      if (ctx->pipe->set_vertex_sampler_views)
         ctx->pipe->set_vertex_sampler_views(ctx->pipe, 0, NULL);
      if (ctx->pipe->set_stream_output_targets)
         ctx->pipe->set_stream_output_targets(ctx->pipe, 0, NULL, 0);
   }

   /* free fragment samplers, views */
   info = &ctx->fragment_samplers;   
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_sampler_view_reference(&info->views[i], NULL);
      pipe_sampler_view_reference(&info->views_saved[i], NULL);
   }

   /* free vertex samplers, views */
   info = &ctx->vertex_samplers;   
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_sampler_view_reference(&info->views[i], NULL);
      pipe_sampler_view_reference(&info->views_saved[i], NULL);
   }

   util_unreference_framebuffer_state(&ctx->fb);
   util_unreference_framebuffer_state(&ctx->fb_saved);

   util_copy_vertex_buffers(ctx->vertex_buffers,
                            &ctx->nr_vertex_buffers,
                            NULL, 0);
   util_copy_vertex_buffers(ctx->vertex_buffers_saved,
                            &ctx->nr_vertex_buffers_saved,
                            NULL, 0);

   for (i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
      pipe_so_target_reference(&ctx->so_targets[i], NULL);
      pipe_so_target_reference(&ctx->so_targets_saved[i], NULL);
   }

   if (ctx->cache) {
      cso_cache_delete( ctx->cache );
      ctx->cache = NULL;
   }
}


/**
 * Free the CSO context.  NOTE: the state tracker should have previously called
 * cso_release_all().
 */
void cso_destroy_context( struct cso_context *ctx )
{
   if (ctx) {
      FREE( ctx );
   }
}


/* Those function will either find the state of the given template
 * in the cache or they will create a new state from the given
 * template, insert it in the cache and return it.
 */

/*
 * If the driver returns 0 from the create method then they will assign
 * the data member of the cso to be the template itself.
 */

enum pipe_error cso_set_blend(struct cso_context *ctx,
                              const struct pipe_blend_state *templ)
{
   unsigned key_size, hash_key;
   struct cso_hash_iter iter;
   void *handle;

   key_size = templ->independent_blend_enable ? sizeof(struct pipe_blend_state) :
              (char *)&(templ->rt[1]) - (char *)templ;
   hash_key = cso_construct_key((void*)templ, key_size);
   iter = cso_find_state_template(ctx->cache, hash_key, CSO_BLEND, (void*)templ, key_size);

   if (cso_hash_iter_is_null(iter)) {
      struct cso_blend *cso = MALLOC(sizeof(struct cso_blend));
      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memset(&cso->state, 0, sizeof cso->state);
      memcpy(&cso->state, templ, key_size);
      cso->data = ctx->pipe->create_blend_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_blend_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_BLEND, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_blend *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->blend != handle) {
      ctx->blend = handle;
      ctx->pipe->bind_blend_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_save_blend(struct cso_context *ctx)
{
   assert(!ctx->blend_saved);
   ctx->blend_saved = ctx->blend;
}

void cso_restore_blend(struct cso_context *ctx)
{
   if (ctx->blend != ctx->blend_saved) {
      ctx->blend = ctx->blend_saved;
      ctx->pipe->bind_blend_state(ctx->pipe, ctx->blend_saved);
   }
   ctx->blend_saved = NULL;
}



enum pipe_error cso_set_depth_stencil_alpha(struct cso_context *ctx,
                                            const struct pipe_depth_stencil_alpha_state *templ)
{
   unsigned key_size = sizeof(struct pipe_depth_stencil_alpha_state);
   unsigned hash_key = cso_construct_key((void*)templ, key_size);
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, 
                                                       CSO_DEPTH_STENCIL_ALPHA,
                                                       (void*)templ, key_size);
   void *handle;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_depth_stencil_alpha *cso = MALLOC(sizeof(struct cso_depth_stencil_alpha));
      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(&cso->state, templ, sizeof(*templ));
      cso->data = ctx->pipe->create_depth_stencil_alpha_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_depth_stencil_alpha_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_DEPTH_STENCIL_ALPHA, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_depth_stencil_alpha *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->depth_stencil != handle) {
      ctx->depth_stencil = handle;
      ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_save_depth_stencil_alpha(struct cso_context *ctx)
{
   assert(!ctx->depth_stencil_saved);
   ctx->depth_stencil_saved = ctx->depth_stencil;
}

void cso_restore_depth_stencil_alpha(struct cso_context *ctx)
{
   if (ctx->depth_stencil != ctx->depth_stencil_saved) {
      ctx->depth_stencil = ctx->depth_stencil_saved;
      ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, ctx->depth_stencil_saved);
   }
   ctx->depth_stencil_saved = NULL;
}



enum pipe_error cso_set_rasterizer(struct cso_context *ctx,
                                   const struct pipe_rasterizer_state *templ)
{
   unsigned key_size = sizeof(struct pipe_rasterizer_state);
   unsigned hash_key = cso_construct_key((void*)templ, key_size);
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_RASTERIZER,
                                                       (void*)templ, key_size);
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_rasterizer *cso = MALLOC(sizeof(struct cso_rasterizer));
      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(&cso->state, templ, sizeof(*templ));
      cso->data = ctx->pipe->create_rasterizer_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_rasterizer_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_RASTERIZER, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_rasterizer *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->rasterizer != handle) {
      ctx->rasterizer = handle;
      ctx->pipe->bind_rasterizer_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_save_rasterizer(struct cso_context *ctx)
{
   assert(!ctx->rasterizer_saved);
   ctx->rasterizer_saved = ctx->rasterizer;
}

void cso_restore_rasterizer(struct cso_context *ctx)
{
   if (ctx->rasterizer != ctx->rasterizer_saved) {
      ctx->rasterizer = ctx->rasterizer_saved;
      ctx->pipe->bind_rasterizer_state(ctx->pipe, ctx->rasterizer_saved);
   }
   ctx->rasterizer_saved = NULL;
}



enum pipe_error cso_set_fragment_shader_handle(struct cso_context *ctx,
                                               void *handle )
{
   if (ctx->fragment_shader != handle) {
      ctx->fragment_shader = handle;
      ctx->pipe->bind_fs_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_delete_fragment_shader(struct cso_context *ctx, void *handle )
{
   if (handle == ctx->fragment_shader) {
      /* unbind before deleting */
      ctx->pipe->bind_fs_state(ctx->pipe, NULL);
      ctx->fragment_shader = NULL;
   }
   ctx->pipe->delete_fs_state(ctx->pipe, handle);
}

/* Not really working:
 */
#if 0
enum pipe_error cso_set_fragment_shader(struct cso_context *ctx,
                                        const struct pipe_shader_state *templ)
{
   const struct tgsi_token *tokens = templ->tokens;
   unsigned num_tokens = tgsi_num_tokens(tokens);
   size_t tokens_size = num_tokens*sizeof(struct tgsi_token);
   unsigned hash_key = cso_construct_key((void*)tokens, tokens_size);
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, 
                                                       CSO_FRAGMENT_SHADER,
                                                       (void*)tokens,
                                                       sizeof(*templ)); /* XXX correct? tokens_size? */
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_fragment_shader *cso = MALLOC(sizeof(struct cso_fragment_shader) + tokens_size);
      struct tgsi_token *cso_tokens = (struct tgsi_token *)((char *)cso + sizeof(*cso));

      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(cso_tokens, tokens, tokens_size);
      cso->state.tokens = cso_tokens;
      cso->data = ctx->pipe->create_fs_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_fs_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_FRAGMENT_SHADER, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_fragment_shader *)cso_hash_iter_data(iter))->data;
   }

   return cso_set_fragment_shader_handle( ctx, handle );
}
#endif

void cso_save_fragment_shader(struct cso_context *ctx)
{
   assert(!ctx->fragment_shader_saved);
   ctx->fragment_shader_saved = ctx->fragment_shader;
}

void cso_restore_fragment_shader(struct cso_context *ctx)
{
   if (ctx->fragment_shader_saved != ctx->fragment_shader) {
      ctx->pipe->bind_fs_state(ctx->pipe, ctx->fragment_shader_saved);
      ctx->fragment_shader = ctx->fragment_shader_saved;
   }
   ctx->fragment_shader_saved = NULL;
}


enum pipe_error cso_set_vertex_shader_handle(struct cso_context *ctx,
                                             void *handle )
{
   if (ctx->vertex_shader != handle) {
      ctx->vertex_shader = handle;
      ctx->pipe->bind_vs_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_delete_vertex_shader(struct cso_context *ctx, void *handle )
{
   if (handle == ctx->vertex_shader) {
      /* unbind before deleting */
      ctx->pipe->bind_vs_state(ctx->pipe, NULL);
      ctx->vertex_shader = NULL;
   }
   ctx->pipe->delete_vs_state(ctx->pipe, handle);
}


/* Not really working:
 */
#if 0
enum pipe_error cso_set_vertex_shader(struct cso_context *ctx,
                                      const struct pipe_shader_state *templ)
{
   unsigned hash_key = cso_construct_key((void*)templ,
                                         sizeof(struct pipe_shader_state));
   struct cso_hash_iter iter = cso_find_state_template(ctx->cache,
                                                       hash_key, CSO_VERTEX_SHADER,
                                                       (void*)templ,
                                                       sizeof(*templ));
   void *handle = NULL;

   if (cso_hash_iter_is_null(iter)) {
      struct cso_vertex_shader *cso = MALLOC(sizeof(struct cso_vertex_shader));

      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(cso->state, templ, sizeof(*templ));
      cso->data = ctx->pipe->create_vs_state(ctx->pipe, &cso->state);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_vs_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_VERTEX_SHADER, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_vertex_shader *)cso_hash_iter_data(iter))->data;
   }

   return cso_set_vertex_shader_handle( ctx, handle );
}
#endif



void cso_save_vertex_shader(struct cso_context *ctx)
{
   assert(!ctx->vertex_shader_saved);
   ctx->vertex_shader_saved = ctx->vertex_shader;
}

void cso_restore_vertex_shader(struct cso_context *ctx)
{
   if (ctx->vertex_shader_saved != ctx->vertex_shader) {
      ctx->pipe->bind_vs_state(ctx->pipe, ctx->vertex_shader_saved);
      ctx->vertex_shader = ctx->vertex_shader_saved;
   }
   ctx->vertex_shader_saved = NULL;
}


enum pipe_error cso_set_framebuffer(struct cso_context *ctx,
                                    const struct pipe_framebuffer_state *fb)
{
   if (memcmp(&ctx->fb, fb, sizeof(*fb)) != 0) {
      util_copy_framebuffer_state(&ctx->fb, fb);
      ctx->pipe->set_framebuffer_state(ctx->pipe, fb);
   }
   return PIPE_OK;
}

void cso_save_framebuffer(struct cso_context *ctx)
{
   util_copy_framebuffer_state(&ctx->fb_saved, &ctx->fb);
}

void cso_restore_framebuffer(struct cso_context *ctx)
{
   if (memcmp(&ctx->fb, &ctx->fb_saved, sizeof(ctx->fb))) {
      util_copy_framebuffer_state(&ctx->fb, &ctx->fb_saved);
      ctx->pipe->set_framebuffer_state(ctx->pipe, &ctx->fb);
      util_unreference_framebuffer_state(&ctx->fb_saved);
   }
}


enum pipe_error cso_set_viewport(struct cso_context *ctx,
                                 const struct pipe_viewport_state *vp)
{
   if (memcmp(&ctx->vp, vp, sizeof(*vp))) {
      ctx->vp = *vp;
      ctx->pipe->set_viewport_state(ctx->pipe, vp);
   }
   return PIPE_OK;
}

void cso_save_viewport(struct cso_context *ctx)
{
   ctx->vp_saved = ctx->vp;
}


void cso_restore_viewport(struct cso_context *ctx)
{
   if (memcmp(&ctx->vp, &ctx->vp_saved, sizeof(ctx->vp))) {
      ctx->vp = ctx->vp_saved;
      ctx->pipe->set_viewport_state(ctx->pipe, &ctx->vp);
   }
}


enum pipe_error cso_set_blend_color(struct cso_context *ctx,
                                    const struct pipe_blend_color *bc)
{
   if (memcmp(&ctx->blend_color, bc, sizeof(ctx->blend_color))) {
      ctx->blend_color = *bc;
      ctx->pipe->set_blend_color(ctx->pipe, bc);
   }
   return PIPE_OK;
}

enum pipe_error cso_set_sample_mask(struct cso_context *ctx,
                                    unsigned sample_mask)
{
   if (ctx->sample_mask != sample_mask) {
      ctx->sample_mask = sample_mask;
      ctx->pipe->set_sample_mask(ctx->pipe, sample_mask);
   }
   return PIPE_OK;
}

enum pipe_error cso_set_stencil_ref(struct cso_context *ctx,
                                    const struct pipe_stencil_ref *sr)
{
   if (memcmp(&ctx->stencil_ref, sr, sizeof(ctx->stencil_ref))) {
      ctx->stencil_ref = *sr;
      ctx->pipe->set_stencil_ref(ctx->pipe, sr);
   }
   return PIPE_OK;
}

void cso_save_stencil_ref(struct cso_context *ctx)
{
   ctx->stencil_ref_saved = ctx->stencil_ref;
}


void cso_restore_stencil_ref(struct cso_context *ctx)
{
   if (memcmp(&ctx->stencil_ref, &ctx->stencil_ref_saved, sizeof(ctx->stencil_ref))) {
      ctx->stencil_ref = ctx->stencil_ref_saved;
      ctx->pipe->set_stencil_ref(ctx->pipe, &ctx->stencil_ref);
   }
}

enum pipe_error cso_set_geometry_shader_handle(struct cso_context *ctx,
                                               void *handle)
{
   assert(ctx->has_geometry_shader || !handle);

   if (ctx->has_geometry_shader && ctx->geometry_shader != handle) {
      ctx->geometry_shader = handle;
      ctx->pipe->bind_gs_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_delete_geometry_shader(struct cso_context *ctx, void *handle)
{
    if (handle == ctx->geometry_shader) {
      /* unbind before deleting */
      ctx->pipe->bind_gs_state(ctx->pipe, NULL);
      ctx->geometry_shader = NULL;
   }
   ctx->pipe->delete_gs_state(ctx->pipe, handle);
}

void cso_save_geometry_shader(struct cso_context *ctx)
{
   if (!ctx->has_geometry_shader) {
      return;
   }

   assert(!ctx->geometry_shader_saved);
   ctx->geometry_shader_saved = ctx->geometry_shader;
}

void cso_restore_geometry_shader(struct cso_context *ctx)
{
   if (!ctx->has_geometry_shader) {
      return;
   }

   if (ctx->geometry_shader_saved != ctx->geometry_shader) {
      ctx->pipe->bind_gs_state(ctx->pipe, ctx->geometry_shader_saved);
      ctx->geometry_shader = ctx->geometry_shader_saved;
   }
   ctx->geometry_shader_saved = NULL;
}

/* clip state */

static INLINE void
clip_state_cpy(struct pipe_clip_state *dst,
               const struct pipe_clip_state *src)
{
   memcpy(dst->ucp, src->ucp, sizeof(dst->ucp));
}

static INLINE int
clip_state_cmp(const struct pipe_clip_state *a,
               const struct pipe_clip_state *b)
{
   return memcmp(a->ucp, b->ucp, sizeof(a->ucp));
}

void
cso_set_clip(struct cso_context *ctx,
             const struct pipe_clip_state *clip)
{
   if (clip_state_cmp(&ctx->clip, clip)) {
      clip_state_cpy(&ctx->clip, clip);
      ctx->pipe->set_clip_state(ctx->pipe, clip);
   }
}

void
cso_save_clip(struct cso_context *ctx)
{
   clip_state_cpy(&ctx->clip_saved, &ctx->clip);
}

void
cso_restore_clip(struct cso_context *ctx)
{
   if (clip_state_cmp(&ctx->clip, &ctx->clip_saved)) {
      clip_state_cpy(&ctx->clip, &ctx->clip_saved);
      ctx->pipe->set_clip_state(ctx->pipe, &ctx->clip_saved);
   }
}

enum pipe_error cso_set_vertex_elements(struct cso_context *ctx,
                                        unsigned count,
                                        const struct pipe_vertex_element *states)
{
   unsigned key_size, hash_key;
   struct cso_hash_iter iter;
   void *handle;
   struct cso_velems_state velems_state;

   /* need to include the count into the stored state data too.
      Otherwise first few count pipe_vertex_elements could be identical even if count
      is different, and there's no guarantee the hash would be different in that
      case neither */
   key_size = sizeof(struct pipe_vertex_element) * count + sizeof(unsigned);
   velems_state.count = count;
   memcpy(velems_state.velems, states, sizeof(struct pipe_vertex_element) * count);
   hash_key = cso_construct_key((void*)&velems_state, key_size);
   iter = cso_find_state_template(ctx->cache, hash_key, CSO_VELEMENTS, (void*)&velems_state, key_size);

   if (cso_hash_iter_is_null(iter)) {
      struct cso_velements *cso = MALLOC(sizeof(struct cso_velements));
      if (!cso)
         return PIPE_ERROR_OUT_OF_MEMORY;

      memcpy(&cso->state, &velems_state, key_size);
      cso->data = ctx->pipe->create_vertex_elements_state(ctx->pipe, count, &cso->state.velems[0]);
      cso->delete_state = (cso_state_callback)ctx->pipe->delete_vertex_elements_state;
      cso->context = ctx->pipe;

      iter = cso_insert_state(ctx->cache, hash_key, CSO_VELEMENTS, cso);
      if (cso_hash_iter_is_null(iter)) {
         FREE(cso);
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      handle = cso->data;
   }
   else {
      handle = ((struct cso_velements *)cso_hash_iter_data(iter))->data;
   }

   if (ctx->velements != handle) {
      ctx->velements = handle;
      ctx->pipe->bind_vertex_elements_state(ctx->pipe, handle);
   }
   return PIPE_OK;
}

void cso_save_vertex_elements(struct cso_context *ctx)
{
   assert(!ctx->velements_saved);
   ctx->velements_saved = ctx->velements;
}

void cso_restore_vertex_elements(struct cso_context *ctx)
{
   if (ctx->velements != ctx->velements_saved) {
      ctx->velements = ctx->velements_saved;
      ctx->pipe->bind_vertex_elements_state(ctx->pipe, ctx->velements_saved);
   }
   ctx->velements_saved = NULL;
}

/* vertex buffers */

void cso_set_vertex_buffers(struct cso_context *ctx,
                            unsigned count,
                            const struct pipe_vertex_buffer *buffers)
{
   if (count != ctx->nr_vertex_buffers ||
       memcmp(buffers, ctx->vertex_buffers,
              sizeof(struct pipe_vertex_buffer) * count) != 0) {
      util_copy_vertex_buffers(ctx->vertex_buffers, &ctx->nr_vertex_buffers,
                               buffers, count);
      ctx->pipe->set_vertex_buffers(ctx->pipe, count, buffers);
   }
}

void cso_save_vertex_buffers(struct cso_context *ctx)
{
   util_copy_vertex_buffers(ctx->vertex_buffers_saved,
                            &ctx->nr_vertex_buffers_saved,
                            ctx->vertex_buffers,
                            ctx->nr_vertex_buffers);
}

void cso_restore_vertex_buffers(struct cso_context *ctx)
{
   util_copy_vertex_buffers(ctx->vertex_buffers,
                            &ctx->nr_vertex_buffers,
                            ctx->vertex_buffers_saved,
                            ctx->nr_vertex_buffers_saved);
   ctx->pipe->set_vertex_buffers(ctx->pipe, ctx->nr_vertex_buffers,
                                 ctx->vertex_buffers);
}


/**************** fragment/vertex sampler view state *************************/

static enum pipe_error
single_sampler(struct cso_context *ctx,
               struct sampler_info *info,
               unsigned idx,
               const struct pipe_sampler_state *templ)
{
   void *handle = NULL;

   if (templ != NULL) {
      unsigned key_size = sizeof(struct pipe_sampler_state);
      unsigned hash_key = cso_construct_key((void*)templ, key_size);
      struct cso_hash_iter iter =
         cso_find_state_template(ctx->cache,
                                 hash_key, CSO_SAMPLER,
                                 (void *) templ, key_size);

      if (cso_hash_iter_is_null(iter)) {
         struct cso_sampler *cso = MALLOC(sizeof(struct cso_sampler));
         if (!cso)
            return PIPE_ERROR_OUT_OF_MEMORY;

         memcpy(&cso->state, templ, sizeof(*templ));
         cso->data = ctx->pipe->create_sampler_state(ctx->pipe, &cso->state);
         cso->delete_state = (cso_state_callback)ctx->pipe->delete_sampler_state;
         cso->context = ctx->pipe;

         iter = cso_insert_state(ctx->cache, hash_key, CSO_SAMPLER, cso);
         if (cso_hash_iter_is_null(iter)) {
            FREE(cso);
            return PIPE_ERROR_OUT_OF_MEMORY;
         }

         handle = cso->data;
      }
      else {
         handle = ((struct cso_sampler *)cso_hash_iter_data(iter))->data;
      }
   }

   info->samplers[idx] = handle;

   return PIPE_OK;
}

enum pipe_error
cso_single_sampler(struct cso_context *ctx,
                   unsigned idx,
                   const struct pipe_sampler_state *templ)
{
   return single_sampler(ctx, &ctx->fragment_samplers, idx, templ);
}

enum pipe_error
cso_single_vertex_sampler(struct cso_context *ctx,
                          unsigned idx,
                          const struct pipe_sampler_state *templ)
{
   return single_sampler(ctx, &ctx->vertex_samplers, idx, templ);
}



static void
single_sampler_done(struct cso_context *ctx,
                    struct sampler_info *info)
{
   unsigned i;

   /* find highest non-null sampler */
   for (i = PIPE_MAX_SAMPLERS; i > 0; i--) {
      if (info->samplers[i - 1] != NULL)
         break;
   }

   info->nr_samplers = i;

   if (info->hw.nr_samplers != info->nr_samplers ||
       memcmp(info->hw.samplers,
              info->samplers,
              info->nr_samplers * sizeof(void *)) != 0) 
   {
      memcpy(info->hw.samplers,
             info->samplers,
             info->nr_samplers * sizeof(void *));
      info->hw.nr_samplers = info->nr_samplers;

      if (info == &ctx->fragment_samplers) {
         ctx->pipe->bind_fragment_sampler_states(ctx->pipe,
                                                 info->nr_samplers,
                                                 info->samplers);
      }
      else if (info == &ctx->vertex_samplers) {
         ctx->pipe->bind_vertex_sampler_states(ctx->pipe,
                                               info->nr_samplers,
                                               info->samplers);
      }
      else {
         assert(0);
      }
   }
}

void
cso_single_sampler_done( struct cso_context *ctx )
{
   single_sampler_done(ctx, &ctx->fragment_samplers);
}

void
cso_single_vertex_sampler_done(struct cso_context *ctx)
{
   single_sampler_done(ctx, &ctx->vertex_samplers);
}


/*
 * If the function encouters any errors it will return the
 * last one. Done to always try to set as many samplers
 * as possible.
 */
static enum pipe_error
set_samplers(struct cso_context *ctx,
             struct sampler_info *info,
             unsigned nr,
             const struct pipe_sampler_state **templates)
{
   unsigned i;
   enum pipe_error temp, error = PIPE_OK;

   /* TODO: fastpath
    */

   for (i = 0; i < nr; i++) {
      temp = single_sampler(ctx, info, i, templates[i]);
      if (temp != PIPE_OK)
         error = temp;
   }

   for ( ; i < info->nr_samplers; i++) {
      temp = single_sampler(ctx, info, i, NULL);
      if (temp != PIPE_OK)
         error = temp;
   }

   single_sampler_done(ctx, info);

   return error;
}

enum pipe_error
cso_set_samplers(struct cso_context *ctx,
                 unsigned nr,
                 const struct pipe_sampler_state **templates)
{
   return set_samplers(ctx, &ctx->fragment_samplers, nr, templates);
}

enum pipe_error
cso_set_vertex_samplers(struct cso_context *ctx,
                        unsigned nr,
                        const struct pipe_sampler_state **templates)
{
   return set_samplers(ctx, &ctx->vertex_samplers, nr, templates);
}



static void
save_samplers(struct cso_context *ctx, struct sampler_info *info)
{
   info->nr_samplers_saved = info->nr_samplers;
   memcpy(info->samplers_saved, info->samplers, sizeof(info->samplers));
}

void
cso_save_samplers(struct cso_context *ctx)
{
   save_samplers(ctx, &ctx->fragment_samplers);
}

void
cso_save_vertex_samplers(struct cso_context *ctx)
{
   save_samplers(ctx, &ctx->vertex_samplers);
}



static void
restore_samplers(struct cso_context *ctx, struct sampler_info *info)
{
   info->nr_samplers = info->nr_samplers_saved;
   memcpy(info->samplers, info->samplers_saved, sizeof(info->samplers));
   single_sampler_done(ctx, info);
}

void
cso_restore_samplers(struct cso_context *ctx)
{
   restore_samplers(ctx, &ctx->fragment_samplers);
}

void
cso_restore_vertex_samplers(struct cso_context *ctx)
{
   restore_samplers(ctx, &ctx->vertex_samplers);
}



static void
set_sampler_views(struct cso_context *ctx,
                  struct sampler_info *info,
                  void (*set_views)(struct pipe_context *,
                                    unsigned num_views,
                                    struct pipe_sampler_view **),
                  uint count,
                  struct pipe_sampler_view **views)
{
   uint i;

   /* reference new views */
   for (i = 0; i < count; i++) {
      pipe_sampler_view_reference(&info->views[i], views[i]);
   }
   /* unref extra old views, if any */
   for (; i < info->nr_views; i++) {
      pipe_sampler_view_reference(&info->views[i], NULL);
   }

   info->nr_views = count;

   /* bind the new sampler views */
   set_views(ctx->pipe, count, info->views);
}

void
cso_set_fragment_sampler_views(struct cso_context *ctx,
                               uint count,
                               struct pipe_sampler_view **views)
{
   set_sampler_views(ctx, &ctx->fragment_samplers,
                     ctx->pipe->set_fragment_sampler_views,
                     count, views);
}

void
cso_set_vertex_sampler_views(struct cso_context *ctx,
                             uint count,
                             struct pipe_sampler_view **views)
{
   set_sampler_views(ctx, &ctx->vertex_samplers,
                     ctx->pipe->set_vertex_sampler_views,
                     count, views);
}



static void
save_sampler_views(struct cso_context *ctx,
                   struct sampler_info *info)
{
   uint i;

   info->nr_views_saved = info->nr_views;

   for (i = 0; i < info->nr_views; i++) {
      assert(!info->views_saved[i]);
      pipe_sampler_view_reference(&info->views_saved[i], info->views[i]);
   }
}

void
cso_save_fragment_sampler_views(struct cso_context *ctx)
{
   save_sampler_views(ctx, &ctx->fragment_samplers);
}

void
cso_save_vertex_sampler_views(struct cso_context *ctx)
{
   save_sampler_views(ctx, &ctx->vertex_samplers);
}


static void
restore_sampler_views(struct cso_context *ctx,
                      struct sampler_info *info,
                      void (*set_views)(struct pipe_context *,
                                        unsigned num_views,
                                        struct pipe_sampler_view **))
{
   uint i;

   for (i = 0; i < info->nr_views_saved; i++) {
      pipe_sampler_view_reference(&info->views[i], NULL);
      /* move the reference from one pointer to another */
      info->views[i] = info->views_saved[i];
      info->views_saved[i] = NULL;
   }
   for (; i < info->nr_views; i++) {
      pipe_sampler_view_reference(&info->views[i], NULL);
   }

   /* bind the old/saved sampler views */
   set_views(ctx->pipe, info->nr_views_saved, info->views);

   info->nr_views = info->nr_views_saved;
   info->nr_views_saved = 0;
}

void
cso_restore_fragment_sampler_views(struct cso_context *ctx)
{
   restore_sampler_views(ctx, &ctx->fragment_samplers,
                         ctx->pipe->set_fragment_sampler_views);
}

void
cso_restore_vertex_sampler_views(struct cso_context *ctx)
{
   restore_sampler_views(ctx, &ctx->vertex_samplers,
                         ctx->pipe->set_vertex_sampler_views);
}


void
cso_set_stream_outputs(struct cso_context *ctx,
                       unsigned num_targets,
                       struct pipe_stream_output_target **targets,
                       unsigned append_bitmask)
{
   struct pipe_context *pipe = ctx->pipe;
   uint i;

   if (!ctx->has_streamout) {
      assert(num_targets == 0);
      return;
   }

   if (ctx->nr_so_targets == 0 && num_targets == 0) {
      /* Nothing to do. */
      return;
   }

   /* reference new targets */
   for (i = 0; i < num_targets; i++) {
      pipe_so_target_reference(&ctx->so_targets[i], targets[i]);
   }
   /* unref extra old targets, if any */
   for (; i < ctx->nr_so_targets; i++) {
      pipe_so_target_reference(&ctx->so_targets[i], NULL);
   }

   pipe->set_stream_output_targets(pipe, num_targets, targets,
                                   append_bitmask);
   ctx->nr_so_targets = num_targets;
}

void
cso_save_stream_outputs(struct cso_context *ctx)
{
   uint i;

   if (!ctx->has_streamout) {
      return;
   }

   ctx->nr_so_targets_saved = ctx->nr_so_targets;

   for (i = 0; i < ctx->nr_so_targets; i++) {
      assert(!ctx->so_targets_saved[i]);
      pipe_so_target_reference(&ctx->so_targets_saved[i], ctx->so_targets[i]);
   }
}

void
cso_restore_stream_outputs(struct cso_context *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   uint i;

   if (!ctx->has_streamout) {
      return;
   }

   if (ctx->nr_so_targets == 0 && ctx->nr_so_targets_saved == 0) {
      /* Nothing to do. */
      return;
   }

   for (i = 0; i < ctx->nr_so_targets_saved; i++) {
      pipe_so_target_reference(&ctx->so_targets[i], NULL);
      /* move the reference from one pointer to another */
      ctx->so_targets[i] = ctx->so_targets_saved[i];
      ctx->so_targets_saved[i] = NULL;
   }
   for (; i < ctx->nr_so_targets; i++) {
      pipe_so_target_reference(&ctx->so_targets[i], NULL);
   }

   /* ~0 means append */
   pipe->set_stream_output_targets(pipe, ctx->nr_so_targets_saved,
                                   ctx->so_targets, ~0);

   ctx->nr_so_targets = ctx->nr_so_targets_saved;
   ctx->nr_so_targets_saved = 0;
}
