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

#include "u_surfaces.h"
#include "util/u_hash_table.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"

boolean
util_surfaces_do_get(struct util_surfaces *us, unsigned surface_struct_size,
                     struct pipe_context *ctx, struct pipe_resource *pt,
                     unsigned level, unsigned layer, unsigned flags,
                     struct pipe_surface **res)
{
   struct pipe_surface *ps;

   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
   {    /* or 2D array */
      if(!us->u.hash)
         us->u.hash = cso_hash_create();

      ps = cso_hash_iter_data(cso_hash_find(us->u.hash, (layer << 8) | level));
   }
   else
   {
      if(!us->u.array)
         us->u.array = CALLOC(pt->last_level + 1, sizeof(struct pipe_surface *));
      ps = us->u.array[level];
   }

   if(ps && ps->context == ctx)
   {
      p_atomic_inc(&ps->reference.count);
      *res = ps;
      return FALSE;
   }

   ps = (struct pipe_surface *)CALLOC(1, surface_struct_size);
   if(!ps)
   {
      *res = NULL;
      return FALSE;
   }

   pipe_surface_init(ctx, ps, pt, level, layer, flags);

   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
      cso_hash_insert(us->u.hash, (layer << 8) | level, ps);
   else
      us->u.array[level] = ps;

   *res = ps;
   return TRUE;
}

void
util_surfaces_do_detach(struct util_surfaces *us, struct pipe_surface *ps)
{
   struct pipe_resource *pt = ps->texture;
   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
   {    /* or 2D array */
      cso_hash_erase(us->u.hash, cso_hash_find(us->u.hash, (ps->u.tex.first_layer << 8) | ps->u.tex.level));
   }
   else
      us->u.array[ps->u.tex.level] = 0;
}

void
util_surfaces_destroy(struct util_surfaces *us, struct pipe_resource *pt, void (*destroy_surface) (struct pipe_surface *))
{
   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
   {    /* or 2D array */
      if(us->u.hash)
      {
         struct cso_hash_iter iter;
         iter = cso_hash_first_node(us->u.hash);
         while (!cso_hash_iter_is_null(iter)) {
            destroy_surface(cso_hash_iter_data(iter));
            iter = cso_hash_iter_next(iter);
         }

         cso_hash_delete(us->u.hash);
         us->u.hash = NULL;
      }
   }
   else
   {
      if(us->u.array)
      {
         unsigned i;
         for(i = 0; i <= pt->last_level; ++i)
         {
            struct pipe_surface *ps = us->u.array[i];
            if(ps)
               destroy_surface(ps);
         }
         FREE(us->u.array);
         us->u.array = NULL;
      }
   }
}
