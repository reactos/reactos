#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "u_format_etc.h"

/* define etc1_parse_block and etc. */
#define UINT8_TYPE uint8_t
#define TAG(x) x
#include "../../../mesa/main/texcompress_etc_tmp.h"
#undef TAG
#undef UINT8_TYPE

void
util_format_etc1_rgb8_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc1_block block;
   unsigned x, y, i, j;

   for (y = 0; y < height; y += bh) {
      const uint8_t *src = src_row;

      for (x = 0; x < width; x+= bw) {
         etc1_parse_block(&block, src);

         for (j = 0; j < bh; j++) {
            uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
            for (i = 0; i < bw; i++) {
               etc1_fetch_texel(&block, i, j, dst);
               dst[3] = 255;
               dst += comps;
            }
         }

         src += bs;
      }

      src_row += src_stride;
   }
}

void
util_format_etc1_rgb8_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   assert(0);
}

void
util_format_etc1_rgb8_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc1_block block;
   unsigned x, y, i, j;

   for (y = 0; y < height; y += bh) {
      const uint8_t *src = src_row;

      for (x = 0; x < width; x+= bw) {
         etc1_parse_block(&block, src);

         for (j = 0; j < bh; j++) {
            float *dst = dst_row + (y + j) * dst_stride / sizeof(*dst_row) + x * comps;
            uint8_t tmp[3];

            for (i = 0; i < bw; i++) {
               etc1_fetch_texel(&block, i, j, tmp);
               dst[0] = ubyte_to_float(tmp[0]);
               dst[1] = ubyte_to_float(tmp[1]);
               dst[2] = ubyte_to_float(tmp[2]);
               dst[3] = 1.0f;
               dst += comps;
            }
         }

         src += bs;
      }

      src_row += src_stride;
   }
}

void
util_format_etc1_rgb8_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   assert(0);
}

void
util_format_etc1_rgb8_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   const unsigned bw = 4, bh = 4;
   struct etc1_block block;
   uint8_t tmp[3];

   assert(i < bw && j < bh);

   etc1_parse_block(&block, src);
   etc1_fetch_texel(&block, i, j, tmp);

   dst[0] = ubyte_to_float(tmp[0]);
   dst[1] = ubyte_to_float(tmp[1]);
   dst[2] = ubyte_to_float(tmp[2]);
   dst[3] = 1.0f;
}
