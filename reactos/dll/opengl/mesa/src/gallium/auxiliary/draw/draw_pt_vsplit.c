/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "util/u_math.h"
#include "util/u_memory.h"

#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"

#define SEGMENT_SIZE 1024
#define MAP_SIZE     256

struct vsplit_frontend {
   struct draw_pt_front_end base;
   struct draw_context *draw;

   unsigned prim;

   struct draw_pt_middle_end *middle;

   unsigned max_vertices;
   ushort segment_size;

   /* buffers for splitting */
   unsigned fetch_elts[SEGMENT_SIZE];
   ushort draw_elts[SEGMENT_SIZE];
   ushort identity_draw_elts[SEGMENT_SIZE];

   struct {
      /* map a fetch element to a draw element */
      unsigned fetches[MAP_SIZE];
      ushort draws[MAP_SIZE];
      boolean has_max_fetch;

      ushort num_fetch_elts;
      ushort num_draw_elts;
   } cache;
};


static void
vsplit_clear_cache(struct vsplit_frontend *vsplit)
{
   memset(vsplit->cache.fetches, 0xff, sizeof(vsplit->cache.fetches));
   vsplit->cache.has_max_fetch = FALSE;
   vsplit->cache.num_fetch_elts = 0;
   vsplit->cache.num_draw_elts = 0;
}

static void
vsplit_flush_cache(struct vsplit_frontend *vsplit, unsigned flags)
{
   vsplit->middle->run(vsplit->middle,
         vsplit->fetch_elts, vsplit->cache.num_fetch_elts,
         vsplit->draw_elts, vsplit->cache.num_draw_elts, flags);
}

/**
 * Add a fetch element and add it to the draw elements.
 */
static INLINE void
vsplit_add_cache(struct vsplit_frontend *vsplit, unsigned fetch)
{
   struct draw_context *draw = vsplit->draw;
   unsigned hash;

   fetch = MIN2(fetch, draw->pt.max_index);

   hash = fetch % MAP_SIZE;

   if (vsplit->cache.fetches[hash] != fetch) {
      /* update cache */
      vsplit->cache.fetches[hash] = fetch;
      vsplit->cache.draws[hash] = vsplit->cache.num_fetch_elts;

      /* add fetch */
      assert(vsplit->cache.num_fetch_elts < vsplit->segment_size);
      vsplit->fetch_elts[vsplit->cache.num_fetch_elts++] = fetch;
   }

   vsplit->draw_elts[vsplit->cache.num_draw_elts++] = vsplit->cache.draws[hash];
}


/**
 * Add a fetch element and add it to the draw elements.  The fetch element is
 * in full range (uint).
 */
static INLINE void
vsplit_add_cache_uint(struct vsplit_frontend *vsplit, unsigned fetch)
{
   /* special care for 0xffffffff */
   if (fetch == 0xffffffff && !vsplit->cache.has_max_fetch) {
      unsigned hash = fetch % MAP_SIZE;
      vsplit->cache.fetches[hash] = fetch - 1; /* force update */
      vsplit->cache.has_max_fetch = TRUE;
   }

   vsplit_add_cache(vsplit, fetch);
}


#define FUNC vsplit_run_linear
#include "draw_pt_vsplit_tmp.h"

#define FUNC vsplit_run_ubyte
#define ELT_TYPE ubyte
#define ADD_CACHE(vsplit, fetch) vsplit_add_cache(vsplit, fetch)
#include "draw_pt_vsplit_tmp.h"

#define FUNC vsplit_run_ushort
#define ELT_TYPE ushort
#define ADD_CACHE(vsplit, fetch) vsplit_add_cache(vsplit, fetch)
#include "draw_pt_vsplit_tmp.h"

#define FUNC vsplit_run_uint
#define ELT_TYPE uint
#define ADD_CACHE(vsplit, fetch) vsplit_add_cache_uint(vsplit, fetch)
#include "draw_pt_vsplit_tmp.h"


static void vsplit_prepare(struct draw_pt_front_end *frontend,
                           unsigned in_prim,
                           struct draw_pt_middle_end *middle,
                           unsigned opt)
{
   struct vsplit_frontend *vsplit = (struct vsplit_frontend *) frontend;

   switch (vsplit->draw->pt.user.eltSize) {
   case 0:
      vsplit->base.run = vsplit_run_linear;
      break;
   case 1:
      vsplit->base.run = vsplit_run_ubyte;
      break;
   case 2:
      vsplit->base.run = vsplit_run_ushort;
      break;
   case 4:
      vsplit->base.run = vsplit_run_uint;
      break;
   default:
      assert(0);
      break;
   }

   /* split only */
   vsplit->prim = in_prim;

   vsplit->middle = middle;
   middle->prepare(middle, vsplit->prim, opt, &vsplit->max_vertices);

   vsplit->segment_size = MIN2(SEGMENT_SIZE, vsplit->max_vertices);
}


static void vsplit_finish(struct draw_pt_front_end *frontend)
{
   struct vsplit_frontend *vsplit = (struct vsplit_frontend *) frontend;
   vsplit->middle->finish(vsplit->middle);
   vsplit->middle = NULL;
}


static void vsplit_destroy(struct draw_pt_front_end *frontend)
{
   FREE(frontend);
}


struct draw_pt_front_end *draw_pt_vsplit(struct draw_context *draw)
{
   struct vsplit_frontend *vsplit = CALLOC_STRUCT(vsplit_frontend);
   ushort i;

   if (!vsplit)
      return NULL;

   vsplit->base.prepare = vsplit_prepare;
   vsplit->base.run     = NULL;
   vsplit->base.finish  = vsplit_finish;
   vsplit->base.destroy = vsplit_destroy;
   vsplit->draw = draw;

   for (i = 0; i < SEGMENT_SIZE; i++)
      vsplit->identity_draw_elts[i] = i;

   return &vsplit->base;
}
