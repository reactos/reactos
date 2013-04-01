/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008  VMware, Inc.  All rights reserved.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Mipmap generation utility
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "util/u_debug.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_draw_quad.h"
#include "util/u_gen_mipmap.h"
#include "util/u_simple_shaders.h"
#include "util/u_math.h"
#include "util/u_texture.h"
#include "util/u_half.h"
#include "util/u_surface.h"

#include "cso_cache/cso_context.h"


struct gen_mipmap_state
{
   struct pipe_context *pipe;
   struct cso_context *cso;

   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;
   struct pipe_vertex_element velem[2];

   void *vs;
   void *fs[TGSI_TEXTURE_COUNT]; /**< Not all are used, but simplifies code */

   struct pipe_resource *vbuf;  /**< quad vertices */
   unsigned vbuf_slot;

   float vertices[4][2][4];   /**< vertex/texcoords for quad */
};



enum dtype
{
   DTYPE_UBYTE,
   DTYPE_UBYTE_3_3_2,
   DTYPE_USHORT,
   DTYPE_USHORT_4_4_4_4,
   DTYPE_USHORT_5_6_5,
   DTYPE_USHORT_1_5_5_5_REV,
   DTYPE_UINT,
   DTYPE_FLOAT,
   DTYPE_HALF_FLOAT
};


typedef uint16_t half_float;


/**
 * \name Support macros for do_row and do_row_3d
 *
 * The macro madness is here for two reasons.  First, it compacts the code
 * slightly.  Second, it makes it much easier to adjust the specifics of the
 * filter to tune the rounding characteristics.
 */
/*@{*/
#define DECLARE_ROW_POINTERS(t, e) \
      const t(*rowA)[e] = (const t(*)[e]) srcRowA; \
      const t(*rowB)[e] = (const t(*)[e]) srcRowB; \
      const t(*rowC)[e] = (const t(*)[e]) srcRowC; \
      const t(*rowD)[e] = (const t(*)[e]) srcRowD; \
      t(*dst)[e] = (t(*)[e]) dstRow

#define DECLARE_ROW_POINTERS0(t) \
      const t *rowA = (const t *) srcRowA; \
      const t *rowB = (const t *) srcRowB; \
      const t *rowC = (const t *) srcRowC; \
      const t *rowD = (const t *) srcRowD; \
      t *dst = (t *) dstRow

#define FILTER_SUM_3D(Aj, Ak, Bj, Bk, Cj, Ck, Dj, Dk) \
   ((unsigned) Aj + (unsigned) Ak \
    + (unsigned) Bj + (unsigned) Bk \
    + (unsigned) Cj + (unsigned) Ck \
    + (unsigned) Dj + (unsigned) Dk \
    + 4) >> 3

#define FILTER_3D(e) \
   do { \
      dst[i][e] = FILTER_SUM_3D(rowA[j][e], rowA[k][e], \
                                rowB[j][e], rowB[k][e], \
                                rowC[j][e], rowC[k][e], \
                                rowD[j][e], rowD[k][e]); \
   } while(0)

#define FILTER_F_3D(e) \
   do { \
      dst[i][e] = (rowA[j][e] + rowA[k][e] \
                   + rowB[j][e] + rowB[k][e] \
                   + rowC[j][e] + rowC[k][e] \
                   + rowD[j][e] + rowD[k][e]) * 0.125F; \
   } while(0)

#define FILTER_HF_3D(e) \
   do { \
      const float aj = util_half_to_float(rowA[j][e]); \
      const float ak = util_half_to_float(rowA[k][e]); \
      const float bj = util_half_to_float(rowB[j][e]); \
      const float bk = util_half_to_float(rowB[k][e]); \
      const float cj = util_half_to_float(rowC[j][e]); \
      const float ck = util_half_to_float(rowC[k][e]); \
      const float dj = util_half_to_float(rowD[j][e]); \
      const float dk = util_half_to_float(rowD[k][e]); \
      dst[i][e] = util_float_to_half((aj + ak + bj + bk + cj + ck + dj + dk) \
                                      * 0.125F); \
   } while(0)
/*@}*/


/**
 * Average together two rows of a source image to produce a single new
 * row in the dest image.  It's legal for the two source rows to point
 * to the same data.  The source width must be equal to either the
 * dest width or two times the dest width.
 * \param datatype  GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT, etc.
 * \param comps  number of components per pixel (1..4)
 */
static void
do_row(enum dtype datatype, uint comps, int srcWidth,
       const void *srcRowA, const void *srcRowB,
       int dstWidth, void *dstRow)
{
   const uint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const uint colStride = (srcWidth == dstWidth) ? 1 : 2;

   assert(comps >= 1);
   assert(comps <= 4);

   /* This assertion is no longer valid with non-power-of-2 textures
   assert(srcWidth == dstWidth || srcWidth == 2 * dstWidth);
   */

   if (datatype == DTYPE_UBYTE && comps == 4) {
      uint i, j, k;
      const ubyte(*rowA)[4] = (const ubyte(*)[4]) srcRowA;
      const ubyte(*rowB)[4] = (const ubyte(*)[4]) srcRowB;
      ubyte(*dst)[4] = (ubyte(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == DTYPE_UBYTE && comps == 3) {
      uint i, j, k;
      const ubyte(*rowA)[3] = (const ubyte(*)[3]) srcRowA;
      const ubyte(*rowB)[3] = (const ubyte(*)[3]) srcRowB;
      ubyte(*dst)[3] = (ubyte(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == DTYPE_UBYTE && comps == 2) {
      uint i, j, k;
      const ubyte(*rowA)[2] = (const ubyte(*)[2]) srcRowA;
      const ubyte(*rowB)[2] = (const ubyte(*)[2]) srcRowB;
      ubyte(*dst)[2] = (ubyte(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) >> 2;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) >> 2;
      }
   }
   else if (datatype == DTYPE_UBYTE && comps == 1) {
      uint i, j, k;
      const ubyte *rowA = (const ubyte *) srcRowA;
      const ubyte *rowB = (const ubyte *) srcRowB;
      ubyte *dst = (ubyte *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) >> 2;
      }
   }

   else if (datatype == DTYPE_USHORT && comps == 4) {
      uint i, j, k;
      const ushort(*rowA)[4] = (const ushort(*)[4]) srcRowA;
      const ushort(*rowB)[4] = (const ushort(*)[4]) srcRowB;
      ushort(*dst)[4] = (ushort(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == DTYPE_USHORT && comps == 3) {
      uint i, j, k;
      const ushort(*rowA)[3] = (const ushort(*)[3]) srcRowA;
      const ushort(*rowB)[3] = (const ushort(*)[3]) srcRowB;
      ushort(*dst)[3] = (ushort(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == DTYPE_USHORT && comps == 2) {
      uint i, j, k;
      const ushort(*rowA)[2] = (const ushort(*)[2]) srcRowA;
      const ushort(*rowB)[2] = (const ushort(*)[2]) srcRowB;
      ushort(*dst)[2] = (ushort(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
      }
   }
   else if (datatype == DTYPE_USHORT && comps == 1) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
      }
   }

   else if (datatype == DTYPE_FLOAT && comps == 4) {
      uint i, j, k;
      const float(*rowA)[4] = (const float(*)[4]) srcRowA;
      const float(*rowB)[4] = (const float(*)[4]) srcRowB;
      float(*dst)[4] = (float(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
         dst[i][2] = (rowA[j][2] + rowA[k][2] +
                      rowB[j][2] + rowB[k][2]) * 0.25F;
         dst[i][3] = (rowA[j][3] + rowA[k][3] +
                      rowB[j][3] + rowB[k][3]) * 0.25F;
      }
   }
   else if (datatype == DTYPE_FLOAT && comps == 3) {
      uint i, j, k;
      const float(*rowA)[3] = (const float(*)[3]) srcRowA;
      const float(*rowB)[3] = (const float(*)[3]) srcRowB;
      float(*dst)[3] = (float(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
         dst[i][2] = (rowA[j][2] + rowA[k][2] +
                      rowB[j][2] + rowB[k][2]) * 0.25F;
      }
   }
   else if (datatype == DTYPE_FLOAT && comps == 2) {
      uint i, j, k;
      const float(*rowA)[2] = (const float(*)[2]) srcRowA;
      const float(*rowB)[2] = (const float(*)[2]) srcRowB;
      float(*dst)[2] = (float(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
      }
   }
   else if (datatype == DTYPE_FLOAT && comps == 1) {
      uint i, j, k;
      const float *rowA = (const float *) srcRowA;
      const float *rowB = (const float *) srcRowB;
      float *dst = (float *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) * 0.25F;
      }
   }

   else if (datatype == DTYPE_HALF_FLOAT && comps == 4) {
      uint i, j, k, comp;
      const half_float(*rowA)[4] = (const half_float(*)[4]) srcRowA;
      const half_float(*rowB)[4] = (const half_float(*)[4]) srcRowB;
      half_float(*dst)[4] = (half_float(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 4; comp++) {
            float aj, ak, bj, bk;
            aj = util_half_to_float(rowA[j][comp]);
            ak = util_half_to_float(rowA[k][comp]);
            bj = util_half_to_float(rowB[j][comp]);
            bk = util_half_to_float(rowB[k][comp]);
            dst[i][comp] = util_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == DTYPE_HALF_FLOAT && comps == 3) {
      uint i, j, k, comp;
      const half_float(*rowA)[3] = (const half_float(*)[3]) srcRowA;
      const half_float(*rowB)[3] = (const half_float(*)[3]) srcRowB;
      half_float(*dst)[3] = (half_float(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 3; comp++) {
            float aj, ak, bj, bk;
            aj = util_half_to_float(rowA[j][comp]);
            ak = util_half_to_float(rowA[k][comp]);
            bj = util_half_to_float(rowB[j][comp]);
            bk = util_half_to_float(rowB[k][comp]);
            dst[i][comp] = util_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == DTYPE_HALF_FLOAT && comps == 2) {
      uint i, j, k, comp;
      const half_float(*rowA)[2] = (const half_float(*)[2]) srcRowA;
      const half_float(*rowB)[2] = (const half_float(*)[2]) srcRowB;
      half_float(*dst)[2] = (half_float(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 2; comp++) {
            float aj, ak, bj, bk;
            aj = util_half_to_float(rowA[j][comp]);
            ak = util_half_to_float(rowA[k][comp]);
            bj = util_half_to_float(rowB[j][comp]);
            bk = util_half_to_float(rowB[k][comp]);
            dst[i][comp] = util_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == DTYPE_HALF_FLOAT && comps == 1) {
      uint i, j, k;
      const half_float *rowA = (const half_float *) srcRowA;
      const half_float *rowB = (const half_float *) srcRowB;
      half_float *dst = (half_float *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         float aj, ak, bj, bk;
         aj = util_half_to_float(rowA[j]);
         ak = util_half_to_float(rowA[k]);
         bj = util_half_to_float(rowB[j]);
         bk = util_half_to_float(rowB[k]);
         dst[i] = util_float_to_half((aj + ak + bj + bk) * 0.25F);
      }
   }

   else if (datatype == DTYPE_UINT && comps == 1) {
      uint i, j, k;
      const uint *rowA = (const uint *) srcRowA;
      const uint *rowB = (const uint *) srcRowB;
      uint *dst = (uint *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = rowA[j] / 4 + rowA[k] / 4 + rowB[j] / 4 + rowB[k] / 4;
      }
   }

   else if (datatype == DTYPE_USHORT_5_6_5 && comps == 3) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x1f;
         const int rowAr1 = rowA[k] & 0x1f;
         const int rowBr0 = rowB[j] & 0x1f;
         const int rowBr1 = rowB[k] & 0x1f;
         const int rowAg0 = (rowA[j] >> 5) & 0x3f;
         const int rowAg1 = (rowA[k] >> 5) & 0x3f;
         const int rowBg0 = (rowB[j] >> 5) & 0x3f;
         const int rowBg1 = (rowB[k] >> 5) & 0x3f;
         const int rowAb0 = (rowA[j] >> 11) & 0x1f;
         const int rowAb1 = (rowA[k] >> 11) & 0x1f;
         const int rowBb0 = (rowB[j] >> 11) & 0x1f;
         const int rowBb1 = (rowB[k] >> 11) & 0x1f;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         dst[i] = (blue << 11) | (green << 5) | red;
      }
   }
   else if (datatype == DTYPE_USHORT_4_4_4_4 && comps == 4) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0xf;
         const int rowAr1 = rowA[k] & 0xf;
         const int rowBr0 = rowB[j] & 0xf;
         const int rowBr1 = rowB[k] & 0xf;
         const int rowAg0 = (rowA[j] >> 4) & 0xf;
         const int rowAg1 = (rowA[k] >> 4) & 0xf;
         const int rowBg0 = (rowB[j] >> 4) & 0xf;
         const int rowBg1 = (rowB[k] >> 4) & 0xf;
         const int rowAb0 = (rowA[j] >> 8) & 0xf;
         const int rowAb1 = (rowA[k] >> 8) & 0xf;
         const int rowBb0 = (rowB[j] >> 8) & 0xf;
         const int rowBb1 = (rowB[k] >> 8) & 0xf;
         const int rowAa0 = (rowA[j] >> 12) & 0xf;
         const int rowAa1 = (rowA[k] >> 12) & 0xf;
         const int rowBa0 = (rowB[j] >> 12) & 0xf;
         const int rowBa1 = (rowB[k] >> 12) & 0xf;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const int alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (alpha << 12) | (blue << 8) | (green << 4) | red;
      }
   }
   else if (datatype == DTYPE_USHORT_1_5_5_5_REV && comps == 4) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x1f;
         const int rowAr1 = rowA[k] & 0x1f;
         const int rowBr0 = rowB[j] & 0x1f;
         const int rowBr1 = rowB[k] & 0x1f;
         const int rowAg0 = (rowA[j] >> 5) & 0x1f;
         const int rowAg1 = (rowA[k] >> 5) & 0x1f;
         const int rowBg0 = (rowB[j] >> 5) & 0x1f;
         const int rowBg1 = (rowB[k] >> 5) & 0x1f;
         const int rowAb0 = (rowA[j] >> 10) & 0x1f;
         const int rowAb1 = (rowA[k] >> 10) & 0x1f;
         const int rowBb0 = (rowB[j] >> 10) & 0x1f;
         const int rowBb1 = (rowB[k] >> 10) & 0x1f;
         const int rowAa0 = (rowA[j] >> 15) & 0x1;
         const int rowAa1 = (rowA[k] >> 15) & 0x1;
         const int rowBa0 = (rowB[j] >> 15) & 0x1;
         const int rowBa1 = (rowB[k] >> 15) & 0x1;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const int alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (alpha << 15) | (blue << 10) | (green << 5) | red;
      }
   }
   else if (datatype == DTYPE_UBYTE_3_3_2 && comps == 3) {
      uint i, j, k;
      const ubyte *rowA = (const ubyte *) srcRowA;
      const ubyte *rowB = (const ubyte *) srcRowB;
      ubyte *dst = (ubyte *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x3;
         const int rowAr1 = rowA[k] & 0x3;
         const int rowBr0 = rowB[j] & 0x3;
         const int rowBr1 = rowB[k] & 0x3;
         const int rowAg0 = (rowA[j] >> 2) & 0x7;
         const int rowAg1 = (rowA[k] >> 2) & 0x7;
         const int rowBg0 = (rowB[j] >> 2) & 0x7;
         const int rowBg1 = (rowB[k] >> 2) & 0x7;
         const int rowAb0 = (rowA[j] >> 5) & 0x7;
         const int rowAb1 = (rowA[k] >> 5) & 0x7;
         const int rowBb0 = (rowB[j] >> 5) & 0x7;
         const int rowBb1 = (rowB[k] >> 5) & 0x7;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         dst[i] = (blue << 5) | (green << 2) | red;
      }
   }
   else {
      debug_printf("bad format in do_row()");
   }
}


/**
 * Average together four rows of a source image to produce a single new
 * row in the dest image.  It's legal for the two source rows to point
 * to the same data.  The source width must be equal to either the
 * dest width or two times the dest width.
 *
 * \param datatype  GL pixel type \c GL_UNSIGNED_BYTE, \c GL_UNSIGNED_SHORT,
 *                  \c GL_FLOAT, etc.
 * \param comps     number of components per pixel (1..4)
 * \param srcWidth  Width of a row in the source data
 * \param srcRowA   Pointer to one of the rows of source data
 * \param srcRowB   Pointer to one of the rows of source data
 * \param srcRowC   Pointer to one of the rows of source data
 * \param srcRowD   Pointer to one of the rows of source data
 * \param dstWidth  Width of a row in the destination data
 * \param srcRowA   Pointer to the row of destination data
 */
static void
do_row_3D(enum dtype datatype, uint comps, int srcWidth,
          const void *srcRowA, const void *srcRowB,
          const void *srcRowC, const void *srcRowD,
          int dstWidth, void *dstRow)
{
   const uint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const uint colStride = (srcWidth == dstWidth) ? 1 : 2;
   uint i, j, k;

   assert(comps >= 1);
   assert(comps <= 4);

   if ((datatype == DTYPE_UBYTE) && (comps == 4)) {
      DECLARE_ROW_POINTERS(ubyte, 4);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
         FILTER_3D(3);
      }
   }
   else if ((datatype == DTYPE_UBYTE) && (comps == 3)) {
      DECLARE_ROW_POINTERS(ubyte, 3);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
      }
   }
   else if ((datatype == DTYPE_UBYTE) && (comps == 2)) {
      DECLARE_ROW_POINTERS(ubyte, 2);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
      }
   }
   else if ((datatype == DTYPE_UBYTE) && (comps == 1)) {
      DECLARE_ROW_POINTERS(ubyte, 1);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
      }
   }
   else if ((datatype == DTYPE_USHORT) && (comps == 4)) {
      DECLARE_ROW_POINTERS(ushort, 4);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
         FILTER_3D(3);
      }
   }
   else if ((datatype == DTYPE_USHORT) && (comps == 3)) {
      DECLARE_ROW_POINTERS(ushort, 3);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
      }
   }
   else if ((datatype == DTYPE_USHORT) && (comps == 2)) {
      DECLARE_ROW_POINTERS(ushort, 2);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
      }
   }
   else if ((datatype == DTYPE_USHORT) && (comps == 1)) {
      DECLARE_ROW_POINTERS(ushort, 1);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
      }
   }
   else if ((datatype == DTYPE_FLOAT) && (comps == 4)) {
      DECLARE_ROW_POINTERS(float, 4);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
         FILTER_F_3D(1);
         FILTER_F_3D(2);
         FILTER_F_3D(3);
      }
   }
   else if ((datatype == DTYPE_FLOAT) && (comps == 3)) {
      DECLARE_ROW_POINTERS(float, 3);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
         FILTER_F_3D(1);
         FILTER_F_3D(2);
      }
   }
   else if ((datatype == DTYPE_FLOAT) && (comps == 2)) {
      DECLARE_ROW_POINTERS(float, 2);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
         FILTER_F_3D(1);
      }
   }
   else if ((datatype == DTYPE_FLOAT) && (comps == 1)) {
      DECLARE_ROW_POINTERS(float, 1);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
      }
   }
   else if ((datatype == DTYPE_HALF_FLOAT) && (comps == 4)) {
      DECLARE_ROW_POINTERS(half_float, 4);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
         FILTER_HF_3D(1);
         FILTER_HF_3D(2);
         FILTER_HF_3D(3);
      }
   }
   else if ((datatype == DTYPE_HALF_FLOAT) && (comps == 3)) {
      DECLARE_ROW_POINTERS(half_float, 4);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
         FILTER_HF_3D(1);
         FILTER_HF_3D(2);
      }
   }
   else if ((datatype == DTYPE_HALF_FLOAT) && (comps == 2)) {
      DECLARE_ROW_POINTERS(half_float, 4);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
         FILTER_HF_3D(1);
      }
   }
   else if ((datatype == DTYPE_HALF_FLOAT) && (comps == 1)) {
      DECLARE_ROW_POINTERS(half_float, 4);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
      }
   }
   else if ((datatype == DTYPE_UINT) && (comps == 1)) {
      const uint *rowA = (const uint *) srcRowA;
      const uint *rowB = (const uint *) srcRowB;
      const uint *rowC = (const uint *) srcRowC;
      const uint *rowD = (const uint *) srcRowD;
      float *dst = (float *) dstRow;

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const uint64_t tmp = (((uint64_t) rowA[j] + (uint64_t) rowA[k])
                               + ((uint64_t) rowB[j] + (uint64_t) rowB[k])
                               + ((uint64_t) rowC[j] + (uint64_t) rowC[k])
                               + ((uint64_t) rowD[j] + (uint64_t) rowD[k]));
         dst[i] = (float)((double) tmp * 0.125);
      }
   }
   else if ((datatype == DTYPE_USHORT_5_6_5) && (comps == 3)) {
      DECLARE_ROW_POINTERS0(ushort);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x1f;
         const int rowAr1 = rowA[k] & 0x1f;
         const int rowBr0 = rowB[j] & 0x1f;
         const int rowBr1 = rowB[k] & 0x1f;
         const int rowCr0 = rowC[j] & 0x1f;
         const int rowCr1 = rowC[k] & 0x1f;
         const int rowDr0 = rowD[j] & 0x1f;
         const int rowDr1 = rowD[k] & 0x1f;
         const int rowAg0 = (rowA[j] >> 5) & 0x3f;
         const int rowAg1 = (rowA[k] >> 5) & 0x3f;
         const int rowBg0 = (rowB[j] >> 5) & 0x3f;
         const int rowBg1 = (rowB[k] >> 5) & 0x3f;
         const int rowCg0 = (rowC[j] >> 5) & 0x3f;
         const int rowCg1 = (rowC[k] >> 5) & 0x3f;
         const int rowDg0 = (rowD[j] >> 5) & 0x3f;
         const int rowDg1 = (rowD[k] >> 5) & 0x3f;
         const int rowAb0 = (rowA[j] >> 11) & 0x1f;
         const int rowAb1 = (rowA[k] >> 11) & 0x1f;
         const int rowBb0 = (rowB[j] >> 11) & 0x1f;
         const int rowBb1 = (rowB[k] >> 11) & 0x1f;
         const int rowCb0 = (rowC[j] >> 11) & 0x1f;
         const int rowCb1 = (rowC[k] >> 11) & 0x1f;
         const int rowDb0 = (rowD[j] >> 11) & 0x1f;
         const int rowDb1 = (rowD[k] >> 11) & 0x1f;
         const int r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const int g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const int b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         dst[i] = (b << 11) | (g << 5) | r;
      }
   }
   else if ((datatype == DTYPE_USHORT_4_4_4_4) && (comps == 4)) {
      DECLARE_ROW_POINTERS0(ushort);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0xf;
         const int rowAr1 = rowA[k] & 0xf;
         const int rowBr0 = rowB[j] & 0xf;
         const int rowBr1 = rowB[k] & 0xf;
         const int rowCr0 = rowC[j] & 0xf;
         const int rowCr1 = rowC[k] & 0xf;
         const int rowDr0 = rowD[j] & 0xf;
         const int rowDr1 = rowD[k] & 0xf;
         const int rowAg0 = (rowA[j] >> 4) & 0xf;
         const int rowAg1 = (rowA[k] >> 4) & 0xf;
         const int rowBg0 = (rowB[j] >> 4) & 0xf;
         const int rowBg1 = (rowB[k] >> 4) & 0xf;
         const int rowCg0 = (rowC[j] >> 4) & 0xf;
         const int rowCg1 = (rowC[k] >> 4) & 0xf;
         const int rowDg0 = (rowD[j] >> 4) & 0xf;
         const int rowDg1 = (rowD[k] >> 4) & 0xf;
         const int rowAb0 = (rowA[j] >> 8) & 0xf;
         const int rowAb1 = (rowA[k] >> 8) & 0xf;
         const int rowBb0 = (rowB[j] >> 8) & 0xf;
         const int rowBb1 = (rowB[k] >> 8) & 0xf;
         const int rowCb0 = (rowC[j] >> 8) & 0xf;
         const int rowCb1 = (rowC[k] >> 8) & 0xf;
         const int rowDb0 = (rowD[j] >> 8) & 0xf;
         const int rowDb1 = (rowD[k] >> 8) & 0xf;
         const int rowAa0 = (rowA[j] >> 12) & 0xf;
         const int rowAa1 = (rowA[k] >> 12) & 0xf;
         const int rowBa0 = (rowB[j] >> 12) & 0xf;
         const int rowBa1 = (rowB[k] >> 12) & 0xf;
         const int rowCa0 = (rowC[j] >> 12) & 0xf;
         const int rowCa1 = (rowC[k] >> 12) & 0xf;
         const int rowDa0 = (rowD[j] >> 12) & 0xf;
         const int rowDa1 = (rowD[k] >> 12) & 0xf;
         const int r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const int g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const int b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         const int a = FILTER_SUM_3D(rowAa0, rowAa1, rowBa0, rowBa1,
                                       rowCa0, rowCa1, rowDa0, rowDa1);

         dst[i] = (a << 12) | (b << 8) | (g << 4) | r;
      }
   }
   else if ((datatype == DTYPE_USHORT_1_5_5_5_REV) && (comps == 4)) {
      DECLARE_ROW_POINTERS0(ushort);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x1f;
         const int rowAr1 = rowA[k] & 0x1f;
         const int rowBr0 = rowB[j] & 0x1f;
         const int rowBr1 = rowB[k] & 0x1f;
         const int rowCr0 = rowC[j] & 0x1f;
         const int rowCr1 = rowC[k] & 0x1f;
         const int rowDr0 = rowD[j] & 0x1f;
         const int rowDr1 = rowD[k] & 0x1f;
         const int rowAg0 = (rowA[j] >> 5) & 0x1f;
         const int rowAg1 = (rowA[k] >> 5) & 0x1f;
         const int rowBg0 = (rowB[j] >> 5) & 0x1f;
         const int rowBg1 = (rowB[k] >> 5) & 0x1f;
         const int rowCg0 = (rowC[j] >> 5) & 0x1f;
         const int rowCg1 = (rowC[k] >> 5) & 0x1f;
         const int rowDg0 = (rowD[j] >> 5) & 0x1f;
         const int rowDg1 = (rowD[k] >> 5) & 0x1f;
         const int rowAb0 = (rowA[j] >> 10) & 0x1f;
         const int rowAb1 = (rowA[k] >> 10) & 0x1f;
         const int rowBb0 = (rowB[j] >> 10) & 0x1f;
         const int rowBb1 = (rowB[k] >> 10) & 0x1f;
         const int rowCb0 = (rowC[j] >> 10) & 0x1f;
         const int rowCb1 = (rowC[k] >> 10) & 0x1f;
         const int rowDb0 = (rowD[j] >> 10) & 0x1f;
         const int rowDb1 = (rowD[k] >> 10) & 0x1f;
         const int rowAa0 = (rowA[j] >> 15) & 0x1;
         const int rowAa1 = (rowA[k] >> 15) & 0x1;
         const int rowBa0 = (rowB[j] >> 15) & 0x1;
         const int rowBa1 = (rowB[k] >> 15) & 0x1;
         const int rowCa0 = (rowC[j] >> 15) & 0x1;
         const int rowCa1 = (rowC[k] >> 15) & 0x1;
         const int rowDa0 = (rowD[j] >> 15) & 0x1;
         const int rowDa1 = (rowD[k] >> 15) & 0x1;
         const int r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const int g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const int b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         const int a = FILTER_SUM_3D(rowAa0, rowAa1, rowBa0, rowBa1,
                                       rowCa0, rowCa1, rowDa0, rowDa1);

         dst[i] = (a << 15) | (b << 10) | (g << 5) | r;
      }
   }
   else if ((datatype == DTYPE_UBYTE_3_3_2) && (comps == 3)) {
      DECLARE_ROW_POINTERS0(ushort);

      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x3;
         const int rowAr1 = rowA[k] & 0x3;
         const int rowBr0 = rowB[j] & 0x3;
         const int rowBr1 = rowB[k] & 0x3;
         const int rowCr0 = rowC[j] & 0x3;
         const int rowCr1 = rowC[k] & 0x3;
         const int rowDr0 = rowD[j] & 0x3;
         const int rowDr1 = rowD[k] & 0x3;
         const int rowAg0 = (rowA[j] >> 2) & 0x7;
         const int rowAg1 = (rowA[k] >> 2) & 0x7;
         const int rowBg0 = (rowB[j] >> 2) & 0x7;
         const int rowBg1 = (rowB[k] >> 2) & 0x7;
         const int rowCg0 = (rowC[j] >> 2) & 0x7;
         const int rowCg1 = (rowC[k] >> 2) & 0x7;
         const int rowDg0 = (rowD[j] >> 2) & 0x7;
         const int rowDg1 = (rowD[k] >> 2) & 0x7;
         const int rowAb0 = (rowA[j] >> 5) & 0x7;
         const int rowAb1 = (rowA[k] >> 5) & 0x7;
         const int rowBb0 = (rowB[j] >> 5) & 0x7;
         const int rowBb1 = (rowB[k] >> 5) & 0x7;
         const int rowCb0 = (rowC[j] >> 5) & 0x7;
         const int rowCb1 = (rowC[k] >> 5) & 0x7;
         const int rowDb0 = (rowD[j] >> 5) & 0x7;
         const int rowDb1 = (rowD[k] >> 5) & 0x7;
         const int r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const int g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const int b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         dst[i] = (b << 5) | (g << 2) | r;
      }
   }
   else {
      debug_printf("bad format in do_row_3D()");
   }
}



static void
format_to_type_comps(enum pipe_format pformat,
                     enum dtype *datatype, uint *comps)
{
   /* XXX I think this could be implemented in terms of the pf_*() functions */
   switch (pformat) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_X8R8G8B8_UNORM:
   case PIPE_FORMAT_A8B8G8R8_SRGB:
   case PIPE_FORMAT_X8B8G8R8_SRGB:
   case PIPE_FORMAT_B8G8R8A8_SRGB:
   case PIPE_FORMAT_B8G8R8X8_SRGB:
   case PIPE_FORMAT_A8R8G8B8_SRGB:
   case PIPE_FORMAT_X8R8G8B8_SRGB:
   case PIPE_FORMAT_R8G8B8_SRGB:
      *datatype = DTYPE_UBYTE;
      *comps = 4;
      return;
   case PIPE_FORMAT_B5G5R5X1_UNORM:
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      *datatype = DTYPE_USHORT_1_5_5_5_REV;
      *comps = 4;
      return;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      *datatype = DTYPE_USHORT_4_4_4_4;
      *comps = 4;
      return;
   case PIPE_FORMAT_B5G6R5_UNORM:
      *datatype = DTYPE_USHORT_5_6_5;
      *comps = 3;
      return;
   case PIPE_FORMAT_L8_UNORM:
   case PIPE_FORMAT_L8_SRGB:
   case PIPE_FORMAT_A8_UNORM:
   case PIPE_FORMAT_I8_UNORM:
      *datatype = DTYPE_UBYTE;
      *comps = 1;
      return;
   case PIPE_FORMAT_L8A8_UNORM:
   case PIPE_FORMAT_L8A8_SRGB:
      *datatype = DTYPE_UBYTE;
      *comps = 2;
      return;
   default:
      assert(0);
      *datatype = DTYPE_UBYTE;
      *comps = 0;
      break;
   }
}


static void
reduce_1d(enum pipe_format pformat,
          int srcWidth, const ubyte *srcPtr,
          int dstWidth, ubyte *dstPtr)
{
   enum dtype datatype;
   uint comps;

   format_to_type_comps(pformat, &datatype, &comps);

   /* we just duplicate the input row, kind of hack, saves code */
   do_row(datatype, comps,
          srcWidth, srcPtr, srcPtr,
          dstWidth, dstPtr);
}


/**
 * Strides are in bytes.  If zero, it'll be computed as width * bpp.
 */
static void
reduce_2d(enum pipe_format pformat,
          int srcWidth, int srcHeight,
          int srcRowStride, const ubyte *srcPtr,
          int dstWidth, int dstHeight,
          int dstRowStride, ubyte *dstPtr)
{
   enum dtype datatype;
   uint comps;
   const int bpt = util_format_get_blocksize(pformat);
   const ubyte *srcA, *srcB;
   ubyte *dst;
   int row;

   format_to_type_comps(pformat, &datatype, &comps);

   if (!srcRowStride)
      srcRowStride = bpt * srcWidth;

   if (!dstRowStride)
      dstRowStride = bpt * dstWidth;

   /* Compute src and dst pointers */
   srcA = srcPtr;
   if (srcHeight > 1) 
      srcB = srcA + srcRowStride;
   else
      srcB = srcA;
   dst = dstPtr;

   for (row = 0; row < dstHeight; row++) {
      do_row(datatype, comps,
             srcWidth, srcA, srcB,
             dstWidth, dst);
      srcA += 2 * srcRowStride;
      srcB += 2 * srcRowStride;
      dst += dstRowStride;
   }
}


static void
reduce_3d(enum pipe_format pformat,
          int srcWidth, int srcHeight, int srcDepth,
          int srcRowStride, int srcImageStride, const ubyte *srcPtr,
          int dstWidth, int dstHeight, int dstDepth,
          int dstRowStride, int dstImageStride, ubyte *dstPtr)
{
   const int bpt = util_format_get_blocksize(pformat);
   int img, row;
   int srcImageOffset, srcRowOffset;
   enum dtype datatype;
   uint comps;

   format_to_type_comps(pformat, &datatype, &comps);

   /* XXX I think we should rather assert those strides */
   if (!srcImageStride)
      srcImageStride = srcWidth * srcHeight * bpt;
   if (!dstImageStride)
      dstImageStride = dstWidth * dstHeight * bpt;

   if (!srcRowStride)
      srcRowStride = srcWidth * bpt;
   if (!dstRowStride)
      dstRowStride = dstWidth * bpt;

   /* Offset between adjacent src images to be averaged together */
   srcImageOffset = (srcDepth == dstDepth) ? 0 : srcImageStride;

   /* Offset between adjacent src rows to be averaged together */
   srcRowOffset = (srcHeight == dstHeight) ? 0 : srcRowStride;

   /*
    * Need to average together up to 8 src pixels for each dest pixel.
    * Break that down into 3 operations:
    *   1. take two rows from source image and average them together.
    *   2. take two rows from next source image and average them together.
    *   3. take the two averaged rows and average them for the final dst row.
    */

   /*
   printf("mip3d %d x %d x %d  ->  %d x %d x %d\n",
          srcWidth, srcHeight, srcDepth, dstWidth, dstHeight, dstDepth);
   */

   for (img = 0; img < dstDepth; img++) {
      /* first source image pointer */
      const ubyte *imgSrcA = srcPtr
         + img * (srcImageStride + srcImageOffset);
      /* second source image pointer */
      const ubyte *imgSrcB = imgSrcA + srcImageOffset;
      /* address of the dest image */
      ubyte *imgDst = dstPtr + img * dstImageStride;

      /* setup the four source row pointers and the dest row pointer */
      const ubyte *srcImgARowA = imgSrcA;
      const ubyte *srcImgARowB = imgSrcA + srcRowOffset;
      const ubyte *srcImgBRowA = imgSrcB;
      const ubyte *srcImgBRowB = imgSrcB + srcRowOffset;
      ubyte *dstImgRow = imgDst;

      for (row = 0; row < dstHeight; row++) {
         do_row_3D(datatype, comps, srcWidth, 
                   srcImgARowA, srcImgARowB,
                   srcImgBRowA, srcImgBRowB,
                   dstWidth, dstImgRow);

         /* advance to next rows */
         srcImgARowA += srcRowStride + srcRowOffset;
         srcImgARowB += srcRowStride + srcRowOffset;
         srcImgBRowA += srcRowStride + srcRowOffset;
         srcImgBRowB += srcRowStride + srcRowOffset;
         dstImgRow += dstImageStride;
      }
   }
}




static void
make_1d_mipmap(struct gen_mipmap_state *ctx,
               struct pipe_resource *pt,
               uint layer, uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = ctx->pipe;
   uint dstLevel;

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_transfer *srcTrans, *dstTrans;
      void *srcMap, *dstMap;

      srcTrans = pipe_get_transfer(pipe, pt, srcLevel, layer,
                                   PIPE_TRANSFER_READ, 0, 0,
                                   u_minify(pt->width0, srcLevel),
                                   u_minify(pt->height0, srcLevel));
      dstTrans = pipe_get_transfer(pipe, pt, dstLevel, layer,
                                   PIPE_TRANSFER_WRITE, 0, 0,
                                   u_minify(pt->width0, dstLevel),
                                   u_minify(pt->height0, dstLevel));

      srcMap = (ubyte *) pipe->transfer_map(pipe, srcTrans);
      dstMap = (ubyte *) pipe->transfer_map(pipe, dstTrans);

      reduce_1d(pt->format,
                srcTrans->box.width, srcMap,
                dstTrans->box.width, dstMap);

      pipe->transfer_unmap(pipe, srcTrans);
      pipe->transfer_unmap(pipe, dstTrans);

      pipe->transfer_destroy(pipe, srcTrans);
      pipe->transfer_destroy(pipe, dstTrans);
   }
}


static void
make_2d_mipmap(struct gen_mipmap_state *ctx,
               struct pipe_resource *pt,
               uint layer, uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = ctx->pipe;
   uint dstLevel;

   assert(util_format_get_blockwidth(pt->format) == 1);
   assert(util_format_get_blockheight(pt->format) == 1);

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_transfer *srcTrans, *dstTrans;
      ubyte *srcMap, *dstMap;

      srcTrans = pipe_get_transfer(pipe, pt, srcLevel, layer,
                                   PIPE_TRANSFER_READ, 0, 0,
                                   u_minify(pt->width0, srcLevel),
                                   u_minify(pt->height0, srcLevel));
      dstTrans = pipe_get_transfer(pipe, pt, dstLevel, layer,
                                   PIPE_TRANSFER_WRITE, 0, 0,
                                   u_minify(pt->width0, dstLevel),
                                   u_minify(pt->height0, dstLevel));

      srcMap = (ubyte *) pipe->transfer_map(pipe, srcTrans);
      dstMap = (ubyte *) pipe->transfer_map(pipe, dstTrans);

      reduce_2d(pt->format,
                srcTrans->box.width, srcTrans->box.height,
                srcTrans->stride, srcMap,
                dstTrans->box.width, dstTrans->box.height,
                dstTrans->stride, dstMap);

      pipe->transfer_unmap(pipe, srcTrans);
      pipe->transfer_unmap(pipe, dstTrans);

      pipe->transfer_destroy(pipe, srcTrans);
      pipe->transfer_destroy(pipe, dstTrans);
   }
}


/* XXX looks a bit more like it could work now but need to test */
static void
make_3d_mipmap(struct gen_mipmap_state *ctx,
               struct pipe_resource *pt,
               uint face, uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = ctx->pipe;
   uint dstLevel;
   struct pipe_box src_box, dst_box;

   assert(util_format_get_blockwidth(pt->format) == 1);
   assert(util_format_get_blockheight(pt->format) == 1);

   src_box.x = src_box.y = src_box.z = 0;
   dst_box.x = dst_box.y = dst_box.z = 0;

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_transfer *srcTrans, *dstTrans;
      ubyte *srcMap, *dstMap;
      struct pipe_box src_box, dst_box;
      src_box.width = u_minify(pt->width0, srcLevel);
      src_box.height = u_minify(pt->height0, srcLevel);
      src_box.depth = u_minify(pt->depth0, srcLevel);
      dst_box.width = u_minify(pt->width0, dstLevel);
      dst_box.height = u_minify(pt->height0, dstLevel);
      dst_box.depth = u_minify(pt->depth0, dstLevel);

      srcTrans = pipe->get_transfer(pipe, pt, srcLevel,
                                    PIPE_TRANSFER_READ,
                                    &src_box);
      dstTrans = pipe->get_transfer(pipe, pt, dstLevel,
                                    PIPE_TRANSFER_WRITE,
                                    &dst_box);

      srcMap = (ubyte *) pipe->transfer_map(pipe, srcTrans);
      dstMap = (ubyte *) pipe->transfer_map(pipe, dstTrans);

      reduce_3d(pt->format,
                srcTrans->box.width, srcTrans->box.height, srcTrans->box.depth,
                srcTrans->stride, srcTrans->layer_stride, srcMap,
                dstTrans->box.width, dstTrans->box.height, dstTrans->box.depth,
                dstTrans->stride, dstTrans->layer_stride, dstMap);

      pipe->transfer_unmap(pipe, srcTrans);
      pipe->transfer_unmap(pipe, dstTrans);

      pipe->transfer_destroy(pipe, srcTrans);
      pipe->transfer_destroy(pipe, dstTrans);
   }
}


static void
fallback_gen_mipmap(struct gen_mipmap_state *ctx,
                    struct pipe_resource *pt,
                    uint layer, uint baseLevel, uint lastLevel)
{
   switch (pt->target) {
   case PIPE_TEXTURE_1D:
      make_1d_mipmap(ctx, pt, layer, baseLevel, lastLevel);
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_CUBE:
      make_2d_mipmap(ctx, pt, layer, baseLevel, lastLevel);
      break;
   case PIPE_TEXTURE_3D:
      make_3d_mipmap(ctx, pt, layer, baseLevel, lastLevel);
      break;
   default:
      assert(0);
   }
}


/**
 * Create a mipmap generation context.
 * The idea is to create one of these and re-use it each time we need to
 * generate a mipmap.
 */
struct gen_mipmap_state *
util_create_gen_mipmap(struct pipe_context *pipe,
                       struct cso_context *cso)
{
   struct gen_mipmap_state *ctx;
   uint i;

   ctx = CALLOC_STRUCT(gen_mipmap_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;
   ctx->cso = cso;

   /* disabled blending/masking */
   memset(&ctx->blend, 0, sizeof(ctx->blend));
   ctx->blend.rt[0].colormask = PIPE_MASK_RGBA;

   /* no-op depth/stencil/alpha */
   memset(&ctx->depthstencil, 0, sizeof(ctx->depthstencil));

   /* rasterizer */
   memset(&ctx->rasterizer, 0, sizeof(ctx->rasterizer));
   ctx->rasterizer.cull_face = PIPE_FACE_NONE;
   ctx->rasterizer.gl_rasterization_rules = 1;
   ctx->rasterizer.depth_clip = 1;

   /* sampler state */
   memset(&ctx->sampler, 0, sizeof(ctx->sampler));
   ctx->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   ctx->sampler.normalized_coords = 1;

   /* vertex elements state */
   memset(&ctx->velem[0], 0, sizeof(ctx->velem[0]) * 2);
   for (i = 0; i < 2; i++) {
      ctx->velem[i].src_offset = i * 4 * sizeof(float);
      ctx->velem[i].instance_divisor = 0;
      ctx->velem[i].vertex_buffer_index = 0;
      ctx->velem[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }

   /* vertex data that doesn't change */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][0][2] = 0.0f; /* z */
      ctx->vertices[i][0][3] = 1.0f; /* w */
      ctx->vertices[i][1][3] = 1.0f; /* q */
   }

   /* Note: the actual vertex buffer is allocated as needed below */

   return ctx;
}


/**
 * Helper function to set the fragment shaders.
 */
static INLINE void
set_fragment_shader(struct gen_mipmap_state *ctx, uint type)
{
   if (!ctx->fs[type])
      ctx->fs[type] =
         util_make_fragment_tex_shader(ctx->pipe, type,
                                       TGSI_INTERPOLATE_LINEAR);

   cso_set_fragment_shader_handle(ctx->cso, ctx->fs[type]);
}


/**
 * Helper function to set the vertex shader.
 */
static INLINE void
set_vertex_shader(struct gen_mipmap_state *ctx)
{
   /* vertex shader - still required to provide the linkage between
    * fragment shader input semantics and vertex_element/buffers.
    */
   if (!ctx->vs)
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(ctx->pipe, 2,
                                                    semantic_names,
                                                    semantic_indexes);
   }

   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);
}


/**
 * Get next "slot" of vertex space in the vertex buffer.
 * We're allocating one large vertex buffer and using it piece by piece.
 */
static unsigned
get_next_slot(struct gen_mipmap_state *ctx)
{
   const unsigned max_slots = 4096 / sizeof ctx->vertices;

   if (ctx->vbuf_slot >= max_slots) 
      util_gen_mipmap_flush( ctx );

   if (!ctx->vbuf) {
      ctx->vbuf = pipe_buffer_create(ctx->pipe->screen,
                                     PIPE_BIND_VERTEX_BUFFER,
                                     PIPE_USAGE_STREAM,
                                     max_slots * sizeof ctx->vertices);
   }
   
   return ctx->vbuf_slot++ * sizeof ctx->vertices;
}


static unsigned
set_vertex_data(struct gen_mipmap_state *ctx,
                enum pipe_texture_target tex_target,
                uint layer, float r)
{
   unsigned offset;

   /* vert[0].position */
   ctx->vertices[0][0][0] = -1.0f; /*x*/
   ctx->vertices[0][0][1] = -1.0f; /*y*/

   /* vert[1].position */
   ctx->vertices[1][0][0] = 1.0f;
   ctx->vertices[1][0][1] = -1.0f;

   /* vert[2].position */
   ctx->vertices[2][0][0] = 1.0f;
   ctx->vertices[2][0][1] = 1.0f;

   /* vert[3].position */
   ctx->vertices[3][0][0] = -1.0f;
   ctx->vertices[3][0][1] = 1.0f;

   /* Setup vertex texcoords.  This is a little tricky for cube maps. */
   if (tex_target == PIPE_TEXTURE_CUBE) {
      static const float st[4][2] = {
         {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
      };

      util_map_texcoords2d_onto_cubemap(layer, &st[0][0], 2,
                                        &ctx->vertices[0][1][0], 8);
   }
   else if (tex_target == PIPE_TEXTURE_1D_ARRAY) {
      /* 1D texture array  */
      ctx->vertices[0][1][0] = 0.0f; /*s*/
      ctx->vertices[0][1][1] = r; /*t*/
      ctx->vertices[0][1][2] = 0.0f;    /*r*/

      ctx->vertices[1][1][0] = 1.0f;
      ctx->vertices[1][1][1] = r;
      ctx->vertices[1][1][2] = 0.0f;

      ctx->vertices[2][1][0] = 1.0f;
      ctx->vertices[2][1][1] = r;
      ctx->vertices[2][1][2] = 0.0f;

      ctx->vertices[3][1][0] = 0.0f;
      ctx->vertices[3][1][1] = r;
      ctx->vertices[3][1][2] = 0.0f;
   } else {
      /* 1D/2D/3D/2D array */
      ctx->vertices[0][1][0] = 0.0f; /*s*/
      ctx->vertices[0][1][1] = 0.0f; /*t*/
      ctx->vertices[0][1][2] = r;    /*r*/

      ctx->vertices[1][1][0] = 1.0f;
      ctx->vertices[1][1][1] = 0.0f;
      ctx->vertices[1][1][2] = r;

      ctx->vertices[2][1][0] = 1.0f;
      ctx->vertices[2][1][1] = 1.0f;
      ctx->vertices[2][1][2] = r;

      ctx->vertices[3][1][0] = 0.0f;
      ctx->vertices[3][1][1] = 1.0f;
      ctx->vertices[3][1][2] = r;
   }

   offset = get_next_slot( ctx );

   pipe_buffer_write_nooverlap(ctx->pipe, ctx->vbuf,
                               offset, sizeof(ctx->vertices), ctx->vertices);

   return offset;
}



/**
 * Destroy a mipmap generation context
 */
void
util_destroy_gen_mipmap(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   unsigned i;

   for (i = 0; i < Elements(ctx->fs); i++)
      if (ctx->fs[i])
         pipe->delete_fs_state(pipe, ctx->fs[i]);

   if (ctx->vs)
      pipe->delete_vs_state(pipe, ctx->vs);

   pipe_resource_reference(&ctx->vbuf, NULL);

   FREE(ctx);
}



/* Release vertex buffer at end of frame to avoid synchronous
 * rendering.
 */
void util_gen_mipmap_flush( struct gen_mipmap_state *ctx )
{
   pipe_resource_reference(&ctx->vbuf, NULL);
   ctx->vbuf_slot = 0;
} 


/**
 * Generate mipmap images.  It's assumed all needed texture memory is
 * already allocated.
 *
 * \param psv  the sampler view to the texture to generate mipmap levels for
 * \param face  which cube face to generate mipmaps for (0 for non-cube maps)
 * \param baseLevel  the first mipmap level to use as a src
 * \param lastLevel  the last mipmap level to generate
 * \param filter  the minification filter used to generate mipmap levels with
 * \param filter  one of PIPE_TEX_FILTER_LINEAR, PIPE_TEX_FILTER_NEAREST
 */
void
util_gen_mipmap(struct gen_mipmap_state *ctx,
                struct pipe_sampler_view *psv,
                uint face, uint baseLevel, uint lastLevel, uint filter)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb;
   struct pipe_resource *pt = psv->texture;
   uint dstLevel;
   uint offset;
   uint type;

   /* The texture object should have room for the levels which we're
    * about to generate.
    */
   assert(lastLevel <= pt->last_level);

   /* If this fails, why are we here? */
   assert(lastLevel > baseLevel);

   assert(filter == PIPE_TEX_FILTER_LINEAR ||
          filter == PIPE_TEX_FILTER_NEAREST);

   switch (pt->target) {
   case PIPE_TEXTURE_1D:
      type = TGSI_TEXTURE_1D;
      break;
   case PIPE_TEXTURE_2D:
      type = TGSI_TEXTURE_2D;
      break;
   case PIPE_TEXTURE_3D:
      type = TGSI_TEXTURE_3D;
      break;
   case PIPE_TEXTURE_CUBE:
      type = TGSI_TEXTURE_CUBE;
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      type = TGSI_TEXTURE_1D_ARRAY;
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      type = TGSI_TEXTURE_2D_ARRAY;
      break;
   default:
      assert(0);
      type = TGSI_TEXTURE_2D;
   }

   /* check if we can render in the texture's format */
   if (!screen->is_format_supported(screen, psv->format, pt->target,
                                    pt->nr_samples, PIPE_BIND_RENDER_TARGET)) {
      fallback_gen_mipmap(ctx, pt, face, baseLevel, lastLevel);
      return;
   }

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_fragment_sampler_views(ctx->cso);
   cso_save_stream_outputs(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_geometry_shader(ctx->cso);
   cso_save_viewport(ctx->cso);
   cso_save_vertex_elements(ctx->cso);

   /* bind our state */
   cso_set_blend(ctx->cso, &ctx->blend);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->depthstencil);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_vertex_elements(ctx->cso, 2, ctx->velem);
   cso_set_stream_outputs(ctx->cso, 0, NULL, 0);

   set_fragment_shader(ctx, type);
   set_vertex_shader(ctx);
   cso_set_geometry_shader_handle(ctx->cso, NULL);

   /* init framebuffer state */
   memset(&fb, 0, sizeof(fb));
   fb.nr_cbufs = 1;

   /* set min/mag to same filter for faster sw speed */
   ctx->sampler.mag_img_filter = filter;
   ctx->sampler.min_img_filter = filter;

   /*
    * XXX for small mipmap levels, it may be faster to use the software
    * fallback path...
    */
   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_viewport_state vp;
      unsigned nr_layers, layer, i;
      float rcoord = 0.0f;

      if (pt->target == PIPE_TEXTURE_3D)
         nr_layers = u_minify(pt->depth0, dstLevel);
      else if (pt->target == PIPE_TEXTURE_2D_ARRAY || pt->target == PIPE_TEXTURE_1D_ARRAY)
	 nr_layers = pt->array_size;
      else
         nr_layers = 1;

      for (i = 0; i < nr_layers; i++) {
         struct pipe_surface *surf, surf_templ;
         if (pt->target == PIPE_TEXTURE_3D) {
            /* in theory with geom shaders and driver with full layer support
               could do that in one go. */
            layer = i;
            /* XXX hmm really? */
            rcoord = (float)layer / (float)nr_layers + 1.0f / (float)(nr_layers * 2);
         } else if (pt->target == PIPE_TEXTURE_2D_ARRAY || pt->target == PIPE_TEXTURE_1D_ARRAY) {
	    layer = i;
	    rcoord = (float)layer;
	 } else
            layer = face;

         memset(&surf_templ, 0, sizeof(surf_templ));
         u_surface_default_template(&surf_templ, pt, PIPE_BIND_RENDER_TARGET);
         surf_templ.u.tex.level = dstLevel;
         surf_templ.u.tex.first_layer = layer;
         surf_templ.u.tex.last_layer = layer;
         surf = pipe->create_surface(pipe, pt, &surf_templ);

         /*
          * Setup framebuffer / dest surface
          */
         fb.cbufs[0] = surf;
         fb.width = u_minify(pt->width0, dstLevel);
         fb.height = u_minify(pt->height0, dstLevel);
         cso_set_framebuffer(ctx->cso, &fb);

         /* viewport */
         vp.scale[0] = 0.5f * fb.width;
         vp.scale[1] = 0.5f * fb.height;
         vp.scale[2] = 1.0f;
         vp.scale[3] = 1.0f;
         vp.translate[0] = 0.5f * fb.width;
         vp.translate[1] = 0.5f * fb.height;
         vp.translate[2] = 0.0f;
         vp.translate[3] = 0.0f;
         cso_set_viewport(ctx->cso, &vp);

         /*
          * Setup sampler state
          * Note: we should only have to set the min/max LOD clamps to ensure
          * we grab texels from the right mipmap level.  But some hardware
          * has trouble with min clamping so we also set the lod_bias to
          * try to work around that.
          */
         ctx->sampler.min_lod = ctx->sampler.max_lod = (float) srcLevel;
         ctx->sampler.lod_bias = (float) srcLevel;
         cso_single_sampler(ctx->cso, 0, &ctx->sampler);
         cso_single_sampler_done(ctx->cso);

         cso_set_fragment_sampler_views(ctx->cso, 1, &psv);

         /* quad coords in clip coords */
         offset = set_vertex_data(ctx,
                                  pt->target,
                                  face,
                                  rcoord);

         util_draw_vertex_buffer(ctx->pipe,
                                 ctx->cso,
                                 ctx->vbuf,
                                 offset,
                                 PIPE_PRIM_TRIANGLE_FAN,
                                 4,  /* verts */
                                 2); /* attribs/vert */

         /* need to signal that the texture has changed _after_ rendering to it */
         pipe_surface_reference( &surf, NULL );
      }
   }

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_samplers(ctx->cso);
   cso_restore_fragment_sampler_views(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_geometry_shader(ctx->cso);
   cso_restore_viewport(ctx->cso);
   cso_restore_vertex_elements(ctx->cso);
   cso_restore_stream_outputs(ctx->cso);
}
