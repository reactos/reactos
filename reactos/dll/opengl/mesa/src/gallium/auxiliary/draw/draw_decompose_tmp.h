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
 *
 * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
 *    Chia-I Wu <olv@lunarg.com>
 */

/* these macros are optional */
#ifndef LOCAL_VARS
#define LOCAL_VARS
#endif
#ifndef FUNC_ENTER
#define FUNC_ENTER do {} while (0)
#endif
#ifndef FUNC_EXIT
#define FUNC_EXIT do {} while (0)
#endif
#ifndef LINE_ADJ
#define LINE_ADJ(flags, a0, i0, i1, a1) LINE(flags, i0, i1)
#endif
#ifndef TRIANGLE_ADJ
#define TRIANGLE_ADJ(flags, i0, a0, i1, a1, i2, a2) TRIANGLE(flags, i0, i1, i2)
#endif

static void
FUNC(FUNC_VARS)
{
   unsigned idx[6], i;
   ushort flags;
   LOCAL_VARS

   FUNC_ENTER;

   /* prim, prim_flags, count, and last_vertex_last should have been defined */
   if (0) {
      debug_printf("%s: prim 0x%x, prim_flags 0x%x, count %d, last_vertex_last %d\n",
            __FUNCTION__, prim, prim_flags, count, last_vertex_last);
   }

   switch (prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i++) {
         idx[0] = GET_ELT(i);
         POINT(idx[0]);
      }
      break;

   case PIPE_PRIM_LINES:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 0; i + 1 < count; i += 2) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         LINE(flags, idx[0], idx[1]);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
   case PIPE_PRIM_LINE_STRIP:
      if (count >= 2) {
         flags = (prim_flags & DRAW_SPLIT_BEFORE) ? 0 : DRAW_PIPE_RESET_STIPPLE;
         idx[1] = GET_ELT(0);
         idx[2] = idx[1];

         for (i = 1; i < count; i++, flags = 0) {
            idx[0] = idx[1];
            idx[1] = GET_ELT(i);
            LINE(flags, idx[0], idx[1]);
         }
         /* close the loop */
         if (prim == PIPE_PRIM_LINE_LOOP && !prim_flags)
            LINE(flags, idx[1], idx[2]);
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
      for (i = 0; i + 2 < count; i += 3) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         idx[2] = GET_ELT(i + 2);
         TRIANGLE(flags, idx[0], idx[1], idx[2]);
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (count >= 3) {
         flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
         idx[1] = GET_ELT(0);
         idx[2] = GET_ELT(1);

         if (last_vertex_last) {
            for (i = 0; i + 2 < count; i++) {
               idx[0] = idx[1];
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[2] last */
               if (i & 1)
                  TRIANGLE(flags, idx[1], idx[0], idx[2]);
               else
                  TRIANGLE(flags, idx[0], idx[1], idx[2]);
            }
         }
         else {
            for (i = 0; i + 2 < count; i++) {
               idx[0] = idx[1];
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[0] first */
               if (i & 1)
                  TRIANGLE(flags, idx[0], idx[2], idx[1]);
               else
                  TRIANGLE(flags, idx[0], idx[1], idx[2]);
            }
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
         idx[0] = GET_ELT(0);
         idx[2] = GET_ELT(1);

         /* idx[0] is neither the first nor the last vertex */
         if (last_vertex_last) {
            for (i = 0; i + 2 < count; i++) {
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[2] last */
               TRIANGLE(flags, idx[0], idx[1], idx[2]);
            }
         }
         else {
            for (i = 0; i + 2 < count; i++) {
               idx[1] = idx[2];
               idx[2] = GET_ELT(i + 2);
               /* always emit idx[1] first */
               TRIANGLE(flags, idx[1], idx[2], idx[0]);
            }
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      if (last_vertex_last) {
         for (i = 0; i + 3 < count; i += 4) {
            idx[0] = GET_ELT(i);
            idx[1] = GET_ELT(i + 1);
            idx[2] = GET_ELT(i + 2);
            idx[3] = GET_ELT(i + 3);

            flags = DRAW_PIPE_RESET_STIPPLE |
                    DRAW_PIPE_EDGE_FLAG_0 |
                    DRAW_PIPE_EDGE_FLAG_2;
            /* always emit idx[3] last */
            TRIANGLE(flags, idx[0], idx[1], idx[3]);

            flags = DRAW_PIPE_EDGE_FLAG_0 |
                    DRAW_PIPE_EDGE_FLAG_1;
            TRIANGLE(flags, idx[1], idx[2], idx[3]);
         }
      }
      else {
         for (i = 0; i + 3 < count; i += 4) {
            idx[0] = GET_ELT(i);
            idx[1] = GET_ELT(i + 1);
            idx[2] = GET_ELT(i + 2);
            idx[3] = GET_ELT(i + 3);

            flags = DRAW_PIPE_RESET_STIPPLE |
                    DRAW_PIPE_EDGE_FLAG_0 |
                    DRAW_PIPE_EDGE_FLAG_1;
            /* XXX should always emit idx[0] first */
            /* always emit idx[3] first */
            TRIANGLE(flags, idx[3], idx[0], idx[1]);

            flags = DRAW_PIPE_EDGE_FLAG_1 |
                    DRAW_PIPE_EDGE_FLAG_2;
            TRIANGLE(flags, idx[3], idx[1], idx[2]);
         }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (count >= 4) {
         idx[2] = GET_ELT(0);
         idx[3] = GET_ELT(1);

         if (last_vertex_last) {
            for (i = 0; i + 3 < count; i += 2) {
               idx[0] = idx[2];
               idx[1] = idx[3];
               idx[2] = GET_ELT(i + 2);
               idx[3] = GET_ELT(i + 3);

               /* always emit idx[3] last */
               flags = DRAW_PIPE_RESET_STIPPLE |
                       DRAW_PIPE_EDGE_FLAG_0 |
                       DRAW_PIPE_EDGE_FLAG_2;
               TRIANGLE(flags, idx[2], idx[0], idx[3]);

               flags = DRAW_PIPE_EDGE_FLAG_0 |
                       DRAW_PIPE_EDGE_FLAG_1;
               TRIANGLE(flags, idx[0], idx[1], idx[3]);
            }
         }
         else {
            for (i = 0; i + 3 < count; i += 2) {
               idx[0] = idx[2];
               idx[1] = idx[3];
               idx[2] = GET_ELT(i + 2);
               idx[3] = GET_ELT(i + 3);

               flags = DRAW_PIPE_RESET_STIPPLE |
                       DRAW_PIPE_EDGE_FLAG_0 |
                       DRAW_PIPE_EDGE_FLAG_1;
               /* XXX should always emit idx[0] first */
               /* always emit idx[3] first */
               TRIANGLE(flags, idx[3], idx[2], idx[0]);

               flags = DRAW_PIPE_EDGE_FLAG_1 |
                       DRAW_PIPE_EDGE_FLAG_2;
               TRIANGLE(flags, idx[3], idx[0], idx[1]);
            }
         }
      }
      break;

   case PIPE_PRIM_POLYGON:
      if (count >= 3) {
         ushort edge_next, edge_finish;

         if (last_vertex_last) {
            flags = (DRAW_PIPE_RESET_STIPPLE |
                     DRAW_PIPE_EDGE_FLAG_0);
            if (!(prim_flags & DRAW_SPLIT_BEFORE))
               flags |= DRAW_PIPE_EDGE_FLAG_2;

            edge_next = DRAW_PIPE_EDGE_FLAG_0;
            edge_finish =
               (prim_flags & DRAW_SPLIT_AFTER) ? 0 : DRAW_PIPE_EDGE_FLAG_1;
         }
         else {
            flags = (DRAW_PIPE_RESET_STIPPLE |
                     DRAW_PIPE_EDGE_FLAG_1);
            if (!(prim_flags & DRAW_SPLIT_BEFORE))
               flags |= DRAW_PIPE_EDGE_FLAG_0;

            edge_next = DRAW_PIPE_EDGE_FLAG_1;
            edge_finish =
               (prim_flags & DRAW_SPLIT_AFTER) ? 0 : DRAW_PIPE_EDGE_FLAG_2;
         }

         idx[0] = GET_ELT(0);
         idx[2] = GET_ELT(1);

         for (i = 0; i + 2 < count; i++, flags = edge_next) {
            idx[1] = idx[2];
            idx[2] = GET_ELT(i + 2);

            if (i + 3 == count)
               flags |= edge_finish;

            /* idx[0] is both the first and the last vertex */
            if (last_vertex_last)
               TRIANGLE(flags, idx[1], idx[2], idx[0]);
            else
               TRIANGLE(flags, idx[0], idx[1], idx[2]);
         }
      }
      break;

   case PIPE_PRIM_LINES_ADJACENCY:
      flags = DRAW_PIPE_RESET_STIPPLE;
      for (i = 0; i + 3 < count; i += 4) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         idx[2] = GET_ELT(i + 2);
         idx[3] = GET_ELT(i + 3);
         LINE_ADJ(flags, idx[0], idx[1], idx[2], idx[3]);
      }
      break;

   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
      if (count >= 4) {
         flags = (prim_flags & DRAW_SPLIT_BEFORE) ? 0 : DRAW_PIPE_RESET_STIPPLE;
         idx[1] = GET_ELT(0);
         idx[2] = GET_ELT(1);
         idx[3] = GET_ELT(2);

         for (i = 1; i + 2 < count; i++, flags = 0) {
            idx[0] = idx[1];
            idx[1] = idx[2];
            idx[2] = idx[3];
            idx[3] = GET_ELT(i + 2);
            LINE_ADJ(flags, idx[0], idx[1], idx[2], idx[3]);
         }
      }
      break;

   case PIPE_PRIM_TRIANGLES_ADJACENCY:
      flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
      for (i = 0; i + 5 < count; i += 6) {
         idx[0] = GET_ELT(i);
         idx[1] = GET_ELT(i + 1);
         idx[2] = GET_ELT(i + 2);
         idx[3] = GET_ELT(i + 3);
         idx[4] = GET_ELT(i + 4);
         idx[5] = GET_ELT(i + 5);
         TRIANGLE_ADJ(flags, idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      if (count >= 6) {
         flags = DRAW_PIPE_RESET_STIPPLE | DRAW_PIPE_EDGE_FLAG_ALL;
         idx[0] = GET_ELT(1);
         idx[2] = GET_ELT(0);
         idx[4] = GET_ELT(2);
         idx[3] = GET_ELT(4);

         /*
          * The vertices of the i-th triangle are stored in
          * idx[0,2,4] = { 2*i, 2*i+2, 2*i+4 };
          *
          * The adjacent vertices are stored in
          * idx[1,3,5] = { 2*i-2, 2*i+6, 2*i+3 }.
          *
          * However, there are two exceptions:
          *
          * For the first triangle, idx[1] = 1;
          * For the  last triangle, idx[3] = 2*i+5.
          */
         if (last_vertex_last) {
            for (i = 0; i + 5 < count; i += 2) {
               idx[1] = idx[0];

               idx[0] = idx[2];
               idx[2] = idx[4];
               idx[4] = idx[3];

               idx[3] = GET_ELT(i + ((i + 7 < count) ? 6 : 5));
               idx[5] = GET_ELT(i + 3);

               /*
                * alternate the first two vertices (idx[0] and idx[2]) and the
                * corresponding adjacent vertices (idx[3] and idx[5]) to have
                * the correct orientation
                */
               if (i & 2) {
                  TRIANGLE_ADJ(flags,
                        idx[2], idx[1], idx[0], idx[5], idx[4], idx[3]);
               }
               else {
                  TRIANGLE_ADJ(flags,
                        idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);
               }
            }
         }
         else {
            for (i = 0; i + 5 < count; i += 2) {
               idx[1] = idx[0];

               idx[0] = idx[2];
               idx[2] = idx[4];
               idx[4] = idx[3];

               idx[3] = GET_ELT(i + ((i + 7 < count) ? 6 : 5));
               idx[5] = GET_ELT(i + 3);

               /*
                * alternate the last two vertices (idx[2] and idx[4]) and the
                * corresponding adjacent vertices (idx[1] and idx[5]) to have
                * the correct orientation
                */
               if (i & 2) {
                  TRIANGLE_ADJ(flags,
                        idx[0], idx[5], idx[4], idx[3], idx[2], idx[1]);
               }
               else {
                  TRIANGLE_ADJ(flags,
                        idx[0], idx[1], idx[2], idx[3], idx[4], idx[5]);
               }
            }
         }
      }
      break;

   default:
      assert(0);
      break;
   }

   FUNC_EXIT;
}

#undef LOCAL_VARS
#undef FUNC_ENTER
#undef FUNC_EXIT
#undef LINE_ADJ
#undef TRIANGLE_ADJ

#undef FUNC
#undef FUNC_VARS
#undef GET_ELT
#undef POINT
#undef LINE
#undef TRIANGLE
