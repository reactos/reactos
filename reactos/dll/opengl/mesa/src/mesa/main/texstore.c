/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
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
 *    pixel transfer (scale, bais, lookup, etc)
 *
 * These functions can handle most everything, including processing full
 * images and sub-images.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "image.h"
#include "macros.h"
#include "mipmap.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "pack.h"
#include "pbo.h"
#include "imports.h"
#include "texcompress.h"
#include "texcompress_fxt1.h"
#include "texcompress_rgtc.h"
#include "texcompress_s3tc.h"
#include "texcompress_etc.h"
#include "teximage.h"
#include "texstore.h"
#include "enums.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"


enum {
   ZERO = 4, 
   ONE = 5
};


/**
 * Texture image storage function.
 */
typedef GLboolean (*StoreTexImageFunc)(TEXSTORE_PARAMS);


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
   case GL_RG:
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
   IDX_RG,
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

   {
      IDX_RG,
      MAP4(0, 1, ZERO, ONE),
      MAP2(0, 1)
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
   case GL_RG: return IDX_RG;
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

#if 0
   printf("from %x/%s to %x/%s map %d %d %d %d %d %d\n",
	  inFormat, _mesa_lookup_enum_by_nr(inFormat),
	  outFormat, _mesa_lookup_enum_by_nr(outFormat),
	  map[0], 
	  map[1], 
	  map[2], 
	  map[3], 
	  map[4], 
	  map[5]); 
#endif
}


/**
 * Make a temporary (color) texture image with GLfloat components.
 * Apply all needed pixel unpacking and pixel transfer operations.
 * Note that there are both logicalBaseFormat and textureBaseFormat parameters.
 * Suppose the user specifies GL_LUMINANCE as the internal texture format
 * but the graphics hardware doesn't support luminance textures.  So, we might
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
GLfloat *
_mesa_make_temp_float_image(struct gl_context *ctx, GLuint dims,
			    GLenum logicalBaseFormat,
			    GLenum textureBaseFormat,
			    GLint srcWidth, GLint srcHeight, GLint srcDepth,
			    GLenum srcFormat, GLenum srcType,
			    const GLvoid *srcAddr,
			    const struct gl_pixelstore_attrib *srcPacking,
			    GLbitfield transferOps)
{
   GLfloat *tempImage;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   const GLint srcStride =
      _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLfloat *dst;
   GLint img, row;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_RG ||
          logicalBaseFormat == GL_RED ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY ||
          logicalBaseFormat == GL_DEPTH_COMPONENT);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_RG ||
          textureBaseFormat == GL_RED ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY ||
          textureBaseFormat == GL_DEPTH_COMPONENT);

   tempImage = (GLfloat *) malloc(srcWidth * srcHeight * srcDepth
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

      newImage = (GLfloat *) malloc(srcWidth * srcHeight * srcDepth
                                          * texComponents * sizeof(GLfloat));
      if (!newImage) {
         free(tempImage);
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

      free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}


/**
 * Make temporary image with uint pixel values.  Used for unsigned
 * integer-valued textures.
 */
static GLuint *
make_temp_uint_image(struct gl_context *ctx, GLuint dims,
                     GLenum logicalBaseFormat,
                     GLenum textureBaseFormat,
                     GLint srcWidth, GLint srcHeight, GLint srcDepth,
                     GLenum srcFormat, GLenum srcType,
                     const GLvoid *srcAddr,
                     const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint *tempImage;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   const GLint srcStride =
      _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLuint *dst;
   GLint img, row;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_RG ||
          logicalBaseFormat == GL_RED ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_INTENSITY ||
          logicalBaseFormat == GL_ALPHA);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_RG ||
          textureBaseFormat == GL_RED ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_INTENSITY ||
          textureBaseFormat == GL_ALPHA);

   tempImage = (GLuint *) malloc(srcWidth * srcHeight * srcDepth
                                 * components * sizeof(GLuint));
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
	 _mesa_unpack_color_span_uint(ctx, srcWidth, logicalBaseFormat,
                                      dst, srcFormat, srcType, src,
                                      srcPacking);
	 dst += srcWidth * components;
	 src += srcStride;
      }
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* more work */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLuint *newImage;
      GLint i, n;
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLuint *) malloc(srcWidth * srcHeight * srcDepth
                                   * texComponents * sizeof(GLuint));
      if (!newImage) {
         free(tempImage);
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
               newImage[i * texComponents + k] = 1;
            else
               newImage[i * texComponents + k] = tempImage[i * logComponents + j];
         }
      }

      free(tempImage);
      tempImage = newImage;
   }

   return tempImage;
}



/**
 * Make a temporary (color) texture image with GLubyte components.
 * Apply all needed pixel unpacking and pixel transfer operations.
 * Note that there are both logicalBaseFormat and textureBaseFormat parameters.
 * Suppose the user specifies GL_LUMINANCE as the internal texture format
 * but the graphics hardware doesn't support luminance textures.  So, we might
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
 * \return resulting image with format = textureBaseFormat and type = GLubyte.
 */
GLubyte *
_mesa_make_temp_ubyte_image(struct gl_context *ctx, GLuint dims,
                            GLenum logicalBaseFormat,
                            GLenum textureBaseFormat,
                            GLint srcWidth, GLint srcHeight, GLint srcDepth,
                            GLenum srcFormat, GLenum srcType,
                            const GLvoid *srcAddr,
                            const struct gl_pixelstore_attrib *srcPacking)
{
   GLuint transferOps = ctx->_ImageTransferState;
   const GLint components = _mesa_components_in_format(logicalBaseFormat);
   GLint img, row;
   GLubyte *tempImage, *dst;

   ASSERT(dims >= 1 && dims <= 3);

   ASSERT(logicalBaseFormat == GL_RGBA ||
          logicalBaseFormat == GL_RGB ||
          logicalBaseFormat == GL_RG ||
          logicalBaseFormat == GL_RED ||
          logicalBaseFormat == GL_LUMINANCE_ALPHA ||
          logicalBaseFormat == GL_LUMINANCE ||
          logicalBaseFormat == GL_ALPHA ||
          logicalBaseFormat == GL_INTENSITY);

   ASSERT(textureBaseFormat == GL_RGBA ||
          textureBaseFormat == GL_RGB ||
          textureBaseFormat == GL_RG ||
          textureBaseFormat == GL_RED ||
          textureBaseFormat == GL_LUMINANCE_ALPHA ||
          textureBaseFormat == GL_LUMINANCE ||
          textureBaseFormat == GL_ALPHA ||
          textureBaseFormat == GL_INTENSITY);

   /* unpack and transfer the source image */
   tempImage = (GLubyte *) malloc(srcWidth * srcHeight * srcDepth
                                       * components * sizeof(GLubyte));
   if (!tempImage) {
      return NULL;
   }

   dst = tempImage;
   for (img = 0; img < srcDepth; img++) {
      const GLint srcStride =
         _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
      const GLubyte *src =
         (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                               srcWidth, srcHeight,
                                               srcFormat, srcType,
                                               img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
         _mesa_unpack_color_span_ubyte(ctx, srcWidth, logicalBaseFormat, dst,
                                       srcFormat, srcType, src, srcPacking,
                                       transferOps);
         dst += srcWidth * components;
         src += srcStride;
      }
   }

   if (logicalBaseFormat != textureBaseFormat) {
      /* one more conversion step */
      GLint texComponents = _mesa_components_in_format(textureBaseFormat);
      GLint logComponents = _mesa_components_in_format(logicalBaseFormat);
      GLubyte *newImage;
      GLint i, n;
      GLubyte map[6];

      /* we only promote up to RGB, RGBA and LUMINANCE_ALPHA formats for now */
      ASSERT(textureBaseFormat == GL_RGB || textureBaseFormat == GL_RGBA ||
             textureBaseFormat == GL_LUMINANCE_ALPHA);

      /* The actual texture format should have at least as many components
       * as the logical texture format.
       */
      ASSERT(texComponents >= logComponents);

      newImage = (GLubyte *) malloc(srcWidth * srcHeight * srcDepth
                                         * texComponents * sizeof(GLubyte));
      if (!newImage) {
         free(tempImage);
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
               newImage[i * texComponents + k] = 255;
            else
               newImage[i * texComponents + k] = tempImage[i * logComponents + j];
         }
      }

      free(tempImage);
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


/**
 * For 1-byte/pixel formats (or 8_8_8_8 packed formats), return a
 * mapping array depending on endianness.
 */
static const GLubyte *
type_mapping( GLenum srcType )
{
   switch (srcType) {
   case GL_BYTE:
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


/**
 * For 1-byte/pixel formats (or 8_8_8_8 packed formats), return a
 * mapping array depending on pixelstore byte swapping state.
 */
static const GLubyte *
byteswap_mapping( GLboolean swapBytes,
		  GLenum srcType )
{
   if (!swapBytes) 
      return map_identity;

   switch (srcType) {
   case GL_BYTE:
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
_mesa_swizzle_ubyte_image(struct gl_context *ctx, 
			  GLuint dimensions,
			  GLenum srcFormat,
			  GLenum srcType,

			  GLenum baseInternalFormat,

			  const GLubyte *rgba2dst,
			  GLuint dstComponents,

			  GLint dstRowStride,
                          GLubyte **dstSlices,

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

/*    printf("map %d %d %d %d\n", map[0], map[1], map[2], map[3]);  */

   if (srcComponents == dstComponents &&
       srcRowStride == dstRowStride &&
       srcRowStride == srcWidth * srcComponents &&
       dimensions < 3) {
      /* 1 and 2D images only */
      GLubyte *dstImage = dstSlices[0];
      swizzle_copy(dstImage, dstComponents, srcImage, srcComponents, map, 
		   srcWidth * srcHeight);
   }
   else {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = dstSlices[img];
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
memcpy_texture(struct gl_context *ctx,
	       GLuint dimensions,
               gl_format dstFormat,
               GLint dstRowStride,
               GLubyte **dstSlices,
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
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
   const GLint bytesPerRow = srcWidth * texelBytes;

   if (dstRowStride == srcRowStride &&
       dstRowStride == bytesPerRow) {
      /* memcpy image by image */
      GLint img;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstImage = dstSlices[img];
         memcpy(dstImage, srcImage, bytesPerRow * srcHeight);
         srcImage += srcImageStride;
      }
   }
   else {
      /* memcpy row by row */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         const GLubyte *srcRow = srcImage;
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            memcpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcRowStride;
         }
         srcImage += srcImageStride;
      }
   }
}



/**
 * Store a 32-bit integer or float depth component texture image.
 */
static GLboolean
_mesa_texstore_z32(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffffff;
   GLenum dstType;
   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z32 ||
          dstFormat == MESA_FORMAT_Z32_FLOAT);
   ASSERT(_mesa_get_format_bytes(dstFormat) == sizeof(GLuint));

   if (dstFormat == MESA_FORMAT_Z32)
      dstType = GL_UNSIGNED_INT;
   else
      dstType = GL_FLOAT;

   if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == dstType) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    dstType, dstRow,
                                    depthScale, srcType, src, srcPacking);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store a 24-bit integer depth component texture image.
 */
static GLboolean
_mesa_texstore_x8_z24(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;

   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_X8_Z24);

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
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


/**
 * Store a 24-bit integer depth component texture image.
 */
static GLboolean
_mesa_texstore_z24_x8(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;

   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z24_X8);

   {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            const GLvoid *src = _mesa_image_address(dims, srcPacking,
                srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, row, 0);
            GLuint *dst = (GLuint *) dstRow;
            GLint i;
            _mesa_unpack_depth_span(ctx, srcWidth,
                                    GL_UNSIGNED_INT, dst,
                                    depthScale, srcType, src, srcPacking);
            for (i = 0; i < srcWidth; i++)
               dst[i] <<= 8;
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}


/**
 * Store a 16-bit integer depth component texture image.
 */
static GLboolean
_mesa_texstore_z16(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffff;
   (void) dims;
   ASSERT(dstFormat == MESA_FORMAT_Z16);
   ASSERT(_mesa_get_format_bytes(dstFormat) == sizeof(GLushort));

   if (ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_DEPTH_COMPONENT &&
       srcFormat == GL_DEPTH_COMPONENT &&
       srcType == GL_UNSIGNED_SHORT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
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
static GLboolean
_mesa_texstore_rgb565(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGB565 ||
          dstFormat == MESA_FORMAT_RGB565_REV);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_RGB565 &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_SHORT_5_6_5) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
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
      const GLint srcRowStride =
         _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
      const GLubyte *src = (const GLubyte *)
         _mesa_image_address(dims, srcPacking, srcAddr, srcWidth, srcHeight,
                             srcFormat, srcType, 0, 0, 0);
      GLubyte *dst = dstSlices[0];
      GLint row, col;
      for (row = 0; row < srcHeight; row++) {
         const GLubyte *srcUB = (const GLubyte *) src;
         GLushort *dstUS = (GLushort *) dst;
         /* check for byteswapped format */
         if (dstFormat == MESA_FORMAT_RGB565) {
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
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            /* check for byteswapped format */
            if (dstFormat == MESA_FORMAT_RGB565) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_565( src[RCOMP],
                                               src[GCOMP],
                                               src[BCOMP] );
                  src += 3;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_565_REV( src[RCOMP],
                                                   src[GCOMP],
                                                   src[BCOMP] );
                  src += 3;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a texture in MESA_FORMAT_RGBA8888 or MESA_FORMAT_RGBA8888_REV.
 */
static GLboolean
_mesa_texstore_rgba8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA8888 ||
          dstFormat == MESA_FORMAT_RGBA8888_REV ||
          dstFormat == MESA_FORMAT_RGBX8888 ||
          dstFormat == MESA_FORMAT_RGBX8888_REV);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
      (dstFormat == MESA_FORMAT_RGBA8888 ||
       dstFormat == MESA_FORMAT_RGBX8888) &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian))) {
       /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
      (dstFormat == MESA_FORMAT_RGBA8888_REV ||
       dstFormat == MESA_FORMAT_RGBX8888_REV) &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
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
      if ((littleEndian && (dstFormat == MESA_FORMAT_RGBA8888 ||
                            dstFormat == MESA_FORMAT_RGBX8888)) ||
	  (!littleEndian && (dstFormat == MESA_FORMAT_RGBA8888_REV ||
	                     dstFormat == MESA_FORMAT_RGBX8888_REV))) {
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
				dstRowStride, dstSlices,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == MESA_FORMAT_RGBA8888 ||
                dstFormat == MESA_FORMAT_RGBX8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( src[RCOMP],
                                                src[GCOMP],
                                                src[BCOMP],
                                                src[ACOMP] );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888_REV( src[RCOMP],
                                                    src[GCOMP],
                                                    src[BCOMP],
                                                    src[ACOMP] );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_argb8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = GL_RGBA;

   ASSERT(dstFormat == MESA_FORMAT_ARGB8888 ||
          dstFormat == MESA_FORMAT_ARGB8888_REV ||
          dstFormat == MESA_FORMAT_XRGB8888 ||
          dstFormat == MESA_FORMAT_XRGB8888_REV );
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       (dstFormat == MESA_FORMAT_ARGB8888 ||
        dstFormat == MESA_FORMAT_XRGB8888) &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_BYTE && littleEndian) ||
        srcType == GL_UNSIGNED_INT_8_8_8_8_REV)) {
      /* simple memcpy path (little endian) */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       (dstFormat == MESA_FORMAT_ARGB8888_REV ||
        dstFormat == MESA_FORMAT_XRGB8888_REV) &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       ((srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
        srcType == GL_UNSIGNED_INT_8_8_8_8)) {
      /* simple memcpy path (big endian) */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
            !srcPacking->SwapBytes &&
	    (dstFormat == MESA_FORMAT_ARGB8888 ||
             dstFormat == MESA_FORMAT_XRGB8888) &&
            srcFormat == GL_RGB &&
	    (baseInternalFormat == GL_RGBA ||
	     baseInternalFormat == GL_RGB) &&
            srcType == GL_UNSIGNED_BYTE) {
      int img, row, col;
      for (img = 0; img < srcDepth; img++) {
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = dstSlices[img];
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
	    dstFormat == MESA_FORMAT_ARGB8888 &&
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
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = dstSlices[img];
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
      if ((littleEndian && dstFormat == MESA_FORMAT_ARGB8888) ||
          (littleEndian && dstFormat == MESA_FORMAT_XRGB8888) ||
	  (!littleEndian && dstFormat == MESA_FORMAT_ARGB8888_REV) ||
	  (!littleEndian && dstFormat == MESA_FORMAT_XRGB8888_REV)) {
	 dstmap[3] = 3;		/* alpha */
	 dstmap[2] = 0;		/* red */
	 dstmap[1] = 1;		/* green */
	 dstmap[0] = 2;		/* blue */
      }
      else {
	 assert((littleEndian && dstFormat == MESA_FORMAT_ARGB8888_REV) ||
		(!littleEndian && dstFormat == MESA_FORMAT_ARGB8888) ||
		(littleEndian && dstFormat == MESA_FORMAT_XRGB8888_REV) ||
		(!littleEndian && dstFormat == MESA_FORMAT_XRGB8888));
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
				dstRowStride,
                                dstSlices,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == MESA_FORMAT_ARGB8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( src[ACOMP],
                                                src[RCOMP],
                                                src[GCOMP],
                                                src[BCOMP] );
                  src += 4;
               }
            }
            else if (dstFormat == MESA_FORMAT_XRGB8888) {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888( 0xff,
                                                src[RCOMP],
                                                src[GCOMP],
                                                src[BCOMP] );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUI[col] = PACK_COLOR_8888_REV( src[ACOMP],
                                                    src[RCOMP],
                                                    src[GCOMP],
                                                    src[BCOMP] );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_rgb888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGB888);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 3);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_BGR &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
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
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = dstSlices[img];
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
				dstRowStride, dstSlices,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = (const GLubyte *) tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
#if 0
            if (littleEndian) {
               for (col = 0; col < srcWidth; col++) {
                  dstRow[col * 3 + 0] = src[RCOMP];
                  dstRow[col * 3 + 1] = src[GCOMP];
                  dstRow[col * 3 + 2] = src[BCOMP];
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
               dstRow[col * 3 + 0] = src[BCOMP];
               dstRow[col * 3 + 1] = src[GCOMP];
               dstRow[col * 3 + 2] = src[RCOMP];
               src += 3;
            }
#endif
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_bgr888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_BGR888);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 3);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
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
         const GLint srcRowStride =
            _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
         GLubyte *srcRow = (GLubyte *) _mesa_image_address(dims, srcPacking,
                  srcAddr, srcWidth, srcHeight, srcFormat, srcType, img, 0, 0);
         GLubyte *dstRow = dstSlices[img];
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
				dstRowStride, dstSlices,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = (const GLubyte *) tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col * 3 + 0] = src[RCOMP];
               dstRow[col * 3 + 1] = src[GCOMP];
               dstRow[col * 3 + 2] = src[BCOMP];
               src += 3;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_argb4444(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_ARGB4444 ||
          dstFormat == MESA_FORMAT_ARGB4444_REV);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_ARGB4444 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == MESA_FORMAT_ARGB4444) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_4444( src[ACOMP],
                                                src[RCOMP],
                                                src[GCOMP],
                                                src[BCOMP] );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_4444_REV( src[ACOMP],
                                                    src[RCOMP],
                                                    src[GCOMP],
                                                    src[BCOMP] );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_rgba5551(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA5551);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_RGBA5551 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_RGBA &&
       srcType == GL_UNSIGNED_SHORT_5_5_5_1) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src =tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
	       dstUS[col] = PACK_COLOR_5551( src[RCOMP],
					     src[GCOMP],
					     src[BCOMP],
					     src[ACOMP] );
	      src += 4;
	    }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_argb1555(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_ARGB1555 ||
          dstFormat == MESA_FORMAT_ARGB1555_REV);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_ARGB1555 &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_BGRA &&
       srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src =tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == MESA_FORMAT_ARGB1555) {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_1555( src[ACOMP],
                                                src[RCOMP],
                                                src[GCOMP],
                                                src[BCOMP] );
                  src += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dstUS[col] = PACK_COLOR_1555_REV( src[ACOMP],
                                                    src[RCOMP],
                                                    src[GCOMP],
                                                    src[BCOMP] );
                  src += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_argb2101010(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_ARGB2101010);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_ARGB2101010 &&
       srcFormat == GL_BGRA &&
       srcType == GL_UNSIGNED_INT_2_10_10_10_REV &&
       baseInternalFormat == GL_RGBA) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         if (baseInternalFormat == GL_RGBA) {
            for (row = 0; row < srcHeight; row++) {
               GLuint *dstUI = (GLuint *) dstRow;
               for (col = 0; col < srcWidth; col++) {
                  GLushort a,r,g,b;

                  UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
                  UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
                  UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
                  UNCLAMPED_FLOAT_TO_USHORT(b, src[BCOMP]);
                  dstUI[col] = PACK_COLOR_2101010_US(a, r, g, b);
                  src += 4;
               }
               dstRow += dstRowStride;
            }
         } else if (baseInternalFormat == GL_RGB) {
            for (row = 0; row < srcHeight; row++) {
               GLuint *dstUI = (GLuint *) dstRow;
               for (col = 0; col < srcWidth; col++) {
                  GLushort r,g,b;

                  UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
                  UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
                  UNCLAMPED_FLOAT_TO_USHORT(b, src[BCOMP]);
                  dstUI[col] = PACK_COLOR_2101010_US(0xffff, r, g, b);
                  src += 4;
               }
               dstRow += dstRowStride;
            }
         } else {
            ASSERT(0);
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Do texstore for 2-channel, 4-bit/channel, unsigned normalized formats.
 */
static GLboolean
_mesa_texstore_unorm44(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_AL44);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 1);

   {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLubyte *dstUS = (GLubyte *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               /* src[0] is luminance, src[1] is alpha */
               dstUS[col] = PACK_COLOR_44( src[1],
                                           src[0] );
               src += 2;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Do texstore for 2-channel, 8-bit/channel, unsigned normalized formats.
 */
static GLboolean
_mesa_texstore_unorm88(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_AL88 ||
          dstFormat == MESA_FORMAT_AL88_REV ||
          dstFormat == MESA_FORMAT_GR88 ||
          dstFormat == MESA_FORMAT_RG88);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       ((dstFormat == MESA_FORMAT_AL88 &&
         baseInternalFormat == GL_LUMINANCE_ALPHA &&
         srcFormat == GL_LUMINANCE_ALPHA) ||
        (dstFormat == MESA_FORMAT_GR88 &&
         baseInternalFormat == srcFormat)) &&
       srcType == GL_UNSIGNED_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
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
      if (dstFormat == MESA_FORMAT_AL88 || dstFormat == MESA_FORMAT_AL88_REV) {
	 if ((littleEndian && dstFormat == MESA_FORMAT_AL88) ||
	     (!littleEndian && dstFormat == MESA_FORMAT_AL88_REV)) {
	    dstmap[0] = 0;
	    dstmap[1] = 3;
	 }
	 else {
	    dstmap[0] = 3;
	    dstmap[1] = 0;
	 }
      }
      else {
	 if ((littleEndian && dstFormat == MESA_FORMAT_GR88) ||
	     (!littleEndian && dstFormat == MESA_FORMAT_RG88)) {
	    dstmap[0] = 0;
	    dstmap[1] = 1;
	 }
	 else {
	    dstmap[0] = 1;
	    dstmap[1] = 0;
	 }
      }
      dstmap[2] = ZERO;		/* ? */
      dstmap[3] = ONE;		/* ? */
      
      _mesa_swizzle_ubyte_image(ctx, dims,
				srcFormat,
				srcType,
				baseInternalFormat,
				dstmap, 2,
				dstRowStride, dstSlices,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            if (dstFormat == MESA_FORMAT_AL88 ||
		dstFormat == MESA_FORMAT_GR88) {
               for (col = 0; col < srcWidth; col++) {
                  /* src[0] is luminance (or R), src[1] is alpha (or G) */
                 dstUS[col] = PACK_COLOR_88( src[1],
                                             src[0] );
                 src += 2;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  /* src[0] is luminance (or R), src[1] is alpha (or G) */
                 dstUS[col] = PACK_COLOR_88_REV( src[1],
                                                 src[0] );
                 src += 2;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Do texstore for 2-channel, 16-bit/channel, unsigned normalized formats.
 */
static GLboolean
_mesa_texstore_unorm1616(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_AL1616 ||
          dstFormat == MESA_FORMAT_AL1616_REV ||
	  dstFormat == MESA_FORMAT_RG1616 ||
          dstFormat == MESA_FORMAT_RG1616_REV);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       ((dstFormat == MESA_FORMAT_AL1616 &&
         baseInternalFormat == GL_LUMINANCE_ALPHA &&
         srcFormat == GL_LUMINANCE_ALPHA) ||
        (dstFormat == MESA_FORMAT_RG1616 &&
         baseInternalFormat == srcFormat)) &&
       srcType == GL_UNSIGNED_SHORT &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            if (dstFormat == MESA_FORMAT_AL1616 ||
		dstFormat == MESA_FORMAT_RG1616) {
               for (col = 0; col < srcWidth; col++) {
		  GLushort l, a;

		  UNCLAMPED_FLOAT_TO_USHORT(l, src[0]);
		  UNCLAMPED_FLOAT_TO_USHORT(a, src[1]);
		  dstUI[col] = PACK_COLOR_1616(a, l);
		  src += 2;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
		  GLushort l, a;

		  UNCLAMPED_FLOAT_TO_USHORT(l, src[0]);
		  UNCLAMPED_FLOAT_TO_USHORT(a, src[1]);
		  dstUI[col] = PACK_COLOR_1616_REV(a, l);
		  src += 2;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/* Texstore for R16, A16, L16, I16. */
static GLboolean
_mesa_texstore_unorm16(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_R16 ||
          dstFormat == MESA_FORMAT_A16 ||
          dstFormat == MESA_FORMAT_L16 ||
          dstFormat == MESA_FORMAT_I16);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_SHORT &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
	       GLushort r;

	       UNCLAMPED_FLOAT_TO_USHORT(r, src[0]);
	       dstUS[col] = r;
	       src += 1;
	    }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_rgba_16(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA_16);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 8);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGBA &&
       srcFormat == GL_RGBA &&
       srcType == GL_UNSIGNED_SHORT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstUS = (GLushort *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               GLushort r, g, b, a;

               UNCLAMPED_FLOAT_TO_USHORT(r, src[0]);
               UNCLAMPED_FLOAT_TO_USHORT(g, src[1]);
               UNCLAMPED_FLOAT_TO_USHORT(b, src[2]);
               UNCLAMPED_FLOAT_TO_USHORT(a, src[3]);
               dstUS[col*4+0] = r;
               dstUS[col*4+1] = g;
               dstUS[col*4+2] = b;
               dstUS[col*4+3] = a;
               src += 4;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_signed_rgba_16(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RGB_16 ||
          dstFormat == MESA_FORMAT_SIGNED_RGBA_16);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGBA &&
       dstFormat == MESA_FORMAT_SIGNED_RGBA_16 &&
       srcFormat == GL_RGBA &&
       srcType == GL_SHORT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      const GLuint comps = _mesa_get_format_bytes(dstFormat) / 2;
      GLint img, row, col;

      if (!tempImage)
         return GL_FALSE;

      /* Note: tempImage is always float[4] / RGBA.  We convert to 1, 2,
       * 3 or 4 components/pixel here.
       */
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLshort *dstRowS = (GLshort *) dstRow;
            if (dstFormat == MESA_FORMAT_SIGNED_RGBA_16) {
               for (col = 0; col < srcWidth; col++) {
                  GLuint c;
                  for (c = 0; c < comps; c++) {
                     GLshort p;
                     UNCLAMPED_FLOAT_TO_SHORT(p, src[col * 4 + c]);
                     dstRowS[col * comps + c] = p;
                  }
               }
               dstRow += dstRowStride;
               src += 4 * srcWidth;
            } else {
               for (col = 0; col < srcWidth; col++) {
                  GLuint c;
                  for (c = 0; c < comps; c++) {
                     GLshort p;
                     UNCLAMPED_FLOAT_TO_SHORT(p, src[col * 3 + c]);
                     dstRowS[col * comps + c] = p;
                  }
               }
               dstRow += dstRowStride;
               src += 3 * srcWidth;
            }
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_rgb332(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGB332);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == GL_RGB &&
       srcFormat == GL_RGB && srcType == GL_UNSIGNED_BYTE_3_3_2) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = PACK_COLOR_332( src[RCOMP],
                                             src[GCOMP],
                                             src[BCOMP] );
               src += 3;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Texstore for _mesa_texformat_a8, _mesa_texformat_l8, _mesa_texformat_i8.
 */
static GLboolean
_mesa_texstore_unorm8(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_A8 ||
          dstFormat == MESA_FORMAT_L8 ||
          dstFormat == MESA_FORMAT_I8 ||
          dstFormat == MESA_FORMAT_R8);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
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
      if (dstFormat == MESA_FORMAT_A8) {
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
				dstRowStride, dstSlices,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path */
      const GLubyte *tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking);
      const GLubyte *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = src[col];
            }
            dstRow += dstRowStride;
            src += srcWidth;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}



/**
 * Texstore for _mesa_texformat_ycbcr or _mesa_texformat_ycbcr_REV.
 */
static GLboolean
_mesa_texstore_ycbcr(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();

   (void) ctx; (void) dims; (void) baseInternalFormat;

   ASSERT((dstFormat == MESA_FORMAT_YCBCR) ||
          (dstFormat == MESA_FORMAT_YCBCR_REV));
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);
   ASSERT(ctx->Extensions.MESA_ycbcr_texture);
   ASSERT(srcFormat == GL_YCBCR_MESA);
   ASSERT((srcType == GL_UNSIGNED_SHORT_8_8_MESA) ||
          (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA));
   ASSERT(baseInternalFormat == GL_YCBCR_MESA);

   /* always just memcpy since no pixel transfer ops apply */
   memcpy_texture(ctx, dims,
                  dstFormat,
                  dstRowStride, dstSlices,
                  srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                  srcAddr, srcPacking);

   /* Check if we need byte swapping */
   /* XXX the logic here _might_ be wrong */
   if (srcPacking->SwapBytes ^
       (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA) ^
       (dstFormat == MESA_FORMAT_YCBCR_REV) ^
       !littleEndian) {
      GLint img, row;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            _mesa_swap2((GLushort *) dstRow, srcWidth);
            dstRow += dstRowStride;
         }
      }
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_dudv8(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_DUDV8);
   ASSERT(texelBytes == 2);
   ASSERT(ctx->Extensions.ATI_envmap_bumpmap);
   ASSERT((srcFormat == GL_DU8DV8_ATI) ||
	  (srcFormat == GL_DUDV_ATI));
   ASSERT(baseInternalFormat == GL_DUDV_ATI);

   if (!srcPacking->SwapBytes && srcType == GL_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (srcType == GL_BYTE) {
      GLubyte dstmap[4];

      /* dstmap - how to swizzle from RGBA to dst format:
       */
      if (littleEndian) {
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
				GL_LUMINANCE_ALPHA, /* hack */
				GL_UNSIGNED_BYTE, /* hack */
				GL_LUMINANCE_ALPHA, /* hack */
				dstmap, 2,
				dstRowStride, dstSlices,
				srcWidth, srcHeight, srcDepth, srcAddr,
				srcPacking);      
   }   
   else {
      /* general path - note this is defined for 2d textures only */
      const GLint components = _mesa_components_in_format(baseInternalFormat);
      const GLint srcStride = _mesa_image_row_stride(srcPacking, srcWidth,
                                                     srcFormat, srcType);
      GLbyte *tempImage, *dst, *src;
      GLint row;

      tempImage = (GLbyte *) malloc(srcWidth * srcHeight * srcDepth
                                          * components * sizeof(GLbyte));
      if (!tempImage)
         return GL_FALSE;

      src = (GLbyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                           srcWidth, srcHeight,
                                           srcFormat, srcType,
                                           0, 0, 0);

      dst = tempImage;
      for (row = 0; row < srcHeight; row++) {
         _mesa_unpack_dudv_span_byte(ctx, srcWidth, baseInternalFormat,
                                     dst, srcFormat, srcType, src,
                                     srcPacking, 0);
         dst += srcWidth * components;
         src += srcStride;
      }
 
      src = tempImage;
      dst = (GLbyte *) dstSlices[0];
      for (row = 0; row < srcHeight; row++) {
         memcpy(dst, src, srcWidth * texelBytes);
         dst += dstRowStride;
         src += srcWidth * texelBytes;
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a texture in a signed normalized 8-bit format.
 */
static GLboolean
_mesa_texstore_snorm8(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_A8 ||
          dstFormat == MESA_FORMAT_SIGNED_L8 ||
          dstFormat == MESA_FORMAT_SIGNED_I8 ||
          dstFormat == MESA_FORMAT_SIGNED_R8);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 1);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLbyte *dstRow = (GLbyte *) dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            for (col = 0; col < srcWidth; col++) {
               dstRow[col] = FLOAT_TO_BYTE_TEX(src[col]);
            }
            dstRow += dstRowStride;
            src += srcWidth;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a texture in a signed normalized two-channel 16-bit format.
 */
static GLboolean
_mesa_texstore_snorm88(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_AL88 ||
          dstFormat == MESA_FORMAT_SIGNED_RG88_REV);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_BYTE &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLbyte *dstRow = (GLbyte *) dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLbyte *dst = dstRow;
            for (col = 0; col < srcWidth; col++) {
               dst[0] = FLOAT_TO_BYTE_TEX(src[0]);
               dst[1] = FLOAT_TO_BYTE_TEX(src[1]);
               src += 2;
               dst += 2;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

/* Texstore for signed R16, A16, L16, I16. */
static GLboolean
_mesa_texstore_snorm16(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_R16 ||
          dstFormat == MESA_FORMAT_SIGNED_A16 ||
          dstFormat == MESA_FORMAT_SIGNED_L16 ||
          dstFormat == MESA_FORMAT_SIGNED_I16);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 2);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_SHORT &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLshort *dstUS = (GLshort *) dstRow;
	    for (col = 0; col < srcWidth; col++) {
	       GLushort r;

	       UNCLAMPED_FLOAT_TO_SHORT(r, src[0]);
	       dstUS[col] = r;
	       src += 1;
	    }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

/**
 * Do texstore for 2-channel, 16-bit/channel, signed normalized formats.
 */
static GLboolean
_mesa_texstore_snorm1616(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_AL1616 ||
          dstFormat == MESA_FORMAT_SIGNED_GR1616);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_SHORT &&
       littleEndian) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLshort *dst = (GLshort *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               GLushort l, a;

               UNCLAMPED_FLOAT_TO_SHORT(l, src[0]);
               UNCLAMPED_FLOAT_TO_SHORT(a, src[1]);
               dst[0] = l;
               dst[1] = a;
               src += 2;
               dst += 2;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

/**
 * Store a texture in MESA_FORMAT_SIGNED_RGBX8888.
 */
static GLboolean
_mesa_texstore_signed_rgbx8888(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RGBX8888);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *srcRow = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLbyte *dstRow = (GLbyte *) dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLbyte *dst = dstRow;
            for (col = 0; col < srcWidth; col++) {
               dst[3] = FLOAT_TO_BYTE_TEX(srcRow[RCOMP]);
               dst[2] = FLOAT_TO_BYTE_TEX(srcRow[GCOMP]);
               dst[1] = FLOAT_TO_BYTE_TEX(srcRow[BCOMP]);
               dst[0] = 127;
               srcRow += 3;
               dst += 4;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}



/**
 * Store a texture in MESA_FORMAT_SIGNED_RGBA8888 or
 * MESA_FORMAT_SIGNED_RGBA8888_REV
 */
static GLboolean
_mesa_texstore_signed_rgba8888(TEXSTORE_PARAMS)
{
   const GLboolean littleEndian = _mesa_little_endian();
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RGBA8888 ||
          dstFormat == MESA_FORMAT_SIGNED_RGBA8888_REV);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_SIGNED_RGBA8888 &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_BYTE && !littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_BYTE && littleEndian))) {
       /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_SIGNED_RGBA8888_REV &&
       baseInternalFormat == GL_RGBA &&
      ((srcFormat == GL_RGBA && srcType == GL_BYTE && littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_BYTE && !littleEndian))) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *srcRow = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLbyte *dstRow = (GLbyte *) dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLbyte *dst = dstRow;
            if (dstFormat == MESA_FORMAT_SIGNED_RGBA8888) {
               for (col = 0; col < srcWidth; col++) {
                  dst[3] = FLOAT_TO_BYTE_TEX(srcRow[RCOMP]);
                  dst[2] = FLOAT_TO_BYTE_TEX(srcRow[GCOMP]);
                  dst[1] = FLOAT_TO_BYTE_TEX(srcRow[BCOMP]);
                  dst[0] = FLOAT_TO_BYTE_TEX(srcRow[ACOMP]);
                  srcRow += 4;
                  dst += 4;
               }
            }
            else {
               for (col = 0; col < srcWidth; col++) {
                  dst[0] = FLOAT_TO_BYTE_TEX(srcRow[RCOMP]);
                  dst[1] = FLOAT_TO_BYTE_TEX(srcRow[GCOMP]);
                  dst[2] = FLOAT_TO_BYTE_TEX(srcRow[BCOMP]);
                  dst[3] = FLOAT_TO_BYTE_TEX(srcRow[ACOMP]);
                  srcRow += 4;
                  dst += 4;
               }
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}


/**
 * Store a combined depth/stencil texture image.
 */
static GLboolean
_mesa_texstore_z24_s8(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;
   const GLint srcRowStride
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLint img, row;

   ASSERT(dstFormat == MESA_FORMAT_Z24_S8);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT ||
          srcFormat == GL_DEPTH_COMPONENT ||
          srcFormat == GL_STENCIL_INDEX);
   ASSERT(srcFormat != GL_DEPTH_STENCIL_EXT || srcType == GL_UNSIGNED_INT_24_8_EXT);

   if (srcFormat == GL_DEPTH_STENCIL && ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes) {
      /* simple path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (srcFormat == GL_DEPTH_COMPONENT ||
            srcFormat == GL_STENCIL_INDEX) {
      /* In case we only upload depth we need to preserve the stencil */
      for (img = 0; img < srcDepth; img++) {
	 GLuint *dstRow = (GLuint *) dstSlices[img];
         const GLubyte *src
            = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                  srcWidth, srcHeight,
                  srcFormat, srcType,
                  img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLuint depth[MAX_WIDTH];
	    GLubyte stencil[MAX_WIDTH];
            GLint i;
	    GLboolean keepdepth = GL_FALSE, keepstencil = GL_FALSE;

	    if (srcFormat == GL_DEPTH_COMPONENT) { /* preserve stencil */
	       keepstencil = GL_TRUE;
	    }
            else if (srcFormat == GL_STENCIL_INDEX) { /* preserve depth */
	       keepdepth = GL_TRUE;
	    }

	    if (keepdepth == GL_FALSE)
	       /* the 24 depth bits will be in the low position: */
	       _mesa_unpack_depth_span(ctx, srcWidth,
				       GL_UNSIGNED_INT, /* dst type */
				       keepstencil ? depth : dstRow, /* dst addr */
				       depthScale,
				       srcType, src, srcPacking);

	    if (keepstencil == GL_FALSE)
	       /* get the 8-bit stencil values */
	       _mesa_unpack_stencil_span(ctx, srcWidth,
					 GL_UNSIGNED_BYTE, /* dst type */
					 stencil, /* dst addr */
					 srcType, src, srcPacking,
					 ctx->_ImageTransferState);

	    for (i = 0; i < srcWidth; i++) {
	       if (keepstencil)
		  dstRow[i] = depth[i] << 8 | (dstRow[i] & 0x000000FF);
	       else
		  dstRow[i] = (dstRow[i] & 0xFFFFFF00) | (stencil[i] & 0xFF);
	    }

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
static GLboolean
_mesa_texstore_s8_z24(TEXSTORE_PARAMS)
{
   const GLuint depthScale = 0xffffff;
   const GLint srcRowStride
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
   GLint img, row;

   ASSERT(dstFormat == MESA_FORMAT_S8_Z24);
   ASSERT(srcFormat == GL_DEPTH_STENCIL_EXT ||
          srcFormat == GL_DEPTH_COMPONENT ||
          srcFormat == GL_STENCIL_INDEX);
   ASSERT(srcFormat != GL_DEPTH_STENCIL_EXT ||
          srcType == GL_UNSIGNED_INT_24_8_EXT);

   for (img = 0; img < srcDepth; img++) {
      GLuint *dstRow = (GLuint *) dstSlices[img];
      const GLubyte *src
	 = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
						srcWidth, srcHeight,
						srcFormat, srcType,
						img, 0, 0);
      for (row = 0; row < srcHeight; row++) {
	 GLuint depth[MAX_WIDTH];
	 GLubyte stencil[MAX_WIDTH];
	 GLint i;
	 GLboolean keepdepth = GL_FALSE, keepstencil = GL_FALSE;
	 
	 if (srcFormat == GL_DEPTH_COMPONENT) { /* preserve stencil */
	    keepstencil = GL_TRUE;
	 }
         else if (srcFormat == GL_STENCIL_INDEX) { /* preserve depth */
	    keepdepth = GL_TRUE;
	 }

	 if (keepdepth == GL_FALSE)
	    /* the 24 depth bits will be in the low position: */
	    _mesa_unpack_depth_span(ctx, srcWidth,
				    GL_UNSIGNED_INT, /* dst type */
				    keepstencil ? depth : dstRow, /* dst addr */
				    depthScale,
				    srcType, src, srcPacking);	 

	 if (keepstencil == GL_FALSE)
	    /* get the 8-bit stencil values */
	    _mesa_unpack_stencil_span(ctx, srcWidth,
				      GL_UNSIGNED_BYTE, /* dst type */
				      stencil, /* dst addr */
				      srcType, src, srcPacking,
				      ctx->_ImageTransferState);

	 /* merge stencil values into depth values */
	 for (i = 0; i < srcWidth; i++) {
	    if (keepstencil)
	       dstRow[i] = depth[i] | (dstRow[i] & 0xFF000000);
	    else
	       dstRow[i] = (dstRow[i] & 0xFFFFFF) | (stencil[i] << 24);

	 }
	 src += srcRowStride;
	 dstRow += dstRowStride / sizeof(GLuint);
      }
   }
   return GL_TRUE;
}


/**
 * Store simple 8-bit/value stencil texture data.
 */
static GLboolean
_mesa_texstore_s8(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == MESA_FORMAT_S8);
   ASSERT(srcFormat == GL_STENCIL_INDEX);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      const GLint srcRowStride
	 = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType);
      GLint img, row;
      
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         const GLubyte *src
            = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                                   srcWidth, srcHeight,
                                                   srcFormat, srcType,
                                                   img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            GLubyte stencil[MAX_WIDTH];
            GLint i;

            /* get the 8-bit stencil values */
            _mesa_unpack_stencil_span(ctx, srcWidth,
                                      GL_UNSIGNED_BYTE, /* dst type */
                                      stencil, /* dst addr */
                                      srcType, src, srcPacking,
                                      ctx->_ImageTransferState);
            /* merge stencil values into depth values */
            for (i = 0; i < srcWidth; i++)
               dstRow[i] = stencil[i];

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(GLubyte);
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
static GLboolean
_mesa_texstore_rgba_float32(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA_FLOAT32 ||
          dstFormat == MESA_FORMAT_RGB_FLOAT32 ||
          dstFormat == MESA_FORMAT_ALPHA_FLOAT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_FLOAT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32 ||
          dstFormat == MESA_FORMAT_INTENSITY_FLOAT32 ||
          dstFormat == MESA_FORMAT_R_FLOAT32 ||
          dstFormat == MESA_FORMAT_RG_FLOAT32);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_RG);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLfloat));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       baseInternalFormat == baseFormat &&
       srcType == GL_FLOAT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *srcRow = tempImage;
      GLint bytesPerRow;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      bytesPerRow = srcWidth * components * sizeof(GLfloat);
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            memcpy(dstRow, srcRow, bytesPerRow);
            dstRow += dstRowStride;
            srcRow += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}



/**
 * As above, but store 16-bit floats.
 */
static GLboolean
_mesa_texstore_rgba_float16(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGBA_FLOAT16 ||
          dstFormat == MESA_FORMAT_RGB_FLOAT16 ||
          dstFormat == MESA_FORMAT_ALPHA_FLOAT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_FLOAT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16 ||
          dstFormat == MESA_FORMAT_INTENSITY_FLOAT16 ||
          dstFormat == MESA_FORMAT_R_FLOAT16 ||
          dstFormat == MESA_FORMAT_RG_FLOAT16);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_RG);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLhalfARB));

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       baseInternalFormat == baseFormat &&
       srcType == GL_HALF_FLOAT_ARB) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
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

      free((void *) tempImage);
   }
   return GL_TRUE;
}


/* non-normalized, signed int8 */
static GLboolean
_mesa_texstore_rgba_int8(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_R_INT8 ||
          dstFormat == MESA_FORMAT_RG_INT8 ||
          dstFormat == MESA_FORMAT_RGB_INT8 ||
          dstFormat == MESA_FORMAT_RGBA_INT8 ||
          dstFormat == MESA_FORMAT_ALPHA_INT8 ||
          dstFormat == MESA_FORMAT_INTENSITY_INT8 ||
          dstFormat == MESA_FORMAT_LUMINANCE_INT8 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_INT8);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_RG ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLbyte));

   /* Note: Pixel transfer ops (scale, bias, table lookup) do not apply
    * to integer formats.
    */
   if (!srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLuint *tempImage = make_temp_uint_image(ctx, dims,
						     baseInternalFormat,
						     baseFormat,
						     srcWidth, srcHeight, srcDepth,
						     srcFormat, srcType,
						     srcAddr,
						     srcPacking);
      const GLuint *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLbyte *dstTexel = (GLbyte *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = (GLbyte) src[i];
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


/* non-normalized, signed int16 */
static GLboolean
_mesa_texstore_rgba_int16(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_R_INT16 ||
          dstFormat == MESA_FORMAT_RG_INT16 ||
          dstFormat == MESA_FORMAT_RGB_INT16 ||
          dstFormat == MESA_FORMAT_RGBA_INT16 ||
          dstFormat == MESA_FORMAT_ALPHA_INT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_INT16 ||
          dstFormat == MESA_FORMAT_INTENSITY_INT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_INT16);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_RG ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLshort));

   /* Note: Pixel transfer ops (scale, bias, table lookup) do not apply
    * to integer formats.
    */
   if (!srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_SHORT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLuint *tempImage = make_temp_uint_image(ctx, dims,
						     baseInternalFormat,
						     baseFormat,
						     srcWidth, srcHeight, srcDepth,
						     srcFormat, srcType,
						     srcAddr,
						     srcPacking);
      const GLuint *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLshort *dstTexel = (GLshort *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = (GLint) src[i];
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


/* non-normalized, signed int32 */
static GLboolean
_mesa_texstore_rgba_int32(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_R_INT32 ||
          dstFormat == MESA_FORMAT_RG_INT32 ||
          dstFormat == MESA_FORMAT_RGB_INT32 ||
          dstFormat == MESA_FORMAT_RGBA_INT32 ||
          dstFormat == MESA_FORMAT_ALPHA_INT32 ||
          dstFormat == MESA_FORMAT_INTENSITY_INT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_INT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_INT32);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_RG ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLint));

   /* Note: Pixel transfer ops (scale, bias, table lookup) do not apply
    * to integer formats.
    */
   if (!srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_INT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLuint *tempImage = make_temp_uint_image(ctx, dims,
						     baseInternalFormat,
						     baseFormat,
						     srcWidth, srcHeight, srcDepth,
						     srcFormat, srcType,
						     srcAddr,
						     srcPacking);
      const GLuint *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLint *dstTexel = (GLint *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = (GLint) src[i];
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


/* non-normalized, unsigned int8 */
static GLboolean
_mesa_texstore_rgba_uint8(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_R_UINT8 ||
          dstFormat == MESA_FORMAT_RG_UINT8 ||
          dstFormat == MESA_FORMAT_RGB_UINT8 ||
          dstFormat == MESA_FORMAT_RGBA_UINT8 ||
          dstFormat == MESA_FORMAT_ALPHA_UINT8 ||
          dstFormat == MESA_FORMAT_INTENSITY_UINT8 ||
          dstFormat == MESA_FORMAT_LUMINANCE_UINT8 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_UINT8);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_RG ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLubyte));

   /* Note: Pixel transfer ops (scale, bias, table lookup) do not apply
    * to integer formats.
    */
   if (!srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_BYTE) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLuint *tempImage =
         make_temp_uint_image(ctx, dims, baseInternalFormat, baseFormat,
                              srcWidth, srcHeight, srcDepth,
                              srcFormat, srcType, srcAddr, srcPacking);
      const GLuint *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLubyte *dstTexel = (GLubyte *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = (GLubyte) CLAMP(src[i], 0, 0xff);
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


/* non-normalized, unsigned int16 */
static GLboolean
_mesa_texstore_rgba_uint16(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_R_UINT16 ||
          dstFormat == MESA_FORMAT_RG_UINT16 ||
          dstFormat == MESA_FORMAT_RGB_UINT16 ||
          dstFormat == MESA_FORMAT_RGBA_UINT16 ||
          dstFormat == MESA_FORMAT_ALPHA_UINT16 ||
          dstFormat == MESA_FORMAT_INTENSITY_UINT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_UINT16 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_UINT16);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_RG ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLushort));

   /* Note: Pixel transfer ops (scale, bias, table lookup) do not apply
    * to integer formats.
    */
   if (!srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_SHORT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLuint *tempImage =
         make_temp_uint_image(ctx, dims, baseInternalFormat, baseFormat,
                              srcWidth, srcHeight, srcDepth,
                              srcFormat, srcType, srcAddr, srcPacking);
      const GLuint *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLushort *dstTexel = (GLushort *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = (GLushort) CLAMP(src[i], 0, 0xffff);
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


/* non-normalized, unsigned int32 */
static GLboolean
_mesa_texstore_rgba_uint32(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);
   const GLint components = _mesa_components_in_format(baseFormat);

   ASSERT(dstFormat == MESA_FORMAT_R_UINT32 ||
          dstFormat == MESA_FORMAT_RG_UINT32 ||
          dstFormat == MESA_FORMAT_RGB_UINT32 ||
          dstFormat == MESA_FORMAT_RGBA_UINT32 ||
          dstFormat == MESA_FORMAT_ALPHA_UINT32 ||
          dstFormat == MESA_FORMAT_INTENSITY_UINT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_UINT32 ||
          dstFormat == MESA_FORMAT_LUMINANCE_ALPHA_UINT32);
   ASSERT(baseInternalFormat == GL_RGBA ||
          baseInternalFormat == GL_RGB ||
          baseInternalFormat == GL_RG ||
          baseInternalFormat == GL_RED ||
          baseInternalFormat == GL_ALPHA ||
          baseInternalFormat == GL_LUMINANCE ||
          baseInternalFormat == GL_LUMINANCE_ALPHA ||
          baseInternalFormat == GL_INTENSITY);
   ASSERT(_mesa_get_format_bytes(dstFormat) == components * sizeof(GLuint));

   /* Note: Pixel transfer ops (scale, bias, table lookup) do not apply
    * to integer formats.
    */
   if (!srcPacking->SwapBytes &&
       baseInternalFormat == srcFormat &&
       srcType == GL_UNSIGNED_INT) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLuint *tempImage =
         make_temp_uint_image(ctx, dims, baseInternalFormat, baseFormat,
                              srcWidth, srcHeight, srcDepth,
                              srcFormat, srcType, srcAddr, srcPacking);
      const GLuint *src = tempImage;
      GLint img, row;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstTexel = (GLuint *) dstRow;
            GLint i;
            for (i = 0; i < srcWidth * components; i++) {
               dstTexel[i] = src[i];
            }
            dstRow += dstRowStride;
            src += srcWidth * components;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}




#if FEATURE_EXT_texture_sRGB
static GLboolean
_mesa_texstore_srgb8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SRGB8);

   /* reuse normal rgb texstore code */
   newDstFormat = MESA_FORMAT_RGB888;

   k = _mesa_texstore_rgb888(ctx, dims, baseInternalFormat,
                             newDstFormat,
                             dstRowStride, dstSlices,
                             srcWidth, srcHeight, srcDepth,
                             srcFormat, srcType,
                             srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_srgba8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SRGBA8);

   /* reuse normal rgba texstore code */
   newDstFormat = MESA_FORMAT_RGBA8888;
   k = _mesa_texstore_rgba8888(ctx, dims, baseInternalFormat,
                               newDstFormat,
                               dstRowStride, dstSlices,
                               srcWidth, srcHeight, srcDepth,
                               srcFormat, srcType,
                               srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_sargb8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SARGB8);

   /* reuse normal rgba texstore code */
   newDstFormat = MESA_FORMAT_ARGB8888;

   k = _mesa_texstore_argb8888(ctx, dims, baseInternalFormat,
                               newDstFormat,
                               dstRowStride, dstSlices,
                               srcWidth, srcHeight, srcDepth,
                               srcFormat, srcType,
                               srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_sl8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SL8);

   newDstFormat = MESA_FORMAT_L8;

   /* _mesa_textore_a8 handles luminance8 too */
   k = _mesa_texstore_unorm8(ctx, dims, baseInternalFormat,
                             newDstFormat,
                             dstRowStride, dstSlices,
                             srcWidth, srcHeight, srcDepth,
                             srcFormat, srcType,
                             srcAddr, srcPacking);
   return k;
}


static GLboolean
_mesa_texstore_sla8(TEXSTORE_PARAMS)
{
   gl_format newDstFormat;
   GLboolean k;

   ASSERT(dstFormat == MESA_FORMAT_SLA8);

   /* reuse normal luminance/alpha texstore code */
   newDstFormat = MESA_FORMAT_AL88;

   k = _mesa_texstore_unorm88(ctx, dims, baseInternalFormat,
			      newDstFormat,
			      dstRowStride, dstSlices,
			      srcWidth, srcHeight, srcDepth,
			      srcFormat, srcType,
			      srcAddr, srcPacking);
   return k;
}

#else

/* these are used only in texstore_funcs[] below */
#define _mesa_texstore_srgb8 NULL
#define _mesa_texstore_srgba8 NULL
#define _mesa_texstore_sargb8 NULL
#define _mesa_texstore_sl8 NULL
#define _mesa_texstore_sla8 NULL

#endif /* FEATURE_EXT_texture_sRGB */

static GLboolean
_mesa_texstore_rgb9_e5(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_RGB9_E5_FLOAT);
   ASSERT(baseInternalFormat == GL_RGB);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_INT_5_9_9_9_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *srcRow = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint*)dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUI[col] = float3_to_rgb9e5(&srcRow[col * 3]);
            }
            dstRow += dstRowStride;
            srcRow += srcWidth * 3;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_r11_g11_b10f(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_R11_G11_B10_FLOAT);
   ASSERT(baseInternalFormat == GL_RGB);

   if (!ctx->_ImageTransferState &&
       !srcPacking->SwapBytes &&
       srcFormat == GL_RGB &&
       srcType == GL_UNSIGNED_INT_10F_11F_11F_REV) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLfloat *tempImage = _mesa_make_temp_float_image(ctx, dims,
                                                 baseInternalFormat,
                                                 baseFormat,
                                                 srcWidth, srcHeight, srcDepth,
                                                 srcFormat, srcType, srcAddr,
                                                 srcPacking,
                                                 ctx->_ImageTransferState);
      const GLfloat *srcRow = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];
         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint*)dstRow;
            for (col = 0; col < srcWidth; col++) {
               dstUI[col] = float3_to_r11g11b10f(&srcRow[col * 3]);
            }
            dstRow += dstRowStride;
            srcRow += srcWidth * 3;
         }
      }

      free((void *) tempImage);
   }
   return GL_TRUE;
}


static GLboolean
_mesa_texstore_z32f_x24s8(TEXSTORE_PARAMS)
{
   ASSERT(dstFormat == MESA_FORMAT_Z32_FLOAT_X24S8);
   ASSERT(srcFormat == GL_DEPTH_STENCIL ||
          srcFormat == GL_DEPTH_COMPONENT ||
          srcFormat == GL_STENCIL_INDEX);
   ASSERT(srcFormat != GL_DEPTH_STENCIL ||
          srcType == GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

   if (srcFormat == GL_DEPTH_STENCIL &&
       ctx->Pixel.DepthScale == 1.0f &&
       ctx->Pixel.DepthBias == 0.0f &&
       !srcPacking->SwapBytes) {
      /* simple path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else if (srcFormat == GL_DEPTH_COMPONENT ||
            srcFormat == GL_STENCIL_INDEX) {
      GLint img, row;
      const GLint srcRowStride
         = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
         / sizeof(uint64_t);

      /* In case we only upload depth we need to preserve the stencil */
      for (img = 0; img < srcDepth; img++) {
         uint64_t *dstRow = (uint64_t *) dstSlices[img];
         const uint64_t *src
            = (const uint64_t *) _mesa_image_address(dims, srcPacking, srcAddr,
                  srcWidth, srcHeight,
                  srcFormat, srcType,
                  img, 0, 0);
         for (row = 0; row < srcHeight; row++) {
            /* The unpack functions with:
             *    dstType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV
             * only write their own dword, so the other dword (stencil
             * or depth) is preserved. */
            if (srcFormat != GL_STENCIL_INDEX)
               _mesa_unpack_depth_span(ctx, srcWidth,
                                       GL_FLOAT_32_UNSIGNED_INT_24_8_REV, /* dst type */
                                       dstRow, /* dst addr */
                                       ~0U, srcType, src, srcPacking);

            if (srcFormat != GL_DEPTH_COMPONENT)
               _mesa_unpack_stencil_span(ctx, srcWidth,
                                         GL_FLOAT_32_UNSIGNED_INT_24_8_REV, /* dst type */
                                         dstRow, /* dst addr */
                                         srcType, src, srcPacking,
                                         ctx->_ImageTransferState);

            src += srcRowStride;
            dstRow += dstRowStride / sizeof(uint64_t);
         }
      }
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_argb2101010_uint(TEXSTORE_PARAMS)
{
   const GLenum baseFormat = _mesa_get_format_base_format(dstFormat);

   ASSERT(dstFormat == MESA_FORMAT_ARGB2101010_UINT);
   ASSERT(_mesa_get_format_bytes(dstFormat) == 4);

   if (!srcPacking->SwapBytes &&
       dstFormat == MESA_FORMAT_ARGB2101010_UINT &&
       srcFormat == GL_BGRA_INTEGER_EXT &&
       srcType == GL_UNSIGNED_INT_2_10_10_10_REV &&
       baseInternalFormat == GL_RGBA) {
      /* simple memcpy path */
      memcpy_texture(ctx, dims,
                     dstFormat,
                     dstRowStride, dstSlices,
                     srcWidth, srcHeight, srcDepth, srcFormat, srcType,
                     srcAddr, srcPacking);
   }
   else {
      /* general path */
      const GLuint *tempImage = make_temp_uint_image(ctx, dims,
                                                     baseInternalFormat,
                                                     baseFormat,
                                                     srcWidth, srcHeight,
                                                     srcDepth, srcFormat,
                                                     srcType, srcAddr,
                                                     srcPacking);
      const GLuint *src = tempImage;
      GLint img, row, col;
      if (!tempImage)
         return GL_FALSE;
      for (img = 0; img < srcDepth; img++) {
         GLubyte *dstRow = dstSlices[img];

         for (row = 0; row < srcHeight; row++) {
            GLuint *dstUI = (GLuint *) dstRow;
            for (col = 0; col < srcWidth; col++) {
               GLushort a,r,g,b;
               r = src[RCOMP];
               g = src[GCOMP];
               b = src[BCOMP];
               a = src[ACOMP];
               dstUI[col] = (a << 30) | (r << 20) | (g << 10) | (b);
               src += 4;
            }
            dstRow += dstRowStride;
         }
      }
      free((void *) tempImage);
   }
   return GL_TRUE;
}

static GLboolean
_mesa_texstore_null(TEXSTORE_PARAMS)
{
   (void) ctx; (void) dims;
   (void) baseInternalFormat;
   (void) dstFormat;
   (void) dstRowStride; (void) dstSlices,
   (void) srcWidth; (void) srcHeight; (void) srcDepth;
   (void) srcFormat; (void) srcType;
   (void) srcAddr;
   (void) srcPacking;

   /* should never happen */
   _mesa_problem(NULL, "_mesa_texstore_null() is called");
   return GL_FALSE;
}


/**
 * Return the StoreTexImageFunc pointer to store an image in the given format.
 */
static StoreTexImageFunc
_mesa_get_texstore_func(gl_format format)
{
   static StoreTexImageFunc table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      table[MESA_FORMAT_NONE] = _mesa_texstore_null;

      table[MESA_FORMAT_RGBA8888] = _mesa_texstore_rgba8888;
      table[MESA_FORMAT_RGBA8888_REV] = _mesa_texstore_rgba8888;
      table[MESA_FORMAT_ARGB8888] = _mesa_texstore_argb8888;
      table[MESA_FORMAT_ARGB8888_REV] = _mesa_texstore_argb8888;
      table[MESA_FORMAT_RGBX8888] = _mesa_texstore_rgba8888;
      table[MESA_FORMAT_RGBX8888_REV] = _mesa_texstore_rgba8888;
      table[MESA_FORMAT_XRGB8888] = _mesa_texstore_argb8888;
      table[MESA_FORMAT_XRGB8888_REV] = _mesa_texstore_argb8888;
      table[MESA_FORMAT_RGB888] = _mesa_texstore_rgb888;
      table[MESA_FORMAT_BGR888] = _mesa_texstore_bgr888;
      table[MESA_FORMAT_RGB565] = _mesa_texstore_rgb565;
      table[MESA_FORMAT_RGB565_REV] = _mesa_texstore_rgb565;
      table[MESA_FORMAT_ARGB4444] = _mesa_texstore_argb4444;
      table[MESA_FORMAT_ARGB4444_REV] = _mesa_texstore_argb4444;
      table[MESA_FORMAT_RGBA5551] = _mesa_texstore_rgba5551;
      table[MESA_FORMAT_ARGB1555] = _mesa_texstore_argb1555;
      table[MESA_FORMAT_ARGB1555_REV] = _mesa_texstore_argb1555;
      table[MESA_FORMAT_AL44] = _mesa_texstore_unorm44;
      table[MESA_FORMAT_AL88] = _mesa_texstore_unorm88;
      table[MESA_FORMAT_AL88_REV] = _mesa_texstore_unorm88;
      table[MESA_FORMAT_AL1616] = _mesa_texstore_unorm1616;
      table[MESA_FORMAT_AL1616_REV] = _mesa_texstore_unorm1616;
      table[MESA_FORMAT_RGB332] = _mesa_texstore_rgb332;
      table[MESA_FORMAT_A8] = _mesa_texstore_unorm8;
      table[MESA_FORMAT_A16] = _mesa_texstore_unorm16;
      table[MESA_FORMAT_L8] = _mesa_texstore_unorm8;
      table[MESA_FORMAT_L16] = _mesa_texstore_unorm16;
      table[MESA_FORMAT_I8] = _mesa_texstore_unorm8;
      table[MESA_FORMAT_I16] = _mesa_texstore_unorm16;
      table[MESA_FORMAT_YCBCR] = _mesa_texstore_ycbcr;
      table[MESA_FORMAT_YCBCR_REV] = _mesa_texstore_ycbcr;
      table[MESA_FORMAT_R8] = _mesa_texstore_unorm8;
      table[MESA_FORMAT_GR88] = _mesa_texstore_unorm88;
      table[MESA_FORMAT_RG88] = _mesa_texstore_unorm88;
      table[MESA_FORMAT_R16] = _mesa_texstore_unorm16;
      table[MESA_FORMAT_RG1616] = _mesa_texstore_unorm1616;
      table[MESA_FORMAT_RG1616_REV] = _mesa_texstore_unorm1616;
      table[MESA_FORMAT_ARGB2101010] = _mesa_texstore_argb2101010;
      table[MESA_FORMAT_Z24_S8] = _mesa_texstore_z24_s8;
      table[MESA_FORMAT_S8_Z24] = _mesa_texstore_s8_z24;
      table[MESA_FORMAT_Z16] = _mesa_texstore_z16;
      table[MESA_FORMAT_X8_Z24] = _mesa_texstore_x8_z24;
      table[MESA_FORMAT_Z24_X8] = _mesa_texstore_z24_x8;
      table[MESA_FORMAT_Z32] = _mesa_texstore_z32;
      table[MESA_FORMAT_S8] = _mesa_texstore_s8;
      table[MESA_FORMAT_SRGB8] = _mesa_texstore_srgb8;
      table[MESA_FORMAT_SRGBA8] = _mesa_texstore_srgba8;
      table[MESA_FORMAT_SARGB8] = _mesa_texstore_sargb8;
      table[MESA_FORMAT_SL8] = _mesa_texstore_sl8;
      table[MESA_FORMAT_SLA8] = _mesa_texstore_sla8;
      table[MESA_FORMAT_SRGB_DXT1] = _mesa_texstore_rgb_dxt1;
      table[MESA_FORMAT_SRGBA_DXT1] = _mesa_texstore_rgba_dxt1;
      table[MESA_FORMAT_SRGBA_DXT3] = _mesa_texstore_rgba_dxt3;
      table[MESA_FORMAT_SRGBA_DXT5] = _mesa_texstore_rgba_dxt5;
      table[MESA_FORMAT_RGB_FXT1] = _mesa_texstore_rgb_fxt1;
      table[MESA_FORMAT_RGBA_FXT1] = _mesa_texstore_rgba_fxt1;
      table[MESA_FORMAT_RGB_DXT1] = _mesa_texstore_rgb_dxt1;
      table[MESA_FORMAT_RGBA_DXT1] = _mesa_texstore_rgba_dxt1;
      table[MESA_FORMAT_RGBA_DXT3] = _mesa_texstore_rgba_dxt3;
      table[MESA_FORMAT_RGBA_DXT5] = _mesa_texstore_rgba_dxt5;
      table[MESA_FORMAT_RGBA_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_RGBA_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_RGB_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_RGB_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_ALPHA_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_ALPHA_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_LUMINANCE_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_LUMINANCE_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_INTENSITY_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_INTENSITY_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_R_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_R_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_RG_FLOAT32] = _mesa_texstore_rgba_float32;
      table[MESA_FORMAT_RG_FLOAT16] = _mesa_texstore_rgba_float16;
      table[MESA_FORMAT_DUDV8] = _mesa_texstore_dudv8;
      table[MESA_FORMAT_SIGNED_R8] = _mesa_texstore_snorm8;
      table[MESA_FORMAT_SIGNED_RG88_REV] = _mesa_texstore_snorm88;
      table[MESA_FORMAT_SIGNED_RGBX8888] = _mesa_texstore_signed_rgbx8888;
      table[MESA_FORMAT_SIGNED_RGBA8888] = _mesa_texstore_signed_rgba8888;
      table[MESA_FORMAT_SIGNED_RGBA8888_REV] = _mesa_texstore_signed_rgba8888;
      table[MESA_FORMAT_SIGNED_R16] = _mesa_texstore_snorm16;
      table[MESA_FORMAT_SIGNED_GR1616] = _mesa_texstore_snorm1616;
      table[MESA_FORMAT_SIGNED_RGB_16] = _mesa_texstore_signed_rgba_16;
      table[MESA_FORMAT_SIGNED_RGBA_16] = _mesa_texstore_signed_rgba_16;
      table[MESA_FORMAT_RGBA_16] = _mesa_texstore_rgba_16;
      table[MESA_FORMAT_RED_RGTC1] = _mesa_texstore_red_rgtc1;
      table[MESA_FORMAT_SIGNED_RED_RGTC1] = _mesa_texstore_signed_red_rgtc1;
      table[MESA_FORMAT_RG_RGTC2] = _mesa_texstore_rg_rgtc2;
      table[MESA_FORMAT_SIGNED_RG_RGTC2] = _mesa_texstore_signed_rg_rgtc2;
      table[MESA_FORMAT_L_LATC1] = _mesa_texstore_red_rgtc1;
      table[MESA_FORMAT_SIGNED_L_LATC1] = _mesa_texstore_signed_red_rgtc1;
      table[MESA_FORMAT_LA_LATC2] = _mesa_texstore_rg_rgtc2;
      table[MESA_FORMAT_SIGNED_LA_LATC2] = _mesa_texstore_signed_rg_rgtc2;
      table[MESA_FORMAT_ETC1_RGB8] = _mesa_texstore_etc1_rgb8;
      table[MESA_FORMAT_SIGNED_A8] = _mesa_texstore_snorm8;
      table[MESA_FORMAT_SIGNED_L8] = _mesa_texstore_snorm8;
      table[MESA_FORMAT_SIGNED_AL88] = _mesa_texstore_snorm88;
      table[MESA_FORMAT_SIGNED_I8] = _mesa_texstore_snorm8;
      table[MESA_FORMAT_SIGNED_A16] = _mesa_texstore_snorm16;
      table[MESA_FORMAT_SIGNED_L16] = _mesa_texstore_snorm16;
      table[MESA_FORMAT_SIGNED_AL1616] = _mesa_texstore_snorm1616;
      table[MESA_FORMAT_SIGNED_I16] = _mesa_texstore_snorm16;
      table[MESA_FORMAT_RGB9_E5_FLOAT] = _mesa_texstore_rgb9_e5;
      table[MESA_FORMAT_R11_G11_B10_FLOAT] = _mesa_texstore_r11_g11_b10f;
      table[MESA_FORMAT_Z32_FLOAT] = _mesa_texstore_z32;
      table[MESA_FORMAT_Z32_FLOAT_X24S8] = _mesa_texstore_z32f_x24s8;

      table[MESA_FORMAT_ALPHA_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_ALPHA_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_ALPHA_UINT32] = _mesa_texstore_rgba_uint32;
      table[MESA_FORMAT_ALPHA_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_ALPHA_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_ALPHA_INT32] = _mesa_texstore_rgba_int32;

      table[MESA_FORMAT_INTENSITY_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_INTENSITY_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_INTENSITY_UINT32] = _mesa_texstore_rgba_uint32;
      table[MESA_FORMAT_INTENSITY_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_INTENSITY_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_INTENSITY_INT32] = _mesa_texstore_rgba_int32;

      table[MESA_FORMAT_LUMINANCE_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_LUMINANCE_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_LUMINANCE_UINT32] = _mesa_texstore_rgba_uint32;
      table[MESA_FORMAT_LUMINANCE_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_LUMINANCE_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_LUMINANCE_INT32] = _mesa_texstore_rgba_int32;

      table[MESA_FORMAT_LUMINANCE_ALPHA_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_LUMINANCE_ALPHA_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_LUMINANCE_ALPHA_UINT32] = _mesa_texstore_rgba_uint32;
      table[MESA_FORMAT_LUMINANCE_ALPHA_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_LUMINANCE_ALPHA_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_LUMINANCE_ALPHA_INT32] = _mesa_texstore_rgba_int32;

      table[MESA_FORMAT_R_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_RG_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_RGB_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_RGBA_INT8] = _mesa_texstore_rgba_int8;
      table[MESA_FORMAT_R_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_RG_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_RGB_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_RGBA_INT16] = _mesa_texstore_rgba_int16;
      table[MESA_FORMAT_R_INT32] = _mesa_texstore_rgba_int32;
      table[MESA_FORMAT_RG_INT32] = _mesa_texstore_rgba_int32;
      table[MESA_FORMAT_RGB_INT32] = _mesa_texstore_rgba_int32;
      table[MESA_FORMAT_RGBA_INT32] = _mesa_texstore_rgba_int32;

      table[MESA_FORMAT_R_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_RG_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_RGB_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_RGBA_UINT8] = _mesa_texstore_rgba_uint8;
      table[MESA_FORMAT_R_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_RG_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_RGB_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_RGBA_UINT16] = _mesa_texstore_rgba_uint16;
      table[MESA_FORMAT_R_UINT32] = _mesa_texstore_rgba_uint32;
      table[MESA_FORMAT_RG_UINT32] = _mesa_texstore_rgba_uint32;
      table[MESA_FORMAT_RGB_UINT32] = _mesa_texstore_rgba_uint32;
      table[MESA_FORMAT_RGBA_UINT32] = _mesa_texstore_rgba_uint32;

      table[MESA_FORMAT_ARGB2101010_UINT] = _mesa_texstore_argb2101010_uint;
      initialized = GL_TRUE;
   }

   ASSERT(table[format]);
   return table[format];
}


/**
 * Store user data into texture memory.
 * Called via glTex[Sub]Image1/2/3D()
 */
GLboolean
_mesa_texstore(TEXSTORE_PARAMS)
{
   StoreTexImageFunc storeImage;
   GLboolean success;

   storeImage = _mesa_get_texstore_func(dstFormat);

   success = storeImage(ctx, dims, baseInternalFormat,
                        dstFormat,
                        dstRowStride, dstSlices,
                        srcWidth, srcHeight, srcDepth,
                        srcFormat, srcType, srcAddr, srcPacking);
   return success;
}


/**
 * Normally, we'll only _write_ texel data to a texture when we map it.
 * But if the user is providing depth or stencil values and the texture
 * image is a combined depth/stencil format, we'll actually read from
 * the texture buffer too (in order to insert the depth or stencil values.
 * \param userFormat  the user-provided image format
 * \param texFormat  the destination texture format
 */
static GLbitfield
get_read_write_mode(GLenum userFormat, gl_format texFormat)
{
   if ((userFormat == GL_STENCIL_INDEX || userFormat == GL_DEPTH_COMPONENT)
       && _mesa_get_format_base_format(texFormat) == GL_DEPTH_STENCIL)
      return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
   else
      return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;
}


/**
 * Helper function for storing 1D, 2D, 3D whole and subimages into texture
 * memory.
 * The source of the image data may be user memory or a PBO.  In the later
 * case, we'll map the PBO, copy from it, then unmap it.
 */
static void
store_texsubimage(struct gl_context *ctx,
                  struct gl_texture_image *texImage,
                  GLint xoffset, GLint yoffset, GLint zoffset,
                  GLint width, GLint height, GLint depth,
                  GLenum format, GLenum type, const GLvoid *pixels,
                  const struct gl_pixelstore_attrib *packing,
                  const char *caller)

{
   const GLbitfield mapMode = get_read_write_mode(format, texImage->TexFormat);
   const GLenum target = texImage->TexObject->Target;
   GLboolean success = GL_FALSE;
   GLuint dims, slice, numSlices = 1, sliceOffset = 0;
   GLint srcImageStride = 0;
   const GLubyte *src;

   assert(xoffset + width <= texImage->Width);
   assert(yoffset + height <= texImage->Height);
   assert(zoffset + depth <= texImage->Depth);

   switch (target) {
   case GL_TEXTURE_1D:
      dims = 1;
      break;
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_3D:
      dims = 3;
      break;
   default:
      dims = 2;
   }

   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   src = (const GLubyte *)
      _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
                                  format, type, pixels, packing, caller);
   if (!src)
      return;

   /* compute slice info (and do some sanity checks) */
   switch (target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
      /* one image slice, nothing special needs to be done */
      break;
   case GL_TEXTURE_1D:
      assert(height == 1);
      assert(depth == 1);
      assert(yoffset == 0);
      assert(zoffset == 0);
      break;
   case GL_TEXTURE_1D_ARRAY:
      assert(depth == 1);
      assert(zoffset == 0);
      numSlices = height;
      sliceOffset = yoffset;
      height = 1;
      yoffset = 0;
      srcImageStride = _mesa_image_row_stride(packing, width, format, type);
      break;
   case GL_TEXTURE_2D_ARRAY:
      numSlices = depth;
      sliceOffset = zoffset;
      depth = 1;
      zoffset = 0;
      srcImageStride = _mesa_image_image_stride(packing, width, height,
                                                format, type);
      break;
   case GL_TEXTURE_3D:
      /* we'll store 3D images as a series of slices */
      numSlices = depth;
      sliceOffset = zoffset;
      srcImageStride = _mesa_image_image_stride(packing, width, height,
                                                format, type);
      break;
   default:
      _mesa_warning(ctx, "Unexpected target 0x%x in store_texsubimage()", target);
      return;
   }

   assert(numSlices == 1 || srcImageStride != 0);

   for (slice = 0; slice < numSlices; slice++) {
      GLubyte *dstMap;
      GLint dstRowStride;

      ctx->Driver.MapTextureImage(ctx, texImage,
                                  slice + sliceOffset,
                                  xoffset, yoffset, width, height,
                                  mapMode, &dstMap, &dstRowStride);
      if (dstMap) {
         /* Note: we're only storing a 2D (or 1D) slice at a time but we need
          * to pass the right 'dims' value so that GL_UNPACK_SKIP_IMAGES is
          * used for 3D images.
          */
         success = _mesa_texstore(ctx, dims, texImage->_BaseFormat,
                                  texImage->TexFormat,
                                  dstRowStride,
                                  &dstMap,
                                  width, height, 1,  /* w, h, d */
                                  format, type, src, packing);

         ctx->Driver.UnmapTextureImage(ctx, texImage, slice + sliceOffset);
      }

      src += srcImageStride;

      if (!success)
         break;
   }

   if (!success)
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);

   _mesa_unmap_teximage_pbo(ctx, packing);
}



/**
 * This is the fallback for Driver.TexImage1D().
 */
void
_mesa_store_teximage1d(struct gl_context *ctx,
                       struct gl_texture_image *texImage,
                       GLint internalFormat,
                       GLint width, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing)
{
   if (width == 0)
      return;

   /* allocate storage for texture data */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage, texImage->TexFormat,
                                            width, 1, 1)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
      return;
   }

   store_texsubimage(ctx, texImage,
                     0, 0, 0, width, 1, 1,
                     format, type, pixels, packing, "glTexImage1D");
}


/**
 * This is the fallback for Driver.TexImage2D().
 */
void
_mesa_store_teximage2d(struct gl_context *ctx,
                       struct gl_texture_image *texImage,
                       GLint internalFormat,
                       GLint width, GLint height, GLint border,
                       GLenum format, GLenum type, const void *pixels,
                       const struct gl_pixelstore_attrib *packing)
{
   if (width == 0 || height == 0)
      return;

   /* allocate storage for texture data */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage, texImage->TexFormat,
                                            width, height, 1)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      return;
   }

   store_texsubimage(ctx, texImage,
                     0, 0, 0, width, height, 1,
                     format, type, pixels, packing, "glTexImage2D");
}



/**
 * This is the fallback for Driver.TexImage3D().
 */
void
_mesa_store_teximage3d(struct gl_context *ctx,
                       struct gl_texture_image *texImage,
                       GLint internalFormat,
                       GLint width, GLint height, GLint depth, GLint border,
                       GLenum format, GLenum type, const void *pixels,
                       const struct gl_pixelstore_attrib *packing)
{
   if (width == 0 || height == 0 || depth == 0)
      return;

   /* allocate storage for texture data */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage, texImage->TexFormat,
                                            width, height, depth)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
      return;
   }

   store_texsubimage(ctx, texImage,
                     0, 0, 0, width, height, depth,
                     format, type, pixels, packing, "glTexImage3D");
}




/*
 * This is the fallback for Driver.TexSubImage1D().
 */
void
_mesa_store_texsubimage1d(struct gl_context *ctx,
                          struct gl_texture_image *texImage,
                          GLint xoffset, GLint width,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing)
{
   store_texsubimage(ctx, texImage,
                     xoffset, 0, 0, width, 1, 1,
                     format, type, pixels, packing, "glTexSubImage1D");
}



/**
 * This is the fallback for Driver.TexSubImage2D().
 */
void
_mesa_store_texsubimage2d(struct gl_context *ctx,
                          struct gl_texture_image *texImage,
                          GLint xoffset, GLint yoffset,
                          GLint width, GLint height,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing)
{
   store_texsubimage(ctx, texImage,
                     xoffset, yoffset, 0, width, height, 1,
                     format, type, pixels, packing, "glTexSubImage2D");
}


/*
 * This is the fallback for Driver.TexSubImage3D().
 */
void
_mesa_store_texsubimage3d(struct gl_context *ctx,
                          struct gl_texture_image *texImage,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLint width, GLint height, GLint depth,
                          GLenum format, GLenum type, const void *pixels,
                          const struct gl_pixelstore_attrib *packing)
{
   store_texsubimage(ctx, texImage,
                     xoffset, yoffset, zoffset, width, height, depth,
                     format, type, pixels, packing, "glTexSubImage3D");
}


/*
 * Fallback for Driver.CompressedTexImage1D()
 */
void
_mesa_store_compressed_teximage1d(struct gl_context *ctx,
                                  struct gl_texture_image *texImage,
                                  GLint internalFormat,
                                  GLint width, GLint border,
                                  GLsizei imageSize, const GLvoid *data)
{
   /* no compressed 1D image formats at this time */
   (void) ctx;
   (void) internalFormat;
   (void) width; (void) border;
   (void) imageSize; (void) data;
   (void) texImage;
}



/**
 * Fallback for Driver.CompressedTexImage2D()
 */
void
_mesa_store_compressed_teximage2d(struct gl_context *ctx,
                                  struct gl_texture_image *texImage,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint border,
                                  GLsizei imageSize, const GLvoid *data)
{
   /* This is pretty simple, because unlike the general texstore path we don't
    * have to worry about the usual image unpacking or image transfer
    * operations.
    */
   ASSERT(texImage);
   ASSERT(texImage->Width > 0);
   ASSERT(texImage->Height > 0);
   ASSERT(texImage->Depth == 1);

   /* allocate storage for texture data */
   if (!ctx->Driver.AllocTextureImageBuffer(ctx, texImage, texImage->TexFormat,
                                            width, height, 1)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
      return;
   }

   _mesa_store_compressed_texsubimage2d(ctx, texImage,
					0, 0,
					width, height,
					texImage->TexFormat,
					imageSize, data);
}



/*
 * Fallback for Driver.CompressedTexImage3D()
 */
void
_mesa_store_compressed_teximage3d(struct gl_context *ctx,
                                  struct gl_texture_image *texImage,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint depth,
                                  GLint border,
                                  GLsizei imageSize, const GLvoid *data)
{
   /* this space intentionally left blank */
   (void) ctx;
   (void) internalFormat;
   (void) width; (void) height; (void) depth;
   (void) border;
   (void) imageSize; (void) data;
   (void) texImage;
}



/**
 * Fallback for Driver.CompressedTexSubImage1D()
 */
void
_mesa_store_compressed_texsubimage1d(struct gl_context *ctx,
                                     struct gl_texture_image *texImage,
                                     GLint xoffset, GLsizei width,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data)
{
   /* there are no compressed 1D texture formats yet */
   (void) ctx;
   (void) xoffset; (void) width;
   (void) format;
   (void) imageSize; (void) data;
   (void) texImage;
}


/**
 * Fallback for Driver.CompressedTexSubImage2D()
 */
void
_mesa_store_compressed_texsubimage2d(struct gl_context *ctx,
                                     struct gl_texture_image *texImage,
                                     GLint xoffset, GLint yoffset,
                                     GLsizei width, GLsizei height,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data)
{
   GLint bytesPerRow, dstRowStride, srcRowStride;
   GLint i, rows;
   GLubyte *dstMap;
   const GLubyte *src;
   const gl_format texFormat = texImage->TexFormat;
   GLuint bw, bh;

   _mesa_get_format_block_size(texFormat, &bw, &bh);

   /* these should have been caught sooner */
   ASSERT((width % bw) == 0 || width < bw);
   ASSERT((height % bh) == 0 || height < bh);
   ASSERT((xoffset % bw) == 0);
   ASSERT((yoffset % bh) == 0);

   /* get pointer to src pixels (may be in a pbo which we'll map here) */
   data = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, data,
                                                 &ctx->Unpack,
                                                 "glCompressedTexSubImage2D");
   if (!data)
      return;

   srcRowStride = _mesa_format_row_stride(texFormat, width);
   src = (const GLubyte *) data;

   /* Map dest texture buffer */
   ctx->Driver.MapTextureImage(ctx, texImage, 0,
                               xoffset, yoffset, width, height,
                               GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT,
                               &dstMap, &dstRowStride);

   if (dstMap) {
      bytesPerRow = srcRowStride;  /* bytes per row of blocks */
      rows = (height + bh - 1) / bh;  /* rows in blocks */

      /* copy rows of blocks */
      for (i = 0; i < rows; i++) {
         memcpy(dstMap, src, bytesPerRow);
         dstMap += dstRowStride;
         src += srcRowStride;
      }

      ctx->Driver.UnmapTextureImage(ctx, texImage, 0);
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage2D");
   }

   _mesa_unmap_teximage_pbo(ctx, &ctx->Unpack);
}


/**
 * Fallback for Driver.CompressedTexSubImage3D()
 */
void
_mesa_store_compressed_texsubimage3d(struct gl_context *ctx,
                                     struct gl_texture_image *texImage,
                                     GLint xoffset, GLint yoffset, GLint zoffset,
                                     GLsizei width, GLsizei height, GLsizei depth,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data)
{
   /* there are no compressed 3D texture formats yet */
   (void) ctx;
   (void) xoffset; (void) yoffset; (void) zoffset;
   (void) width; (void) height; (void) depth;
   (void) format;
   (void) imageSize; (void) data;
   (void) texImage;
}
