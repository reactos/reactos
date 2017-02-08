/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

static void
FUNC(FUNC_VARS)
{
   unsigned first, incr;
   LOCAL_VARS

   /*
    * prim, start, count, and max_count_{simple,loop,fan} should have been
    * defined
    */
   if (0) {
      debug_printf("%s: prim 0x%x, start %d, count %d, max_count_simple %d, "
                   "max_count_loop %d, max_count_fan %d\n",
                   __FUNCTION__, prim, start, count, max_count_simple,
                   max_count_loop, max_count_fan);
   }

   draw_pt_split_prim(prim, &first, &incr);
   /* sanitize primitive length */
   count = draw_pt_trim_count(count, first, incr);
   if (count < first)
      return;

   /* try flushing the entire primitive */
   if (PRIMITIVE(start, count))
      return;

   /* must be able to at least flush two complete primitives */
   assert(max_count_simple >= first + incr &&
          max_count_loop >= first + incr &&
          max_count_fan >= first + incr);

   /* no splitting required */
   if (count <= max_count_simple) {
      SEGMENT_SIMPLE(0x0, start, count);
   }
   else {
      const unsigned rollback = first - incr;
      unsigned flags = DRAW_SPLIT_AFTER, seg_start = 0, seg_max;

      /*
       * Both count and seg_max below are explicitly trimmed.  Because
       *
       *   seg_start = N * (seg_max - rollback) = N' * incr,
       *
       * we have
       *
       *   remaining = count - seg_start = first + N'' * incr.
       *
       * That is, remaining is implicitly trimmed.
       */
      switch (prim) {
      case PIPE_PRIM_POINTS:
      case PIPE_PRIM_LINES:
      case PIPE_PRIM_LINE_STRIP:
      case PIPE_PRIM_TRIANGLES:
      case PIPE_PRIM_TRIANGLE_STRIP:
      case PIPE_PRIM_QUADS:
      case PIPE_PRIM_QUAD_STRIP:
      case PIPE_PRIM_LINES_ADJACENCY:
      case PIPE_PRIM_LINE_STRIP_ADJACENCY:
      case PIPE_PRIM_TRIANGLES_ADJACENCY:
      case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
         seg_max =
            draw_pt_trim_count(MIN2(max_count_simple, count), first, incr);
         if (prim == PIPE_PRIM_TRIANGLE_STRIP ||
             prim == PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY) {
            /* make sure we flush even number of triangles at a time */
            if (seg_max < count && !(((seg_max - first) / incr) & 1))
               seg_max -= incr;
         }

         do {
            const unsigned remaining = count - seg_start;

            if (remaining > seg_max) {
               SEGMENT_SIMPLE(flags, start + seg_start, seg_max);
               seg_start += seg_max - rollback;

               flags |= DRAW_SPLIT_BEFORE;
            }
            else {
               flags &= ~DRAW_SPLIT_AFTER;

               SEGMENT_SIMPLE(flags, start + seg_start, remaining);
               seg_start += remaining;
            }
         } while (seg_start < count);
         break;

      case PIPE_PRIM_LINE_LOOP:
         seg_max =
            draw_pt_trim_count(MIN2(max_count_loop, count), first, incr);

         do {
            const unsigned remaining = count - seg_start;

            if (remaining > seg_max) {
               SEGMENT_LOOP(flags, start + seg_start, seg_max, start);
               seg_start += seg_max - rollback;

               flags |= DRAW_SPLIT_BEFORE;
            }
            else {
               flags &= ~DRAW_SPLIT_AFTER;

               SEGMENT_LOOP(flags, start + seg_start, remaining, start);
               seg_start += remaining;
            }
         } while (seg_start < count);
         break;

      case PIPE_PRIM_TRIANGLE_FAN:
      case PIPE_PRIM_POLYGON:
         seg_max =
            draw_pt_trim_count(MIN2(max_count_fan, count), first, incr);

         do {
            const unsigned remaining = count - seg_start;

            if (remaining > seg_max) {
               SEGMENT_FAN(flags, start + seg_start, seg_max, start);
               seg_start += seg_max - rollback;

               flags |= DRAW_SPLIT_BEFORE;
            }
            else {
               flags &= ~DRAW_SPLIT_AFTER;

               SEGMENT_FAN(flags, start + seg_start, remaining, start);
               seg_start += remaining;
            }
         } while (seg_start < count);
         break;

      default:
         assert(0);
         break;
      }
   }
}

#undef FUNC
#undef FUNC_VARS
#undef LOCAL_VARS

#undef PRIMITIVE
#undef SEGMENT_SIMPLE
#undef SEGMENT_LOOP
#undef SEGMENT_FAN
