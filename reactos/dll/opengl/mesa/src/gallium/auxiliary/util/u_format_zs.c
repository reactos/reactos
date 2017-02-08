/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#include "u_debug.h"
#include "u_math.h"
#include "u_format_zs.h"


/*
 * z32_unorm conversion functions
 */

static INLINE uint16_t
z32_unorm_to_z16_unorm(uint32_t z)
{
   /* z * 0xffff / 0xffffffff */
   return z >> 16;
}

static INLINE uint32_t
z16_unorm_to_z32_unorm(uint16_t z)
{
   /* z * 0xffffffff / 0xffff */
   return (z << 16) | z;
}

static INLINE uint32_t
z32_unorm_to_z24_unorm(uint32_t z)
{
   /* z * 0xffffff / 0xffffffff */
   return z >> 8;
}

static INLINE uint32_t
z24_unorm_to_z32_unorm(uint32_t z)
{
   /* z * 0xffffffff / 0xffffff */
   return (z << 8) | (z >> 16);
}


/*
 * z32_float conversion functions
 */

static INLINE uint16_t
z32_float_to_z16_unorm(float z)
{
   const float scale = 0xffff;
   return (uint16_t)(z * scale);
}

static INLINE float
z16_unorm_to_z32_float(uint16_t z)
{
   const float scale = 1.0 / 0xffff;
   return (float)(z * scale);
}

static INLINE uint32_t
z32_float_to_z24_unorm(float z)
{
   const double scale = 0xffffff;
   return (uint32_t)(z * scale) & 0xffffff;
}

static INLINE float
z24_unorm_to_z32_float(uint32_t z)
{
   const double scale = 1.0 / 0xffffff;
   return (float)(z * scale);
}

static INLINE uint32_t
z32_float_to_z32_unorm(float z)
{
   const double scale = 0xffffffff;
   return (uint32_t)(z * scale);
}

static INLINE float
z32_unorm_to_z32_float(uint32_t z)
{
   const double scale = 1.0 / 0xffffffff;
   return (float)(z * scale);
}


void
util_format_s8_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z16_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                     const uint8_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint16_t *src = (const uint16_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint16_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         *dst++ = z16_unorm_to_z32_float(value);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z16_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                   const float *src_row, unsigned src_stride,
                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint16_t *dst = (uint16_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint16_t value;
         value = z32_float_to_z16_unorm(*src++);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z16_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint16_t *src = (const uint16_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint16_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         *dst++ = z16_unorm_to_z32_unorm(value);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z16_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                     const uint32_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint16_t *dst = (uint16_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint16_t value;
         value = z32_unorm_to_z16_unorm(*src++);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                     const uint8_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z32_unorm_to_z32_float(value);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                   const float *src_row, unsigned src_stride,
                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = z32_float_to_z32_unorm(*src++);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                     const uint32_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_unpack_z_float(float *dst_row, unsigned dst_stride,
                                     const uint8_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                   const float *src_row, unsigned src_stride,
                                   unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const float *src = (const float *)src_row;
      for(x = 0; x < width; ++x) {
         *dst++ = z32_float_to_z32_unorm(*src++);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                     const uint32_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      float *dst = (float *)dst_row;
      for(x = 0; x < width; ++x) {
         *dst++ = z32_unorm_to_z32_float(*src++);
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24_unorm_s8_uint_unpack_z_float(float *dst_row, unsigned dst_stride,
                                                const uint8_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_float(value & 0xffffff);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24_unorm_s8_uint_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                              const float *src_row, unsigned src_stride,
                                              unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *dst;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         value &= 0xff000000;
         value |= z32_float_to_z24_unorm(*src++);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24_unorm_s8_uint_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                                  const uint8_t *src_row, unsigned src_stride,
                                                  unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_unorm(value & 0xffffff);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24_unorm_s8_uint_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                                const uint32_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value= *dst;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         value &= 0xff000000;
         value |= z32_unorm_to_z24_unorm(*src++);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24_unorm_s8_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                                   const uint8_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint8_t *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value >> 24;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24_unorm_s8_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                                 const uint8_t *src_row, unsigned src_stride,
                                                 unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint8_t *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *dst;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         value &= 0x00ffffff;
         value |= *src++ << 24;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_s8_uint_z24_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                                const uint8_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_float(value >> 8);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uint_z24_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                              const float *src_row, unsigned src_stride,
                                              unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *dst;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         value &= 0x000000ff;
         value |= z32_float_to_z24_unorm(*src++) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_s8_uint_z24_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                                  const uint8_t *src_row, unsigned src_stride,
                                                  unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_unorm(value >> 8);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uint_z24_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                                const uint32_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *dst;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         value &= 0x000000ff;
         value |= *src++ & 0xffffff00;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_s8_uint_z24_unorm_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                                   const uint8_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint8_t *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value & 0xff;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uint_z24_unorm_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                                 const uint8_t *src_row, unsigned src_stride,
                                                 unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint8_t *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *dst;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         value &= 0xffffff00;
         value |= *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24x8_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_float(value & 0xffffff);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24x8_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                     const float *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = z32_float_to_z24_unorm(*src++);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24x8_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_unorm(value & 0xffffff);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24x8_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                       const uint32_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = z32_unorm_to_z24_unorm(*src++);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_x8z24_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint32_t *src = (uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_float(value >> 8);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_x8z24_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                     const float *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = z32_float_to_z24_unorm(*src++) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_x8z24_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = z24_unorm_to_z32_unorm(value >> 8);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_x8z24_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                       const uint32_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = z32_unorm_to_z24_unorm(*src++) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_float_s8x24_uint_unpack_z_float(float *dst_row, unsigned dst_stride,
                                                   const uint8_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const float *src = (const float *)src_row;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 2;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_s8x24_uint_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                                 const float *src_row, unsigned src_stride,
                                                 unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      float *dst = (float *)dst_row;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 1;
         dst += 2;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_float_s8x24_uint_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                                     const uint8_t *src_row, unsigned src_stride,
                                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const float *src = (const float *)src_row;
      for(x = 0; x < width; ++x) {
         *dst = z32_float_to_z32_unorm(*src);
         src += 2;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_s8x24_uint_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                                   const uint32_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      float *dst = (float *)dst_row;
      for(x = 0; x < width; ++x) {
         *dst++ = z32_unorm_to_z32_float(*src++);
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_float_s8x24_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                                      const uint8_t *src_row, unsigned src_stride,
                                                      unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint8_t *dst = dst_row;
      const uint8_t *src = src_row + 4;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 8;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_s8x24_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
                                                    const uint8_t *src_row, unsigned src_stride,
                                                    unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint8_t *src = src_row;
      uint8_t *dst = dst_row + 4;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 1;
         dst += 8;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}


void
util_format_x24s8_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_z24_unorm_s8_uint_unpack_s_8uint(dst_row, dst_stride,
						      src_row, src_stride,
						      width, height);
}

void
util_format_x24s8_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_z24_unorm_s8_uint_pack_s_8uint(dst_row, dst_stride,
						    src_row, src_stride,
						    width, height);
}

void
util_format_s8x24_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_s8_uint_z24_unorm_unpack_s_8uint(dst_row, dst_stride,
						      src_row, src_stride,
						      width, height);
}

void
util_format_s8x24_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_s8_uint_z24_unorm_pack_s_8uint(dst_row, dst_stride,
						      src_row, src_stride,
						      width, height);
}

void
util_format_x32_s8x24_uint_unpack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
						const uint8_t *src_row, unsigned src_stride,
						unsigned width, unsigned height)
{
   util_format_z32_float_s8x24_uint_unpack_s_8uint(dst_row, dst_stride,
							 src_row, src_stride,
							 width, height);

}

void
util_format_x32_s8x24_uint_pack_s_8uint(uint8_t *dst_row, unsigned dst_stride,
					      const uint8_t *src_row, unsigned src_stride,
					      unsigned width, unsigned height)
{
   util_format_z32_float_s8x24_uint_pack_s_8uint(dst_row, dst_stride,
                                                       src_row, src_stride,
						       width, height);
}
