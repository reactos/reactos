/*
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "u_indices.h"
#include "u_indices_priv.h"

static void translate_memcpy_ushort( const void *in,
                                     unsigned nr,
                                     void *out )
{
   memcpy(out, in, nr*sizeof(short));
}
                              
static void translate_memcpy_uint( const void *in,
                                   unsigned nr,
                                   void *out )
{
   memcpy(out, in, nr*sizeof(int));
}
                              

int u_index_translator( unsigned hw_mask,
                        unsigned prim,
                        unsigned in_index_size,
                        unsigned nr,
                        unsigned in_pv,
                        unsigned out_pv,
                        unsigned *out_prim,
                        unsigned *out_index_size,
                        unsigned *out_nr,
                        u_translate_func *out_translate )
{
   unsigned in_idx;
   unsigned out_idx;
   int ret = U_TRANSLATE_NORMAL;

   u_index_init();

   in_idx = in_size_idx(in_index_size);
   *out_index_size = (in_index_size == 4) ? 4 : 2;
   out_idx = out_size_idx(*out_index_size);

   if ((hw_mask & (1<<prim)) && 
       in_index_size == *out_index_size &&
       in_pv == out_pv) 
   {
      if (in_index_size == 4)
         *out_translate = translate_memcpy_uint;
      else
         *out_translate = translate_memcpy_ushort;

      *out_prim = prim;
      *out_nr = nr;

      return U_TRANSLATE_MEMCPY;
   }
   else {
      switch (prim) {
      case PIPE_PRIM_POINTS:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_POINTS;
         *out_nr = nr;
         break;

      case PIPE_PRIM_LINES:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_LINES;
         *out_nr = nr;
         break;

      case PIPE_PRIM_LINE_STRIP:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_LINES;
         *out_nr = (nr - 1) * 2;
         break;

      case PIPE_PRIM_LINE_LOOP:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_LINES;
         *out_nr = nr * 2;
         break;

      case PIPE_PRIM_TRIANGLES:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = nr;
         break;

      case PIPE_PRIM_TRIANGLE_STRIP:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         break;

      case PIPE_PRIM_TRIANGLE_FAN:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         break;

      case PIPE_PRIM_QUADS:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr / 4) * 6;
         break;

      case PIPE_PRIM_QUAD_STRIP:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         break;

      case PIPE_PRIM_POLYGON:
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         break;

      default:
         assert(0);
         *out_translate = translate[in_idx][out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_POINTS;
         *out_nr = nr;
         return U_TRANSLATE_ERROR;
      }
   }

   return ret;
}





int u_index_generator( unsigned hw_mask,
                       unsigned prim,
                       unsigned start,
                       unsigned nr,
                       unsigned in_pv,
                       unsigned out_pv,
                       unsigned *out_prim,
                       unsigned *out_index_size,
                       unsigned *out_nr,
                       u_generate_func *out_generate )

{
   unsigned out_idx;

   u_index_init();

   *out_index_size = ((start + nr) > 0xfffe) ? 4 : 2;
   out_idx = out_size_idx(*out_index_size);

   if ((hw_mask & (1<<prim)) && 
       (in_pv == out_pv)) {
       
      *out_generate = generate[out_idx][in_pv][out_pv][PIPE_PRIM_POINTS];
      *out_prim = prim;
      *out_nr = nr;
      return U_GENERATE_LINEAR;
   }
   else {
      switch (prim) {
      case PIPE_PRIM_POINTS:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_POINTS;
         *out_nr = nr;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_LINES:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_LINES;
         *out_nr = nr;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_LINE_STRIP:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_LINES;
         *out_nr = (nr - 1) * 2;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_LINE_LOOP:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_LINES;
         *out_nr = nr * 2;
         return U_GENERATE_ONE_OFF;

      case PIPE_PRIM_TRIANGLES:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = nr;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_TRIANGLE_STRIP:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_TRIANGLE_FAN:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_QUADS:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr / 4) * 6;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_QUAD_STRIP:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         return U_GENERATE_REUSABLE;

      case PIPE_PRIM_POLYGON:
         *out_generate = generate[out_idx][in_pv][out_pv][prim];
         *out_prim = PIPE_PRIM_TRIANGLES;
         *out_nr = (nr - 2) * 3;
         return U_GENERATE_REUSABLE;

      default:
         assert(0);
         *out_generate = generate[out_idx][in_pv][out_pv][PIPE_PRIM_POINTS];
         *out_prim = PIPE_PRIM_POINTS;
         *out_nr = nr;
         return U_TRANSLATE_ERROR;
      }
   }
}
