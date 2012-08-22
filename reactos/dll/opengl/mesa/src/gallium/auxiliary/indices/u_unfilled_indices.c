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


static void translate_ubyte_ushort( const void *in,
                                    unsigned nr,
                                    void *out )
{
   const ubyte *in_ub = (const ubyte *)in;
   ushort *out_us = (ushort *)out;
   unsigned i;
   for (i = 0; i < nr; i++)
      out_us[i] = (ushort) in_ub[i];
}

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


static void generate_linear_ushort( unsigned nr,
                                    void *out )
{
   ushort *out_us = (ushort *)out;
   unsigned i;
   for (i = 0; i < nr; i++)
      out_us[i] = (ushort) i;
}
                              
static void generate_linear_uint( unsigned nr,
                                  void *out )
{
   unsigned *out_ui = (unsigned *)out;
   unsigned i;
   for (i = 0; i < nr; i++)
      out_ui[i] = i;
}


/**
 * Given a primitive type and number of vertices, return the number of vertices
 * needed to draw the primitive with fill mode = PIPE_POLYGON_MODE_LINE using
 * separate lines (PIPE_PRIM_LINES).
 */
static unsigned nr_lines( unsigned prim,
                          unsigned nr )
{
   switch (prim) {
   case PIPE_PRIM_TRIANGLES:
      return (nr / 3) * 6; 
   case PIPE_PRIM_TRIANGLE_STRIP:
      return (nr - 2) * 6;
   case PIPE_PRIM_TRIANGLE_FAN:
      return (nr - 2)  * 6;
   case PIPE_PRIM_QUADS:
      return (nr / 4) * 8;
   case PIPE_PRIM_QUAD_STRIP:
      return (nr - 2) / 2 * 8;
   case PIPE_PRIM_POLYGON:
      return 2 * nr; /* a line (two verts) for each polygon edge */
   default:
      assert(0);
      return 0;
   }
}
                              


int u_unfilled_translator( unsigned prim,
                        unsigned in_index_size,
                        unsigned nr,
                        unsigned unfilled_mode,
                        unsigned *out_prim,
                        unsigned *out_index_size,
                        unsigned *out_nr,
                        u_translate_func *out_translate )
{
   unsigned in_idx;
   unsigned out_idx;

   u_unfilled_init();

   in_idx = in_size_idx(in_index_size);
   *out_index_size = (in_index_size == 4) ? 4 : 2;
   out_idx = out_size_idx(*out_index_size);

   if (unfilled_mode == PIPE_POLYGON_MODE_POINT) 
   {
      *out_prim = PIPE_PRIM_POINTS;
      *out_nr = nr;

      switch (in_index_size)
      {
      case 1:
         *out_translate = translate_ubyte_ushort;
         return U_TRANSLATE_NORMAL;
      case 2:
         *out_translate = translate_memcpy_uint;
         return U_TRANSLATE_MEMCPY;
      case 4:
         *out_translate = translate_memcpy_ushort;
         return U_TRANSLATE_MEMCPY;
      default:
         *out_translate = translate_memcpy_uint;
         *out_nr = 0;
         assert(0);
         return U_TRANSLATE_ERROR;
      }
   }
   else {
      assert(unfilled_mode == PIPE_POLYGON_MODE_LINE);
      *out_prim = PIPE_PRIM_LINES;
      *out_translate = translate_line[in_idx][out_idx][prim];
      *out_nr = nr_lines( prim, nr );
      return U_TRANSLATE_NORMAL;
   }
}



int u_unfilled_generator( unsigned prim,
                          unsigned start,
                          unsigned nr,
                          unsigned unfilled_mode,
                          unsigned *out_prim,
                          unsigned *out_index_size,
                          unsigned *out_nr,
                          u_generate_func *out_generate )
{
   unsigned out_idx;

   u_unfilled_init();

   *out_index_size = ((start + nr) > 0xfffe) ? 4 : 2;
   out_idx = out_size_idx(*out_index_size);

   if (unfilled_mode == PIPE_POLYGON_MODE_POINT) {

      if (*out_index_size == 4)
         *out_generate = generate_linear_uint;
      else
         *out_generate = generate_linear_ushort;

      *out_prim = PIPE_PRIM_POINTS;
      *out_nr = nr;
      return U_GENERATE_LINEAR;
   }
   else {
      assert(unfilled_mode == PIPE_POLYGON_MODE_LINE);
      *out_prim = PIPE_PRIM_LINES;
      *out_generate = generate_line[out_idx][prim];
      *out_nr = nr_lines( prim, nr );

      return U_GENERATE_REUSABLE;
   }
}

