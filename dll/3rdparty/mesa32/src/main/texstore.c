/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 */

/*
 * Authors:
 *   Brian Paul
 */

/**
 * The GL texture image functions in teximage.c basically just do
 * error checking and data structure allocation.  They in turn call
 * device driver functions which actually copy/convert/store the user's
 * texture image data.
 *
 * However, most device drivers will be able to use the fallback functions
 * in this file.  That is, most drivers will have the following bit of
 * code:
 *   ctx->Driver.TexImage1D = _mesa_store_teximage1d;
 *   ctx->Driver.TexImage2D = _mesa_store_teximage2d;
 *   ctx->Driver.TexImage3D = _mesa_store_teximage3d;
 *   etc...
 *
 * Texture image processing is actually kind of complicated.  We have to do:
 *    Format/type conversions
 *    pixel unpacking
 *    pixel transfer (scale, bais, lookup, convolution!, etc)
 *
 * These functions can handle most everything, including processing full
 * images and sub-images.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "context.h"
#if FEATURE_convolve
#include "convolve.h"
#endif
#include "image.h"
#include "macros.h"
#include "mipmap.h"
#include "imports.h"
#include "texcompress.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "enums.h"


enum {
   ZERO = 4, 
   ONE = 5
};


/**
 * Return GL_TRUE if the given image format is one that be converted
 * to another format by swizzling.
 */
static GLboolean
can_swizzle(GLenum logicalBaseFormat)
{
   switch (logicalBaseFormat) {
   case GL_RGBA:
   case GL_RGB:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_BGR:
   case GL_BGRA:
   case GL_ABGR_EXT:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}



enum {
   IDX_LUMINANCE = 0,
   IDX_ALPHA,
   IDX_INTENSITY,
   IDX_LUMINANCE_ALPHA,
   IDX_RGB,
   IDX_RGBA,
   IDX_RED,
   IDX_GREEN,
   IDX_BLUE,
   IDX_BGR,
   IDX_BGRA,
   IDX_ABGR,
   MAX_IDX
};

#define MAP1(x)       MAP4(x, ZERO, ZERO, ZERO)
#define MAP2(x,y)     MAP4(x, y, ZERO, ZERO)
#define MAP3(x,y,z)   MAP4(x, y, z, ZERO)
#define MAP4(x,y,z,w) { x, y, z, w, ZERO, ONE }


static const struct {
   GLubyte format_idx;
   GLubyte to_rgba[6];
   GLubyte from_rgba[6];
} mappings[MAX_IDX] = 
{
   {
      IDX_LUMINANCE,
      MAP4(0,0,0,ONE),
      MAP1(0)
   },

   {
      IDX_ALPHA,
      MAP4(ZERO, ZERO, ZERO, 0),
      MAP1(3)
   },

   {
      IDX_INTENSITY,
      MAP4(0, 0, 0, 0),
      MAP1(0),
   },

   {
      IDX_LUMINANCE_ALPHA,
      MAP4(0,0,0,1),
      MAP2(0,3)
   },

   {
      IDX_RGB,
      MAP4(0,1,2,ONE),
      MAP3(0,1,2)
   },

   {
      IDX_RGBA,
      MAP4(0,1,2,3),
      MAP4(0,1,2,3),
   },


   {
      IDX_RED,
      MAP4(0, ZERO, ZERO, ONE),
      MAP1(0),
   },

   {
      IDX_GREEN,
      MAP4(ZERO, 0, ZERO, ONE),
      MAP1(1),
   },

   {
      IDX_BLUE,
      MAP4(ZERO, ZERO, 0, ONE),
      MAP1(2),
   },

   {
      IDX_BGR,
      MAP4(2,1,0,ONE),
      MAP3(2,1,0)
   },

   {
      IDX_BGRA,
      MAP4(2,1,0,3),
      MAP4(2,1,0,3)
   },

   {
      IDX_ABGR,
      MAP4(3,2,1,0),
      MAP4(3,2,1,0)
   },
};



/**
 * Convert a GL image format enum to an IDX_* value (see above).
 */
static int
get_map_idx(GLenum value)
{
   switch (value) {
   case GL_LUMINANCE: return IDX_LUMINANCE;
   case GL_ALPHA: return IDX_ALPHA;
   case GL_INTENSITY: return IDX_INTENSITY;
   case GL_LUMINANCE_ALPHA: return IDX_LUMINANCE_ALPHA;
   case GL_RGB: return IDX_RGB;
   case GL_RGBA: return IDX_RGBA;
   case GL_RED: return IDX_RED;
   case GL_GREEN: return IDX_GREEN;
   case GL_BLUE: return IDX_BLUE;
   case GL_BGR: return IDX_BGR;
   case GL_BGRA: return IDX_BGRA;
   case GL_ABGR_EXT: return IDX_ABGR;
   default:
      _mesa_problem(NULL, "Unexpected inFormat");
      return 0;
   }
}   


/**
 * When promoting texture formats (see below) we need to compute the
 * mapping of dest components back to source components.
 * This function does that.
 * \param inFormat  the incoming format of the texture
 * \param outFormat  the final texture format
 * \return map[6]  a full 6-component map
 */
static void
compute_component_mapping(GLenum inFormat, GLenum outFormat, 
			  GLubyte *map)
{
   const int inFmt = get_map_idx(inFormat);
   const int outFmt = get_map_idx(outFormat);
   const GLubyte *in2rgba = mappings[inFmt].to_rgba;
   const GLubyte *rgba2out = mappings[outFmt].from_rgba;
   int i;
   
   for (i = 0; i < 4; i++)
      map[i] = in2rgba[rgba2out[i]];

   map[ZERO] = ZERO;
   map[ONE] = ONE;   

/*
   _mesa_printf("from %x/%s to %x/%s map %d %d %d %d %d %d\n",
		inFormat, _mesa_lookup_enum_by_nr(inFormat),
		outFormat, _mesa_lookup_enum_by_nr(outFormat),
		map[0], 
		map[1], 
		map[2], 
		map[3], 
		map[4], 
		map[5]); 
*/
}


#if !FEATURE_convolve
static void
_mesa_adjust_image_for_convolution(GLcontext *ctx, GLuint dims,
                                   GLsizei *srcWidth, GLsizei *srcHeight)
{
   /* no-op */
}
#endif


/**
 * Make a temporary (color) texture image with GLfloat components.
 * Apply all needed pixel unpacking and pixel transfer operations.
 * Note that there are both logicalBaseFormat and textureBaseFormat parameters.
 * Suppose the user specifies GL_LUMINANCE as the internal texture format
 * but the graphics hardware doesn't support luminance textures.  So, might
 * use an RGB hardware format instead.
 * If logicalBaseFormat != textureBaseFormat we have some extra work to do.
 *
 * \param ctx  the rendering context
 * \param dims  image dimensions: 1, 2 or 3
 * \param logicalBaseFormat  basic texture derived from the user's
 *    internal texture format value
 * \param textureBaseFormat  the actual basic format of the texture
 * \param srcWidth  source image width
 * \param srcHeight  source image height
 * \param srcDepth  source image depth
 * \param srcFormat  source image format
 * \param srcType  source image type
 * \param srcAddr  source image address
 * \param srcPacking  source image pixel packing
 * \return resulting image with format = textureBaseFormat and type = GLfloat.
 */
static GLfloat *
make_temp_float_image(GLcontext *ctx, GLuint dims,
                      GLenum logicalBaseFormat,
                      GLenum textureBaseFormat,
                      GLint srcWidth, GLint srcHeight, GLint srcDepth,
                      GLenum srcFormat, GLenum srcType,
                      const GLvoid *srcAddr,
                      const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint transferOps = ctx->_ImageTransferState;
   GLfloat *tempImage;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY ||
          logicalBaseFormat == GL_COLOR_INDEX ||
          logicalBaseFormat == GL_DEPTH_COMPONENT);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY ||
          textureBaseFormat == GL_COLOR_INDEX ||
          textureBaseFormat == GL_DEPTH_COMPONENT);

   /* conventional color image */

   if ((dims == 1 && ctx->Pixel.Convolution1DEnabled) ||
       (dims >= 2 && ctx->Pixel.Convolution2DEnabled) ||
       (dims >= 2 && ctx->Pixel.Separable2DEnabled)) {
      /* need image convolution */
      const GLuint preConvTransferOps
         = (transferOps & IMAGE_PRE_CONVOLUTION_BITS) | IMAGE_CLAMP_BIT;
      const GLuint postConvTransferOps
         = (transferOps & IMAGE_POST_CONVOLUTION_BITS) | IMAGE_CLAMP_BIT;
      GLint img, row;
      GLint convWidth, convHeight;
      GLfloat *convImage;

      /* pre-convolution image buffer (3D) */
      tempImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                           * 4 * sizeof(GLfloat));
      if (!tempImage)
         return NULL;

      /* post-convolution image buffer (2D) */
      convImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight
                                           * 4 * sizeof(GLfloat));
      if (!convImage) {
         _mesa_free(tempImage);
         return NULL;
      }

      /* loop over 3D image slices */
      for (img = 0; img < srcDepth; img++) {
         GLfloat *dst = tempImage + img * (srcWidth * srcHeight * 4);

         /* unpack and do transfer ops up to convolution */
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                                              srcAddr, srcWidth, srcHeight,
                                              srcFormat, srcType, img, row, 0);
            _mesa_unpack_color_span_float(ctx, srcWidth, GL_RGBA, dst,
                                          srcFormat, srcType, src,
                                          srcPacking,
                                          preConvTransferOps);
            dst += srcWidth * 4;
         }

         /* size after optional convolution */
         convWidth = srcWidth;
         convHeight = srcHeight;

#if FEATURE_convolve
         /* do convolution */
         {
            GLfloat *src = tempImage + img * (srcWidth * srcHeight * 4);
            if (dims == 1) {
               ASSERT(ctx->Pixel.Convolution1DEnabled);
               _mesa_convolve_1d_image(ctx, &convWidth, src, convImage);
            }
            else {
               if (ctx->Pixel.Convolution2DEnabled) {
                  _mesa_convolve_2d_image(ctx, &convWidth, &convHeight,
                                          src, convImage);
               }
               else {
                  ASSERT(ctx->Pixel.Separable2DEnabled);
                  _mesa_convolve_sep_image(ctx, &convWidth, &convHeight,
                                           src, convImage);
               }
            }
         }
#endif
         /* do post-convolution transfer and pack into tempImage */
         {
            const GLint logComponents
               = _mesa_components_in_format(logicalBaseFormat);
            const GLfloat *src = convImage;
            GLfloat *dst = tempImage + img * (convWidth * convHeight * 4);
            for (row = 0; row < convHeight; row++) {
               _mesa_pack_rgba_span_float(ctx, convWidth,
                                          (GLfloat (*)[4]) src,
                                          logicalBaseFormat, GL_FLOAT,
                                          dst, &ctx->DefaultPacking,
                                          postConvTransferOps);
               src += convWidth * 4;
               dst += convWidth * logComponents;
            }
         }
      } /* loop over 3D image slices */

      _mesa_free(convImage);

      /* might need these below */
      srcWidth = convWidth;
      srcHeight = convHeight;
   }
   else {
      /* no convolution */
      const GLint components = _mesa_components_in_format(logicalBaseFormat);
      const GLint srcStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
      GLfloat *dst;
      GLint img, row;

      tempImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                           * components * sizeof(GLfloat));
      if (!tempImage)
         return NULL;

      dst = tempImage;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *src
            = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                                    srcWidth, srcHeight,
                                                    srcFormat, srcType,
                                                    img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            _mesa_unpack_color_span_float(ctx, srcWidth, logicalBaseFormat,
                                          dst, srcFormat, srcType, src,
                                          srcPacking, transferOps);
            dst += srcWidth * components;
            src += srcStride;
         }
      }
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* more work */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLfloat *newImage;
      GLint i, n;
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLfloat *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                          * texComponents * sizeof(GLfloat));
      if (!newImage) {
         _mesa_free(tempImage);
         return NULL;
      }

      compute_component_mapping(logicalBaseFormat, textureBaseFormat, map);

      n = srcWidth * srcHeight * srcDepth;
      for (i = 0; i < n; i++) {
         GLint k;
         for (k = 0; k < texComponents; k++) {
            GLint j = map[k];
            if (j == ZERO)
               newImage[i * texComponents + k] = 0.0F;
            else if (j == ONE)
               newImage[i * texComponents + k] = 1.0F;
            else
               newImage[i * texComponents + k] = tempImage[i * logComponents + j];
         }
      }

      _mesa_free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}


/**
 * Make a temporary (color) texture image with GLchan components.
 * Apply all needed pixel unpacking and pixel transfer operations.
 * Note that there are both logicalBaseFormat and textureBaseFormat parameters.
 * Suppose the user specifies GL_LUMINANCE as the internal texture format
 * but the graphics hardware doesn't support luminance textures.  So, might
 * use an RGB hardware format instead.
 * If logicalBaseFormat != textureBaseFormat we have some extra work to do.
 *
 * \param ctx  the rendering context
 * \param dims  image dimensions: 1, 2 or 3
 * \param logicalBaseFormat  basic texture derived from the user's
 *    internal texture format value
 * \param textureBaseFormat  the actual basic format of the texture
 * \param srcWidth  source image width
 * \param srcHeight  source image height
 * \param srcDepth  source image depth
 * \param srcFormat  source image format
 * \param srcType  source image type
 * \param srcAddr  source image address
 * \param srcPacking  source image pixel packing
 * \return resulting image with format = textureBaseFormat and type = GLchan.
 */
GLchan *
_mesa_make_temp_chan_image(GLcontext *ctx, GLuint dims,
                           GLenum logicalBaseFormat,
                           GLenum textureBaseFormat,
                           GLint srcWidth, GLint srcHeight, GLint srcDepth,
                           GLenum srcFormat, GLenum srcType,
                           const GLvoid *srcAddr,
                           const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint transferOps = ctx->_ImageTransferState;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   GLboolean freeSrcImage = GL_FALSE;
   GLint img, row;
   GLchan *tempImage, *dst;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY);

#if FEATURE_convolve
   if ((dims == 1 && ctx->Pixel.Convolution1DEnabled) ||
       (dims >= 2 && ctx->Pixel.Convolution2DEnabled) ||
       (dims >= 2 && ctx->Pixel.Separable2DEnabled)) {
      /* get convolved image */
      GLfloat *convImage = make_temp_float_image(ctx, dims,
                                                 logicalBaseFormat,
                                                 logicalBaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType,
                                                 srcAddr, srcPacking);
      if (!convImage)
         return NULL;
      /* the convolved image is our new source image */
      srcAddr = convImage;
      srcFormat = logicalBaseFormat;
      srcType = GL_FLOAT;
      srcPacking = &ctx->DefaultPacking;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      transferOps = 0;
      freeSrcImage = GL_TRUE;
   }
#endif

   /* unpack and transfer the source image */
   tempImage = (GLchan *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                       * components * sizeof(GLchan));
   if (!tempImage)
      return NULL;

   dst = tempImage;
   for (img = 0; img < srcDepth; img++) {
      const GLint srcStride = _mesa_image_row_stride(srcPacking,
                                                     srcWidth, srcFormat,
                                                     srcType);
      const GLubyte *src
         = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                                 srcWidth, srcHeight,
                                                 srcFormat, srcType,
                                                 img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
         _mesa_unpack_color_span_chan(ctx, srcWidth, logicalBaseFormat, dst,
                                      srcFormat, srcType, src, srcPacking,
                                      transferOps);
         dst += srcWidth * components;
         src += srcStride;
      }
   }

   /* If we made a temporary image for convolution, free it here */
   if (freeSrcImage) {
      _mesa_free((void *) srcAddr);
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* one more conversion step */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLchan *newImage;
      GLint i, n;
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLchan *) _mesa_malloc(srcWidth * srcHeight * srcDepth
                                         * texComponents * sizeof(GLchan));
      if (!newImage) {
         _mesa_free(tempImage);
         return NULL;
      }

      compute_component_mapping(logicalBaseFormat, textureBaseFormat, map);

      n = srcWidth * srcHeight * srcDepth;
      for (i = 0; i < n; i++) {
         GLint k;
         for (k = 0; k < texComponents; k++) {
            GLint j = map[k];
            if (j == ZERO)
               newImage[i * texComponents + k] = 0;
            else if (j == ONE)
               newImage[i * texComponents + k] = CHAN_MAX;
            else
               newImage[i * texComponents + k] = tempImage[i * logComponents + j];
         }
      }

      _mesa_free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}


/**
 * Copy GLubyte pixels from <src> to <dst> with swizzling.
 * \param dst  destination pixels
 * \param dstComponents  number of color components in destination pixels
 * \param src  source pixels
 * \param srcComponents  number of color components in source pixels
 * \param map  the swizzle mapping.  map[X] says where to find the X component
 *             in the source image's pixels.  For example, if the source image
 *             is GL_BGRA and X = red, map[0] yields 2.
 * \param count  number of pixels to copy/swizzle.
 */
static void
swizzle_copy(GLubyte *dst, GLuint dstComponents, const GLubyte *src, 
             GLuint srcComponents, const GLubyte *map, GLuint count)
{
#define SWZ_CPY(dst, src, count, dstComps, srcComps) \
   do {                                              \
      GLuint i;                                      \
      for (i = 0; i < count; i++) {                  \
         GLuint j;                                   \
         if (srcComps == 4) {                        \
            COPY_4UBV(tmp, src);                     \
         }                                           \
         else {                                      \
            for (j = 0; j < srcComps; j++) {         \
               tmp[j] = src[j];                      \
            }                                        \
         }                                           \
         src += srcComps;                            \
         for (j = 0; j < dstComps; j++) {            \
            dst[j] = tmp[map[j]];                    \
         }                                           \
         dst += dstComps;                            \
      }                                              \
   } while (0)

   GLubyte tmp[6];

   tmp[ZERO] = 0x0;
   tmp[ONE] = 0xff;

   ASSERT(srcComponents <= 4);
   ASSERT(dstComponents <= 4);

   switch (dstComponents) {
   case 4:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 4, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 4, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 4, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 4, 1);
         break;
      default:
         ;
      }
      break;
   case 3:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 3, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 3, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 3, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 3, 1);
         break;
      default:
         ;
      }
      break;
   case 2:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 2, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 2, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 2, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 2, 1);
         break;
      default:
         ;
      }
      break;
   case 1:
      switch (srcComponents) {
      case 4:
         SWZ_CPY(dst, src, count, 1, 4);
         break;
      case 3:
         SWZ_CPY(dst, src, count, 1, 3);
         break;
      case 2:
         SWZ_CPY(dst, src, count, 1, 2);
         break;
      case 1:
         SWZ_CPY(dst, src, count, 1, 1);
         break;
      default:
         ;
      }
      break;
   default:
      ;
   }
#undef SWZ_CPY
}



static const GLubyte map_identity[6] = { 0, 1, 2, 3, ZERO, ONE };
static const GLubyte map_3210[6] = { 3, 2, 1, 0, ZERO, ONE };

/* Deal with the _REV input types:
 */
static const GLubyte *
type_mapping( GLenum srcType )
{
   switch (srcType) {
   case GL_UNSIGNED_BYTE:
      return map_identity;
   case GL_UNSIGNED_INT_8_8_8_8:
      return _mesa_little_endian() ? map_3210 : map_identity;
   case GL_UNSIGNED_INT_8_8_8_8_REV:
      return _mesa_little_endian() ? map_identity : map_3210;
   default:
      return NULL;
   }
}

/* Mapping required if input type is 
 */
static const GLubyte *
byteswap_mapping( GLboolean swapBytes,
		  GLenum srcType )
{
   if (!swapBytes) 
      return map_identity;

   switch (srcType) {
   case GL_UNSIGNED_BYTE:
      return map_identity;
   case GL_UNSIGNED_INT_8_8_8_8:
   case GL_UNSIGNED_INT_8_8_8_8_REV:
      return map_3210;
   default:
      return NULL;
   }
}



/**
 * Transfer a GLubyte texture image with component swizzling.
 */
static void
_mesa_swizzle_ubyte_image(GLcontext *ctx, 
			  GLuint dimensions,
			  GLenum srcFormat,
			  GLenum srcType,

			  GLenum baseInternalFormat,

			  const GLubyte *rgba2dst,
			  GLuint dstComponents,

			  GLvoid *dstAddr,
			  GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
			  GLint dstRowStride,
                          const GLuint *dstImageOffsets,

			  GLint srcWidth, GLint srcHeight, GLint srcDepth,
			  const GLvoid *srcAddr,
			  const struct gl_pixelstore_attrib *srcPacking )
{
   GLint srcComponents = _mesa_components_in_format(srcFormat);
   const GLubyte *srctype2ubyte, *swap;
   GLubyte map[4], src2base[6], base2rgba[6];
   GLint i;
   const GLint srcRowStride =
      _mesa_image_row_stride(srcPacking, srcWidth,
                             srcFormat, GL_UNSIGNED_BYTE);
   const GLint srcImageStride
      = _mesa_image_image_stride(srcPacking, srcWidth, srcHeight, srcFormat,
                                 GL_UNSIGNED_BYTE);
   const GLubyte *srcImage
      = (const GLubyte *) _mesa_image_address(dimensions, srcPacking, srcAddr,
                                              srcWidth, srcHeight, srcFormat,
                                              GL_UNSIGNED_BYTE, 0, 0, 0);

   (void) ctx;

   /* Translate from src->baseInternal->GL_RGBA->dst.  This will
    * correctly deal with RGBA->RGB->RGBA conversions where the final
    * A value must be 0xff regardless of the incoming alpha values.
    */
   compute_component_mapping(srcFormat, baseInternalFormat, src2base);
   compute_component_mapping(baseInternalFormat, GL_RGBA, base2rgba);
   swap = byteswap_mapping(srcPacking->SwapBytes, srcType);
   srctype2ubyte = type_mapping(srcType);


   for (i = 0; i < 4; i++)
      map[i] = srctype2ubyte[swap[src2base[base2rgba[rgba2dst[i]]]]];

/*    _mesa_printf("map %d %d %d %d\n", map[0], map[1], map[2], map[3]);  */

   if (srcRowStride == dstRowStride &&
       srcComponents == dstComponents &&
       srcRowStride == srcWidth * srcComponents &&
       dimensions < 3) {
      /* 1 and 2D images only */
      GLubyte *dstImage = (GLubyte *) dstAddr
         + dstYoffset * dstRowStride
         + dstXoffset * dstComponents;
      swizzle_copy(dstImage, dstComponents, srcImage, srcComponents, map, 
		   srcWidth * srcHeight);
   }
   else {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstComponents
            + dstYoffset * dstRowStride
            + dstXoffset * dstComponents;
         for (row = 0; row < srcHeight; row++) {
	    swizzle_copy(dstRow, dstComponents, srcRow, srcComponents, map, srcWidth);
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         srcImage += srcImageStride;
      }
   }
}


/**
 * Teximage storage routine for when a simple memcpy will do.
 * No pixel transfer operations or special texel encodings allowed.
 * 1D, 2D and 3D images supported.
 */
static void
memcpy_texture(GLcontext *ctx,
	       GLuint dimensions,
               const struct gl_texture_format *dstFormat,
               GLvoid *dstAddr,
               GLint dstXoffset, GLint dstYoffset, GLint dstZoffset,
               GLint dstRowStride,
               const GLuint *dstImageOffsets,
               GLint srcWidth, GLint srcHeight, GLint srcDepth,
               GLenum srcFormat, GLenum srcType,
               const GLvoid *srcAddr,
               const struct gl_pixelstore_attrib *srcPacking)
{
   const GLint srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                     srcFormat, srcType);
   const GLint srcImageStride = _mesa_image_image_stride(srcPacking,
                                      srcWidth, srcHeight, srcFormat, srcType);
   const GLubyte *srcImage = (const GLubyte *) _mesa_image_address(dimensions,
        srcPacking, srcAddr, srcWidth, srcHeight, srcFormat, srcType, 0, 0, 0);
   const GLint bytesPerRow = srcWidth * dstFormat->TexelBytes;

#if 0
   /* XXX update/re-enable for dstImageOffsets array */
   const GLint bytesPerImage = srcHeight * bytesPerRow;
   const GLint bytesPerTexture = srcDepth * bytesPerImage;
   GLubyte *dstImage = (GLubyte *) dstAddr
                     + dstZoffset * dstImageStride
                     + dstYoffset * dstRowStride
                     + dstXoffset * dstFormat->TexelBytes;

   if (dstRowStride == srcRowStride &&
       dstRowStride == bytesPerRow &&
       ((dstImageStride == srcImageStride &&
         dstImageStride == bytesPerImage) ||
        (srcDepth == 1))) {
      /* one big memcpy */
      ctx->Driver.TextureMemCpy(dstImage, srcImage, bytesPerTexture);
   }
   else
   {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            ctx->Driver.TextureMemCpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         srcImage += srcImageStride;
         dstImage += dstImageStride;
      }
   }
#endif

   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      const GLubyte *srcRow = srcImage;
      GLubyte *dstRow = (GLubyte *) dstAddr
         + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
         + dstYoffset * dstRowStride
         + dstXoffset * dstFormat->TexelBytes;
      for (row = 0; row < srcHeight; row++) {
         ctx->Driver.TextureMemCpy(dstRow, srcRow, bytesPerRow);
         dstRow += dstRowStride;
         srcRow += srcRowStride;
      }
      srcImage += srcImageStride;
   }
}



/**
 * Store an image in any of the formats:
 *   _mesa_texformat_rgba
 *   _mesa_texformat_rgb
 *   _mesa_texformat_alpha
 *   _mesa_texformat_luminance
 *   _mesa_texformat_luminance_alpha
 *   _mesa_texformat_intensity
 * 
 */
GLboolean
_mesa_texstore_rgba(TEXSTORE_PARAMS)
{
   const GLint components = _mesa_components_in_format(baseInternalFormat);

   ASSERT(dstFormat == &_mesa_texformat_rgba ||
          dstFormat == &_mesa_texformat_rgb ||
          dstFormat == &_mesa_texformat_alpha ||
          dstFormat == &_mesa_texformat_luminance ||
          dstFormat == &_mesa_texformat_luminance_alpha ||
          dstFormat == &_mesa_texformat_intensity);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(dstFormat->TexelBytes == components * sizeof(GLchan));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == CHAN_TYPE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            dstFormat == &_mesa_texformat_rgb &&
            srcFormat == GL_RGBA &&
            srcType == CHAN_TYPE) {
      /* extract RGB from RGBA */
      GLint img, row, col;
      for (img = 0; img < srcDepth; img++) {
         GLchan *dstImage = (GLchan *)
            ((GLubyte *) dstAddr
             + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
             + dstYoffset * dstRowStride
             + dstXoffset * dstFormat->TexelBytes);

         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
         GLchan *srcRow = (GLchan *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLchan *dstRow = dstImage;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + RCOMP] = srcRow[col * 4 + RCOMP];
               dstRow[col * 3 + GCOMP] = srcRow[col * 4 + GCOMP];
               dstRow[col * 3 + BCOMP] = srcRow[col * 4 + BCOMP];
            }
            dstRow += dstRowStride / sizeof(GLchan);
            srcRow = (GLchan *) ((GLubyte *) srcRow + srcRowStride);
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
	    CHAN_TYPE == GL_UNSIGNED_BYTE &&
	    (srcType == GL_UNSIGNED_BYTE ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8 ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8_REV) &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      const GLubyte *dstmap;
      GLuint components;

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if (dstFormat == &_mesa_texformat_rgba) {
	 dstmap = mappings[IDX_RGBA].from_rgba;
	 components = 4;
      }
      else if (dstFormat == &_mesa_texformat_rgb) {
	 dstmap = mappings[IDX_RGB].from_rgba;
	 components = 3;
      }
      else if (dstFormat == &_mesa_texformat_alpha) {
	 dstmap = mappings[IDX_ALPHA].from_rgba;
	 components = 1;
      }
      else if (dstFormat == &_mesa_texformat_luminance) {
	 dstmap = mappings[IDX_LUMINANCE].from_rgba;
	 components = 1;
      }
      else if (dstFormat == &_mesa_texformat_luminance_alpha) {
	 dstmap = mappings[IDX_LUMINANCE_ALPHA].from_rgba;
	 components = 2;
      }
      else if (dstFormat == &_mesa_texformat_intensity) {
	 dstmap = mappings[IDX_INTENSITY].from_rgba;
	 components = 1;
      }
      else {
         _mesa_problem(ctx, "Unexpected dstFormat in _mesa_texstore_rgba");
         return GL_FALSE;
      }

      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, components,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint bytesPerRow;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      bytesPerRow = srcWidth * components * sizeof(GLchan);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            _mesa_memcpy(dstRow, src, bytesPerRow);
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a 32-bit integer depth component texture image.
 */
GLboolean
_mesa_texstore_z32(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffffff;
   (void) dims;
   ASSERT(dstFormat == &_mesa_texformat_z32);
   ASSERT(dstFormat->TexelBytes == sizeof(GLuint));

   if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == GL_UNSIGNED_INT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, (GLuint *) dstRow,
                                    depthScale, srcType, src, srcPacking);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}

#define STRIDE_3D 0

/**
 * Store a 16-bit integer depth component texture image.
 */
GLboolean
_mesa_texstore_z16(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffff;
   (void) dims;
   ASSERT(dstFormat == &_mesa_texformat_z16);
   ASSERT(dstFormat->TexelBytes == sizeof(GLushort));

   if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == GL_UNSIGNED_SHORT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            GLushort *dst16 = (GLushort *) dstRow;
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_SHORT, dst16, depthScale,
                                    srcType, src, srcPacking);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store an rgb565 or rgb565_rev texture image.
 */
GLboolean
_mesa_texstore_rgb565(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_rgb565 ||
          dstFormat == &_mesa_texformat_rgb565_rev);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_rgb565 &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_SHORT_5_6_5) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            baseInternalFormat == GL_RGB &&
            srcFormat == GL_RGB &&
            srcType == GL_UNSIGNED_BYTE &&
            dims == 2) {
      /* do optimized tex store */
      const GLint srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                        srcFormat, srcType);
      const GLubyte *src = (const GLubyte *)
         _mesa_image_address(dims, srcPacking, srcAddr, srcWidth, srcHeight,
                             srcFormat, srcType, 0, 0, 0);
      GLubyte *dst = (GLubyte *) dstAddr
                   + dstYoffset * dstRowStride
                   + dstXoffset * dstFormat->TexelBytes;
      GLint row, col;
      for (row = 0; row < srcHeight; row++) {
         const GLubyte *srcUB = (const GLubyte *) src;
         GLushort *dstUS = (GLushort *) dst;
         /* check for byteswapped format */
         if (dstFormat == &_mesa_texformat_rgb565) {
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_565( srcUB[0], srcUB[1], srcUB[2] );
               srcUB += 3;
            }
         }
         else {
            for (col = 0; col < srcWidth; col++) {
               dstUS[col] = PACK_COLOR_565_REV( srcUB[0], srcUB[1], srcUB[2] );
               srcUB += 3;
            }
         }
         dst += dstRowStride;
         src += srcRowStride;
      }
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            /* check for byteswapped format */
            if (dstFormat == &_mesa_texformat_rgb565) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_565( CHAN_TO_UBYTE(src[RCOMP]),
                                               CHAN_TO_UBYTE(src[GCOMP]),
                                               CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 3;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_565_REV( CHAN_TO_UBYTE(src[RCOMP]),
                                                   CHAN_TO_UBYTE(src[GCOMP]),
                                                   CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 3;
               }
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a texture in MESA_FORMAT_RGBA8888 or MESA_FORMAT_RGBA8888_REV.
 */
GLboolean
_mesa_texstore_rgba8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();

   ASSERT(dstFormat == &_mesa_texformat_rgba8888 ||
          dstFormat == &_mesa_texformat_rgba8888_rev);
   ASSERT(dstFormat->TexelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_rgba8888 &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian))) {
       /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_rgba8888_rev &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
	    (srcType == GL_UNSIGNED_BYTE ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8 ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8_REV) &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if ((littleEndian && dstFormat == &_mesa_texformat_rgba8888) ||
	  (!littleEndian && dstFormat == &_mesa_texformat_rgba8888_rev)) {
	 dstmap[3] = 0;
	 dstmap[2] = 1;
	 dstmap[1] = 2;
	 dstmap[0] = 3;
      }
      else {
	 dstmap[3] = 3;
	 dstmap[2] = 2;
	 dstmap[1] = 1;
	 dstmap[0] = 0;
      }
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 4,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == &_mesa_texformat_rgba8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]),
                                                CHAN_TO_UBYTE(src[ACOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888_REV( CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]),
                                                    CHAN_TO_UBYTE(src[ACOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_argb8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();

   ASSERT(dstFormat == &_mesa_texformat_argb8888 ||
          dstFormat == &_mesa_texformat_argb8888_rev);
   ASSERT(dstFormat->TexelBytes == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_argb8888 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
        srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) {
      /* simple memcpy path (little endian) */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_argb8888_rev &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
        srcType == GL_UNSIGNED_INT_8_8_8_8)) {
      /* simple memcpy path (big endian) */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
	    dstFormat == &_mesa_texformat_argb8888 &&
            srcFormat == GL_RGB &&
	    (baseInternalFormat == GL_RGBA ||
	     baseInternalFormat == GL_RGB) &&
            srcType == GL_UNSIGNED_BYTE) {
      int img, row, col;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *d4 = (GLuint *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               d4[col] = PACK_COLOR_8888(0xff,
                                         srcRow[col * 3 + RCOMP],
                                         srcRow[col * 3 + GCOMP],
                                         srcRow[col * 3 + BCOMP]);
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
	    dstFormat == &_mesa_texformat_argb8888 &&
            srcFormat == GL_RGBA &&
	    baseInternalFormat == GL_RGBA &&
            srcType == GL_UNSIGNED_BYTE) {
      /* same as above case, but src data has alpha too */
      GLint img, row, col;
      /* For some reason, streaming copies to write-combined regions
       * are extremely sensitive to the characteristics of how the
       * source data is retrieved.  By reordering the source reads to
       * be in-order, the speed of this operation increases by half.
       * Strangely the same isn't required for the RGB path, above.
       */
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *d4 = (GLuint *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               d4[col] = PACK_COLOR_8888(srcRow[col * 4 + ACOMP],
                                         srcRow[col * 4 + RCOMP],
                                         srcRow[col * 4 + GCOMP],
                                         srcRow[col * 4 + BCOMP]);
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
	    (srcType == GL_UNSIGNED_BYTE ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8 ||
	     srcType == GL_UNSIGNED_INT_8_8_8_8_REV) &&
	    can_swizzle(baseInternalFormat) &&	   
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if ((littleEndian && dstFormat == &_mesa_texformat_argb8888) ||
	  (!littleEndian && dstFormat == &_mesa_texformat_argb8888_rev)) {
	 dstmap[3] = 3;		/* alpha */
	 dstmap[2] = 0;		/* red */
	 dstmap[1] = 1;		/* green */
	 dstmap[0] = 2;		/* blue */
      }
      else {
	 assert((littleEndian && dstFormat == &_mesa_texformat_argb8888_rev) ||
		(!littleEndian && dstFormat == &_mesa_texformat_argb8888));
	 dstmap[3] = 2;
	 dstmap[2] = 1;
	 dstmap[1] = 0;
	 dstmap[0] = 3;
      }
 
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,

				baseInternalFormat,
				dstmap, 4,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride,
                                dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == &_mesa_texformat_argb8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( CHAN_TO_UBYTE(src[ACOMP]),
                                                CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888_REV( CHAN_TO_UBYTE(src[ACOMP]),
                                                    CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_rgb888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();

   ASSERT(dstFormat == &_mesa_texformat_rgb888);
   ASSERT(dstFormat->TexelBytes == 3);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_BGR &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            srcFormat == GL_RGBA &&
            srcType == GL_UNSIGNED_BYTE) {
      /* extract RGB from RGBA */
      GLint img, row, col;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = srcRow[col * 4 + BCOMP];
               dstRow[col * 3 + 1] = srcRow[col * 4 + GCOMP];
               dstRow[col * 3 + 2] = srcRow[col * 4 + RCOMP];
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      dstmap[0] = 2;
      dstmap[1] = 1;
      dstmap[2] = 0;
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 3,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = (const GLchan *) tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
#if 0
            if (littleEndian) {
               for (col = 0; col < srcWidth; col++) {
                  dstRow[col * 3 + 0] = CHAN_TO_UBYTE(src[RCOMP]);
                  dstRow[col * 3 + 1] = CHAN_TO_UBYTE(src[GCOMP]);
                  dstRow[col * 3 + 2] = CHAN_TO_UBYTE(src[BCOMP]);
                  srcUB += 3;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstRow[col * 3 + 0] = srcUB[BCOMP];
                  dstRow[col * 3 + 1] = srcUB[GCOMP];
                  dstRow[col * 3 + 2] = srcUB[RCOMP];
                  srcUB += 3;
               }
            }
#else
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = CHAN_TO_UBYTE(src[BCOMP]);
               dstRow[col * 3 + 1] = CHAN_TO_UBYTE(src[GCOMP]);
               dstRow[col * 3 + 2] = CHAN_TO_UBYTE(src[RCOMP]);
               src += 3;
            }
#endif
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_bgr888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();

   ASSERT(dstFormat == &_mesa_texformat_bgr888);
   ASSERT(dstFormat->TexelBytes == 3);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
            srcFormat == GL_RGBA &&
            srcType == GL_UNSIGNED_BYTE) {
      /* extract BGR from RGBA */
      int img, row, col;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride = _mesa_image_row_stride(srcPacking,
                                                 srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = srcRow[col * 4 + RCOMP];
               dstRow[col * 3 + 1] = srcRow[col * 4 + GCOMP];
               dstRow[col * 3 + 2] = srcRow[col * 4 + BCOMP];
            }
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
      }
   }
   else if (!ctx->_ImageTransferState &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      dstmap[0] = 0;
      dstmap[1] = 1;
      dstmap[2] = 2;
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 3,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = (const GLchan *) tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = CHAN_TO_UBYTE(src[RCOMP]);
               dstRow[col * 3 + 1] = CHAN_TO_UBYTE(src[GCOMP]);
               dstRow[col * 3 + 2] = CHAN_TO_UBYTE(src[BCOMP]);
               src += 3;
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}

GLboolean
_mesa_texstore_rgba4444(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_rgba4444);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_rgba4444 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_RGBA &&
       srcType == GL_UNSIGNED_SHORT_4_4_4_4){
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
	      dstUS[col] = PACK_COLOR_4444( CHAN_TO_UBYTE(src[RCOMP]),
					    CHAN_TO_UBYTE(src[GCOMP]),
					    CHAN_TO_UBYTE(src[BCOMP]),
					    CHAN_TO_UBYTE(src[ACOMP]) );
	      src += 4;
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}

GLboolean
_mesa_texstore_argb4444(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_argb4444 ||
          dstFormat == &_mesa_texformat_argb4444_rev);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_argb4444 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == &_mesa_texformat_argb4444) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_4444( CHAN_TO_UBYTE(src[ACOMP]),
                                                CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_4444_REV( CHAN_TO_UBYTE(src[ACOMP]),
                                                    CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}

GLboolean
_mesa_texstore_rgba5551(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_rgba5551);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_rgba5551 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_RGBA &&
       srcType == GL_UNSIGNED_SHORT_5_5_5_1) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src =tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
	       dstUS[col] = PACK_COLOR_5551( CHAN_TO_UBYTE(src[RCOMP]),
					     CHAN_TO_UBYTE(src[GCOMP]),
					     CHAN_TO_UBYTE(src[BCOMP]),
					     CHAN_TO_UBYTE(src[ACOMP]) );
	      src += 4;
	    }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}

GLboolean
_mesa_texstore_argb1555(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_argb1555 ||
          dstFormat == &_mesa_texformat_argb1555_rev);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_argb1555 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src =tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == &_mesa_texformat_argb1555) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_1555( CHAN_TO_UBYTE(src[ACOMP]),
                                                CHAN_TO_UBYTE(src[RCOMP]),
                                                CHAN_TO_UBYTE(src[GCOMP]),
                                                CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_1555_REV( CHAN_TO_UBYTE(src[ACOMP]),
                                                    CHAN_TO_UBYTE(src[RCOMP]),
                                                    CHAN_TO_UBYTE(src[GCOMP]),
                                                    CHAN_TO_UBYTE(src[BCOMP]) );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_al88(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();

   ASSERT(dstFormat == &_mesa_texformat_al88 ||
          dstFormat == &_mesa_texformat_al88_rev);
   ASSERT(dstFormat->TexelBytes == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == &_mesa_texformat_al88 &&
       baseInternalFormat == GL_LUMINANCE_ALPHA &&
       srcFormat == GL_LUMINANCE_ALPHA &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
	    littleEndian &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if ((littleEndian && dstFormat == &_mesa_texformat_al88) ||
	  (!littleEndian && dstFormat == &_mesa_texformat_al88_rev)) {
	 dstmap[0] = 0;
	 dstmap[1] = 3;
      }
      else {
	 dstmap[0] = 3;
	 dstmap[1] = 0;
      }
      dstmap[2] = ZERO;		/* ? */
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 2,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == &_mesa_texformat_al88) {
               for (col = 0; col < srcWidth; col++) {
                  /* src[0] is luminance, src[1] is alpha */
                 dstUS[col] = PACK_COLOR_88( CHAN_TO_UBYTE(src[1]),
                                             CHAN_TO_UBYTE(src[0]) );
                 src += 2;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  /* src[0] is luminance, src[1] is alpha */
                 dstUS[col] = PACK_COLOR_88_REV( CHAN_TO_UBYTE(src[1]),
                                                 CHAN_TO_UBYTE(src[0]) );
                 src += 2;
               }
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


GLboolean
_mesa_texstore_rgb332(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_rgb332);
   ASSERT(dstFormat->TexelBytes == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB && srcType == GL_UNSIGNED_BYTE_3_3_2) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = PACK_COLOR_332( CHAN_TO_UBYTE(src[RCOMP]),
                                             CHAN_TO_UBYTE(src[GCOMP]),
                                             CHAN_TO_UBYTE(src[BCOMP]) );
               src += 3;
            }
            dstRow += dstRowStride;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Texstore for _mesa_texformat_a8, _mesa_texformat_l8, _mesa_texformat_i8.
 */
GLboolean
_mesa_texstore_a8(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == &_mesa_texformat_a8 ||
          dstFormat == &_mesa_texformat_l8 ||
          dstFormat == &_mesa_texformat_i8);
   ASSERT(dstFormat->TexelBytes == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
	    srcType == GL_UNSIGNED_BYTE &&
	    can_swizzle(baseInternalFormat) &&
	    can_swizzle(srcFormat)) {

      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if (dstFormat == &_mesa_texformat_a8) {
	 dstmap[0] = 3;
      }
      else {
	 dstmap[0] = 0;
      }
      dstmap[1] = ZERO;		/* ? */
      dstmap[2] = ZERO;		/* ? */
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 1,
				dstAddr, dstXoffset, dstYoffset, dstZoffset,
				dstRowStride, dstImageOffsets,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLchan *tempImage = _mesa_make_temp_chan_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLchan *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = CHAN_TO_UBYTE(src[col]);
            }
            dstRow += dstRowStride;
            src += srcWidth;
         }
      }
      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}



GLboolean
_mesa_texstore_ci8(TEXSTORE_PARAMS)
{
   (void) dims; (void) baseInternalFormat;
   ASSERT(dstFormat == &_mesa_texformat_ci8);
   ASSERT(dstFormat->TexelBytes == 1);
   ASSERT(baseInternalFormat == GL_COLOR_INDEX);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       srcFormat == GL_COLOR_INDEX &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_index_span(ctx, srcWidth, GL_UNSIGNED_BYTE, dstRow,
                                    srcType, src, srcPacking,
                                    ctx->_ImageTransferState);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Texstore for _mesa_texformat_ycbcr or _mesa_texformat_ycbcr_rev.
 */
GLboolean
_mesa_texstore_ycbcr(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   (void) ctx; (void) dims; (void) baseInternalFormat;

   ASSERT((dstFormat == &_mesa_texformat_ycbcr) ||
          (dstFormat == &_mesa_texformat_ycbcr_rev));
   ASSERT(dstFormat->TexelBytes == 2);
   ASSERT(ctx->Extensions.MESA_ycbcr_texture);
   ASSERT(srcFormat == GL_YCBCR_MESA);
   ASSERT((srcType == GL_UNSIGNED_SHORT_8_8_MESA) ||
          (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA));
   ASSERT(baseInternalFormat == GL_YCBCR_MESA);

   /* always just memcpy since no pixel transfer ops apply */
   memcpy_texture(ctx, dims,
                  dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                  dstRowStride,
                  dstImageOffsets,
                  srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                  srcAddr, srcPacking);

   /* Check if we need byte swapping */
   /* XXX the logic here _might_ be wrong */
   if (srcPacking->SwapBytes ^
       (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA) ^
       (dstFormat == &_mesa_texformat_ycbcr_rev) ^
       !littleEndian) {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            _mesa_swap2((GLushort *) dstRow, srcWidth);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}



/**
 * Store a combined depth/stencil texture image.
 */
GLboolean
_mesa_texstore_z24_s8(TEXSTORE_PARAMS)
{
   const GLfloat depthScale = (GLfloat) 0xffffff;

   ASSERT(dstFormat == &_mesa_texformat_z24_s8);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT);
   ASSERT(srcType == GL_UNSIGNED_INT_24_8_EXT);

   if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes) {
      /* simple path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLint srcRowStride
         = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
         / sizeof(GLuint);
      GLint img, row;

      for (img = 0; img < srcDepth; img++) {
         GLuint *dstRow = (GLuint *) dstAddr
            + dstImageOffsets[dstZoffset + img]
            + dstYoffset * dstRowStride / sizeof(GLuint)
            + dstXoffset;
         const GLuint *src
            = (const GLuint *) _mesa_image_address(dims, srcPacking, srcAddr,
                                                   srcWidth, srcHeight,
                                                   srcFormat, srcType,
                                                   img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLubyte stencil[MAX_WIDTH];
            GLint i;
            /* the 24 depth bits will be in the high position: */
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT_24_8_EXT, /* dst type */
                                    dstRow, /* dst addr */
                                    (GLuint) depthScale,
                                    srcType, src, srcPacking);
            /* get the 8-bit stencil values */
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_UNSIGNED_BYTE, /* dst type */
                                      stencil, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);
            /* merge stencil values into depth values */
            for (i = 0; i < srcWidth; i++)
               dstRow[i] |= stencil[i];

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLuint);
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store a combined depth/stencil texture image.
 */
GLboolean
_mesa_texstore_s8_z24(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;
   const GLint srcRowStride
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
      / sizeof(GLuint);
   GLint img, row;

   ASSERT(dstFormat == &_mesa_texformat_s8_z24);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT || srcFormat == GL_DEPTH_COMPONENT);
   ASSERT(srcFormat != GL_DEPTH_STENCIL_EXT || srcType == GL_UNSIGNED_INT_24_8_EXT);

   /* In case we only upload depth we need to preserve the stencil */
   if (srcFormat == GL_DEPTH_COMPONENT) {
      for (img = 0; img < srcDepth; img++) {
         GLuint *dstRow = (GLuint *) dstAddr
            + dstImageOffsets[dstZoffset + img]
            + dstYoffset * dstRowStride / sizeof(GLuint)
            + dstXoffset;
         const GLuint *src
            = (const GLuint *) _mesa_image_address(dims, srcPacking, srcAddr,
                  srcWidth, srcHeight,
                  srcFormat, srcType,
                  img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLuint depth[MAX_WIDTH];
            GLint i;
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, /* dst type */
                                    depth, /* dst addr */
                                    depthScale,
                                    srcType, src, srcPacking);

            for (i = 0; i < srcWidth; i++)
               dstRow[i] = depth[i] | (dstRow[i] & 0xFF000000);

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLuint);
         }
      }
   }
   else {
      for (img = 0; img < srcDepth; img++) {
         GLuint *dstRow = (GLuint *) dstAddr
            + dstImageOffsets[dstZoffset + img]
            + dstYoffset * dstRowStride / sizeof(GLuint)
            + dstXoffset;
         const GLuint *src
            = (const GLuint *) _mesa_image_address(dims, srcPacking, srcAddr,
                  srcWidth, srcHeight,
                  srcFormat, srcType,
                  img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLubyte stencil[MAX_WIDTH];
            GLint i;
            /* the 24 depth bits will be in the low position: */
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, /* dst type */
                                    dstRow, /* dst addr */
                                    depthScale,
                                    srcType, src, srcPacking);
            /* get the 8-bit stencil values */
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_UNSIGNED_BYTE, /* dst type */
                                      stencil, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);
            /* merge stencil values into depth values */
            for (i = 0; i < srcWidth; i++)
               dstRow[i] |= stencil[i] << 24;

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLuint);
         }
      }
   }
   return GL_TRUE;
}

/**
 * Store an image in any of the formats:
 *   _mesa_texformat_rgba_float32
 *   _mesa_texformat_rgb_float32
 *   _mesa_texformat_alpha_float32
 *   _mesa_texformat_luminance_float32
 *   _mesa_texformat_luminance_alpha_float32
 *   _mesa_texformat_intensity_float32
 */
GLboolean
_mesa_texstore_rgba_float32(TEXSTORE_PARAMS)
{
   const GLint components = _mesa_components_in_format(dstFormat->BaseFormat);

   ASSERT(dstFormat == &_mesa_texformat_rgba_float32 ||
          dstFormat == &_mesa_texformat_rgb_float32 ||
          dstFormat == &_mesa_texformat_alpha_float32 ||
          dstFormat == &_mesa_texformat_luminance_float32 ||
          dstFormat == &_mesa_texformat_luminance_alpha_float32 ||
          dstFormat == &_mesa_texformat_intensity_float32);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(dstFormat->TexelBytes == components * sizeof(GLfloat));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_FLOAT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *srcRow = tempImage;
      GLint bytesPerRow;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      bytesPerRow = srcWidth * components * sizeof(GLfloat);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            _mesa_memcpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcWidth * components;
         }
      }

      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * As above, but store 16-bit floats.
 */
GLboolean
_mesa_texstore_rgba_float16(TEXSTORE_PARAMS)
{
   const GLint components = _mesa_components_in_format(dstFormat->BaseFormat);

   ASSERT(dstFormat == &_mesa_texformat_rgba_float16 ||
          dstFormat == &_mesa_texformat_rgb_float16 ||
          dstFormat == &_mesa_texformat_alpha_float16 ||
          dstFormat == &_mesa_texformat_luminance_float16 ||
          dstFormat == &_mesa_texformat_luminance_alpha_float16 ||
          dstFormat == &_mesa_texformat_intensity_float16);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(dstFormat->TexelBytes == components * sizeof(GLhalfARB));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_HALF_FLOAT_ARB) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat, dstAddr, dstXoffset, dstYoffset, dstZoffset,
                     dstRowStride,
                     dstImageOffsets,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 dstFormat->BaseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLfloat *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      _mesa_adjust_image_for_convolution(ctx, dims, &srcWidth, &srcHeight);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = (GLubyte *) dstAddr
            + dstImageOffsets[dstZoffset + img] * dstFormat->TexelBytes
            + dstYoffset * dstRowStride
            + dstXoffset * dstFormat->TexelBytes;
         for (row = 0; row < srcHeight; row++) {
            GLhalfARB *dstTexel = (GLhalfARB *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = _mesa_float_to_half(src[i]);
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      _mesa_free((void *) tempImage);
   }
   return GL_TRUE;
}


#if FEATURE_EXT_texture_sRGB
GLboolean
_mesa_texstore_srgb8(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const struct gl_texture_format *newDstFormat;
   StoreTexImageFunc store;
   GLboolean k;

   ASSERT(dstFormat == &_mesa_texformat_srgb8);

   /* reuse normal rgb texstore code */
   if (littleEndian) {
      newDstFormat = &_mesa_texformat_bgr888;
      store = _mesa_texstore_bgr888;
   }
   else {
      newDstFormat = &_mesa_texformat_rgb888;
      store = _mesa_texstore_rgb888;
   }

   k = store(ctx, dims, baseInternalFormat,
             newDstFormat, dstAddr,
             dstXoffset, dstYoffset, dstZoffset,
             dstRowStride, dstImageOffsets,
             srcWidth, srcHeight, srcDepth,
             srcFormat, srcType,
             srcAddr, srcPacking);
   return k;
}


GLboolean
_mesa_texstore_srgba8(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const struct gl_texture_format *newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == &_mesa_texformat_srgba8);

   /* reuse normal rgba texstore code */
   if (littleEndian)
      newDstFormat = &_mesa_texformat_rgba8888_rev;
   else
      newDstFormat = &_mesa_texformat_rgba8888;

   k = _mesa_texstore_rgba8888(ctx, dims, baseInternalFormat,
                               newDstFormat, dstAddr,
                               dstXoffset, dstYoffset, dstZoffset,
                               dstRowStride, dstImageOffsets,
                               srcWidth, srcHeight, srcDepth,
                               srcFormat, srcType,
                               srcAddr, srcPacking);
   return k;
}


GLboolean
_mesa_texstore_sl8(TEXSTORE_PARAMS)
{
   const struct gl_texture_format *newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == &_mesa_texformat_sl8);

   newDstFormat = &_mesa_texformat_l8;

   /* _mesa_textore_a8 handles luminance8 too */
   k = _mesa_texstore_a8(ctx, dims, baseInternalFormat,
                         newDstFormat, dstAddr,
                         dstXoffset, dstYoffset, dstZoffset,
                         dstRowStride, dstImageOffsets,
                         srcWidth, srcHeight, srcDepth,
                         srcFormat, srcType,
                         srcAddr, srcPacking);
   return k;
}


GLboolean
_mesa_texstore_sla8(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const struct gl_texture_format *newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == &_mesa_texformat_sla8);

   /* reuse normal luminance/alpha texstore code */
   if (littleEndian)
      newDstFormat = &_mesa_texformat_al88;
   else
      newDstFormat = &_mesa_texformat_al88_rev;

   k = _mesa_texstore_al88(ctx, dims, baseInternalFormat,
                           newDstFormat, dstAddr,
                           dstXoffset, dstYoffset, dstZoffset,
                           dstRowStride, dstImageOffsets,
                           srcWidth, srcHeight, srcDepth,
                           srcFormat, srcType,
                           srcAddr, srcPacking);
   return k;
}

#endif /* FEATURE_EXT_texture_sRGB */


/**
 * Check if an unpack PBO is active prior to fetching a texture image.
 * If so, do bounds checking and map the buffer into main memory.
 * Any errors detected will be recorded.
 * The caller _must_ call _mesa_unmap_teximage_pbo() too!
 */
const GLvoid *
_mesa_validate_pbo_teximage(GLcontext *ctx, GLuint dimensions,
			    GLsizei width, GLsizei height, GLsizei depth,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *unpack,
			    const char *funcName)
{
   GLubyte *buf;

   if (unpack->BufferObj->Name == 0) {
      /* no PBO */
      return pixels;
   }
   if (!_mesa_validate_pbo_access(dimensions, unpack, width, height, depth,
                                  format, type, pixels)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(invalid PBO access");
      return NULL;
   }

   buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                          GL_READ_ONLY_ARB, unpack->BufferObj);
   if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(PBO is mapped");
      return NULL;
   }

   return ADD_POINTERS(buf, pixels);
}


/**
 * Check if an unpack PBO is active prior to fetching a compressed texture
 * image.
 * If so, do bounds checking and map the buffer into main memory.
 * Any errors detected will be recorded.
 * The caller _must_ call _mesa_unmap_teximage_pbo() too!
 */
const GLvoid *
_mesa_validate_pbo_compressed_teximage(GLcontext *ctx,
                                 GLsizei imageSize, const GLvoid *pixels,
                                 const struct gl_pixelstore_attrib *packing,
                                 const char *funcName)
{
   GLubyte *buf;

   if (packing->BufferObj->Name == 0) {
      /* not using a PBO - return pointer unchanged */
      return pixels;
   }
   if ((const GLubyte *) pixels + imageSize >
       ((const GLubyte *) 0) + packing->BufferObj->Size) {
      /* out of bounds read! */
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(invalid PBO access");
      return NULL;
   }

   buf = (GLubyte*) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                         GL_READ_ONLY_ARB, packing->BufferObj);
   if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, funcName, "(PBO is mapped");
      return NULL;
   }

   return ADD_POINTERS(buf, pixels);
}


/**
 * This function must be called after either of the validate_pbo_*_teximage()
 * functions.  It unmaps the PBO buffer if it was mapped earlier.
 */
void
_mesa_unmap_teximage_pbo(GLcontext *ctx,
                         const struct gl_pixelstore_attrib *unpack)
{
   if (unpack->BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }
}



/**
 * Adaptor for fetching a GLchan texel from a float-valued texture.
 */
static void
fetch_texel_float_to_chan(const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLchan *texelOut)
{
   GLfloat temp[4];
   ASSERT(texImage->FetchTexelf);
   texImage->FetchTexelf(texImage, i, j, k, temp);
   if (texImage->TexFormat->BaseFormat == GL_DEPTH_COMPONENT ||
       texImage->TexFormat->BaseFormat == GL_DEPTH_STENCIL_EXT) {
      /* just one channel */
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[0], temp[0]);
   }
   else {
      /* four channels */
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[0], temp[0]);
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[1], temp[1]);
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[2], temp[2]);
      UNCLAMPED_FLOAT_TO_CHAN(texelOut[3], temp[3]);
   }
}


/**
 * Adaptor for fetching a float texel from a GLchan-valued texture.
 */
static void
fetch_texel_chan_to_float(const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texelOut)
{
   GLchan temp[4];
   ASSERT(texImage->FetchTexelc);
   texImage->FetchTexelc(texImage, i, j, k, temp);
   if (texImage->TexFormat->BaseFormat == GL_DEPTH_COMPONENT ||
       texImage->TexFormat->BaseFormat == GL_DEPTH_STENCIL_EXT) {
      /* just one channel */
      texelOut[0] = CHAN_TO_FLOAT(temp[0]);
   }
   else {
      /* four channels */
      texelOut[0] = CHAN_TO_FLOAT(temp[0]);
      texelOut[1] = CHAN_TO_FLOAT(temp[1]);
      texelOut[2] = CHAN_TO_FLOAT(temp[2]);
      texelOut[3] = CHAN_TO_FLOAT(temp[3]);
   }
}


/**
 * Initialize the texture image's FetchTexelc and FetchTexelf methods.
 */
void
_mesa_set_fetch_functions(struct gl_texture_image *texImage, GLuint dims)
{
   ASSERT(dims == 1 || dims == 2 || dims == 3);
   ASSERT(texImage->TexFormat);

   switch (dims) {
   case 1:
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel1D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel1Df;
      break;
   case 2:
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel2D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel2Df;
      break;
   case 3:
      texImage->FetchTexelc = texImage->TexFormat->FetchTexel3D;
      texImage->FetchTexelf = texImage->TexFormat->FetchTexel3Df;
      break;
   default:
      ;
   }

   /* now check if we need to use a float/chan adaptor */
   if (!texImage->FetchTexelc) {
      texImage->FetchTexelc = fetch_texel_float_to_chan;
   }
   else if (!texImage->FetchTexelf) {
      texImage->FetchTexelf = fetch_texel_chan_to_float;
   }


   ASSERT(texImage->FetchTexelc);
   ASSERT(texImage->FetchTexelf);
}


/**
 * Choose the actual storage format for a new texture image.
 * Mainly, this is a wrapper for the driver's ChooseTextureFormat() function.
 * Also set some other texImage fields related to texture compression, etc.
 * \param ctx  rendering context
 * \param texImage  the gl_texture_image
 * \param dims  texture dimensions (1, 2 or 3)
 * \param format  the user-specified format parameter
 * \param type  the user-specified type parameter
 * \param internalFormat  the user-specified internal format hint
 */
static void
choose_texture_format(GLcontext *ctx, struct gl_texture_image *texImage,
                      GLuint dims,
                      GLenum format, GLenum type, GLint internalFormat)
{
   ASSERT(dims == 1 || dims == 2 || dims == 3);
   ASSERT(ctx->Driver.ChooseTextureFormat);

   texImage->TexFormat
      = ctx->Driver.ChooseTextureFormat(ctx, internalFormat, format, type);

   ASSERT(texImage->TexFormat);

   _mesa_set_fetch_functions(texImage, dims);

   if (texImage->TexFormat->TexelBytes == 0) {
      /* must be a compressed format */
      texImage->IsCompressed = GL_TRUE;
      texImage->CompressedSize =
         ctx->Driver.CompressedTextureSize(ctx, texImage->Width,
                                           texImage->Height, texImage->Depth,
                                           texImage->TexFormat->MesaFormat);
   }
   else {
      /* non-compressed format */
      texImage->IsCompressed = GL_FALSE;
      texImage->CompressedSize = 0;
   }
}



/**
 * This is the software fallback for Driver.TexImage1D()
 * and Driver.CopyTexImage1D().
 * \sa _mesa_store_teximage2d()
 * Note that the width may not be the actual texture width since it may
 * be changed by convolution w/ GL_REDUCE.  The texImage->Width field will
 * have the actual texture size.
 */
void
_mesa_store_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage)
{
   GLint sizeInBytes;
   (void) border;

   choose_texture_format(ctx, texImage, 1, format, type, internalFormat);

   /* allocate memory */
   if (texImage->IsCompressed)
      sizeInBytes = texImage->CompressedSize;
   else
      sizeInBytes = texImage->Width * texImage->TexFormat->TexelBytes;
   texImage->Data = _mesa_alloc_texmemory(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
      return;
   }

   pixels = _mesa_validate_pbo_teximage(ctx, 1, width, 1, 1, format, type,
                                        pixels, packing, "glTexImage1D");
   if (!pixels) {
      /* Note: we check for a NULL image pointer here, _after_ we allocated
       * memory for the texture.  That's what the GL spec calls for.
       */
      return;
   }
   else {
      const GLint dstRowStride = 0;
      GLboolean success;
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 1, texImage->_BaseFormat,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                0, 0, 0,  /* dstX/Y/Zoffset */
                                                dstRowStride,
                                                texImage->ImageOffsets,
                                                width, 1, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
      }
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}


/**
 * This is the software fallback for Driver.TexImage2D()
 * and Driver.CopyTexImage2D().
 *
 * This function is oriented toward storing images in main memory, rather
 * than VRAM.  Device driver's can easily plug in their own replacement.
 *
 * Note: width and height may be pre-convolved dimensions, but
 * texImage->Width and texImage->Height will be post-convolved dimensions.
 */
void
_mesa_store_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint border,
                       GLenum format, GLenum type, const void *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage)
{
   GLint texelBytes, sizeInBytes;
   (void) border;

   choose_texture_format(ctx, texImage, 2, format, type, internalFormat);

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   if (texImage->IsCompressed)
      sizeInBytes = texImage->CompressedSize;
   else
      sizeInBytes = texImage->Width * texImage->Height * texelBytes;
   texImage->Data = _mesa_alloc_texmemory(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      return;
   }

   pixels = _mesa_validate_pbo_teximage(ctx, 2, width, height, 1, format, type,
                                        pixels, packing, "glTexImage2D");
   if (!pixels) {
      /* Note: we check for a NULL image pointer here, _after_ we allocated
       * memory for the texture.  That's what the GL spec calls for.
       */
      return;
   }
   else {
      GLint dstRowStride;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride
            = _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat, width);
      }
      else {
         dstRowStride = texImage->RowStride * texImage->TexFormat->TexelBytes;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 2, texImage->_BaseFormat,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                0, 0, 0,  /* dstX/Y/Zoffset */
                                                dstRowStride,
                                                texImage->ImageOffsets,
                                                width, height, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      }
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}



/**
 * This is the software fallback for Driver.TexImage3D()
 * and Driver.CopyTexImage3D().
 * \sa _mesa_store_teximage2d()
 */
void
_mesa_store_teximage3d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint depth, GLint border,
                       GLenum format, GLenum type, const void *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage)
{
   GLint texelBytes, sizeInBytes;
   (void) border;

   choose_texture_format(ctx, texImage, 3, format, type, internalFormat);

   texelBytes = texImage->TexFormat->TexelBytes;

   /* allocate memory */
   if (texImage->IsCompressed)
      sizeInBytes = texImage->CompressedSize;
   else
      sizeInBytes = width * height * depth * texelBytes;
   texImage->Data = _mesa_alloc_texmemory(sizeInBytes);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
      return;
   }

   pixels = _mesa_validate_pbo_teximage(ctx, 3, width, height, depth, format,
                                        type, pixels, packing, "glTexImage3D");
   if (!pixels) {
      /* Note: we check for a NULL image pointer here, _after_ we allocated
       * memory for the texture.  That's what the GL spec calls for.
       */
      return;
   }
   else {
      GLint dstRowStride;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride
            = _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat, width);
      }
      else {
         dstRowStride = texImage->RowStride * texImage->TexFormat->TexelBytes;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 3, texImage->_BaseFormat,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                0, 0, 0,  /* dstX/Y/Zoffset */
                                                dstRowStride,
                                                texImage->ImageOffsets,
                                                width, height, depth,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
      }
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}




/*
 * This is the software fallback for Driver.TexSubImage1D()
 * and Driver.CopyTexSubImage1D().
 */
void
_mesa_store_texsubimage1d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint width,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   pixels = _mesa_validate_pbo_teximage(ctx, 1, width, 1, 1, format, type,
                                        pixels, packing, "glTexSubImage1D");
   if (!pixels)
      return;

   {
      const GLint dstRowStride = 0;
      GLboolean success;
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 1, texImage->_BaseFormat,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                xoffset, 0, 0,  /* offsets */
                                                dstRowStride,
                                                texImage->ImageOffsets,
                                                width, 1, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D");
      }
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}



/**
 * This is the software fallback for Driver.TexSubImage2D()
 * and Driver.CopyTexSubImage2D().
 */
void
_mesa_store_texsubimage2d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLint width, GLint height,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   pixels = _mesa_validate_pbo_teximage(ctx, 2, width, height, 1, format, type,
                                        pixels, packing, "glTexSubImage2D");
   if (!pixels)
      return;

   {
      GLint dstRowStride = 0;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride = _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat,
                                                    texImage->Width);
      }
      else {
         dstRowStride = texImage->RowStride * texImage->TexFormat->TexelBytes;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 2, texImage->_BaseFormat,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                xoffset, yoffset, 0,
                                                dstRowStride,
                                                texImage->ImageOffsets,
                                                width, height, 1,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
      }
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}


/*
 * This is the software fallback for Driver.TexSubImage3D().
 * and Driver.CopyTexSubImage3D().
 */
void
_mesa_store_texsubimage3d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLint width, GLint height, GLint depth,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   pixels = _mesa_validate_pbo_teximage(ctx, 3, width, height, depth, format,
                                        type, pixels, packing,
                                        "glTexSubImage3D");
   if (!pixels)
      return;

   {
      GLint dstRowStride;
      GLboolean success;
      if (texImage->IsCompressed) {
         dstRowStride = _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat,
                                                    texImage->Width);
      }
      else {
         dstRowStride = texImage->RowStride * texImage->TexFormat->TexelBytes;
      }
      ASSERT(texImage->TexFormat->StoreImage);
      success = texImage->TexFormat->StoreImage(ctx, 3, texImage->_BaseFormat,
                                                texImage->TexFormat,
                                                texImage->Data,
                                                xoffset, yoffset, zoffset,
                                                dstRowStride,
                                                texImage->ImageOffsets,
                                                width, height, depth,
                                                format, type, pixels, packing);
      if (!success) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage3D");
      }
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, packing);
}


/*
 * Fallback for Driver.CompressedTexImage1D()
 */
void
_mesa_store_compressed_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage)
{
   /* this space intentionally left blank */
   (void) ctx;
   (void) target; (void) level;
   (void) internalFormat;
   (void) width; (void) border;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
}



/**
 * Fallback for Driver.CompressedTexImage2D()
 */
void
_mesa_store_compressed_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage)
{
   (void) width; (void) height; (void) border;

   /* This is pretty simple, basically just do a memcpy without worrying
    * about the usual image unpacking or image transfer operations.
    */
   ASSERT(texObj);
   ASSERT(texImage);
   ASSERT(texImage->Width > 0);
   ASSERT(texImage->Height > 0);
   ASSERT(texImage->Depth == 1);
   ASSERT(texImage->Data == NULL); /* was freed in glCompressedTexImage2DARB */

   choose_texture_format(ctx, texImage, 2, 0, 0, internalFormat);

   /* allocate storage */
   texImage->Data = _mesa_alloc_texmemory(imageSize);
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2DARB");
      return;
   }

   data = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, data,
                                                 &ctx->Unpack,
                                                 "glCompressedTexImage2D");
   if (!data)
      return;

   /* copy the data */
   ASSERT(texImage->CompressedSize == (GLuint) imageSize);
   MEMCPY(texImage->Data, data, imageSize);

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, &ctx->Unpack);
}



/*
 * Fallback for Driver.CompressedTexImage3D()
 */
void
_mesa_store_compressed_teximage3d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint depth,
                                  GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage)
{
   /* this space intentionally left blank */
   (void) ctx;
   (void) target; (void) level;
   (void) internalFormat;
   (void) width; (void) height; (void) depth;
   (void) border;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
}



/**
 * Fallback for Driver.CompressedTexSubImage1D()
 */
void
_mesa_store_compressed_texsubimage1d(GLcontext *ctx, GLenum target,
                                     GLint level,
                                     GLint xoffset, GLsizei width,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data,
                                     struct gl_texture_object *texObj,
                                     struct gl_texture_image *texImage)
{
   /* there are no compressed 1D texture formats yet */
   (void) ctx;
   (void) target; (void) level;
   (void) xoffset; (void) width;
   (void) format;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
}


/**
 * Fallback for Driver.CompressedTexSubImage2D()
 */
void
_mesa_store_compressed_texsubimage2d(GLcontext *ctx, GLenum target,
                                     GLint level,
                                     GLint xoffset, GLint yoffset,
                                     GLsizei width, GLsizei height,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data,
                                     struct gl_texture_object *texObj,
                                     struct gl_texture_image *texImage)
{
   GLint bytesPerRow, destRowStride, srcRowStride;
   GLint i, rows;
   GLubyte *dest;
   const GLubyte *src;
   const GLuint mesaFormat = texImage->TexFormat->MesaFormat;

   (void) format;

   /* these should have been caught sooner */
   ASSERT((width & 3) == 0 || width == 2 || width == 1);
   ASSERT((height & 3) == 0 || height == 2 || height == 1);
   ASSERT((xoffset & 3) == 0);
   ASSERT((yoffset & 3) == 0);

   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   data = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, data,
                                                 &ctx->Unpack,
                                                 "glCompressedTexSubImage2D");
   if (!data)
      return;

   srcRowStride = _mesa_compressed_row_stride(mesaFormat, width);
   src = (const GLubyte *) data;

   destRowStride = _mesa_compressed_row_stride(mesaFormat, texImage->Width);
   dest = _mesa_compressed_image_address(xoffset, yoffset, 0,
                                         texImage->TexFormat->MesaFormat,
                                         texImage->Width,
                                         (GLubyte *) texImage->Data);

   bytesPerRow = srcRowStride;
   rows = height / 4;

   for (i = 0; i < rows; i++) {
      MEMCPY(dest, src, bytesPerRow);
      dest += destRowStride;
      src += srcRowStride;
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }

   _mesa_unmap_teximage_pbo(ctx, &ctx->Unpack);
}


/**
 * Fallback for Driver.CompressedTexSubImage3D()
 */
void
_mesa_store_compressed_texsubimage3d(GLcontext *ctx, GLenum target,
                                GLint level,
                                GLint xoffset, GLint yoffset, GLint zoffset,
                                GLsizei width, GLsizei height, GLsizei depth,
                                GLenum format,
                                GLsizei imageSize, const GLvoid *data,
                                struct gl_texture_object *texObj,
                                struct gl_texture_image *texImage)
{
   /* there are no compressed 3D texture formats yet */
   (void) ctx;
   (void) target; (void) level;
   (void) xoffset; (void) yoffset; (void) zoffset;
   (void) width; (void) height; (void) depth;
   (void) format;
   (void) imageSize; (void) data;
   (void) texObj;
   (void) texImage;
}




#if FEATURE_EXT_texture_sRGB

/**
 * Test if given texture image is an sRGB format.
 */
static GLboolean
is_srgb_teximage(const struct gl_texture_image *texImage)
{
   switch (texImage->TexFormat->MesaFormat) {
   case MESA_FORMAT_SRGB8:
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SL8:
   case MESA_FORMAT_SLA8:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}

#endif /* FEATURE_EXT_texture_sRGB */


/**
 * This is the software fallback for Driver.GetTexImage().
 * All error checking will have been done before this routine is called.
 */
void
_mesa_get_teximage(GLcontext *ctx, GLenum target, GLint level,
                   GLenum format, GLenum type, GLvoid *pixels,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   const GLuint dimensions = (target == GL_TEXTURE_3D) ? 3 : 2;

   if (ctx->Pack.BufferObj->Name) {
      /* Packing texture image into a PBO.
       * Map the (potentially) VRAM-based buffer into our process space so
       * we can write into it with the code below.
       * A hardware driver might use a sophisticated blit to move the
       * texture data to the PBO if the PBO is in VRAM along with the texture.
       */
      GLubyte *buf = (GLubyte *)
         ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                               GL_WRITE_ONLY_ARB, ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,"glGetTexImage(PBO is mapped)");
         return;
      }
      /* <pixels> was an offset into the PBO.
       * Now make it a real, client-side pointer inside the mapped region.
       */
      pixels = ADD_POINTERS(buf, pixels);
   }
   else if (!pixels) {
      /* not an error */
      return;
   }

   {
      const GLint width = texImage->Width;
      const GLint height = texImage->Height;
      const GLint depth = texImage->Depth;
      GLint img, row;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            /* compute destination address in client memory */
            GLvoid *dest = _mesa_image_address( dimensions, &ctx->Pack, pixels,
                                                width, height, format, type,
                                                img, row, 0);
            assert(dest);

            if (format == GL_COLOR_INDEX) {
               GLuint indexRow[MAX_WIDTH];
               GLint col;
               /* Can't use FetchTexel here because that returns RGBA */
               if (texImage->TexFormat->IndexBits == 8) {
                  const GLubyte *src = (const GLubyte *) texImage->Data;
                  src += width * (img * texImage->Height + row);
                  for (col = 0; col < width; col++) {
                     indexRow[col] = src[col];
                  }
               }
               else if (texImage->TexFormat->IndexBits == 16) {
                  const GLushort *src = (const GLushort *) texImage->Data;
                  src += width * (img * texImage->Height + row);
                  for (col = 0; col < width; col++) {
                     indexRow[col] = src[col];
                  }
               }
               else {
                  _mesa_problem(ctx,
                                "Color index problem in _mesa_GetTexImage");
               }
               _mesa_pack_index_span(ctx, width, type, dest,
                                     indexRow, &ctx->Pack,
                                     0 /* no image transfer */);
            }
            else if (format == GL_DEPTH_COMPONENT) {
               GLfloat depthRow[MAX_WIDTH];
               GLint col;
               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexelf)(texImage, col, row, img,
                                           depthRow + col);
               }
               _mesa_pack_depth_span(ctx, width, dest, type,
                                     depthRow, &ctx->Pack);
            }
            else if (format == GL_DEPTH_STENCIL_EXT) {
               /* XXX Note: we're bypassing texImage->FetchTexel()! */
               const GLuint *src = (const GLuint *) texImage->Data;
               src += width * row + width * height * img;
               _mesa_memcpy(dest, src, width * sizeof(GLuint));
               if (ctx->Pack.SwapBytes) {
                  _mesa_swap4((GLuint *) dest, width);
               }
            }
            else if (format == GL_YCBCR_MESA) {
               /* No pixel transfer */
               const GLint rowstride = texImage->RowStride;
               MEMCPY(dest,
                      (const GLushort *) texImage->Data + row * rowstride,
                      width * sizeof(GLushort));
               /* check for byte swapping */
               if ((texImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR
                    && type == GL_UNSIGNED_SHORT_8_8_REV_MESA) ||
                   (texImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR_REV
                    && type == GL_UNSIGNED_SHORT_8_8_MESA)) {
                  if (!ctx->Pack.SwapBytes)
                     _mesa_swap2((GLushort *) dest, width);
               }
               else if (ctx->Pack.SwapBytes) {
                  _mesa_swap2((GLushort *) dest, width);
               }
            }
#if FEATURE_EXT_texture_sRGB
            else if (is_srgb_teximage(texImage)) {
               /* no pixel transfer and no non-linear to linear conversion */
               const GLint comps = texImage->TexFormat->TexelBytes;
               const GLint rowstride = comps * texImage->RowStride;
               MEMCPY(dest,
                      (const GLubyte *) texImage->Data + row * rowstride,
                      comps * width * sizeof(GLubyte));
            }
#endif /* FEATURE_EXT_texture_sRGB */
            else {
               /* general case:  convert row to RGBA format */
               GLfloat rgba[MAX_WIDTH][4];
               GLint col;
               GLbitfield transferOps = 0x0;

               if (type == GL_FLOAT && 
                   ((ctx->Color.ClampReadColor == GL_TRUE) ||
                    (ctx->Color.ClampReadColor == GL_FIXED_ONLY_ARB &&
                     texImage->TexFormat->DataType != GL_FLOAT)))
                  transferOps |= IMAGE_CLAMP_BIT;

               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexelf)(texImage, col, row, img, rgba[col]);
                  if (texImage->TexFormat->BaseFormat == GL_ALPHA) {
                     rgba[col][RCOMP] = 0.0;
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                  }
                  else if (texImage->TexFormat->BaseFormat == GL_LUMINANCE) {
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                     rgba[col][ACOMP] = 1.0;
                  }
                  else if (texImage->TexFormat->BaseFormat == GL_LUMINANCE_ALPHA) {
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                  }
                  else if (texImage->TexFormat->BaseFormat == GL_INTENSITY) {
                     rgba[col][GCOMP] = 0.0;
                     rgba[col][BCOMP] = 0.0;
                     rgba[col][ACOMP] = 1.0;
                  }
               }
               _mesa_pack_rgba_span_float(ctx, width, (GLfloat (*)[4]) rgba,
                                          format, type, dest,
                                          &ctx->Pack, transferOps /*image xfer ops*/);
            } /* format */
         } /* row */
      } /* img */
   }

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}



/**
 * This is the software fallback for Driver.GetCompressedTexImage().
 * All error checking will have been done before this routine is called.
 */
void
_mesa_get_compressed_teximage(GLcontext *ctx, GLenum target, GLint level,
                              GLvoid *img,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage)
{
   GLuint size;

   if (ctx->Pack.BufferObj->Name) {
      /* pack texture image into a PBO */
      GLubyte *buf;
      if ((const GLubyte *) img + texImage->CompressedSize >
          (const GLubyte *) ctx->Pack.BufferObj->Size) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetCompressedTexImage(PBO is mapped)");
         return;
      }
      img = ADD_POINTERS(buf, img);
   }
   else if (!img) {
      /* not an error */
      return;
   }

   /* don't use texImage->CompressedSize since that may be padded out */
   size = _mesa_compressed_texture_size(ctx, texImage->Width, texImage->Height,
                                        texImage->Depth,
                                        texImage->TexFormat->MesaFormat);

   /* just memcpy, no pixelstore or pixel transfer */
   _mesa_memcpy(img, texImage->Data, size);

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}
