/**************************************************************************
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "u_dl.h"
#include "u_math.h"
#include "u_format.h"
#include "u_format_s3tc.h"


#if defined(_WIN32) || defined(WIN32)
#define DXTN_LIBNAME "dxtn.dll"
#elif defined(__APPLE__)
#define DXTN_LIBNAME "libtxc_dxtn.dylib"
#else
#define DXTN_LIBNAME "libtxc_dxtn.so"
#endif


static void
util_format_dxt1_rgb_fetch_stub(int src_stride,
                                const uint8_t *src,
                                int col, int row,
                                uint8_t *dst)
{
   assert(0);
}


static void
util_format_dxt1_rgba_fetch_stub(int src_stride,
                                 const uint8_t *src,
                                 int col, int row,
                                 uint8_t *dst )
{
   assert(0);
}


static void
util_format_dxt3_rgba_fetch_stub(int src_stride,
                                 const uint8_t *src,
                                 int col, int row,
                                 uint8_t *dst )
{
   assert(0);
}


static void
util_format_dxt5_rgba_fetch_stub(int src_stride,
                                 const uint8_t *src,
                                 int col, int row,
                                 uint8_t *dst )
{
   assert(0);
}


static void
util_format_dxtn_pack_stub(int src_comps,
                           int width, int height,
                           const uint8_t *src,
                           enum util_format_dxtn dst_format,
                           uint8_t *dst,
                           int dst_stride)
{
   assert(0);
}


boolean util_format_s3tc_enabled = FALSE;

util_format_dxtn_fetch_t util_format_dxt1_rgb_fetch = util_format_dxt1_rgb_fetch_stub;
util_format_dxtn_fetch_t util_format_dxt1_rgba_fetch = util_format_dxt1_rgba_fetch_stub;
util_format_dxtn_fetch_t util_format_dxt3_rgba_fetch = util_format_dxt3_rgba_fetch_stub;
util_format_dxtn_fetch_t util_format_dxt5_rgba_fetch = util_format_dxt5_rgba_fetch_stub;

util_format_dxtn_pack_t util_format_dxtn_pack = util_format_dxtn_pack_stub;


void
util_format_s3tc_init(void)
{
   static boolean first_time = TRUE;
   struct util_dl_library *library = NULL;
   util_dl_proc fetch_2d_texel_rgb_dxt1;
   util_dl_proc fetch_2d_texel_rgba_dxt1;
   util_dl_proc fetch_2d_texel_rgba_dxt3;
   util_dl_proc fetch_2d_texel_rgba_dxt5;
   util_dl_proc tx_compress_dxtn;

   if (!first_time)
      return;
   first_time = FALSE;

   if (util_format_s3tc_enabled)
      return;

   library = util_dl_open(DXTN_LIBNAME);
   if (!library) {
      if (getenv("force_s3tc_enable") &&
          !strcmp(getenv("force_s3tc_enable"), "true")) {
         debug_printf("couldn't open " DXTN_LIBNAME ", enabling DXTn due to "
            "force_s3tc_enable=true environment variable\n");
         util_format_s3tc_enabled = TRUE;
      } else {
         debug_printf("couldn't open " DXTN_LIBNAME ", software DXTn "
            "compression/decompression unavailable\n");
      }
      return;
   }

   fetch_2d_texel_rgb_dxt1 =
         util_dl_get_proc_address(library, "fetch_2d_texel_rgb_dxt1");
   fetch_2d_texel_rgba_dxt1 =
         util_dl_get_proc_address(library, "fetch_2d_texel_rgba_dxt1");
   fetch_2d_texel_rgba_dxt3 =
         util_dl_get_proc_address(library, "fetch_2d_texel_rgba_dxt3");
   fetch_2d_texel_rgba_dxt5 =
         util_dl_get_proc_address(library, "fetch_2d_texel_rgba_dxt5");
   tx_compress_dxtn =
         util_dl_get_proc_address(library, "tx_compress_dxtn");

   if (!util_format_dxt1_rgb_fetch ||
       !util_format_dxt1_rgba_fetch ||
       !util_format_dxt3_rgba_fetch ||
       !util_format_dxt5_rgba_fetch ||
       !util_format_dxtn_pack) {
      debug_printf("couldn't reference all symbols in " DXTN_LIBNAME
                   ", software DXTn compression/decompression "
                   "unavailable\n");
      util_dl_close(library);
      return;
   }

   util_format_dxt1_rgb_fetch = (util_format_dxtn_fetch_t)fetch_2d_texel_rgb_dxt1;
   util_format_dxt1_rgba_fetch = (util_format_dxtn_fetch_t)fetch_2d_texel_rgba_dxt1;
   util_format_dxt3_rgba_fetch = (util_format_dxtn_fetch_t)fetch_2d_texel_rgba_dxt3;
   util_format_dxt5_rgba_fetch = (util_format_dxtn_fetch_t)fetch_2d_texel_rgba_dxt5;
   util_format_dxtn_pack = (util_format_dxtn_pack_t)tx_compress_dxtn;
   util_format_s3tc_enabled = TRUE;
}


/*
 * Pixel fetch.
 */

void
util_format_dxt1_rgb_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt1_rgb_fetch(0, src, i, j, dst);
}

void
util_format_dxt1_rgba_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt1_rgba_fetch(0, src, i, j, dst);
}

void
util_format_dxt3_rgba_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt3_rgba_fetch(0, src, i, j, dst);
}

void
util_format_dxt5_rgba_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt5_rgba_fetch(0, src, i, j, dst);
}

void
util_format_dxt1_rgb_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   uint8_t tmp[4];
   util_format_dxt1_rgb_fetch(0, src, i, j, tmp);
   dst[0] = ubyte_to_float(tmp[0]);
   dst[1] = ubyte_to_float(tmp[1]);
   dst[2] = ubyte_to_float(tmp[2]);
   dst[3] = 1.0;
}

void
util_format_dxt1_rgba_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   uint8_t tmp[4];
   util_format_dxt1_rgba_fetch(0, src, i, j, tmp);
   dst[0] = ubyte_to_float(tmp[0]);
   dst[1] = ubyte_to_float(tmp[1]);
   dst[2] = ubyte_to_float(tmp[2]);
   dst[3] = ubyte_to_float(tmp[3]);
}

void
util_format_dxt3_rgba_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   uint8_t tmp[4];
   util_format_dxt3_rgba_fetch(0, src, i, j, tmp);
   dst[0] = ubyte_to_float(tmp[0]);
   dst[1] = ubyte_to_float(tmp[1]);
   dst[2] = ubyte_to_float(tmp[2]);
   dst[3] = ubyte_to_float(tmp[3]);
}

void
util_format_dxt5_rgba_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   uint8_t tmp[4];
   util_format_dxt5_rgba_fetch(0, src, i, j, tmp);
   dst[0] = ubyte_to_float(tmp[0]);
   dst[1] = ubyte_to_float(tmp[1]);
   dst[2] = ubyte_to_float(tmp[2]);
   dst[3] = ubyte_to_float(tmp[3]);
}


/*
 * Block decompression.
 */

static INLINE void
util_format_dxtn_rgb_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                        const uint8_t *src_row, unsigned src_stride,
                                        unsigned width, unsigned height,
                                        util_format_dxtn_fetch_t fetch,
                                        unsigned block_size)
{
   const unsigned bw = 4, bh = 4, comps = 4;
   unsigned x, y, i, j;
   for(y = 0; y < height; y += bh) {
      const uint8_t *src = src_row;
      for(x = 0; x < width; x += bw) {
         for(j = 0; j < bh; ++j) {
            for(i = 0; i < bw; ++i) {
               uint8_t *dst = dst_row + (y + j)*dst_stride/sizeof(*dst_row) + (x + i)*comps;
               fetch(0, src, i, j, dst);
            }
         }
         src += block_size;
      }
      src_row += src_stride;
   }
}

void
util_format_dxt1_rgb_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                        const uint8_t *src_row, unsigned src_stride,
                                        unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_8unorm(dst_row, dst_stride,
                                           src_row, src_stride,
                                           width, height,
                                           util_format_dxt1_rgb_fetch, 8);
}

void
util_format_dxt1_rgba_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_8unorm(dst_row, dst_stride,
                                           src_row, src_stride,
                                           width, height,
                                           util_format_dxt1_rgba_fetch, 8);
}

void
util_format_dxt3_rgba_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_8unorm(dst_row, dst_stride,
                                           src_row, src_stride,
                                           width, height,
                                           util_format_dxt3_rgba_fetch, 16);
}

void
util_format_dxt5_rgba_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_8unorm(dst_row, dst_stride,
                                           src_row, src_stride,
                                           width, height,
                                           util_format_dxt5_rgba_fetch, 16);
}

static INLINE void
util_format_dxtn_rgb_unpack_rgba_float(float *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height,
                                       util_format_dxtn_fetch_t fetch,
                                       unsigned block_size)
{
   unsigned x, y, i, j;
   for(y = 0; y < height; y += 4) {
      const uint8_t *src = src_row;
      for(x = 0; x < width; x += 4) {
         for(j = 0; j < 4; ++j) {
            for(i = 0; i < 4; ++i) {
               float *dst = dst_row + (y + j)*dst_stride/sizeof(*dst_row) + (x + i)*4;
               uint8_t tmp[4];
               fetch(0, src, i, j, tmp);
               dst[0] = ubyte_to_float(tmp[0]);
               dst[1] = ubyte_to_float(tmp[1]);
               dst[2] = ubyte_to_float(tmp[2]);
               dst[3] = ubyte_to_float(tmp[3]);
            }
         }
         src += block_size;
      }
      src_row += src_stride;
   }
}

void
util_format_dxt1_rgb_unpack_rgba_float(float *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_float(dst_row, dst_stride,
                                          src_row, src_stride,
                                          width, height,
                                          util_format_dxt1_rgb_fetch, 8);
}

void
util_format_dxt1_rgba_unpack_rgba_float(float *dst_row, unsigned dst_stride,
                                        const uint8_t *src_row, unsigned src_stride,
                                        unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_float(dst_row, dst_stride,
                                          src_row, src_stride,
                                          width, height,
                                          util_format_dxt1_rgba_fetch, 8);
}

void
util_format_dxt3_rgba_unpack_rgba_float(float *dst_row, unsigned dst_stride,
                                        const uint8_t *src_row, unsigned src_stride,
                                        unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_float(dst_row, dst_stride,
                                          src_row, src_stride,
                                          width, height,
                                          util_format_dxt3_rgba_fetch, 16);
}

void
util_format_dxt5_rgba_unpack_rgba_float(float *dst_row, unsigned dst_stride,
                                        const uint8_t *src_row, unsigned src_stride,
                                        unsigned width, unsigned height)
{
   util_format_dxtn_rgb_unpack_rgba_float(dst_row, dst_stride,
                                          src_row, src_stride,
                                          width, height,
                                          util_format_dxt5_rgba_fetch, 16);
}


/*
 * Block compression.
 */

void
util_format_dxt1_rgb_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                      const uint8_t *src, unsigned src_stride,
                                      unsigned width, unsigned height)
{
   const unsigned bw = 4, bh = 4, bytes_per_block = 8;
   unsigned x, y, i, j, k;
   for(y = 0; y < height; y += bh) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += bw) {
         uint8_t tmp[4][4][3];  /* [bh][bw][comps] */
         for(j = 0; j < bh; ++j) {
            for(i = 0; i < bw; ++i) {
               for(k = 0; k < 3; ++k) {
                  tmp[j][i][k] = src[(y + j)*src_stride/sizeof(*src) + (x + i)*4 + k];
               }
            }
         }
         util_format_dxtn_pack(3, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT1_RGB, dst, 0);
         dst += bytes_per_block;
      }
      dst_row += dst_stride / sizeof(*dst_row);
   }
}

void
util_format_dxt1_rgba_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   const unsigned bw = 4, bh = 4, comps = 4, bytes_per_block = 8;
   unsigned x, y, i, j, k;
   for(y = 0; y < height; y += bh) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += bw) {
         uint8_t tmp[4][4][4];  /* [bh][bw][comps] */
         for(j = 0; j < bh; ++j) {
            for(i = 0; i < bw; ++i) {
               for(k = 0; k < comps; ++k) {
                  tmp[j][i][k] = src[(y + j)*src_stride/sizeof(*src) + (x + i)*comps + k];
               }
            }
         }
         util_format_dxtn_pack(4, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT1_RGBA, dst, 0);
         dst += bytes_per_block;
      }
      dst_row += dst_stride / sizeof(*dst_row);
   }
}

void
util_format_dxt3_rgba_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   const unsigned bw = 4, bh = 4, comps = 4, bytes_per_block = 16;
   unsigned x, y, i, j, k;
   for(y = 0; y < height; y += bh) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += bw) {
         uint8_t tmp[4][4][4];  /* [bh][bw][comps] */
         for(j = 0; j < bh; ++j) {
            for(i = 0; i < bw; ++i) {
               for(k = 0; k < comps; ++k) {
                  tmp[j][i][k] = src[(y + j)*src_stride/sizeof(*src) + (x + i)*comps + k];
               }
            }
         }
         util_format_dxtn_pack(4, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT3_RGBA, dst, 0);
         dst += bytes_per_block;
      }
      dst_row += dst_stride / sizeof(*dst_row);
   }
}

void
util_format_dxt5_rgba_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   const unsigned bw = 4, bh = 4, comps = 4, bytes_per_block = 16;
   unsigned x, y, i, j, k;

   for(y = 0; y < height; y += bh) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += bw) {
         uint8_t tmp[4][4][4];  /* [bh][bw][comps] */
         for(j = 0; j < bh; ++j) {
            for(i = 0; i < bw; ++i) {
               for(k = 0; k < comps; ++k) {
                  tmp[j][i][k] = src[(y + j)*src_stride/sizeof(*src) + (x + i)*comps + k];
               }
            }
         }
         util_format_dxtn_pack(4, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT5_RGBA, dst, 0);
         dst += bytes_per_block;
      }
      dst_row += dst_stride / sizeof(*dst_row);
   }
}

void
util_format_dxt1_rgb_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride,
                                     const float *src, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y, i, j, k;
   for(y = 0; y < height; y += 4) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += 4) {
         uint8_t tmp[4][4][3];
         for(j = 0; j < 4; ++j) {
            for(i = 0; i < 4; ++i) {
               for(k = 0; k < 3; ++k) {
                  tmp[j][i][k] = float_to_ubyte(src[(y + j)*src_stride/sizeof(*src) + (x+i)*4 + k]);
               }
            }
         }
         util_format_dxtn_pack(3, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT1_RGB, dst, 0);
         dst += 8;
      }
      dst_row += 4*dst_stride/sizeof(*dst_row);
   }
}

void
util_format_dxt1_rgba_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride,
                                      const float *src, unsigned src_stride,
                                      unsigned width, unsigned height)
{
   unsigned x, y, i, j, k;
   for(y = 0; y < height; y += 4) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += 4) {
         uint8_t tmp[4][4][4];
         for(j = 0; j < 4; ++j) {
            for(i = 0; i < 4; ++i) {
               for(k = 0; k < 4; ++k) {
                  tmp[j][i][k] = float_to_ubyte(src[(y + j)*src_stride/sizeof(*src) + (x+i)*4 + k]);
               }
            }
         }
         util_format_dxtn_pack(4, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT1_RGBA, dst, 0);
         dst += 8;
      }
      dst_row += 4*dst_stride/sizeof(*dst_row);
   }
}

void
util_format_dxt3_rgba_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride,
                                      const float *src, unsigned src_stride,
                                      unsigned width, unsigned height)
{
   unsigned x, y, i, j, k;
   for(y = 0; y < height; y += 4) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += 4) {
         uint8_t tmp[4][4][4];
         for(j = 0; j < 4; ++j) {
            for(i = 0; i < 4; ++i) {
               for(k = 0; k < 4; ++k) {
                  tmp[j][i][k] = float_to_ubyte(src[(y + j)*src_stride/sizeof(*src) + (x+i)*4 + k]);
               }
            }
         }
         util_format_dxtn_pack(4, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT3_RGBA, dst, 0);
         dst += 16;
      }
      dst_row += 4*dst_stride/sizeof(*dst_row);
   }
}

void
util_format_dxt5_rgba_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride,
                                      const float *src, unsigned src_stride,
                                      unsigned width, unsigned height)
{
   unsigned x, y, i, j, k;
   for(y = 0; y < height; y += 4) {
      uint8_t *dst = dst_row;
      for(x = 0; x < width; x += 4) {
         uint8_t tmp[4][4][4];
         for(j = 0; j < 4; ++j) {
            for(i = 0; i < 4; ++i) {
               for(k = 0; k < 4; ++k) {
                  tmp[j][i][k] = float_to_ubyte(src[(y + j)*src_stride/sizeof(*src) + (x+i)*4 + k]);
               }
            }
         }
         util_format_dxtn_pack(4, 4, 4, &tmp[0][0][0], UTIL_FORMAT_DXT5_RGBA, dst, 0);
         dst += 16;
      }
      dst_row += 4*dst_stride/sizeof(*dst_row);
   }
}


/*
 * SRGB variants.
 *
 * FIXME: shunts to RGB for now
 */

void
util_format_dxt1_srgb_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgb_unpack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgb_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgb_pack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgb_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt1_rgb_fetch_rgba_8unorm(dst, src, i, j);
}

void
util_format_dxt1_srgba_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgba_unpack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgba_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgba_pack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgba_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt1_rgba_fetch_rgba_8unorm(dst, src, i, j);
}

void
util_format_dxt3_srgba_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt3_rgba_unpack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt3_srgba_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt3_rgba_pack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt3_srgba_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt3_rgba_fetch_rgba_8unorm(dst, src, i, j);
}

void
util_format_dxt5_srgba_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt5_rgba_unpack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt5_srgba_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt5_rgba_pack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt5_srgba_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt5_rgba_fetch_rgba_8unorm(dst, src, i, j);
}

void
util_format_dxt1_srgb_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgb_unpack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgb_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgb_pack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgb_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt1_rgb_fetch_rgba_float(dst, src, i, j);
}

void
util_format_dxt1_srgba_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgba_unpack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgba_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt1_rgba_pack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt1_srgba_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt1_rgba_fetch_rgba_float(dst, src, i, j);
}

void
util_format_dxt3_srgba_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt3_rgba_unpack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt3_srgba_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt3_rgba_pack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt3_srgba_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt3_rgba_fetch_rgba_float(dst, src, i, j);
}

void
util_format_dxt5_srgba_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt5_rgba_unpack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt5_srgba_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_dxt5_rgba_pack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_dxt5_srgba_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
   util_format_dxt5_rgba_fetch_rgba_float(dst, src, i, j);
}

