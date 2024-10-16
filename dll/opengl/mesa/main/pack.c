/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
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
 * THEA AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file pack.c
 * Image and pixel span packing and unpacking.
 */

#include <precomp.h>

/**
 * Flip the 8 bits in each byte of the given array.
 *
 * \param p array.
 * \param n number of bytes.
 *
 * \todo try this trick to flip bytes someday:
 * \code
 *  v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555);
 *  v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333);
 *  v = ((v & 0x0f0f0f0f) << 4) | ((v >> 4) & 0x0f0f0f0f);
 * \endcode
 */
static void
flip_bytes( GLubyte *p, GLuint n )
{
   GLuint i, a, b;
   for (i = 0; i < n; i++) {
      b = (GLuint) p[i];        /* words are often faster than bytes */
      a = ((b & 0x01) << 7) |
	  ((b & 0x02) << 5) |
	  ((b & 0x04) << 3) |
	  ((b & 0x08) << 1) |
	  ((b & 0x10) >> 1) |
	  ((b & 0x20) >> 3) |
	  ((b & 0x40) >> 5) |
	  ((b & 0x80) >> 7);
      p[i] = (GLubyte) a;
   }
}



/*
 * Unpack a 32x32 pixel polygon stipple from user memory using the
 * current pixel unpack settings.
 */
void
_mesa_unpack_polygon_stipple( const GLubyte *pattern, GLuint dest[32],
                              const struct gl_pixelstore_attrib *unpacking )
{
   GLubyte *ptrn = (GLubyte *) _mesa_unpack_bitmap(32, 32, pattern, unpacking);
   if (ptrn) {
      /* Convert pattern from GLubytes to GLuints and handle big/little
       * endian differences
       */
      GLubyte *p = ptrn;
      GLint i;
      for (i = 0; i < 32; i++) {
         dest[i] = (p[0] << 24)
                 | (p[1] << 16)
                 | (p[2] <<  8)
                 | (p[3]      );
         p += 4;
      }
      free(ptrn);
   }
}


/*
 * Pack polygon stipple into user memory given current pixel packing
 * settings.
 */
void
_mesa_pack_polygon_stipple( const GLuint pattern[32], GLubyte *dest,
                            const struct gl_pixelstore_attrib *packing )
{
   /* Convert pattern from GLuints to GLubytes to handle big/little
    * endian differences.
    */
   GLubyte ptrn[32*4];
   GLint i;
   for (i = 0; i < 32; i++) {
      ptrn[i * 4 + 0] = (GLubyte) ((pattern[i] >> 24) & 0xff);
      ptrn[i * 4 + 1] = (GLubyte) ((pattern[i] >> 16) & 0xff);
      ptrn[i * 4 + 2] = (GLubyte) ((pattern[i] >> 8 ) & 0xff);
      ptrn[i * 4 + 3] = (GLubyte) ((pattern[i]      ) & 0xff);
   }

   _mesa_pack_bitmap(32, 32, ptrn, dest, packing);
}


/*
 * Unpack bitmap data.  Resulting data will be in most-significant-bit-first
 * order with row alignment = 1 byte.
 */
GLvoid *
_mesa_unpack_bitmap( GLint width, GLint height, const GLubyte *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
   GLint bytes, row, width_in_bytes;
   GLubyte *buffer, *dst;

   if (!pixels)
      return NULL;

   /* Alloc dest storage */
   bytes = ((width + 7) / 8 * height);
   buffer = (GLubyte *) malloc( bytes );
   if (!buffer)
      return NULL;

   width_in_bytes = CEILING( width, 8 );
   dst = buffer;
   for (row = 0; row < height; row++) {
      const GLubyte *src = (const GLubyte *)
         _mesa_image_address2d(packing, pixels, width, height,
                               GL_COLOR_INDEX, GL_BITMAP, row, 0);
      if (!src) {
         free(buffer);
         return NULL;
      }

      if ((packing->SkipPixels & 7) == 0) {
         memcpy( dst, src, width_in_bytes );
         if (packing->LsbFirst) {
            flip_bytes( dst, width_in_bytes );
         }
      }
      else {
         /* handling SkipPixels is a bit tricky (no pun intended!) */
         GLint i;
         if (packing->LsbFirst) {
            GLubyte srcMask = 1 << (packing->SkipPixels & 0x7);
            GLubyte dstMask = 128;
            const GLubyte *s = src;
            GLubyte *d = dst;
            *d = 0;
            for (i = 0; i < width; i++) {
               if (*s & srcMask) {
                  *d |= dstMask;
               }
               if (srcMask == 128) {
                  srcMask = 1;
                  s++;
               }
               else {
                  srcMask = srcMask << 1;
               }
               if (dstMask == 1) {
                  dstMask = 128;
                  d++;
                  *d = 0;
               }
               else {
                  dstMask = dstMask >> 1;
               }
            }
         }
         else {
            GLubyte srcMask = 128 >> (packing->SkipPixels & 0x7);
            GLubyte dstMask = 128;
            const GLubyte *s = src;
            GLubyte *d = dst;
            *d = 0;
            for (i = 0; i < width; i++) {
               if (*s & srcMask) {
                  *d |= dstMask;
               }
               if (srcMask == 1) {
                  srcMask = 128;
                  s++;
               }
               else {
                  srcMask = srcMask >> 1;
               }
               if (dstMask == 1) {
                  dstMask = 128;
                  d++;
                  *d = 0;
               }
               else {
                  dstMask = dstMask >> 1;
               }
            }
         }
      }
      dst += width_in_bytes;
   }

   return buffer;
}


/*
 * Pack bitmap data.
 */
void
_mesa_pack_bitmap( GLint width, GLint height, const GLubyte *source,
                   GLubyte *dest, const struct gl_pixelstore_attrib *packing )
{
   GLint row, width_in_bytes;
   const GLubyte *src;

   if (!source)
      return;

   width_in_bytes = CEILING( width, 8 );
   src = source;
   for (row = 0; row < height; row++) {
      GLubyte *dst = (GLubyte *) _mesa_image_address2d(packing, dest,
                       width, height, GL_COLOR_INDEX, GL_BITMAP, row, 0);
      if (!dst)
         return;

      if ((packing->SkipPixels & 7) == 0) {
         memcpy( dst, src, width_in_bytes );
         if (packing->LsbFirst) {
            flip_bytes( dst, width_in_bytes );
         }
      }
      else {
         /* handling SkipPixels is a bit tricky (no pun intended!) */
         GLint i;
         if (packing->LsbFirst) {
            GLubyte srcMask = 128;
            GLubyte dstMask = 1 << (packing->SkipPixels & 0x7);
            const GLubyte *s = src;
            GLubyte *d = dst;
            *d = 0;
            for (i = 0; i < width; i++) {
               if (*s & srcMask) {
                  *d |= dstMask;
               }
               if (srcMask == 1) {
                  srcMask = 128;
                  s++;
               }
               else {
                  srcMask = srcMask >> 1;
               }
               if (dstMask == 128) {
                  dstMask = 1;
                  d++;
                  *d = 0;
               }
               else {
                  dstMask = dstMask << 1;
               }
            }
         }
         else {
            GLubyte srcMask = 128;
            GLubyte dstMask = 128 >> (packing->SkipPixels & 0x7);
            const GLubyte *s = src;
            GLubyte *d = dst;
            *d = 0;
            for (i = 0; i < width; i++) {
               if (*s & srcMask) {
                  *d |= dstMask;
               }
               if (srcMask == 1) {
                  srcMask = 128;
                  s++;
               }
               else {
                  srcMask = srcMask >> 1;
               }
               if (dstMask == 1) {
                  dstMask = 128;
                  d++;
                  *d = 0;
               }
               else {
                  dstMask = dstMask >> 1;
               }
            }
         }
      }
      src += width_in_bytes;
   }
}


/**
 * Get indexes of color components for a basic color format, such as
 * GL_RGBA, GL_RED, GL_LUMINANCE_ALPHA, etc.  Return -1 for indexes
 * that do not apply.
 */
static void
get_component_indexes(GLenum format,
                      GLint *redIndex,
                      GLint *greenIndex,
                      GLint *blueIndex,
                      GLint *alphaIndex,
                      GLint *luminanceIndex,
                      GLint *intensityIndex)
{
   *redIndex = -1;
   *greenIndex = -1;
   *blueIndex = -1;
   *alphaIndex = -1;
   *luminanceIndex = -1;
   *intensityIndex = -1;

   switch (format) {
   case GL_LUMINANCE:
   case GL_LUMINANCE_INTEGER_EXT:
      *luminanceIndex = 0;
      break;
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      *luminanceIndex = 0;
      *alphaIndex = 1;
      break;
   case GL_INTENSITY:
      *intensityIndex = 0;
      break;
   case GL_RED:
   case GL_RED_INTEGER_EXT:
      *redIndex = 0;
      break;
   case GL_GREEN:
   case GL_GREEN_INTEGER_EXT:
      *greenIndex = 0;
      break;
   case GL_BLUE:
   case GL_BLUE_INTEGER_EXT:
      *blueIndex = 0;
      break;
   case GL_ALPHA:
   case GL_ALPHA_INTEGER_EXT:
      *alphaIndex = 0;
      break;
   case GL_RG:
   case GL_RG_INTEGER:
      *redIndex = 0;
      *greenIndex = 1;
      break;
   case GL_RGB:
   case GL_RGB_INTEGER_EXT:
      *redIndex = 0;
      *greenIndex = 1;
      *blueIndex = 2;
      break;
   case GL_BGR:
   case GL_BGR_INTEGER_EXT:
      *blueIndex = 0;
      *greenIndex = 1;
      *redIndex = 2;
      break;
   case GL_RGBA:
   case GL_RGBA_INTEGER_EXT:
      *redIndex = 0;
      *greenIndex = 1;
      *blueIndex = 2;
      *alphaIndex = 3;
      break;
   case GL_BGRA:
   case GL_BGRA_INTEGER:
      *redIndex = 2;
      *greenIndex = 1;
      *blueIndex = 0;
      *alphaIndex = 3;
      break;
   case GL_ABGR_EXT:
      *redIndex = 3;
      *greenIndex = 2;
      *blueIndex = 1;
      *alphaIndex = 0;
      break;
   default:
      assert(0 && "bad format in get_component_indexes()");
   }
}



/**
 * For small integer types, return the min and max possible values.
 * Used for clamping floats to unscaled integer types.
 * \return GL_TRUE if type is handled, GL_FALSE otherwise.
 */
static GLboolean
get_type_min_max(GLenum type, GLfloat *min, GLfloat *max)
{
   switch (type) {
   case GL_BYTE:
      *min = -128.0;
      *max = 127.0;
      return GL_TRUE;
   case GL_UNSIGNED_BYTE:
      *min = 0.0;
      *max = 255.0;
      return GL_TRUE;
   case GL_SHORT:
      *min = -32768.0;
      *max = 32767.0;
      return GL_TRUE;
   case GL_UNSIGNED_SHORT:
      *min = 0.0;
      *max = 65535.0;
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}

/* Customization of integer packing.  We always treat src as uint, and can pack dst
 * as any integer type/format combo.
 */
#define SRC_TYPE GLuint

#define DST_TYPE GLuint
#define SRC_CONVERT(x) (x)
#define FN_NAME pack_uint_from_uint_rgba
#include "pack_tmp.h"
#undef DST_TYPE
#undef SRC_CONVERT
#undef FN_NAME

#define DST_TYPE GLushort
#define SRC_CONVERT(x) MIN2(x, 0xffff)
#define FN_NAME pack_ushort_from_uint_rgba
#include "pack_tmp.h"
#undef DST_TYPE
#undef SRC_CONVERT
#undef FN_NAME

#define DST_TYPE GLshort
#define SRC_CONVERT(x) CLAMP((int)x, -32768, 32767)
#define FN_NAME pack_short_from_uint_rgba
#include "pack_tmp.h"
#undef DST_TYPE
#undef SRC_CONVERT
#undef FN_NAME

#define DST_TYPE GLubyte
#define SRC_CONVERT(x) MIN2(x, 0xff)
#define FN_NAME pack_ubyte_from_uint_rgba
#include "pack_tmp.h"
#undef DST_TYPE
#undef SRC_CONVERT
#undef FN_NAME

#define DST_TYPE GLbyte
#define SRC_CONVERT(x) CLAMP((int)x, -128, 127)
#define FN_NAME pack_byte_from_uint_rgba
#include "pack_tmp.h"
#undef DST_TYPE
#undef SRC_CONVERT
#undef FN_NAME

void
_mesa_pack_rgba_span_int(struct gl_context *ctx, GLuint n, GLuint rgba[][4],
                         GLenum dstFormat, GLenum dstType,
                         GLvoid *dstAddr)
{
   switch(dstType) {
   case GL_UNSIGNED_INT:
      pack_uint_from_uint_rgba(dstAddr, dstFormat, rgba, n);
      break;
   case GL_INT:
      /* No conversion necessary. */
      pack_uint_from_uint_rgba(dstAddr, dstFormat, rgba, n);
      break;
   case GL_UNSIGNED_SHORT:
      pack_ushort_from_uint_rgba(dstAddr, dstFormat, rgba, n);
      break;
   case GL_SHORT:
      pack_short_from_uint_rgba(dstAddr, dstFormat, rgba, n);
      break;
   case GL_UNSIGNED_BYTE:
      pack_ubyte_from_uint_rgba(dstAddr, dstFormat, rgba, n);
      break;
   case GL_BYTE:
      pack_byte_from_uint_rgba(dstAddr, dstFormat, rgba, n);
      break;
   default:
      assert(0);
      return;
   }
}


/**
 * Used to pack an array [][4] of RGBA float colors as specified
 * by the dstFormat, dstType and dstPacking.  Used by glReadPixels.
 * Historically, the RGBA values were in [0,1] and rescaled to fit
 * into GLubytes, etc.  But with new integer formats, the RGBA values
 * may have any value and we don't always rescale when converting to
 * integers.
 *
 * Note: the rgba values will be modified by this function when any pixel
 * transfer ops are enabled.
 */
void
_mesa_pack_rgba_span_float(struct gl_context *ctx, GLuint n, GLfloat rgba[][4],
                           GLenum dstFormat, GLenum dstType,
                           GLvoid *dstAddr,
                           const struct gl_pixelstore_attrib *dstPacking,
                           GLbitfield transferOps)
{
   GLfloat *luminance;
   const GLint comps = _mesa_components_in_format(dstFormat);
   const GLboolean intDstFormat = _mesa_is_integer_format(dstFormat);
   GLuint i;

   if (dstFormat == GL_LUMINANCE ||
       dstFormat == GL_LUMINANCE_ALPHA ||
       dstFormat == GL_LUMINANCE_INTEGER_EXT ||
       dstFormat == GL_LUMINANCE_ALPHA_INTEGER_EXT) {
      luminance = (GLfloat *) malloc(n * sizeof(GLfloat));
      if (!luminance) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel packing");
         return;
      }
   }
   else {
      luminance = NULL;
   }

   /* EXT_texture_integer specifies no transfer ops on integer
    * types in the resolved issues section. Just set them to 0
    * for integer surfaces.
    */
   if (intDstFormat)
      transferOps = 0;

   if (transferOps) {
      _mesa_apply_rgba_transfer_ops(ctx, transferOps, n, rgba);
   }

   /*
    * Component clamping (besides clamping to [0,1] in
    * _mesa_apply_rgba_transfer_ops()).
    */
   if (intDstFormat) {
      /* clamping to dest type's min/max values */
      GLfloat min, max;
      if (get_type_min_max(dstType, &min, &max)) {
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = CLAMP(rgba[i][RCOMP], min, max);
            rgba[i][GCOMP] = CLAMP(rgba[i][GCOMP], min, max);
            rgba[i][BCOMP] = CLAMP(rgba[i][BCOMP], min, max);
            rgba[i][ACOMP] = CLAMP(rgba[i][ACOMP], min, max);
         }
      }
   }
   else if (dstFormat == GL_LUMINANCE || dstFormat == GL_LUMINANCE_ALPHA) {
      /* compute luminance values */
      if (transferOps & IMAGE_CLAMP_BIT) {
         for (i = 0; i < n; i++) {
            GLfloat sum = rgba[i][RCOMP] + rgba[i][GCOMP] + rgba[i][BCOMP];
            luminance[i] = CLAMP(sum, 0.0F, 1.0F);
         }
      }
      else {
         for (i = 0; i < n; i++) {
            luminance[i] = rgba[i][RCOMP] + rgba[i][GCOMP] + rgba[i][BCOMP];
         }
      }
   }

   /*
    * Pack/store the pixels.  Ugh!  Lots of cases!!!
    */
   switch (dstType) {
      case GL_UNSIGNED_BYTE:
         {
            GLubyte *dst = (GLubyte *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UBYTE(rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UBYTE(rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UBYTE(luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_UBYTE(luminance[i]);
                     dst[i*2+1] = FLOAT_TO_UBYTE(rgba[i][ACOMP]);
                  }
                  break;
               case GL_RG:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
                     dst[i*2+1] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
                     dst[i*3+1] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_UBYTE(rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
                     dst[i*4+1] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_UBYTE(rgba[i][BCOMP]);
                     dst[i*4+3] = FLOAT_TO_UBYTE(rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_UBYTE(rgba[i][BCOMP]);
                     dst[i*3+1] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_UBYTE(rgba[i][BCOMP]);
                     dst[i*4+1] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
                     dst[i*4+3] = FLOAT_TO_UBYTE(rgba[i][ACOMP]);
                  }
                  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_UBYTE(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_UBYTE(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_UBYTE(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_UBYTE(rgba[i][RCOMP]);
                  }
                  break;
               case GL_RED_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLubyte) rgba[i][RCOMP];
                  }
                  break;
               case GL_GREEN_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLubyte) rgba[i][GCOMP];
                  }
                  break;
               case GL_BLUE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLubyte) rgba[i][BCOMP];
                  }
                  break;
               case GL_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLubyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_RG_INTEGER:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLubyte) rgba[i][RCOMP];
                     dst[i*2+1] = (GLubyte) rgba[i][GCOMP];
                  }
                  break;
               case GL_RGB_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLubyte) rgba[i][RCOMP];
                     dst[i*3+1] = (GLubyte) rgba[i][GCOMP];
                     dst[i*3+2] = (GLubyte) rgba[i][BCOMP];
                  }
                  break;
               case GL_RGBA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLubyte) rgba[i][RCOMP];
                     dst[i*4+1] = (GLubyte) rgba[i][GCOMP];
                     dst[i*4+2] = (GLubyte) rgba[i][BCOMP];
                     dst[i*4+3] = (GLubyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_BGR_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLubyte) rgba[i][BCOMP];
                     dst[i*3+1] = (GLubyte) rgba[i][GCOMP];
                     dst[i*3+2] = (GLubyte) rgba[i][RCOMP];
                  }
                  break;
               case GL_BGRA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLubyte) rgba[i][BCOMP];
                     dst[i*4+1] = (GLubyte) rgba[i][GCOMP];
                     dst[i*4+2] = (GLubyte) rgba[i][RCOMP];
                     dst[i*4+3] = (GLubyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLubyte) (rgba[i][RCOMP] +
                                             rgba[i][GCOMP] +
                                             rgba[i][BCOMP]);
                     dst[i*2+1] = (GLubyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLubyte) (rgba[i][RCOMP] +
                                         rgba[i][GCOMP] +
                                         rgba[i][BCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
         }
         break;
      case GL_BYTE:
         {
            GLbyte *dst = (GLbyte *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_BYTE(rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_BYTE(rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_BYTE(luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_BYTE(luminance[i]);
                     dst[i*2+1] = FLOAT_TO_BYTE(rgba[i][ACOMP]);
                  }
                  break;
               case GL_RG:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
                     dst[i*2+1] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
                     dst[i*3+1] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_BYTE(rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
                     dst[i*4+1] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_BYTE(rgba[i][BCOMP]);
                     dst[i*4+3] = FLOAT_TO_BYTE(rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_BYTE(rgba[i][BCOMP]);
                     dst[i*3+1] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_BYTE(rgba[i][BCOMP]);
                     dst[i*4+1] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
                     dst[i*4+3] = FLOAT_TO_BYTE(rgba[i][ACOMP]);
                  }
		  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_BYTE(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_BYTE(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
                  }
                  break;
               case GL_RED_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLbyte) rgba[i][RCOMP];
                  }
                  break;
               case GL_GREEN_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLbyte) rgba[i][GCOMP];
                  }
                  break;
               case GL_BLUE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLbyte) rgba[i][BCOMP];
                  }
                  break;
               case GL_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLbyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_RG_INTEGER:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLbyte) rgba[i][RCOMP];
                     dst[i*2+1] = (GLbyte) rgba[i][GCOMP];
                  }
                  break;
               case GL_RGB_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLbyte) rgba[i][RCOMP];
                     dst[i*3+1] = (GLbyte) rgba[i][GCOMP];
                     dst[i*3+2] = (GLbyte) rgba[i][BCOMP];
                  }
                  break;
               case GL_RGBA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLbyte) rgba[i][RCOMP];
                     dst[i*4+1] = (GLbyte) rgba[i][GCOMP];
                     dst[i*4+2] = (GLbyte) rgba[i][BCOMP];
                     dst[i*4+3] = (GLbyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_BGR_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLbyte) rgba[i][BCOMP];
                     dst[i*3+1] = (GLbyte) rgba[i][GCOMP];
                     dst[i*3+2] = (GLbyte) rgba[i][RCOMP];
                  }
                  break;
               case GL_BGRA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLbyte) rgba[i][BCOMP];
                     dst[i*4+1] = (GLbyte) rgba[i][GCOMP];
                     dst[i*4+2] = (GLbyte) rgba[i][RCOMP];
                     dst[i*4+3] = (GLbyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLbyte) (rgba[i][RCOMP] +
                                            rgba[i][GCOMP] +
                                            rgba[i][BCOMP]);
                     dst[i*2+1] = (GLbyte) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLbyte) (rgba[i][RCOMP] +
                                        rgba[i][GCOMP] +
                                        rgba[i][BCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLushort *dst = (GLushort *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     CLAMPED_FLOAT_TO_USHORT(dst[i], rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     CLAMPED_FLOAT_TO_USHORT(dst[i], rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     CLAMPED_FLOAT_TO_USHORT(dst[i], rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     CLAMPED_FLOAT_TO_USHORT(dst[i], rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     UNCLAMPED_FLOAT_TO_USHORT(dst[i], luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     UNCLAMPED_FLOAT_TO_USHORT(dst[i*2+0], luminance[i]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*2+1], rgba[i][ACOMP]);
                  }
                  break;
               case GL_RG:
                  for (i=0;i<n;i++) {
                     CLAMPED_FLOAT_TO_USHORT(dst[i*2+0], rgba[i][RCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*2+1], rgba[i][GCOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     CLAMPED_FLOAT_TO_USHORT(dst[i*3+0], rgba[i][RCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*3+1], rgba[i][GCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*3+2], rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+0], rgba[i][RCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+1], rgba[i][GCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+2], rgba[i][BCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+3], rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     CLAMPED_FLOAT_TO_USHORT(dst[i*3+0], rgba[i][BCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*3+1], rgba[i][GCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*3+2], rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+0], rgba[i][BCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+1], rgba[i][GCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+2], rgba[i][RCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+3], rgba[i][ACOMP]);
                  }
                  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+0], rgba[i][ACOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+1], rgba[i][BCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+2], rgba[i][GCOMP]);
                     CLAMPED_FLOAT_TO_USHORT(dst[i*4+3], rgba[i][RCOMP]);
                  }
                  break;
               case GL_RED_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLushort) rgba[i][RCOMP];
                  }
                  break;
               case GL_GREEN_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLushort) rgba[i][GCOMP];
                  }
                  break;
               case GL_BLUE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLushort) rgba[i][BCOMP];
                  }
                  break;
               case GL_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLushort) rgba[i][ACOMP];
                  }
                  break;
               case GL_RG_INTEGER:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLushort) rgba[i][RCOMP];
                     dst[i*2+1] = (GLushort) rgba[i][GCOMP];
                  }
                  break;
               case GL_RGB_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLushort) rgba[i][RCOMP];
                     dst[i*3+1] = (GLushort) rgba[i][GCOMP];
                     dst[i*3+2] = (GLushort) rgba[i][BCOMP];
                  }
                  break;
               case GL_RGBA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLushort) rgba[i][RCOMP];
                     dst[i*4+1] = (GLushort) rgba[i][GCOMP];
                     dst[i*4+2] = (GLushort) rgba[i][BCOMP];
                     dst[i*4+3] = (GLushort) rgba[i][ACOMP];
                  }
                  break;
               case GL_BGR_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLushort) rgba[i][BCOMP];
                     dst[i*3+1] = (GLushort) rgba[i][GCOMP];
                     dst[i*3+2] = (GLushort) rgba[i][RCOMP];
                  }
                  break;
               case GL_BGRA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLushort) rgba[i][BCOMP];
                     dst[i*4+1] = (GLushort) rgba[i][GCOMP];
                     dst[i*4+2] = (GLushort) rgba[i][RCOMP];
                     dst[i*4+3] = (GLushort) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLushort) (rgba[i][RCOMP] +
                                              rgba[i][GCOMP] +
                                              rgba[i][BCOMP]);
                     dst[i*2+1] = (GLushort) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLushort) (rgba[i][RCOMP] +
                                          rgba[i][GCOMP] +
                                          rgba[i][BCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
         }
         break;
      case GL_SHORT:
         {
            GLshort *dst = (GLshort *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_SHORT(rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_SHORT(rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_SHORT(luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_SHORT(luminance[i]);
                     dst[i*2+1] = FLOAT_TO_SHORT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_RG:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                     dst[i*2+1] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                     dst[i*3+1] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_SHORT(rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                     dst[i*4+1] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_SHORT(rgba[i][BCOMP]);
                     dst[i*4+3] = FLOAT_TO_SHORT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_SHORT(rgba[i][BCOMP]);
                     dst[i*3+1] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_SHORT(rgba[i][BCOMP]);
                     dst[i*4+1] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                     dst[i*4+3] = FLOAT_TO_SHORT(rgba[i][ACOMP]);
                  }
		  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_SHORT(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_SHORT(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                  }
                  break;
               case GL_RED_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLshort) rgba[i][RCOMP];
                  }
                  break;
               case GL_GREEN_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLshort) rgba[i][GCOMP];
                  }
                  break;
               case GL_BLUE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLshort) rgba[i][BCOMP];
                  }
                  break;
               case GL_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLshort) rgba[i][ACOMP];
                  }
                  break;
               case GL_RG_INTEGER:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLshort) rgba[i][RCOMP];
                     dst[i*2+1] = (GLshort) rgba[i][GCOMP];
                  }
                  break;
               case GL_RGB_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLshort) rgba[i][RCOMP];
                     dst[i*3+1] = (GLshort) rgba[i][GCOMP];
                     dst[i*3+2] = (GLshort) rgba[i][BCOMP];
                  }
                  break;
               case GL_RGBA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLshort) rgba[i][RCOMP];
                     dst[i*4+1] = (GLshort) rgba[i][GCOMP];
                     dst[i*4+2] = (GLshort) rgba[i][BCOMP];
                     dst[i*4+3] = (GLshort) rgba[i][ACOMP];
                  }
                  break;
               case GL_BGR_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLshort) rgba[i][BCOMP];
                     dst[i*3+1] = (GLshort) rgba[i][GCOMP];
                     dst[i*3+2] = (GLshort) rgba[i][RCOMP];
                  }
                  break;
               case GL_BGRA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLshort) rgba[i][BCOMP];
                     dst[i*4+1] = (GLshort) rgba[i][GCOMP];
                     dst[i*4+2] = (GLshort) rgba[i][RCOMP];
                     dst[i*4+3] = (GLshort) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLshort) (rgba[i][RCOMP] +
                                             rgba[i][GCOMP] +
                                             rgba[i][BCOMP]);
                     dst[i*2+1] = (GLshort) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLshort) (rgba[i][RCOMP] +
                                         rgba[i][GCOMP] +
                                         rgba[i][BCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *dst = (GLuint *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UINT(rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UINT(rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UINT(rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UINT(rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_UINT(luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_UINT(luminance[i]);
                     dst[i*2+1] = FLOAT_TO_UINT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_RG:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_UINT(rgba[i][RCOMP]);
                     dst[i*2+1] = FLOAT_TO_UINT(rgba[i][GCOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_UINT(rgba[i][RCOMP]);
                     dst[i*3+1] = FLOAT_TO_UINT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_UINT(rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_UINT(rgba[i][RCOMP]);
                     dst[i*4+1] = FLOAT_TO_UINT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_UINT(rgba[i][BCOMP]);
                     dst[i*4+3] = FLOAT_TO_UINT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_UINT(rgba[i][BCOMP]);
                     dst[i*3+1] = FLOAT_TO_UINT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_UINT(rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_UINT(rgba[i][BCOMP]);
                     dst[i*4+1] = FLOAT_TO_UINT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_UINT(rgba[i][RCOMP]);
                     dst[i*4+3] = FLOAT_TO_UINT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_UINT(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_UINT(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_UINT(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_UINT(rgba[i][RCOMP]);
                  }
                  break;
               case GL_RED_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLuint) rgba[i][RCOMP];
                  }
                  break;
               case GL_GREEN_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLuint) rgba[i][GCOMP];
                  }
                  break;
               case GL_BLUE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLuint) rgba[i][BCOMP];
                  }
                  break;
               case GL_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLuint) rgba[i][ACOMP];
                  }
                  break;
               case GL_RG_INTEGER:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLuint) rgba[i][RCOMP];
                     dst[i*2+1] = (GLuint) rgba[i][GCOMP];
                  }
                  break;
               case GL_RGB_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLuint) rgba[i][RCOMP];
                     dst[i*3+1] = (GLuint) rgba[i][GCOMP];
                     dst[i*3+2] = (GLuint) rgba[i][BCOMP];
                  }
                  break;
               case GL_RGBA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLuint) rgba[i][RCOMP];
                     dst[i*4+1] = (GLuint) rgba[i][GCOMP];
                     dst[i*4+2] = (GLuint) rgba[i][BCOMP];
                     dst[i*4+3] = (GLuint) rgba[i][ACOMP];
                  }
                  break;
               case GL_BGR_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLuint) rgba[i][BCOMP];
                     dst[i*3+1] = (GLuint) rgba[i][GCOMP];
                     dst[i*3+2] = (GLuint) rgba[i][RCOMP];
                  }
                  break;
               case GL_BGRA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLuint) rgba[i][BCOMP];
                     dst[i*4+1] = (GLuint) rgba[i][GCOMP];
                     dst[i*4+2] = (GLuint) rgba[i][RCOMP];
                     dst[i*4+3] = (GLuint) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLuint) (rgba[i][RCOMP] +
                                            rgba[i][GCOMP] +
                                            rgba[i][BCOMP]);
                     dst[i*2+1] = (GLuint) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLuint) (rgba[i][RCOMP] +
                                        rgba[i][GCOMP] +
                                        rgba[i][BCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
         }
         break;
      case GL_INT:
         {
            GLint *dst = (GLint *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_INT(rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_INT(rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_INT(rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_INT(rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_INT(luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_INT(luminance[i]);
                     dst[i*2+1] = FLOAT_TO_INT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_RG:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_INT(rgba[i][RCOMP]);
                     dst[i*2+1] = FLOAT_TO_INT(rgba[i][GCOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_INT(rgba[i][RCOMP]);
                     dst[i*3+1] = FLOAT_TO_INT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_INT(rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_INT(rgba[i][RCOMP]);
                     dst[i*4+1] = FLOAT_TO_INT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_INT(rgba[i][BCOMP]);
                     dst[i*4+3] = FLOAT_TO_INT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_INT(rgba[i][BCOMP]);
                     dst[i*3+1] = FLOAT_TO_INT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_INT(rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_INT(rgba[i][BCOMP]);
                     dst[i*4+1] = FLOAT_TO_INT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_INT(rgba[i][RCOMP]);
                     dst[i*4+3] = FLOAT_TO_INT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_INT(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_INT(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_INT(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_INT(rgba[i][RCOMP]);
                  }
                  break;
               case GL_RED_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLint) rgba[i][RCOMP];
                  }
                  break;
               case GL_GREEN_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLint) rgba[i][GCOMP];
                  }
                  break;
               case GL_BLUE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLint) rgba[i][BCOMP];
                  }
                  break;
               case GL_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLint) rgba[i][ACOMP];
                  }
                  break;
               case GL_RG_INTEGER:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLint) rgba[i][RCOMP];
                     dst[i*2+1] = (GLint) rgba[i][GCOMP];
                  }
                  break;
               case GL_RGB_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLint) rgba[i][RCOMP];
                     dst[i*3+1] = (GLint) rgba[i][GCOMP];
                     dst[i*3+2] = (GLint) rgba[i][BCOMP];
                  }
                  break;
               case GL_RGBA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLint) rgba[i][RCOMP];
                     dst[i*4+1] = (GLint) rgba[i][GCOMP];
                     dst[i*4+2] = (GLint) rgba[i][BCOMP];
                     dst[i*4+3] = (GLint) rgba[i][ACOMP];
                  }
                  break;
               case GL_BGR_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = (GLint) rgba[i][BCOMP];
                     dst[i*3+1] = (GLint) rgba[i][GCOMP];
                     dst[i*3+2] = (GLint) rgba[i][RCOMP];
                  }
                  break;
               case GL_BGRA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = (GLint) rgba[i][BCOMP];
                     dst[i*4+1] = (GLint) rgba[i][GCOMP];
                     dst[i*4+2] = (GLint) rgba[i][RCOMP];
                     dst[i*4+3] = (GLint) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = (GLint) (rgba[i][RCOMP] +
                                           rgba[i][GCOMP] +
                                           rgba[i][BCOMP]);
                     dst[i*2+1] = (GLint) rgba[i][ACOMP];
                  }
                  break;
               case GL_LUMINANCE_ALPHA_INTEGER_EXT:
                  for (i=0;i<n;i++) {
                     dst[i] = (GLint) (rgba[i][RCOMP] +
                                       rgba[i][GCOMP] +
                                       rgba[i][BCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
         }
         break;
      case GL_FLOAT:
         {
            GLfloat *dst = (GLfloat *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     dst[i] = rgba[i][RCOMP];
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = rgba[i][GCOMP];
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = rgba[i][BCOMP];
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = rgba[i][ACOMP];
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = luminance[i];
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = luminance[i];
                     dst[i*2+1] = rgba[i][ACOMP];
                  }
                  break;
               case GL_RG:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = rgba[i][RCOMP];
                     dst[i*2+1] = rgba[i][GCOMP];
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = rgba[i][RCOMP];
                     dst[i*3+1] = rgba[i][GCOMP];
                     dst[i*3+2] = rgba[i][BCOMP];
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = rgba[i][RCOMP];
                     dst[i*4+1] = rgba[i][GCOMP];
                     dst[i*4+2] = rgba[i][BCOMP];
                     dst[i*4+3] = rgba[i][ACOMP];
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = rgba[i][BCOMP];
                     dst[i*3+1] = rgba[i][GCOMP];
                     dst[i*3+2] = rgba[i][RCOMP];
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = rgba[i][BCOMP];
                     dst[i*4+1] = rgba[i][GCOMP];
                     dst[i*4+2] = rgba[i][RCOMP];
                     dst[i*4+3] = rgba[i][ACOMP];
                  }
                  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = rgba[i][ACOMP];
                     dst[i*4+1] = rgba[i][BCOMP];
                     dst[i*4+2] = rgba[i][GCOMP];
                     dst[i*4+3] = rgba[i][RCOMP];
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
         }
         break;
      case GL_UNSIGNED_BYTE_3_3_2:
         if (dstFormat == GL_RGB) {
            GLubyte *dst = (GLubyte *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 7.0F) << 5)
                      | (IROUND(rgba[i][GCOMP] * 7.0F) << 2)
                      | (IROUND(rgba[i][BCOMP] * 3.0F)     );
            }
         }
         break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         if (dstFormat == GL_RGB) {
            GLubyte *dst = (GLubyte *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 7.0F)     )
                      | (IROUND(rgba[i][GCOMP] * 7.0F) << 3)
                      | (IROUND(rgba[i][BCOMP] * 3.0F) << 6);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5:
         if (dstFormat == GL_RGB) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 31.0F) << 11)
                      | (IROUND(rgba[i][GCOMP] * 63.0F) <<  5)
                      | (IROUND(rgba[i][BCOMP] * 31.0F)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (dstFormat == GL_RGB) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 31.0F)      )
                      | (IROUND(rgba[i][GCOMP] * 63.0F) <<  5)
                      | (IROUND(rgba[i][BCOMP] * 31.0F) << 11);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 15.0F) << 12)
                      | (IROUND(rgba[i][GCOMP] * 15.0F) <<  8)
                      | (IROUND(rgba[i][BCOMP] * 15.0F) <<  4)
                      | (IROUND(rgba[i][ACOMP] * 15.0F)      );
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][BCOMP] * 15.0F) << 12)
                      | (IROUND(rgba[i][GCOMP] * 15.0F) <<  8)
                      | (IROUND(rgba[i][RCOMP] * 15.0F) <<  4)
                      | (IROUND(rgba[i][ACOMP] * 15.0F)      );
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][ACOMP] * 15.0F) << 12)
                      | (IROUND(rgba[i][BCOMP] * 15.0F) <<  8)
                      | (IROUND(rgba[i][GCOMP] * 15.0F) <<  4)
                      | (IROUND(rgba[i][RCOMP] * 15.0F)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 15.0F)      )
                      | (IROUND(rgba[i][GCOMP] * 15.0F) <<  4)
                      | (IROUND(rgba[i][BCOMP] * 15.0F) <<  8)
                      | (IROUND(rgba[i][ACOMP] * 15.0F) << 12);
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][BCOMP] * 15.0F)      )
                      | (IROUND(rgba[i][GCOMP] * 15.0F) <<  4)
                      | (IROUND(rgba[i][RCOMP] * 15.0F) <<  8)
                      | (IROUND(rgba[i][ACOMP] * 15.0F) << 12);
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][ACOMP] * 15.0F)      )
                      | (IROUND(rgba[i][BCOMP] * 15.0F) <<  4)
                      | (IROUND(rgba[i][GCOMP] * 15.0F) <<  8)
                      | (IROUND(rgba[i][RCOMP] * 15.0F) << 12);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_5_5_1:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 31.0F) << 11)
                      | (IROUND(rgba[i][GCOMP] * 31.0F) <<  6)
                      | (IROUND(rgba[i][BCOMP] * 31.0F) <<  1)
                      | (IROUND(rgba[i][ACOMP] *  1.0F)      );
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][BCOMP] * 31.0F) << 11)
                      | (IROUND(rgba[i][GCOMP] * 31.0F) <<  6)
                      | (IROUND(rgba[i][RCOMP] * 31.0F) <<  1)
                      | (IROUND(rgba[i][ACOMP] *  1.0F)      );
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][ACOMP] * 31.0F) << 11)
                      | (IROUND(rgba[i][BCOMP] * 31.0F) <<  6)
                      | (IROUND(rgba[i][GCOMP] * 31.0F) <<  1)
                      | (IROUND(rgba[i][RCOMP] *  1.0F)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 31.0F)      )
                      | (IROUND(rgba[i][GCOMP] * 31.0F) <<  5)
                      | (IROUND(rgba[i][BCOMP] * 31.0F) << 10)
                      | (IROUND(rgba[i][ACOMP] *  1.0F) << 15);
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][BCOMP] * 31.0F)      )
                      | (IROUND(rgba[i][GCOMP] * 31.0F) <<  5)
                      | (IROUND(rgba[i][RCOMP] * 31.0F) << 10)
                      | (IROUND(rgba[i][ACOMP] *  1.0F) << 15);
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][ACOMP] * 31.0F)      )
                      | (IROUND(rgba[i][BCOMP] * 31.0F) <<  5)
                      | (IROUND(rgba[i][GCOMP] * 31.0F) << 10)
                      | (IROUND(rgba[i][RCOMP] *  1.0F) << 15);
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8:
         if (dstFormat == GL_RGBA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 255.F) << 24)
                      | (IROUND(rgba[i][GCOMP] * 255.F) << 16)
                      | (IROUND(rgba[i][BCOMP] * 255.F) <<  8)
                      | (IROUND(rgba[i][ACOMP] * 255.F)      );
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][BCOMP] * 255.F) << 24)
                      | (IROUND(rgba[i][GCOMP] * 255.F) << 16)
                      | (IROUND(rgba[i][RCOMP] * 255.F) <<  8)
                      | (IROUND(rgba[i][ACOMP] * 255.F)      );
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][ACOMP] * 255.F) << 24)
                      | (IROUND(rgba[i][BCOMP] * 255.F) << 16)
                      | (IROUND(rgba[i][GCOMP] * 255.F) <<  8)
                      | (IROUND(rgba[i][RCOMP] * 255.F)      );
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         if (dstFormat == GL_RGBA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][RCOMP] * 255.0F)      )
                      | (IROUND(rgba[i][GCOMP] * 255.0F) <<  8)
                      | (IROUND(rgba[i][BCOMP] * 255.0F) << 16)
                      | (IROUND(rgba[i][ACOMP] * 255.0F) << 24);
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][BCOMP] * 255.0F)      )
                      | (IROUND(rgba[i][GCOMP] * 255.0F) <<  8)
                      | (IROUND(rgba[i][RCOMP] * 255.0F) << 16)
                      | (IROUND(rgba[i][ACOMP] * 255.0F) << 24);
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (IROUND(rgba[i][ACOMP] * 255.0F)      )
                      | (IROUND(rgba[i][BCOMP] * 255.0F) <<  8)
                      | (IROUND(rgba[i][GCOMP] * 255.0F) << 16)
                      | (IROUND(rgba[i][RCOMP] * 255.0F) << 24);
            }
         }
         break;
      default:
         _mesa_problem(ctx, "bad type in _mesa_pack_rgba_span_float");
         free(luminance);
         return;
   }

   if (dstPacking->SwapBytes) {
      GLint swapSize = _mesa_sizeof_packed_type(dstType);
      if (swapSize == 2) {
         if (dstPacking->SwapBytes) {
            _mesa_swap2((GLushort *) dstAddr, n * comps);
         }
      }
      else if (swapSize == 4) {
         if (dstPacking->SwapBytes) {
            _mesa_swap4((GLuint *) dstAddr, n * comps);
         }
      }
   }

   free(luminance);
}



#define SWAP2BYTE(VALUE)			\
   {						\
      GLubyte *bytes = (GLubyte *) &(VALUE);	\
      GLubyte tmp = bytes[0];			\
      bytes[0] = bytes[1];			\
      bytes[1] = tmp;				\
   }

#define SWAP4BYTE(VALUE)			\
   {						\
      GLubyte *bytes = (GLubyte *) &(VALUE);	\
      GLubyte tmp = bytes[0];			\
      bytes[0] = bytes[3];			\
      bytes[3] = tmp;				\
      tmp = bytes[1];				\
      bytes[1] = bytes[2];			\
      bytes[2] = tmp;				\
   }


static void
extract_uint_indexes(GLuint n, GLuint indexes[],
                     GLenum srcFormat, GLenum srcType, const GLvoid *src,
                     const struct gl_pixelstore_attrib *unpack )
{
   ASSERT(srcFormat == GL_COLOR_INDEX || srcFormat == GL_STENCIL_INDEX);

   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT);

   switch (srcType) {
      case GL_BITMAP:
         {
            GLubyte *ubsrc = (GLubyte *) src;
            if (unpack->LsbFirst) {
               GLubyte mask = 1 << (unpack->SkipPixels & 0x7);
               GLuint i;
               for (i = 0; i < n; i++) {
                  indexes[i] = (*ubsrc & mask) ? 1 : 0;
                  if (mask == 128) {
                     mask = 1;
                     ubsrc++;
                  }
                  else {
                     mask = mask << 1;
                  }
               }
            }
            else {
               GLubyte mask = 128 >> (unpack->SkipPixels & 0x7);
               GLuint i;
               for (i = 0; i < n; i++) {
                  indexes[i] = (*ubsrc & mask) ? 1 : 0;
                  if (mask == 1) {
                     mask = 128;
                     ubsrc++;
                  }
                  else {
                     mask = mask >> 1;
                  }
               }
            }
         }
         break;
      case GL_UNSIGNED_BYTE:
         {
            GLuint i;
            const GLubyte *s = (const GLubyte *) src;
            for (i = 0; i < n; i++)
               indexes[i] = s[i];
         }
         break;
      case GL_BYTE:
         {
            GLuint i;
            const GLbyte *s = (const GLbyte *) src;
            for (i = 0; i < n; i++)
               indexes[i] = s[i];
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLuint i;
            const GLushort *s = (const GLushort *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLushort value = s[i];
                  SWAP2BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_SHORT:
         {
            GLuint i;
            const GLshort *s = (const GLshort *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLshort value = s[i];
                  SWAP2BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint i;
            const GLuint *s = (const GLuint *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLuint value = s[i];
                  SWAP4BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_INT:
         {
            GLuint i;
            const GLint *s = (const GLint *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLint value = s[i];
                  SWAP4BYTE(value);
                  indexes[i] = value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = s[i];
            }
         }
         break;
      case GL_FLOAT:
         {
            GLuint i;
            const GLfloat *s = (const GLfloat *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLfloat value = s[i];
                  SWAP4BYTE(value);
                  indexes[i] = (GLuint) value;
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = (GLuint) s[i];
            }
         }
         break;

      default:
         _mesa_problem(NULL, "bad srcType in extract_uint_indexes");
         return;
   }
}


/**
 * Return source/dest RGBA indexes for unpacking pixels.
 */
static void
get_component_mapping(GLenum format,
                      GLint *rSrc,
                      GLint *gSrc,
                      GLint *bSrc,
                      GLint *aSrc,
                      GLint *rDst,
                      GLint *gDst,
                      GLint *bDst,
                      GLint *aDst)
{
   switch (format) {
   case GL_RED:
   case GL_RED_INTEGER_EXT:
      *rSrc = 0;
      *gSrc = *bSrc = *aSrc = -1;
      break;
   case GL_GREEN:
   case GL_GREEN_INTEGER_EXT:
      *gSrc = 0;
      *rSrc = *bSrc = *aSrc = -1;
      break;
   case GL_BLUE:
   case GL_BLUE_INTEGER_EXT:
      *bSrc = 0;
      *rSrc = *gSrc = *aSrc = -1;
      break;
   case GL_ALPHA:
   case GL_ALPHA_INTEGER_EXT:
      *rSrc = *gSrc = *bSrc = -1;
      *aSrc = 0;
      break;
   case GL_LUMINANCE:
   case GL_LUMINANCE_INTEGER_EXT:
      *rSrc = *gSrc = *bSrc = 0;
      *aSrc = -1;
      break;
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      *rSrc = *gSrc = *bSrc = 0;
      *aSrc = 1;
      break;
   case GL_INTENSITY:
      *rSrc = *gSrc = *bSrc = *aSrc = 0;
      break;
   case GL_RG:
   case GL_RG_INTEGER:
      *rSrc = 0;
      *gSrc = 1;
      *bSrc = -1;
      *aSrc = -1;
      *rDst = 0;
      *gDst = 1;
      *bDst = 2;
      *aDst = 3;
      break;
   case GL_RGB:
   case GL_RGB_INTEGER:
      *rSrc = 0;
      *gSrc = 1;
      *bSrc = 2;
      *aSrc = -1;
      *rDst = 0;
      *gDst = 1;
      *bDst = 2;
      *aDst = 3;
      break;
   case GL_BGR:
   case GL_BGR_INTEGER:
      *rSrc = 2;
      *gSrc = 1;
      *bSrc = 0;
      *aSrc = -1;
      *rDst = 2;
      *gDst = 1;
      *bDst = 0;
      *aDst = 3;
      break;
   case GL_RGBA:
   case GL_RGBA_INTEGER:
      *rSrc = 0;
      *gSrc = 1;
      *bSrc = 2;
      *aSrc = 3;
      *rDst = 0;
      *gDst = 1;
      *bDst = 2;
      *aDst = 3;
      break;
   case GL_BGRA:
   case GL_BGRA_INTEGER:
      *rSrc = 2;
      *gSrc = 1;
      *bSrc = 0;
      *aSrc = 3;
      *rDst = 2;
      *gDst = 1;
      *bDst = 0;
      *aDst = 3;
      break;
   case GL_ABGR_EXT:
      *rSrc = 3;
      *gSrc = 2;
      *bSrc = 1;
      *aSrc = 0;
      *rDst = 3;
      *gDst = 2;
      *bDst = 1;
      *aDst = 0;
      break;
   default:
      _mesa_problem(NULL, "bad srcFormat %s in get_component_mapping",
                    _mesa_lookup_enum_by_nr(format));
      return;
   }
}



/*
 * This function extracts floating point RGBA values from arbitrary
 * image data.  srcFormat and srcType are the format and type parameters
 * passed to glDrawPixels, glTexImage[123]D, glTexSubImage[123]D, etc.
 *
 * Refering to section 3.6.4 of the OpenGL 1.2 spec, this function
 * implements the "Conversion to floating point", "Conversion to RGB",
 * and "Final Expansion to RGBA" operations.
 *
 * Args:  n - number of pixels
 *        rgba - output colors
 *        srcFormat - format of incoming data
 *        srcType - data type of incoming data
 *        src - source data pointer
 *        swapBytes - perform byteswapping of incoming data?
 */
static void
extract_float_rgba(GLuint n, GLfloat rgba[][4],
                   GLenum srcFormat, GLenum srcType, const GLvoid *src,
                   GLboolean swapBytes)
{
   GLint rSrc, gSrc, bSrc, aSrc;
   GLint stride;
   GLint rDst, bDst, gDst, aDst;
   GLboolean intFormat;
   GLfloat rs = 1.0f, gs = 1.0f, bs = 1.0f, as = 1.0f; /* scale factors */

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RG ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT ||
          srcFormat == GL_RED_INTEGER_EXT ||
          srcFormat == GL_GREEN_INTEGER_EXT ||
          srcFormat == GL_BLUE_INTEGER_EXT ||
          srcFormat == GL_ALPHA_INTEGER_EXT ||
          srcFormat == GL_RG_INTEGER ||
          srcFormat == GL_RGB_INTEGER_EXT ||
          srcFormat == GL_RGBA_INTEGER_EXT ||
          srcFormat == GL_BGR_INTEGER_EXT ||
          srcFormat == GL_BGRA_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_ALPHA_INTEGER_EXT);

   ASSERT(srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT ||
          srcType == GL_UNSIGNED_BYTE_3_3_2 ||
          srcType == GL_UNSIGNED_BYTE_2_3_3_REV ||
          srcType == GL_UNSIGNED_SHORT_5_6_5 ||
          srcType == GL_UNSIGNED_SHORT_5_6_5_REV ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4 ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
          srcType == GL_UNSIGNED_SHORT_5_5_5_1 ||
          srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
          srcType == GL_UNSIGNED_INT_8_8_8_8 ||
          srcType == GL_UNSIGNED_INT_8_8_8_8_REV ||
          srcType == GL_UNSIGNED_INT_5_9_9_9_REV ||
          srcType == GL_UNSIGNED_INT_10F_11F_11F_REV);

   get_component_mapping(srcFormat,
                         &rSrc, &gSrc, &bSrc, &aSrc,
                         &rDst, &gDst, &bDst, &aDst);

   stride = _mesa_components_in_format(srcFormat);

   intFormat = _mesa_is_integer_format(srcFormat);

#define PROCESS(SRC_INDEX, DST_INDEX, DEFAULT_FLT, DEFAULT_INT, TYPE, CONVERSION) \
   if ((SRC_INDEX) < 0) {						\
      GLuint i;								\
      if (intFormat) {							\
         for (i = 0; i < n; i++) {					\
            rgba[i][DST_INDEX] = DEFAULT_INT;				\
         }								\
      }									\
      else {								\
         for (i = 0; i < n; i++) {					\
            rgba[i][DST_INDEX] = DEFAULT_FLT;				\
         }								\
      }									\
   }									\
   else if (swapBytes) {						\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         TYPE value = s[SRC_INDEX];					\
         if (sizeof(TYPE) == 2) {					\
            SWAP2BYTE(value);						\
         }								\
         else if (sizeof(TYPE) == 4) {					\
            SWAP4BYTE(value);						\
         }								\
         if (intFormat)							\
            rgba[i][DST_INDEX] = (GLfloat) value;			\
         else								\
            rgba[i][DST_INDEX] = (GLfloat) CONVERSION(value);		\
         s += stride;							\
      }									\
   }									\
   else {								\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      if (intFormat) {							\
         for (i = 0; i < n; i++) {					\
            rgba[i][DST_INDEX] = (GLfloat) s[SRC_INDEX];		\
            s += stride;						\
         }								\
      }									\
      else {								\
         for (i = 0; i < n; i++) {					\
            rgba[i][DST_INDEX] = (GLfloat) CONVERSION(s[SRC_INDEX]);	\
            s += stride;						\
         }								\
      }									\
   }

   switch (srcType) {
      case GL_UNSIGNED_BYTE:
         PROCESS(rSrc, RCOMP, 0.0F,   0, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(gSrc, GCOMP, 0.0F,   0, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(bSrc, BCOMP, 0.0F,   0, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(aSrc, ACOMP, 1.0F, 255, GLubyte, UBYTE_TO_FLOAT);
         break;
      case GL_BYTE:
         PROCESS(rSrc, RCOMP, 0.0F,   0, GLbyte, BYTE_TO_FLOATZ);
         PROCESS(gSrc, GCOMP, 0.0F,   0, GLbyte, BYTE_TO_FLOATZ);
         PROCESS(bSrc, BCOMP, 0.0F,   0, GLbyte, BYTE_TO_FLOATZ);
         PROCESS(aSrc, ACOMP, 1.0F, 127, GLbyte, BYTE_TO_FLOATZ);
         break;
      case GL_UNSIGNED_SHORT:
         PROCESS(rSrc, RCOMP, 0.0F,      0, GLushort, USHORT_TO_FLOAT);
         PROCESS(gSrc, GCOMP, 0.0F,      0, GLushort, USHORT_TO_FLOAT);
         PROCESS(bSrc, BCOMP, 0.0F,      0, GLushort, USHORT_TO_FLOAT);
         PROCESS(aSrc, ACOMP, 1.0F, 0xffff, GLushort, USHORT_TO_FLOAT);
         break;
      case GL_SHORT:
         PROCESS(rSrc, RCOMP, 0.0F,     0, GLshort, SHORT_TO_FLOATZ);
         PROCESS(gSrc, GCOMP, 0.0F,     0, GLshort, SHORT_TO_FLOATZ);
         PROCESS(bSrc, BCOMP, 0.0F,     0, GLshort, SHORT_TO_FLOATZ);
         PROCESS(aSrc, ACOMP, 1.0F, 32767, GLshort, SHORT_TO_FLOATZ);
         break;
      case GL_UNSIGNED_INT:
         PROCESS(rSrc, RCOMP, 0.0F,          0, GLuint, UINT_TO_FLOAT);
         PROCESS(gSrc, GCOMP, 0.0F,          0, GLuint, UINT_TO_FLOAT);
         PROCESS(bSrc, BCOMP, 0.0F,          0, GLuint, UINT_TO_FLOAT);
         PROCESS(aSrc, ACOMP, 1.0F, 0xffffffff, GLuint, UINT_TO_FLOAT);
         break;
      case GL_INT:
         PROCESS(rSrc, RCOMP, 0.0F,          0, GLint, INT_TO_FLOAT);
         PROCESS(gSrc, GCOMP, 0.0F,          0, GLint, INT_TO_FLOAT);
         PROCESS(bSrc, BCOMP, 0.0F,          0, GLint, INT_TO_FLOAT);
         PROCESS(aSrc, ACOMP, 1.0F, 2147483647, GLint, INT_TO_FLOAT);
         break;
      case GL_FLOAT:
         PROCESS(rSrc, RCOMP, 0.0F, 0.0F, GLfloat, (GLfloat));
         PROCESS(gSrc, GCOMP, 0.0F, 0.0F, GLfloat, (GLfloat));
         PROCESS(bSrc, BCOMP, 0.0F, 0.0F, GLfloat, (GLfloat));
         PROCESS(aSrc, ACOMP, 1.0F, 1.0F, GLfloat, (GLfloat));
         break;
      case GL_UNSIGNED_BYTE_3_3_2:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            if (!intFormat) {
               rs = 1.0F / 7.0F;
               gs = 1.0F / 7.0F;
               bs = 1.0F / 3.0F;
            }
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][rDst] = ((p >> 5)      ) * rs;
               rgba[i][gDst] = ((p >> 2) & 0x7) * gs;
               rgba[i][bDst] = ((p     ) & 0x3) * bs;
               rgba[i][aDst] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            if (!intFormat) {
               rs = 1.0F / 7.0F;
               gs = 1.0F / 7.0F;
               bs = 1.0F / 3.0F;
            }
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][rDst] = ((p     ) & 0x7) * rs;
               rgba[i][gDst] = ((p >> 3) & 0x7) * gs;
               rgba[i][bDst] = ((p >> 6)      ) * bs;
               rgba[i][aDst] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5:
         if (!intFormat) {
            rs = 1.0F / 31.0F;
            gs = 1.0F / 63.0F;
            bs = 1.0F / 31.0F;
         }
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p >> 11)       ) * rs;
               rgba[i][gDst] = ((p >>  5) & 0x3f) * gs;
               rgba[i][bDst] = ((p      ) & 0x1f) * bs;
               rgba[i][aDst] = 1.0F;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p >> 11)       ) * rs;
               rgba[i][gDst] = ((p >>  5) & 0x3f) * gs;
               rgba[i][bDst] = ((p      ) & 0x1f) * bs;
               rgba[i][aDst] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (!intFormat) {
            rs = 1.0F / 31.0F;
            gs = 1.0F / 63.0F;
            bs = 1.0F / 31.0F;
         }
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p      ) & 0x1f) * rs;
               rgba[i][gDst] = ((p >>  5) & 0x3f) * gs;
               rgba[i][bDst] = ((p >> 11)       ) * bs;
               rgba[i][aDst] = 1.0F;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p      ) & 0x1f) * rs;
               rgba[i][gDst] = ((p >>  5) & 0x3f) * gs;
               rgba[i][bDst] = ((p >> 11)       ) * bs;
               rgba[i][aDst] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
         if (!intFormat) {
            rs = gs = bs = as = 1.0F / 15.0F;
         }
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p >> 12)      ) * rs;
               rgba[i][gDst] = ((p >>  8) & 0xf) * gs;
               rgba[i][bDst] = ((p >>  4) & 0xf) * bs;
               rgba[i][aDst] = ((p      ) & 0xf) * as;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p >> 12)      ) * rs;
               rgba[i][gDst] = ((p >>  8) & 0xf) * gs;
               rgba[i][bDst] = ((p >>  4) & 0xf) * bs;
               rgba[i][aDst] = ((p      ) & 0xf) * as;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         if (!intFormat) {
            rs = gs = bs = as = 1.0F / 15.0F;
         }
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p      ) & 0xf) * rs;
               rgba[i][gDst] = ((p >>  4) & 0xf) * gs;
               rgba[i][bDst] = ((p >>  8) & 0xf) * bs;
               rgba[i][aDst] = ((p >> 12)      ) * as;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p      ) & 0xf) * rs;
               rgba[i][gDst] = ((p >>  4) & 0xf) * gs;
               rgba[i][bDst] = ((p >>  8) & 0xf) * bs;
               rgba[i][aDst] = ((p >> 12)      ) * as;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_5_5_1:
         if (!intFormat) {
            rs = gs = bs = 1.0F / 31.0F;
         }
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p >> 11)       ) * rs;
               rgba[i][gDst] = ((p >>  6) & 0x1f) * gs;
               rgba[i][bDst] = ((p >>  1) & 0x1f) * bs;
               rgba[i][aDst] = ((p      ) & 0x1)  * as;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p >> 11)       ) * rs;
               rgba[i][gDst] = ((p >>  6) & 0x1f) * gs;
               rgba[i][bDst] = ((p >>  1) & 0x1f) * bs;
               rgba[i][aDst] = ((p      ) & 0x1)  * as;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (!intFormat) {
            rs = gs = bs = 1.0F / 31.0F;
         }
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p      ) & 0x1f) * rs;
               rgba[i][gDst] = ((p >>  5) & 0x1f) * gs;
               rgba[i][bDst] = ((p >> 10) & 0x1f) * bs;
               rgba[i][aDst] = ((p >> 15)       ) * as;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p      ) & 0x1f) * rs;
               rgba[i][gDst] = ((p >>  5) & 0x1f) * gs;
               rgba[i][bDst] = ((p >> 10) & 0x1f) * bs;
               rgba[i][aDst] = ((p >> 15)       ) * as;
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            if (intFormat) {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = (GLfloat) ((p      ) & 0xff);
                  rgba[i][gDst] = (GLfloat) ((p >>  8) & 0xff);
                  rgba[i][bDst] = (GLfloat) ((p >> 16) & 0xff);
                  rgba[i][aDst] = (GLfloat) ((p >> 24)       );
               }
            }
            else {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = UBYTE_TO_FLOAT((p      ) & 0xff);
                  rgba[i][gDst] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
                  rgba[i][bDst] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
                  rgba[i][aDst] = UBYTE_TO_FLOAT((p >> 24)       );
               }
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            if (intFormat) {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = (GLfloat) ((p >> 24)       );
                  rgba[i][gDst] = (GLfloat) ((p >> 16) & 0xff);
                  rgba[i][bDst] = (GLfloat) ((p >>  8) & 0xff);
                  rgba[i][aDst] = (GLfloat) ((p      ) & 0xff);
               }
            }
            else {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = UBYTE_TO_FLOAT((p >> 24)       );
                  rgba[i][gDst] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
                  rgba[i][bDst] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
                  rgba[i][aDst] = UBYTE_TO_FLOAT((p      ) & 0xff);
               }
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            if (intFormat) {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = (GLfloat) ((p >> 24)       );
                  rgba[i][gDst] = (GLfloat) ((p >> 16) & 0xff);
                  rgba[i][bDst] = (GLfloat) ((p >>  8) & 0xff);
                  rgba[i][aDst] = (GLfloat) ((p      ) & 0xff);
               }
            }
            else {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = UBYTE_TO_FLOAT((p >> 24)       );
                  rgba[i][gDst] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
                  rgba[i][bDst] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
                  rgba[i][aDst] = UBYTE_TO_FLOAT((p      ) & 0xff);
               }
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            if (intFormat) {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = (GLfloat) ((p      ) & 0xff);
                  rgba[i][gDst] = (GLfloat) ((p >>  8) & 0xff);
                  rgba[i][bDst] = (GLfloat) ((p >> 16) & 0xff);
                  rgba[i][aDst] = (GLfloat) ((p >> 24)       );
               }
            }
            else {
               for (i = 0; i < n; i ++) {
                  GLuint p = uisrc[i];
                  rgba[i][rDst] = UBYTE_TO_FLOAT((p      ) & 0xff);
                  rgba[i][gDst] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
                  rgba[i][bDst] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
                  rgba[i][aDst] = UBYTE_TO_FLOAT((p >> 24)       );
               }
            }
         }
         break;
      default:
         _mesa_problem(NULL, "bad srcType in extract float data");
         break;
   }
#undef PROCESS
}


static inline GLuint
clamp_float_to_uint(GLfloat f)
{
   return f < 0.0F ? 0 : IROUND(f);
}

/**
 * \sa extract_float_rgba()
 */
static void
extract_uint_rgba(GLuint n, GLuint rgba[][4],
                  GLenum srcFormat, GLenum srcType, const GLvoid *src,
                  GLboolean swapBytes)
{
   GLint rSrc, gSrc, bSrc, aSrc;
   GLint stride;
   GLint rDst, bDst, gDst, aDst;

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RG ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT ||
          srcFormat == GL_RED_INTEGER_EXT ||
          srcFormat == GL_RG_INTEGER ||
          srcFormat == GL_GREEN_INTEGER_EXT ||
          srcFormat == GL_BLUE_INTEGER_EXT ||
          srcFormat == GL_ALPHA_INTEGER_EXT ||
          srcFormat == GL_RGB_INTEGER_EXT ||
          srcFormat == GL_RGBA_INTEGER_EXT ||
          srcFormat == GL_BGR_INTEGER_EXT ||
          srcFormat == GL_BGRA_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_ALPHA_INTEGER_EXT);

   ASSERT(srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT ||
          srcType == GL_UNSIGNED_BYTE_3_3_2 ||
          srcType == GL_UNSIGNED_BYTE_2_3_3_REV ||
          srcType == GL_UNSIGNED_SHORT_5_6_5 ||
          srcType == GL_UNSIGNED_SHORT_5_6_5_REV ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4 ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
          srcType == GL_UNSIGNED_SHORT_5_5_5_1 ||
          srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
          srcType == GL_UNSIGNED_INT_8_8_8_8 ||
          srcType == GL_UNSIGNED_INT_8_8_8_8_REV ||
          srcType == GL_UNSIGNED_INT_5_9_9_9_REV ||
          srcType == GL_UNSIGNED_INT_10F_11F_11F_REV);

   get_component_mapping(srcFormat,
                         &rSrc, &gSrc, &bSrc, &aSrc,
                         &rDst, &gDst, &bDst, &aDst);

   stride = _mesa_components_in_format(srcFormat);

#define PROCESS(SRC_INDEX, DST_INDEX, DEFAULT, TYPE, CONVERSION)	\
   if ((SRC_INDEX) < 0) {						\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         rgba[i][DST_INDEX] = DEFAULT;					\
      }									\
   }									\
   else if (swapBytes) {						\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         TYPE value = s[SRC_INDEX];					\
         if (sizeof(TYPE) == 2) {					\
            SWAP2BYTE(value);						\
         }								\
         else if (sizeof(TYPE) == 4) {					\
            SWAP4BYTE(value);						\
         }								\
         rgba[i][DST_INDEX] = CONVERSION(value);                        \
         s += stride;							\
      }									\
   }									\
   else {								\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         rgba[i][DST_INDEX] = CONVERSION(s[SRC_INDEX]);			\
         s += stride;							\
      }									\
   }

   switch (srcType) {
      case GL_UNSIGNED_BYTE:
         PROCESS(rSrc, RCOMP, 0, GLubyte, (GLuint));
         PROCESS(gSrc, GCOMP, 0, GLubyte, (GLuint));
         PROCESS(bSrc, BCOMP, 0, GLubyte, (GLuint));
         PROCESS(aSrc, ACOMP, 1, GLubyte, (GLuint));
         break;
      case GL_BYTE:
         PROCESS(rSrc, RCOMP, 0, GLbyte, (GLuint));
         PROCESS(gSrc, GCOMP, 0, GLbyte, (GLuint));
         PROCESS(bSrc, BCOMP, 0, GLbyte, (GLuint));
         PROCESS(aSrc, ACOMP, 1, GLbyte, (GLuint));
         break;
      case GL_UNSIGNED_SHORT:
         PROCESS(rSrc, RCOMP, 0, GLushort, (GLuint));
         PROCESS(gSrc, GCOMP, 0, GLushort, (GLuint));
         PROCESS(bSrc, BCOMP, 0, GLushort, (GLuint));
         PROCESS(aSrc, ACOMP, 1, GLushort, (GLuint));
         break;
      case GL_SHORT:
         PROCESS(rSrc, RCOMP, 0, GLshort, (GLuint));
         PROCESS(gSrc, GCOMP, 0, GLshort, (GLuint));
         PROCESS(bSrc, BCOMP, 0, GLshort, (GLuint));
         PROCESS(aSrc, ACOMP, 1, GLshort, (GLuint));
         break;
      case GL_UNSIGNED_INT:
         PROCESS(rSrc, RCOMP, 0, GLuint, (GLuint));
         PROCESS(gSrc, GCOMP, 0, GLuint, (GLuint));
         PROCESS(bSrc, BCOMP, 0, GLuint, (GLuint));
         PROCESS(aSrc, ACOMP, 1, GLuint, (GLuint));
         break;
      case GL_INT:
         PROCESS(rSrc, RCOMP, 0, GLint, (GLuint));
         PROCESS(gSrc, GCOMP, 0, GLint, (GLuint));
         PROCESS(bSrc, BCOMP, 0, GLint, (GLuint));
         PROCESS(aSrc, ACOMP, 1, GLint, (GLuint));
         break;
      case GL_FLOAT:
         PROCESS(rSrc, RCOMP, 0, GLfloat, clamp_float_to_uint);
         PROCESS(gSrc, GCOMP, 0, GLfloat, clamp_float_to_uint);
         PROCESS(bSrc, BCOMP, 0, GLfloat, clamp_float_to_uint);
         PROCESS(aSrc, ACOMP, 1, GLfloat, clamp_float_to_uint);
         break;
      case GL_UNSIGNED_BYTE_3_3_2:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][rDst] = ((p >> 5)      );
               rgba[i][gDst] = ((p >> 2) & 0x7);
               rgba[i][bDst] = ((p     ) & 0x3);
               rgba[i][aDst] = 1;
            }
         }
         break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][rDst] = ((p     ) & 0x7);
               rgba[i][gDst] = ((p >> 3) & 0x7);
               rgba[i][bDst] = ((p >> 6)      );
               rgba[i][aDst] = 1;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p >> 11)       );
               rgba[i][gDst] = ((p >>  5) & 0x3f);
               rgba[i][bDst] = ((p      ) & 0x1f);
               rgba[i][aDst] = 1;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p >> 11)       );
               rgba[i][gDst] = ((p >>  5) & 0x3f);
               rgba[i][bDst] = ((p      ) & 0x1f);
               rgba[i][aDst] = 1;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p      ) & 0x1f);
               rgba[i][gDst] = ((p >>  5) & 0x3f);
               rgba[i][bDst] = ((p >> 11)       );
               rgba[i][aDst] = 1;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p      ) & 0x1f);
               rgba[i][gDst] = ((p >>  5) & 0x3f);
               rgba[i][bDst] = ((p >> 11)       );
               rgba[i][aDst] = 1;
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p >> 12)      );
               rgba[i][gDst] = ((p >>  8) & 0xf);
               rgba[i][bDst] = ((p >>  4) & 0xf);
               rgba[i][aDst] = ((p      ) & 0xf);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p >> 12)      );
               rgba[i][gDst] = ((p >>  8) & 0xf);
               rgba[i][bDst] = ((p >>  4) & 0xf);
               rgba[i][aDst] = ((p      ) & 0xf);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p      ) & 0xf);
               rgba[i][gDst] = ((p >>  4) & 0xf);
               rgba[i][bDst] = ((p >>  8) & 0xf);
               rgba[i][aDst] = ((p >> 12)      );
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p      ) & 0xf);
               rgba[i][gDst] = ((p >>  4) & 0xf);
               rgba[i][bDst] = ((p >>  8) & 0xf);
               rgba[i][aDst] = ((p >> 12)      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_5_5_1:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p >> 11)       );
               rgba[i][gDst] = ((p >>  6) & 0x1f);
               rgba[i][bDst] = ((p >>  1) & 0x1f);
               rgba[i][aDst] = ((p      ) & 0x1 );
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p >> 11)       );
               rgba[i][gDst] = ((p >>  6) & 0x1f);
               rgba[i][bDst] = ((p >>  1) & 0x1f);
               rgba[i][aDst] = ((p      ) & 0x1 );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (swapBytes) {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               SWAP2BYTE(p);
               rgba[i][rDst] = ((p      ) & 0x1f);
               rgba[i][gDst] = ((p >>  5) & 0x1f);
               rgba[i][bDst] = ((p >> 10) & 0x1f);
               rgba[i][aDst] = ((p >> 15)       );
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rDst] = ((p      ) & 0x1f);
               rgba[i][gDst] = ((p >>  5) & 0x1f);
               rgba[i][bDst] = ((p >> 10) & 0x1f);
               rgba[i][aDst] = ((p >> 15)       );
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rDst] = ((p      ) & 0xff);
               rgba[i][gDst] = ((p >>  8) & 0xff);
               rgba[i][bDst] = ((p >> 16) & 0xff);
               rgba[i][aDst] = ((p >> 24)       );
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rDst] = ((p >> 24)       );
               rgba[i][gDst] = ((p >> 16) & 0xff);
               rgba[i][bDst] = ((p >>  8) & 0xff);
               rgba[i][aDst] = ((p      ) & 0xff);
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rDst] = ((p >> 24)       );
               rgba[i][gDst] = ((p >> 16) & 0xff);
               rgba[i][bDst] = ((p >>  8) & 0xff);
               rgba[i][aDst] = ((p      ) & 0xff);
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rDst] = ((p      ) & 0xff);
               rgba[i][gDst] = ((p >>  8) & 0xff);
               rgba[i][bDst] = ((p >> 16) & 0xff);
               rgba[i][aDst] = ((p >> 24)       );
            }
         }
         break;
      default:
         _mesa_problem(NULL, "bad srcType in extract uint data");
         break;
   }
#undef PROCESS
}



/*
 * Unpack a row of color image data from a client buffer according to
 * the pixel unpacking parameters.
 * Return GLubyte values in the specified dest image format.
 * This is used by glDrawPixels and glTexImage?D().
 * \param ctx - the context
 *         n - number of pixels in the span
 *         dstFormat - format of destination color array
 *         dest - the destination color array
 *         srcFormat - source image format
 *         srcType - source image  data type
 *         source - source image pointer
 *         srcPacking - pixel unpacking parameters
 *         transferOps - bitmask of IMAGE_*_BIT values of operations to apply
 *
 * XXX perhaps expand this to process whole images someday.
 */
void
_mesa_unpack_color_span_ubyte(struct gl_context *ctx,
                              GLuint n, GLenum dstFormat, GLubyte dest[],
                              GLenum srcFormat, GLenum srcType,
                              const GLvoid *source,
                              const struct gl_pixelstore_attrib *srcPacking,
                              GLbitfield transferOps )
{
   GLboolean intFormat = _mesa_is_integer_format(srcFormat);
   ASSERT(dstFormat == GL_ALPHA ||
          dstFormat == GL_LUMINANCE ||
          dstFormat == GL_LUMINANCE_ALPHA ||
          dstFormat == GL_INTENSITY ||
          dstFormat == GL_RED ||
          dstFormat == GL_RG ||
          dstFormat == GL_RGB ||
          dstFormat == GL_RGBA);

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RG ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT ||
          srcFormat == GL_COLOR_INDEX);

   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT ||
          srcType == GL_UNSIGNED_BYTE_3_3_2 ||
          srcType == GL_UNSIGNED_BYTE_2_3_3_REV ||
          srcType == GL_UNSIGNED_SHORT_5_6_5 ||
          srcType == GL_UNSIGNED_SHORT_5_6_5_REV ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4 ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
          srcType == GL_UNSIGNED_SHORT_5_5_5_1 ||
          srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
          srcType == GL_UNSIGNED_INT_8_8_8_8 ||
          srcType == GL_UNSIGNED_INT_8_8_8_8_REV ||
          srcType == GL_UNSIGNED_INT_5_9_9_9_REV ||
          srcType == GL_UNSIGNED_INT_10F_11F_11F_REV);

   /* EXT_texture_integer specifies no transfer ops on integer
    * types in the resolved issues section. Just set them to 0
    * for integer surfaces.
    */
   if (intFormat)
      transferOps = 0;

   /* Try simple cases first */
   if (transferOps == 0) {
      if (srcType == GL_UNSIGNED_BYTE) {
         if (dstFormat == GL_RGBA) {
            if (srcFormat == GL_RGBA) {
               memcpy( dest, source, n * 4 * sizeof(GLubyte) );
               return;
            }
            else if (srcFormat == GL_RGB) {
               GLuint i;
               const GLubyte *src = (const GLubyte *) source;
               GLubyte *dst = dest;
               for (i = 0; i < n; i++) {
                  dst[0] = src[0];
                  dst[1] = src[1];
                  dst[2] = src[2];
                  dst[3] = 255;
                  src += 3;
                  dst += 4;
               }
               return;
            }
         }
         else if (dstFormat == GL_RGB) {
            if (srcFormat == GL_RGB) {
               memcpy( dest, source, n * 3 * sizeof(GLubyte) );
               return;
            }
            else if (srcFormat == GL_RGBA) {
               GLuint i;
               const GLubyte *src = (const GLubyte *) source;
               GLubyte *dst = dest;
               for (i = 0; i < n; i++) {
                  dst[0] = src[0];
                  dst[1] = src[1];
                  dst[2] = src[2];
                  src += 4;
                  dst += 3;
               }
               return;
            }
         }
         else if (dstFormat == srcFormat) {
            GLint comps = _mesa_components_in_format(srcFormat);
            assert(comps > 0);
            memcpy( dest, source, n * comps * sizeof(GLubyte) );
            return;
         }
      }
   }


   /* general solution begins here */
   {
      GLint dstComponents;
      GLint rDst, gDst, bDst, aDst, lDst, iDst;
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) malloc(4 * n * sizeof(GLfloat));

      if (!rgba) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel unpacking");
         return;
      }

      dstComponents = _mesa_components_in_format( dstFormat );
      /* source & dest image formats should have been error checked by now */
      assert(dstComponents > 0);

      /*
       * Extract image data and convert to RGBA floats
       */
      if (srcFormat == GL_COLOR_INDEX) {
         GLuint *indexes = (GLuint *) malloc(n * sizeof(GLuint));

         if (!indexes) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel unpacking");
            free(rgba);
            return;
         }

         extract_uint_indexes(n, indexes, srcFormat, srcType, source,
                              srcPacking);

	 /* Convert indexes to RGBA */
	 if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
	    _mesa_shift_and_offset_ci(ctx, n, indexes);
	 }
	 _mesa_map_ci_to_rgba(ctx, n, indexes, rgba);

         /* Don't do RGBA scale/bias or RGBA->RGBA mapping if starting
          * with color indexes.
          */
         transferOps &= ~(IMAGE_SCALE_BIAS_BIT | IMAGE_MAP_COLOR_BIT);

         free(indexes);
      }
      else {
         /* non-color index data */
         extract_float_rgba(n, rgba, srcFormat, srcType, source,
                            srcPacking->SwapBytes);
      }

      /* Need to clamp if returning GLubytes */
      transferOps |= IMAGE_CLAMP_BIT;

      if (transferOps) {
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, n, rgba);
      }

      get_component_indexes(dstFormat,
                            &rDst, &gDst, &bDst, &aDst, &lDst, &iDst);

      /* Now return the GLubyte data in the requested dstFormat */
      if (rDst >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_UBYTE(dst[rDst], rgba[i][RCOMP]);
            dst += dstComponents;
         }
      }

      if (gDst >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_UBYTE(dst[gDst], rgba[i][GCOMP]);
            dst += dstComponents;
         }
      }

      if (bDst >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_UBYTE(dst[bDst], rgba[i][BCOMP]);
            dst += dstComponents;
         }
      }

      if (aDst >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_UBYTE(dst[aDst], rgba[i][ACOMP]);
            dst += dstComponents;
         }
      }

      if (iDst >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         assert(iDst == 0);
         assert(dstComponents == 1);
         for (i = 0; i < n; i++) {
            /* Intensity comes from red channel */
            CLAMPED_FLOAT_TO_UBYTE(dst[i], rgba[i][RCOMP]);
         }
      }

      if (lDst >= 0) {
         GLubyte *dst = dest;
         GLuint i;
         assert(lDst == 0);
         for (i = 0; i < n; i++) {
            /* Luminance comes from red channel */
            CLAMPED_FLOAT_TO_UBYTE(dst[0], rgba[i][RCOMP]);
            dst += dstComponents;
         }
      }

      free(rgba);
   }
}


/**
 * Same as _mesa_unpack_color_span_ubyte(), but return GLfloat data
 * instead of GLubyte.
 */
void
_mesa_unpack_color_span_float( struct gl_context *ctx,
                               GLuint n, GLenum dstFormat, GLfloat dest[],
                               GLenum srcFormat, GLenum srcType,
                               const GLvoid *source,
                               const struct gl_pixelstore_attrib *srcPacking,
                               GLbitfield transferOps )
{
   ASSERT(dstFormat == GL_ALPHA ||
          dstFormat == GL_LUMINANCE ||
          dstFormat == GL_LUMINANCE_ALPHA ||
          dstFormat == GL_INTENSITY ||
          dstFormat == GL_RED ||
          dstFormat == GL_RG ||
          dstFormat == GL_RGB ||
          dstFormat == GL_RGBA);

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RG ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT ||
          srcFormat == GL_RED_INTEGER_EXT ||
          srcFormat == GL_GREEN_INTEGER_EXT ||
          srcFormat == GL_BLUE_INTEGER_EXT ||
          srcFormat == GL_ALPHA_INTEGER_EXT ||
          srcFormat == GL_RG_INTEGER ||
          srcFormat == GL_RGB_INTEGER_EXT ||
          srcFormat == GL_RGBA_INTEGER_EXT ||
          srcFormat == GL_BGR_INTEGER_EXT ||
          srcFormat == GL_BGRA_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_ALPHA_INTEGER_EXT ||
          srcFormat == GL_COLOR_INDEX);

   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT ||
          srcType == GL_UNSIGNED_BYTE_3_3_2 ||
          srcType == GL_UNSIGNED_BYTE_2_3_3_REV ||
          srcType == GL_UNSIGNED_SHORT_5_6_5 ||
          srcType == GL_UNSIGNED_SHORT_5_6_5_REV ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4 ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
          srcType == GL_UNSIGNED_SHORT_5_5_5_1 ||
          srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
          srcType == GL_UNSIGNED_INT_8_8_8_8 ||
          srcType == GL_UNSIGNED_INT_8_8_8_8_REV ||
          srcType == GL_UNSIGNED_INT_5_9_9_9_REV ||
          srcType == GL_UNSIGNED_INT_10F_11F_11F_REV);

   /* general solution, no special cases, yet */
   {
      GLint dstComponents;
      GLint rDst, gDst, bDst, aDst, lDst, iDst;
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) malloc(4 * n * sizeof(GLfloat));
      GLboolean intFormat = _mesa_is_integer_format(srcFormat);

      if (!rgba) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel unpacking");
         return;
      }

      dstComponents = _mesa_components_in_format( dstFormat );
      /* source & dest image formats should have been error checked by now */
      assert(dstComponents > 0);

      /* EXT_texture_integer specifies no transfer ops on integer
       * types in the resolved issues section. Just set them to 0
       * for integer surfaces.
       */
      if (intFormat)
         transferOps = 0;

      /*
       * Extract image data and convert to RGBA floats
       */
      if (srcFormat == GL_COLOR_INDEX) {
         GLuint *indexes = (GLuint *) malloc(n * sizeof(GLuint));

         if (!indexes) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel unpacking");
            free(rgba);
            return;
         }

         extract_uint_indexes(n, indexes, srcFormat, srcType, source,
                              srcPacking);

	 /* Convert indexes to RGBA */
	 if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
	    _mesa_shift_and_offset_ci(ctx, n, indexes);
	 }
	 _mesa_map_ci_to_rgba(ctx, n, indexes, rgba);

         /* Don't do RGBA scale/bias or RGBA->RGBA mapping if starting
          * with color indexes.
          */
         transferOps &= ~(IMAGE_SCALE_BIAS_BIT | IMAGE_MAP_COLOR_BIT);

         free(indexes);
      }
      else {
         /* non-color index data */
         extract_float_rgba(n, rgba, srcFormat, srcType, source,
                            srcPacking->SwapBytes);
      }

      if (transferOps) {
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, n, rgba);
      }

      get_component_indexes(dstFormat,
                            &rDst, &gDst, &bDst, &aDst, &lDst, &iDst);

      /* Now pack results in the requested dstFormat */
      if (rDst >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[rDst] = rgba[i][RCOMP];
            dst += dstComponents;
         }
      }

      if (gDst >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[gDst] = rgba[i][GCOMP];
            dst += dstComponents;
         }
      }

      if (bDst >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[bDst] = rgba[i][BCOMP];
            dst += dstComponents;
         }
      }

      if (aDst >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[aDst] = rgba[i][ACOMP];
            dst += dstComponents;
         }
      }

      if (iDst >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         assert(iDst == 0);
         assert(dstComponents == 1);
         for (i = 0; i < n; i++) {
            /* Intensity comes from red channel */
            dst[i] = rgba[i][RCOMP];
         }
      }

      if (lDst >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         assert(lDst == 0);
         for (i = 0; i < n; i++) {
            /* Luminance comes from red channel */
            dst[0] = rgba[i][RCOMP];
            dst += dstComponents;
         }
      }

      free(rgba);
   }
}


/**
 * Same as _mesa_unpack_color_span_ubyte(), but return GLuint data
 * instead of GLubyte.
 * No pixel transfer ops are applied.
 */
void
_mesa_unpack_color_span_uint(struct gl_context *ctx,
                             GLuint n, GLenum dstFormat, GLuint *dest,
                             GLenum srcFormat, GLenum srcType,
                             const GLvoid *source,
                             const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint (*rgba)[4] = (GLuint (*)[4]) malloc(n * 4 * sizeof(GLfloat));

   if (!rgba) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel unpacking");
      return;
   }

   ASSERT(dstFormat == GL_ALPHA ||
          dstFormat == GL_LUMINANCE ||
          dstFormat == GL_LUMINANCE_ALPHA ||
          dstFormat == GL_INTENSITY ||
          dstFormat == GL_RED ||
          dstFormat == GL_RG ||
          dstFormat == GL_RGB ||
          dstFormat == GL_RGBA);

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RG ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT ||
          srcFormat == GL_RED_INTEGER_EXT ||
          srcFormat == GL_GREEN_INTEGER_EXT ||
          srcFormat == GL_BLUE_INTEGER_EXT ||
          srcFormat == GL_ALPHA_INTEGER_EXT ||
          srcFormat == GL_RG_INTEGER ||
          srcFormat == GL_RGB_INTEGER_EXT ||
          srcFormat == GL_RGBA_INTEGER_EXT ||
          srcFormat == GL_BGR_INTEGER_EXT ||
          srcFormat == GL_BGRA_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_INTEGER_EXT ||
          srcFormat == GL_LUMINANCE_ALPHA_INTEGER_EXT);

   ASSERT(srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT ||
          srcType == GL_UNSIGNED_BYTE_3_3_2 ||
          srcType == GL_UNSIGNED_BYTE_2_3_3_REV ||
          srcType == GL_UNSIGNED_SHORT_5_6_5 ||
          srcType == GL_UNSIGNED_SHORT_5_6_5_REV ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4 ||
          srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ||
          srcType == GL_UNSIGNED_SHORT_5_5_5_1 ||
          srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ||
          srcType == GL_UNSIGNED_INT_8_8_8_8 ||
          srcType == GL_UNSIGNED_INT_8_8_8_8_REV ||
          srcType == GL_UNSIGNED_INT_5_9_9_9_REV ||
          srcType == GL_UNSIGNED_INT_10F_11F_11F_REV);


   /* Extract image data as uint[4] pixels */
   extract_uint_rgba(n, rgba, srcFormat, srcType, source,
                     srcPacking->SwapBytes);

   if (dstFormat == GL_RGBA) {
      /* simple case */
      memcpy(dest, rgba, 4 * sizeof(GLuint) * n);
   }
   else {
      /* general case */
      GLint rDst, gDst, bDst, aDst, lDst, iDst;
      GLint dstComponents = _mesa_components_in_format( dstFormat );

      assert(dstComponents > 0);

      get_component_indexes(dstFormat,
                            &rDst, &gDst, &bDst, &aDst, &lDst, &iDst);

      /* Now pack values in the requested dest format */
      if (rDst >= 0) {
         GLuint *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[rDst] = rgba[i][RCOMP];
            dst += dstComponents;
         }
      }

      if (gDst >= 0) {
         GLuint *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[gDst] = rgba[i][GCOMP];
            dst += dstComponents;
         }
      }

      if (bDst >= 0) {
         GLuint *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[bDst] = rgba[i][BCOMP];
            dst += dstComponents;
         }
      }

      if (aDst >= 0) {
         GLuint *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[aDst] = rgba[i][ACOMP];
            dst += dstComponents;
         }
      }

      if (iDst >= 0) {
         GLuint *dst = dest;
         GLuint i;
         assert(iDst == 0);
         assert(dstComponents == 1);
         for (i = 0; i < n; i++) {
            /* Intensity comes from red channel */
            dst[i] = rgba[i][RCOMP];
         }
      }

      if (lDst >= 0) {
         GLuint *dst = dest;
         GLuint i;
         assert(lDst == 0);
         for (i = 0; i < n; i++) {
            /* Luminance comes from red channel */
            dst[0] = rgba[i][RCOMP];
            dst += dstComponents;
         }
      }
   }

   free(rgba);
}


/*
 * Unpack a row of color index data from a client buffer according to
 * the pixel unpacking parameters.
 * This is (or will be) used by glDrawPixels, glTexImage[123]D, etc.
 *
 * Args:  ctx - the context
 *        n - number of pixels
 *        dstType - destination data type
 *        dest - destination array
 *        srcType - source pixel type
 *        source - source data pointer
 *        srcPacking - pixel unpacking parameters
 *        transferOps - the pixel transfer operations to apply
 */
void
_mesa_unpack_index_span( struct gl_context *ctx, GLuint n,
                         GLenum dstType, GLvoid *dest,
                         GLenum srcType, const GLvoid *source,
                         const struct gl_pixelstore_attrib *srcPacking,
                         GLbitfield transferOps )
{
   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT);

   ASSERT(dstType == GL_UNSIGNED_BYTE ||
          dstType == GL_UNSIGNED_SHORT ||
          dstType == GL_UNSIGNED_INT);


   transferOps &= (IMAGE_MAP_COLOR_BIT | IMAGE_SHIFT_OFFSET_BIT);

   /*
    * Try simple cases first
    */
   if (transferOps == 0 && srcType == GL_UNSIGNED_BYTE
       && dstType == GL_UNSIGNED_BYTE) {
      memcpy(dest, source, n * sizeof(GLubyte));
   }
   else if (transferOps == 0 && srcType == GL_UNSIGNED_INT
            && dstType == GL_UNSIGNED_INT && !srcPacking->SwapBytes) {
      memcpy(dest, source, n * sizeof(GLuint));
   }
   else {
      /*
       * general solution
       */
      GLuint *indexes = (GLuint *) malloc(n * sizeof(GLuint));

      if (!indexes) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel unpacking");
         return;
      }

      extract_uint_indexes(n, indexes, GL_COLOR_INDEX, srcType, source,
                           srcPacking);

      if (transferOps)
         _mesa_apply_ci_transfer_ops(ctx, transferOps, n, indexes);

      /* convert to dest type */
      switch (dstType) {
         case GL_UNSIGNED_BYTE:
            {
               GLubyte *dst = (GLubyte *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLubyte) (indexes[i] & 0xff);
               }
            }
            break;
         case GL_UNSIGNED_SHORT:
            {
               GLuint *dst = (GLuint *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLushort) (indexes[i] & 0xffff);
               }
            }
            break;
         case GL_UNSIGNED_INT:
            memcpy(dest, indexes, n * sizeof(GLuint));
            break;
         default:
            _mesa_problem(ctx, "bad dstType in _mesa_unpack_index_span");
      }

      free(indexes);
   }
}


void
_mesa_pack_index_span( struct gl_context *ctx, GLuint n,
                       GLenum dstType, GLvoid *dest, const GLuint *source,
                       const struct gl_pixelstore_attrib *dstPacking,
                       GLbitfield transferOps )
{
   GLuint *indexes = (GLuint *) malloc(n * sizeof(GLuint));

   if (!indexes) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel packing");
      return;
   }

   transferOps &= (IMAGE_MAP_COLOR_BIT | IMAGE_SHIFT_OFFSET_BIT);

   if (transferOps & (IMAGE_MAP_COLOR_BIT | IMAGE_SHIFT_OFFSET_BIT)) {
      /* make a copy of input */
      memcpy(indexes, source, n * sizeof(GLuint));
      _mesa_apply_ci_transfer_ops(ctx, transferOps, n, indexes);
      source = indexes;
   }

   switch (dstType) {
   case GL_UNSIGNED_BYTE:
      {
         GLubyte *dst = (GLubyte *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            *dst++ = (GLubyte) source[i];
         }
      }
      break;
   case GL_BYTE:
      {
         GLbyte *dst = (GLbyte *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = (GLbyte) source[i];
         }
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         GLushort *dst = (GLushort *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = (GLushort) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   case GL_SHORT:
      {
         GLshort *dst = (GLshort *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = (GLshort) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   case GL_UNSIGNED_INT:
      {
         GLuint *dst = (GLuint *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = (GLuint) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   case GL_INT:
      {
         GLint *dst = (GLint *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = (GLint) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   case GL_FLOAT:
      {
         GLfloat *dst = (GLfloat *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = (GLfloat) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   default:
      _mesa_problem(ctx, "bad type in _mesa_pack_index_span");
   }

   free(indexes);
}


/*
 * Unpack a row of stencil data from a client buffer according to
 * the pixel unpacking parameters.
 * This is (or will be) used by glDrawPixels
 *
 * Args:  ctx - the context
 *        n - number of pixels
 *        dstType - destination data type
 *        dest - destination array
 *        srcType - source pixel type
 *        source - source data pointer
 *        srcPacking - pixel unpacking parameters
 *        transferOps - apply offset/bias/lookup ops?
 */
void
_mesa_unpack_stencil_span( struct gl_context *ctx, GLuint n,
                           GLenum dstType, GLvoid *dest,
                           GLenum srcType, const GLvoid *source,
                           const struct gl_pixelstore_attrib *srcPacking,
                           GLbitfield transferOps )
{
   ASSERT(srcType == GL_BITMAP ||
          srcType == GL_UNSIGNED_BYTE ||
          srcType == GL_BYTE ||
          srcType == GL_UNSIGNED_SHORT ||
          srcType == GL_SHORT ||
          srcType == GL_UNSIGNED_INT ||
          srcType == GL_INT ||
          srcType == GL_HALF_FLOAT_ARB ||
          srcType == GL_FLOAT);

   ASSERT(dstType == GL_UNSIGNED_BYTE ||
          dstType == GL_UNSIGNED_SHORT ||
          dstType == GL_UNSIGNED_INT);

   /* only shift and offset apply to stencil */
   transferOps &= IMAGE_SHIFT_OFFSET_BIT;

   /*
    * Try simple cases first
    */
   if (transferOps == 0 &&
       !ctx->Pixel.MapStencilFlag &&
       srcType == GL_UNSIGNED_BYTE &&
       dstType == GL_UNSIGNED_BYTE) {
      memcpy(dest, source, n * sizeof(GLubyte));
   }
   else if (transferOps == 0 &&
            !ctx->Pixel.MapStencilFlag &&
            srcType == GL_UNSIGNED_INT &&
            dstType == GL_UNSIGNED_INT &&
            !srcPacking->SwapBytes) {
      memcpy(dest, source, n * sizeof(GLuint));
   }
   else {
      /*
       * general solution
       */
      GLuint *indexes = (GLuint *) malloc(n * sizeof(GLuint));

      if (!indexes) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "stencil unpacking");
         return;
      }

      extract_uint_indexes(n, indexes, GL_STENCIL_INDEX, srcType, source,
                           srcPacking);

      if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
         /* shift and offset indexes */
         _mesa_shift_and_offset_ci(ctx, n, indexes);
      }

      if (ctx->Pixel.MapStencilFlag) {
         /* Apply stencil lookup table */
         const GLuint mask = ctx->PixelMaps.StoS.Size - 1;
         GLuint i;
         for (i = 0; i < n; i++) {
            indexes[i] = (GLuint)ctx->PixelMaps.StoS.Map[ indexes[i] & mask ];
         }
      }

      /* convert to dest type */
      switch (dstType) {
         case GL_UNSIGNED_BYTE:
            {
               GLubyte *dst = (GLubyte *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLubyte) (indexes[i] & 0xff);
               }
            }
            break;
         case GL_UNSIGNED_SHORT:
            {
               GLuint *dst = (GLuint *) dest;
               GLuint i;
               for (i = 0; i < n; i++) {
                  dst[i] = (GLushort) (indexes[i] & 0xffff);
               }
            }
            break;
         case GL_UNSIGNED_INT:
            memcpy(dest, indexes, n * sizeof(GLuint));
            break;
         default:
            _mesa_problem(ctx, "bad dstType in _mesa_unpack_stencil_span");
      }

      free(indexes);
   }
}


void
_mesa_pack_stencil_span( struct gl_context *ctx, GLuint n,
                         GLenum dstType, GLvoid *dest, const GLubyte *source,
                         const struct gl_pixelstore_attrib *dstPacking )
{
   GLubyte *stencil = (GLubyte *) malloc(n * sizeof(GLubyte));

   if (!stencil) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "stencil packing");
      return;
   }

   if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset ||
       ctx->Pixel.MapStencilFlag) {
      /* make a copy of input */
      memcpy(stencil, source, n * sizeof(GLubyte));
      _mesa_apply_stencil_transfer_ops(ctx, n, stencil);
      source = stencil;
   }

   switch (dstType) {
   case GL_UNSIGNED_BYTE:
      memcpy(dest, source, n);
      break;
   case GL_BYTE:
      {
         GLbyte *dst = (GLbyte *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLbyte) (source[i] & 0x7f);
         }
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         GLushort *dst = (GLushort *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLushort) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   case GL_SHORT:
      {
         GLshort *dst = (GLshort *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLshort) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   case GL_UNSIGNED_INT:
      {
         GLuint *dst = (GLuint *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLuint) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   case GL_INT:
      {
         GLint *dst = (GLint *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLint) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   case GL_FLOAT:
      {
         GLfloat *dst = (GLfloat *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLfloat) source[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   case GL_BITMAP:
      if (dstPacking->LsbFirst) {
         GLubyte *dst = (GLubyte *) dest;
         GLint shift = 0;
         GLuint i;
         for (i = 0; i < n; i++) {
            if (shift == 0)
               *dst = 0;
            *dst |= ((source[i] != 0) << shift);
            shift++;
            if (shift == 8) {
               shift = 0;
               dst++;
            }
         }
      }
      else {
         GLubyte *dst = (GLubyte *) dest;
         GLint shift = 7;
         GLuint i;
         for (i = 0; i < n; i++) {
            if (shift == 7)
               *dst = 0;
            *dst |= ((source[i] != 0) << shift);
            shift--;
            if (shift < 0) {
               shift = 7;
               dst++;
            }
         }
      }
      break;
   default:
      _mesa_problem(ctx, "bad type in _mesa_pack_index_span");
   }

   free(stencil);
}

#define DEPTH_VALUES(GLTYPE, GLTYPE2FLOAT)                              \
    do {                                                                \
        GLuint i;                                                       \
        const GLTYPE *src = (const GLTYPE *)source;                     \
        for (i = 0; i < n; i++) {                                       \
            GLTYPE value = src[i];                                      \
            if (srcPacking->SwapBytes) {                                \
                if (sizeof(GLTYPE) == 2) {                              \
                    SWAP2BYTE(value);                                   \
                } else if (sizeof(GLTYPE) == 4) {                       \
                    SWAP4BYTE(value);                                   \
                }                                                       \
            }                                                           \
            depthValues[i] = GLTYPE2FLOAT(value);                       \
        }                                                               \
    } while (0)


/**
 * Unpack a row of depth/z values from memory, returning GLushort, GLuint
 * or GLfloat values.
 * The glPixelTransfer (scale/bias) params will be applied.
 *
 * \param dstType  one of GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, GL_FLOAT
 * \param depthMax  max value for returned GLushort or GLuint values
 *                  (ignored for GLfloat).
 */
void
_mesa_unpack_depth_span( struct gl_context *ctx, GLuint n,
                         GLenum dstType, GLvoid *dest, GLuint depthMax,
                         GLenum srcType, const GLvoid *source,
                         const struct gl_pixelstore_attrib *srcPacking )
{
   GLfloat *depthTemp = NULL, *depthValues;
   GLboolean needClamp = GL_FALSE;

   /* Look for special cases first.
    * Not only are these faster, they're less prone to numeric conversion
    * problems.  Otherwise, converting from an int type to a float then
    * back to an int type can introduce errors that will show up as
    * artifacts in things like depth peeling which uses glCopyTexImage.
    */
   if (ctx->Pixel.DepthScale == 1.0 && ctx->Pixel.DepthBias == 0.0) {
      if (srcType == GL_UNSIGNED_INT && dstType == GL_UNSIGNED_SHORT) {
         const GLuint *src = (const GLuint *) source;
         GLushort *dst = (GLushort *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = src[i] >> 16;
         }
         return;
      }
      if (srcType == GL_UNSIGNED_SHORT
          && dstType == GL_UNSIGNED_INT
          && depthMax == 0xffffffff) {
         const GLushort *src = (const GLushort *) source;
         GLuint *dst = (GLuint *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = src[i] | (src[i] << 16);
         }
         return;
      }
      /* XXX may want to add additional cases here someday */
   }

   /* general case path follows */

   if (dstType == GL_FLOAT) {
      depthValues = (GLfloat *) dest;
   }
   else {
      depthTemp = (GLfloat *) malloc(n * sizeof(GLfloat));
      if (!depthTemp) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel unpacking");
         return;
      }

      depthValues = depthTemp;
   }

   /* Convert incoming values to GLfloat.  Some conversions will require
    * clamping, below.
    */
   switch (srcType) {
      case GL_BYTE:
         DEPTH_VALUES(GLbyte, BYTE_TO_FLOATZ);
         needClamp = GL_TRUE;
         break;
      case GL_UNSIGNED_BYTE:
         DEPTH_VALUES(GLubyte, UBYTE_TO_FLOAT);
         break;
      case GL_SHORT:
         DEPTH_VALUES(GLshort, SHORT_TO_FLOATZ);
         needClamp = GL_TRUE;
         break;
      case GL_UNSIGNED_SHORT:
         DEPTH_VALUES(GLushort, USHORT_TO_FLOAT);
         break;
      case GL_INT:
         DEPTH_VALUES(GLint, INT_TO_FLOAT);
         needClamp = GL_TRUE;
         break;
      case GL_UNSIGNED_INT:
         DEPTH_VALUES(GLuint, UINT_TO_FLOAT);
         break;
      case GL_FLOAT:
         DEPTH_VALUES(GLfloat, 1*);
         needClamp = GL_TRUE;
         break;
      default:
         _mesa_problem(NULL, "bad type in _mesa_unpack_depth_span()");
         free(depthTemp);
         return;
   }

   /* apply depth scale and bias */
   {
      const GLfloat scale = ctx->Pixel.DepthScale;
      const GLfloat bias = ctx->Pixel.DepthBias;
      if (scale != 1.0 || bias != 0.0) {
         GLuint i;
         for (i = 0; i < n; i++) {
            depthValues[i] = depthValues[i] * scale + bias;
         }
         needClamp = GL_TRUE;
      }
   }

   /* clamp to [0, 1] */
   if (needClamp) {
      GLuint i;
      for (i = 0; i < n; i++) {
         depthValues[i] = (GLfloat)CLAMP(depthValues[i], 0.0, 1.0);
      }
   }

   /*
    * Convert values to dstType
    */
   if (dstType == GL_UNSIGNED_INT) {
      GLuint *zValues = (GLuint *) dest;
      GLuint i;
      if (depthMax <= 0xffffff) {
         /* no overflow worries */
         for (i = 0; i < n; i++) {
            zValues[i] = (GLuint) (depthValues[i] * (GLfloat) depthMax);
         }
      }
      else {
         /* need to use double precision to prevent overflow problems */
         for (i = 0; i < n; i++) {
            GLdouble z = depthValues[i] * (GLfloat) depthMax;
            if (z >= (GLdouble) 0xffffffff)
               zValues[i] = 0xffffffff;
            else
               zValues[i] = (GLuint) z;
         }
      }
   }
   else if (dstType == GL_UNSIGNED_SHORT) {
      GLushort *zValues = (GLushort *) dest;
      GLuint i;
      ASSERT(depthMax <= 0xffff);
      for (i = 0; i < n; i++) {
         zValues[i] = (GLushort) (depthValues[i] * (GLfloat) depthMax);
      }
   }
   else if (dstType == GL_FLOAT) {
      /* Nothing to do. depthValues is pointing to dest. */
   }
   else {
      ASSERT(0);
   }

   free(depthTemp);
}


/*
 * Pack an array of depth values.  The values are floats in [0,1].
 */
void
_mesa_pack_depth_span( struct gl_context *ctx, GLuint n, GLvoid *dest,
                       GLenum dstType, const GLfloat *depthSpan,
                       const struct gl_pixelstore_attrib *dstPacking )
{
   GLfloat *depthCopy = (GLfloat *) malloc(n * sizeof(GLfloat));
   if (!depthCopy) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "pixel packing");
      return;
   }

   if (ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0) {
      memcpy(depthCopy, depthSpan, n * sizeof(GLfloat));
      _mesa_scale_and_bias_depth(ctx, n, depthCopy);
      depthSpan = depthCopy;
   }

   switch (dstType) {
   case GL_UNSIGNED_BYTE:
      {
         GLubyte *dst = (GLubyte *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = FLOAT_TO_UBYTE( depthSpan[i] );
         }
      }
      break;
   case GL_BYTE:
      {
         GLbyte *dst = (GLbyte *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = FLOAT_TO_BYTE( depthSpan[i] );
         }
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         GLushort *dst = (GLushort *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_USHORT(dst[i], depthSpan[i]);
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   case GL_SHORT:
      {
         GLshort *dst = (GLshort *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = FLOAT_TO_SHORT( depthSpan[i] );
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   case GL_UNSIGNED_INT:
      {
         GLuint *dst = (GLuint *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = FLOAT_TO_UINT( depthSpan[i] );
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   case GL_INT:
      {
         GLint *dst = (GLint *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = FLOAT_TO_INT( depthSpan[i] );
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   case GL_FLOAT:
      {
         GLfloat *dst = (GLfloat *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = depthSpan[i];
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap4( (GLuint *) dst, n );
         }
      }
      break;
   default:
      _mesa_problem(ctx, "bad type in _mesa_pack_depth_span");
   }

   free(depthCopy);
}



/**
 * Unpack image data.  Apply byte swapping, byte flipping (bitmap).
 * Return all image data in a contiguous block.  This is used when we
 * compile glDrawPixels, glTexImage, etc into a display list.  We
 * need a copy of the data in a standard format.
 */
void *
_mesa_unpack_image( GLuint dimensions,
                    GLsizei width, GLsizei height, GLsizei depth,
                    GLenum format, GLenum type, const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *unpack )
{
   GLint bytesPerRow, compsPerRow;
   GLboolean flipBytes, swap2, swap4;

   if (!pixels)
      return NULL;  /* not necessarily an error */

   if (width <= 0 || height <= 0 || depth <= 0)
      return NULL;  /* generate error later */

   if (type == GL_BITMAP) {
      bytesPerRow = (width + 7) >> 3;
      flipBytes = unpack->LsbFirst;
      swap2 = swap4 = GL_FALSE;
      compsPerRow = 0;
   }
   else {
      const GLint bytesPerPixel = _mesa_bytes_per_pixel(format, type);
      GLint components = _mesa_components_in_format(format);
      GLint bytesPerComp;

      if (_mesa_type_is_packed(type))
          components = 1;

      if (bytesPerPixel <= 0 || components <= 0)
         return NULL;   /* bad format or type.  generate error later */
      bytesPerRow = bytesPerPixel * width;
      bytesPerComp = bytesPerPixel / components;
      flipBytes = GL_FALSE;
      swap2 = (bytesPerComp == 2) && unpack->SwapBytes;
      swap4 = (bytesPerComp == 4) && unpack->SwapBytes;
      compsPerRow = components * width;
      assert(compsPerRow >= width);
   }

   {
      GLubyte *destBuffer
         = (GLubyte *) malloc(bytesPerRow * height * depth);
      GLubyte *dst;
      GLint img, row;
      if (!destBuffer)
         return NULL;   /* generate GL_OUT_OF_MEMORY later */

      dst = destBuffer;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            const GLvoid *src = _mesa_image_address(dimensions, unpack, pixels,
                               width, height, format, type, img, row, 0);

            if ((type == GL_BITMAP) && (unpack->SkipPixels & 0x7)) {
               GLint i;
               flipBytes = GL_FALSE;
               if (unpack->LsbFirst) {
                  GLubyte srcMask = 1 << (unpack->SkipPixels & 0x7);
                  GLubyte dstMask = 128;
                  const GLubyte *s = src;
                  GLubyte *d = dst;
                  *d = 0;
                  for (i = 0; i < width; i++) {
                     if (*s & srcMask) {
                        *d |= dstMask;
                     }      
                     if (srcMask == 128) {
                        srcMask = 1;
                        s++;
                     }
                     else {
                        srcMask = srcMask << 1;
                     }
                     if (dstMask == 1) {
                        dstMask = 128;
                        d++;
                        *d = 0;
                     }
                     else {
                        dstMask = dstMask >> 1;
                     }
                  }
               }
               else {
                  GLubyte srcMask = 128 >> (unpack->SkipPixels & 0x7);
                  GLubyte dstMask = 128;
                  const GLubyte *s = src;
                  GLubyte *d = dst;
                  *d = 0;
                  for (i = 0; i < width; i++) {
                     if (*s & srcMask) {
                        *d |= dstMask;
                     }
                     if (srcMask == 1) {
                        srcMask = 128;
                        s++;
                     }
                     else {
                        srcMask = srcMask >> 1;
                     }
                     if (dstMask == 1) {
                        dstMask = 128;
                        d++;
                        *d = 0;
                     }
                     else {
                        dstMask = dstMask >> 1;
                     }      
                  }
               }
            }
            else {
               memcpy(dst, src, bytesPerRow);
            }

            /* byte flipping/swapping */
            if (flipBytes) {
               flip_bytes((GLubyte *) dst, bytesPerRow);
            }
            else if (swap2) {
               _mesa_swap2((GLushort*) dst, compsPerRow);
            }
            else if (swap4) {
               _mesa_swap4((GLuint*) dst, compsPerRow);
            }
            dst += bytesPerRow;
         }
      }
      return destBuffer;
   }
}



/**
 * If we unpack colors from a luminance surface, we'll get pixel colors
 * such as (l, l, l, a).
 * When we call _mesa_pack_rgba_span_float(format=GL_LUMINANCE), that
 * function will compute L=R+G+B before packing.  The net effect is we'll
 * accidentally store luminance values = 3*l.
 * This function compensates for that by converting (aka rebasing) (l,l,l,a)
 * to be (l,0,0,a).
 * It's a similar story for other formats such as LUMINANCE_ALPHA, ALPHA
 * and INTENSITY.
 *
 * Finally, we also need to do this when the actual surface format does
 * not match the logical surface format.  For example, suppose the user
 * requests a GL_LUMINANCE texture but the driver stores it as RGBA.
 * Again, we'll get pixel values like (l,l,l,a).
 */
void
_mesa_rebase_rgba_float(GLuint n, GLfloat rgba[][4], GLenum baseFormat)
{
   GLuint i;

   switch (baseFormat) {
   case GL_ALPHA:
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = 0.0F;
         rgba[i][GCOMP] = 0.0F;
         rgba[i][BCOMP] = 0.0F;
      }
      break;
   case GL_INTENSITY:
      /* fall-through */
   case GL_LUMINANCE:
      for (i = 0; i < n; i++) {
         rgba[i][GCOMP] = 0.0F;
         rgba[i][BCOMP] = 0.0F;
         rgba[i][ACOMP] = 1.0F;
      }
      break;
   case GL_LUMINANCE_ALPHA:
      for (i = 0; i < n; i++) {
         rgba[i][GCOMP] = 0.0F;
         rgba[i][BCOMP] = 0.0F;
      }
      break;
   default:
      /* no-op */
      ;
   }
}


/**
 * As above, but GLuint components.
 */
void
_mesa_rebase_rgba_uint(GLuint n, GLuint rgba[][4], GLenum baseFormat)
{
   GLuint i;

   switch (baseFormat) {
   case GL_ALPHA:
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = 0;
         rgba[i][GCOMP] = 0;
         rgba[i][BCOMP] = 0;
      }
      break;
   case GL_INTENSITY:
      /* fall-through */
   case GL_LUMINANCE:
      for (i = 0; i < n; i++) {
         rgba[i][GCOMP] = 0;
         rgba[i][BCOMP] = 0;
         rgba[i][ACOMP] = 1;
      }
      break;
   case GL_LUMINANCE_ALPHA:
      for (i = 0; i < n; i++) {
         rgba[i][GCOMP] = 0;
         rgba[i][BCOMP] = 0;
      }
      break;
   default:
      /* no-op */
      ;
   }
}


