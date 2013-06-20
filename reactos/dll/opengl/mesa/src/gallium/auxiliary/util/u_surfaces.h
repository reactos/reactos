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

#ifndef U_SURFACES_H_
#define U_SURFACES_H_

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_atomic.h"
#include "cso_cache/cso_hash.h"

struct util_surfaces
{
   union
   {
      struct cso_hash *hash;
      struct pipe_surface **array;
      void* pv;
   } u;
};

/* Return value indicates if the pipe surface result is new */
boolean
util_surfaces_do_get(struct util_surfaces *us, unsigned surface_struct_size,
                     struct pipe_context *ctx, struct pipe_resource *pt,
                     unsigned level, unsigned layer, unsigned flags,
                     struct pipe_surface **res);

/* fast inline path for the very common case */
static INLINE boolean
util_surfaces_get(struct util_surfaces *us, unsigned surface_struct_size,
                  struct pipe_context *ctx, struct pipe_resource *pt,
                  unsigned level, unsigned layer, unsigned flags,
                  struct pipe_surface **res)
{
   if(likely((pt->target == PIPE_TEXTURE_2D || pt->target == PIPE_TEXTURE_RECT) && us->u.array))
   {
      struct pipe_surface *ps = us->u.array[level];
      if(ps && ps->context == ctx)
      {
	 p_atomic_inc(&ps->reference.count);
	 *res = ps;
	 return FALSE;
      }
   }

   return util_surfaces_do_get(us, surface_struct_size, ctx, pt, level, layer, flags, res);
}

static INLINE struct pipe_surface *
util_surfaces_peek(struct util_surfaces *us, struct pipe_resource *pt, unsigned level, unsigned layer)
{
   if(!us->u.pv)
      return 0;

   if(unlikely(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE))
      return cso_hash_iter_data(cso_hash_find(us->u.hash, (layer << 8) | level));
   else
      return us->u.array[level];
}

void util_surfaces_do_detach(struct util_surfaces *us, struct pipe_surface *ps);

static INLINE void
util_surfaces_detach(struct util_surfaces *us, struct pipe_surface *ps)
{
   if(likely(ps->texture->target == PIPE_TEXTURE_2D || ps->texture->target == PIPE_TEXTURE_RECT))
   {
      us->u.array[ps->u.tex.level] = 0;
      return;
   }

   util_surfaces_do_detach(us, ps);
}

void util_surfaces_destroy(struct util_surfaces *us, struct pipe_resource *pt, void (*destroy_surface) (struct pipe_surface *));

#endif
