/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * quad blending
 * \author Brian Paul
 */

#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "sp_context.h"
#include "sp_state.h"
#include "sp_quad.h"
#include "sp_tile_cache.h"
#include "sp_quad_pipe.h"


enum format
{
   RGBA,
   RGB,
   LUMINANCE,
   LUMINANCE_ALPHA,
   INTENSITY
};


/** Subclass of quad_stage */
struct blend_quad_stage
{
   struct quad_stage base;
   boolean clamp[PIPE_MAX_COLOR_BUFS];  /**< clamp colors to [0,1]? */
   enum format base_format[PIPE_MAX_COLOR_BUFS];
   enum util_format_type format_type[PIPE_MAX_COLOR_BUFS];
};


/** cast wrapper */
static INLINE struct blend_quad_stage *
blend_quad_stage(struct quad_stage *stage)
{
   return (struct blend_quad_stage *) stage;
}


#define VEC4_COPY(DST, SRC) \
do { \
    DST[0] = SRC[0]; \
    DST[1] = SRC[1]; \
    DST[2] = SRC[2]; \
    DST[3] = SRC[3]; \
} while(0)

#define VEC4_SCALAR(DST, SRC) \
do { \
    DST[0] = SRC; \
    DST[1] = SRC; \
    DST[2] = SRC; \
    DST[3] = SRC; \
} while(0)

#define VEC4_ADD(R, A, B) \
do { \
   R[0] = A[0] + B[0]; \
   R[1] = A[1] + B[1]; \
   R[2] = A[2] + B[2]; \
   R[3] = A[3] + B[3]; \
} while (0)

#define VEC4_SUB(R, A, B) \
do { \
   R[0] = A[0] - B[0]; \
   R[1] = A[1] - B[1]; \
   R[2] = A[2] - B[2]; \
   R[3] = A[3] - B[3]; \
} while (0)

/** Add and limit result to ceiling of 1.0 */
#define VEC4_ADD_SAT(R, A, B) \
do { \
   R[0] = A[0] + B[0];  if (R[0] > 1.0f) R[0] = 1.0f; \
   R[1] = A[1] + B[1];  if (R[1] > 1.0f) R[1] = 1.0f; \
   R[2] = A[2] + B[2];  if (R[2] > 1.0f) R[2] = 1.0f; \
   R[3] = A[3] + B[3];  if (R[3] > 1.0f) R[3] = 1.0f; \
} while (0)

/** Subtract and limit result to floor of 0.0 */
#define VEC4_SUB_SAT(R, A, B) \
do { \
   R[0] = A[0] - B[0];  if (R[0] < 0.0f) R[0] = 0.0f; \
   R[1] = A[1] - B[1];  if (R[1] < 0.0f) R[1] = 0.0f; \
   R[2] = A[2] - B[2];  if (R[2] < 0.0f) R[2] = 0.0f; \
   R[3] = A[3] - B[3];  if (R[3] < 0.0f) R[3] = 0.0f; \
} while (0)

#define VEC4_MUL(R, A, B) \
do { \
   R[0] = A[0] * B[0]; \
   R[1] = A[1] * B[1]; \
   R[2] = A[2] * B[2]; \
   R[3] = A[3] * B[3]; \
} while (0)

#define VEC4_MIN(R, A, B) \
do { \
   R[0] = (A[0] < B[0]) ? A[0] : B[0]; \
   R[1] = (A[1] < B[1]) ? A[1] : B[1]; \
   R[2] = (A[2] < B[2]) ? A[2] : B[2]; \
   R[3] = (A[3] < B[3]) ? A[3] : B[3]; \
} while (0)

#define VEC4_MAX(R, A, B) \
do { \
   R[0] = (A[0] > B[0]) ? A[0] : B[0]; \
   R[1] = (A[1] > B[1]) ? A[1] : B[1]; \
   R[2] = (A[2] > B[2]) ? A[2] : B[2]; \
   R[3] = (A[3] > B[3]) ? A[3] : B[3]; \
} while (0)



static void
logicop_quad(struct quad_stage *qs, 
             float (*quadColor)[4],
             float (*dest)[4])
{
   struct softpipe_context *softpipe = qs->softpipe;
   ubyte src[4][4], dst[4][4], res[4][4];
   uint *src4 = (uint *) src;
   uint *dst4 = (uint *) dst;
   uint *res4 = (uint *) res;
   uint j;


   /* convert to ubyte */
   for (j = 0; j < 4; j++) { /* loop over R,G,B,A channels */
      dst[j][0] = float_to_ubyte(dest[j][0]); /* P0 */
      dst[j][1] = float_to_ubyte(dest[j][1]); /* P1 */
      dst[j][2] = float_to_ubyte(dest[j][2]); /* P2 */
      dst[j][3] = float_to_ubyte(dest[j][3]); /* P3 */

      src[j][0] = float_to_ubyte(quadColor[j][0]); /* P0 */
      src[j][1] = float_to_ubyte(quadColor[j][1]); /* P1 */
      src[j][2] = float_to_ubyte(quadColor[j][2]); /* P2 */
      src[j][3] = float_to_ubyte(quadColor[j][3]); /* P3 */
   }

   switch (softpipe->blend->logicop_func) {
   case PIPE_LOGICOP_CLEAR:
      for (j = 0; j < 4; j++)
         res4[j] = 0;
      break;
   case PIPE_LOGICOP_NOR:
      for (j = 0; j < 4; j++)
         res4[j] = ~(src4[j] | dst4[j]);
      break;
   case PIPE_LOGICOP_AND_INVERTED:
      for (j = 0; j < 4; j++)
         res4[j] = ~src4[j] & dst4[j];
      break;
   case PIPE_LOGICOP_COPY_INVERTED:
      for (j = 0; j < 4; j++)
         res4[j] = ~src4[j];
      break;
   case PIPE_LOGICOP_AND_REVERSE:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] & ~dst4[j];
      break;
   case PIPE_LOGICOP_INVERT:
      for (j = 0; j < 4; j++)
         res4[j] = ~dst4[j];
      break;
   case PIPE_LOGICOP_XOR:
      for (j = 0; j < 4; j++)
         res4[j] = dst4[j] ^ src4[j];
      break;
   case PIPE_LOGICOP_NAND:
      for (j = 0; j < 4; j++)
         res4[j] = ~(src4[j] & dst4[j]);
      break;
   case PIPE_LOGICOP_AND:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] & dst4[j];
      break;
   case PIPE_LOGICOP_EQUIV:
      for (j = 0; j < 4; j++)
         res4[j] = ~(src4[j] ^ dst4[j]);
      break;
   case PIPE_LOGICOP_NOOP:
      for (j = 0; j < 4; j++)
         res4[j] = dst4[j];
      break;
   case PIPE_LOGICOP_OR_INVERTED:
      for (j = 0; j < 4; j++)
         res4[j] = ~src4[j] | dst4[j];
      break;
   case PIPE_LOGICOP_COPY:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j];
      break;
   case PIPE_LOGICOP_OR_REVERSE:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] | ~dst4[j];
      break;
   case PIPE_LOGICOP_OR:
      for (j = 0; j < 4; j++)
         res4[j] = src4[j] | dst4[j];
      break;
   case PIPE_LOGICOP_SET:
      for (j = 0; j < 4; j++)
         res4[j] = ~0;
      break;
   default:
      assert(0 && "invalid logicop mode");
   }

   for (j = 0; j < 4; j++) {
      quadColor[j][0] = ubyte_to_float(res[j][0]);
      quadColor[j][1] = ubyte_to_float(res[j][1]);
      quadColor[j][2] = ubyte_to_float(res[j][2]);
      quadColor[j][3] = ubyte_to_float(res[j][3]);
   }
}



/**
 * Do blending for a 2x2 quad for one color buffer.
 * \param quadColor  the incoming quad colors
 * \param dest  the destination/framebuffer quad colors
 * \param const_blend_color  the constant blend color
 * \param blend_index  which set of blending terms to use
 */
static void
blend_quad(struct quad_stage *qs, 
           float (*quadColor)[4],
           float (*dest)[4],
           const float const_blend_color[4],
           unsigned blend_index)
{
   static const float zero[4] = { 0, 0, 0, 0 };
   static const float one[4] = { 1, 1, 1, 1 };
   struct softpipe_context *softpipe = qs->softpipe;
   float source[4][QUAD_SIZE] = { { 0 } };
   float blend_dest[4][QUAD_SIZE];

   /*
    * Compute src/first term RGB
    */
   switch (softpipe->blend->rt[blend_index].rgb_src_factor) {
   case PIPE_BLENDFACTOR_ONE:
      VEC4_COPY(source[0], quadColor[0]); /* R */
      VEC4_COPY(source[1], quadColor[1]); /* G */
      VEC4_COPY(source[2], quadColor[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      VEC4_MUL(source[0], quadColor[0], quadColor[0]); /* R */
      VEC4_MUL(source[1], quadColor[1], quadColor[1]); /* G */
      VEC4_MUL(source[2], quadColor[2], quadColor[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      {
         const float *alpha = quadColor[3];
         VEC4_MUL(source[0], quadColor[0], alpha); /* R */
         VEC4_MUL(source[1], quadColor[1], alpha); /* G */
         VEC4_MUL(source[2], quadColor[2], alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      VEC4_MUL(source[0], quadColor[0], dest[0]); /* R */
      VEC4_MUL(source[1], quadColor[1], dest[1]); /* G */
      VEC4_MUL(source[2], quadColor[2], dest[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_DST_ALPHA:
      {
         const float *alpha = dest[3];
         VEC4_MUL(source[0], quadColor[0], alpha); /* R */
         VEC4_MUL(source[1], quadColor[1], alpha); /* G */
         VEC4_MUL(source[2], quadColor[2], alpha); /* B */
      } 
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      {
         const float *alpha = quadColor[3];
         float diff[4], temp[4];
         VEC4_SUB(diff, one, dest[3]);
         VEC4_MIN(temp, alpha, diff);
         VEC4_MUL(source[0], quadColor[0], temp); /* R */
         VEC4_MUL(source[1], quadColor[1], temp); /* G */
         VEC4_MUL(source[2], quadColor[2], temp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      {
         float comp[4];
         VEC4_SCALAR(comp, const_blend_color[0]); /* R */
         VEC4_MUL(source[0], quadColor[0], comp); /* R */
         VEC4_SCALAR(comp, const_blend_color[1]); /* G */
         VEC4_MUL(source[1], quadColor[1], comp); /* G */
         VEC4_SCALAR(comp, const_blend_color[2]); /* B */
         VEC4_MUL(source[2], quadColor[2], comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      {
         float alpha[4];
         VEC4_SCALAR(alpha, const_blend_color[3]);
         VEC4_MUL(source[0], quadColor[0], alpha); /* R */
         VEC4_MUL(source[1], quadColor[1], alpha); /* G */
         VEC4_MUL(source[2], quadColor[2], alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_SRC1_COLOR:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_ZERO:
      VEC4_COPY(source[0], zero); /* R */
      VEC4_COPY(source[1], zero); /* G */
      VEC4_COPY(source[2], zero); /* B */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      {
         float inv_comp[4];
         VEC4_SUB(inv_comp, one, quadColor[0]); /* R */
         VEC4_MUL(source[0], quadColor[0], inv_comp); /* R */
         VEC4_SUB(inv_comp, one, quadColor[1]); /* G */
         VEC4_MUL(source[1], quadColor[1], inv_comp); /* G */
         VEC4_SUB(inv_comp, one, quadColor[2]); /* B */
         VEC4_MUL(source[2], quadColor[2], inv_comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SUB(inv_alpha, one, quadColor[3]);
         VEC4_MUL(source[0], quadColor[0], inv_alpha); /* R */
         VEC4_MUL(source[1], quadColor[1], inv_alpha); /* G */
         VEC4_MUL(source[2], quadColor[2], inv_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SUB(inv_alpha, one, dest[3]);
         VEC4_MUL(source[0], quadColor[0], inv_alpha); /* R */
         VEC4_MUL(source[1], quadColor[1], inv_alpha); /* G */
         VEC4_MUL(source[2], quadColor[2], inv_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      {
         float inv_comp[4];
         VEC4_SUB(inv_comp, one, dest[0]); /* R */
         VEC4_MUL(source[0], quadColor[0], inv_comp); /* R */
         VEC4_SUB(inv_comp, one, dest[1]); /* G */
         VEC4_MUL(source[1], quadColor[1], inv_comp); /* G */
         VEC4_SUB(inv_comp, one, dest[2]); /* B */
         VEC4_MUL(source[2], quadColor[2], inv_comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      {
         float inv_comp[4];
         /* R */
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[0]);
         VEC4_MUL(source[0], quadColor[0], inv_comp);
         /* G */
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[1]);
         VEC4_MUL(source[1], quadColor[1], inv_comp);
         /* B */
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[2]);
         VEC4_MUL(source[2], quadColor[2], inv_comp);
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SCALAR(inv_alpha, 1.0f - const_blend_color[3]);
         VEC4_MUL(source[0], quadColor[0], inv_alpha); /* R */
         VEC4_MUL(source[1], quadColor[1], inv_alpha); /* G */
         VEC4_MUL(source[2], quadColor[2], inv_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
      assert(0); /* to do */
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      assert(0); /* to do */
      break;
   default:
      assert(0 && "invalid rgb src factor");
   }

   /*
    * Compute src/first term A
    */
   switch (softpipe->blend->rt[blend_index].alpha_src_factor) {
   case PIPE_BLENDFACTOR_ONE:
      VEC4_COPY(source[3], quadColor[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      {
         const float *alpha = quadColor[3];
         VEC4_MUL(source[3], quadColor[3], alpha); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_DST_ALPHA:
      VEC4_MUL(source[3], quadColor[3], dest[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      /* multiply alpha by 1.0 */
      VEC4_COPY(source[3], quadColor[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      {
         float comp[4];
         VEC4_SCALAR(comp, const_blend_color[3]); /* A */
         VEC4_MUL(source[3], quadColor[3], comp); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_ZERO:
      VEC4_COPY(source[3], zero); /* A */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SUB(inv_alpha, one, quadColor[3]);
         VEC4_MUL(source[3], quadColor[3], inv_alpha); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      {
         float inv_alpha[4];
         VEC4_SUB(inv_alpha, one, dest[3]);
         VEC4_MUL(source[3], quadColor[3], inv_alpha); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      {
         float inv_comp[4];
         /* A */
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[3]);
         VEC4_MUL(source[3], quadColor[3], inv_comp);
      }
      break;
   default:
      assert(0 && "invalid alpha src factor");
   }

   /* Save the original dest for use in masking */
   VEC4_COPY(blend_dest[0], dest[0]);
   VEC4_COPY(blend_dest[1], dest[1]);
   VEC4_COPY(blend_dest[2], dest[2]);
   VEC4_COPY(blend_dest[3], dest[3]);


   /*
    * Compute blend_dest/second term RGB
    */
   switch (softpipe->blend->rt[blend_index].rgb_dst_factor) {
   case PIPE_BLENDFACTOR_ONE:
      /* blend_dest = blend_dest * 1   NO-OP, leave blend_dest as-is */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      VEC4_MUL(blend_dest[0], blend_dest[0], quadColor[0]); /* R */
      VEC4_MUL(blend_dest[1], blend_dest[1], quadColor[1]); /* G */
      VEC4_MUL(blend_dest[2], blend_dest[2], quadColor[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      VEC4_MUL(blend_dest[0], blend_dest[0], quadColor[3]); /* R * A */
      VEC4_MUL(blend_dest[1], blend_dest[1], quadColor[3]); /* G * A */
      VEC4_MUL(blend_dest[2], blend_dest[2], quadColor[3]); /* B * A */
      break;
   case PIPE_BLENDFACTOR_DST_ALPHA:
      VEC4_MUL(blend_dest[0], blend_dest[0], blend_dest[3]); /* R * A */
      VEC4_MUL(blend_dest[1], blend_dest[1], blend_dest[3]); /* G * A */
      VEC4_MUL(blend_dest[2], blend_dest[2], blend_dest[3]); /* B * A */
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      VEC4_MUL(blend_dest[0], blend_dest[0], blend_dest[0]); /* R */
      VEC4_MUL(blend_dest[1], blend_dest[1], blend_dest[1]); /* G */
      VEC4_MUL(blend_dest[2], blend_dest[2], blend_dest[2]); /* B */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      {
         const float *alpha = quadColor[3];
         float diff[4], temp[4];
         VEC4_SUB(diff, one, blend_dest[3]);
         VEC4_MIN(temp, alpha, diff);
         VEC4_MUL(blend_dest[0], quadColor[0], temp); /* R */
         VEC4_MUL(blend_dest[1], quadColor[1], temp); /* G */
         VEC4_MUL(blend_dest[2], quadColor[2], temp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      {
         float comp[4];
         VEC4_SCALAR(comp, const_blend_color[0]); /* R */
         VEC4_MUL(blend_dest[0], blend_dest[0], comp); /* R */
         VEC4_SCALAR(comp, const_blend_color[1]); /* G */
         VEC4_MUL(blend_dest[1], blend_dest[1], comp); /* G */
         VEC4_SCALAR(comp, const_blend_color[2]); /* B */
         VEC4_MUL(blend_dest[2], blend_dest[2], comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      {
         float comp[4];
         VEC4_SCALAR(comp, const_blend_color[3]); /* A */
         VEC4_MUL(blend_dest[0], blend_dest[0], comp); /* R */
         VEC4_MUL(blend_dest[1], blend_dest[1], comp); /* G */
         VEC4_MUL(blend_dest[2], blend_dest[2], comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_ZERO:
      VEC4_COPY(blend_dest[0], zero); /* R */
      VEC4_COPY(blend_dest[1], zero); /* G */
      VEC4_COPY(blend_dest[2], zero); /* B */
      break;
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
      /* XXX what are these? */
      assert(0);
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      {
         float inv_comp[4];
         VEC4_SUB(inv_comp, one, quadColor[0]); /* R */
         VEC4_MUL(blend_dest[0], inv_comp, blend_dest[0]); /* R */
         VEC4_SUB(inv_comp, one, quadColor[1]); /* G */
         VEC4_MUL(blend_dest[1], inv_comp, blend_dest[1]); /* G */
         VEC4_SUB(inv_comp, one, quadColor[2]); /* B */
         VEC4_MUL(blend_dest[2], inv_comp, blend_dest[2]); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      {
         float one_minus_alpha[QUAD_SIZE];
         VEC4_SUB(one_minus_alpha, one, quadColor[3]);
         VEC4_MUL(blend_dest[0], blend_dest[0], one_minus_alpha); /* R */
         VEC4_MUL(blend_dest[1], blend_dest[1], one_minus_alpha); /* G */
         VEC4_MUL(blend_dest[2], blend_dest[2], one_minus_alpha); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      {
         float inv_comp[4];
         VEC4_SUB(inv_comp, one, blend_dest[3]); /* A */
         VEC4_MUL(blend_dest[0], inv_comp, blend_dest[0]); /* R */
         VEC4_MUL(blend_dest[1], inv_comp, blend_dest[1]); /* G */
         VEC4_MUL(blend_dest[2], inv_comp, blend_dest[2]); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      {
         float inv_comp[4];
         VEC4_SUB(inv_comp, one, blend_dest[0]); /* R */
         VEC4_MUL(blend_dest[0], blend_dest[0], inv_comp); /* R */
         VEC4_SUB(inv_comp, one, blend_dest[1]); /* G */
         VEC4_MUL(blend_dest[1], blend_dest[1], inv_comp); /* G */
         VEC4_SUB(inv_comp, one, blend_dest[2]); /* B */
         VEC4_MUL(blend_dest[2], blend_dest[2], inv_comp); /* B */
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      {
         float inv_comp[4];
         /* R */
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[0]);
         VEC4_MUL(blend_dest[0], blend_dest[0], inv_comp);
         /* G */
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[1]);
         VEC4_MUL(blend_dest[1], blend_dest[1], inv_comp);
         /* B */
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[2]);
         VEC4_MUL(blend_dest[2], blend_dest[2], inv_comp);
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      {
         float inv_comp[4];
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[3]);
         VEC4_MUL(blend_dest[0], blend_dest[0], inv_comp);
         VEC4_MUL(blend_dest[1], blend_dest[1], inv_comp);
         VEC4_MUL(blend_dest[2], blend_dest[2], inv_comp);
      }
      break;
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
      /* XXX what are these? */
      assert(0);
      break;
   default:
      assert(0 && "invalid rgb dst factor");
   }

   /*
    * Compute blend_dest/second term A
    */
   switch (softpipe->blend->rt[blend_index].alpha_dst_factor) {
   case PIPE_BLENDFACTOR_ONE:
      /* blend_dest = blend_dest * 1   NO-OP, leave blend_dest as-is */
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      VEC4_MUL(blend_dest[3], blend_dest[3], quadColor[3]); /* A * A */
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_DST_ALPHA:
      VEC4_MUL(blend_dest[3], blend_dest[3], blend_dest[3]); /* A */
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      /* blend_dest = blend_dest * 1   NO-OP, leave blend_dest as-is */
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      {
         float comp[4];
         VEC4_SCALAR(comp, const_blend_color[3]); /* A */
         VEC4_MUL(blend_dest[3], blend_dest[3], comp); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_ZERO:
      VEC4_COPY(blend_dest[3], zero); /* A */
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      {
         float one_minus_alpha[QUAD_SIZE];
         VEC4_SUB(one_minus_alpha, one, quadColor[3]);
         VEC4_MUL(blend_dest[3], blend_dest[3], one_minus_alpha); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      {
         float inv_comp[4];
         VEC4_SUB(inv_comp, one, blend_dest[3]); /* A */
         VEC4_MUL(blend_dest[3], inv_comp, blend_dest[3]); /* A */
      }
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* fall-through */
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      {
         float inv_comp[4];
         VEC4_SCALAR(inv_comp, 1.0f - const_blend_color[3]);
         VEC4_MUL(blend_dest[3], blend_dest[3], inv_comp);
      }
      break;
   default:
      assert(0 && "invalid alpha dst factor");
   }

   /*
    * Combine RGB terms
    */
   switch (softpipe->blend->rt[blend_index].rgb_func) {
   case PIPE_BLEND_ADD:
      VEC4_ADD(quadColor[0], source[0], blend_dest[0]); /* R */
      VEC4_ADD(quadColor[1], source[1], blend_dest[1]); /* G */
      VEC4_ADD(quadColor[2], source[2], blend_dest[2]); /* B */
      break;
   case PIPE_BLEND_SUBTRACT:
      VEC4_SUB(quadColor[0], source[0], blend_dest[0]); /* R */
      VEC4_SUB(quadColor[1], source[1], blend_dest[1]); /* G */
      VEC4_SUB(quadColor[2], source[2], blend_dest[2]); /* B */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      VEC4_SUB(quadColor[0], blend_dest[0], source[0]); /* R */
      VEC4_SUB(quadColor[1], blend_dest[1], source[1]); /* G */
      VEC4_SUB(quadColor[2], blend_dest[2], source[2]); /* B */
      break;
   case PIPE_BLEND_MIN:
      VEC4_MIN(quadColor[0], source[0], blend_dest[0]); /* R */
      VEC4_MIN(quadColor[1], source[1], blend_dest[1]); /* G */
      VEC4_MIN(quadColor[2], source[2], blend_dest[2]); /* B */
      break;
   case PIPE_BLEND_MAX:
      VEC4_MAX(quadColor[0], source[0], blend_dest[0]); /* R */
      VEC4_MAX(quadColor[1], source[1], blend_dest[1]); /* G */
      VEC4_MAX(quadColor[2], source[2], blend_dest[2]); /* B */
      break;
   default:
      assert(0 && "invalid rgb blend func");
   }

   /*
    * Combine A terms
    */
   switch (softpipe->blend->rt[blend_index].alpha_func) {
   case PIPE_BLEND_ADD:
      VEC4_ADD(quadColor[3], source[3], blend_dest[3]); /* A */
      break;
   case PIPE_BLEND_SUBTRACT:
      VEC4_SUB(quadColor[3], source[3], blend_dest[3]); /* A */
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      VEC4_SUB(quadColor[3], blend_dest[3], source[3]); /* A */
      break;
   case PIPE_BLEND_MIN:
      VEC4_MIN(quadColor[3], source[3], blend_dest[3]); /* A */
      break;
   case PIPE_BLEND_MAX:
      VEC4_MAX(quadColor[3], source[3], blend_dest[3]); /* A */
      break;
   default:
      assert(0 && "invalid alpha blend func");
   }
}

static void
colormask_quad(unsigned colormask,
               float (*quadColor)[4],
               float (*dest)[4])
{
   /* R */
   if (!(colormask & PIPE_MASK_R))
      COPY_4V(quadColor[0], dest[0]);

   /* G */
   if (!(colormask & PIPE_MASK_G))
      COPY_4V(quadColor[1], dest[1]);

   /* B */
   if (!(colormask & PIPE_MASK_B))
      COPY_4V(quadColor[2], dest[2]);

   /* A */
   if (!(colormask & PIPE_MASK_A))
      COPY_4V(quadColor[3], dest[3]);
}


/**
 * Clamp all colors in a quad to [0, 1]
 */
static void
clamp_colors(float (*quadColor)[4])
{
   unsigned i, j;

   for (j = 0; j < QUAD_SIZE; j++) {
      for (i = 0; i < 4; i++) {
         quadColor[i][j] = CLAMP(quadColor[i][j], 0.0F, 1.0F);
      }
   }
}


/**
 * If we're drawing to a luminance, luminance/alpha or intensity surface
 * we have to adjust (rebase) the fragment/quad colors before writing them
 * to the tile cache.  The tile cache always stores RGBA colors but if
 * we're caching a L/A surface (for example) we need to be sure that R=G=B
 * so that subsequent reads from the surface cache appear to return L/A
 * values.
 * The piglit fbo-blending-formats test will exercise this.
 */
static void
rebase_colors(enum format base_format, float (*quadColor)[4])
{
   unsigned i;

   switch (base_format) {
   case RGB:
      for (i = 0; i < 4; i++) {
         /* A = 1 */
         quadColor[3][i] = 1.0F;
      }
      break;
   case LUMINANCE:
      for (i = 0; i < 4; i++) {
         /* B = G = R */
         quadColor[2][i] = quadColor[1][i] = quadColor[0][i];
         /* A = 1 */
         quadColor[3][i] = 1.0F;
      }
      break;
   case LUMINANCE_ALPHA:
      for (i = 0; i < 4; i++) {
         /* B = G = R */
         quadColor[2][i] = quadColor[1][i] = quadColor[0][i];
      }
      break;
   case INTENSITY:
      for (i = 0; i < 4; i++) {
         /* A = B = G = R */
         quadColor[3][i] = quadColor[2][i] = quadColor[1][i] = quadColor[0][i];
      }
      break;
   default:
      ; /* nothing */
   }
}



static void
blend_fallback(struct quad_stage *qs, 
               struct quad_header *quads[],
               unsigned nr)
{
   const struct blend_quad_stage *bqs = blend_quad_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;
   const struct pipe_blend_state *blend = softpipe->blend;
   unsigned cbuf;
   boolean write_all;

   write_all = softpipe->fs_variant->info.color0_writes_all_cbufs;

   for (cbuf = 0; cbuf < softpipe->framebuffer.nr_cbufs; cbuf++) 
   {
      /* which blend/mask state index to use: */
      const uint blend_buf = blend->independent_blend_enable ? cbuf : 0;
      float dest[4][QUAD_SIZE];
      struct softpipe_cached_tile *tile
         = sp_get_cached_tile(softpipe->cbuf_cache[cbuf],
                              quads[0]->input.x0, 
                              quads[0]->input.y0);
      const boolean clamp = bqs->clamp[cbuf];
      const float *blend_color;
      uint q, i, j;

      if (clamp)
         blend_color = softpipe->blend_color_clamped.color;
      else
         blend_color = softpipe->blend_color.color;

      for (q = 0; q < nr; q++) {
         struct quad_header *quad = quads[q];
         float (*quadColor)[4];
         float temp_quad_color[QUAD_SIZE][4];
         const int itx = (quad->input.x0 & (TILE_SIZE-1));
         const int ity = (quad->input.y0 & (TILE_SIZE-1));

         if (write_all) {
            for (j = 0; j < QUAD_SIZE; j++) {
               for (i = 0; i < 4; i++) {
                  temp_quad_color[i][j] = quad->output.color[0][i][j];
               }
            }
            quadColor = temp_quad_color;
         } else {
            quadColor = quad->output.color[cbuf];
         }

         /* If fixed-point dest color buffer, need to clamp the incoming
          * fragment colors now.
          */
         if (clamp || softpipe->rasterizer->clamp_fragment_color) {
            clamp_colors(quadColor);
         }

         /* get/swizzle dest colors
          */
         for (j = 0; j < QUAD_SIZE; j++) {
            int x = itx + (j & 1);
            int y = ity + (j >> 1);
            for (i = 0; i < 4; i++) {
               dest[i][j] = tile->data.color[y][x][i];
            }
         }


         if (blend->logicop_enable) {
            if (bqs->format_type[cbuf] != UTIL_FORMAT_TYPE_FLOAT) {
               logicop_quad( qs, quadColor, dest );
            }
         }
         else if (blend->rt[blend_buf].blend_enable) {
            blend_quad(qs, quadColor, dest, blend_color, blend_buf);

            /* If fixed-point dest color buffer, need to clamp the outgoing
             * fragment colors now.
             */
            if (clamp) {
               clamp_colors(quadColor);
            }
         }

         rebase_colors(bqs->base_format[cbuf], quadColor);

         if (blend->rt[blend_buf].colormask != 0xf)
            colormask_quad( blend->rt[cbuf].colormask, quadColor, dest);
   
         /* Output color values
          */
         for (j = 0; j < QUAD_SIZE; j++) {
            if (quad->inout.mask & (1 << j)) {
               int x = itx + (j & 1);
               int y = ity + (j >> 1);
               for (i = 0; i < 4; i++) { /* loop over color chans */
                  tile->data.color[y][x][i] = quadColor[i][j];
               }
            }
         }
      }
   }
}


static void
blend_single_add_src_alpha_inv_src_alpha(struct quad_stage *qs, 
                                         struct quad_header *quads[],
                                         unsigned nr)
{
   const struct blend_quad_stage *bqs = blend_quad_stage(qs);
   static const float one[4] = { 1, 1, 1, 1 };
   float one_minus_alpha[QUAD_SIZE];
   float dest[4][QUAD_SIZE];
   float source[4][QUAD_SIZE];
   uint i, j, q;

   struct softpipe_cached_tile *tile
      = sp_get_cached_tile(qs->softpipe->cbuf_cache[0],
                           quads[0]->input.x0, 
                           quads[0]->input.y0);

   for (q = 0; q < nr; q++) {
      struct quad_header *quad = quads[q];
      float (*quadColor)[4] = quad->output.color[0];
      const float *alpha = quadColor[3];
      const int itx = (quad->input.x0 & (TILE_SIZE-1));
      const int ity = (quad->input.y0 & (TILE_SIZE-1));
      
      /* get/swizzle dest colors */
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = itx + (j & 1);
         int y = ity + (j >> 1);
         for (i = 0; i < 4; i++) {
            dest[i][j] = tile->data.color[y][x][i];
         }
      }

      /* If fixed-point dest color buffer, need to clamp the incoming
       * fragment colors now.
       */
      if (bqs->clamp[0] || qs->softpipe->rasterizer->clamp_fragment_color) {
         clamp_colors(quadColor);
      }

      VEC4_MUL(source[0], quadColor[0], alpha); /* R */
      VEC4_MUL(source[1], quadColor[1], alpha); /* G */
      VEC4_MUL(source[2], quadColor[2], alpha); /* B */
      VEC4_MUL(source[3], quadColor[3], alpha); /* A */

      VEC4_SUB(one_minus_alpha, one, alpha);
      VEC4_MUL(dest[0], dest[0], one_minus_alpha); /* R */
      VEC4_MUL(dest[1], dest[1], one_minus_alpha); /* G */
      VEC4_MUL(dest[2], dest[2], one_minus_alpha); /* B */
      VEC4_MUL(dest[3], dest[3], one_minus_alpha); /* A */

      VEC4_ADD(quadColor[0], source[0], dest[0]); /* R */
      VEC4_ADD(quadColor[1], source[1], dest[1]); /* G */
      VEC4_ADD(quadColor[2], source[2], dest[2]); /* B */
      VEC4_ADD(quadColor[3], source[3], dest[3]); /* A */

      /* If fixed-point dest color buffer, need to clamp the outgoing
       * fragment colors now.
       */
      if (bqs->clamp[0]) {
         clamp_colors(quadColor);
      }

      rebase_colors(bqs->base_format[0], quadColor);

      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->inout.mask & (1 << j)) {
            int x = itx + (j & 1);
            int y = ity + (j >> 1);
            for (i = 0; i < 4; i++) { /* loop over color chans */
               tile->data.color[y][x][i] = quadColor[i][j];
            }
         }
      }
   }
}

static void
blend_single_add_one_one(struct quad_stage *qs, 
                         struct quad_header *quads[],
                         unsigned nr)
{
   const struct blend_quad_stage *bqs = blend_quad_stage(qs);
   float dest[4][QUAD_SIZE];
   uint i, j, q;

   struct softpipe_cached_tile *tile
      = sp_get_cached_tile(qs->softpipe->cbuf_cache[0],
                           quads[0]->input.x0, 
                           quads[0]->input.y0);

   for (q = 0; q < nr; q++) {
      struct quad_header *quad = quads[q];
      float (*quadColor)[4] = quad->output.color[0];
      const int itx = (quad->input.x0 & (TILE_SIZE-1));
      const int ity = (quad->input.y0 & (TILE_SIZE-1));
      
      /* get/swizzle dest colors */
      for (j = 0; j < QUAD_SIZE; j++) {
         int x = itx + (j & 1);
         int y = ity + (j >> 1);
         for (i = 0; i < 4; i++) {
            dest[i][j] = tile->data.color[y][x][i];
         }
      }
     
      /* If fixed-point dest color buffer, need to clamp the incoming
       * fragment colors now.
       */
      if (bqs->clamp[0] || qs->softpipe->rasterizer->clamp_fragment_color) {
         clamp_colors(quadColor);
      }

      VEC4_ADD(quadColor[0], quadColor[0], dest[0]); /* R */
      VEC4_ADD(quadColor[1], quadColor[1], dest[1]); /* G */
      VEC4_ADD(quadColor[2], quadColor[2], dest[2]); /* B */
      VEC4_ADD(quadColor[3], quadColor[3], dest[3]); /* A */

      /* If fixed-point dest color buffer, need to clamp the outgoing
       * fragment colors now.
       */
      if (bqs->clamp[0]) {
         clamp_colors(quadColor);
      }

      rebase_colors(bqs->base_format[0], quadColor);

      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->inout.mask & (1 << j)) {
            int x = itx + (j & 1);
            int y = ity + (j >> 1);
            for (i = 0; i < 4; i++) { /* loop over color chans */
               tile->data.color[y][x][i] = quadColor[i][j];
            }
         }
      }
   }
}


/**
 * Just copy the quad color to the framebuffer tile (respecting the writemask),
 * for one color buffer.
 * Clamping will be done, if needed (depending on the color buffer's
 * datatype) when we write/pack the colors later.
 */
static void
single_output_color(struct quad_stage *qs, 
                    struct quad_header *quads[],
                    unsigned nr)
{
   const struct blend_quad_stage *bqs = blend_quad_stage(qs);
   uint i, j, q;

   struct softpipe_cached_tile *tile
      = sp_get_cached_tile(qs->softpipe->cbuf_cache[0],
                           quads[0]->input.x0, 
                           quads[0]->input.y0);

   for (q = 0; q < nr; q++) {
      struct quad_header *quad = quads[q];
      float (*quadColor)[4] = quad->output.color[0];
      const int itx = (quad->input.x0 & (TILE_SIZE-1));
      const int ity = (quad->input.y0 & (TILE_SIZE-1));

      if (qs->softpipe->rasterizer->clamp_fragment_color)
         clamp_colors(quadColor);

      rebase_colors(bqs->base_format[0], quadColor);

      for (j = 0; j < QUAD_SIZE; j++) {
         if (quad->inout.mask & (1 << j)) {
            int x = itx + (j & 1);
            int y = ity + (j >> 1);
            for (i = 0; i < 4; i++) { /* loop over color chans */
               tile->data.color[y][x][i] = quadColor[i][j];
            }
         }
      }
   }
}

static void
blend_noop(struct quad_stage *qs, 
           struct quad_header *quads[],
           unsigned nr)
{
}


static void
choose_blend_quad(struct quad_stage *qs, 
                  struct quad_header *quads[],
                  unsigned nr)
{
   struct blend_quad_stage *bqs = blend_quad_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;
   const struct pipe_blend_state *blend = softpipe->blend;
   unsigned i;

   qs->run = blend_fallback;
   
   if (softpipe->framebuffer.nr_cbufs == 0) {
      qs->run = blend_noop;
   }
   else if (!softpipe->blend->logicop_enable &&
            softpipe->blend->rt[0].colormask == 0xf &&
            softpipe->framebuffer.nr_cbufs == 1)
   {
      if (!blend->rt[0].blend_enable) {
         qs->run = single_output_color;
      }
      else if (blend->rt[0].rgb_src_factor == blend->rt[0].alpha_src_factor &&
               blend->rt[0].rgb_dst_factor == blend->rt[0].alpha_dst_factor &&
               blend->rt[0].rgb_func == blend->rt[0].alpha_func)
      {
         if (blend->rt[0].alpha_func == PIPE_BLEND_ADD) {
            if (blend->rt[0].rgb_src_factor == PIPE_BLENDFACTOR_ONE &&
                blend->rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_ONE) {
               qs->run = blend_single_add_one_one;
            }
            else if (blend->rt[0].rgb_src_factor == PIPE_BLENDFACTOR_SRC_ALPHA &&
                blend->rt[0].rgb_dst_factor == PIPE_BLENDFACTOR_INV_SRC_ALPHA)
               qs->run = blend_single_add_src_alpha_inv_src_alpha;

         }
      }
   }

   /* For each color buffer, determine if the buffer has destination alpha and
    * whether color clamping is needed.
    */
   for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++) {
      const enum pipe_format format = softpipe->framebuffer.cbufs[i]->format;
      const struct util_format_description *desc =
         util_format_description(format);
      /* assuming all or no color channels are normalized: */
      bqs->clamp[i] = desc->channel[0].normalized;
      bqs->format_type[i] = desc->channel[0].type;

      if (util_format_is_intensity(format))
         bqs->base_format[i] = INTENSITY;
      else if (util_format_is_luminance(format))
         bqs->base_format[i] = LUMINANCE;
      else if (util_format_is_luminance_alpha(format))
         bqs->base_format[i] = LUMINANCE_ALPHA;
      else if (util_format_is_rgb_no_alpha(format))
         bqs->base_format[i] = RGB;
      else
         bqs->base_format[i] = RGBA;
   }

   qs->run(qs, quads, nr);
}


static void blend_begin(struct quad_stage *qs)
{
   qs->run = choose_blend_quad;
}


static void blend_destroy(struct quad_stage *qs)
{
   FREE( qs );
}


struct quad_stage *sp_quad_blend_stage( struct softpipe_context *softpipe )
{
   struct blend_quad_stage *stage = CALLOC_STRUCT(blend_quad_stage);

   if (!stage)
      return NULL;

   stage->base.softpipe = softpipe;
   stage->base.begin = blend_begin;
   stage->base.run = choose_blend_quad;
   stage->base.destroy = blend_destroy;

   return &stage->base;
}
