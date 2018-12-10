/* $Id: image.c,v 1.19 1997/11/07 03:49:04 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: image.c,v $
 * Revision 1.19  1997/11/07 03:49:04  brianp
 * more error checking work (but more to be done)
 *
 * Revision 1.18  1997/11/02 20:19:47  brianp
 * added more error checking to gl_unpack_image3D()
 *
 * Revision 1.17  1997/10/16 01:04:51  brianp
 * added code to normalize color, depth values in gl_unpack_image3d()
 *
 * Revision 1.16  1997/09/27 00:15:39  brianp
 * changed parameters to gl_unpack_image()
 *
 * Revision 1.15  1997/08/11 01:23:10  brianp
 * added a pointer cast
 *
 * Revision 1.14  1997/07/24 01:25:18  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.13  1997/05/28 03:25:26  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.12  1997/04/29 01:26:25  brianp
 * added #include "context.h"
 *
 * Revision 1.11  1997/04/20 20:28:49  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.10  1997/04/06 17:49:32  brianp
 * image reference count wasn't always initialized to zero (Christopher Lloyd)
 *
 * Revision 1.9  1997/02/09 20:05:03  brianp
 * new arguments for gl_pixel_addr_in_image()
 *
 * Revision 1.8  1997/02/09 18:52:53  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.7  1997/01/09 21:25:54  brianp
 * initialize image reference count to zero
 *
 * Revision 1.6  1996/11/13 03:58:31  brianp
 * fixed undefined "format" variable in gl_unpack_image()
 *
 * Revision 1.5  1996/11/10 17:48:03  brianp
 * check if format is GL_DEPTH_COMPONENT or GL_STENCIL_COMPONENT
 *
 * Revision 1.4  1996/11/06 04:23:01  brianp
 * changed gl_unpack_image() components argument to srcFormat
 *
 * Revision 1.3  1996/09/27 01:27:10  brianp
 * removed unused variables
 *
 * Revision 1.2  1996/09/26 22:35:10  brianp
 * fixed a few compiler warnings from IRIX 6 -n32 and -64 compiler
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "image.h"
#include "macros.h"
#include "pixel.h"
#include "types.h"
#endif



/*
 * Flip the 8 bits in each byte of the given array.
 */
void gl_flip_bytes( GLubyte *p, GLuint n )
{
   register GLuint i, a, b;

   for (i=0;i<n;i++) {
      b = (GLuint) p[i];
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
 * Flip the order of the 2 bytes in each word in the given array.
 */
void gl_swap2( GLushort *p, GLuint n )
{
   register GLuint i;

   for (i=0;i<n;i++) {
      p[i] = (p[i] >> 8) | ((p[i] << 8) & 0xff00);
   }
}



/*
 * Flip the order of the 4 bytes in each word in the given array.
 */
void gl_swap4( GLuint *p, GLuint n )
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




/*
 * Return the size, in bytes, of the given GL datatype.
 * Return 0 if GL_BITMAP.
 * Return -1 if invalid type enum.
 */
GLint gl_sizeof_type( GLenum type )
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
      default:
         return -1;
   }
}



/*
 * Return the number of components in a GL enum pixel type.
 * Return -1 if bad format.
 */
GLint gl_components_in_format( GLenum format )
{
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
         return 1;
      case GL_LUMINANCE_ALPHA:
	 return 2;
      case GL_RGB:
      case GL_BGR_EXT:
	 return 3;
      case GL_RGBA:
      case GL_BGRA_EXT:
	 return 4;
      default:
         return -1;
   }
}


/*
 * Return the address of a pixel in an image (actually a volume).
 * Pixel unpacking/packing parameters are observed according to 'packing'.
 * Input:  image - start of image data
 *         width, height - size of image
 *         format - image format
 *         type - pixel component type
 *         packing - GL_TRUE = use packing params
 *                   GL_FALSE = use unpacking params.
 *         img - which image in the volume (0 for 2-D images)
 *         row, column - location of pixel in the image
 * Return:  address of pixel at (image,row,column) in image or NULL if error.
 */
GLvoid *gl_pixel_addr_in_image( struct gl_pixelstore_attrib *packing,
                                const GLvoid *image, GLsizei width,
                                GLsizei height, GLenum format, GLenum type,
                                GLint row)
{
   GLint bytes_per_comp;   /* bytes per component */
   GLint comp_per_pixel;   /* components per pixel */
   GLint comps_per_row;    /* components per row */
   GLint pixels_per_row;   /* pixels per row */
   GLint alignment;        /* 1, 2 or 4 */
   GLint skiprows;
   GLint skippixels;
   GLubyte *pixel_addr;

   /* Compute bytes per component */
   bytes_per_comp = gl_sizeof_type( type );
   if (bytes_per_comp<0) {
      return NULL;
   }

   /* Compute number of components per pixel */
   comp_per_pixel = gl_components_in_format( format );
   if (comp_per_pixel<0) {
      return NULL;
   }

   alignment = packing->Alignment;
   if (packing->RowLength>0) {
      pixels_per_row = packing->RowLength;
   }
   else {
      pixels_per_row = width;
   }
   skiprows = packing->SkipRows;
   skippixels = packing->SkipPixels;

   if (type==GL_BITMAP) {
      /* BITMAP data */
      GLint bytes_per_row;

      bytes_per_row = alignment
                    * CEILING( comp_per_pixel*pixels_per_row, 8*alignment );

      pixel_addr = (GLubyte *) image
                 + (skiprows + row) * bytes_per_row
                 + (skippixels) / 8;
   }
   else {
      /* Non-BITMAP data */

      if (bytes_per_comp>=alignment) {
	 comps_per_row = comp_per_pixel * pixels_per_row;
      }
      else {
         GLint bytes_per_row = bytes_per_comp * comp_per_pixel
                             * pixels_per_row;

	 comps_per_row = alignment / bytes_per_comp
                       * CEILING( bytes_per_row, alignment );
      }

      /* Copy/unpack pixel data to buffer */
      pixel_addr = (GLubyte *) image
                 + (skiprows + row) * bytes_per_comp * comps_per_row
                 + (skippixels) * bytes_per_comp * comp_per_pixel;
   }

   return (GLvoid *) pixel_addr;
}



/*
 * Unpack a 2-D image from user-supplied address, returning a pointer to
 * a new gl_image struct.
 *
 * Input:  width, height - size in pixels
 *         srcFormat - format of incoming pixel data, ignored if
 *                     srcType BITMAP.
 *         srcType - GL_UNSIGNED_BYTE .. GL_FLOAT
 *         pixels - pointer to unpacked image in client memory space.
 */
struct gl_image *gl_unpack_image( GLcontext *ctx,
                                  GLint width, GLint height,
                                  GLenum srcFormat, GLenum srcType,
                                  const GLvoid *pixels )
{
   GLint components;
   GLenum destType;

   if (srcType==GL_UNSIGNED_BYTE) {
      destType = GL_UNSIGNED_BYTE;
   }
   else if (srcType==GL_BITMAP) {
      destType = GL_BITMAP;
   }
   else {
      destType = GL_FLOAT;
   }

   components = gl_components_in_format( srcFormat );

   if (components < 0)
      return NULL;

   if (srcType==GL_BITMAP || destType==GL_BITMAP) {
      struct gl_image *image;
      GLint bytes, i, width_in_bytes;
      GLubyte *buffer, *dst;
      assert( srcType==GL_BITMAP );
      assert( destType==GL_BITMAP );

      /* Alloc dest storage */
      if (width > 0 && height > 0)
         bytes = ((width+7)/8 * height);
      else
         bytes = 0;
      if (bytes>0 && pixels!=NULL) {
         buffer = (GLubyte *) malloc( bytes );
         if (!buffer) {
            return NULL;
         }
         /* Copy/unpack pixel data to buffer */
         width_in_bytes = CEILING( width, 8 );
         dst = buffer;
         for (i=0; i<height; i++) {
            GLvoid *src = gl_pixel_addr_in_image( &ctx->Unpack, pixels,
                                                  width, height,
                                                  GL_COLOR_INDEX, srcType,
                                                  i);
            if (!src) {
               free(buffer);
               return NULL;
            }
            MEMCPY( dst, src, width_in_bytes );
            dst += width_in_bytes;
         }
         /* Bit flipping */
         if (ctx->Unpack.LsbFirst) {
            gl_flip_bytes( buffer, bytes );
         }
      }
      else {
         /* a 'null' bitmap */
         buffer = NULL;
      }

      image = (struct gl_image *) malloc( sizeof(struct gl_image) );
      if (image) {
         image->Width = width;
         image->Height = height;
         image->Components = 0;
         image->Format = GL_COLOR_INDEX;
         image->Type = GL_BITMAP;
         image->Data = buffer;
         image->RefCount = 0;
      }
      else {
         if (buffer)
            free( buffer );
         return NULL;
      }
      return image;
   }
   else if (srcFormat==GL_DEPTH_COMPONENT) {
      /* TODO: pack as GLdepth values (GLushort or GLuint) */

   }
   else if (srcFormat==GL_STENCIL_INDEX) {
      /* TODO: pack as GLstencil (GLubyte or GLushort) */

   }
   else if (destType==GL_UNSIGNED_BYTE) {
      struct gl_image *image;
      GLint width_in_bytes;
      GLubyte *buffer, *dst;
      GLint i;
      assert( srcType==GL_UNSIGNED_BYTE );

      width_in_bytes = width * components * sizeof(GLubyte);
      buffer = (GLubyte *) malloc( height * width_in_bytes );
      if (!buffer) {
         return NULL;
      }
      /* Copy/unpack pixel data to buffer */
      dst = buffer;
      for (i=0;i<height;i++) {
         GLubyte *src = (GLubyte *) gl_pixel_addr_in_image( &ctx->Unpack,
                        pixels, width, height, srcFormat, srcType, i);
         if (!src) {
            free(buffer);
            return NULL;
         }
         MEMCPY( dst, src, width_in_bytes );
         dst += width_in_bytes;
      }

      if (ctx->Unpack.LsbFirst) {
         gl_flip_bytes( buffer, height * width_in_bytes );
      }

      image = (struct gl_image *) malloc( sizeof(struct gl_image) );
      if (image) {
         image->Width = width;
         image->Height = height;
         image->Components = components;
         image->Format = srcFormat;
         image->Type = GL_UNSIGNED_BYTE;
         image->Data = buffer;
         image->RefCount = 0;
      }
      else {
         free( buffer );
         return NULL;
      }
      return image;
   }
   else if (destType==GL_FLOAT) {
      struct gl_image *image;
      GLfloat *buffer, *dst;
      GLint elems_per_row;
      GLint i, j;
      GLboolean normalize;
      elems_per_row = width * components;
      buffer = (GLfloat *) malloc( height * elems_per_row * sizeof(GLfloat));
      if (!buffer) {
         return NULL;
      }

      normalize = (srcFormat != GL_COLOR_INDEX)
               && (srcFormat != GL_STENCIL_INDEX);

      dst = buffer;
      /**      img_pixels= pixels;*/
      for (i=0;i<height;i++) {
         GLvoid *src = gl_pixel_addr_in_image( &ctx->Unpack, pixels,
                                               width, height,
                                               srcFormat, srcType,
                                               i);
         if (!src) {
            free(buffer);
            return NULL;
         }

         switch (srcType) {
            case GL_UNSIGNED_BYTE:
               if (normalize) {
                  for (j=0;j<elems_per_row;j++) {
                     *dst++ = UBYTE_TO_FLOAT(((GLubyte*)src)[j]);
                  }
               }
               else {
                  for (j=0;j<elems_per_row;j++) {
                     *dst++ = (GLfloat) ((GLubyte*)src)[j];
                  }
               }
               break;
            case GL_BYTE:
               if (normalize) {
                  for (j=0;j<elems_per_row;j++) {
                     *dst++ = BYTE_TO_FLOAT(((GLbyte*)src)[j]);
                  }
               }
               else {
                  for (j=0;j<elems_per_row;j++) {
                     *dst++ = (GLfloat) ((GLbyte*)src)[j];
                  }
               }
               break;
            case GL_UNSIGNED_SHORT:
               if (ctx->Unpack.SwapBytes) {
                  for (j=0;j<elems_per_row;j++) {
                     GLushort value = ((GLushort*)src)[j];
                     value = ((value >> 8) & 0xff) | ((value&0xff) << 8);
                     if (normalize) {
                        *dst++ = USHORT_TO_FLOAT(value);
                     }
                     else {
                        *dst++ = (GLfloat) value;
                     }
                  }
               }
               else {
                  if (normalize) {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = USHORT_TO_FLOAT(((GLushort*)src)[j]);
                     }
                  }
                  else {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = (GLfloat) ((GLushort*)src)[j];
                     }
                  }
               }
               break;
            case GL_SHORT:
               if (ctx->Unpack.SwapBytes) {
                  for (j=0;j<elems_per_row;j++) {
                     GLshort value = ((GLshort*)src)[j];
                     value = ((value >> 8) & 0xff) | ((value&0xff) << 8);
                     if (normalize) {
                        *dst++ = SHORT_TO_FLOAT(value);
                     }
                     else {
                        *dst++ = (GLfloat) value;
                     }
                  }
               }
               else {
                  if (normalize) {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = SHORT_TO_FLOAT(((GLshort*)src)[j]);
                     }
                  }
                  else {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = (GLfloat) ((GLshort*)src)[j];
                     }
                  }
               }
               break;
            case GL_UNSIGNED_INT:
               if (ctx->Unpack.SwapBytes) {
                  GLuint value;
                  for (j=0;j<elems_per_row;j++) {
                     value = ((GLuint*)src)[j];
                     value = ((value & 0xff000000) >> 24)
                           | ((value & 0x00ff0000) >> 8)
                           | ((value & 0x0000ff00) << 8)
                           | ((value & 0x000000ff) << 24);
                     if (normalize) {
                        *dst++ = UINT_TO_FLOAT(value);
                     }
                     else {
                        *dst++ = (GLfloat) value;
                     }
                  }
               }
               else {
                  if (normalize) {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = UINT_TO_FLOAT(((GLuint*)src)[j]);
                     }
                  }
                  else {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = (GLfloat) ((GLuint*)src)[j];
                     }
                  }
               }
               break;
            case GL_INT:
               if (ctx->Unpack.SwapBytes) {
                  GLint value;
                  for (j=0;j<elems_per_row;j++) {
                     value = ((GLint*)src)[j];
                     value = ((value & 0xff000000) >> 24)
                           | ((value & 0x00ff0000) >> 8)
                           | ((value & 0x0000ff00) << 8)
                           | ((value & 0x000000ff) << 24);
                     if (normalize) {
                        *dst++ = INT_TO_FLOAT(value);
                     }
                     else {
                        *dst++ = (GLfloat) value;
                     }
                  }
               }
               else {
                  if (normalize) {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = INT_TO_FLOAT(((GLint*)src)[j]);
                     }
                  }
                  else {
                     for (j=0;j<elems_per_row;j++) {
                        *dst++ = (GLfloat) ((GLint*)src)[j];
                     }
                  }
               }
               break;
            case GL_FLOAT:
               if (ctx->Unpack.SwapBytes) {
                  GLint value;
                  for (j=0;j<elems_per_row;j++) {
                     value = ((GLuint*)src)[j];
                     value = ((value & 0xff000000) >> 24)
                           | ((value & 0x00ff0000) >> 8)
                           | ((value & 0x0000ff00) << 8)
                           | ((value & 0x000000ff) << 24);
                     *dst++ = *((GLfloat*) &value);
                  }
               }
               else {
                  MEMCPY( dst, src, elems_per_row*sizeof(GLfloat) );
                  dst += elems_per_row;
               }
               break;
            default:
               gl_problem(ctx, "Bad type in gl_unpack_image3D");
               return NULL;
         } /*switch*/
      } /* for height */

      image = (struct gl_image *) malloc( sizeof(struct gl_image) );
      if (image) {
         image->Width = width;
         image->Height = height;
         image->Components = components;
         image->Format = srcFormat;
         image->Type = GL_FLOAT;
         image->Data = buffer;
         image->RefCount = 0;
      }
      else {
         free( buffer );
         return NULL;
      }
      return image;
   }
   else {
      gl_problem(ctx, "Bad dest type in gl_unpack_image3D");
      return NULL;
   }
   return NULL;  /* never get here */
}



void gl_free_image( struct gl_image *image )
{
   if (image->Data) {
      free(image->Data);
   }
   free(image);
}
