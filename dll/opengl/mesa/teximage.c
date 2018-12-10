/* $Id: teximage.c,v 1.35 1998/02/07 14:36:41 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
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
 * $Log: teximage.c,v $
 * Revision 1.35  1998/02/07 14:36:41  brianp
 * fixed bug when passing NULL proxy image to glTexImageXD() (Wes Bethel)
 *
 * Revision 1.34  1998/01/16 03:46:07  brianp
 * fixed a few Windows compilation warnings (Theodore Jump)
 *
 * Revision 1.33  1997/12/31 06:10:03  brianp
 * added Henk Kok's texture validation optimization (AnyDirty flag)
 *
 * Revision 1.32  1997/12/07 17:30:39  brianp
 * added DavidB's patches for v0.21 driver (TexSubImage)
 *
 * Revision 1.31  1997/11/07 03:38:07  brianp
 * added stdio.h include for SunOS 4.x
 *
 * Revision 1.30  1997/11/02 20:20:30  brianp
 * rewrote gl_TexSubImage[123]D()
 *
 * Revision 1.29  1997/10/16 01:14:04  brianp
 * removed teximage Dirty flag
 *
 * Revision 1.28  1997/10/14 00:40:20  brianp
 * added DavidB's v19 fxmesa changes
 *
 * Revision 1.27  1997/09/29 23:28:14  brianp
 * updated for new device driver texture functions
 *
 * Revision 1.26  1997/09/28 15:29:03  brianp
 * initialize image->Depth = 1 in read_color_image() (Hiroki Honda)
 *
 * Revision 1.25  1997/09/27 00:14:39  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.24  1997/09/03 13:17:17  brianp
 * added a few pointer casts
 *
 * Revision 1.23  1997/08/23 18:40:46  brianp
 * glTexImage[123]D() with NULL image pointer is correctly handled now
 *
 * Revision 1.22  1997/08/11 01:23:29  brianp
 * added a few pointer casts
 *
 * Revision 1.21  1997/07/24 01:25:34  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.20  1997/07/05 16:21:17  brianp
 * fixed unitialized variable bug in gl_TexSubImage1D()
 *
 * Revision 1.19  1997/06/24 01:13:53  brianp
 * call gl_free_image() in gl_TexSubImage[123]D() if ref count==0
 *
 * Revision 1.18  1997/06/04 00:33:14  brianp
 * fixed reference count bug in gl_CopyTexImage1/2D() (Randy Frank)
 *
 * Revision 1.17  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.16  1997/05/03 00:52:19  brianp
 * set texture object Dirty flag when changing texture image
 *
 * Revision 1.15  1997/04/20 20:29:11  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.14  1997/03/04 19:18:29  brianp
 * added texture image Width2, Height2, and Depth2 fields
 *
 * Revision 1.13  1997/02/27 19:58:08  brianp
 * call gl_problem() instead of gl_warning()
 *
 * Revision 1.12  1997/02/09 18:53:05  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.11  1997/01/16 03:35:10  brianp
 * added calls to device driver TexImage() function
 *
 * Revision 1.10  1997/01/09 21:26:46  brianp
 * gl_TexImage[12]D() didn't free the incoming image- a memory leak
 *
 * Revision 1.9  1997/01/09 19:55:52  brianp
 * fixed a few error messages
 *
 * Revision 1.8  1997/01/09 19:49:18  brianp
 * better error checking
 *
 * Revision 1.7  1996/12/02 18:59:54  brianp
 * added code to handle GL_COLOR_INDEX textures, per Randy Frank
 *
 * Revision 1.6  1996/11/07 04:13:24  brianp
 * all new texture image handling, now pixel scale, bias, mapping work
 *
 * Revision 1.5  1996/09/27 01:29:57  brianp
 * removed unused variables, fixed cut&paste bug in color scaling
 *
 * Revision 1.4  1996/09/26 22:35:10  brianp
 * fixed a few compiler warnings from IRIX 6 -n32 and -64 compiler
 *
 * Revision 1.3  1996/09/15 14:18:55  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.2  1996/09/15 01:48:58  brianp
 * removed #define NULL 0
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
#include "span.h"
#include "teximage.h"
#include "types.h"
#endif


/*
 * NOTES:
 *
 * The internal texture storage convension is an array of N GLubytes
 * where N = width * height * components.  There is no padding.
 */




/*
 * Compute log base 2 of n.
 * If n isn't an exact power of two return -1.
 * If n<0 return -1.
 */
static int logbase2( int n )
{
   GLint i = 1;
   GLint log2 = 0;

   if (n<0) {
      return -1;
   }

   while ( n > i ) {
      i *= 2;
      log2++;
   }
   if (i != n) {
      return -1;
   }
   else {
      return log2;
   }
}



/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.  Return -1 if
 * invalid enum.
 */
static GLint decode_internal_format( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return GL_COLOR_INDEX;
      default:
         return -1;  /* error */
   }
}



/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.  Return the
 * number of components for the format.  Return -1 if invalid enum.
 */
static GLint components_in_intformat( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return 1;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return 1;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return 2;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return 1;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return 3;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return 4;
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return 1;
      default:
         return -1;  /* error */
   }
}



struct gl_texture_image *gl_alloc_texture_image( void )
{
   return (struct gl_texture_image *) calloc( 1, sizeof(struct gl_texture_image) );
}



void gl_free_texture_image( struct gl_texture_image *teximage )
{
   if (teximage->Data) {
      free( teximage->Data );
   }
   free( teximage );
}



/*
 * Given a gl_image, apply the pixel transfer scale, bias, and mapping
 * to produce a gl_texture_image.  Convert image data to GLubytes.
 * Input:  image - the incoming gl_image
 *         internalFormat - desired format of resultant texture
 *         border - texture border width (0 or 1)
 * Return:  pointer to a gl_texture_image or NULL if an error occurs.
 */
static struct gl_texture_image *
image_to_texture( GLcontext *ctx, const struct gl_image *image,
                  GLenum internalFormat, GLint border )
{
   GLint components;
   struct gl_texture_image *texImage;
   GLint numPixels, pixel;
   GLboolean scaleOrBias;

   assert(image);
   assert(image->Width>0);
   assert(image->Height>0);

   /*   internalFormat = decode_internal_format(internalFormat);*/
   components = components_in_intformat(internalFormat);
   numPixels = image->Width * image->Height;

   texImage = gl_alloc_texture_image();
   if (!texImage)
      return NULL;

   texImage->Format = decode_internal_format(internalFormat);
   texImage->IntFormat = internalFormat;
   texImage->Border = border;
   texImage->Width = image->Width;
   texImage->Height = image->Height;
   texImage->WidthLog2 = logbase2(image->Width - 2*border);
   if (image->Height==1)  /* 1-D texture */
      texImage->HeightLog2 = 0;
   else
      texImage->HeightLog2 = logbase2(image->Height - 2*border);
   texImage->Width2 = 1 << texImage->WidthLog2;
   texImage->Height2 = 1 << texImage->HeightLog2;
   texImage->MaxLog2 = MAX2( texImage->WidthLog2, texImage->HeightLog2 );
   texImage->Data = (GLubyte *) malloc( numPixels * components );

   assert(texImage->WidthLog2>=0);
   assert(texImage->HeightLog2>=0);

   if (!texImage->Data) {
      /* out of memory */
      gl_free_texture_image( texImage );
      return NULL;
   }

   /* Determine if scaling and/or biasing is needed */
   if (ctx->Pixel.RedScale!=1.0F   || ctx->Pixel.RedBias!=0.0F ||
       ctx->Pixel.GreenScale!=1.0F || ctx->Pixel.GreenBias!=0.0F ||
       ctx->Pixel.BlueScale!=1.0F  || ctx->Pixel.BlueBias!=0.0F ||
       ctx->Pixel.AlphaScale!=1.0F || ctx->Pixel.AlphaBias!=0.0F) {
      scaleOrBias = GL_TRUE;
   }
   else {
      scaleOrBias = GL_FALSE;
   }

   switch (image->Type) {
      case GL_BITMAP:
         {
            GLint shift = ctx->Pixel.IndexShift;
            GLint offset = ctx->Pixel.IndexOffset;
            /* MapIto[RGBA]Size must be powers of two */
            GLint rMask = ctx->Pixel.MapItoRsize-1;
            GLint gMask = ctx->Pixel.MapItoGsize-1;
            GLint bMask = ctx->Pixel.MapItoBsize-1;
            GLint aMask = ctx->Pixel.MapItoAsize-1;
            GLint i, j;
            GLubyte *srcPtr = (GLubyte *) image->Data;

            assert( image->Format==GL_COLOR_INDEX );

            for (j=0; j<image->Height; j++) {
               GLubyte bitMask = 128;
               for (i=0; i<image->Width; i++) {
                  GLint index;
                  GLubyte red, green, blue, alpha;

                  /* Fetch image color index */
                  index = (*srcPtr & bitMask) ? 1 : 0;
                  bitMask = bitMask >> 1;
                  if (bitMask==0) {
                     bitMask = 128;
                     srcPtr++;
                  }
                  /* apply index shift and offset */
                  if (shift>=0) {
                     index = (index << shift) + offset;
                  }
                  else {
                     index = (index >> -shift) + offset;
                  }
                  /* convert index to RGBA */
                  red   = (GLint) (ctx->Pixel.MapItoR[index & rMask] * 255.0F);
                  green = (GLint) (ctx->Pixel.MapItoG[index & gMask] * 255.0F);
                  blue  = (GLint) (ctx->Pixel.MapItoB[index & bMask] * 255.0F);
                  alpha = (GLint) (ctx->Pixel.MapItoA[index & aMask] * 255.0F);

                  /* store texel (components are GLubytes in [0,255]) */
                  pixel = j * image->Width + i;
                  switch (texImage->Format) {
                     case GL_ALPHA:
                        texImage->Data[pixel] = alpha;
                        break;
                     case GL_LUMINANCE:
                        texImage->Data[pixel] = red;
                        break;
                     case GL_LUMINANCE_ALPHA:
                        texImage->Data[pixel*2+0] = red;
                        texImage->Data[pixel*2+1] = alpha;
                        break;
                     case GL_INTENSITY:
                        texImage->Data[pixel] = red;
                        break;
                     case GL_RGB:
                        texImage->Data[pixel*3+0] = red;
                        texImage->Data[pixel*3+1] = green;
                        texImage->Data[pixel*3+2] = blue;
                        break;
                     case GL_RGBA:
                        texImage->Data[pixel*4+0] = red;
                        texImage->Data[pixel*4+1] = green;
                        texImage->Data[pixel*4+2] = blue;
                        texImage->Data[pixel*4+3] = alpha;
                        break;
                     default:
                        gl_problem(ctx,"Bad format in image_to_texture");
                        return NULL;
                  }
               }
               if (bitMask!=128) {
                  srcPtr++;
               }
            }
         }
         break;

      case GL_UNSIGNED_BYTE:
         for (pixel=0; pixel<numPixels; pixel++) {
            GLubyte red, green, blue, alpha;
            switch (image->Format) {
               case GL_COLOR_INDEX:
                  if (decode_internal_format(internalFormat)==GL_COLOR_INDEX) {
                     /* a paletted texture */
                     GLint index = ((GLubyte*)image->Data)[pixel];
                     red = index;
                  }
                  else {
                     /* convert color index to RGBA */
                     GLint index = ((GLubyte*)image->Data)[pixel];
                     red   = 255.0F * ctx->Pixel.MapItoR[index];
                     green = 255.0F * ctx->Pixel.MapItoG[index];
                     blue  = 255.0F * ctx->Pixel.MapItoB[index];
                     alpha = 255.0F * ctx->Pixel.MapItoA[index];
                  }
                  break;
               case GL_RGB:
                  /* Fetch image RGBA values */
                  red   = ((GLubyte*) image->Data)[pixel*3+0];
                  green = ((GLubyte*) image->Data)[pixel*3+1];
                  blue  = ((GLubyte*) image->Data)[pixel*3+2];
                  alpha = 255;
                  break;
               case GL_BGR_EXT:
                   blue  = ((GLubyte*) image->Data)[pixel*3+0];
                   green = ((GLubyte*) image->Data)[pixel*3+1];
                   red   = ((GLubyte*) image->Data)[pixel*3+2];
                   alpha = 255;
                 break;
               case GL_RGBA:
                  red   = ((GLubyte*) image->Data)[pixel*4+0];
                  green = ((GLubyte*) image->Data)[pixel*4+1];
                  blue  = ((GLubyte*) image->Data)[pixel*4+2];
                  alpha = ((GLubyte*) image->Data)[pixel*4+3];
                  break;
               case GL_BGRA_EXT:
                  blue  = ((GLubyte*) image->Data)[pixel*4+0];
                  green = ((GLubyte*) image->Data)[pixel*4+1];
                  red   = ((GLubyte*) image->Data)[pixel*4+2];
                  alpha = ((GLubyte*) image->Data)[pixel*4+3];
                  break;
               case GL_RED:
                  red   = ((GLubyte*) image->Data)[pixel];
                  green = 0;
                  blue  = 0;
                  alpha = 255;
                  break;
               case GL_GREEN:
                  red   = 0;
                  green = ((GLubyte*) image->Data)[pixel];
                  blue  = 0;
                  alpha = 255;
                  break;
               case GL_BLUE:
                  red   = 0;
                  green = 0;
                  blue  = ((GLubyte*) image->Data)[pixel];
                  alpha = 255;
                  break;
               case GL_ALPHA:
                  red   = 0;
                  green = 0;
                  blue  = 0;
                  alpha = ((GLubyte*) image->Data)[pixel];
                  break;
               case GL_LUMINANCE: 
                  red   = ((GLubyte*) image->Data)[pixel];
                  green = red;
                  blue  = red;
                  alpha = 255;
                  break;
              case GL_LUMINANCE_ALPHA:
                  red   = ((GLubyte*) image->Data)[pixel*2+0];
                  green = red;
                  blue  = red;
                  alpha = ((GLubyte*) image->Data)[pixel*2+1];
                  break;
              default:
                 gl_problem(ctx,"Bad format (2) in image_to_texture");
                 return NULL;
            }
            
            if (scaleOrBias || ctx->Pixel.MapColorFlag) {
               /* Apply RGBA scale and bias */
               GLfloat r = red   * (1.0F/255.0F);
               GLfloat g = green * (1.0F/255.0F);
               GLfloat b = blue  * (1.0F/255.0F);
               GLfloat a = alpha * (1.0F/255.0F);
               if (scaleOrBias) {
                  /* r,g,b,a now in [0,1] */
                  r = r * ctx->Pixel.RedScale   + ctx->Pixel.RedBias;
                  g = g * ctx->Pixel.GreenScale + ctx->Pixel.GreenBias;
                  b = b * ctx->Pixel.BlueScale  + ctx->Pixel.BlueBias;
                  a = a * ctx->Pixel.AlphaScale + ctx->Pixel.AlphaBias;
                  r = CLAMP( r, 0.0F, 1.0F );
                  g = CLAMP( g, 0.0F, 1.0F );
                  b = CLAMP( b, 0.0F, 1.0F );
                  a = CLAMP( a, 0.0F, 1.0F );
               }
               /* Apply pixel maps */
               if (ctx->Pixel.MapColorFlag) {
                  GLint ir = (GLint) (r*ctx->Pixel.MapRtoRsize);
                  GLint ig = (GLint) (g*ctx->Pixel.MapGtoGsize);
                  GLint ib = (GLint) (b*ctx->Pixel.MapBtoBsize);
                  GLint ia = (GLint) (a*ctx->Pixel.MapAtoAsize);
                  r = ctx->Pixel.MapRtoR[ir];
                  g = ctx->Pixel.MapGtoG[ig];
                  b = ctx->Pixel.MapBtoB[ib];
                  a = ctx->Pixel.MapAtoA[ia];
               }
               red   = (GLint) (r * 255.0F);
               green = (GLint) (g * 255.0F);
               blue  = (GLint) (b * 255.0F);
               alpha = (GLint) (a * 255.0F);
            }

            /* store texel (components are GLubytes in [0,255]) */
            switch (texImage->Format) {
               case GL_COLOR_INDEX:
                  texImage->Data[pixel] = red; /* really an index */
                  break;
               case GL_ALPHA:
                  texImage->Data[pixel] = alpha;
                  break;
               case GL_LUMINANCE:
                  texImage->Data[pixel] = red;
                  break;
               case GL_LUMINANCE_ALPHA:
                  texImage->Data[pixel*2+0] = red;
                  texImage->Data[pixel*2+1] = alpha;
                  break;
               case GL_INTENSITY:
                  texImage->Data[pixel] = red;
                  break;
               case GL_RGB:
                  texImage->Data[pixel*3+0] = red;
                  texImage->Data[pixel*3+1] = green;
                  texImage->Data[pixel*3+2] = blue;
                  break;
               case GL_RGBA:
                  texImage->Data[pixel*4+0] = red;
                  texImage->Data[pixel*4+1] = green;
                  texImage->Data[pixel*4+2] = blue;
                  texImage->Data[pixel*4+3] = alpha;
                  break;
               default:
                  gl_problem(ctx,"Bad format (3) in image_to_texture");
                  return NULL;
            }
         }
         break;

      case GL_FLOAT:
         for (pixel=0; pixel<numPixels; pixel++) {
            GLfloat red, green, blue, alpha;
            switch (texImage->Format) {
               case GL_COLOR_INDEX:
                  if (decode_internal_format(internalFormat)==GL_COLOR_INDEX) {
                     /* a paletted texture */
                     GLint index = (GLint) ((GLfloat*) image->Data)[pixel];
                     red = index;
                  }
                  else {
                     GLint shift = ctx->Pixel.IndexShift;
                     GLint offset = ctx->Pixel.IndexOffset;
                     /* MapIto[RGBA]Size must be powers of two */
                     GLint rMask = ctx->Pixel.MapItoRsize-1;
                     GLint gMask = ctx->Pixel.MapItoGsize-1;
                     GLint bMask = ctx->Pixel.MapItoBsize-1;
                     GLint aMask = ctx->Pixel.MapItoAsize-1;
                     /* Fetch image color index */
                     GLint index = (GLint) ((GLfloat*) image->Data)[pixel];
                     /* apply index shift and offset */
                     if (shift>=0) {
                        index = (index << shift) + offset;
                     }
                     else {
                        index = (index >> -shift) + offset;
                     }
                     /* convert index to RGBA */
                     red   = ctx->Pixel.MapItoR[index & rMask];
                     green = ctx->Pixel.MapItoG[index & gMask];
                     blue  = ctx->Pixel.MapItoB[index & bMask];
                     alpha = ctx->Pixel.MapItoA[index & aMask];
                  }
                  break;
               case GL_RGB:
                  /* Fetch image RGBA values */
                  red   = ((GLfloat*) image->Data)[pixel*3+0];
                  green = ((GLfloat*) image->Data)[pixel*3+1];
                  blue  = ((GLfloat*) image->Data)[pixel*3+2];
                  alpha = 1.0;
                  break;
               case GL_RGBA:
                  red   = ((GLfloat*) image->Data)[pixel*4+0];
                  green = ((GLfloat*) image->Data)[pixel*4+1];
                  blue  = ((GLfloat*) image->Data)[pixel*4+2];
                  alpha = ((GLfloat*) image->Data)[pixel*4+3];
                  break;
               case GL_RED:
                  red   = ((GLfloat*) image->Data)[pixel];
                  green = 0.0;
                  blue  = 0.0;
                  alpha = 1.0;
                  break;
               case GL_GREEN:
                  red   = 0.0;
                  green = ((GLfloat*) image->Data)[pixel];
                  blue  = 0.0;
                  alpha = 1.0;
                  break;
               case GL_BLUE:
                  red   = 0.0;
                  green = 0.0;
                  blue  = ((GLfloat*) image->Data)[pixel];
                  alpha = 1.0;
                  break;
               case GL_ALPHA:
                  red   = 0.0;
                  green = 0.0;
                  blue  = 0.0;
                  alpha = ((GLfloat*) image->Data)[pixel];
                  break;
               case GL_LUMINANCE: 
                  red   = ((GLfloat*) image->Data)[pixel];
                  green = red;
                  blue  = red;
                  alpha = 1.0;
                  break;
              case GL_LUMINANCE_ALPHA:
                  red   = ((GLfloat*) image->Data)[pixel*2+0];
                  green = red;
                  blue  = red;
                  alpha = ((GLfloat*) image->Data)[pixel*2+1];
                  break;
               default:
                  gl_problem(ctx,"Bad format (4) in image_to_texture");
                  return NULL;
            }
            
            if (image->Format!=GL_COLOR_INDEX) {
               /* Apply RGBA scale and bias */
               if (scaleOrBias) {
                  red   = red   * ctx->Pixel.RedScale   + ctx->Pixel.RedBias;
                  green = green * ctx->Pixel.GreenScale + ctx->Pixel.GreenBias;
                  blue  = blue  * ctx->Pixel.BlueScale  + ctx->Pixel.BlueBias;
                  alpha = alpha * ctx->Pixel.AlphaScale + ctx->Pixel.AlphaBias;
                  red   = CLAMP( red,    0.0F, 1.0F );
                  green = CLAMP( green,  0.0F, 1.0F );
                  blue  = CLAMP( blue,   0.0F, 1.0F );
                  alpha = CLAMP( alpha,  0.0F, 1.0F );
               }
               /* Apply pixel maps */
               if (ctx->Pixel.MapColorFlag) {
                  GLint ir = (GLint) (red  *ctx->Pixel.MapRtoRsize);
                  GLint ig = (GLint) (green*ctx->Pixel.MapGtoGsize);
                  GLint ib = (GLint) (blue *ctx->Pixel.MapBtoBsize);
                  GLint ia = (GLint) (alpha*ctx->Pixel.MapAtoAsize);
                  red   = ctx->Pixel.MapRtoR[ir];
                  green = ctx->Pixel.MapGtoG[ig];
                  blue  = ctx->Pixel.MapBtoB[ib];
                  alpha = ctx->Pixel.MapAtoA[ia];
               }
            }

            /* store texel (components are GLubytes in [0,255]) */
            switch (texImage->Format) {
               case GL_COLOR_INDEX:
                  /* a paletted texture */
                  texImage->Data[pixel] = (GLint) (red * 255.0F);
                  break;
               case GL_ALPHA:
                  texImage->Data[pixel] = (GLint) (alpha * 255.0F);
                  break;
               case GL_LUMINANCE:
                  texImage->Data[pixel] = (GLint) (red * 255.0F);
                  break;
               case GL_LUMINANCE_ALPHA:
                  texImage->Data[pixel*2+0] = (GLint) (red * 255.0F);
                  texImage->Data[pixel*2+1] = (GLint) (alpha * 255.0F);
                  break;
               case GL_INTENSITY:
                  texImage->Data[pixel] = (GLint) (red * 255.0F);
                  break;
               case GL_RGB:
                  texImage->Data[pixel*3+0] = (GLint) (red   * 255.0F);
                  texImage->Data[pixel*3+1] = (GLint) (green * 255.0F);
                  texImage->Data[pixel*3+2] = (GLint) (blue  * 255.0F);
                  break;
               case GL_RGBA:
                  texImage->Data[pixel*4+0] = (GLint) (red   * 255.0F);
                  texImage->Data[pixel*4+1] = (GLint) (green * 255.0F);
                  texImage->Data[pixel*4+2] = (GLint) (blue  * 255.0F);
                  texImage->Data[pixel*4+3] = (GLint) (alpha * 255.0F);
                  break;
               default:
                  gl_problem(ctx,"Bad format (5) in image_to_texture");
                  return NULL;
            }
         }
         break;

      default:
         gl_problem(ctx, "Bad image type in image_to_texture");
         return NULL;
   }

   return texImage;
}



/*
 * glTexImage[123]D can accept a NULL image pointer.  In this case we
 * create a texture image with unspecified image contents per the OpenGL
 * spec.
 */
static struct gl_texture_image *
make_null_texture( GLcontext *ctx, GLenum internalFormat,
                   GLsizei width, GLsizei height, GLsizei depth, GLint border )
{
   GLint components;
   struct gl_texture_image *texImage;
   GLint numPixels;

   /*internalFormat = decode_internal_format(internalFormat);*/
   components = components_in_intformat(internalFormat);
   numPixels = width * height * depth;

   texImage = gl_alloc_texture_image();
   if (!texImage)
      return NULL;

   texImage->Format = decode_internal_format(internalFormat);
   texImage->IntFormat = internalFormat;
   texImage->Border = border;
   texImage->Width = width;
   texImage->Height = height;
   texImage->WidthLog2 = logbase2(width - 2*border);
   if (height==1)  /* 1-D texture */
      texImage->HeightLog2 = 0;
   else
      texImage->HeightLog2 = logbase2(height - 2*border);
   texImage->Width2 = 1 << texImage->WidthLog2;
   texImage->Height2 = 1 << texImage->HeightLog2;
   texImage->MaxLog2 = MAX2( texImage->WidthLog2, texImage->HeightLog2 );

   /* XXX should we really allocate memory for the image or let it be NULL? */
   /*texImage->Data = NULL;*/

   texImage->Data = (GLubyte *) malloc( numPixels * components );

   /*
    * Let's see if anyone finds this.  If glTexImage2D() is called with
    * a NULL image pointer then load the texture image with something
    * interesting instead of leaving it indeterminate.
    */
   if (texImage->Data) {
      char message[8][32] = {
         "   X   X  XXXXX   XXX     X    ",
         "   XX XX  X      X   X   X X   ",
         "   X X X  X      X      X   X  ",
         "   X   X  XXXX    XXX   XXXXX  ",
         "   X   X  X          X  X   X  ",
         "   X   X  X      X   X  X   X  ",
         "   X   X  XXXXX   XXX   X   X  ",
         "                               "
      };

      GLubyte *imgPtr = texImage->Data;
      GLint i, j, k;
      for (i=0;i<height;i++) {
         GLint srcRow = 7 - i % 8;
         for (j=0;j<width;j++) {
            GLint srcCol = j % 32;
            GLubyte texel = (message[srcRow][srcCol]=='X') ? 255 : 70;
            for (k=0;k<components;k++) {
               *imgPtr++ = texel;
            }
         }
      }
   }

   return texImage;
}




/*
 * Test glTexImagee1D() parameters for errors.
 * Return:  GL_TRUE = an error was detected, GL_FALSE = no errors
 */
static GLboolean texture_1d_error_check( GLcontext *ctx, GLenum target,
                                         GLint level, GLenum internalFormat,
                                         GLenum format, GLenum type,
                                         GLint width, GLint border )
{
   GLint iformat;
   if (target!=GL_TEXTURE_1D && target!=GL_PROXY_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D" );
      return GL_TRUE;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(level)" );
      return GL_TRUE;
   }
   iformat = decode_internal_format( internalFormat );
   if (iformat<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(internalFormat)" );
      return GL_TRUE;
   }
   if (border!=0 && border!=1) {
      if (target!=GL_PROXY_TEXTURE_1D) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(border)" );
      }
      return GL_TRUE;
   }
   if (width<2*border || width>2+MAX_TEXTURE_SIZE) {
      if (target!=GL_PROXY_TEXTURE_1D) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(width)" );
      }
      return GL_TRUE;
   }
   if (logbase2( width-2*border )<0) {
      gl_error( ctx, GL_INVALID_VALUE,
               "glTexImage1D(width != 2^k + 2*border)");
      return GL_TRUE;
   }
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_BGR_EXT:
      case GL_RGBA:
      case GL_BGRA_EXT:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D(format)" );
         return GL_TRUE;
   }
   switch (type) {
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case GL_FLOAT:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D(type)" );
         return GL_TRUE;
   }
   return GL_FALSE;
}


/*
 * Test glTexImagee2D() parameters for errors.
 * Return:  GL_TRUE = an error was detected, GL_FALSE = no errors
 */
static GLboolean texture_2d_error_check( GLcontext *ctx, GLenum target,
                                         GLint level, GLenum internalFormat,
                                         GLenum format, GLenum type,
                                         GLint width, GLint height,
                                         GLint border )
{
   GLint iformat;
   if (target!=GL_TEXTURE_2D && target!=GL_PROXY_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
      return GL_TRUE;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(level)" );
      return GL_TRUE;
   }
   iformat = decode_internal_format( internalFormat );
   if (iformat<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(internalFormat)" );
      return GL_TRUE;
   }
   if (border!=0 && border!=1) {
      if (target!=GL_PROXY_TEXTURE_2D) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(border)" );
      }
      return GL_TRUE;
   }
   if (width<2*border || width>2+MAX_TEXTURE_SIZE) {
      if (target!=GL_PROXY_TEXTURE_2D) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(width)" );
      }
      return GL_TRUE;
   }
   if (height<2*border || height>2+MAX_TEXTURE_SIZE) {
      if (target!=GL_PROXY_TEXTURE_2D) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(height)" );
      }
      return GL_TRUE;
   }
   if (logbase2( width-2*border )<0) {
      gl_error( ctx,GL_INVALID_VALUE,
               "glTexImage2D(width != 2^k + 2*border)");
      return GL_TRUE;
   }
   if (logbase2( height-2*border )<0) {
      gl_error( ctx,GL_INVALID_VALUE,
               "glTexImage2D(height != 2^k + 2*border)");
      return GL_TRUE;
   }
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_BGR_EXT:
      case GL_RGBA:
      case GL_BGRA_EXT:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(format)" );
         return GL_TRUE;
   }
   switch (type) {
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case GL_UNSIGNED_INT:
      case GL_INT:
      case GL_FLOAT:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(type)" );
         return GL_TRUE;
   }
   return GL_FALSE;
}


/*
 * Called from the API.  Note that width includes the border.
 */
void gl_TexImage1D( GLcontext *ctx,
                    GLenum target, GLint level, GLint internalformat,
		    GLsizei width, GLint border, GLenum format,
		    GLenum type, struct gl_image *image )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexImage1D" );
      return;
   }

   if (target==GL_TEXTURE_1D) {
      struct gl_texture_image *teximage;
      if (texture_1d_error_check( ctx, target, level, internalformat,
                                  format, type, width, border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (ctx->Texture.Current1D->Image[level]) {
         gl_free_texture_image( ctx->Texture.Current1D->Image[level] );
      }

      /* make new texture from source image */
      if (image) {
         teximage = image_to_texture(ctx, image, internalformat, border);
      }
      else {
         teximage = make_null_texture(ctx, internalformat,
                                      width, 1, 1, border);
      }

      /* install new texture image */
      ctx->Texture.Current1D->Image[level] = teximage;
      ctx->Texture.Current1D->Dirty = GL_TRUE;
      ctx->Texture.AnyDirty = GL_TRUE;
      ctx->NewState |= NEW_TEXTURING;

      /* free the source image */
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D,
                                  ctx->Texture.Current1D,
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_1d_error_check( ctx, target, level, internalformat,
                                  format, type, width, border )) {
         if (level>=0 && level<MAX_TEXTURE_LEVELS) {
            MEMSET( ctx->Texture.Proxy1D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy1D->Image[level]->Format = internalformat;
         ctx->Texture.Proxy1D->Image[level]->Border = border;
         ctx->Texture.Proxy1D->Image[level]->Width = width;
         ctx->Texture.Proxy1D->Image[level]->Height = 1;
      }
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
      return;
   }
}




/*
 * Called by the API or display list executor.
 * Note that width and height include the border.
 */
void gl_TexImage2D( GLcontext *ctx,
                    GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLint border,
                    GLenum format, GLenum type,
                    struct gl_image *image )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexImage2D" );
      return;
   }

   if (target==GL_TEXTURE_2D) {
      struct gl_texture_image *teximage;
      if (texture_2d_error_check( ctx, target, level, internalformat,
                                  format, type, width, height, border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (ctx->Texture.Current2D->Image[level]) {
         gl_free_texture_image( ctx->Texture.Current2D->Image[level] );
      }

      /* make new texture from source image */
      if (image) {
         teximage = image_to_texture(ctx, image, internalformat, border);
      }
      else {
         teximage = make_null_texture(ctx, internalformat,
                                      width, height, 1, border);
      }

      /* install new texture image */
      ctx->Texture.Current2D->Image[level] = teximage;
      ctx->Texture.Current2D->Dirty = GL_TRUE;
      ctx->Texture.AnyDirty = GL_TRUE;
      ctx->NewState |= NEW_TEXTURING;

      /* free the source image */
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D,
                                  ctx->Texture.Current2D,
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_2D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_2d_error_check( ctx, target, level, internalformat,
                                  format, type, width, height, border )) {
         if (level>=0 && level<MAX_TEXTURE_LEVELS) {
            MEMSET( ctx->Texture.Proxy2D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy2D->Image[level]->Format = internalformat;
         ctx->Texture.Proxy2D->Image[level]->Border = border;
         ctx->Texture.Proxy2D->Image[level]->Width = width;
         ctx->Texture.Proxy2D->Image[level]->Height = height;
      }
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
      return;
   }
}



void gl_GetTexImage( GLcontext *ctx, GLenum target, GLint level, GLenum format,
                     GLenum type, GLvoid *pixels )
{
   gl_problem(ctx, "glGetTexImage not implemented");
   /* TODO */
}




/*
 * Unpack the image data given to glTexSubImage[123]D.
 * This function is just a wrapper for gl_unpack_image() but it does
 * some extra error checking.
 */
struct gl_image *
gl_unpack_texsubimage( GLcontext *ctx, GLint width, GLint height,
                       GLenum format, GLenum type, const GLvoid *pixels )
{
   if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
      return NULL;
   }

   if (format==GL_STENCIL_INDEX || format==GL_DEPTH_COMPONENT){
      return NULL;
   }

   if (gl_sizeof_type(type)<=0) {
      return NULL;
   }

   return gl_unpack_image( ctx, width, height, format, type, pixels );
}



void gl_TexSubImage1D( GLcontext *ctx,
                       GLenum target, GLint level, GLint xoffset,
                       GLsizei width, GLenum format, GLenum type,
                       struct gl_image *image )
{
   struct gl_texture_image *destTex;

   if (target!=GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(target)" );
      return;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(level)" );
      return;
   }

   destTex = ctx->Texture.Current1D->Image[level];
   if (!destTex) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexSubImage1D" );
      return;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage1D(xoffset)" );
      return;
   }
   if (xoffset + width > destTex->Width + destTex->Border) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage1D(xoffset+width)" );
      return;
   }

   if (image) {
      /* unpacking must have been error-free */
      GLint texcomponents = components_in_intformat(destTex->Format);

      if (image->Type==GL_UNSIGNED_BYTE && texcomponents==image->Components) {
         /* Simple case, just byte copy image data into texture image */
         /* row by row. */
         GLubyte *dst = destTex->Data + texcomponents * xoffset;
         GLubyte *src = (GLubyte *) image->Data;
         MEMCPY( dst, src, width * texcomponents );
      }
      else {
         /* General case, convert image pixels into texels, scale, bias, etc */
         struct gl_texture_image *subTexImg = image_to_texture(ctx, image,
                                        destTex->IntFormat, destTex->Border);
         GLubyte *dst = destTex->Data + texcomponents * xoffset;
         GLubyte *src = subTexImg->Data;
         MEMCPY( dst, src, width * texcomponents );
         gl_free_texture_image(subTexImg);
      }

      /* if the image's reference count is zero, delete it now */
      if (image->RefCount==0) {
         gl_free_image(image);
      }

      ctx->Texture.Current1D->Dirty = GL_TRUE;
      ctx->Texture.AnyDirty = GL_TRUE;

      /* tell driver about change */
      if (ctx->Driver.TexSubImage) {
	(*ctx->Driver.TexSubImage)( ctx, GL_TEXTURE_1D,
				    ctx->Texture.Current1D, level,
				    xoffset,0,width,1,
				    ctx->Texture.Current1D->Image[level]->IntFormat,
				    destTex );
      }
      else {
	if (ctx->Driver.TexImage) {
	  (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D,
				   ctx->Texture.Current1D,
				   level, ctx->Texture.Current1D->Image[level]->IntFormat,
				   destTex );
	}
      }
   }
   else {
      /* if no image, an error must have occured, do more testing now */
      GLint components, size;

      if (width<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage1D(width)" );
         return;
      }
      if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(format)" );
         return;
      }
      components = components_in_intformat( format );
      if (components<0 || format==GL_STENCIL_INDEX
          || format==GL_DEPTH_COMPONENT){
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(format)" );
         return;
      }
      size = gl_sizeof_type( type );
      if (size<=0) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(type)" );
         return;
      }
      /* if we get here, probably ran out of memory during unpacking */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D" );
   }
}



void gl_TexSubImage2D( GLcontext *ctx,
                       GLenum target, GLint level,
                       GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       struct gl_image *image )
{
   struct gl_texture_image *destTex;

   if (target!=GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
      return;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(level)" );
      return;
   }

   destTex = ctx->Texture.Current2D->Image[level];
   if (!destTex) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexSubImage2D" );
      return;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(xoffset)" );
      return;
   }
   if (yoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(yoffset)" );
      return;
   }
   if (xoffset + width > destTex->Width + destTex->Border) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(xoffset+width)" );
      return;
   }
   if (yoffset + height > destTex->Height + destTex->Border) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(yoffset+height)" );
      return;
   }

   if (image) {
      /* unpacking must have been error-free */
      GLint texcomponents = components_in_intformat(destTex->Format);

      if (image->Type==GL_UNSIGNED_BYTE && texcomponents==image->Components) {
         /* Simple case, just byte copy image data into texture image */
         /* row by row. */
         GLubyte *dst = destTex->Data 
                      + (yoffset * destTex->Width + xoffset) * texcomponents;
         GLubyte *src = (GLubyte *) image->Data;
         GLint  j;
         for (j=0;j<height;j++) {
            MEMCPY( dst, src, width * texcomponents );
            dst += destTex->Width * texcomponents * sizeof(GLubyte);
            src += width * texcomponents * sizeof(GLubyte);
         }
      }
      else {
         /* General case, convert image pixels into texels, scale, bias, etc */
         struct gl_texture_image *subTexImg = image_to_texture(ctx, image,
                                        destTex->IntFormat, destTex->Border);
         GLubyte *dst = destTex->Data
                  + (yoffset * destTex->Width + xoffset) * texcomponents;
         GLubyte *src = subTexImg->Data;
         GLint j;
         for (j=0;j<height;j++) {
            MEMCPY( dst, src, width * texcomponents );
            dst += destTex->Width * texcomponents * sizeof(GLubyte);
            src += width * texcomponents * sizeof(GLubyte);
         }
         gl_free_texture_image(subTexImg);
      }

      /* if the image's reference count is zero, delete it now */
      if (image->RefCount==0) {
         gl_free_image(image);
      }

      ctx->Texture.Current2D->Dirty = GL_TRUE;
      ctx->Texture.AnyDirty = GL_TRUE;

      /* tell driver about change */
      if (ctx->Driver.TexSubImage) {
	(*ctx->Driver.TexSubImage)( ctx, GL_TEXTURE_2D, ctx->Texture.Current2D, level,
				    xoffset, yoffset, width, height,
				    ctx->Texture.Current2D->Image[level]->IntFormat,
				    destTex );
      }
      else {
	if (ctx->Driver.TexImage) {
	  (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D, ctx->Texture.Current2D,
				   level, ctx->Texture.Current2D->Image[level]->IntFormat,
				   destTex );
	}
      }
   }
   else {
      /* if no image, an error must have occured, do more testing now */
      GLint components, size;

      if (width<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(width)" );
         return;
      }
      if (height<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(height)" );
         return;
      }
      if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(format)" );
         return;
      }
      components = gl_components_in_format( format );
      if (components<0 || format==GL_STENCIL_INDEX
          || format==GL_DEPTH_COMPONENT){
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(format)" );
         return;
      }
      size = gl_sizeof_type( type );
      if (size<=0) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(type)" );
         return;
      }
      /* if we get here, probably ran out of memory during unpacking */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D" );
   }
}

/*
 * Read an RGBA image from the frame buffer.
 * Input:  ctx - the context
 *         x, y - lower left corner
 *         width, height - size of region to read
 *         format - one of GL_RED, GL_RGB, GL_LUMINANCE, etc.
 * Return: gl_image pointer or NULL if out of memory
 */
static struct gl_image *read_color_image( GLcontext *ctx, GLint x, GLint y,
                                          GLsizei width, GLsizei height,
                                          GLint format )
{
   struct gl_image *image;
   GLubyte *imgptr;
   GLint components;
   GLint i, j;

   components = components_in_intformat( format );

   /*
    * Allocate image struct and image data buffer
    */
   image = (struct gl_image *) malloc( sizeof(struct gl_image) );
   if (image) {
      image->Width = width;
      image->Height = height;
      image->Components = components;
      image->Format = format;
      image->Type = GL_UNSIGNED_BYTE;
      image->RefCount = 0;
      image->Data = (GLubyte *) malloc( width * height * components );
      if (!image->Data) {
         free(image);
         return NULL;
      }
   }
   else {
      return NULL;
   }

   imgptr = (GLubyte *) image->Data;

   /* Select buffer to read from */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );

   for (j=0;j<height;j++) {
      GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
      GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
      gl_read_color_span( ctx, width, x, y+j, red, green, blue, alpha );

      if (!ctx->Visual->EightBitColor) {
         /* scale red, green, blue, alpha values to range [0,255] */
         GLfloat rscale = 255.0f * ctx->Visual->InvRedScale;
         GLfloat gscale = 255.0f * ctx->Visual->InvGreenScale;
         GLfloat bscale = 255.0f * ctx->Visual->InvBlueScale;
         GLfloat ascale = 255.0f * ctx->Visual->InvAlphaScale;
         for (i=0;i<width;i++) {
            red[i]   = (GLubyte) (GLint) (red[i]   * rscale);
            green[i] = (GLubyte) (GLint) (green[i] * gscale);
            blue[i]  = (GLubyte) (GLint) (blue[i]  * bscale);
            alpha[i] = (GLubyte) (GLint) (alpha[i] * ascale);
         }
      }

      switch (format) {
         case GL_ALPHA:
            for (i=0;i<width;i++) {
               *imgptr++ = alpha[i];
            }
            break;
         case GL_LUMINANCE:
            for (i=0;i<width;i++) {
               *imgptr++ = red[i];
            }
            break;
         case GL_LUMINANCE_ALPHA:
            for (i=0;i<width;i++) {
               *imgptr++ = red[i];
               *imgptr++ = alpha[i];
            }
            break;
         case GL_INTENSITY:
            for (i=0;i<width;i++) {
               *imgptr++ = red[i];
            }
            break;
         case GL_RGB:
            for (i=0;i<width;i++) {
               *imgptr++ = red[i];
               *imgptr++ = green[i];
               *imgptr++ = blue[i];
            }
            break;
         case GL_RGBA:
            for (i=0;i<width;i++) {
               *imgptr++ = red[i];
               *imgptr++ = green[i];
               *imgptr++ = blue[i];
               *imgptr++ = alpha[i];
            }
            break;
      } /*switch*/

   } /*for*/         

   /* Restore drawing buffer */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );

   return image;
}




void gl_CopyTexImage1D( GLcontext *ctx,
                        GLenum target, GLint level,
                        GLenum internalformat,
                        GLint x, GLint y,
                        GLsizei width, GLint border )
{
   GLint format;
   struct gl_image *teximage;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexImage1D" );
      return;
   }
   if (target!=GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexImage1D(target)" );
      return;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(level)" );
      return;
   }
   if (border!=0 && border!=1) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(border)" );
      return;
   }
   if (width<2*border || width>2+MAX_TEXTURE_SIZE || width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(width)" );
      return;
   }
   format = decode_internal_format( internalformat );
   if (format<0 || (internalformat>=1 && internalformat<=4)) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(format)" );
      return;
   }

   teximage = read_color_image( ctx, x, y, width, 1, format );
   if (!teximage) {
      gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D" );
      return;
   }

   gl_TexImage1D( ctx, target, level, internalformat, width,
                  border, GL_RGBA, GL_UNSIGNED_BYTE, teximage );

   /* teximage was freed in gl_TexImage1D */
}



void gl_CopyTexImage2D( GLcontext *ctx,
                        GLenum target, GLint level, GLenum internalformat,
                        GLint x, GLint y, GLsizei width, GLsizei height,
                        GLint border )
{
   GLint format;
   struct gl_image *teximage;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexImage2D" );
      return;
   }
   if (target!=GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
      return;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(level)" );
      return;
   }
   if (border!=0 && border!=1) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(border)" );
      return;
   }
   if (width<2*border || width>2+MAX_TEXTURE_SIZE || width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(width)" );
      return;
   }
   if (height<2*border || height>2+MAX_TEXTURE_SIZE || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(height)" );
      return;
   }
   format = decode_internal_format( internalformat );
   if (format<0 || (internalformat>=1 && internalformat<=4)) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(format)" );
      return;
   }

   teximage = read_color_image( ctx, x, y, width, height, format );
   if (!teximage) {
      gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D" );
      return;
   }

   gl_TexImage2D( ctx, target, level, internalformat, width, height,
                  border, GL_RGBA, GL_UNSIGNED_BYTE, teximage );

   /* teximage was freed in gl_TexImage2D */
}




/*
 * Do the work of glCopyTexSubImage[12]D.
 * TODO: apply pixel bias scale and mapping.
 */
static void copy_tex_sub_image( GLcontext *ctx, struct gl_texture_image *dest,
                                GLint width, GLint height,
                                GLint srcx, GLint srcy,
                                GLint dstx, GLint dsty)
{
   GLint i, j;
   GLint format, components;

   format = dest->Format;
   components = components_in_intformat( format );

   for (j=0;j<height;j++) {
      GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
      GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
      GLubyte *texptr;

      gl_read_color_span( ctx, width, srcx, srcy+j, red, green, blue, alpha );

      if (!ctx->Visual->EightBitColor) {
         /* scale red, green, blue, alpha values to range [0,255] */
         GLfloat rscale = 255.0f * ctx->Visual->InvRedScale;
         GLfloat gscale = 255.0f * ctx->Visual->InvGreenScale;
         GLfloat bscale = 255.0f * ctx->Visual->InvBlueScale;
         GLfloat ascale = 255.0f * ctx->Visual->InvAlphaScale;
         for (i=0;i<width;i++) {
            red[i]   = (GLubyte) (GLint) (red[i]   * rscale);
            green[i] = (GLubyte) (GLint) (green[i] * gscale);
            blue[i]  = (GLubyte) (GLint) (blue[i]  * bscale);
            alpha[i] = (GLubyte) (GLint) (alpha[i] * ascale);
         }
      }

      texptr = dest->Data + ( (dsty+j) * width + dstx) * components;

      switch (format) {
         case GL_ALPHA:
            for (i=0;i<width;i++) {
               *texptr++ = alpha[i];
            }
            break;
         case GL_LUMINANCE:
            for (i=0;i<width;i++) {
               *texptr++ = red[i];
            }
            break;
         case GL_LUMINANCE_ALPHA:
            for (i=0;i<width;i++) {
               *texptr++ = red[i];
               *texptr++ = alpha[i];
            }
            break;
         case GL_INTENSITY:
            for (i=0;i<width;i++) {
               *texptr++ = red[i];
            }
            break;
         case GL_RGB:
            for (i=0;i<width;i++) {
               *texptr++ = red[i];
               *texptr++ = green[i];
               *texptr++ = blue[i];
            }
            break;
         case GL_RGBA:
            for (i=0;i<width;i++) {
               *texptr++ = red[i];
               *texptr++ = green[i];
               *texptr++ = blue[i];
               *texptr++ = alpha[i];
            }
            break;
      } /*switch*/
   } /*for*/         
}




void gl_CopyTexSubImage1D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint x, GLint y, GLsizei width )
{
   struct gl_texture_image *teximage;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexSubImage1D" );
      return;
   }
   if (target!=GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage1D(target)" );
      return;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage1D(level)" );
      return;
   }
   if (width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage1D(width)" );
      return;
   }

   teximage = ctx->Texture.Current1D->Image[level];

   if (teximage) {
      if (xoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage1D(xoffset)" );
         return;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (xoffset+width > teximage->Width+teximage->Border) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage1D(xoffset+width)" );
         return;
      }
      if (teximage->Data) {
         copy_tex_sub_image( ctx, teximage, width, 1, x, y, xoffset, 0);
      }
   }
   else {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexSubImage1D" );
   }
}



void gl_CopyTexSubImage2D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_image *teximage;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexSubImage2D" );
      return;
   }
   if (target!=GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
      return;
   }
   if (level<0 || level>=MAX_TEXTURE_LEVELS) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(level)" );
      return;
   }
   if (width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(width)" );
      return;
   }
   if (height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(height)" );
      return;
   }

   teximage = ctx->Texture.Current2D->Image[level];

   if (teximage) {
      if (xoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(xoffset)" );
         return;
      }
      if (yoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(yoffset)" );
         return;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (xoffset+width > teximage->Width+teximage->Border) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage2D(xoffset+width)" );
         return;
      }
      if (yoffset+height > teximage->Height+teximage->Border) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage2D(yoffset+height)" );
         return;
      }

      if (teximage->Data) {
         copy_tex_sub_image( ctx, teximage, width, height,
                             x, y, xoffset, yoffset);
      }
   }
   else {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexSubImage2D" );
   }
}

