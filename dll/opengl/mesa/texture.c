/* $Id: texture.c,v 1.36 1997/11/17 02:04:06 brianp Exp $ */

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
 * $Log: texture.c,v $
 * Revision 1.36  1997/11/17 02:04:06  brianp
 * fixed a typo in texture code
 *
 * Revision 1.35  1997/11/13 02:17:32  brianp
 * added a few const keywords
 *
 * Revision 1.34  1997/10/16 01:59:08  brianp
 * added GL_EXT_shared_texture_palette extension
 *
 * Revision 1.33  1997/09/27 00:14:39  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.32  1997/08/14 01:03:12  brianp
 * fixed small bug in GL_SPHERE_MAP texgen (Petri Nordlund)
 *
 * Revision 1.31  1997/07/24 01:25:34  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.30  1997/07/22 01:23:48  brianp
 * fixed negative texture coord sampling problem (Magnus Lundin)
 *
 * Revision 1.29  1997/07/09 00:32:26  brianp
 * fixed bug involving sampling and incomplete texture objects
 *
 * Revision 1.28  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.27  1997/05/17 03:51:10  brianp
 * fixed bug in PROD macro (Waldemar Celes)
 *
 * Revision 1.26  1997/05/03 00:53:56  brianp
 * all-new texture sampling via gl_texture_object's SampleFunc pointer
 *
 * Revision 1.25  1997/05/01 01:40:14  brianp
 * replaced sqrt() with GL_SQRT, use NORMALIZE_3FV instead of NORMALIZE_3V
 *
 * Revision 1.24  1997/04/28 02:04:18  brianp
 * GL_SPHERE_MAP texgen was wrong (Eamon O'Dea)
 *
 * Revision 1.23  1997/04/20 20:29:11  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.22  1997/04/16 23:56:41  brianp
 * added a few #include files
 *
 * Revision 1.21  1997/04/14 02:02:39  brianp
 * moved many functions into new texstate.c file
 *
 * Revision 1.20  1997/04/01 04:19:14  brianp
 * call gl_analyze_modelview_matrix instead of gl_compute_modelview_inverse
 *
 * Revision 1.19  1997/03/04 19:55:58  brianp
 * small texture sampling optimizations.  better comments.
 *
 * Revision 1.18  1997/03/04 19:19:20  brianp
 * fixed a number of problems with texture borders
 *
 * Revision 1.17  1997/02/27 19:58:08  brianp
 * call gl_problem() instead of gl_warning()
 *
 * Revision 1.16  1997/02/09 19:53:43  brianp
 * now use TEXTURE_xD enable constants
 *
 * Revision 1.15  1997/02/09 18:53:14  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.14  1997/01/30 21:06:03  brianp
 * added some missing glGetTexLevelParameter() GLenums
 *
 * Revision 1.13  1997/01/16 03:36:01  brianp
 * added calls to device driver TexParameter() and TexEnv() functions
 *
 * Revision 1.12  1997/01/09 19:48:30  brianp
 * better error checking
 * added gl_texturing_enabled()
 *
 * Revision 1.11  1996/12/20 20:22:30  brianp
 * linear interpolation between mipmap levels was reverse weighted
 * max mipmap level was incorrectly tested for
 *
 * Revision 1.10  1996/12/12 22:33:05  brianp
 * minor changes to gl_texgen()
 *
 * Revision 1.9  1996/12/07 10:35:41  brianp
 * implmented glGetTexGen*() functions
 *
 * Revision 1.8  1996/11/14 01:03:09  brianp
 * removed const's from gl_texgen() function to avoid VMS compiler warning
 *
 * Revision 1.7  1996/11/08 02:19:52  brianp
 * gl_do_texgen() replaced with gl_texgen()
 *
 * Revision 1.6  1996/10/26 17:17:30  brianp
 * glTexGen GL_EYE_PLANE vector now transformed by inverse modelview matrix
 *
 * Revision 1.5  1996/10/11 03:42:38  brianp
 * replaced old _EXT symbols
 *
 * Revision 1.4  1996/09/27 01:30:24  brianp
 * added missing default cases to switches
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
#include <math.h>
#include <stdlib.h>
#include "context.h"
#include "macros.h"
#include "mmath.h"
#include "pb.h"
#include "texture.h"
#include "types.h"
#endif



/*
 * Perform automatic texture coordinate generation.
 * Input:  ctx - the context
 *         n - number of texture coordinates to generate
 *         obj - array of vertexes in object coordinate system
 *         eye - array of vertexes in eye coordinate system
 *         normal - array of normal vectores in eye coordinate system
 * Output:  texcoord - array of resuling texture coordinates
 */
void gl_texgen( GLcontext *ctx, GLint n,
                GLfloat obj[][4], GLfloat eye[][4],
                GLfloat normal[][3], GLfloat texcoord[][4] )
{
   /* special case: S and T sphere mapping */
   if (ctx->Texture.TexGenEnabled==(S_BIT|T_BIT)
       && ctx->Texture.GenModeS==GL_SPHERE_MAP
       && ctx->Texture.GenModeT==GL_SPHERE_MAP) {
      GLint i;
      for (i=0;i<n;i++) {
         GLfloat u[3], two_nu, m, fx, fy, fz;
         COPY_3V( u, eye[i] );
         NORMALIZE_3FV( u );
         two_nu = 2.0F * DOT3(normal[i],u);
         fx = u[0] - normal[i][0] * two_nu;
         fy = u[1] - normal[i][1] * two_nu;
         fz = u[2] - normal[i][2] * two_nu;
         m = 2.0F * GL_SQRT( fx*fx + fy*fy + (fz+1.0F)*(fz+1.0F) );
         if (m==0.0F) {
            texcoord[i][0] = 0.5F;
            texcoord[i][1] = 0.5F;
         }
         else {
            GLfloat mInv = 1.0F / m;
            texcoord[i][0] = fx * mInv + 0.5F;
            texcoord[i][1] = fy * mInv + 0.5F;
         }
      }
      return;
   }

   /* general solution */
   if (ctx->Texture.TexGenEnabled & S_BIT) {
      GLint i;
      switch (ctx->Texture.GenModeS) {
         case GL_OBJECT_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][0] = DOT4( obj[i], ctx->Texture.ObjectPlaneS );
            }
            break;
         case GL_EYE_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][0] = DOT4( eye[i], ctx->Texture.EyePlaneS );
            }
            break;
         case GL_SPHERE_MAP:
            for (i=0;i<n;i++) {
               GLfloat u[3], two_nu, m, fx, fy, fz;
               COPY_3V( u, eye[i] );
               NORMALIZE_3FV( u );
               two_nu = 2.0*DOT3(normal[i],u);
               fx = u[0] - normal[i][0] * two_nu;
               fy = u[1] - normal[i][1] * two_nu;
               fz = u[2] - normal[i][2] * two_nu;
               m = 2.0F * GL_SQRT( fx*fx + fy*fy + (fz+1.0)*(fz+1.0) );
               if (m==0.0F) {
                  texcoord[i][0] = 0.5F;
               }
               else {
                  texcoord[i][0] = fx / m + 0.5F;
               }
            }
            break;
         default:
            gl_problem(ctx, "Bad S texgen");
            return;
      }
   }

   if (ctx->Texture.TexGenEnabled & T_BIT) {
      GLint i;
      switch (ctx->Texture.GenModeT) {
         case GL_OBJECT_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][1] = DOT4( obj[i], ctx->Texture.ObjectPlaneT );
            }
            break;
         case GL_EYE_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][1] = DOT4( eye[i], ctx->Texture.EyePlaneT );
            }
            break;
         case GL_SPHERE_MAP:
            for (i=0;i<n;i++) {
               GLfloat u[3], two_nu, m, fx, fy, fz;
               COPY_3V( u, eye[i] );
               NORMALIZE_3FV( u );
               two_nu = 2.0*DOT3(normal[i],u);
               fx = u[0] - normal[i][0] * two_nu;
               fy = u[1] - normal[i][1] * two_nu;
               fz = u[2] - normal[i][2] * two_nu;
               m = 2.0F * GL_SQRT( fx*fx + fy*fy + (fz+1.0)*(fz+1.0) );
               if (m==0.0F) {
                  texcoord[i][1] = 0.5F;
               }
               else {
                  texcoord[i][1] = fy / m + 0.5F;
               }
            }
            break;
         default:
            gl_problem(ctx, "Bad T texgen");
            return;
      }
   }

   if (ctx->Texture.TexGenEnabled & R_BIT) {
      GLint i;
      switch (ctx->Texture.GenModeR) {
         case GL_OBJECT_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][2] = DOT4( obj[i], ctx->Texture.ObjectPlaneR );
            }
            break;
         case GL_EYE_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][2] = DOT4( eye[i], ctx->Texture.EyePlaneR );
            }
            break;
         default:
            gl_problem(ctx, "Bad R texgen");
            return;
      }
   }

   if (ctx->Texture.TexGenEnabled & Q_BIT) {
      GLint i;
      switch (ctx->Texture.GenModeQ) {
         case GL_OBJECT_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][3] = DOT4( obj[i], ctx->Texture.ObjectPlaneQ );
            }
            break;
         case GL_EYE_LINEAR:
            for (i=0;i<n;i++) {
               texcoord[i][3] = DOT4( eye[i], ctx->Texture.EyePlaneQ );
            }
            break;
         default:
            gl_problem(ctx, "Bad Q texgen");
            return;
      }
   }
}



/*
 * Paletted texture sampling.
 * Input:  tObj - the texture object
 *         index - the palette index (8-bit only)
 * Output:  red, green, blue, alpha - the texel color
 */
static void palette_sample(const struct gl_texture_object *tObj,
                           GLubyte index, GLubyte *red, GLubyte *green,
                           GLubyte *blue, GLubyte *alpha)
{
   GLint i = index;
   const GLubyte *palette;

   palette = tObj->Palette;

   switch (tObj->PaletteFormat) {
      case GL_ALPHA:
         *alpha = tObj->Palette[index];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         *red = palette[index];
         return;
      case GL_LUMINANCE_ALPHA:
         *red   = palette[(index << 1) + 0];
         *alpha = palette[(index << 1) + 1];
         return;
      case GL_RGB:
         *red   = palette[index * 3 + 0];
         *green = palette[index * 3 + 1];
         *blue  = palette[index * 3 + 2];
         return;
      case GL_RGBA:
         *red   = palette[(i << 2) + 0];
         *green = palette[(i << 2) + 1];
         *blue  = palette[(i << 2) + 2];
         *alpha = palette[(i << 2) + 3];
         return;
      default:
         gl_problem(NULL, "Bad palette format in palette_sample");
   }
}




/**********************************************************************/
/*                    1-D Texture Sampling Functions                  */
/**********************************************************************/


/*
 * Return the fractional part of x.
 */
#define frac(x) ((GLfloat)(x)-floor((GLfloat)x))



/*
 * Given 1-D texture image and an (i) texel column coordinate, return the
 * texel color.
 */
static void get_1d_texel( const struct gl_texture_object *tObj,
                          const struct gl_texture_image *img, GLint i,
                          GLubyte *red, GLubyte *green, GLubyte *blue,
                          GLubyte *alpha )
{
   GLubyte *texel;

#ifdef DEBUG
   GLint width = img->Width;
   if (i<0 || i>=width)  abort();
#endif

   switch (img->Format) {
      case GL_COLOR_INDEX:
         {
            GLubyte index = img->Data[i];
            palette_sample(tObj, index, red, green, blue, alpha);
            return;
         }
         return;
      case GL_ALPHA:
         *alpha = img->Data[ i ];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         *red   = img->Data[ i ];
         return;
      case GL_LUMINANCE_ALPHA:
         texel = img->Data + i * 2;
         *red   = texel[0];
         *alpha = texel[1];
         return;
      case GL_RGB:
         texel = img->Data + i * 3;
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         return;
      case GL_RGBA:
         texel = img->Data + i * 4;
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         *alpha = texel[3];
         return;
      default:
         gl_problem(NULL, "Bad format in get_1d_texel");
         return;
   }
}



/*
 * Return the texture sample for coordinate (s) using GL_NEAREST filter.
 */
static void sample_1d_nearest( const struct gl_texture_object *tObj,
                               const struct gl_texture_image *img,
                               GLfloat s,
                               GLubyte *red, GLubyte *green,
                               GLubyte *blue, GLubyte *alpha )
{
   GLint width = img->Width2;  /* without border, power of two */
   GLint i;
   GLubyte *texel;

   /* Clamp/Repeat S and convert to integer texel coordinate */
   if (tObj->WrapS==GL_REPEAT) {
      /* s limited to [0,1) */
      /* i limited to [0,width-1] */
      i = (GLint) (s * width);
      if (s<0.0F)  i -= 1;
      i &= (width-1);
   }
   else {
      /* s limited to [0,1] */
      /* i limited to [0,width-1] */
      if (s<0.0F)        i = 0;
      else if (s>1.0F)   i = width-1;
      else               i = (GLint) (s * width);
   }

   /* skip over the border, if any */
   i += img->Border;

   /* Get the texel */
   switch (img->Format) {
      case GL_COLOR_INDEX:
         {
            GLubyte index = img->Data[i];
            palette_sample(tObj, index, red, green, blue, alpha);
            return;
         }
      case GL_ALPHA:
         *alpha = img->Data[i];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         *red   = img->Data[i];
         return;
      case GL_LUMINANCE_ALPHA:
         texel = img->Data + i * 2;
         *red   = texel[0];
         *alpha = texel[1];
         return;
      case GL_RGB:
         texel = img->Data + i * 3;
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         return;
      case GL_RGBA:
         texel = img->Data + i * 4;
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         *alpha = texel[3];
         return;
      default:
         gl_problem(NULL, "Bad format in sample_1d_nearest");
   }
}



/*
 * Return the texture sample for coordinate (s) using GL_LINEAR filter.
 */
static void sample_1d_linear( const struct gl_texture_object *tObj,
                              const struct gl_texture_image *img,
                              GLfloat s,
                              GLubyte *red, GLubyte *green,
                              GLubyte *blue, GLubyte *alpha )
{
   GLint width = img->Width2;
   GLint i0, i1;
   GLfloat u;
   GLint i0border, i1border;

   u = s * width;
   if (tObj->WrapS==GL_REPEAT) {
      i0 = ((GLint) floor(u - 0.5F)) % width;
      i1 = (i0 + 1) & (width-1);
      i0border = i1border = 0;
   }
   else {
      i0 = (GLint) floor(u - 0.5F);
      i1 = i0 + 1;
      i0border = (i0<0) | (i0>=width);
      i1border = (i1<0) | (i1>=width);
   }

   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
      i0border = i1border = 0;
   }
   else {
      i0 &= (width-1);
   }

   {
      GLfloat a = frac(u - 0.5F);

      GLint w0 = (GLint) ((1.0F-a) * 256.0F);
      GLint w1 = (GLint) (      a  * 256.0F);

      GLubyte red0, green0, blue0, alpha0;
      GLubyte red1, green1, blue1, alpha1;

      if (i0border) {
         red0   = tObj->BorderColor[0];
         green0 = tObj->BorderColor[1];
         blue0  = tObj->BorderColor[2];
         alpha0 = tObj->BorderColor[3];
      }
      else {
         get_1d_texel( tObj, img, i0, &red0, &green0, &blue0, &alpha0 );
      }
      if (i1border) {
         red1   = tObj->BorderColor[0];
         green1 = tObj->BorderColor[1];
         blue1  = tObj->BorderColor[2];
         alpha1 = tObj->BorderColor[3];
      }
      else {
         get_1d_texel( tObj, img, i1, &red1, &green1, &blue1, &alpha1 );
      }

      *red   = (w0*red0   + w1*red1)   >> 8;
      *green = (w0*green0 + w1*green1) >> 8;
      *blue  = (w0*blue0  + w1*blue1)  >> 8;
      *alpha = (w0*alpha0 + w1*alpha1) >> 8;
   }
}


static void
sample_1d_nearest_mipmap_nearest( const struct gl_texture_object *tObj,
                                  GLfloat s, GLfloat lambda,
                                  GLubyte *red, GLubyte *green,
                                  GLubyte *blue, GLubyte *alpha )
{
   GLint level;
   if (lambda<=0.5F) {
      level = 0;
   }
   else {
      GLint widthlog2 = tObj->Image[0]->WidthLog2;
      level = (GLint) (lambda + 0.499999F);
      if (level>widthlog2 ) {
         level = widthlog2;
      }
   }
   sample_1d_nearest( tObj, tObj->Image[level],
                      s, red, green, blue, alpha );
}


static void
sample_1d_linear_mipmap_nearest( const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat lambda,
                                 GLubyte *red, GLubyte *green,
                                 GLubyte *blue, GLubyte *alpha )
{
   GLint level;
   if (lambda<=0.5F) {
      level = 0;
   }
   else {
      GLint widthlog2 = tObj->Image[0]->WidthLog2;
      level = (GLint) (lambda + 0.499999F);
      if (level>widthlog2 ) {
         level = widthlog2;
      }
   }
   sample_1d_linear( tObj, tObj->Image[level],
                     s, red, green, blue, alpha );
}



static void
sample_1d_nearest_mipmap_linear( const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat lambda,
                                 GLubyte *red, GLubyte *green,
                                 GLubyte *blue, GLubyte *alpha )
{
   GLint max = tObj->Image[0]->MaxLog2;

   if (lambda>=max) {
      sample_1d_nearest( tObj, tObj->Image[max],
                         s, red, green, blue, alpha );
   }
   else {
      GLubyte red0, green0, blue0, alpha0;
      GLubyte red1, green1, blue1, alpha1;
      GLfloat f = frac(lambda);
      GLint level = (GLint) (lambda + 1.0F);
      level = CLAMP( level, 1, max );
      sample_1d_nearest( tObj, tObj->Image[level-1],
                         s, &red0, &green0, &blue0, &alpha0 );
      sample_1d_nearest( tObj, tObj->Image[level],
                         s, &red1, &green1, &blue1, &alpha1 );
      *red   = (1.0F-f)*red0   + f*red1;
      *green = (1.0F-f)*green0 + f*green1;
      *blue  = (1.0F-f)*blue0  + f*blue1;
      *alpha = (1.0F-f)*alpha0 + f*alpha1;
   }
}



static void
sample_1d_linear_mipmap_linear( const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat lambda,
                                GLubyte *red, GLubyte *green,
                                GLubyte *blue, GLubyte *alpha )
{
   GLint max = tObj->Image[0]->MaxLog2;

   if (lambda>=max) {
      sample_1d_linear( tObj, tObj->Image[max],
                        s, red, green, blue, alpha );
   }
   else {
      GLubyte red0, green0, blue0, alpha0;
      GLubyte red1, green1, blue1, alpha1;
      GLfloat f = frac(lambda);
      GLint level = (GLint) (lambda + 1.0F);
      level = CLAMP( level, 1, max );
      sample_1d_linear( tObj, tObj->Image[level-1],
                        s, &red0, &green0, &blue0, &alpha0 );
      sample_1d_linear( tObj, tObj->Image[level],
                        s, &red1, &green1, &blue1, &alpha1 );
      *red   = (1.0F-f)*red0   + f*red1;
      *green = (1.0F-f)*green0 + f*green1;
      *blue  = (1.0F-f)*blue0  + f*blue1;
      *alpha = (1.0F-f)*alpha0 + f*alpha1;
   }
}



static void sample_nearest_1d( const struct gl_texture_object *tObj, GLuint n,
                               const GLfloat s[], const GLfloat t[],
                               const GLfloat u[], const GLfloat lambda[],
                               GLubyte red[], GLubyte green[], GLubyte blue[],
                               GLubyte alpha[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      sample_1d_nearest( tObj, tObj->Image[0], s[i],
                         &red[i], &green[i], &blue[i], &alpha[i]);
   }
}



static void sample_linear_1d( const struct gl_texture_object *tObj, GLuint n,
                              const GLfloat s[], const GLfloat t[],
                              const GLfloat u[], const GLfloat lambda[],
                              GLubyte red[], GLubyte green[], GLubyte blue[],
                              GLubyte alpha[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      sample_1d_linear( tObj, tObj->Image[0], s[i],
                        &red[i], &green[i], &blue[i], &alpha[i]);
   }
}


/*
 * Given an (s) texture coordinate and lambda (level of detail) value,
 * return a texture sample.
 *
 */
static void sample_lambda_1d( const struct gl_texture_object *tObj, GLuint n,
                              const GLfloat s[], const GLfloat t[],
                              const GLfloat u[], const GLfloat lambda[],
                              GLubyte red[], GLubyte green[], GLubyte blue[],
                              GLubyte alpha[] )
{
   GLuint i;

   for (i=0;i<n;i++) {
      if (lambda[i] > tObj->MinMagThresh) {
         /* minification */
         switch (tObj->MinFilter) {
            case GL_NEAREST:
               sample_1d_nearest( tObj, tObj->Image[0], s[i],
                                  &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR:
               sample_1d_linear( tObj, tObj->Image[0], s[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_NEAREST_MIPMAP_NEAREST:
               sample_1d_nearest_mipmap_nearest( tObj, lambda[i], s[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR_MIPMAP_NEAREST:
               sample_1d_linear_mipmap_nearest( tObj, s[i], lambda[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_NEAREST_MIPMAP_LINEAR:
               sample_1d_nearest_mipmap_linear( tObj, s[i], lambda[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR_MIPMAP_LINEAR:
               sample_1d_linear_mipmap_linear( tObj, s[i], lambda[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            default:
               gl_problem(NULL, "Bad min filter in sample_1d_texture");
               return;
         }
      }
      else {
         /* magnification */
         switch (tObj->MagFilter) {
            case GL_NEAREST:
               sample_1d_nearest( tObj, tObj->Image[0], s[i],
                                  &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR:
               sample_1d_linear( tObj, tObj->Image[0], s[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            default:
               gl_problem(NULL, "Bad mag filter in sample_1d_texture");
               return;
         }
      }
   }
}




/**********************************************************************/
/*                    2-D Texture Sampling Functions                  */
/**********************************************************************/


/*
 * Given a texture image and an (i,j) integer texel coordinate, return the
 * texel color.
 */
static void get_2d_texel( const struct gl_texture_object *tObj,
                          const struct gl_texture_image *img, GLint i, GLint j,
                          GLubyte *red, GLubyte *green, GLubyte *blue,
                          GLubyte *alpha )
{
   GLint width = img->Width;    /* includes border */
   GLubyte *texel;

#ifdef DEBUG
   GLint height = img->Height;  /* includes border */
   if (i<0 || i>=width)  abort();
   if (j<0 || j>=height)  abort();
#endif

   switch (img->Format) {
      case GL_COLOR_INDEX:
         {
            GLubyte index = img->Data[ width *j + i ];
            palette_sample(tObj, index, red, green, blue, alpha);
            return;
         }
      case GL_ALPHA:
         *alpha = img->Data[ width * j + i ];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         *red   = img->Data[ width * j + i ];
         return;
      case GL_LUMINANCE_ALPHA:
         texel = img->Data + (width * j + i) * 2;
         *red   = texel[0];
         *alpha = texel[1];
         return;
      case GL_RGB:
         texel = img->Data + (width * j + i) * 3;
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         return;
      case GL_RGBA:
         texel = img->Data + (width * j + i) * 4;
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         *alpha = texel[3];
         return;
      default:
         gl_problem(NULL, "Bad format in get_2d_texel");
   }
}



/*
 * Return the texture sample for coordinate (s,t) using GL_NEAREST filter.
 */
static void sample_2d_nearest( const struct gl_texture_object *tObj,
                               const struct gl_texture_image *img,
                               GLfloat s, GLfloat t,
                               GLubyte *red, GLubyte *green,
                               GLubyte *blue, GLubyte *alpha )
{
   GLint imgWidth = img->Width;  /* includes border */
   GLint width = img->Width2;    /* without border, power of two */
   GLint height = img->Height2;  /* without border, power of two */
   GLint i, j;
   GLubyte *texel;

   /* Clamp/Repeat S and convert to integer texel coordinate */
   if (tObj->WrapS==GL_REPEAT) {
      /* s limited to [0,1) */
      /* i limited to [0,width-1] */
      i = (GLint) (s * width);
      if (s<0.0F)  i -= 1;
      i &= (width-1);
   }
   else {
      /* s limited to [0,1] */
      /* i limited to [0,width-1] */
      if (s<=0.0F)      i = 0;
      else if (s>1.0F)  i = width-1;
      else              i = (GLint) (s * width);
   }

   /* Clamp/Repeat T and convert to integer texel coordinate */
   if (tObj->WrapT==GL_REPEAT) {
      /* t limited to [0,1) */
      /* j limited to [0,height-1] */
      j = (GLint) (t * height);
      if (t<0.0F)  j -= 1;
      j &= (height-1);
   }
   else {
      /* t limited to [0,1] */
      /* j limited to [0,height-1] */
      if (t<=0.0F)      j = 0;
      else if (t>1.0F)  j = height-1;
      else              j = (GLint) (t * height);
   }

   /* skip over the border, if any */
   i += img->Border;
   j += img->Border;

   switch (img->Format) {
      case GL_COLOR_INDEX:
         {
            GLubyte index = img->Data[ j * imgWidth + i ];
            palette_sample(tObj, index, red, green, blue, alpha);
            return;
         }
      case GL_ALPHA:
         *alpha = img->Data[ j * imgWidth + i ];
         return;
      case GL_LUMINANCE:
      case GL_INTENSITY:
         *red   = img->Data[ j * imgWidth + i ];
         return;
      case GL_LUMINANCE_ALPHA:
         texel = img->Data + ((j * imgWidth + i) << 1);
         *red   = texel[0];
         *alpha = texel[1];
         return;
      case GL_RGB:
         texel = img->Data + (j * imgWidth + i) * 3;
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         return;
      case GL_RGBA:
         texel = img->Data + ((j * imgWidth + i) << 2);
         *red   = texel[0];
         *green = texel[1];
         *blue  = texel[2];
         *alpha = texel[3];
         return;
      default:
         gl_problem(NULL, "Bad format in sample_2d_nearest");
   }
}



/*
 * Return the texture sample for coordinate (s,t) using GL_LINEAR filter.
 */
static void sample_2d_linear( const struct gl_texture_object *tObj,
                              const struct gl_texture_image *img,
                              GLfloat s, GLfloat t,
                              GLubyte *red, GLubyte *green,
                              GLubyte *blue, GLubyte *alpha )
{
   GLint width = img->Width2;
   GLint height = img->Height2;
   GLint i0, j0, i1, j1;
   GLint i0border, j0border, i1border, j1border;
   GLfloat u, v;

   u = s * width;
   if (tObj->WrapS==GL_REPEAT) {
      i0 = ((GLint) floor(u - 0.5F)) % width;
      i1 = (i0 + 1) & (width-1);
      i0border = i1border = 0;
   }
   else {
      i0 = (GLint) floor(u - 0.5F);
      i1 = i0 + 1;
      i0border = (i0<0) | (i0>=width);
      i1border = (i1<0) | (i1>=width);
   }

   v = t * height;
   if (tObj->WrapT==GL_REPEAT) {
      j0 = ((GLint) floor(v - 0.5F)) % height;
      j1 = (j0 + 1) & (height-1);
      j0border = j1border = 0;
   }
   else {
      j0 = (GLint) floor(v - 0.5F );
      j1 = j0 + 1;
      j0border = (j0<0) | (j0>=height);
      j1border = (j1<0) | (j1>=height);
   }

   if (img->Border) {
      i0 += img->Border;
      i1 += img->Border;
      j0 += img->Border;
      j1 += img->Border;
      i0border = i1border = 0;
      j0border = j1border = 0;
   }
   else {
      i0 &= (width-1);
      j0 &= (height-1);
   }

   {
      GLfloat a = frac(u - 0.5F);
      GLfloat b = frac(v - 0.5F);

      GLint w00 = (GLint) ((1.0F-a)*(1.0F-b) * 256.0F);
      GLint w10 = (GLint) (      a *(1.0F-b) * 256.0F);
      GLint w01 = (GLint) ((1.0F-a)*      b  * 256.0F);
      GLint w11 = (GLint) (      a *      b  * 256.0F);

      GLubyte red00, green00, blue00, alpha00;
      GLubyte red10, green10, blue10, alpha10;
      GLubyte red01, green01, blue01, alpha01;
      GLubyte red11, green11, blue11, alpha11;

      if (i0border | j0border) {
         red00   = tObj->BorderColor[0];
         green00 = tObj->BorderColor[1];
         blue00  = tObj->BorderColor[2];
         alpha00 = tObj->BorderColor[3];
      }
      else {
         get_2d_texel( tObj, img, i0, j0, &red00, &green00, &blue00, &alpha00);
      }
      if (i1border | j0border) {
         red10   = tObj->BorderColor[0];
         green10 = tObj->BorderColor[1];
         blue10  = tObj->BorderColor[2];
         alpha10 = tObj->BorderColor[3];
      }
      else {
         get_2d_texel( tObj, img, i1, j0, &red10, &green10, &blue10, &alpha10);
      }
      if (i0border | j1border) {
         red01   = tObj->BorderColor[0];
         green01 = tObj->BorderColor[1];
         blue01  = tObj->BorderColor[2];
         alpha01 = tObj->BorderColor[3];
      }
      else {
         get_2d_texel( tObj, img, i0, j1, &red01, &green01, &blue01, &alpha01);
      }
      if (i1border | j1border) {
         red11   = tObj->BorderColor[0];
         green11 = tObj->BorderColor[1];
         blue11  = tObj->BorderColor[2];
         alpha11 = tObj->BorderColor[3];
      }
      else {
         get_2d_texel( tObj, img, i1, j1, &red11, &green11, &blue11, &alpha11);
      }

      *red   = (w00*red00   + w10*red10   + w01*red01   + w11*red11  ) >> 8;
      *green = (w00*green00 + w10*green10 + w01*green01 + w11*green11) >> 8;
      *blue  = (w00*blue00  + w10*blue10  + w01*blue01  + w11*blue11 ) >> 8;
      *alpha = (w00*alpha00 + w10*alpha10 + w01*alpha01 + w11*alpha11) >> 8;
   }
}



static void
sample_2d_nearest_mipmap_nearest( const struct gl_texture_object *tObj,
                                  GLfloat s, GLfloat t, GLfloat lambda,
                                  GLubyte *red, GLubyte *green,
                                  GLubyte *blue, GLubyte *alpha )
{
   GLint level;
   if (lambda<=0.5F) {
      level = 0;
   }
   else {
      GLint max = tObj->Image[0]->MaxLog2;
      level = (GLint) (lambda + 0.499999F);
      if (level>max) {
         level = max;
      }
   }
   sample_2d_nearest( tObj, tObj->Image[level],
                      s, t, red, green, blue, alpha );
}



static void
sample_2d_linear_mipmap_nearest( const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat t, GLfloat lambda,
                                 GLubyte *red, GLubyte *green,
                                 GLubyte *blue, GLubyte *alpha )
{
   GLint level;
   if (lambda<=0.5F) {
      level = 0;
   }
   else {
      GLint max = tObj->Image[0]->MaxLog2;
      level = (GLint) (lambda + 0.499999F);
      if (level>max) {
         level = max;
      }
   }
   sample_2d_linear( tObj, tObj->Image[level],
                     s, t, red, green, blue, alpha );
}



static void
sample_2d_nearest_mipmap_linear( const struct gl_texture_object *tObj,
                                 GLfloat s, GLfloat t, GLfloat lambda,
                                 GLubyte *red, GLubyte *green,
                                 GLubyte *blue, GLubyte *alpha )
{
   GLint max = tObj->Image[0]->MaxLog2;

   if (lambda>=max) {
      sample_2d_nearest( tObj, tObj->Image[max],
                         s, t, red, green, blue, alpha );
   }
   else {
      GLubyte red0, green0, blue0, alpha0;
      GLubyte red1, green1, blue1, alpha1;
      GLfloat f = frac(lambda);
      GLint level = (GLint) (lambda + 1.0F);
      level = CLAMP( level, 1, max );
      sample_2d_nearest( tObj, tObj->Image[level-1], s, t,
                         &red0, &green0, &blue0, &alpha0 );
      sample_2d_nearest( tObj, tObj->Image[level], s, t,
                         &red1, &green1, &blue1, &alpha1 );
      *red   = (1.0F-f)*red0   + f*red1;
      *green = (1.0F-f)*green0 + f*green1;
      *blue  = (1.0F-f)*blue0  + f*blue1;
      *alpha = (1.0F-f)*alpha0 + f*alpha1;
   }
}



static void
sample_2d_linear_mipmap_linear( const struct gl_texture_object *tObj,
                                GLfloat s, GLfloat t, GLfloat lambda,
                                GLubyte *red, GLubyte *green,
                                GLubyte *blue, GLubyte *alpha )
{
   GLint max = tObj->Image[0]->MaxLog2;

   if (lambda>=max) {
      sample_2d_linear( tObj, tObj->Image[max],
                        s, t, red, green, blue, alpha );
   }
   else {
      GLubyte red0, green0, blue0, alpha0;
      GLubyte red1, green1, blue1, alpha1;
      GLfloat f = frac(lambda);
      GLint level = (GLint) (lambda + 1.0F);
      level = CLAMP( level, 1, max );
      sample_2d_linear( tObj, tObj->Image[level-1], s, t,
                        &red0, &green0, &blue0, &alpha0 );
      sample_2d_linear( tObj, tObj->Image[level], s, t,
                        &red1, &green1, &blue1, &alpha1 );
      *red   = (1.0F-f)*red0   + f*red1;
      *green = (1.0F-f)*green0 + f*green1;
      *blue  = (1.0F-f)*blue0  + f*blue1;
      *alpha = (1.0F-f)*alpha0 + f*alpha1;
   }
}



static void sample_nearest_2d( const struct gl_texture_object *tObj, GLuint n,
                               const GLfloat s[], const GLfloat t[],
                               const GLfloat u[], const GLfloat lambda[],
                               GLubyte red[], GLubyte green[], GLubyte blue[],
                               GLubyte alpha[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      sample_2d_nearest( tObj, tObj->Image[0], s[i], t[i],
                         &red[i], &green[i], &blue[i], &alpha[i]);
   }
}



static void sample_linear_2d( const struct gl_texture_object *tObj, GLuint n,
                              const GLfloat s[], const GLfloat t[],
                              const GLfloat u[], const GLfloat lambda[],
                              GLubyte red[], GLubyte green[], GLubyte blue[],
                              GLubyte alpha[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      sample_2d_linear( tObj, tObj->Image[0], s[i], t[i],
                        &red[i], &green[i], &blue[i], &alpha[i]);
   }
}


/*
 * Given an (s,t) texture coordinate and lambda (level of detail) value,
 * return a texture sample.
 */
static void sample_lambda_2d( const struct gl_texture_object *tObj,
                              GLuint n,
                              const GLfloat s[], const GLfloat t[],
                              const GLfloat u[], const GLfloat lambda[],
                              GLubyte red[], GLubyte green[], GLubyte blue[],
                              GLubyte alpha[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      if (lambda[i] > tObj->MinMagThresh) {
         /* minification */
         switch (tObj->MinFilter) {
            case GL_NEAREST:
               sample_2d_nearest( tObj, tObj->Image[0], s[i], t[i],
                                  &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR:
               sample_2d_linear( tObj, tObj->Image[0], s[i], t[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_NEAREST_MIPMAP_NEAREST:
               sample_2d_nearest_mipmap_nearest( tObj, s[i], t[i], lambda[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR_MIPMAP_NEAREST:
               sample_2d_linear_mipmap_nearest( tObj, s[i], t[i], lambda[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_NEAREST_MIPMAP_LINEAR:
               sample_2d_nearest_mipmap_linear( tObj, s[i], t[i], lambda[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR_MIPMAP_LINEAR:
               sample_2d_linear_mipmap_linear( tObj, s[i], t[i], lambda[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            default:
               gl_problem(NULL, "Bad min filter in sample_2d_texture");
               return;
         }
      }
      else {
         /* magnification */
         switch (tObj->MagFilter) {
            case GL_NEAREST:
               sample_2d_nearest( tObj, tObj->Image[0], s[i], t[i],
                                  &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            case GL_LINEAR:
               sample_2d_linear( tObj, tObj->Image[0], s[i], t[i],
                                 &red[i], &green[i], &blue[i], &alpha[i] );
               break;
            default:
               gl_problem(NULL, "Bad mag filter in sample_2d_texture");
         }
      }
   }
}


/*
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    No border
 *    Format = GL_RGB
 */
static void opt_sample_rgb_2d( const struct gl_texture_object *tObj,
                               GLuint n, const GLfloat s[], const GLfloat t[],
                               const GLfloat u[], const GLfloat lamda[],
                               GLubyte red[], GLubyte green[],
                               GLubyte blue[], GLubyte alpha[] )
{
   const struct gl_texture_image *img = tObj->Image[0];
   GLfloat width = img->Width, height = img->Height;
   GLint colMask = img->Width-1, rowMask = img->Height-1;
   GLint shift = img->WidthLog2;
   GLuint k;

   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(img->Border==0);
   ASSERT(img->Format==GL_RGB);

   for (k=0;k<n;k++) {
      GLint i = (GLint) (s[k] * width) & colMask;
      GLint j = (GLint) (t[k] * height) & rowMask;
      GLint pos = (j << shift) | i;
      GLubyte *texel = img->Data + pos + pos + pos;  /* pos*3 */
      red[k]   = texel[0];
      green[k] = texel[1];
      blue[k]  = texel[2];
   }
}


/*
 * Optimized 2-D texture sampling:
 *    S and T wrap mode == GL_REPEAT
 *    No border
 *    Format = GL_RGBA
 */
static void opt_sample_rgba_2d( const struct gl_texture_object *tObj,
                                GLuint n, const GLfloat s[], const GLfloat t[],
                                const GLfloat u[], const GLfloat lamda[],
                                GLubyte red[], GLubyte green[],
                                GLubyte blue[], GLubyte alpha[] )
{
   const struct gl_texture_image *img = tObj->Image[0];
   GLfloat width = img->Width, height = img->Height;
   GLint colMask = img->Width-1, rowMask = img->Height-1;
   GLint shift = img->WidthLog2;
   GLuint k;

   ASSERT(tObj->WrapS==GL_REPEAT);
   ASSERT(tObj->WrapT==GL_REPEAT);
   ASSERT(img->Border==0);
   ASSERT(img->Format==GL_RGBA);

   for (k=0;k<n;k++) {
      GLint i = (GLint) (s[k] * width) & colMask;
      GLint j = (GLint) (t[k] * height) & rowMask;
      GLint pos = (j << shift) | i;
      GLubyte *texel = img->Data + (pos << 2);    /* pos*4 */
      red[k]   = texel[0];
      green[k] = texel[1];
      blue[k]  = texel[2];
      alpha[k] = texel[3];
   }
}


/**********************************************************************/
/*                       Texture Sampling Setup                       */
/**********************************************************************/


/*
 * Setup the texture sampling function for this texture object.
 */
void gl_set_texture_sampler( struct gl_texture_object *t )
{
   if (!t->Complete) {
      t->SampleFunc = NULL;
   }
   else {
      GLboolean needLambda = (t->MinFilter != t->MagFilter);

      if (needLambda) {
         /* Compute min/mag filter threshold */
         if (t->MagFilter==GL_LINEAR
             && (t->MinFilter==GL_NEAREST_MIPMAP_NEAREST ||
                 t->MinFilter==GL_LINEAR_MIPMAP_NEAREST)) {
            t->MinMagThresh = 0.5F;
         }
         else {
            t->MinMagThresh = 0.0F;
         }
      }

      switch (t->Dimensions) {
         case 1:
            if (needLambda) {
               t->SampleFunc = sample_lambda_1d;
            }
            else if (t->MinFilter==GL_LINEAR) {
               t->SampleFunc = sample_linear_1d;
            }
            else {
               ASSERT(t->MinFilter==GL_NEAREST);
               t->SampleFunc = sample_nearest_1d;
            }
            break;
         case 2:
            if (needLambda) {
               t->SampleFunc = sample_lambda_2d;
            }
            else if (t->MinFilter==GL_LINEAR) {
               t->SampleFunc = sample_linear_2d;
            }
            else {
               ASSERT(t->MinFilter==GL_NEAREST);
               if (t->WrapS==GL_REPEAT && t->WrapT==GL_REPEAT
                   && t->Image[0]->Border==0 && t->Image[0]->Format==GL_RGB) {
                  t->SampleFunc = opt_sample_rgb_2d;
               }
               else if (t->WrapS==GL_REPEAT && t->WrapT==GL_REPEAT
                   && t->Image[0]->Border==0 && t->Image[0]->Format==GL_RGBA) {
                  t->SampleFunc = opt_sample_rgba_2d;
               }
               else
                  t->SampleFunc = sample_nearest_2d;
            }
            break;
         default:
            gl_problem(NULL, "invalid dimensions in gl_set_texture_sampler");
      }
   }
}



/**********************************************************************/
/*                      Texture Application                           */
/**********************************************************************/


/*
 * Combine incoming fragment color with texel color to produce output color.
 * Input:  n - number of fragments
 *         format - base internal texture format
 *         env_mode - texture environment mode
 *         Rt, Gt, Bt, At - array of texel colors
 * InOut:  red, green, blue, alpha - incoming fragment colors modified
 *                                   by texel colors according to the
 *                                   texture environment mode.
 */
static void apply_texture( GLcontext *ctx,
         GLuint n, GLint format, GLenum env_mode,
	 GLubyte red[], GLubyte green[], GLubyte blue[], GLubyte alpha[],
	 GLubyte Rt[], GLubyte Gt[], GLubyte Bt[], GLubyte At[] )
{
   GLuint i;
   GLint Rc, Gc, Bc, Ac;

   if (!ctx->Visual->EightBitColor) {
      /* This is a hack!  Rescale input colors from [0,scale] to [0,255]. */
      GLfloat rscale = 255.0 * ctx->Visual->InvRedScale;
      GLfloat gscale = 255.0 * ctx->Visual->InvGreenScale;
      GLfloat bscale = 255.0 * ctx->Visual->InvBlueScale;
      GLfloat ascale = 255.0 * ctx->Visual->InvAlphaScale;
      for (i=0;i<n;i++) {
	 red[i]   = (GLint) (red[i]   * rscale);
	 green[i] = (GLint) (green[i] * gscale);
	 blue[i]  = (GLint) (blue[i]  * bscale);
	 alpha[i] = (GLint) (alpha[i] * ascale);
      }
   }

/*
 * Use (A*(B+1)) >> 8 as a fast approximation of (A*B)/255 for A
 * and B in [0,255]
 */
#define PROD(A,B)   (((GLint)(A) * ((GLint)(B)+1)) >> 8)

   if (format==GL_COLOR_INDEX) {
      format = GL_RGBA;  /* XXXX a hack! */
   }

   switch (env_mode) {
      case GL_REPLACE:
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
                  /* Av = At */
                  alpha[i] = At[i];
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Lt */
                  GLint Lt = Rt[i];
                  red[i] = green[i] = blue[i] = Lt;
                  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
                  GLint Lt = Rt[i];
		  /* Cv = Lt */
		  red[i] = green[i] = blue[i] = Lt;
		  /* Av = At */
		  alpha[i] = At[i];
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = It */
                  GLint It = Rt[i];
                  red[i] = green[i] = blue[i] = It;
                  /* Av = It */
                  alpha[i] = It;
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  red[i]   = Rt[i];
		  green[i] = Gt[i];
		  blue[i]  = Bt[i];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  red[i]   = Rt[i];
		  green[i] = Gt[i];
		  blue[i]  = Bt[i];
		  /* Av = At */
		  alpha[i] = At[i];
	       }
	       break;
            default:
               gl_problem(ctx, "Bad format in apply_texture");
               return;
	 }
	 break;

      case GL_MODULATE:
         switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
		  alpha[i] = PROD( alpha[i], At[i] );
	       }
	       break;
	    case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = LtCf */
                  GLint Lt = Rt[i];
		  red[i]   = PROD( red[i],   Lt );
		  green[i] = PROD( green[i], Lt );
		  blue[i]  = PROD( blue[i],  Lt );
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfLt */
                  GLint Lt = Rt[i];
		  red[i]   = PROD( red[i],   Lt );
		  green[i] = PROD( green[i], Lt );
		  blue[i]  = PROD( blue[i],  Lt );
		  /* Av = AfAt */
		  alpha[i] = PROD( alpha[i], At[i] );
	       }
	       break;
	    case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = CfIt */
                  GLint It = Rt[i];
		  red[i]   = PROD( red[i],   It );
		  green[i] = PROD( green[i], It );
		  blue[i]  = PROD( blue[i],  It );
		  /* Av = AfIt */
		  alpha[i] = PROD( alpha[i], It );
	       }
	       break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  red[i]   = PROD( red[i],   Rt[i] );
		  green[i] = PROD( green[i], Gt[i] );
		  blue[i]  = PROD( blue[i],  Bt[i] );
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = CfCt */
		  red[i]   = PROD( red[i],   Rt[i] );
		  green[i] = PROD( green[i], Gt[i] );
		  blue[i]  = PROD( blue[i],  Bt[i] );
		  /* Av = AfAt */
		  alpha[i] = PROD( alpha[i], At[i] );
	       }
	       break;
            default:
               gl_problem(ctx, "Bad format (2) in apply_texture");
               return;
	 }
	 break;

      case GL_DECAL:
         switch (format) {
            case GL_ALPHA:
            case GL_LUMINANCE:
            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
               /* undefined */
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Ct */
		  red[i]   = Rt[i];
		  green[i] = Gt[i];
		  blue[i]  = Bt[i];
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-At) + CtAt */
		  GLint t = At[i], s = 255 - t;
		  red[i]   = PROD(red[i],  s) + PROD(Rt[i],t);
		  green[i] = PROD(green[i],s) + PROD(Gt[i],t);
		  blue[i]  = PROD(blue[i], s) + PROD(Bt[i],t);
		  /* Av = Af */
	       }
	       break;
            default:
               gl_problem(ctx, "Bad format (3) in apply_texture");
               return;
	 }
	 break;

      case GL_BLEND:
         Rc = (GLint) (ctx->Texture.EnvColor[0] * 255.0F);
         Gc = (GLint) (ctx->Texture.EnvColor[1] * 255.0F);
         Bc = (GLint) (ctx->Texture.EnvColor[2] * 255.0F);
         Ac = (GLint) (ctx->Texture.EnvColor[3] * 255.0F);
	 switch (format) {
	    case GL_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf */
		  /* Av = AfAt */
                  alpha[i] = PROD(alpha[i], At[i]);
	       }
	       break;
            case GL_LUMINANCE:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLint Lt = Rt[i], s = 255 - Lt;
		  red[i]   = PROD(red[i],  s) + PROD(Rc,  Lt);
		  green[i] = PROD(green[i],s) + PROD(Gc,Lt);
		  blue[i]  = PROD(blue[i], s) + PROD(Bc, Lt);
		  /* Av = Af */
	       }
	       break;
	    case GL_LUMINANCE_ALPHA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Lt) + CcLt */
		  GLint Lt = Rt[i], s = 255 - Lt;
		  red[i]   = PROD(red[i],  s) + PROD(Rc,  Lt);
		  green[i] = PROD(green[i],s) + PROD(Gc,Lt);
		  blue[i]  = PROD(blue[i], s) + PROD(Bc, Lt);
		  /* Av = AfAt */
		  alpha[i] = PROD(alpha[i],At[i]);
	       }
	       break;
            case GL_INTENSITY:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-It) + CcLt */
		  GLint It = Rt[i], s = 255 - It;
		  red[i]   = PROD(red[i],  s) + PROD(Rc,It);
		  green[i] = PROD(green[i],s) + PROD(Gc,It);
		  blue[i]  = PROD(blue[i], s) + PROD(Bc,It);
                  /* Av = Af(1-It) + Ac*It */
                  alpha[i] = PROD(alpha[i],s) + PROD(Ac,It);
               }
               break;
	    case GL_RGB:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  red[i]   = PROD(red[i],  (255-Rt[i])) + PROD(Rc,Rt[i]);
		  green[i] = PROD(green[i],(255-Gt[i])) + PROD(Gc,Gt[i]);
		  blue[i]  = PROD(blue[i], (255-Bt[i])) + PROD(Bc,Bt[i]);
		  /* Av = Af */
	       }
	       break;
	    case GL_RGBA:
	       for (i=0;i<n;i++) {
		  /* Cv = Cf(1-Ct) + CcCt */
		  red[i]   = PROD(red[i],  (255-Rt[i])) + PROD(Rc,Rt[i]);
		  green[i] = PROD(green[i],(255-Gt[i])) + PROD(Gc,Gt[i]);
		  blue[i]  = PROD(blue[i], (255-Bt[i])) + PROD(Bc,Bt[i]);
		  /* Av = AfAt */
		  alpha[i] = PROD(alpha[i],At[i]);
	       }
	       break;
	 }
	 break;

      default:
         gl_problem(ctx, "Bad env mode in apply_texture");
         return;
   }
#undef PROD

   if (!ctx->Visual->EightBitColor) {
      /* This is a hack!  Rescale input colors from [0,255] to [0,scale]. */
      GLfloat rscale = ctx->Visual->RedScale   * (1.0F/ 255.0F);
      GLfloat gscale = ctx->Visual->GreenScale * (1.0F/ 255.0F);
      GLfloat bscale = ctx->Visual->BlueScale  * (1.0F/ 255.0F);
      GLfloat ascale = ctx->Visual->AlphaScale * (1.0F/ 255.0F);
      for (i=0;i<n;i++) {
	 red[i]   = (GLint) (red[i]   * rscale);
	 green[i] = (GLint) (green[i] * gscale);
	 blue[i]  = (GLint) (blue[i]  * bscale);
	 alpha[i] = (GLint) (alpha[i] * ascale);
      }
   }
}



void gl_texture_pixels( GLcontext *ctx, GLuint n,
                        const GLfloat s[], const GLfloat t[],
                        const GLfloat r[], const GLfloat lambda[],
                        GLubyte red[], GLubyte green[],
                        GLubyte blue[], GLubyte alpha[] )
{
   GLubyte tred[PB_SIZE];
   GLubyte tgreen[PB_SIZE];
   GLubyte tblue[PB_SIZE];
   GLubyte talpha[PB_SIZE];

   if (!ctx->Texture.Current || !ctx->Texture.Current->SampleFunc)
      return;

   /* Sample the texture. */
   (*ctx->Texture.Current->SampleFunc)( ctx->Texture.Current, n,
                                        s, t, r, lambda,
                                        tred, tgreen, tblue, talpha );

   apply_texture( ctx, n,
                  ctx->Texture.Current->Image[0]->Format,
                  ctx->Texture.EnvMode,
                  red, green, blue, alpha,
                  tred, tgreen, tblue, talpha );
}
