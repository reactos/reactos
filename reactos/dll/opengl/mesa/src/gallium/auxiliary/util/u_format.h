/**************************************************************************
 *
 * Copyright 2009-2010 Vmware, Inc.
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


#ifndef U_FORMAT_H
#define U_FORMAT_H


#include "pipe/p_format.h"
#include "util/u_debug.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Describe how to pack/unpack pixels into/from the prescribed format.
 *
 * XXX: This could be renamed to something like util_format_pack, or broke down
 * in flags inside util_format_block that said exactly what we want.
 */
enum util_format_layout {
   /**
    * Formats with util_format_block::width == util_format_block::height == 1
    * that can be described as an ordinary data structure.
    */
   UTIL_FORMAT_LAYOUT_PLAIN = 0,

   /**
    * Formats with sub-sampled channels.
    *
    * This is for formats like YV12 where there is less than one sample per
    * pixel.
    */
   UTIL_FORMAT_LAYOUT_SUBSAMPLED = 3,

   /**
    * S3 Texture Compression formats.
    */
   UTIL_FORMAT_LAYOUT_S3TC = 4,

   /**
    * Red-Green Texture Compression formats.
    */
   UTIL_FORMAT_LAYOUT_RGTC = 5,

   /**
    * Ericsson Texture Compression
    */
   UTIL_FORMAT_LAYOUT_ETC = 6,

   /**
    * Everything else that doesn't fit in any of the above layouts.
    */
   UTIL_FORMAT_LAYOUT_OTHER = 7
};


struct util_format_block
{
   /** Block width in pixels */
   unsigned width;
   
   /** Block height in pixels */
   unsigned height;

   /** Block size in bits */
   unsigned bits;
};


enum util_format_type {
   UTIL_FORMAT_TYPE_VOID = 0,
   UTIL_FORMAT_TYPE_UNSIGNED = 1,
   UTIL_FORMAT_TYPE_SIGNED = 2,
   UTIL_FORMAT_TYPE_FIXED = 3,
   UTIL_FORMAT_TYPE_FLOAT = 4
};


enum util_format_swizzle {
   UTIL_FORMAT_SWIZZLE_X = 0,
   UTIL_FORMAT_SWIZZLE_Y = 1,
   UTIL_FORMAT_SWIZZLE_Z = 2,
   UTIL_FORMAT_SWIZZLE_W = 3,
   UTIL_FORMAT_SWIZZLE_0 = 4,
   UTIL_FORMAT_SWIZZLE_1 = 5,
   UTIL_FORMAT_SWIZZLE_NONE = 6,
   UTIL_FORMAT_SWIZZLE_MAX = 7  /**< Number of enums counter (must be last) */
};


enum util_format_colorspace {
   UTIL_FORMAT_COLORSPACE_RGB = 0,
   UTIL_FORMAT_COLORSPACE_SRGB = 1,
   UTIL_FORMAT_COLORSPACE_YUV = 2,
   UTIL_FORMAT_COLORSPACE_ZS = 3
};


struct util_format_channel_description
{
   unsigned type:5;        /**< UTIL_FORMAT_TYPE_x */
   unsigned normalized:1;
   unsigned pure_integer:1;
   unsigned size:9;        /**< bits per channel */
};


struct util_format_description
{
   enum pipe_format format;

   const char *name;

   /**
    * Short name, striped of the prefix, lower case.
    */
   const char *short_name;

   /**
    * Pixel block dimensions.
    */
   struct util_format_block block;

   enum util_format_layout layout;

   /**
    * The number of channels.
    */
   unsigned nr_channels:3;

   /**
    * Whether all channels have the same number of (whole) bytes.
    */
   unsigned is_array:1;

   /**
    * Whether the pixel format can be described as a bitfield structure.
    *
    * In particular:
    * - pixel depth must be 8, 16, or 32 bits;
    * - all channels must be unsigned, signed, or void
    */
   unsigned is_bitmask:1;

   /**
    * Whether channels have mixed types (ignoring UTIL_FORMAT_TYPE_VOID).
    */
   unsigned is_mixed:1;

   /**
    * Input channel description.
    *
    * Only valid for UTIL_FORMAT_LAYOUT_PLAIN formats.
    */
   struct util_format_channel_description channel[4];

   /**
    * Output channel swizzle.
    *
    * The order is either:
    * - RGBA
    * - YUV(A)
    * - ZS
    * depending on the colorspace.
    */
   unsigned char swizzle[4];

   /**
    * Colorspace transformation.
    */
   enum util_format_colorspace colorspace;

   /**
    * Unpack pixel blocks to R8G8B8A8_UNORM.
    * Note: strides are in bytes.
    *
    * Only defined for non-depth-stencil formats.
    */
   void
   (*unpack_rgba_8unorm)(uint8_t *dst, unsigned dst_stride,
                         const uint8_t *src, unsigned src_stride,
                         unsigned width, unsigned height);

   /**
    * Pack pixel blocks from R8G8B8A8_UNORM.
    * Note: strides are in bytes.
    *
    * Only defined for non-depth-stencil formats.
    */
   void
   (*pack_rgba_8unorm)(uint8_t *dst, unsigned dst_stride,
                       const uint8_t *src, unsigned src_stride,
                       unsigned width, unsigned height);

   /**
    * Fetch a single pixel (i, j) from a block.
    *
    * XXX: Only defined for a very few select formats.
    */
   void
   (*fetch_rgba_8unorm)(uint8_t *dst,
                        const uint8_t *src,
                        unsigned i, unsigned j);

   /**
    * Unpack pixel blocks to R32G32B32A32_FLOAT.
    * Note: strides are in bytes.
    *
    * Only defined for non-depth-stencil formats.
    */
   void
   (*unpack_rgba_float)(float *dst, unsigned dst_stride,
                        const uint8_t *src, unsigned src_stride,
                        unsigned width, unsigned height);

   /**
    * Pack pixel blocks from R32G32B32A32_FLOAT.
    * Note: strides are in bytes.
    *
    * Only defined for non-depth-stencil formats.
    */
   void
   (*pack_rgba_float)(uint8_t *dst, unsigned dst_stride,
                      const float *src, unsigned src_stride,
                      unsigned width, unsigned height);

   /**
    * Fetch a single pixel (i, j) from a block.
    *
    * Only defined for non-depth-stencil and non-integer formats.
    */
   void
   (*fetch_rgba_float)(float *dst,
                       const uint8_t *src,
                       unsigned i, unsigned j);

   /**
    * Unpack pixels to Z32_UNORM.
    * Note: strides are in bytes.
    *
    * Only defined for depth formats.
    */
   void
   (*unpack_z_32unorm)(uint32_t *dst, unsigned dst_stride,
                       const uint8_t *src, unsigned src_stride,
                       unsigned width, unsigned height);

   /**
    * Pack pixels from Z32_FLOAT.
    * Note: strides are in bytes.
    *
    * Only defined for depth formats.
    */
   void
   (*pack_z_32unorm)(uint8_t *dst, unsigned dst_stride,
                     const uint32_t *src, unsigned src_stride,
                     unsigned width, unsigned height);

   /**
    * Unpack pixels to Z32_FLOAT.
    * Note: strides are in bytes.
    *
    * Only defined for depth formats.
    */
   void
   (*unpack_z_float)(float *dst, unsigned dst_stride,
                     const uint8_t *src, unsigned src_stride,
                     unsigned width, unsigned height);

   /**
    * Pack pixels from Z32_FLOAT.
    * Note: strides are in bytes.
    *
    * Only defined for depth formats.
    */
   void
   (*pack_z_float)(uint8_t *dst, unsigned dst_stride,
                   const float *src, unsigned src_stride,
                   unsigned width, unsigned height);

   /**
    * Unpack pixels to S8_UINT.
    * Note: strides are in bytes.
    *
    * Only defined for stencil formats.
    */
   void
   (*unpack_s_8uint)(uint8_t *dst, unsigned dst_stride,
                     const uint8_t *src, unsigned src_stride,
                     unsigned width, unsigned height);

   /**
    * Pack pixels from S8_UINT.
    * Note: strides are in bytes.
    *
    * Only defined for stencil formats.
    */
   void
   (*pack_s_8uint)(uint8_t *dst, unsigned dst_stride,
                   const uint8_t *src, unsigned src_stride,
                   unsigned width, unsigned height);

  /**
    * Unpack pixel blocks to R32G32B32A32_UINT.
    * Note: strides are in bytes.
    *
    * Only defined for INT formats.
    */
   void
   (*unpack_rgba_uint)(unsigned *dst, unsigned dst_stride,
                       const uint8_t *src, unsigned src_stride,
                       unsigned width, unsigned height);

   void
   (*pack_rgba_uint)(uint8_t *dst, unsigned dst_stride,
                     const unsigned *src, unsigned src_stride,
                     unsigned width, unsigned height);

  /**
    * Unpack pixel blocks to R32G32B32A32_SINT.
    * Note: strides are in bytes.
    *
    * Only defined for INT formats.
    */
   void
   (*unpack_rgba_sint)(signed *dst, unsigned dst_stride,
                       const uint8_t *src, unsigned src_stride,
                       unsigned width, unsigned height);

   void
   (*pack_rgba_sint)(uint8_t *dst, unsigned dst_stride,
                     const int *src, unsigned src_stride,
                     unsigned width, unsigned height);

   /**
    * Fetch a single pixel (i, j) from a block.
    *
    * Only defined for unsigned (pure) integer formats.
    */
   void
   (*fetch_rgba_uint)(uint32_t *dst,
                      const uint8_t *src,
                      unsigned i, unsigned j);

   /**
    * Fetch a single pixel (i, j) from a block.
    *
    * Only defined for signed (pure) integer formats.
    */
   void
   (*fetch_rgba_sint)(int32_t *dst,
                      const uint8_t *src,
                      unsigned i, unsigned j);
};


extern const struct util_format_description 
util_format_description_table[];


const struct util_format_description *
util_format_description(enum pipe_format format);


/*
 * Format query functions.
 */

static INLINE const char *
util_format_name(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return "PIPE_FORMAT_???";
   }

   return desc->name;
}

static INLINE const char *
util_format_short_name(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return "???";
   }

   return desc->short_name;
}

/**
 * Whether this format is plain, see UTIL_FORMAT_LAYOUT_PLAIN for more info.
 */
static INLINE boolean
util_format_is_plain(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   if (!format) {
      return FALSE;
   }

   return desc->layout == UTIL_FORMAT_LAYOUT_PLAIN ? TRUE : FALSE;
}

static INLINE boolean 
util_format_is_compressed(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return FALSE;
   }

   switch (desc->layout) {
   case UTIL_FORMAT_LAYOUT_S3TC:
   case UTIL_FORMAT_LAYOUT_RGTC:
      /* XXX add other formats in the future */
      return TRUE;
   default:
      return FALSE;
   }
}

static INLINE boolean 
util_format_is_s3tc(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return FALSE;
   }

   return desc->layout == UTIL_FORMAT_LAYOUT_S3TC ? TRUE : FALSE;
}

static INLINE boolean 
util_format_is_srgb(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);
   return desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB;
}

static INLINE boolean
util_format_has_depth(const struct util_format_description *desc)
{
   return desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS &&
          desc->swizzle[0] != UTIL_FORMAT_SWIZZLE_NONE;
}

static INLINE boolean
util_format_has_stencil(const struct util_format_description *desc)
{
   return desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS &&
          desc->swizzle[1] != UTIL_FORMAT_SWIZZLE_NONE;
}

static INLINE boolean
util_format_is_depth_or_stencil(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return FALSE;
   }

   return util_format_has_depth(desc) ||
          util_format_has_stencil(desc);
}

static INLINE boolean
util_format_is_depth_and_stencil(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return FALSE;
   }

   return util_format_has_depth(desc) &&
          util_format_has_stencil(desc);
}


/**
 * Give the RGBA colormask of the channels that can be represented in this
 * format.
 *
 * That is, the channels whose values are preserved.
 */
static INLINE unsigned
util_format_colormask(const struct util_format_description *desc)
{
   unsigned colormask;
   unsigned chan;

   switch (desc->colorspace) {
   case UTIL_FORMAT_COLORSPACE_RGB:
   case UTIL_FORMAT_COLORSPACE_SRGB:
   case UTIL_FORMAT_COLORSPACE_YUV:
      colormask = 0;
      for (chan = 0; chan < 4; ++chan) {
         if (desc->swizzle[chan] < 4) {
            colormask |= (1 << chan);
         }
      }
      return colormask;
   case UTIL_FORMAT_COLORSPACE_ZS:
      return 0;
   default:
      assert(0);
      return 0;
   }
}


boolean
util_format_is_float(enum pipe_format format);


boolean
util_format_is_rgb_no_alpha(enum pipe_format format);


boolean
util_format_is_luminance(enum pipe_format format);


boolean
util_format_is_luminance_alpha(enum pipe_format format);


boolean
util_format_is_intensity(enum pipe_format format);

boolean
util_format_is_pure_integer(enum pipe_format format);

boolean
util_format_is_pure_sint(enum pipe_format format);

boolean
util_format_is_pure_uint(enum pipe_format format);

/**
 * Whether the src format can be blitted to destation format with a simple
 * memcpy.
 */
boolean
util_is_format_compatible(const struct util_format_description *src_desc,
                          const struct util_format_description *dst_desc);

/**
 * Whether the format is supported by Gallium for the given bindings.
 * This covers S3TC textures and floating-point render targets.
 */
boolean
util_format_is_supported(enum pipe_format format, unsigned bind);

/**
 * Whether this format is a rgab8 variant.
 *
 * That is, any format that matches the
 *
 *   PIPE_FORMAT_?8?8?8?8_UNORM
 */
static INLINE boolean
util_format_is_rgba8_variant(const struct util_format_description *desc)
{
   unsigned chan;

   if(desc->block.width != 1 ||
      desc->block.height != 1 ||
      desc->block.bits != 32)
      return FALSE;

   for(chan = 0; chan < 4; ++chan) {
      if(desc->channel[chan].type != UTIL_FORMAT_TYPE_UNSIGNED &&
         desc->channel[chan].type != UTIL_FORMAT_TYPE_VOID)
         return FALSE;
      if(desc->channel[chan].size != 8)
         return FALSE;
   }

   return TRUE;
}


/**
 * Return total bits needed for the pixel format per block.
 */
static INLINE uint
util_format_get_blocksizebits(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return 0;
   }

   return desc->block.bits;
}

/**
 * Return bytes per block (not pixel) for the given format.
 */
static INLINE uint
util_format_get_blocksize(enum pipe_format format)
{
   uint bits = util_format_get_blocksizebits(format);

   assert(bits % 8 == 0);

   return bits / 8;
}

static INLINE uint
util_format_get_blockwidth(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return 1;
   }

   return desc->block.width;
}

static INLINE uint
util_format_get_blockheight(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(desc);
   if (!desc) {
      return 1;
   }

   return desc->block.height;
}

static INLINE unsigned
util_format_get_nblocksx(enum pipe_format format,
                         unsigned x)
{
   unsigned blockwidth = util_format_get_blockwidth(format);
   return (x + blockwidth - 1) / blockwidth;
}

static INLINE unsigned
util_format_get_nblocksy(enum pipe_format format,
                         unsigned y)
{
   unsigned blockheight = util_format_get_blockheight(format);
   return (y + blockheight - 1) / blockheight;
}

static INLINE unsigned
util_format_get_nblocks(enum pipe_format format,
                        unsigned width,
                        unsigned height)
{
   return util_format_get_nblocksx(format, width) * util_format_get_nblocksy(format, height);
}

static INLINE size_t
util_format_get_stride(enum pipe_format format,
                       unsigned width)
{
   return util_format_get_nblocksx(format, width) * util_format_get_blocksize(format);
}

static INLINE size_t
util_format_get_2d_size(enum pipe_format format,
                        size_t stride,
                        unsigned height)
{
   return util_format_get_nblocksy(format, height) * stride;
}

static INLINE uint
util_format_get_component_bits(enum pipe_format format,
                               enum util_format_colorspace colorspace,
                               uint component)
{
   const struct util_format_description *desc = util_format_description(format);
   enum util_format_colorspace desc_colorspace;

   assert(format);
   if (!format) {
      return 0;
   }

   assert(component < 4);

   /* Treat RGB and SRGB as equivalent. */
   if (colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      colorspace = UTIL_FORMAT_COLORSPACE_RGB;
   }
   if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
      desc_colorspace = UTIL_FORMAT_COLORSPACE_RGB;
   } else {
      desc_colorspace = desc->colorspace;
   }

   if (desc_colorspace != colorspace) {
      return 0;
   }

   switch (desc->swizzle[component]) {
   case UTIL_FORMAT_SWIZZLE_X:
      return desc->channel[0].size;
   case UTIL_FORMAT_SWIZZLE_Y:
      return desc->channel[1].size;
   case UTIL_FORMAT_SWIZZLE_Z:
      return desc->channel[2].size;
   case UTIL_FORMAT_SWIZZLE_W:
      return desc->channel[3].size;
   default:
      return 0;
   }
}

static INLINE boolean
util_format_has_alpha(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   assert(format);
   if (!format) {
      return FALSE;
   }

   switch (desc->colorspace) {
   case UTIL_FORMAT_COLORSPACE_RGB:
   case UTIL_FORMAT_COLORSPACE_SRGB:
      return desc->swizzle[3] != UTIL_FORMAT_SWIZZLE_1;
   case UTIL_FORMAT_COLORSPACE_YUV:
      return FALSE;
   case UTIL_FORMAT_COLORSPACE_ZS:
      return FALSE;
   default:
      assert(0);
      return FALSE;
   }
}

/**
 * Given a linear RGB colorspace format, return the corresponding SRGB
 * format, or PIPE_FORMAT_NONE if none.
 */
static INLINE enum pipe_format
util_format_srgb(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_L8_UNORM:
      return PIPE_FORMAT_L8_SRGB;
   case PIPE_FORMAT_L8A8_UNORM:
      return PIPE_FORMAT_L8A8_SRGB;
   case PIPE_FORMAT_R8G8B8_UNORM:
      return PIPE_FORMAT_R8G8B8_SRGB;
   case PIPE_FORMAT_A8B8G8R8_UNORM:
      return PIPE_FORMAT_A8B8G8R8_SRGB;
   case PIPE_FORMAT_X8B8G8R8_UNORM:
      return PIPE_FORMAT_X8B8G8R8_SRGB;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return PIPE_FORMAT_B8G8R8A8_SRGB;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return PIPE_FORMAT_B8G8R8X8_SRGB;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return PIPE_FORMAT_A8R8G8B8_SRGB;
   case PIPE_FORMAT_X8R8G8B8_UNORM:
      return PIPE_FORMAT_X8R8G8B8_SRGB;
   case PIPE_FORMAT_DXT1_RGB:
      return PIPE_FORMAT_DXT1_SRGB;
   case PIPE_FORMAT_DXT1_RGBA:
      return PIPE_FORMAT_DXT1_SRGBA;
   case PIPE_FORMAT_DXT3_RGBA:
      return PIPE_FORMAT_DXT3_SRGBA;
   case PIPE_FORMAT_DXT5_RGBA:
      return PIPE_FORMAT_DXT5_SRGBA;
   default:
      return PIPE_FORMAT_NONE;
   }
}

/**
 * Given an sRGB format, return the corresponding linear colorspace format.
 * For non sRGB formats, return the format unchanged.
 */
static INLINE enum pipe_format
util_format_linear(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_L8_SRGB:
      return PIPE_FORMAT_L8_UNORM;
   case PIPE_FORMAT_L8A8_SRGB:
      return PIPE_FORMAT_L8A8_UNORM;
   case PIPE_FORMAT_R8G8B8_SRGB:
      return PIPE_FORMAT_R8G8B8_UNORM;
   case PIPE_FORMAT_A8B8G8R8_SRGB:
      return PIPE_FORMAT_A8B8G8R8_UNORM;
   case PIPE_FORMAT_X8B8G8R8_SRGB:
      return PIPE_FORMAT_X8B8G8R8_UNORM;
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   case PIPE_FORMAT_B8G8R8X8_SRGB:
      return PIPE_FORMAT_B8G8R8X8_UNORM;
   case PIPE_FORMAT_A8R8G8B8_SRGB:
      return PIPE_FORMAT_A8R8G8B8_UNORM;
   case PIPE_FORMAT_X8R8G8B8_SRGB:
      return PIPE_FORMAT_X8R8G8B8_UNORM;
   case PIPE_FORMAT_DXT1_SRGB:
      return PIPE_FORMAT_DXT1_RGB;
   case PIPE_FORMAT_DXT1_SRGBA:
      return PIPE_FORMAT_DXT1_RGBA;
   case PIPE_FORMAT_DXT3_SRGBA:
      return PIPE_FORMAT_DXT3_RGBA;
   case PIPE_FORMAT_DXT5_SRGBA:
      return PIPE_FORMAT_DXT5_RGBA;
   default:
      return format;
   }
}

/**
 * Return the number of components stored.
 * Formats with block size != 1x1 will always have 1 component (the block).
 */
static INLINE unsigned
util_format_get_nr_components(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);
   return desc->nr_channels;
}

/**
 * Return the index of the first non-void channel
 * -1 if no non-void channels
 */
static INLINE int
util_format_get_first_non_void_channel(enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);
   int i;

   for (i = 0; i < 4; i++)
      if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID)
         break;

   if (i == 4)
       return -1;

   return i;
}

/*
 * Format access functions.
 */

void
util_format_read_4f(enum pipe_format format,
                    float *dst, unsigned dst_stride, 
                    const void *src, unsigned src_stride, 
                    unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_write_4f(enum pipe_format format,
                     const float *src, unsigned src_stride, 
                     void *dst, unsigned dst_stride, 
                     unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_read_4ub(enum pipe_format format,
                     uint8_t *dst, unsigned dst_stride, 
                     const void *src, unsigned src_stride, 
                     unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_write_4ub(enum pipe_format format,
                      const uint8_t *src, unsigned src_stride, 
                      void *dst, unsigned dst_stride, 
                      unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_read_4ui(enum pipe_format format,
                     unsigned *dst, unsigned dst_stride,
                     const void *src, unsigned src_stride,
                     unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_write_4ui(enum pipe_format format,
                      const unsigned int *src, unsigned src_stride,
                      void *dst, unsigned dst_stride,
                      unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_read_4i(enum pipe_format format,
                    int *dst, unsigned dst_stride,
                    const void *src, unsigned src_stride,
                    unsigned x, unsigned y, unsigned w, unsigned h);

void
util_format_write_4i(enum pipe_format format,
                     const int *src, unsigned src_stride,
                     void *dst, unsigned dst_stride,
                     unsigned x, unsigned y, unsigned w, unsigned h);

/*
 * Generic format conversion;
 */

boolean
util_format_fits_8unorm(const struct util_format_description *format_desc);

void
util_format_translate(enum pipe_format dst_format,
                      void *dst, unsigned dst_stride,
                      unsigned dst_x, unsigned dst_y,
                      enum pipe_format src_format,
                      const void *src, unsigned src_stride,
                      unsigned src_x, unsigned src_y,
                      unsigned width, unsigned height);

/*
 * Swizzle operations.
 */

/* Compose two sets of swizzles.
 * If V is a 4D vector and the function parameters represent functions that
 * swizzle vector components, this holds:
 *     swz2(swz1(V)) = dst(V)
 */
void util_format_compose_swizzles(const unsigned char swz1[4],
                                  const unsigned char swz2[4],
                                  unsigned char dst[4]);

void util_format_swizzle_4f(float *dst, const float *src,
                            const unsigned char swz[4]);

void util_format_unswizzle_4f(float *dst, const float *src,
                              const unsigned char swz[4]);

#ifdef __cplusplus
} // extern "C" {
#endif

#endif /* ! U_FORMAT_H */
