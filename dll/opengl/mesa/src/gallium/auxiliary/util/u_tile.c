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
 * RGBA/float tile get/put functions.
 * Usable both by drivers and state trackers.
 */


#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_tile.h"


/**
 * Move raw block of pixels from transfer object to user memory.
 */
void
pipe_get_tile_raw(struct pipe_context *pipe,
                  struct pipe_transfer *pt,
                  uint x, uint y, uint w, uint h,
                  void *dst, int dst_stride)
{
   const void *src;

   if (dst_stride == 0)
      dst_stride = util_format_get_stride(pt->resource->format, w);

   if (u_clip_tile(x, y, &w, &h, &pt->box))
      return;

   src = pipe->transfer_map(pipe, pt);
   assert(src);
   if(!src)
      return;

   util_copy_rect(dst, pt->resource->format, dst_stride, 0, 0, w, h, src, pt->stride, x, y);

   pipe->transfer_unmap(pipe, pt);
}


/**
 * Move raw block of pixels from user memory to transfer object.
 */
void
pipe_put_tile_raw(struct pipe_context *pipe,
                  struct pipe_transfer *pt,
                  uint x, uint y, uint w, uint h,
                  const void *src, int src_stride)
{
   void *dst;
   enum pipe_format format = pt->resource->format;

   if (src_stride == 0)
      src_stride = util_format_get_stride(format, w);

   if (u_clip_tile(x, y, &w, &h, &pt->box))
      return;

   dst = pipe->transfer_map(pipe, pt);
   assert(dst);
   if(!dst)
      return;

   util_copy_rect(dst, format, pt->stride, x, y, w, h, src, src_stride, 0, 0);

   pipe->transfer_unmap(pipe, pt);
}




/** Convert short in [-32768,32767] to GLfloat in [-1.0,1.0] */
#define SHORT_TO_FLOAT(S)   ((2.0F * (S) + 1.0F) * (1.0F/65535.0F))

#define UNCLAMPED_FLOAT_TO_SHORT(us, f)  \
   us = ( (short) ( CLAMP((f), -1.0, 1.0) * 32767.0F) )



/*** PIPE_FORMAT_Z16_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z16_get_tile_rgba(const ushort *src,
                  unsigned w, unsigned h,
                  float *p,
                  unsigned dst_stride)
{
   const float scale = 1.0f / 65535.0f;
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = *src++ * scale;
      }
      p += dst_stride;
   }
}




/*** PIPE_FORMAT_Z32_UNORM ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32_get_tile_rgba(const unsigned *src,
                  unsigned w, unsigned h,
                  float *p,
                  unsigned dst_stride)
{
   const double scale = 1.0 / (double) 0xffffffff;
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float) (*src++ * scale);
      }
      p += dst_stride;
   }
}


/*** PIPE_FORMAT_Z24_UNORM_S8_UINT ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
s8z24_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    float *p,
                    unsigned dst_stride)
{
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float) (scale * (*src++ & 0xffffff));
      }
      p += dst_stride;
   }
}


/*** PIPE_FORMAT_S8_UINT_Z24_UNORM ***/

/**
 * Return Z component as four float in [0,1].  Stencil part ignored.
 */
static void
z24s8_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    float *p,
                    unsigned dst_stride)
{
   const double scale = 1.0 / ((1 << 24) - 1);
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float) (scale * (*src++ >> 8));
      }
      p += dst_stride;
   }
}

/*** PIPE_FORMAT_S8X24_UINT ***/

/**
 * Return S component as four uint32_t in [0..255].  Z part ignored.
 */
static void
s8x24_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    float *p,
                    unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;

      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float)((*src++ >> 24) & 0xff);
      }

      p += dst_stride;
   }
}

/*** PIPE_FORMAT_X24S8_UINT ***/

/**
 * Return S component as four uint32_t in [0..255].  Z part ignored.
 */
static void
x24s8_get_tile_rgba(const unsigned *src,
                    unsigned w, unsigned h,
                    float *p,
                    unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float)(*src++ & 0xff);
      }
      p += dst_stride;
   }
}


/**
 * Return S component as four uint32_t in [0..255].  Z part ignored.
 */
static void
s8_get_tile_rgba(const unsigned char *src,
		 unsigned w, unsigned h,
		 float *p,
		 unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float)(*src++ & 0xff);
      }
      p += dst_stride;
   }
}

/*** PIPE_FORMAT_Z32_FLOAT ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32f_get_tile_rgba(const float *src,
                   unsigned w, unsigned h,
                   float *p,
                   unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = *src++;
      }
      p += dst_stride;
   }
}

/*** PIPE_FORMAT_Z32_FLOAT_S8X24_UINT ***/

/**
 * Return each Z value as four floats in [0,1].
 */
static void
z32f_x24s8_get_tile_rgba(const float *src,
                         unsigned w, unsigned h,
                         float *p,
                         unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = *src;
         src += 2;
      }
      p += dst_stride;
   }
}

/*** PIPE_FORMAT_X32_S8X24_UINT ***/

/**
 * Return S component as four uint32_t in [0..255].  Z part ignored.
 */
static void
x32_s8_get_tile_rgba(const unsigned *src,
                     unsigned w, unsigned h,
                     float *p,
                     unsigned dst_stride)
{
   unsigned i, j;

   for (i = 0; i < h; i++) {
      float *pRow = p;
      for (j = 0; j < w; j++, pRow += 4) {
         src++;
         pRow[0] =
         pRow[1] =
         pRow[2] =
         pRow[3] = (float)(*src++ & 0xff);
      }
      p += dst_stride;
   }
}

void
pipe_tile_raw_to_rgba(enum pipe_format format,
                      void *src,
                      uint w, uint h,
                      float *dst, unsigned dst_stride)
{
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      z16_get_tile_rgba((ushort *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z32_UNORM:
      z32_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
      s8z24_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_S8_UINT:
      s8_get_tile_rgba((unsigned char *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_X24S8_UINT:
      s8x24_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      z24s8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_S8X24_UINT:
      x24s8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      z32f_get_tile_rgba((float *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      z32f_x24s8_get_tile_rgba((float *) src, w, h, dst, dst_stride);
      break;
   case PIPE_FORMAT_X32_S8X24_UINT:
      x32_s8_get_tile_rgba((unsigned *) src, w, h, dst, dst_stride);
      break;
   default:
      util_format_read_4f(format,
                          dst, dst_stride * sizeof(float),
                          src, util_format_get_stride(format, w),
                          0, 0, w, h);
   }
}

void
pipe_tile_raw_to_unsigned(enum pipe_format format,
                          void *src,
                          uint w, uint h,
                          unsigned *dst, unsigned dst_stride)
{
  util_format_read_4ui(format,
                       dst, dst_stride * sizeof(float),
                       src, util_format_get_stride(format, w),
                       0, 0, w, h);
}

void
pipe_tile_raw_to_signed(enum pipe_format format,
                          void *src,
                          uint w, uint h,
                          int *dst, unsigned dst_stride)
{
  util_format_read_4i(format,
                      dst, dst_stride * sizeof(float),
                      src, util_format_get_stride(format, w),
                      0, 0, w, h);
}

void
pipe_get_tile_rgba(struct pipe_context *pipe,
                   struct pipe_transfer *pt,
                   uint x, uint y, uint w, uint h,
                   float *p)
{
   pipe_get_tile_rgba_format(pipe, pt, x, y, w, h, pt->resource->format, p);
}


void
pipe_get_tile_rgba_format(struct pipe_context *pipe,
                          struct pipe_transfer *pt,
                          uint x, uint y, uint w, uint h,
                          enum pipe_format format,
                          float *p)
{
   unsigned dst_stride = w * 4;
   void *packed;

   if (u_clip_tile(x, y, &w, &h, &pt->box)) {
      return;
   }

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));
   if (!packed) {
      return;
   }

   if (format == PIPE_FORMAT_UYVY || format == PIPE_FORMAT_YUYV) {
      assert((x & 1) == 0);
   }

   pipe_get_tile_raw(pipe, pt, x, y, w, h, packed, 0);

   pipe_tile_raw_to_rgba(format, packed, w, h, p, dst_stride);

   FREE(packed);
}


void
pipe_put_tile_rgba(struct pipe_context *pipe,
                   struct pipe_transfer *pt,
                   uint x, uint y, uint w, uint h,
                   const float *p)
{
   pipe_put_tile_rgba_format(pipe, pt, x, y, w, h, pt->resource->format, p);
}


void
pipe_put_tile_rgba_format(struct pipe_context *pipe,
                          struct pipe_transfer *pt,
                          uint x, uint y, uint w, uint h,
                          enum pipe_format format,
                          const float *p)
{
   unsigned src_stride = w * 4;
   void *packed;

   if (u_clip_tile(x, y, &w, &h, &pt->box))
      return;

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));

   if (!packed)
      return;

   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      /*z16_put_tile_rgba((ushort *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_Z32_UNORM:
      /*z32_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
      /*s8z24_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      /*z24s8_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      /*z32f_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      /*z32f_s8x24_put_tile_rgba((unsigned *) packed, w, h, p, src_stride);*/
      break;
   default:
      util_format_write_4f(format,
                           p, src_stride * sizeof(float),
                           packed, util_format_get_stride(format, w),
                           0, 0, w, h);
   }

   pipe_put_tile_raw(pipe, pt, x, y, w, h, packed, 0);

   FREE(packed);
}

void
pipe_put_tile_i_format(struct pipe_context *pipe,
                       struct pipe_transfer *pt,
                       uint x, uint y, uint w, uint h,
                       enum pipe_format format,
                       const int *p)
{
   unsigned src_stride = w * 4;
   void *packed;

   if (u_clip_tile(x, y, &w, &h, &pt->box))
      return;

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));

   if (!packed)
      return;

   util_format_write_4i(format,
                        p, src_stride * sizeof(float),
                        packed, util_format_get_stride(format, w),
                        0, 0, w, h);

   pipe_put_tile_raw(pipe, pt, x, y, w, h, packed, 0);

   FREE(packed);
}

void
pipe_put_tile_ui_format(struct pipe_context *pipe,
                        struct pipe_transfer *pt,
                        uint x, uint y, uint w, uint h,
                        enum pipe_format format,
                        const unsigned int *p)
{
   unsigned src_stride = w * 4;
   void *packed;

   if (u_clip_tile(x, y, &w, &h, &pt->box))
      return;

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));

   if (!packed)
      return;

   util_format_write_4ui(format,
                         p, src_stride * sizeof(float),
                         packed, util_format_get_stride(format, w),
                         0, 0, w, h);

   pipe_put_tile_raw(pipe, pt, x, y, w, h, packed, 0);

   FREE(packed);
}

/**
 * Get a block of Z values, converted to 32-bit range.
 */
void
pipe_get_tile_z(struct pipe_context *pipe,
                struct pipe_transfer *pt,
                uint x, uint y, uint w, uint h,
                uint *z)
{
   const uint dstStride = w;
   ubyte *map;
   uint *pDest = z;
   uint i, j;
   enum pipe_format format = pt->resource->format;

   if (u_clip_tile(x, y, &w, &h, &pt->box))
      return;

   map = (ubyte *)pipe->transfer_map(pipe, pt);
   if (!map) {
      assert(0);
      return;
   }

   switch (format) {
   case PIPE_FORMAT_Z32_UNORM:
      {
         const uint *ptrc
            = (const uint *)(map  + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            memcpy(pDest, ptrc, 4 * w);
            pDest += dstStride;
            ptrc += pt->stride/4;
         }
      }
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
      {
         const uint *ptrc
            = (const uint *)(map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 24-bit Z to 32-bit Z */
               pDest[j] = (ptrc[j] << 8) | ((ptrc[j] >> 16) & 0xff);
            }
            pDest += dstStride;
            ptrc += pt->stride/4;
         }
      }
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      {
         const uint *ptrc
            = (const uint *)(map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 24-bit Z to 32-bit Z */
               pDest[j] = (ptrc[j] & 0xffffff00) | ((ptrc[j] >> 24) & 0xff);
            }
            pDest += dstStride;
            ptrc += pt->stride/4;
         }
      }
      break;
   case PIPE_FORMAT_Z16_UNORM:
      {
         const ushort *ptrc
            = (const ushort *)(map + y * pt->stride + x*2);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 16-bit Z to 32-bit Z */
               pDest[j] = (ptrc[j] << 16) | ptrc[j];
            }
            pDest += dstStride;
            ptrc += pt->stride/2;
         }
      }
      break;
   default:
      assert(0);
   }

   pipe->transfer_unmap(pipe, pt);
}


void
pipe_put_tile_z(struct pipe_context *pipe,
                struct pipe_transfer *pt,
                uint x, uint y, uint w, uint h,
                const uint *zSrc)
{
   const uint srcStride = w;
   const uint *ptrc = zSrc;
   ubyte *map;
   uint i, j;
   enum pipe_format format = pt->resource->format;

   if (u_clip_tile(x, y, &w, &h, &pt->box))
      return;

   map = (ubyte *)pipe->transfer_map(pipe, pt);
   if (!map) {
      assert(0);
      return;
   }

   switch (format) {
   case PIPE_FORMAT_Z32_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            memcpy(pDest, ptrc, 4 * w);
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         /*assert((pt->usage & PIPE_TRANSFER_READ_WRITE) == PIPE_TRANSFER_READ_WRITE);*/
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z, preserve stencil */
               pDest[j] = (pDest[j] & 0xff000000) | ptrc[j] >> 8;
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z (0 stencil) */
               pDest[j] = ptrc[j] >> 8;
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         /*assert((pt->usage & PIPE_TRANSFER_READ_WRITE) == PIPE_TRANSFER_READ_WRITE);*/
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z, preserve stencil */
               pDest[j] = (pDest[j] & 0xff) | (ptrc[j] & 0xffffff00);
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_X8Z24_UNORM:
      {
         uint *pDest = (uint *) (map + y * pt->stride + x*4);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 24-bit Z (0 stencil) */
               pDest[j] = ptrc[j] & 0xffffff00;
            }
            pDest += pt->stride/4;
            ptrc += srcStride;
         }
      }
      break;
   case PIPE_FORMAT_Z16_UNORM:
      {
         ushort *pDest = (ushort *) (map + y * pt->stride + x*2);
         for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
               /* convert 32-bit Z to 16-bit Z */
               pDest[j] = ptrc[j] >> 16;
            }
            pDest += pt->stride/2;
            ptrc += srcStride;
         }
      }
      break;
   default:
      assert(0);
   }

   pipe->transfer_unmap(pipe, pt);
}


void
pipe_get_tile_ui_format(struct pipe_context *pipe,
                        struct pipe_transfer *pt,
                        uint x, uint y, uint w, uint h,
                        enum pipe_format format,
                        unsigned int *p)
{
   unsigned dst_stride = w * 4;
   void *packed;

   if (u_clip_tile(x, y, &w, &h, &pt->box)) {
      return;
   }

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));
   if (!packed) {
      return;
   }

   if (format == PIPE_FORMAT_UYVY || format == PIPE_FORMAT_YUYV) {
      assert((x & 1) == 0);
   }

   pipe_get_tile_raw(pipe, pt, x, y, w, h, packed, 0);

   pipe_tile_raw_to_unsigned(format, packed, w, h, p, dst_stride);

   FREE(packed);
}


void
pipe_get_tile_i_format(struct pipe_context *pipe,
                       struct pipe_transfer *pt,
                       uint x, uint y, uint w, uint h,
                       enum pipe_format format,
                       int *p)
{
   unsigned dst_stride = w * 4;
   void *packed;

   if (u_clip_tile(x, y, &w, &h, &pt->box)) {
      return;
   }

   packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));
   if (!packed) {
      return;
   }

   if (format == PIPE_FORMAT_UYVY || format == PIPE_FORMAT_YUYV) {
      assert((x & 1) == 0);
   }

   pipe_get_tile_raw(pipe, pt, x, y, w, h, packed, 0);

   pipe_tile_raw_to_signed(format, packed, w, h, p, dst_stride);

   FREE(packed);
}
