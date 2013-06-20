/* Originally written by Ben Skeggs for the nv50 driver*/

#ifndef U_SPLIT_PRIM_H
#define U_SPLIT_PRIM_H

#include "pipe/p_defines.h"
#include "pipe/p_compiler.h"

#include "util/u_debug.h"

struct util_split_prim {
   void *priv;
   void (*emit)(void *priv, unsigned start, unsigned count);
   void (*edge)(void *priv, boolean enabled);

   unsigned mode;
   unsigned start;
   unsigned p_start;
   unsigned p_end;

   uint repeat_first:1;
   uint close_first:1;
   uint edgeflag_off:1;
};

static INLINE void
util_split_prim_init(struct util_split_prim *s,
                  unsigned mode, unsigned start, unsigned count)
{
   if (mode == PIPE_PRIM_LINE_LOOP) {
      s->mode = PIPE_PRIM_LINE_STRIP;
      s->close_first = 1;
   } else {
      s->mode = mode;
      s->close_first = 0;
   }
   s->start = start;
   s->p_start = start;
   s->p_end = start + count;
   s->edgeflag_off = 0;
   s->repeat_first = 0;
}

static INLINE boolean
util_split_prim_next(struct util_split_prim *s, unsigned max_verts)
{
   int repeat = 0;

   if (s->repeat_first) {
      s->emit(s->priv, s->start, 1);
      max_verts--;
      if (s->edgeflag_off) {
         s->edge(s->priv, TRUE);
         s->edgeflag_off = FALSE;
      }
   }

   if ((s->p_end - s->p_start) + s->close_first <= max_verts) {
      s->emit(s->priv, s->p_start, s->p_end - s->p_start);
      if (s->close_first)
         s->emit(s->priv, s->start, 1);
      return TRUE;
   }

   switch (s->mode) {
   case PIPE_PRIM_LINES:
      max_verts &= ~1;
      break;
   case PIPE_PRIM_LINE_STRIP:
      repeat = 1;
      break;
   case PIPE_PRIM_POLYGON:
      max_verts--;
      s->emit(s->priv, s->p_start, max_verts);
      s->edge(s->priv, FALSE);
      s->emit(s->priv, s->p_start + max_verts, 1);
      s->p_start += max_verts;
      s->repeat_first = TRUE;
      s->edgeflag_off = TRUE;
      return FALSE;
   case PIPE_PRIM_TRIANGLES:
      max_verts = max_verts - (max_verts % 3);
      break;
   case PIPE_PRIM_TRIANGLE_STRIP:
      /* to ensure winding stays correct, always split
       * on an even number of generated triangles
       */
      max_verts = max_verts & ~1;
      repeat = 2;
      break;
   case PIPE_PRIM_TRIANGLE_FAN:
      s->repeat_first = TRUE;
      repeat = 1;
      break;
   case PIPE_PRIM_QUADS:
      max_verts &= ~3;
      break;
   case PIPE_PRIM_QUAD_STRIP:
      max_verts &= ~1;
      repeat = 2;
      break;
   case PIPE_PRIM_POINTS:
      break;
   default:
      /* TODO: implement adjacency primitives */
      assert(0);
   }

   s->emit (s->priv, s->p_start, max_verts);
   s->p_start += (max_verts - repeat);
   return FALSE;
}

#endif /* U_SPLIT_PRIM_H */
