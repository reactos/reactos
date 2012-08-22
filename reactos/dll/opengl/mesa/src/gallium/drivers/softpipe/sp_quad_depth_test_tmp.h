/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/*
 * Template for generating Z test functions
 * Only PIPE_FORMAT_Z16_UNORM supported at this time.
 */


#ifndef NAME
#error "NAME is not defined!"
#endif

#if !defined(OPERATOR) && !defined(ALWAYS)
#error "neither OPERATOR nor ALWAYS is defined!"
#endif


/*
 * NOTE: there's no guarantee that the quads are sequentially side by
 * side.  The fragment shader may have culled some quads, etc.  Sliver
 * triangles may generate non-sequential quads.
 */
static void
NAME(struct quad_stage *qs, 
     struct quad_header *quads[],
     unsigned nr)
{
   unsigned i, pass = 0;
   const unsigned ix = quads[0]->input.x0;
   const unsigned iy = quads[0]->input.y0;
   const float fx = (float) ix;
   const float fy = (float) iy;
   const float dzdx = quads[0]->posCoef->dadx[2];
   const float dzdy = quads[0]->posCoef->dady[2];
   const float z0 = quads[0]->posCoef->a0[2] + dzdx * fx + dzdy * fy;
   struct softpipe_cached_tile *tile;
   ushort (*depth16)[TILE_SIZE];
   ushort init_idepth[4], idepth[4], depth_step;
   const float scale = 65535.0;

   /* compute scaled depth of the four pixels in first quad */
   init_idepth[0] = (ushort)((z0) * scale);
   init_idepth[1] = (ushort)((z0 + dzdx) * scale);
   init_idepth[2] = (ushort)((z0 + dzdy) * scale);
   init_idepth[3] = (ushort)((z0 + dzdx + dzdy) * scale);

   depth_step = (ushort)(dzdx * scale);

   tile = sp_get_cached_tile(qs->softpipe->zsbuf_cache, ix, iy);

   for (i = 0; i < nr; i++) {
      const unsigned outmask = quads[i]->inout.mask;
      const int dx = quads[i]->input.x0 - ix;
      unsigned mask = 0;
      
      /* compute depth for this quad */
      idepth[0] = init_idepth[0] + dx * depth_step;
      idepth[1] = init_idepth[1] + dx * depth_step;
      idepth[2] = init_idepth[2] + dx * depth_step;
      idepth[3] = init_idepth[3] + dx * depth_step;

      depth16 = (ushort (*)[TILE_SIZE])
         &tile->data.depth16[iy % TILE_SIZE][(ix + dx)% TILE_SIZE];

#ifdef ALWAYS
      if (outmask & 1) {
         depth16[0][0] = idepth[0];
         mask |= (1 << 0);
      }

      if (outmask & 2) {
         depth16[0][1] = idepth[1];
         mask |= (1 << 1);
      }

      if (outmask & 4) {
         depth16[1][0] = idepth[2];
         mask |= (1 << 2);
      }

      if (outmask & 8) {
         depth16[1][1] = idepth[3];
         mask |= (1 << 3);
      }
#else
      /* Note: OPERATOR appears here: */
      if ((outmask & 1) && (idepth[0] OPERATOR depth16[0][0])) {
         depth16[0][0] = idepth[0];
         mask |= (1 << 0);
      }

      if ((outmask & 2) && (idepth[1] OPERATOR depth16[0][1])) {
         depth16[0][1] = idepth[1];
         mask |= (1 << 1);
      }

      if ((outmask & 4) && (idepth[2] OPERATOR depth16[1][0])) {
         depth16[1][0] = idepth[2];
         mask |= (1 << 2);
      }

      if ((outmask & 8) && (idepth[3] OPERATOR depth16[1][1])) {
         depth16[1][1] = idepth[3];
         mask |= (1 << 3);
      }
#endif

      depth16 = (ushort (*)[TILE_SIZE]) &depth16[0][2];

      quads[i]->inout.mask = mask;
      if (quads[i]->inout.mask)
         quads[pass++] = quads[i];
   }

   if (pass)
      qs->next->run(qs->next, quads, pass);
}


#undef NAME
#undef OPERATOR
#undef ALWAYS
