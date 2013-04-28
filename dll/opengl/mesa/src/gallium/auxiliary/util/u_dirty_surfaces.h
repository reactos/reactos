/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef U_DIRTY_SURFACES_H_
#define U_DIRTY_SURFACES_H_

#include "pipe/p_state.h"

#include "util/u_double_list.h"
#include "util/u_math.h"

struct pipe_context;

typedef void (*util_dirty_surface_flush_t) (struct pipe_context *, struct pipe_surface *);

struct util_dirty_surfaces
{
   struct list_head dirty_list;
};

struct util_dirty_surface
{
   struct pipe_surface base;
   struct list_head dirty_list;
};

static INLINE void
util_dirty_surfaces_init(struct util_dirty_surfaces *ds)
{
   LIST_INITHEAD(&ds->dirty_list);
}

static INLINE void
util_dirty_surfaces_use_for_sampling(struct pipe_context *pipe, struct util_dirty_surfaces *dss, util_dirty_surface_flush_t flush)
{
   struct list_head *p, *next;
   for(p = dss->dirty_list.next; p != &dss->dirty_list; p = next)
   {
      struct util_dirty_surface *ds = LIST_ENTRY(struct util_dirty_surface, p, dirty_list);
      next = p->next;

      flush(pipe, &ds->base);
   }
}

static INLINE void
util_dirty_surfaces_use_levels_for_sampling(struct pipe_context *pipe, struct util_dirty_surfaces *dss, unsigned first, unsigned last, util_dirty_surface_flush_t flush)
{
   struct list_head *p, *next;
   if(first > last)
      return;
   for(p = dss->dirty_list.next; p != &dss->dirty_list; p = next)
   {
      struct util_dirty_surface *ds = LIST_ENTRY(struct util_dirty_surface, p, dirty_list);
      next = p->next;

      if(ds->base.u.tex.level >= first && ds->base.u.tex.level <= last)
	 flush(pipe, &ds->base);
   }
}

static INLINE void
util_dirty_surfaces_use_for_sampling_with(struct pipe_context *pipe, struct util_dirty_surfaces *dss, struct pipe_sampler_view *psv, struct pipe_sampler_state *pss, util_dirty_surface_flush_t flush)
{
   if(!LIST_IS_EMPTY(&dss->dirty_list))
      util_dirty_surfaces_use_levels_for_sampling(pipe, dss, (unsigned)pss->min_lod + psv->u.tex.first_level,
						  MIN2((unsigned)ceilf(pss->max_lod) + psv->u.tex.first_level, psv->u.tex.last_level), flush);
}

static INLINE void
util_dirty_surface_init(struct util_dirty_surface *ds)
{
   LIST_INITHEAD(&ds->dirty_list);
}

static INLINE boolean
util_dirty_surface_is_dirty(struct util_dirty_surface *ds)
{
   return !LIST_IS_EMPTY(&ds->dirty_list);
}

static INLINE void
util_dirty_surface_set_dirty(struct util_dirty_surfaces *dss, struct util_dirty_surface *ds)
{
   if(LIST_IS_EMPTY(&ds->dirty_list))
      LIST_ADDTAIL(&ds->dirty_list, &dss->dirty_list);
}

static INLINE void
util_dirty_surface_set_clean(struct util_dirty_surfaces *dss, struct util_dirty_surface *ds)
{
   if(!LIST_IS_EMPTY(&ds->dirty_list))
      LIST_DELINIT(&ds->dirty_list);
}

#endif
