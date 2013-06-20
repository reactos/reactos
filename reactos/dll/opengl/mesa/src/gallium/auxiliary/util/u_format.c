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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Pixel format accessor functions.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "u_math.h"
#include "u_memory.h"
#include "u_rect.h"
#include "u_format.h"
#include "u_format_s3tc.h"

#include "pipe/p_defines.h"


boolean
util_format_is_float(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);
   unsigned i;

   assert(desc);
   if (!desc) {
      return FALSE;
   }

   i = util_format_get_first_non_void_channel(format);
   if (i == -1) {
      return FALSE;
   }

   return desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT ? TRUE : FALSE;
}


/**
 * Return the number of logical channels in the given format by
 * examining swizzles.
 * XXX this could be made into a public function if useful elsewhere.
 */
static unsigned
nr_logical_channels(const struct util_format_description *desc)
{
   boolean swizzle_used[UTIL_FORMAT_SWIZZLE_MAX];

   memset(swizzle_used, 0, sizeof(swizzle_used));

   swizzle_used[desc->swizzle[0]] = TRUE;
   swizzle_used[desc->swizzle[1]] = TRUE;
   swizzle_used[desc->swizzle[2]] = TRUE;
   swizzle_used[desc->swizzle[3]] = TRUE;

   return (swizzle_used[UTIL_FORMAT_SWIZZLE_X] +
           swizzle_used[UTIL_FORMAT_SWIZZLE_Y] +
           swizzle_used[UTIL_FORMAT_SWIZZLE_Z] +
           swizzle_used[UTIL_FORMAT_SWIZZLE_W]);
}


/** Test if the format contains RGB, but not alpha */
boolean
util_format_is_rgb_no_alpha(enum pipe_format format)
{
   const struct util_format_description *desc =
      util_format_description(format);

   if ((desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) &&
       nr_logical_channels(desc) == 3) {
      return TRUE;
   }
   return FALSE;
}


boolean
util_format_is_luminance(enum pipe_format format)
{
   const struct util_format_description *desc =
      util_format_description(format);

   if ((desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) &&
       desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[1] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[2] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[3] == UTIL_FORMAT_SWIZZLE_1) {
      return TRUE;
   }
   return FALSE;
}

boolean
util_format_is_pure_integer(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);
   int i;

   /* Find the first non-void channel. */
   i = util_format_get_first_non_void_channel(format);
   if (i == -1)
      return FALSE;

   return desc->channel[i].pure_integer ? TRUE : FALSE;
}

boolean
util_format_is_pure_sint(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);
   int i;

   i = util_format_get_first_non_void_channel(format);
   if (i == -1)
      return FALSE;

   return (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED && desc->channel[i].pure_integer) ? TRUE : FALSE;
}

boolean
util_format_is_pure_uint(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);
   int i;

   i = util_format_get_first_non_void_channel(format);
   if (i == -1)
      return FALSE;

   return (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED && desc->channel[i].pure_integer) ? TRUE : FALSE;
}

boolean
util_format_is_luminance_alpha(enum pipe_format format)
{
   const struct util_format_description *desc =
      util_format_description(format);

   if ((desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) &&
       desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[1] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[2] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[3] == UTIL_FORMAT_SWIZZLE_Y) {
      return TRUE;
   }
   return FALSE;
}


boolean
util_format_is_intensity(enum pipe_format format)
{
   const struct util_format_description *desc =
      util_format_description(format);

   if ((desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
        desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) &&
       desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[1] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[2] == UTIL_FORMAT_SWIZZLE_X &&
       desc->swizzle[3] == UTIL_FORMAT_SWIZZLE_X) {
      return TRUE;
   }
   return FALSE;
}


boolean
util_format_is_supported(enum pipe_format format, unsigned bind)
{
   if (util_format_is_s3tc(format) && !util_format_s3tc_enabled) {
      return FALSE;
   }

#ifndef TEXTURE_FLOAT_ENABLED
   if ((bind & PIPE_BIND_RENDER_TARGET) &&
       format != PIPE_FORMAT_R9G9B9E5_FLOAT &&
       format != PIPE_FORMAT_R11G11B10_FLOAT &&
       util_format_is_float(format)) {
      return FALSE;
   }
#endif

   return TRUE;
}


void
util_format_read_4f(enum pipe_format format,
                    float *dst, unsigned dst_stride,
                    const void *src, unsigned src_stride,
                    unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   const uint8_t *src_row;
   float *dst_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   src_row = (const uint8_t *)src + y*src_stride + x*(format_desc->block.bits/8);
   dst_row = dst;

   format_desc->unpack_rgba_float(dst_row, dst_stride, src_row, src_stride, w, h);
}


void
util_format_write_4f(enum pipe_format format,
                     const float *src, unsigned src_stride,
                     void *dst, unsigned dst_stride,
                     unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   uint8_t *dst_row;
   const float *src_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   dst_row = (uint8_t *)dst + y*dst_stride + x*(format_desc->block.bits/8);
   src_row = src;

   format_desc->pack_rgba_float(dst_row, dst_stride, src_row, src_stride, w, h);
}


void
util_format_read_4ub(enum pipe_format format, uint8_t *dst, unsigned dst_stride, const void *src, unsigned src_stride, unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   const uint8_t *src_row;
   uint8_t *dst_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   src_row = (const uint8_t *)src + y*src_stride + x*(format_desc->block.bits/8);
   dst_row = dst;

   format_desc->unpack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, w, h);
}


void
util_format_write_4ub(enum pipe_format format, const uint8_t *src, unsigned src_stride, void *dst, unsigned dst_stride, unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   uint8_t *dst_row;
   const uint8_t *src_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   dst_row = (uint8_t *)dst + y*dst_stride + x*(format_desc->block.bits/8);
   src_row = src;

   format_desc->pack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, w, h);
}

void
util_format_read_4ui(enum pipe_format format,
                     unsigned *dst, unsigned dst_stride,
                     const void *src, unsigned src_stride,
                     unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   const uint8_t *src_row;
   unsigned *dst_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   src_row = (const uint8_t *)src + y*src_stride + x*(format_desc->block.bits/8);
   dst_row = dst;

   format_desc->unpack_rgba_uint(dst_row, dst_stride, src_row, src_stride, w, h);
}

void
util_format_write_4ui(enum pipe_format format,
                      const unsigned int *src, unsigned src_stride,
                      void *dst, unsigned dst_stride,
                      unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   uint8_t *dst_row;
   const unsigned *src_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   dst_row = (uint8_t *)dst + y*dst_stride + x*(format_desc->block.bits/8);
   src_row = src;

   format_desc->pack_rgba_uint(dst_row, dst_stride, src_row, src_stride, w, h);
}

void
util_format_read_4i(enum pipe_format format,
                    int *dst, unsigned dst_stride,
                    const void *src, unsigned src_stride,
                    unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   const uint8_t *src_row;
   int *dst_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   src_row = (const uint8_t *)src + y*src_stride + x*(format_desc->block.bits/8);
   dst_row = dst;

   format_desc->unpack_rgba_sint(dst_row, dst_stride, src_row, src_stride, w, h);
}

void
util_format_write_4i(enum pipe_format format,
                      const int *src, unsigned src_stride,
                      void *dst, unsigned dst_stride,
                      unsigned x, unsigned y, unsigned w, unsigned h)
{
   const struct util_format_description *format_desc;
   uint8_t *dst_row;
   const int *src_row;

   format_desc = util_format_description(format);

   assert(x % format_desc->block.width == 0);
   assert(y % format_desc->block.height == 0);

   dst_row = (uint8_t *)dst + y*dst_stride + x*(format_desc->block.bits/8);
   src_row = src;

   format_desc->pack_rgba_sint(dst_row, dst_stride, src_row, src_stride, w, h);
}

boolean
util_is_format_compatible(const struct util_format_description *src_desc,
                          const struct util_format_description *dst_desc)
{
   unsigned chan;

   if (src_desc->format == dst_desc->format) {
      return TRUE;
   }

   if (src_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN ||
       dst_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN) {
      return FALSE;
   }

   if (src_desc->block.bits != dst_desc->block.bits ||
       src_desc->nr_channels != dst_desc->nr_channels ||
       src_desc->colorspace != dst_desc->colorspace) {
      return FALSE;
   }

   for (chan = 0; chan < 4; ++chan) {
      if (src_desc->channel[chan].size !=
          dst_desc->channel[chan].size) {
         return FALSE;
      }
   }

   for (chan = 0; chan < 4; ++chan) {
      enum util_format_swizzle swizzle = dst_desc->swizzle[chan];

      if (swizzle < 4) {
         if (src_desc->swizzle[chan] != swizzle) {
            return FALSE;
         }
         if ((src_desc->channel[swizzle].type !=
              dst_desc->channel[swizzle].type) ||
             (src_desc->channel[swizzle].normalized !=
              dst_desc->channel[swizzle].normalized)) {
            return FALSE;
         }
      }
   }

   return TRUE;
}


boolean
util_format_fits_8unorm(const struct util_format_description *format_desc)
{
   unsigned chan;

   /*
    * After linearized sRGB values require more than 8bits.
    */

   if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      return FALSE;
   }

   switch (format_desc->layout) {

   case UTIL_FORMAT_LAYOUT_S3TC:
      /*
       * These are straight forward.
       */
      return TRUE;
   case UTIL_FORMAT_LAYOUT_RGTC:
      if (format_desc->format == PIPE_FORMAT_RGTC1_SNORM ||
          format_desc->format == PIPE_FORMAT_RGTC2_SNORM ||
          format_desc->format == PIPE_FORMAT_LATC1_SNORM ||
          format_desc->format == PIPE_FORMAT_LATC2_SNORM)
         return FALSE;
      return TRUE;

   case UTIL_FORMAT_LAYOUT_PLAIN:
      /*
       * For these we can find a generic rule.
       */

      for (chan = 0; chan < format_desc->nr_channels; ++chan) {
         switch (format_desc->channel[chan].type) {
         case UTIL_FORMAT_TYPE_VOID:
            break;
         case UTIL_FORMAT_TYPE_UNSIGNED:
            if (!format_desc->channel[chan].normalized ||
                format_desc->channel[chan].size > 8) {
               return FALSE;
            }
            break;
         default:
            return FALSE;
         }
      }
      return TRUE;

   default:
      /*
       * Handle all others on a case by case basis.
       */

      switch (format_desc->format) {
      case PIPE_FORMAT_R1_UNORM:
      case PIPE_FORMAT_UYVY:
      case PIPE_FORMAT_YUYV:
      case PIPE_FORMAT_R8G8_B8G8_UNORM:
      case PIPE_FORMAT_G8R8_G8B8_UNORM:
         return TRUE;

      default:
         return FALSE;
      }
   }
}


void
util_format_translate(enum pipe_format dst_format,
                      void *dst, unsigned dst_stride,
                      unsigned dst_x, unsigned dst_y,
                      enum pipe_format src_format,
                      const void *src, unsigned src_stride,
                      unsigned src_x, unsigned src_y,
                      unsigned width, unsigned height)
{
   const struct util_format_description *dst_format_desc;
   const struct util_format_description *src_format_desc;
   uint8_t *dst_row;
   const uint8_t *src_row;
   unsigned x_step, y_step;
   unsigned dst_step;
   unsigned src_step;

   dst_format_desc = util_format_description(dst_format);
   src_format_desc = util_format_description(src_format);

   if (util_is_format_compatible(src_format_desc, dst_format_desc)) {
      /*
       * Trivial case.
       */

      util_copy_rect(dst, dst_format, dst_stride,  dst_x, dst_y,
                     width, height, src, (int)src_stride,
                     src_x, src_y);
      return;
   }

   assert(dst_x % dst_format_desc->block.width == 0);
   assert(dst_y % dst_format_desc->block.height == 0);
   assert(src_x % src_format_desc->block.width == 0);
   assert(src_y % src_format_desc->block.height == 0);

   dst_row = (uint8_t *)dst + dst_y*dst_stride + dst_x*(dst_format_desc->block.bits/8);
   src_row = (const uint8_t *)src + src_y*src_stride + src_x*(src_format_desc->block.bits/8);

   /*
    * This works because all pixel formats have pixel blocks with power of two
    * sizes.
    */

   y_step = MAX2(dst_format_desc->block.height, src_format_desc->block.height);
   x_step = MAX2(dst_format_desc->block.width, src_format_desc->block.width);
   assert(y_step % dst_format_desc->block.height == 0);
   assert(y_step % src_format_desc->block.height == 0);

   dst_step = y_step / dst_format_desc->block.height * dst_stride;
   src_step = y_step / src_format_desc->block.height * src_stride;

   /*
    * TODO: double formats will loose precision
    * TODO: Add a special case for formats that are mere swizzles of each other
    */

   if (src_format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS ||
       dst_format_desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      float *tmp_z = NULL;
      uint8_t *tmp_s = NULL;

      assert(x_step == 1);
      assert(y_step == 1);

      if (src_format_desc->unpack_z_float &&
          dst_format_desc->pack_z_float) {
         tmp_z = MALLOC(width * sizeof *tmp_z);
      }

      if (src_format_desc->unpack_s_8uint &&
          dst_format_desc->pack_s_8uint) {
         tmp_s = MALLOC(width * sizeof *tmp_s);
      }

      while (height--) {
         if (tmp_z) {
            src_format_desc->unpack_z_float(tmp_z, 0, src_row, src_stride, width, 1);
            dst_format_desc->pack_z_float(dst_row, dst_stride, tmp_z, 0, width, 1);
         }

         if (tmp_s) {
            src_format_desc->unpack_s_8uint(tmp_s, 0, src_row, src_stride, width, 1);
            dst_format_desc->pack_s_8uint(dst_row, dst_stride, tmp_s, 0, width, 1);
         }

         dst_row += dst_step;
         src_row += src_step;
      }

      if (tmp_s) {
         FREE(tmp_s);
      }

      if (tmp_z) {
         FREE(tmp_z);
      }

      return;
   }

   if (util_format_fits_8unorm(src_format_desc) ||
       util_format_fits_8unorm(dst_format_desc)) {
      unsigned tmp_stride;
      uint8_t *tmp_row;

      tmp_stride = MAX2(width, x_step) * 4 * sizeof *tmp_row;
      tmp_row = MALLOC(y_step * tmp_stride);
      if (!tmp_row)
         return;

      while (height >= y_step) {
         src_format_desc->unpack_rgba_8unorm(tmp_row, tmp_stride, src_row, src_stride, width, y_step);
         dst_format_desc->pack_rgba_8unorm(dst_row, dst_stride, tmp_row, tmp_stride, width, y_step);

         dst_row += dst_step;
         src_row += src_step;
         height -= y_step;
      }

      if (height) {
         src_format_desc->unpack_rgba_8unorm(tmp_row, tmp_stride, src_row, src_stride, width, height);
         dst_format_desc->pack_rgba_8unorm(dst_row, dst_stride, tmp_row, tmp_stride, width, height);
      }

      FREE(tmp_row);
   }
   else {
      unsigned tmp_stride;
      float *tmp_row;

      tmp_stride = MAX2(width, x_step) * 4 * sizeof *tmp_row;
      tmp_row = MALLOC(y_step * tmp_stride);
      if (!tmp_row)
         return;

      while (height >= y_step) {
         src_format_desc->unpack_rgba_float(tmp_row, tmp_stride, src_row, src_stride, width, y_step);
         dst_format_desc->pack_rgba_float(dst_row, dst_stride, tmp_row, tmp_stride, width, y_step);

         dst_row += dst_step;
         src_row += src_step;
         height -= y_step;
      }

      if (height) {
         src_format_desc->unpack_rgba_float(tmp_row, tmp_stride, src_row, src_stride, width, height);
         dst_format_desc->pack_rgba_float(dst_row, dst_stride, tmp_row, tmp_stride, width, height);
      }

      FREE(tmp_row);
   }
}

void util_format_compose_swizzles(const unsigned char swz1[4],
                                  const unsigned char swz2[4],
                                  unsigned char dst[4])
{
   unsigned i;

   for (i = 0; i < 4; i++) {
      dst[i] = swz2[i] <= UTIL_FORMAT_SWIZZLE_W ?
               swz1[swz2[i]] : swz2[i];
   }
}

void util_format_swizzle_4f(float *dst, const float *src,
                            const unsigned char swz[4])
{
   unsigned i;

   for (i = 0; i < 4; i++) {
      if (swz[i] <= UTIL_FORMAT_SWIZZLE_W)
         dst[i] = src[swz[i]];
      else if (swz[i] == UTIL_FORMAT_SWIZZLE_0)
         dst[i] = 0;
      else if (swz[i] == UTIL_FORMAT_SWIZZLE_1)
         dst[i] = 1;
   }
}

void util_format_unswizzle_4f(float *dst, const float *src,
                              const unsigned char swz[4])
{
   unsigned i;

   for (i = 0; i < 4; i++) {
      switch (swz[i]) {
      case UTIL_FORMAT_SWIZZLE_X:
         dst[0] = src[i];
         break;
      case UTIL_FORMAT_SWIZZLE_Y:
         dst[1] = src[i];
         break;
      case UTIL_FORMAT_SWIZZLE_Z:
         dst[2] = src[i];
         break;
      case UTIL_FORMAT_SWIZZLE_W:
         dst[3] = src[i];
         break;
      }
   }
}
