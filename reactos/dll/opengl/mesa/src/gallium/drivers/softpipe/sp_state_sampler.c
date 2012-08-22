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

/* Authors:
 *  Brian Paul
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "draw/draw_context.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tex_sample.h"
#include "sp_tex_tile_cache.h"


struct sp_sampler {
   struct pipe_sampler_state base;
   struct sp_sampler_variant *variants;
   struct sp_sampler_variant *current;
};

static struct sp_sampler *sp_sampler( struct pipe_sampler_state *sampler )
{
   return (struct sp_sampler *)sampler;
}


static void *
softpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct sp_sampler *sp_sampler = CALLOC_STRUCT(sp_sampler);

   sp_sampler->base = *sampler;
   sp_sampler->variants = NULL;

   return (void *)sp_sampler;
}


static void
softpipe_bind_fragment_sampler_states(struct pipe_context *pipe,
                                      unsigned num, void **sampler)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_fragment_samplers &&
       !memcmp(softpipe->fragment_samplers, sampler, num * sizeof(void *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < num; ++i)
      softpipe->fragment_samplers[i] = sampler[i];
   for (i = num; i < PIPE_MAX_SAMPLERS; ++i)
      softpipe->fragment_samplers[i] = NULL;

   softpipe->num_fragment_samplers = num;

   softpipe->dirty |= SP_NEW_SAMPLER;
}


static void
softpipe_bind_vertex_sampler_states(struct pipe_context *pipe,
                                    unsigned num_samplers,
                                    void **samplers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(num_samplers <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num_samplers == softpipe->num_vertex_samplers &&
       !memcmp(softpipe->vertex_samplers, samplers, num_samplers * sizeof(void *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < num_samplers; ++i)
      softpipe->vertex_samplers[i] = samplers[i];
   for (i = num_samplers; i < PIPE_MAX_VERTEX_SAMPLERS; ++i)
      softpipe->vertex_samplers[i] = NULL;

   softpipe->num_vertex_samplers = num_samplers;

   draw_set_samplers(softpipe->draw,
                     softpipe->vertex_samplers,
                     softpipe->num_vertex_samplers);

   softpipe->dirty |= SP_NEW_SAMPLER;
}

static void
softpipe_bind_geometry_sampler_states(struct pipe_context *pipe,
                                      unsigned num_samplers,
                                      void **samplers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(num_samplers <= PIPE_MAX_GEOMETRY_SAMPLERS);

   /* Check for no-op */
   if (num_samplers == softpipe->num_geometry_samplers &&
       !memcmp(softpipe->geometry_samplers, samplers, num_samplers * sizeof(void *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < num_samplers; ++i)
      softpipe->geometry_samplers[i] = samplers[i];
   for (i = num_samplers; i < PIPE_MAX_GEOMETRY_SAMPLERS; ++i)
      softpipe->geometry_samplers[i] = NULL;

   softpipe->num_geometry_samplers = num_samplers;

   softpipe->dirty |= SP_NEW_SAMPLER;
}


static struct pipe_sampler_view *
softpipe_create_sampler_view(struct pipe_context *pipe,
                             struct pipe_resource *resource,
                             const struct pipe_sampler_view *templ)
{
   struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);

   if (view) {
      *view = *templ;
      view->reference.count = 1;
      view->texture = NULL;
      pipe_resource_reference(&view->texture, resource);
      view->context = pipe;
   }

   return view;
}


static void
softpipe_sampler_view_destroy(struct pipe_context *pipe,
                              struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}


static void
softpipe_set_fragment_sampler_views(struct pipe_context *pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_fragment_sampler_views &&
       !memcmp(softpipe->fragment_sampler_views, views,
               num * sizeof(struct pipe_sampler_view *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&softpipe->fragment_sampler_views[i], view);
      sp_tex_tile_cache_set_sampler_view(softpipe->fragment_tex_cache[i], view);
   }

   softpipe->num_fragment_sampler_views = num;

   softpipe->dirty |= SP_NEW_TEXTURE;
}


static void
softpipe_set_vertex_sampler_views(struct pipe_context *pipe,
                                  unsigned num,
                                  struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_vertex_sampler_views &&
       !memcmp(softpipe->vertex_sampler_views, views, num * sizeof(struct pipe_sampler_view *))) {
      return;
   }

   draw_flush(softpipe->draw);

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&softpipe->vertex_sampler_views[i], view);
      sp_tex_tile_cache_set_sampler_view(softpipe->vertex_tex_cache[i], view);
   }

   softpipe->num_vertex_sampler_views = num;

   draw_set_sampler_views(softpipe->draw,
                          softpipe->vertex_sampler_views,
                          softpipe->num_vertex_sampler_views);

   softpipe->dirty |= SP_NEW_TEXTURE;
}


static void
softpipe_set_geometry_sampler_views(struct pipe_context *pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_GEOMETRY_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_geometry_sampler_views &&
       !memcmp(softpipe->geometry_sampler_views, views, num * sizeof(struct pipe_sampler_view *))) {
      return;
   }

   draw_flush(softpipe->draw);

   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&softpipe->geometry_sampler_views[i], view);
      sp_tex_tile_cache_set_sampler_view(softpipe->geometry_tex_cache[i], view);
   }

   softpipe->num_geometry_sampler_views = num;

   softpipe->dirty |= SP_NEW_TEXTURE;
}


/**
 * Find/create an sp_sampler_variant object for sampling the given texture,
 * sampler and tex unit.
 *
 * Note that the tex unit is significant.  We can't re-use a sampler
 * variant for multiple texture units because the sampler variant contains
 * the texture object pointer.  If the texture object pointer were stored
 * somewhere outside the sampler variant, we could re-use samplers for
 * multiple texture units.
 */
static struct sp_sampler_variant *
get_sampler_variant( unsigned unit,
                     struct sp_sampler *sampler,
                     struct pipe_sampler_view *view,
                     unsigned processor )
{
   struct softpipe_resource *sp_texture = softpipe_resource(view->texture);
   struct sp_sampler_variant *v = NULL;
   union sp_sampler_key key;

   /* if this fails, widen the key.unit field and update this assertion */
   assert(PIPE_MAX_SAMPLERS <= 16);

   key.bits.target = sp_texture->base.target;
   key.bits.is_pot = sp_texture->pot;
   key.bits.processor = processor;
   key.bits.unit = unit;
   key.bits.swizzle_r = view->swizzle_r;
   key.bits.swizzle_g = view->swizzle_g;
   key.bits.swizzle_b = view->swizzle_b;
   key.bits.swizzle_a = view->swizzle_a;
   key.bits.pad = 0;

   if (sampler->current && 
       key.value == sampler->current->key.value) {
      v = sampler->current;
   }

   if (v == NULL) {
      for (v = sampler->variants; v; v = v->next)
         if (v->key.value == key.value)
            break;

      if (v == NULL) {
         v = sp_create_sampler_variant( &sampler->base, key );
         v->next = sampler->variants;
         sampler->variants = v;
      }
   }
   
   sampler->current = v;
   return v;
}


void
softpipe_reset_sampler_variants(struct softpipe_context *softpipe)
{
   int i;

   /* It's a bit hard to build these samplers ahead of time -- don't
    * really know which samplers are going to be used for vertex and
    * fragment programs.
    */
   for (i = 0; i <= softpipe->vs->max_sampler; i++) {
      if (softpipe->vertex_samplers[i]) {
         softpipe->tgsi.vert_samplers_list[i] = 
            get_sampler_variant( i,
                                 sp_sampler(softpipe->vertex_samplers[i]),
                                 softpipe->vertex_sampler_views[i],
                                 TGSI_PROCESSOR_VERTEX );

         sp_sampler_variant_bind_view( softpipe->tgsi.vert_samplers_list[i],
                                       softpipe->vertex_tex_cache[i],
                                       softpipe->vertex_sampler_views[i] );
      }
   }

   if (softpipe->gs) {
      for (i = 0; i <= softpipe->gs->max_sampler; i++) {
         if (softpipe->geometry_samplers[i]) {
            softpipe->tgsi.geom_samplers_list[i] =
               get_sampler_variant(
                  i,
                  sp_sampler(softpipe->geometry_samplers[i]),
                  softpipe->geometry_sampler_views[i],
                  TGSI_PROCESSOR_GEOMETRY );

            sp_sampler_variant_bind_view(
               softpipe->tgsi.geom_samplers_list[i],
               softpipe->geometry_tex_cache[i],
               softpipe->geometry_sampler_views[i] );
         }
      }
   }

   for (i = 0; i <= softpipe->fs_variant->info.file_max[TGSI_FILE_SAMPLER]; i++) {
      if (softpipe->fragment_samplers[i]) {
         assert(softpipe->fragment_sampler_views[i]->texture);
         softpipe->tgsi.frag_samplers_list[i] =
            get_sampler_variant( i,
                                 sp_sampler(softpipe->fragment_samplers[i]),
                                 softpipe->fragment_sampler_views[i],
                                 TGSI_PROCESSOR_FRAGMENT );

         sp_sampler_variant_bind_view( softpipe->tgsi.frag_samplers_list[i],
                                       softpipe->fragment_tex_cache[i],
                                       softpipe->fragment_sampler_views[i] );
      }
   }
}

static void
softpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   struct sp_sampler *sp_sampler = (struct sp_sampler *)sampler;
   struct sp_sampler_variant *v, *tmp;

   for (v = sp_sampler->variants; v; v = tmp) {
      tmp = v->next;
      sp_sampler_variant_destroy(v);
   }

   FREE( sampler );
}


void
softpipe_init_sampler_funcs(struct pipe_context *pipe)
{
   pipe->create_sampler_state = softpipe_create_sampler_state;
   pipe->bind_fragment_sampler_states  = softpipe_bind_fragment_sampler_states;
   pipe->bind_vertex_sampler_states = softpipe_bind_vertex_sampler_states;
   pipe->bind_geometry_sampler_states = softpipe_bind_geometry_sampler_states;
   pipe->delete_sampler_state = softpipe_delete_sampler_state;

   pipe->set_fragment_sampler_views = softpipe_set_fragment_sampler_views;
   pipe->set_vertex_sampler_views = softpipe_set_vertex_sampler_views;
   pipe->set_geometry_sampler_views = softpipe_set_geometry_sampler_views;

   pipe->create_sampler_view = softpipe_create_sampler_view;
   pipe->sampler_view_destroy = softpipe_sampler_view_destroy;
}

