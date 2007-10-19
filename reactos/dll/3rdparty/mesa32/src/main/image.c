/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


/**
 * \file image.c
 * Image handling.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "imports.h"
#include "histogram.h"
#include "macros.h"
#include "pixel.h"
#include "mtypes.h"


/** Compute ceiling of integer quotient of A divided by B. */
#define CEILING( A, B )  ( (A) % (B) == 0 ? (A)/(B) : (A)/(B)+1 )


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
   register GLuint i, a, b;

   for (i=0;i<n;i++) {
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


/**
 * Flip the order of the 2 bytes in each word in the given array.
 *
 * \param p array.
 * \param n number of words.
 */
void
_mesa_swap2( GLushort *p, GLuint n )
{
   register GLuint i;

   for (i=0;i<n;i++) {
      p[i] = (p[i] >> 8) | ((p[i] << 8) & 0xff00);
   }
}



/*
 * Flip the order of the 4 bytes in each word in the given array.
 */
void
_mesa_swap4( GLuint *p, GLuint n )
{
   register GLuint i, a, b;

   for (i=0;i<n;i++) {
      b = p[i];
      a =  (b >> 24)
	| ((b >> 8) & 0xff00)
	| ((b << 8) & 0xff0000)
	| ((b << 24) & 0xff000000);
      p[i] = a;
   }
}


/**
 * Get the size of a GL data type.
 *
 * \param type GL data type.
 *
 * \return the size, in bytes, of the given data type, 0 if a GL_BITMAP, or -1
 * if an invalid type enum.
 */
GLint _mesa_sizeof_type( GLenum type )
{
   switch (type) {
      case GL_BITMAP:
	 return 0;
      case GL_UNSIGNED_BYTE:
         return sizeof(GLubyte);
      case GL_BYTE:
	 return sizeof(GLbyte);
      case GL_UNSIGNED_SHORT:
	 return sizeof(GLushort);
      case GL_SHORT:
	 return sizeof(GLshort);
      case GL_UNSIGNED_INT:
	 return sizeof(GLuint);
      case GL_INT:
	 return sizeof(GLint);
      case GL_FLOAT:
	 return sizeof(GLfloat);
      case GL_HALF_FLOAT_ARB:
	 return sizeof(GLhalfARB);
      default:
         return -1;
   }
}


/**
 * Same as _mesa_sizeof_type() but also accepting the packed pixel
 * format data types.
 */
GLint _mesa_sizeof_packed_type( GLenum type )
{
   switch (type) {
      case GL_BITMAP:
	 return 0;
      case GL_UNSIGNED_BYTE:
         return sizeof(GLubyte);
      case GL_BYTE:
	 return sizeof(GLbyte);
      case GL_UNSIGNED_SHORT:
	 return sizeof(GLushort);
      case GL_SHORT:
	 return sizeof(GLshort);
      case GL_UNSIGNED_INT:
	 return sizeof(GLuint);
      case GL_INT:
	 return sizeof(GLint);
      case GL_HALF_FLOAT_ARB:
	 return sizeof(GLhalfARB);
      case GL_FLOAT:
	 return sizeof(GLfloat);
      case GL_UNSIGNED_BYTE_3_3_2:
         return sizeof(GLubyte);
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         return sizeof(GLubyte);
      case GL_UNSIGNED_SHORT_5_6_5:
         return sizeof(GLushort);
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         return sizeof(GLushort);
      case GL_UNSIGNED_SHORT_4_4_4_4:
         return sizeof(GLushort);
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         return sizeof(GLushort);
      case GL_UNSIGNED_SHORT_5_5_5_1:
         return sizeof(GLushort);
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         return sizeof(GLushort);
      case GL_UNSIGNED_INT_8_8_8_8:
         return sizeof(GLuint);
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         return sizeof(GLuint);
      case GL_UNSIGNED_INT_10_10_10_2:
         return sizeof(GLuint);
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         return sizeof(GLuint);
      case GL_UNSIGNED_SHORT_8_8_MESA:
      case GL_UNSIGNED_SHORT_8_8_REV_MESA:
          return sizeof(GLushort);
      default:
         return -1;
   }
}


/**
 * Get the number of components in a pixel format.
 *
 * \param format pixel format.
 *
 * \return the number of components in the given format, or -1 if a bad format.
 */
GLint _mesa_components_in_format( GLenum format )
{
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_INTENSITY:
         return 1;
      case GL_LUMINANCE_ALPHA:
	 return 2;
      case GL_RGB:
	 return 3;
      case GL_RGBA:
	 return 4;
      case GL_BGR:
	 return 3;
      case GL_BGRA:
	 return 4;
      case GL_ABGR_EXT:
         return 4;
      case GL_YCBCR_MESA:
         return 2;
      default:
         return -1;
   }
}


/**
 * Get the bytes per pixel of pixel format type pair.
 *
 * \param format pixel format.
 * \param type pixel type.
 *
 * \return bytes per pixel, or -1 if a bad format or type was given.
 */
GLint _mesa_bytes_per_pixel( GLenum format, GLenum type )
{
   GLint comps = _mesa_components_in_format( format );
   if (comps < 0)
      return -1;

   switch (type) {
      case GL_BITMAP:
         return 0;  /* special case */
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
         return comps * sizeof(GLubyte);
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
         return comps * sizeof(GLshort);
      case GL_INT:
      case GL_UNSIGNED_INT:
         return comps * sizeof(GLint);
      case GL_FLOAT:
         return comps * sizeof(GLfloat);
      case GL_HALF_FLOAT_ARB:
         return comps * sizeof(GLhalfARB);
      case GL_UNSIGNED_BYTE_3_3_2:
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         if (format == GL_RGB || format == GL_BGR)
            return sizeof(GLubyte);
         else
            return -1;  /* error */
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (format == GL_RGB || format == GL_BGR)
            return sizeof(GLushort);
         else
            return -1;  /* error */
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (format == GL_RGBA || format == GL_BGRA || format == GL_ABGR_EXT)
            return sizeof(GLushort);
         else
            return -1;
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         if (format == GL_RGBA || format == GL_BGRA || format == GL_ABGR_EXT)
            return sizeof(GLuint);
         else
            return -1;
      case GL_UNSIGNED_SHORT_8_8_MESA:
      case GL_UNSIGNED_SHORT_8_8_REV_MESA:
         if (format == GL_YCBCR_MESA)
            return sizeof(GLushort);
         else
            return -1;
      default:
         return -1;
   }
}


/**
 * Test for a legal pixel format and type.
 *
 * \param format pixel format.
 * \param type pixel type.
 *
 * \return GL_TRUE if the given pixel format and type are legal, or GL_FALSE
 * otherwise.
 */
GLboolean
_mesa_is_legal_format_and_type( GLcontext *ctx, GLenum format, GLenum type )
{
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
         switch (type) {
            case GL_BITMAP:
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
               return GL_TRUE;
            case GL_HALF_FLOAT_ARB:
               return ctx->Extensions.ARB_half_float_pixel;
            default:
               return GL_FALSE;
         }
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
#if 0 /* not legal!  see table 3.6 of the 1.5 spec */
      case GL_INTENSITY:
#endif
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_DEPTH_COMPONENT:
         switch (type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
               return GL_TRUE;
            case GL_HALF_FLOAT_ARB:
               return ctx->Extensions.ARB_half_float_pixel;
            default:
               return GL_FALSE;
         }
      case GL_RGB:
         switch (type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
            case GL_UNSIGNED_BYTE_3_3_2:
            case GL_UNSIGNED_BYTE_2_3_3_REV:
            case GL_UNSIGNED_SHORT_5_6_5:
            case GL_UNSIGNED_SHORT_5_6_5_REV:
               return GL_TRUE;
            case GL_HALF_FLOAT_ARB:
               return ctx->Extensions.ARB_half_float_pixel;
            default:
               return GL_FALSE;
         }
      case GL_BGR:
         switch (type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
               return GL_TRUE;
            case GL_HALF_FLOAT_ARB:
               return ctx->Extensions.ARB_half_float_pixel;
            default:
               return GL_FALSE;
         }
      case GL_RGBA:
      case GL_BGRA:
      case GL_ABGR_EXT:
         switch (type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_FLOAT:
            case GL_UNSIGNED_SHORT_4_4_4_4:
            case GL_UNSIGNED_SHORT_4_4_4_4_REV:
            case GL_UNSIGNED_SHORT_5_5_5_1:
            case GL_UNSIGNED_SHORT_1_5_5_5_REV:
            case GL_UNSIGNED_INT_8_8_8_8:
            case GL_UNSIGNED_INT_8_8_8_8_REV:
            case GL_UNSIGNED_INT_10_10_10_2:
            case GL_UNSIGNED_INT_2_10_10_10_REV:
               return GL_TRUE;
            case GL_HALF_FLOAT_ARB:
               return ctx->Extensions.ARB_half_float_pixel;
            default:
               return GL_FALSE;
         }
      case GL_YCBCR_MESA:
         if (type == GL_UNSIGNED_SHORT_8_8_MESA ||
             type == GL_UNSIGNED_SHORT_8_8_REV_MESA)
            return GL_TRUE;
         else
            return GL_FALSE;
      default:
         ; /* fall-through */
   }
   return GL_FALSE;
}


/**
 * Return the address of a specific pixel in an image (1D, 2D or 3D).
 *
 * Pixel unpacking/packing parameters are observed according to \p packing.
 *
 * \param dimensions either 1, 2 or 3 to indicate dimensionality of image
 * \param image  starting address of image data
 * \param width  the image width
 * \param height  theimage height
 * \param format  the pixel format
 * \param type  the pixel data type
 * \param packing  the pixelstore attributes
 * \param img  which image in the volume (0 for 1D or 2D images)
 * \param row  row of pixel in the image (0 for 1D images)
 * \param column column of pixel in the image
 *
 * \return address of pixel on success, or NULL on error.
 *
 * \sa gl_pixelstore_attrib.
 */
GLvoid *
_mesa_image_address( GLuint dimensions,
                     const struct gl_pixelstore_attrib *packing,
                     const GLvoid *image,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     GLint img, GLint row, GLint column )
{
   GLint alignment;        /* 1, 2 or 4 */
   GLint pixels_per_row;
   GLint rows_per_image;
   GLint skiprows;
   GLint skippixels;
   GLint skipimages;       /* for 3-D volume images */
   GLubyte *pixel_addr;

   ASSERT(dimensions >= 1 && dimensions <= 3);

   alignment = packing->Alignment;
   if (packing->RowLength > 0) {
      pixels_per_row = packing->RowLength;
   }
   else {
      pixels_per_row = width;
   }
   if (packing->ImageHeight > 0) {
      rows_per_image = packing->ImageHeight;
   }
   else {
      rows_per_image = height;
   }

   skippixels = packing->SkipPixels;
   /* Note: SKIP_ROWS _is_ used for 1D images */
   skiprows = packing->SkipRows;
   /* Note: SKIP_IMAGES is only used for 3D images */
   skipimages = (dimensions == 3) ? packing->SkipImages : 0;

   if (type == GL_BITMAP) {
      /* BITMAP data */
      GLint comp_per_pixel;   /* components per pixel */
      GLint bytes_per_comp;   /* bytes per component */
      GLint bytes_per_row;
      GLint bytes_per_image;

      /* Compute bytes per component */
      bytes_per_comp = _mesa_sizeof_packed_type( type );
      if (bytes_per_comp < 0) {
         return NULL;
      }

      /* Compute number of components per pixel */
      comp_per_pixel = _mesa_components_in_format( format );
      if (comp_per_pixel < 0) {
         return NULL;
      }

      bytes_per_row = alignment
                    * CEILING( comp_per_pixel*pixels_per_row, 8*alignment );

      bytes_per_image = bytes_per_row * rows_per_image;

      pixel_addr = (GLubyte *) image
                 + (skipimages + img) * bytes_per_image
                 + (skiprows + row) * bytes_per_row
                 + (skippixels + column) / 8;
   }
   else {
      /* Non-BITMAP data */
      GLint bytes_per_pixel, bytes_per_row, remainder, bytes_per_image;
      GLint topOfImage;

      bytes_per_pixel = _mesa_bytes_per_pixel( format, type );

      /* The pixel type and format should have been error checked earlier */
      assert(bytes_per_pixel > 0);

      bytes_per_row = pixels_per_row * bytes_per_pixel;
      remainder = bytes_per_row % alignment;
      if (remainder > 0)
         bytes_per_row += (alignment - remainder);

      ASSERT(bytes_per_row % alignment == 0);

      bytes_per_image = bytes_per_row * rows_per_image;

      if (packing->Invert) {
         /* set pixel_addr to the last row */
         topOfImage = bytes_per_row * (height - 1);
         bytes_per_row = -bytes_per_row;
      }
      else {
         topOfImage = 0;
      }

      /* compute final pixel address */
      pixel_addr = (GLubyte *) image
                 + (skipimages + img) * bytes_per_image
                 + topOfImage
                 + (skiprows + row) * bytes_per_row
                 + (skippixels + column) * bytes_per_pixel;
   }

   return (GLvoid *) pixel_addr;
}


GLvoid *
_mesa_image_address1d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width,
                       GLenum format, GLenum type,
                       GLint column )
{
   return _mesa_image_address(1, packing, image, width, 1,
                              format, type, 0, 0, column);
}


GLvoid *
_mesa_image_address2d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLint row, GLint column )
{
   return _mesa_image_address(2, packing, image, width, height,
                              format, type, 0, row, column);
}


GLvoid *
_mesa_image_address3d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLint img, GLint row, GLint column )
{
   return _mesa_image_address(3, packing, image, width, height,
                              format, type, img, row, column);
}



/**
 * Compute the stride between image rows.
 *
 * \param packing the pixelstore attributes
 * \param width image width.
 * \param format pixel format.
 * \param type pixel data type.
 *
 * \return the stride in bytes for the given parameters.
 *
 * Computes the number of bytes per pixel and row and compensates for alignment.
 *
 * \sa gl_pixelstore_attrib.
 */
GLint
_mesa_image_row_stride( const struct gl_pixelstore_attrib *packing,
                        GLint width, GLenum format, GLenum type )
{
   ASSERT(packing);
   if (type == GL_BITMAP) {
      /* BITMAP data */
      GLint bytes;
      if (packing->RowLength == 0) {
         bytes = (width + 7) / 8;
      }
      else {
         bytes = (packing->RowLength + 7) / 8;
      }
      if (packing->Invert) {
         /* negate the bytes per row (negative row stride) */
         bytes = -bytes;
      }
      return bytes;
   }
   else {
      /* Non-BITMAP data */
      const GLint bytesPerPixel = _mesa_bytes_per_pixel(format, type);
      GLint bytesPerRow, remainder;
      if (bytesPerPixel <= 0)
         return -1;  /* error */
      if (packing->RowLength == 0) {
         bytesPerRow = bytesPerPixel * width;
      }
      else {
         bytesPerRow = bytesPerPixel * packing->RowLength;
      }
      remainder = bytesPerRow % packing->Alignment;
      if (remainder > 0)
         bytesPerRow += (packing->Alignment - remainder);
      if (packing->Invert)
         bytesPerRow = -bytesPerRow;
      return bytesPerRow;
   }
}


#if _HAVE_FULL_GL

/*
 * Compute the stride between images in a 3D texture (in bytes) for the given
 * pixel packing parameters and image width, format and type.
 */
GLint
_mesa_image_image_stride( const struct gl_pixelstore_attrib *packing,
                          GLint width, GLint height,
                          GLenum format, GLenum type )
{
   ASSERT(packing);
   ASSERT(type != GL_BITMAP);

   {
      const GLint bytesPerPixel = _mesa_bytes_per_pixel(format, type);
      GLint bytesPerRow, bytesPerImage, remainder;

      if (bytesPerPixel <= 0)
         return -1;  /* error */
      if (packing->RowLength == 0) {
         bytesPerRow = bytesPerPixel * width;
      }
      else {
         bytesPerRow = bytesPerPixel * packing->RowLength;
      }
      remainder = bytesPerRow % packing->Alignment;
      if (remainder > 0)
         bytesPerRow += (packing->Alignment - remainder);

      if (packing->ImageHeight == 0)
         bytesPerImage = bytesPerRow * height;
      else
         bytesPerImage = bytesPerRow * packing->ImageHeight;

      return bytesPerImage;
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
   GLubyte *ptrn = (GLubyte *) _mesa_unpack_bitmap( 32, 32, pattern, unpacking );
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
      FREE(ptrn);
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
   buffer = (GLubyte *) MALLOC( bytes );
   if (!buffer)
      return NULL;


   width_in_bytes = CEILING( width, 8 );
   dst = buffer;
   for (row = 0; row < height; row++) {
      const GLubyte *src = (const GLubyte *)
         _mesa_image_address2d(packing, pixels, width, height,
                               GL_COLOR_INDEX, GL_BITMAP, row, 0);
      if (!src) {
         FREE(buffer);
         return NULL;
      }

      if (packing->SkipPixels == 0) {
         MEMCPY( dst, src, width_in_bytes );
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

      if (packing->SkipPixels == 0) {
         MEMCPY( dst, src, width_in_bytes );
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
      src += width_in_bytes;
   }
}


/**
 * Apply various pixel transfer operations to an array of RGBA pixels
 * as indicated by the transferOps bitmask
 */
void
_mesa_apply_rgba_transfer_ops(GLcontext *ctx, GLuint transferOps,
                              GLuint n, GLfloat rgba[][4])
{
   /* scale & bias */
   if (transferOps & IMAGE_SCALE_BIAS_BIT) {
      _mesa_scale_and_bias_rgba(n, rgba,
                                ctx->Pixel.RedScale, ctx->Pixel.GreenScale,
                                ctx->Pixel.BlueScale, ctx->Pixel.AlphaScale,
                                ctx->Pixel.RedBias, ctx->Pixel.GreenBias,
                                ctx->Pixel.BlueBias, ctx->Pixel.AlphaBias);
   }
   /* color map lookup */
   if (transferOps & IMAGE_MAP_COLOR_BIT) {
      _mesa_map_rgba( ctx, n, rgba );
   }
   /* GL_COLOR_TABLE lookup */
   if (transferOps & IMAGE_COLOR_TABLE_BIT) {
      _mesa_lookup_rgba_float(&ctx->ColorTable, n, rgba);
   }
   /* convolution */
   if (transferOps & IMAGE_CONVOLUTION_BIT) {
      /* this has to be done in the calling code */
      _mesa_problem(ctx, "IMAGE_CONVOLUTION_BIT set in _mesa_apply_transfer_ops");
   }
   /* GL_POST_CONVOLUTION_RED/GREEN/BLUE/ALPHA_SCALE/BIAS */
   if (transferOps & IMAGE_POST_CONVOLUTION_SCALE_BIAS) {
      _mesa_scale_and_bias_rgba(n, rgba,
                                ctx->Pixel.PostConvolutionScale[RCOMP],
                                ctx->Pixel.PostConvolutionScale[GCOMP],
                                ctx->Pixel.PostConvolutionScale[BCOMP],
                                ctx->Pixel.PostConvolutionScale[ACOMP],
                                ctx->Pixel.PostConvolutionBias[RCOMP],
                                ctx->Pixel.PostConvolutionBias[GCOMP],
                                ctx->Pixel.PostConvolutionBias[BCOMP],
                                ctx->Pixel.PostConvolutionBias[ACOMP]);
   }
   /* GL_POST_CONVOLUTION_COLOR_TABLE lookup */
   if (transferOps & IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT) {
      _mesa_lookup_rgba_float(&ctx->PostConvolutionColorTable, n, rgba);
   }
   /* color matrix transform */
   if (transferOps & IMAGE_COLOR_MATRIX_BIT) {
      _mesa_transform_rgba(ctx, n, rgba);
   }
   /* GL_POST_COLOR_MATRIX_COLOR_TABLE lookup */
   if (transferOps & IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT) {
      _mesa_lookup_rgba_float(&ctx->PostColorMatrixColorTable, n, rgba);
   }
   /* update histogram count */
   if (transferOps & IMAGE_HISTOGRAM_BIT) {
      _mesa_update_histogram(ctx, n, (CONST GLfloat (*)[4]) rgba);
   }
   /* update min/max values */
   if (transferOps & IMAGE_MIN_MAX_BIT) {
      _mesa_update_minmax(ctx, n, (CONST GLfloat (*)[4]) rgba);
   }
   /* clamping to [0,1] */
   if (transferOps & IMAGE_CLAMP_BIT) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = CLAMP(rgba[i][RCOMP], 0.0F, 1.0F);
         rgba[i][GCOMP] = CLAMP(rgba[i][GCOMP], 0.0F, 1.0F);
         rgba[i][BCOMP] = CLAMP(rgba[i][BCOMP], 0.0F, 1.0F);
         rgba[i][ACOMP] = CLAMP(rgba[i][ACOMP], 0.0F, 1.0F);
      }
   }
}



/*
 * Used to pack an array [][4] of RGBA float colors as specified
 * by the dstFormat, dstType and dstPacking.  Used by glReadPixels,
 * glGetConvolutionFilter(), etc.
 */
void
_mesa_pack_rgba_span_float( GLcontext *ctx,
                            GLuint n, CONST GLfloat rgbaIn[][4],
                            GLenum dstFormat, GLenum dstType,
                            GLvoid *dstAddr,
                            const struct gl_pixelstore_attrib *dstPacking,
                            GLuint transferOps )
{
   const GLint comps = _mesa_components_in_format(dstFormat);
   GLfloat luminance[MAX_WIDTH];
   const GLfloat (*rgba)[4];
   GLuint i;

   if (transferOps) {
      /* make copy of incoming data */
      DEFMARRAY(GLfloat, rgbaCopy, MAX_WIDTH, 4);  /* mac 32k limitation */
      CHECKARRAY(rgbaCopy, return);  /* mac 32k limitation */

      _mesa_memcpy(rgbaCopy, rgbaIn, n * 4 * sizeof(GLfloat));
      _mesa_apply_rgba_transfer_ops(ctx, transferOps, n, rgbaCopy);
      rgba = (const GLfloat (*)[4]) rgbaCopy;

      if ((transferOps & IMAGE_MIN_MAX_BIT) && ctx->MinMax.Sink) {
         UNDEFARRAY(rgbaCopy);  /* mac 32k limitation */
         return;
      }
      UNDEFARRAY(rgbaCopy);  /* mac 32k limitation */
   }
   else {
      /* use incoming data, not a copy */
      rgba = (const GLfloat (*)[4]) rgbaIn;
   }

   if (dstFormat == GL_LUMINANCE || dstFormat == GL_LUMINANCE_ALPHA) {
      /* compute luminance values */
      if (ctx->ClampFragmentColors) {
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
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_BYTE(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_BYTE(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_BYTE(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_BYTE(rgba[i][RCOMP]);
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
                     dst[i] = FLOAT_TO_USHORT(rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_USHORT(rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_USHORT(rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_USHORT(rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = FLOAT_TO_USHORT(luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = FLOAT_TO_USHORT(luminance[i]);
                     dst[i*2+1] = FLOAT_TO_USHORT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_USHORT(rgba[i][RCOMP]);
                     dst[i*3+1] = FLOAT_TO_USHORT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_USHORT(rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_USHORT(rgba[i][RCOMP]);
                     dst[i*4+1] = FLOAT_TO_USHORT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_USHORT(rgba[i][BCOMP]);
                     dst[i*4+3] = FLOAT_TO_USHORT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = FLOAT_TO_USHORT(rgba[i][BCOMP]);
                     dst[i*3+1] = FLOAT_TO_USHORT(rgba[i][GCOMP]);
                     dst[i*3+2] = FLOAT_TO_USHORT(rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_USHORT(rgba[i][BCOMP]);
                     dst[i*4+1] = FLOAT_TO_USHORT(rgba[i][GCOMP]);
                     dst[i*4+2] = FLOAT_TO_USHORT(rgba[i][RCOMP]);
                     dst[i*4+3] = FLOAT_TO_USHORT(rgba[i][ACOMP]);
                  }
                  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_USHORT(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_USHORT(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_USHORT(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_USHORT(rgba[i][RCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
            if (dstPacking->SwapBytes) {
               _mesa_swap2( (GLushort *) dst, n * comps);
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
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = FLOAT_TO_SHORT(rgba[i][ACOMP]);
                     dst[i*4+1] = FLOAT_TO_SHORT(rgba[i][BCOMP]);
                     dst[i*4+2] = FLOAT_TO_SHORT(rgba[i][GCOMP]);
                     dst[i*4+3] = FLOAT_TO_SHORT(rgba[i][RCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
            if (dstPacking->SwapBytes) {
               _mesa_swap2( (GLushort *) dst, n * comps );
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
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
            if (dstPacking->SwapBytes) {
               _mesa_swap4( (GLuint *) dst, n * comps );
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
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
            if (dstPacking->SwapBytes) {
               _mesa_swap4( (GLuint *) dst, n * comps );
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
            if (dstPacking->SwapBytes) {
               _mesa_swap4( (GLuint *) dst, n * comps );
            }
         }
         break;
      case GL_HALF_FLOAT_ARB:
         {
            GLhalfARB *dst = (GLhalfARB *) dstAddr;
            switch (dstFormat) {
               case GL_RED:
                  for (i=0;i<n;i++)
                     dst[i] = _mesa_float_to_half(rgba[i][RCOMP]);
                  break;
               case GL_GREEN:
                  for (i=0;i<n;i++)
                     dst[i] = _mesa_float_to_half(rgba[i][GCOMP]);
                  break;
               case GL_BLUE:
                  for (i=0;i<n;i++)
                     dst[i] = _mesa_float_to_half(rgba[i][BCOMP]);
                  break;
               case GL_ALPHA:
                  for (i=0;i<n;i++)
                     dst[i] = _mesa_float_to_half(rgba[i][ACOMP]);
                  break;
               case GL_LUMINANCE:
                  for (i=0;i<n;i++)
                     dst[i] = _mesa_float_to_half(luminance[i]);
                  break;
               case GL_LUMINANCE_ALPHA:
                  for (i=0;i<n;i++) {
                     dst[i*2+0] = _mesa_float_to_half(luminance[i]);
                     dst[i*2+1] = _mesa_float_to_half(rgba[i][ACOMP]);
                  }
                  break;
               case GL_RGB:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = _mesa_float_to_half(rgba[i][RCOMP]);
                     dst[i*3+1] = _mesa_float_to_half(rgba[i][GCOMP]);
                     dst[i*3+2] = _mesa_float_to_half(rgba[i][BCOMP]);
                  }
                  break;
               case GL_RGBA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = _mesa_float_to_half(rgba[i][RCOMP]);
                     dst[i*4+1] = _mesa_float_to_half(rgba[i][GCOMP]);
                     dst[i*4+2] = _mesa_float_to_half(rgba[i][BCOMP]);
                     dst[i*4+3] = _mesa_float_to_half(rgba[i][ACOMP]);
                  }
                  break;
               case GL_BGR:
                  for (i=0;i<n;i++) {
                     dst[i*3+0] = _mesa_float_to_half(rgba[i][BCOMP]);
                     dst[i*3+1] = _mesa_float_to_half(rgba[i][GCOMP]);
                     dst[i*3+2] = _mesa_float_to_half(rgba[i][RCOMP]);
                  }
                  break;
               case GL_BGRA:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = _mesa_float_to_half(rgba[i][BCOMP]);
                     dst[i*4+1] = _mesa_float_to_half(rgba[i][GCOMP]);
                     dst[i*4+2] = _mesa_float_to_half(rgba[i][RCOMP]);
                     dst[i*4+3] = _mesa_float_to_half(rgba[i][ACOMP]);
                  }
                  break;
               case GL_ABGR_EXT:
                  for (i=0;i<n;i++) {
                     dst[i*4+0] = _mesa_float_to_half(rgba[i][ACOMP]);
                     dst[i*4+1] = _mesa_float_to_half(rgba[i][BCOMP]);
                     dst[i*4+2] = _mesa_float_to_half(rgba[i][GCOMP]);
                     dst[i*4+3] = _mesa_float_to_half(rgba[i][RCOMP]);
                  }
                  break;
               default:
                  _mesa_problem(ctx, "bad format in _mesa_pack_rgba_span\n");
            }
            if (dstPacking->SwapBytes) {
               _mesa_swap2( (GLushort *) dst, n * comps );
            }
         }
         break;
      case GL_UNSIGNED_BYTE_3_3_2:
         if (dstFormat == GL_RGB) {
            GLubyte *dst = (GLubyte *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 7.0F)) << 5)
                      | (((GLint) (rgba[i][GCOMP] * 7.0F)) << 2)
                      | (((GLint) (rgba[i][BCOMP] * 3.0F))     );
            }
         }
         break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         if (dstFormat == GL_RGB) {
            GLubyte *dst = (GLubyte *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 7.0F))     )
                      | (((GLint) (rgba[i][GCOMP] * 7.0F)) << 3)
                      | (((GLint) (rgba[i][BCOMP] * 3.0F)) << 5);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5:
         if (dstFormat == GL_RGB) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 31.0F)) << 11)
                      | (((GLint) (rgba[i][GCOMP] * 63.0F)) <<  5)
                      | (((GLint) (rgba[i][BCOMP] * 31.0F))      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         if (dstFormat == GL_RGB) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 31.0F))      )
                      | (((GLint) (rgba[i][GCOMP] * 63.0F)) <<  5)
                      | (((GLint) (rgba[i][BCOMP] * 31.0F)) << 11);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 15.0F)) << 12)
                      | (((GLint) (rgba[i][GCOMP] * 15.0F)) <<  8)
                      | (((GLint) (rgba[i][BCOMP] * 15.0F)) <<  4)
                      | (((GLint) (rgba[i][ACOMP] * 15.0F))      );
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][BCOMP] * 15.0F)) << 12)
                      | (((GLint) (rgba[i][GCOMP] * 15.0F)) <<  8)
                      | (((GLint) (rgba[i][RCOMP] * 15.0F)) <<  4)
                      | (((GLint) (rgba[i][ACOMP] * 15.0F))      );
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][ACOMP] * 15.0F)) <<  4)
                      | (((GLint) (rgba[i][BCOMP] * 15.0F)) <<  8)
                      | (((GLint) (rgba[i][GCOMP] * 15.0F)) << 12)
                      | (((GLint) (rgba[i][RCOMP] * 15.0F))      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 15.0F))      )
                      | (((GLint) (rgba[i][GCOMP] * 15.0F)) <<  4)
                      | (((GLint) (rgba[i][BCOMP] * 15.0F)) <<  8)
                      | (((GLint) (rgba[i][ACOMP] * 15.0F)) << 12);
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][BCOMP] * 15.0F))      )
                      | (((GLint) (rgba[i][GCOMP] * 15.0F)) <<  4)
                      | (((GLint) (rgba[i][RCOMP] * 15.0F)) <<  8)
                      | (((GLint) (rgba[i][ACOMP] * 15.0F)) << 12);
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][ACOMP] * 15.0F))      )
                      | (((GLint) (rgba[i][BCOMP] * 15.0F)) <<  4)
                      | (((GLint) (rgba[i][GCOMP] * 15.0F)) <<  8)
                      | (((GLint) (rgba[i][RCOMP] * 15.0F)) << 12);
            }
         }
         break;
      case GL_UNSIGNED_SHORT_5_5_5_1:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 31.0F)) << 11)
                      | (((GLint) (rgba[i][GCOMP] * 31.0F)) <<  6)
                      | (((GLint) (rgba[i][BCOMP] * 31.0F)) <<  1)
                      | (((GLint) (rgba[i][ACOMP] *  1.0F))      );
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][BCOMP] * 31.0F)) << 11)
                      | (((GLint) (rgba[i][GCOMP] * 31.0F)) <<  6)
                      | (((GLint) (rgba[i][RCOMP] * 31.0F)) <<  1)
                      | (((GLint) (rgba[i][ACOMP] *  1.0F))      );
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][ACOMP] * 31.0F)) << 11)
                      | (((GLint) (rgba[i][BCOMP] * 31.0F)) <<  6)
                      | (((GLint) (rgba[i][GCOMP] * 31.0F)) <<  1)
                      | (((GLint) (rgba[i][RCOMP] *  1.0F))      );
            }
         }
         break;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
         if (dstFormat == GL_RGBA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][RCOMP] * 31.0F))      )
                      | (((GLint) (rgba[i][GCOMP] * 31.0F)) <<  5)
                      | (((GLint) (rgba[i][BCOMP] * 31.0F)) << 10)
                      | (((GLint) (rgba[i][ACOMP] *  1.0F)) << 15);
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][BCOMP] * 31.0F))      )
                      | (((GLint) (rgba[i][GCOMP] * 31.0F)) <<  5)
                      | (((GLint) (rgba[i][RCOMP] * 31.0F)) << 10)
                      | (((GLint) (rgba[i][ACOMP] *  1.0F)) << 15);
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLushort *dst = (GLushort *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLint) (rgba[i][ACOMP] * 31.0F))      )
                      | (((GLint) (rgba[i][BCOMP] * 31.0F)) <<  5)
                      | (((GLint) (rgba[i][GCOMP] * 31.0F)) << 10)
                      | (((GLint) (rgba[i][RCOMP] *  1.0F)) << 15);
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8:
         if (dstFormat == GL_RGBA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][RCOMP] * 255.0F)) << 24)
                      | (((GLuint) (rgba[i][GCOMP] * 255.0F)) << 16)
                      | (((GLuint) (rgba[i][BCOMP] * 255.0F)) <<  8)
                      | (((GLuint) (rgba[i][ACOMP] * 255.0F))      );
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][BCOMP] * 255.0F)) << 24)
                      | (((GLuint) (rgba[i][GCOMP] * 255.0F)) << 16)
                      | (((GLuint) (rgba[i][RCOMP] * 255.0F)) <<  8)
                      | (((GLuint) (rgba[i][ACOMP] * 255.0F))      );
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][ACOMP] * 255.0F)) << 24)
                      | (((GLuint) (rgba[i][BCOMP] * 255.0F)) << 16)
                      | (((GLuint) (rgba[i][GCOMP] * 255.0F)) <<  8)
                      | (((GLuint) (rgba[i][RCOMP] * 255.0F))      );
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         if (dstFormat == GL_RGBA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][RCOMP] * 255.0F))      )
                      | (((GLuint) (rgba[i][GCOMP] * 255.0F)) <<  8)
                      | (((GLuint) (rgba[i][BCOMP] * 255.0F)) << 16)
                      | (((GLuint) (rgba[i][ACOMP] * 255.0F)) << 24);
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][BCOMP] * 255.0F))      )
                      | (((GLuint) (rgba[i][GCOMP] * 255.0F)) <<  8)
                      | (((GLuint) (rgba[i][RCOMP] * 255.0F)) << 16)
                      | (((GLuint) (rgba[i][ACOMP] * 255.0F)) << 24);
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][ACOMP] * 255.0F))      )
                      | (((GLuint) (rgba[i][BCOMP] * 255.0F)) <<  8)
                      | (((GLuint) (rgba[i][GCOMP] * 255.0F)) << 16)
                      | (((GLuint) (rgba[i][RCOMP] * 255.0F)) << 24);
            }
         }
         break;
      case GL_UNSIGNED_INT_10_10_10_2:
         if (dstFormat == GL_RGBA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][RCOMP] * 1023.0F)) << 22)
                      | (((GLuint) (rgba[i][GCOMP] * 1023.0F)) << 12)
                      | (((GLuint) (rgba[i][BCOMP] * 1023.0F)) <<  2)
                      | (((GLuint) (rgba[i][ACOMP] *    3.0F))      );
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][BCOMP] * 1023.0F)) << 22)
                      | (((GLuint) (rgba[i][GCOMP] * 1023.0F)) << 12)
                      | (((GLuint) (rgba[i][RCOMP] * 1023.0F)) <<  2)
                      | (((GLuint) (rgba[i][ACOMP] *    3.0F))      );
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][ACOMP] * 1023.0F)) << 22)
                      | (((GLuint) (rgba[i][BCOMP] * 1023.0F)) << 12)
                      | (((GLuint) (rgba[i][GCOMP] * 1023.0F)) <<  2)
                      | (((GLuint) (rgba[i][RCOMP] *    3.0F))      );
            }
         }
         break;
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         if (dstFormat == GL_RGBA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][RCOMP] * 1023.0F))      )
                      | (((GLuint) (rgba[i][GCOMP] * 1023.0F)) << 10)
                      | (((GLuint) (rgba[i][BCOMP] * 1023.0F)) << 20)
                      | (((GLuint) (rgba[i][ACOMP] *    3.0F)) << 30);
            }
         }
         else if (dstFormat == GL_BGRA) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][BCOMP] * 1023.0F))      )
                      | (((GLuint) (rgba[i][GCOMP] * 1023.0F)) << 10)
                      | (((GLuint) (rgba[i][RCOMP] * 1023.0F)) << 20)
                      | (((GLuint) (rgba[i][ACOMP] *    3.0F)) << 30);
            }
         }
         else if (dstFormat == GL_ABGR_EXT) {
            GLuint *dst = (GLuint *) dstAddr;
            for (i=0;i<n;i++) {
               dst[i] = (((GLuint) (rgba[i][ACOMP] * 1023.0F))      )
                      | (((GLuint) (rgba[i][BCOMP] * 1023.0F)) << 10)
                      | (((GLuint) (rgba[i][GCOMP] * 1023.0F)) << 20)
                      | (((GLuint) (rgba[i][RCOMP] *    3.0F)) << 30);
            }
         }
         break;
      default:
         _mesa_problem(ctx, "bad type in _mesa_pack_rgba_span_float");
   }
}


/*
 * Pack the given RGBA span into client memory at 'dest' address
 * in the given pixel format and type.
 * Optionally apply the enabled pixel transfer ops.
 * Pack into memory using the given packing params struct.
 * This is used by glReadPixels and glGetTexImage?D()
 * \param ctx - the context
 *         n - number of pixels in the span
 *         rgba - the pixels
 *         format - dest packing format
 *         type - dest packing data type
 *         destination - destination packing address
 *         packing - pixel packing parameters
 *         transferOps - bitmask of IMAGE_*_BIT operations to apply
 */
void
_mesa_pack_rgba_span_chan( GLcontext *ctx,
                           GLuint n, CONST GLchan srcRgba[][4],
                           GLenum dstFormat, GLenum dstType,
                           GLvoid *dstAddr,
                           const struct gl_pixelstore_attrib *dstPacking,
                           GLuint transferOps)
{
   ASSERT((ctx->NewState & _NEW_PIXEL) == 0 || transferOps == 0);

   /* Test for optimized case first */
   if (transferOps == 0 && dstFormat == GL_RGBA && dstType == CHAN_TYPE) {
      /* common simple case */
      MEMCPY(dstAddr, srcRgba, n * 4 * sizeof(GLchan));
   }
   else if (transferOps == 0 && dstFormat == GL_RGB && dstType == CHAN_TYPE) {
      /* common simple case */
      GLuint i;
      GLchan *dest = (GLchan *) dstAddr;
      for (i = 0; i < n; i++) {
         dest[0] = srcRgba[i][RCOMP];
         dest[1] = srcRgba[i][GCOMP];
         dest[2] = srcRgba[i][BCOMP];
         dest += 3;
      }
   }
   else if (transferOps == 0 && dstFormat == GL_RGBA && dstType == GL_UNSIGNED_BYTE) {
      /* common simple case */
      GLuint i;
      GLubyte *dest = (GLubyte *) dstAddr;
      for (i = 0; i < n; i++) {
         dest[0] = CHAN_TO_UBYTE(srcRgba[i][RCOMP]);
         dest[1] = CHAN_TO_UBYTE(srcRgba[i][GCOMP]);
         dest[2] = CHAN_TO_UBYTE(srcRgba[i][BCOMP]);
         dest[3] = CHAN_TO_UBYTE(srcRgba[i][ACOMP]);
         dest += 4;
      }
   }
   else {
      /* general solution */
      GLuint i;
      DEFMARRAY(GLfloat, rgba, MAX_WIDTH, 4);  /* mac 32k limitation */
      CHECKARRAY(rgba, return);  /* mac 32k limitation */

      assert(n <= MAX_WIDTH);
      /* convert color components to floating point */
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = CHAN_TO_FLOAT(srcRgba[i][RCOMP]);
         rgba[i][GCOMP] = CHAN_TO_FLOAT(srcRgba[i][GCOMP]);
         rgba[i][BCOMP] = CHAN_TO_FLOAT(srcRgba[i][BCOMP]);
         rgba[i][ACOMP] = CHAN_TO_FLOAT(srcRgba[i][ACOMP]);
      }
      _mesa_pack_rgba_span_float(ctx, n, (const GLfloat (*)[4]) rgba,
                                 dstFormat, dstType, dstAddr,
                                 dstPacking, transferOps);
      UNDEFARRAY(rgba);  /* mac 32k limitation */
   }
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
   assert(srcFormat == GL_COLOR_INDEX);

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
      case GL_HALF_FLOAT_ARB:
         {
            GLuint i;
            const GLhalfARB *s = (const GLhalfARB *) src;
            if (unpack->SwapBytes) {
               for (i = 0; i < n; i++) {
                  GLhalfARB value = s[i];
                  SWAP2BYTE(value);
                  indexes[i] = (GLuint) _mesa_half_to_float(value);
               }
            }
            else {
               for (i = 0; i < n; i++)
                  indexes[i] = (GLuint) _mesa_half_to_float(s[i]);
            }
         }
         break;
      default:
         _mesa_problem(NULL, "bad srcType in extract_uint_indexes");
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
   GLint redIndex, greenIndex, blueIndex, alphaIndex;
   GLint stride;
   GLint rComp, bComp, gComp, aComp;

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
          srcFormat == GL_RGB ||
          srcFormat == GL_BGR ||
          srcFormat == GL_RGBA ||
          srcFormat == GL_BGRA ||
          srcFormat == GL_ABGR_EXT);

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
          srcType == GL_UNSIGNED_INT_10_10_10_2 ||
          srcType == GL_UNSIGNED_INT_2_10_10_10_REV);

   rComp = gComp = bComp = aComp = -1;

   switch (srcFormat) {
      case GL_RED:
         redIndex = 0;
         greenIndex = blueIndex = alphaIndex = -1;
         stride = 1;
         break;
      case GL_GREEN:
         greenIndex = 0;
         redIndex = blueIndex = alphaIndex = -1;
         stride = 1;
         break;
      case GL_BLUE:
         blueIndex = 0;
         redIndex = greenIndex = alphaIndex = -1;
         stride = 1;
         break;
      case GL_ALPHA:
         redIndex = greenIndex = blueIndex = -1;
         alphaIndex = 0;
         stride = 1;
         break;
      case GL_LUMINANCE:
         redIndex = greenIndex = blueIndex = 0;
         alphaIndex = -1;
         stride = 1;
         break;
      case GL_LUMINANCE_ALPHA:
         redIndex = greenIndex = blueIndex = 0;
         alphaIndex = 1;
         stride = 2;
         break;
      case GL_INTENSITY:
         redIndex = greenIndex = blueIndex = alphaIndex = 0;
         stride = 1;
         break;
      case GL_RGB:
         redIndex = 0;
         greenIndex = 1;
         blueIndex = 2;
         alphaIndex = -1;
         rComp = 0;
         gComp = 1;
         bComp = 2;
         aComp = 3;
         stride = 3;
         break;
      case GL_BGR:
         redIndex = 2;
         greenIndex = 1;
         blueIndex = 0;
         alphaIndex = -1;
         rComp = 2;
         gComp = 1;
         bComp = 0;
         aComp = 3;
         stride = 3;
         break;
      case GL_RGBA:
         redIndex = 0;
         greenIndex = 1;
         blueIndex = 2;
         alphaIndex = 3;
         rComp = 0;
         gComp = 1;
         bComp = 2;
         aComp = 3;
         stride = 4;
         break;
      case GL_BGRA:
         redIndex = 2;
         greenIndex = 1;
         blueIndex = 0;
         alphaIndex = 3;
         rComp = 2;
         gComp = 1;
         bComp = 0;
         aComp = 3;
         stride = 4;
         break;
      case GL_ABGR_EXT:
         redIndex = 3;
         greenIndex = 2;
         blueIndex = 1;
         alphaIndex = 0;
         rComp = 3;
         gComp = 2;
         bComp = 1;
         aComp = 0;
         stride = 4;
         break;
      default:
         _mesa_problem(NULL, "bad srcFormat in extract float data");
         return;
   }


#define PROCESS(INDEX, CHANNEL, DEFAULT, TYPE, CONVERSION)		\
   if ((INDEX) < 0) {							\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         rgba[i][CHANNEL] = DEFAULT;					\
      }									\
   }									\
   else if (swapBytes) {						\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         TYPE value = s[INDEX];						\
         if (sizeof(TYPE) == 2) {					\
            SWAP2BYTE(value);						\
         }								\
         else if (sizeof(TYPE) == 4) {					\
            SWAP4BYTE(value);						\
         }								\
         rgba[i][CHANNEL] = (GLfloat) CONVERSION(value);		\
         s += stride;							\
      }									\
   }									\
   else {								\
      const TYPE *s = (const TYPE *) src;				\
      GLuint i;								\
      for (i = 0; i < n; i++) {						\
         rgba[i][CHANNEL] = (GLfloat) CONVERSION(s[INDEX]);		\
         s += stride;							\
      }									\
   }

   switch (srcType) {
      case GL_UNSIGNED_BYTE:
         PROCESS(redIndex,   RCOMP, 0.0F, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLubyte, UBYTE_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLubyte, UBYTE_TO_FLOAT);
         break;
      case GL_BYTE:
         PROCESS(redIndex,   RCOMP, 0.0F, GLbyte, BYTE_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLbyte, BYTE_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLbyte, BYTE_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLbyte, BYTE_TO_FLOAT);
         break;
      case GL_UNSIGNED_SHORT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLushort, USHORT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLushort, USHORT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLushort, USHORT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLushort, USHORT_TO_FLOAT);
         break;
      case GL_SHORT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLshort, SHORT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLshort, SHORT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLshort, SHORT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLshort, SHORT_TO_FLOAT);
         break;
      case GL_UNSIGNED_INT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLuint, UINT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLuint, UINT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLuint, UINT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLuint, UINT_TO_FLOAT);
         break;
      case GL_INT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLint, INT_TO_FLOAT);
         PROCESS(greenIndex, GCOMP, 0.0F, GLint, INT_TO_FLOAT);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLint, INT_TO_FLOAT);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLint, INT_TO_FLOAT);
         break;
      case GL_FLOAT:
         PROCESS(redIndex,   RCOMP, 0.0F, GLfloat, (GLfloat));
         PROCESS(greenIndex, GCOMP, 0.0F, GLfloat, (GLfloat));
         PROCESS(blueIndex,  BCOMP, 0.0F, GLfloat, (GLfloat));
         PROCESS(alphaIndex, ACOMP, 1.0F, GLfloat, (GLfloat));
         break;
      case GL_HALF_FLOAT_ARB:
         PROCESS(redIndex,   RCOMP, 0.0F, GLhalfARB, _mesa_half_to_float);
         PROCESS(greenIndex, GCOMP, 0.0F, GLhalfARB, _mesa_half_to_float);
         PROCESS(blueIndex,  BCOMP, 0.0F, GLhalfARB, _mesa_half_to_float);
         PROCESS(alphaIndex, ACOMP, 1.0F, GLhalfARB, _mesa_half_to_float);
         break;
      case GL_UNSIGNED_BYTE_3_3_2:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][rComp] = ((p >> 5)      ) * (1.0F / 7.0F);
               rgba[i][gComp] = ((p >> 2) & 0x7) * (1.0F / 7.0F);
               rgba[i][bComp] = ((p     ) & 0x3) * (1.0F / 3.0F);
               rgba[i][aComp] = 1.0F;
            }
         }
         break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
         {
            const GLubyte *ubsrc = (const GLubyte *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLubyte p = ubsrc[i];
               rgba[i][rComp] = ((p     ) & 0x7) * (1.0F / 7.0F);
               rgba[i][gComp] = ((p >> 3) & 0x7) * (1.0F / 7.0F);
               rgba[i][bComp] = ((p >> 6)      ) * (1.0F / 3.0F);
               rgba[i][aComp] = 1.0F;
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
               rgba[i][rComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][bComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = 1.0F;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][bComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = 1.0F;
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
               rgba[i][rComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][bComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][aComp] = 1.0F;
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x3f) * (1.0F / 63.0F);
               rgba[i][bComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][aComp] = 1.0F;
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
               rgba[i][rComp] = ((p >> 12)      ) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p >> 12)      ) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
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
               rgba[i][rComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p >> 12)      ) * (1.0F / 15.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p      ) & 0xf) * (1.0F / 15.0F);
               rgba[i][gComp] = ((p >>  4) & 0xf) * (1.0F / 15.0F);
               rgba[i][bComp] = ((p >>  8) & 0xf) * (1.0F / 15.0F);
               rgba[i][aComp] = ((p >> 12)      ) * (1.0F / 15.0F);
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
               rgba[i][rComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  6) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >>  1) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p      ) & 0x1)  * (1.0F /  1.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p >> 11)       ) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  6) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >>  1) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p      ) & 0x1)  * (1.0F /  1.0F);
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
               rgba[i][rComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >> 10) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p >> 15)       ) * (1.0F /  1.0F);
            }
         }
         else {
            const GLushort *ussrc = (const GLushort *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLushort p = ussrc[i];
               rgba[i][rComp] = ((p      ) & 0x1f) * (1.0F / 31.0F);
               rgba[i][gComp] = ((p >>  5) & 0x1f) * (1.0F / 31.0F);
               rgba[i][bComp] = ((p >> 10) & 0x1f) * (1.0F / 31.0F);
               rgba[i][aComp] = ((p >> 15)       ) * (1.0F /  1.0F);
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_TO_FLOAT((p      ) & 0xff);
               rgba[i][gComp] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
               rgba[i][bComp] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
               rgba[i][aComp] = UBYTE_TO_FLOAT((p >> 24)       );
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_TO_FLOAT((p >> 24)       );
               rgba[i][gComp] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
               rgba[i][bComp] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
               rgba[i][aComp] = UBYTE_TO_FLOAT((p      ) & 0xff);
            }
         }
         break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_TO_FLOAT((p >> 24)       );
               rgba[i][gComp] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
               rgba[i][bComp] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
               rgba[i][aComp] = UBYTE_TO_FLOAT((p      ) & 0xff);
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = UBYTE_TO_FLOAT((p      ) & 0xff);
               rgba[i][gComp] = UBYTE_TO_FLOAT((p >>  8) & 0xff);
               rgba[i][bComp] = UBYTE_TO_FLOAT((p >> 16) & 0xff);
               rgba[i][aComp] = UBYTE_TO_FLOAT((p >> 24)       );
            }
         }
         break;
      case GL_UNSIGNED_INT_10_10_10_2:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               SWAP4BYTE(p);
               rgba[i][rComp] = ((p >> 22)        ) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 12) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >>  2) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p      ) & 0x3  ) * (1.0F /    3.0F);
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = ((p >> 22)        ) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 12) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >>  2) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p      ) & 0x3  ) * (1.0F /    3.0F);
            }
         }
         break;
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         if (swapBytes) {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               SWAP4BYTE(p);
               rgba[i][rComp] = ((p      ) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 10) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >> 20) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p >> 30)        ) * (1.0F /    3.0F);
            }
         }
         else {
            const GLuint *uisrc = (const GLuint *) src;
            GLuint i;
            for (i = 0; i < n; i ++) {
               GLuint p = uisrc[i];
               rgba[i][rComp] = ((p      ) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][gComp] = ((p >> 10) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][bComp] = ((p >> 20) & 0x3ff) * (1.0F / 1023.0F);
               rgba[i][aComp] = ((p >> 30)        ) * (1.0F /    3.0F);
            }
         }
         break;
      default:
         _mesa_problem(NULL, "bad srcType in extract float data");
         break;
   }
}


/*
 * Unpack a row of color image data from a client buffer according to
 * the pixel unpacking parameters.
 * Return GLchan values in the specified dest image format.
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
_mesa_unpack_color_span_chan( GLcontext *ctx,
                              GLuint n, GLenum dstFormat, GLchan dest[],
                              GLenum srcFormat, GLenum srcType,
                              const GLvoid *source,
                              const struct gl_pixelstore_attrib *srcPacking,
                              GLuint transferOps )
{
   ASSERT(dstFormat == GL_ALPHA ||
          dstFormat == GL_LUMINANCE ||
          dstFormat == GL_LUMINANCE_ALPHA ||
          dstFormat == GL_INTENSITY ||
          dstFormat == GL_RGB ||
          dstFormat == GL_RGBA ||
          dstFormat == GL_COLOR_INDEX);

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
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
          srcType == GL_UNSIGNED_INT_10_10_10_2 ||
          srcType == GL_UNSIGNED_INT_2_10_10_10_REV);

   /* Try simple cases first */
   if (transferOps == 0) {
      if (srcType == CHAN_TYPE) {
         if (dstFormat == GL_RGBA) {
            if (srcFormat == GL_RGBA) {
               MEMCPY( dest, source, n * 4 * sizeof(GLchan) );
               return;
            }
            else if (srcFormat == GL_RGB) {
               GLuint i;
               const GLchan *src = (const GLchan *) source;
               GLchan *dst = dest;
               for (i = 0; i < n; i++) {
                  dst[0] = src[0];
                  dst[1] = src[1];
                  dst[2] = src[2];
                  dst[3] = CHAN_MAX;
                  src += 3;
                  dst += 4;
               }
               return;
            }
         }
         else if (dstFormat == GL_RGB) {
            if (srcFormat == GL_RGB) {
               MEMCPY( dest, source, n * 3 * sizeof(GLchan) );
               return;
            }
            else if (srcFormat == GL_RGBA) {
               GLuint i;
               const GLchan *src = (const GLchan *) source;
               GLchan *dst = dest;
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
            MEMCPY( dest, source, n * comps * sizeof(GLchan) );
            return;
         }
      }
      /*
       * Common situation, loading 8bit RGBA/RGB source images
       * into 16/32 bit destination. (OSMesa16/32)
       */
      else if (srcType == GL_UNSIGNED_BYTE) {
         if (dstFormat == GL_RGBA) {
            if (srcFormat == GL_RGB) {
               GLuint i;
               const GLubyte *src = (const GLubyte *) source;
               GLchan *dst = dest;
               for (i = 0; i < n; i++) {
                  dst[0] = UBYTE_TO_CHAN(src[0]);
                  dst[1] = UBYTE_TO_CHAN(src[1]);
                  dst[2] = UBYTE_TO_CHAN(src[2]);
                  dst[3] = CHAN_MAX;
                  src += 3;
                  dst += 4;
               }
               return;
            }
            else if (srcFormat == GL_RGBA) {
               GLuint i;
               const GLubyte *src = (const GLubyte *) source;
               GLchan *dst = dest;
               for (i = 0; i < n; i++) {
                  dst[0] = UBYTE_TO_CHAN(src[0]);
                  dst[1] = UBYTE_TO_CHAN(src[1]);
                  dst[2] = UBYTE_TO_CHAN(src[2]);
                  dst[3] = UBYTE_TO_CHAN(src[3]);
                  src += 4;
                  dst += 4;
               }
               return;
             }
         }
         else if (dstFormat == GL_RGB) {
            if (srcFormat == GL_RGB) {
               GLuint i;
               const GLubyte *src = (const GLubyte *) source;
               GLchan *dst = dest;
               for (i = 0; i < n; i++) {
                  dst[0] = UBYTE_TO_CHAN(src[0]);
                  dst[1] = UBYTE_TO_CHAN(src[1]);
                  dst[2] = UBYTE_TO_CHAN(src[2]);
                  src += 3;
                  dst += 3;
               }
               return;
            }
            else if (srcFormat == GL_RGBA) {
               GLuint i;
               const GLubyte *src = (const GLubyte *) source;
               GLchan *dst = dest;
               for (i = 0; i < n; i++) {
                  dst[0] = UBYTE_TO_CHAN(src[0]);
                  dst[1] = UBYTE_TO_CHAN(src[1]);
                  dst[2] = UBYTE_TO_CHAN(src[2]);
                  src += 4;
                  dst += 3;
               }
               return;
            }
         }
      }
   }


   /* general solution begins here */
   {
      GLint dstComponents;
      GLint dstRedIndex, dstGreenIndex, dstBlueIndex, dstAlphaIndex;
      GLint dstLuminanceIndex, dstIntensityIndex;
      DEFMARRAY(GLfloat, rgba, MAX_WIDTH, 4);  /* mac 32k limitation */
      CHECKARRAY(rgba, return);  /* mac 32k limitation */

      dstComponents = _mesa_components_in_format( dstFormat );
      /* source & dest image formats should have been error checked by now */
      assert(dstComponents > 0);

      /*
       * Extract image data and convert to RGBA floats
       */
      assert(n <= MAX_WIDTH);
      if (srcFormat == GL_COLOR_INDEX) {
         GLuint indexes[MAX_WIDTH];
         extract_uint_indexes(n, indexes, srcFormat, srcType, source,
                              srcPacking);

         if (dstFormat == GL_COLOR_INDEX
             && (transferOps & IMAGE_MAP_COLOR_BIT)) {
            _mesa_map_ci(ctx, n, indexes);
         }
         if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
            _mesa_shift_and_offset_ci(ctx, n, indexes);
         }

         if (dstFormat == GL_COLOR_INDEX) {
            /* convert to GLchan and return */
            GLuint i;
            for (i = 0; i < n; i++) {
               dest[i] = (GLchan) (indexes[i] & 0xff);
            }
            UNDEFARRAY(rgba);  /* mac 32k limitation */
            return;
         }
         else {
            /* Convert indexes to RGBA */
            _mesa_map_ci_to_rgba(ctx, n, indexes, rgba);
         }

         /* Don't do RGBA scale/bias or RGBA->RGBA mapping if starting
          * with color indexes.
          */
         transferOps &= ~(IMAGE_SCALE_BIAS_BIT | IMAGE_MAP_COLOR_BIT);
      }
      else {
         /* non-color index data */
         extract_float_rgba(n, rgba, srcFormat, srcType, source,
                            srcPacking->SwapBytes);
      }

      /* Need to clamp if returning GLubytes or GLushorts */
#if CHAN_TYPE != GL_FLOAT
      transferOps |= IMAGE_CLAMP_BIT;
#endif

      if (transferOps) {
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, n, rgba);
      }

      /* Now determine which color channels we need to produce.
       * And determine the dest index (offset) within each color tuple.
       */
      switch (dstFormat) {
         case GL_ALPHA:
            dstAlphaIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = -1;
            dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         case GL_LUMINANCE:
            dstLuminanceIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = dstAlphaIndex = -1;
            dstIntensityIndex = -1;
            break;
         case GL_LUMINANCE_ALPHA:
            dstLuminanceIndex = 0;
            dstAlphaIndex = 1;
            dstRedIndex = dstGreenIndex = dstBlueIndex = -1;
            dstIntensityIndex = -1;
            break;
         case GL_INTENSITY:
            dstIntensityIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = dstAlphaIndex = -1;
            dstLuminanceIndex = -1;
            break;
         case GL_RGB:
            dstRedIndex = 0;
            dstGreenIndex = 1;
            dstBlueIndex = 2;
            dstAlphaIndex = dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         case GL_RGBA:
            dstRedIndex = 0;
            dstGreenIndex = 1;
            dstBlueIndex = 2;
            dstAlphaIndex = 3;
            dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         default:
            _mesa_problem(ctx, "bad dstFormat in _mesa_unpack_chan_span()");
            UNDEFARRAY(rgba);  /* mac 32k limitation */
            return;
      }


      /* Now return the GLchan data in the requested dstFormat */

      if (dstRedIndex >= 0) {
         GLchan *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_CHAN(dst[dstRedIndex], rgba[i][RCOMP]);
            dst += dstComponents;
         }
      }

      if (dstGreenIndex >= 0) {
         GLchan *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_CHAN(dst[dstGreenIndex], rgba[i][GCOMP]);
            dst += dstComponents;
         }
      }

      if (dstBlueIndex >= 0) {
         GLchan *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_CHAN(dst[dstBlueIndex], rgba[i][BCOMP]);
            dst += dstComponents;
         }
      }

      if (dstAlphaIndex >= 0) {
         GLchan *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            CLAMPED_FLOAT_TO_CHAN(dst[dstAlphaIndex], rgba[i][ACOMP]);
            dst += dstComponents;
         }
      }

      if (dstIntensityIndex >= 0) {
         GLchan *dst = dest;
         GLuint i;
         assert(dstIntensityIndex == 0);
         assert(dstComponents == 1);
         for (i = 0; i < n; i++) {
            /* Intensity comes from red channel */
            CLAMPED_FLOAT_TO_CHAN(dst[i], rgba[i][RCOMP]);
         }
      }

      if (dstLuminanceIndex >= 0) {
         GLchan *dst = dest;
         GLuint i;
         assert(dstLuminanceIndex == 0);
         for (i = 0; i < n; i++) {
            /* Luminance comes from red channel */
            CLAMPED_FLOAT_TO_CHAN(dst[0], rgba[i][RCOMP]);
            dst += dstComponents;
         }
      }
      UNDEFARRAY(rgba);  /* mac 32k limitation */
   }
}


/**
 * Same as _mesa_unpack_color_span_chan(), but return GLfloat data
 * instead of GLchan.
 */
void
_mesa_unpack_color_span_float( GLcontext *ctx,
                               GLuint n, GLenum dstFormat, GLfloat dest[],
                               GLenum srcFormat, GLenum srcType,
                               const GLvoid *source,
                               const struct gl_pixelstore_attrib *srcPacking,
                               GLuint transferOps )
{
   ASSERT(dstFormat == GL_ALPHA ||
          dstFormat == GL_LUMINANCE ||
          dstFormat == GL_LUMINANCE_ALPHA ||
          dstFormat == GL_INTENSITY ||
          dstFormat == GL_RGB ||
          dstFormat == GL_RGBA ||
          dstFormat == GL_COLOR_INDEX);

   ASSERT(srcFormat == GL_RED ||
          srcFormat == GL_GREEN ||
          srcFormat == GL_BLUE ||
          srcFormat == GL_ALPHA ||
          srcFormat == GL_LUMINANCE ||
          srcFormat == GL_LUMINANCE_ALPHA ||
          srcFormat == GL_INTENSITY ||
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
          srcType == GL_UNSIGNED_INT_10_10_10_2 ||
          srcType == GL_UNSIGNED_INT_2_10_10_10_REV);

   /* general solution, no special cases, yet */
   {
      GLint dstComponents;
      GLint dstRedIndex, dstGreenIndex, dstBlueIndex, dstAlphaIndex;
      GLint dstLuminanceIndex, dstIntensityIndex;
      DEFMARRAY(GLfloat, rgba, MAX_WIDTH, 4);  /* mac 32k limitation */
      CHECKARRAY(rgba, return);  /* mac 32k limitation */

      dstComponents = _mesa_components_in_format( dstFormat );
      /* source & dest image formats should have been error checked by now */
      assert(dstComponents > 0);

      /*
       * Extract image data and convert to RGBA floats
       */
      assert(n <= MAX_WIDTH);
      if (srcFormat == GL_COLOR_INDEX) {
         GLuint indexes[MAX_WIDTH];
         extract_uint_indexes(n, indexes, srcFormat, srcType, source,
                              srcPacking);

         if (dstFormat == GL_COLOR_INDEX
             && (transferOps & IMAGE_MAP_COLOR_BIT)) {
            _mesa_map_ci(ctx, n, indexes);
         }
         if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
            _mesa_shift_and_offset_ci(ctx, n, indexes);
         }

         if (dstFormat == GL_COLOR_INDEX) {
            /* convert to GLchan and return */
            GLuint i;
            for (i = 0; i < n; i++) {
               dest[i] = (GLchan) (indexes[i] & 0xff);
            }
            UNDEFARRAY(rgba);  /* mac 32k limitation */
            return;
         }
         else {
            /* Convert indexes to RGBA */
            _mesa_map_ci_to_rgba(ctx, n, indexes, rgba);
         }

         /* Don't do RGBA scale/bias or RGBA->RGBA mapping if starting
          * with color indexes.
          */
         transferOps &= ~(IMAGE_SCALE_BIAS_BIT | IMAGE_MAP_COLOR_BIT);
      }
      else {
         /* non-color index data */
         extract_float_rgba(n, rgba, srcFormat, srcType, source,
                            srcPacking->SwapBytes);
      }

      if (transferOps) {
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, n, rgba);
      }

      /* Now determine which color channels we need to produce.
       * And determine the dest index (offset) within each color tuple.
       */
      switch (dstFormat) {
         case GL_ALPHA:
            dstAlphaIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = -1;
            dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         case GL_LUMINANCE:
            dstLuminanceIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = dstAlphaIndex = -1;
            dstIntensityIndex = -1;
            break;
         case GL_LUMINANCE_ALPHA:
            dstLuminanceIndex = 0;
            dstAlphaIndex = 1;
            dstRedIndex = dstGreenIndex = dstBlueIndex = -1;
            dstIntensityIndex = -1;
            break;
         case GL_INTENSITY:
            dstIntensityIndex = 0;
            dstRedIndex = dstGreenIndex = dstBlueIndex = dstAlphaIndex = -1;
            dstLuminanceIndex = -1;
            break;
         case GL_RGB:
            dstRedIndex = 0;
            dstGreenIndex = 1;
            dstBlueIndex = 2;
            dstAlphaIndex = dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         case GL_RGBA:
            dstRedIndex = 0;
            dstGreenIndex = 1;
            dstBlueIndex = 2;
            dstAlphaIndex = 3;
            dstLuminanceIndex = dstIntensityIndex = -1;
            break;
         default:
            _mesa_problem(ctx, "bad dstFormat in _mesa_unpack_color_span_float()");
            UNDEFARRAY(rgba);  /* mac 32k limitation */
            return;
      }

      /* Now pack results in the requested dstFormat */
      if (dstRedIndex >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstRedIndex] = rgba[i][RCOMP];
            dst += dstComponents;
         }
      }

      if (dstGreenIndex >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstGreenIndex] = rgba[i][GCOMP];
            dst += dstComponents;
         }
      }

      if (dstBlueIndex >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstBlueIndex] = rgba[i][BCOMP];
            dst += dstComponents;
         }
      }

      if (dstAlphaIndex >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[dstAlphaIndex] = rgba[i][ACOMP];
            dst += dstComponents;
         }
      }

      if (dstIntensityIndex >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         assert(dstIntensityIndex == 0);
         assert(dstComponents == 1);
         for (i = 0; i < n; i++) {
            /* Intensity comes from red channel */
            dst[i] = rgba[i][RCOMP];
         }
      }

      if (dstLuminanceIndex >= 0) {
         GLfloat *dst = dest;
         GLuint i;
         assert(dstLuminanceIndex == 0);
         for (i = 0; i < n; i++) {
            /* Luminance comes from red channel */
            dst[0] = rgba[i][RCOMP];
            dst += dstComponents;
         }
      }
      UNDEFARRAY(rgba);  /* mac 32k limitation */
   }
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
_mesa_unpack_index_span( const GLcontext *ctx, GLuint n,
                         GLenum dstType, GLvoid *dest,
                         GLenum srcType, const GLvoid *source,
                         const struct gl_pixelstore_attrib *srcPacking,
                         GLuint transferOps )
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
      MEMCPY(dest, source, n * sizeof(GLubyte));
   }
   else if (transferOps == 0 && srcType == GL_UNSIGNED_INT
            && dstType == GL_UNSIGNED_INT && !srcPacking->SwapBytes) {
      MEMCPY(dest, source, n * sizeof(GLuint));
   }
   else {
      /*
       * general solution
       */
      GLuint indexes[MAX_WIDTH];
      assert(n <= MAX_WIDTH);

      extract_uint_indexes(n, indexes, GL_COLOR_INDEX, srcType, source,
                           srcPacking);

      if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
         /* shift and offset indexes */
         _mesa_shift_and_offset_ci(ctx, n, indexes);
      }
      if (transferOps & IMAGE_MAP_COLOR_BIT) {
         /* Apply lookup table */
         _mesa_map_ci(ctx, n, indexes);
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
            MEMCPY(dest, indexes, n * sizeof(GLuint));
            break;
         default:
            _mesa_problem(ctx, "bad dstType in _mesa_unpack_index_span");
      }
   }
}


void
_mesa_pack_index_span( const GLcontext *ctx, GLuint n,
                       GLenum dstType, GLvoid *dest, const GLuint *source,
                       const struct gl_pixelstore_attrib *dstPacking,
                       GLuint transferOps )
{
   GLuint indexes[MAX_WIDTH];

   ASSERT(n <= MAX_WIDTH);

   transferOps &= (IMAGE_MAP_COLOR_BIT | IMAGE_SHIFT_OFFSET_BIT);

   if (transferOps & (IMAGE_MAP_COLOR_BIT | IMAGE_SHIFT_OFFSET_BIT)) {
      /* make a copy of input */
      MEMCPY(indexes, source, n * sizeof(GLuint));
      if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
         _mesa_shift_and_offset_ci( ctx, n, indexes);
      }
      if (transferOps & IMAGE_MAP_COLOR_BIT) {
         _mesa_map_ci(ctx, n, indexes);
      }
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
   case GL_HALF_FLOAT_ARB:
      {
         GLhalfARB *dst = (GLhalfARB *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = _mesa_float_to_half((GLfloat) source[i]);
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   default:
      _mesa_problem(ctx, "bad type in _mesa_pack_index_span");
   }
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
_mesa_unpack_stencil_span( const GLcontext *ctx, GLuint n,
                           GLenum dstType, GLvoid *dest,
                           GLenum srcType, const GLvoid *source,
                           const struct gl_pixelstore_attrib *srcPacking,
                           GLuint transferOps )
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
       srcType == GL_UNSIGNED_BYTE &&
       dstType == GL_UNSIGNED_BYTE) {
      MEMCPY(dest, source, n * sizeof(GLubyte));
   }
   else if (transferOps == 0 &&
            srcType == GL_UNSIGNED_INT &&
            dstType == GL_UNSIGNED_INT &&
            !srcPacking->SwapBytes) {
      MEMCPY(dest, source, n * sizeof(GLuint));
   }
   else {
      /*
       * general solution
       */
      GLuint indexes[MAX_WIDTH];
      assert(n <= MAX_WIDTH);

      extract_uint_indexes(n, indexes, GL_COLOR_INDEX, srcType, source,
                           srcPacking);

      if (transferOps) {
         if (transferOps & IMAGE_SHIFT_OFFSET_BIT) {
            /* shift and offset indexes */
            _mesa_shift_and_offset_ci(ctx, n, indexes);
         }

         if (ctx->Pixel.MapStencilFlag) {
            /* Apply stencil lookup table */
            GLuint mask = ctx->Pixel.MapStoSsize - 1;
            GLuint i;
            for (i=0;i<n;i++) {
               indexes[i] = ctx->Pixel.MapStoS[ indexes[i] & mask ];
            }
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
            MEMCPY(dest, indexes, n * sizeof(GLuint));
            break;
         default:
            _mesa_problem(ctx, "bad dstType in _mesa_unpack_stencil_span");
      }
   }
}


void
_mesa_pack_stencil_span( const GLcontext *ctx, GLuint n,
                         GLenum dstType, GLvoid *dest, const GLstencil *source,
                         const struct gl_pixelstore_attrib *dstPacking )
{
   GLstencil stencil[MAX_WIDTH];

   ASSERT(n <= MAX_WIDTH);

   if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset ||
       ctx->Pixel.MapStencilFlag) {
      /* make a copy of input */
      MEMCPY(stencil, source, n * sizeof(GLstencil));
      if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset) {
         _mesa_shift_and_offset_stencil( ctx, n, stencil );
      }
      if (ctx->Pixel.MapStencilFlag) {
         _mesa_map_stencil( ctx, n, stencil );
      }
      source = stencil;
   }

   switch (dstType) {
   case GL_UNSIGNED_BYTE:
      if (sizeof(GLstencil) == 8) {
         MEMCPY( dest, source, n );
      }
      else {
         GLubyte *dst = (GLubyte *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLubyte) source[i];
         }
      }
      break;
   case GL_BYTE:
      if (sizeof(GLstencil) == 8) {
         MEMCPY( dest, source, n );
      }
      else {
         GLbyte *dst = (GLbyte *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = (GLbyte) source[i];
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
            *dst++ = (GLint) source[i];
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
   case GL_HALF_FLOAT_ARB:
      {
         GLhalfARB *dst = (GLhalfARB *) dest;
         GLuint i;
         for (i=0;i<n;i++) {
            dst[i] = _mesa_float_to_half( (float) source[i] );
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
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
}


void
_mesa_unpack_depth_span( const GLcontext *ctx, GLuint n, GLfloat *dest,
                         GLenum srcType, const GLvoid *source,
                         const struct gl_pixelstore_attrib *srcPacking )
{
   (void) srcPacking;

   switch (srcType) {
      case GL_BYTE:
         {
            GLuint i;
            const GLubyte *src = (const GLubyte *) source;
            for (i = 0; i < n; i++) {
               dest[i] = BYTE_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_UNSIGNED_BYTE:
         {
            GLuint i;
            const GLubyte *src = (const GLubyte *) source;
            for (i = 0; i < n; i++) {
               dest[i] = UBYTE_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_SHORT:
         {
            GLuint i;
            const GLshort *src = (const GLshort *) source;
            for (i = 0; i < n; i++) {
               dest[i] = SHORT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_UNSIGNED_SHORT:
         {
            GLuint i;
            const GLushort *src = (const GLushort *) source;
            for (i = 0; i < n; i++) {
               dest[i] = USHORT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_INT:
         {
            GLuint i;
            const GLint *src = (const GLint *) source;
            for (i = 0; i < n; i++) {
               dest[i] = INT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint i;
            const GLuint *src = (const GLuint *) source;
            for (i = 0; i < n; i++) {
               dest[i] = UINT_TO_FLOAT(src[i]);
            }
         }
         break;
      case GL_FLOAT:
         MEMCPY(dest, source, n * sizeof(GLfloat));
         break;
      case GL_HALF_FLOAT_ARB:
         {
            GLuint i;
            const GLhalfARB *src = (const GLhalfARB *) source;
            for (i = 0; i < n; i++) {
               dest[i] = _mesa_half_to_float(src[i]);
            }
         }
         break;
      default:
         _mesa_problem(NULL, "bad type in _mesa_unpack_depth_span()");
         return;
   }


   /* apply depth scale and bias and clamp to [0,1] */
   if (ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         GLfloat d = dest[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         dest[i] = CLAMP(d, 0.0F, 1.0F);
      }
   }
}


/*
 * Pack an array of depth values.  The values are floats in [0,1].
 */
void
_mesa_pack_depth_span( const GLcontext *ctx, GLuint n, GLvoid *dest,
                       GLenum dstType, const GLfloat *depthSpan,
                       const struct gl_pixelstore_attrib *dstPacking )
{
   GLfloat depthCopy[MAX_WIDTH];
   const GLboolean bias_or_scale = ctx->Pixel.DepthBias != 0.0 ||
                                   ctx->Pixel.DepthScale != 1.0;

   ASSERT(n <= MAX_WIDTH);

   if (bias_or_scale) {
      GLuint i;
      for (i = 0; i < n; i++) {
         GLfloat d;
         d = depthSpan[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         depthCopy[i] = CLAMP(d, 0.0F, 1.0F);
      }
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
            dst[i] = FLOAT_TO_USHORT( depthSpan[i] );
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
   case GL_HALF_FLOAT_ARB:
      {
         GLhalfARB *dst = (GLhalfARB *) dest;
         GLuint i;
         for (i = 0; i < n; i++) {
            dst[i] = _mesa_float_to_half(depthSpan[i]);
         }
         if (dstPacking->SwapBytes) {
            _mesa_swap2( (GLushort *) dst, n );
         }
      }
      break;
   default:
      _mesa_problem(ctx, "bad type in _mesa_pack_depth_span");
   }
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

   if (format == GL_BITMAP) {
      bytesPerRow = (width + 7) >> 3;
      flipBytes = !unpack->LsbFirst;
      swap2 = swap4 = GL_FALSE;
      compsPerRow = 0;
   }
   else {
      const GLint bytesPerPixel = _mesa_bytes_per_pixel(format, type);
      const GLint components = _mesa_components_in_format(format);
      GLint bytesPerComp;
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
      GLubyte *destBuffer = (GLubyte *) MALLOC(bytesPerRow * height * depth);
      GLubyte *dst;
      GLint img, row;
      if (!destBuffer)
         return NULL;   /* generate GL_OUT_OF_MEMORY later */

      dst = destBuffer;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            const GLvoid *src = _mesa_image_address(dimensions, unpack, pixels,
                               width, height, format, type, img, row, 0);
            MEMCPY(dst, src, bytesPerRow);
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

#endif


/**
 * Perform clipping for glDrawPixels.  The image's window position
 * and size, and the unpack skipPixels and skipRows are adjusted so
 * that the image region is entirely within the window and scissor bounds.
 * NOTE: this will only work when glPixelZoom is (1, 1).
 *
 * \return  GL_TRUE if image is ready for drawing or
 *          GL_FALSE if image was completely clipped away (draw nothing)
 */
GLboolean
_mesa_clip_drawpixels(const GLcontext *ctx,
                      GLint *destX, GLint *destY,
                      GLsizei *width, GLsizei *height,
                      GLint *skipPixels, GLint *skipRows)
{
   const GLframebuffer *buffer = ctx->DrawBuffer;

   ASSERT(ctx->Pixel.ZoomX == 1.0F && ctx->Pixel.ZoomY == 1.0F);

   /* left clipping */
   if (*destX < buffer->_Xmin) {
      *skipPixels += (buffer->_Xmin - *destX);
      *width -= (buffer->_Xmin - *destX);
      *destX = buffer->_Xmin;
   }
   /* right clipping */
   if (*destX + *width > buffer->_Xmax)
      *width -= (*destX + *width - buffer->_Xmax);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*destY < buffer->_Ymin) {
      *skipRows += (buffer->_Ymin - *destY);
      *height -= (buffer->_Ymin - *destY);
      *destY = buffer->_Ymin;
   }
   /* top clipping */
   if (*destY + *height > buffer->_Ymax)
      *height -= (*destY + *height - buffer->_Ymax);

   if (*height <= 0)
      return GL_TRUE;

   return GL_TRUE;
}


/**
 * Perform clipping for glReadPixels.  The image's window position
 * and size, and the pack skipPixels and skipRows are adjusted so
 * that the image region is entirely within the window bounds.
 * Note: this is different from _mesa_clip_drawpixels() in that the
 * scissor box is ignored, and we use the bounds of the current "read"
 * surface;
 *
 * \return  GL_TRUE if image is ready for drawing or
 *          GL_FALSE if image was completely clipped away (draw nothing)
 */
GLboolean
_mesa_clip_readpixels(const GLcontext *ctx,
                      GLint *srcX, GLint *srcY,
                      GLsizei *width, GLsizei *height,
                      GLint *skipPixels, GLint *skipRows)
{
   const GLframebuffer *buffer = ctx->ReadBuffer;

   /* left clipping */
   if (*srcX < 0) {
      *skipPixels += (0 - *srcX);
      *width -= (0 - *srcX);
      *srcX = 0;
   }
   /* right clipping */
   if (*srcX + *width > (GLsizei) buffer->Width)
      *width -= (*srcX + *width - buffer->Width);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*srcY < 0) {
      *skipRows += (0 - *srcY);
      *height -= (0 - *srcY);
      *srcY = 0;
   }
   /* top clipping */
   if (*srcY + *height > (GLsizei) buffer->Height)
      *height -= (*srcY + *height - buffer->Height);

   if (*height <= 0)
      return GL_TRUE;

   return GL_TRUE;
}

