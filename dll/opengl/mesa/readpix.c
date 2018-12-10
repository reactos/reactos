/* $Id: readpix.c,v 1.10 1997/07/24 01:25:18 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
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
 * $Log: readpix.c,v $
 * Revision 1.10  1997/07/24 01:25:18  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.9  1997/05/28 03:26:18  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.8  1997/05/08 01:43:50  brianp
 * added error check to gl_ReadPixels() for inside glBegin/glEnd
 *
 * Revision 1.7  1997/02/03 20:31:15  brianp
 * added a few DEFARRAY macros for BeOS
 *
 * Revision 1.6  1997/01/16 19:24:05  brianp
 * replaced a few abort()'s with gl_error() calls
 *
 * Revision 1.5  1996/12/20 20:28:04  brianp
 * use DEF/UNDEFARRAY() macros in read_color_pixels() for Mac compilers
 *
 * Revision 1.4  1996/11/01 03:20:47  brianp
 * reading GL_LUMINANCE pixels weren't clamped
 *
 * Revision 1.3  1996/09/27 01:29:47  brianp
 * added missing default cases to switches
 *
 * Revision 1.2  1996/09/15 14:18:37  brianp
 * now use GLframebuffer and GLvisual
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
#include <string.h>
#include "alphabuf.h"
#include "context.h"
#include "depth.h"
#include "feedback.h"
#include "dlist.h"
#include "macros.h"
#include "image.h"
#include "readpix.h"
#include "span.h"
#include "stencil.h"
#include "types.h"
#endif




/*
 * Read a block of color index pixels.
 */
static void read_index_pixels( GLcontext *ctx,
                               GLint x, GLint y,
			       GLsizei width, GLsizei height,
			       GLenum type, GLvoid *pixels )
{
   GLint i, j;
   GLuint a, s, k, l, start;

   /* error checking */
   if (ctx->Visual->RGBAflag) {
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   /* Size of each component */
   s = gl_sizeof_type( type );
   if (s<=0) {
      gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
      return;
   }

   /* Compute packing parameters */
   a = ctx->Pack.Alignment;
   if (ctx->Pack.RowLength>0) {
      l = ctx->Pack.RowLength;
   }
   else {
      l = width;
   }
   /* k = offset between rows in components */
   if (s>=a) {
      k = l;
   }
   else {
      k = a/s * CEILING( s*l, a );
   }

   /* offset to first component returned */
   start = ctx->Pack.SkipRows * k + ctx->Pack.SkipPixels;

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLuint index[MAX_WIDTH];
      (*ctx->Driver.ReadIndexSpan)( ctx, width, x, y, index );

      if (ctx->Pixel.IndexShift!=0 || ctx->Pixel.IndexOffset!=0) {
	 GLuint s;
	 if (ctx->Pixel.IndexShift<0) {
	    /* right shift */
	    s = -ctx->Pixel.IndexShift;
	    for (i=0;i<width;i++) {
	       index[i] = (index[i] >> s) + ctx->Pixel.IndexOffset;
	    }
	 }
	 else {
	    /* left shift */
	    s = ctx->Pixel.IndexShift;
	    for (i=0;i<width;i++) {
	       index[i] = (index[i] << s) + ctx->Pixel.IndexOffset;
	    }
	 }
      }

      if (ctx->Pixel.MapColorFlag) {
	 for (i=0;i<width;i++) {
	    index[i] = ctx->Pixel.MapItoI[ index[i] ];
	 }
      }

      switch (type) {
	 case GL_UNSIGNED_BYTE:
	    {
	       GLubyte *dst = (GLubyte *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLubyte) index[i];
	       }
	    }
	    break;
	 case GL_BYTE:
	    {
	       GLbyte *dst = (GLbyte *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLbyte) index[i];
	       }
	    }
	    break;
	 case GL_UNSIGNED_SHORT:
	    {
	       GLushort *dst = (GLushort *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLushort) index[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap2( (GLushort *) pixels + start + j * k, width );
	       }
	    }
	    break;
	 case GL_SHORT:
	    {
	       GLshort *dst = (GLshort *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLshort) index[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap2( (GLushort *) pixels + start + j * k, width );
	       }
	    }
	    break;
	 case GL_UNSIGNED_INT:
	    {
	       GLuint *dst = (GLuint *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLuint) index[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap4( (GLuint *) pixels + start + j * k, width );
	       }
	    }
	    break;
	 case GL_INT:
	    {
	       GLint *dst = (GLint *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLint) index[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap4( (GLuint *) pixels + start + j * k, width );
	       }
	    }
	    break;
	 case GL_FLOAT:
	    {
	       GLfloat *dst = (GLfloat *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLfloat) index[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap4( (GLuint *) pixels + start + j * k, width );
	       }
	    }
	    break;
         default:
            gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
      }
   }
}



static void read_depth_pixels( GLcontext *ctx,
                               GLint x, GLint y,
			       GLsizei width, GLsizei height,
			       GLenum type, GLvoid *pixels )
{
   GLint i, j;
   GLuint a, s, k, l, start;
   GLboolean bias_or_scale;

   /* Error checking */
   if (ctx->Visual->DepthBits<=0) {
      /* No depth buffer */
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;

   /* Size of each component */
   s = gl_sizeof_type( type );
   if (s<=0) {
      gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
      return;
   }

   /* Compute packing parameters */
   a = ctx->Pack.Alignment;
   if (ctx->Pack.RowLength>0) {
      l = ctx->Pack.RowLength;
   }
   else {
      l = width;
   }
   /* k = offset between rows in components */
   if (s>=a) {
      k = l;
   }
   else {
      k = a/s * CEILING( s*l, a );
   }

   /* offset to first component returned */
   start = ctx->Pack.SkipRows * k + ctx->Pack.SkipPixels;

   if (type==GL_UNSIGNED_INT && !bias_or_scale && !ctx->Pack.SwapBytes) {
      /* Special case: directly read 32-bit unsigned depth values. */
      /* Compute shift value to scale depth values up to 32-bit uints. */
      GLuint shift = 0;
      GLuint max = MAX_DEPTH;
      while ((max&0x80000000)==0) {
         max = max << 1;
         shift++;
      }
      for (j=0;j<height;j++,y++) {
         GLuint *dst = (GLuint *) pixels + start + j * k;
         (*ctx->Driver.ReadDepthSpanInt)( ctx, width, x, y, (GLdepth*) dst);
         for (i=0;i<width;i++) {
            dst[i] = dst[i] << shift;
         }
      }
   }
   else {
      /* General case (slow) */
      for (j=0;j<height;j++,y++) {
         GLfloat depth[MAX_WIDTH];

         (*ctx->Driver.ReadDepthSpanFloat)( ctx, width, x, y, depth );

         if (bias_or_scale) {
            for (i=0;i<width;i++) {
               GLfloat d;
               d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
               depth[i] = CLAMP( d, 0.0, 1.0 );
            }
         }

         switch (type) {
            case GL_UNSIGNED_BYTE:
               {
                  GLubyte *dst = (GLubyte *) pixels + start + j * k;
                  for (i=0;i<width;i++) {
                     *dst++ = FLOAT_TO_UBYTE( depth[i] );
                  }
               }
               break;
            case GL_BYTE:
               {
                  GLbyte *dst = (GLbyte *) pixels + start + j * k;
                  for (i=0;i<width;i++) {
                     *dst++ = FLOAT_TO_BYTE( depth[i] );
                  }
               }
               break;
            case GL_UNSIGNED_SHORT:
               {
                  GLushort *dst = (GLushort *) pixels + start + j * k;
                  for (i=0;i<width;i++) {
                     *dst++ = FLOAT_TO_USHORT( depth[i] );
                  }
                  if (ctx->Pack.SwapBytes) {
                     gl_swap2( (GLushort *) pixels + start + j * k, width );
                  }
               }
               break;
            case GL_SHORT:
               {
                  GLshort *dst = (GLshort *) pixels + start + j * k;
                  for (i=0;i<width;i++) {
                     *dst++ = FLOAT_TO_SHORT( depth[i] );
                  }
                  if (ctx->Pack.SwapBytes) {
                     gl_swap2( (GLushort *) pixels + start + j * k, width );
                  }
               }
               break;
            case GL_UNSIGNED_INT:
               {
                  GLuint *dst = (GLuint *) pixels + start + j * k;
                  for (i=0;i<width;i++) {
                     *dst++ = FLOAT_TO_UINT( depth[i] );
                  }
                  if (ctx->Pack.SwapBytes) {
                     gl_swap4( (GLuint *) pixels + start + j * k, width );
                  }
               }
               break;
            case GL_INT:
               {
                  GLint *dst = (GLint *) pixels + start + j * k;
                  for (i=0;i<width;i++) {
                     *dst++ = FLOAT_TO_INT( depth[i] );
                  }
                  if (ctx->Pack.SwapBytes) {
                     gl_swap4( (GLuint *) pixels + start + j * k, width );
                  }
               }
               break;
            case GL_FLOAT:
               {
                  GLfloat *dst = (GLfloat *) pixels + start + j * k;
                  for (i=0;i<width;i++) {
                     *dst++ = depth[i];
                  }
                  if (ctx->Pack.SwapBytes) {
                     gl_swap4( (GLuint *) pixels + start + j * k, width );
                  }
               }
               break;
            default:
               gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
         }
      }
   }
}




static void read_stencil_pixels( GLcontext *ctx,
                                 GLint x, GLint y,
				 GLsizei width, GLsizei height,
				 GLenum type, GLvoid *pixels )
{
   GLint i, j;
   GLuint a, s, k, l, start;
   GLboolean shift_or_offset;

   if (ctx->Visual->StencilBits<=0) {
      /* No stencil buffer */
      gl_error( ctx, GL_INVALID_OPERATION, "glReadPixels" );
      return;
   }

   shift_or_offset = ctx->Pixel.IndexShift!=0 || ctx->Pixel.IndexOffset!=0;

   /* Size of each component */
   s = gl_sizeof_type( type );
   if (s<=0) {
      gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
      return;
   }

   /* Compute packing parameters */
   a = ctx->Pack.Alignment;
   if (ctx->Pack.RowLength>0) {
      l = ctx->Pack.RowLength;
   }
   else {
      l = width;
   }
   /* k = offset between rows in components */
   if (s>=a) {
      k = l;
   }
   else {
      k = a/s * CEILING( s*l, a );
   }

   /* offset to first component returned */
   start = ctx->Pack.SkipRows * k + ctx->Pack.SkipPixels;

   /* process image row by row */
   for (j=0;j<height;j++,y++) {
      GLubyte stencil[MAX_WIDTH];

      gl_read_stencil_span( ctx, width, x, y, stencil );

      if (shift_or_offset) {
	 GLuint s;
	 if (ctx->Pixel.IndexShift<0) {
	    /* right shift */
	    s = -ctx->Pixel.IndexShift;
	    for (i=0;i<width;i++) {
	       stencil[i] = (stencil[i] >> s) + ctx->Pixel.IndexOffset;
	    }
	 }
	 else {
	    /* left shift */
	    s = ctx->Pixel.IndexShift;
	    for (i=0;i<width;i++) {
	       stencil[i] = (stencil[i] << s) + ctx->Pixel.IndexOffset;
	    }
	 }
      }

      if (ctx->Pixel.MapStencilFlag) {
	 for (i=0;i<width;i++) {
	    stencil[i] = ctx->Pixel.MapStoS[ stencil[i] ];
	 }
      }

      switch (type) {
	 case GL_UNSIGNED_BYTE:
	    {
	       GLubyte *dst = (GLubyte *) pixels + start + j * k;
	       MEMCPY( dst, stencil, width );
	    }
	    break;
	 case GL_BYTE:
	    {
	       GLbyte *dst = (GLbyte  *) pixels + start + j * k;
	       MEMCPY( dst, stencil, width );
	    }
	    break;
	 case GL_UNSIGNED_SHORT:
	    {
	       GLushort *dst = (GLushort *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLushort) stencil[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap2( (GLushort *) pixels + start +j * k, width );
	       }
	    }
	    break;
	 case GL_SHORT:
	    {
	       GLshort *dst = (GLshort *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLshort) stencil[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap2( (GLushort *) pixels + start +j * k, width );
	       }
	    }
	    break;
	 case GL_UNSIGNED_INT:
	    {
	       GLuint *dst = (GLuint *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLuint) stencil[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap4( (GLuint *) pixels + start +j * k, width );
	       }
	    }
	    break;
	 case GL_INT:
	    {
	       GLint *dst = (GLint *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLint) stencil[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap4( (GLuint *) pixels + start +j * k, width );
	       }
	    }
	    break;
	 case GL_FLOAT:
	    {
	       GLfloat *dst = (GLfloat *) pixels + start + j * k;
	       for (i=0;i<width;i++) {
		  *dst++ = (GLfloat) stencil[i];
	       }
	       if (ctx->Pack.SwapBytes) {
		  gl_swap4( (GLuint *) pixels + start +j * k, width );
	       }
	    }
	    break;
         default:
            gl_error( ctx, GL_INVALID_ENUM, "glReadPixels(type)" );
      }
   }
}



/*
 * Test if scaling or biasing of colors is needed.
 */
static GLboolean scale_or_bias_rgba( GLcontext *ctx )
{
   if (ctx->Pixel.RedScale!=1.0F   || ctx->Pixel.RedBias!=0.0F ||
       ctx->Pixel.GreenScale!=1.0F || ctx->Pixel.GreenBias!=0.0F ||
       ctx->Pixel.BlueScale!=1.0F  || ctx->Pixel.BlueBias!=0.0F ||
       ctx->Pixel.AlphaScale!=1.0F || ctx->Pixel.AlphaBias!=0.0F) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}



/*
 * Apply scale and bias factors to an array of RGBA pixels.
 */
static void scale_and_bias_rgba( GLcontext *ctx,
                                 GLint n,
				 GLfloat red[], GLfloat green[],
				 GLfloat blue[], GLfloat alpha[] )
{
   register GLint i;
   register GLfloat r, g, b, a;

   for (i=0;i<n;i++) {
      r = red[i]   * ctx->Pixel.RedScale   + ctx->Pixel.RedBias;
      g = green[i] * ctx->Pixel.GreenScale + ctx->Pixel.GreenBias;
      b = blue[i]  * ctx->Pixel.BlueScale  + ctx->Pixel.BlueBias;
      a = alpha[i] * ctx->Pixel.AlphaScale + ctx->Pixel.AlphaBias;
      red[i]   = CLAMP( r, 0.0F, 1.0F );
      green[i] = CLAMP( g, 0.0F, 1.0F );
      blue[i]  = CLAMP( b, 0.0F, 1.0F );
      alpha[i] = CLAMP( a, 0.0F, 1.0F );
   }
}



/*
 * Apply pixel mapping to an array of RGBA pixels.
 */
static void map_rgba( GLcontext *ctx,
                      GLint n,
		      GLfloat red[], GLfloat green[],
		      GLfloat blue[], GLfloat alpha[] )
{
   GLfloat rscale = ctx->Pixel.MapRtoRsize-1;
   GLfloat gscale = ctx->Pixel.MapGtoGsize-1;
   GLfloat bscale = ctx->Pixel.MapBtoBsize-1;
   GLfloat ascale = ctx->Pixel.MapAtoAsize-1;
   GLint i;

   for (i=0;i<n;i++) {
      red[i]   = ctx->Pixel.MapRtoR[ (GLint) (red[i]   * rscale) ];
      green[i] = ctx->Pixel.MapGtoG[ (GLint) (green[i] * gscale) ];
      blue[i]  = ctx->Pixel.MapBtoB[ (GLint) (blue[i]  * bscale) ];
      alpha[i] = ctx->Pixel.MapAtoA[ (GLint) (alpha[i] * ascale) ];
   }
}




/*
 * Read R, G, B, A, RGB, L, or LA pixels.
 */
static void read_color_pixels(GLcontext *ctx, GLint x, GLint y, GLsizei width,
        GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    GLint i, j, n, a, s, l, k;
    GLboolean scale_or_bias;
    DEFARRAY(GLfloat, red, MAX_WIDTH);
    DEFARRAY(GLfloat, green, MAX_WIDTH);
    DEFARRAY(GLfloat, blue, MAX_WIDTH);
    DEFARRAY(GLfloat, alpha, MAX_WIDTH);
    DEFARRAY(GLfloat, luminance, MAX_WIDTH);
    GLboolean r_flag, g_flag, b_flag, a_flag, l_flag;
    GLboolean is_bgr = GL_FALSE;
    GLuint start;

    scale_or_bias = scale_or_bias_rgba(ctx);

    /* Determine how many / which components to return */
    r_flag = g_flag = b_flag = a_flag = l_flag = GL_FALSE;
    switch (format)
    {
    case GL_RED:
        r_flag = GL_TRUE;
        n = 1;
        break;
    case GL_GREEN:
        g_flag = GL_TRUE;
        n = 1;
        break;
    case GL_BLUE:
        b_flag = GL_TRUE;
        n = 1;
        break;
    case GL_ALPHA:
        a_flag = GL_TRUE;
        n = 1;
        break;
    case GL_LUMINANCE:
        l_flag = GL_TRUE;
        n = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        l_flag = a_flag = GL_TRUE;
        n = 2;
        break;
    case GL_RGB:
        r_flag = g_flag = b_flag = GL_TRUE;
        n = 3;
        break;
    case GL_BGR_EXT:
        r_flag = g_flag = b_flag = GL_TRUE;
        n = 3;
        is_bgr = GL_TRUE;
        break;
    case GL_RGBA:
        r_flag = g_flag = b_flag = a_flag = GL_TRUE;
        n = 4;
        break;
    case GL_BGRA_EXT:
        r_flag = g_flag = b_flag = a_flag = GL_TRUE;
        n = 4;
        is_bgr = GL_TRUE;
        break;
    default:
        gl_error(ctx, GL_INVALID_ENUM, "glReadPixels(format)");
        UNDEFARRAY( red );
        UNDEFARRAY( green );
        UNDEFARRAY( blue );
        UNDEFARRAY( alpha );
        UNDEFARRAY( luminance );
        return;
    }

    /* Size of each component */
    s = gl_sizeof_type(type);
    if (s <= 0)
    {
        gl_error(ctx, GL_INVALID_ENUM, "glReadPixels(type)");
        UNDEFARRAY( red );
        UNDEFARRAY( green );
        UNDEFARRAY( blue );
        UNDEFARRAY( alpha );
        UNDEFARRAY( luminance );
        return;
    }

    /* Compute packing parameters */
    a = ctx->Pack.Alignment;
    if (ctx->Pack.RowLength > 0)
    {
        l = ctx->Pack.RowLength;
    }
    else
    {
        l = width;
    }
    /* k = offset between rows in components */
    if (s >= a)
    {
        k = n * l;
    }
    else
    {
        k = a / s * CEILING(s * n * l, a);
    }

    /* offset to first component returned */
    start = ctx->Pack.SkipRows * k + ctx->Pack.SkipPixels * n;

    /* process image row by row */
    for (j = 0; j < height; j++, y++)
    {
        /*
         * Read the pixels from frame buffer
         */
        if (ctx->Visual->RGBAflag)
        {
            DEFARRAY(GLubyte, r, MAX_WIDTH);
            DEFARRAY(GLubyte, g, MAX_WIDTH);
            DEFARRAY(GLubyte, b, MAX_WIDTH);
            DEFARRAY(GLubyte, a, MAX_WIDTH);
            GLfloat rscale = 1.0F * ctx->Visual->InvRedScale;
            GLfloat gscale = 1.0F * ctx->Visual->InvGreenScale;
            GLfloat bscale = 1.0F * ctx->Visual->InvBlueScale;
            GLfloat ascale = 1.0F * ctx->Visual->InvAlphaScale;

            /* read colors and convert to floats */
            (*ctx->Driver.ReadColorSpan)(ctx, width, x, y, r, g, b, a);
            if (ctx->RasterMask & ALPHABUF_BIT)
            {
                gl_read_alpha_span(ctx, width, x, y, a);
            }
            for (i = 0; i < width; i++)
            {
                red[i] = r[i] * rscale;
                green[i] = g[i] * gscale;
                blue[i] = b[i] * bscale;
                alpha[i] = a[i] * ascale;
            }

            if (scale_or_bias)
            {
                scale_and_bias_rgba(ctx, width, red, green, blue, alpha);
            }
            if (ctx->Pixel.MapColorFlag)
            {
                map_rgba(ctx, width, red, green, blue, alpha);
            }
            UNDEFARRAY(r);
            UNDEFARRAY(g);
            UNDEFARRAY(b);
            UNDEFARRAY(a);
        }
        else
        {
            /* convert CI values to RGBA */
            GLuint index[MAX_WIDTH];
            (*ctx->Driver.ReadIndexSpan)(ctx, width, x, y, index);

            if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset != 0)
            {
                GLuint s;
                if (ctx->Pixel.IndexShift < 0)
                {
                    /* right shift */
                    s = -ctx->Pixel.IndexShift;
                    for (i = 0; i < width; i++)
                    {
                        index[i] = (index[i] >> s) + ctx->Pixel.IndexOffset;
                    }
                }
                else
                {
                    /* left shift */
                    s = ctx->Pixel.IndexShift;
                    for (i = 0; i < width; i++)
                    {
                        index[i] = (index[i] << s) + ctx->Pixel.IndexOffset;
                    }
                }
            }

            for (i = 0; i < width; i++)
            {
                red[i] = ctx->Pixel.MapItoR[index[i]];
                green[i] = ctx->Pixel.MapItoG[index[i]];
                blue[i] = ctx->Pixel.MapItoB[index[i]];
                alpha[i] = ctx->Pixel.MapItoA[index[i]];
            }
        }

        if (l_flag)
        {
            for (i = 0; i < width; i++)
            {
                GLfloat sum = red[i] + green[i] + blue[i];
                luminance[i] = CLAMP(sum, 0.0F, 1.0F);
            }
        }

        /*
         * Pack/transfer/store the pixels
         */

        switch (type)
        {
        case GL_UNSIGNED_BYTE:
        {
            GLubyte *dst = (GLubyte *) pixels + start + j * k;
            for (i = 0; i < width; i++)
            {
                if (is_bgr)
                {
                    if (b_flag)
                        *dst++ = FLOAT_TO_UBYTE(blue[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_UBYTE(green[i]);
                    if (r_flag)
                        *dst++ = FLOAT_TO_UBYTE(red[i]);
                }
                else
                {
                    if (r_flag)
                        *dst++ = FLOAT_TO_UBYTE(red[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_UBYTE(green[i]);
                    if (b_flag)
                        *dst++ = FLOAT_TO_UBYTE(blue[i]);
                }
                if (l_flag)
                    *dst++ = FLOAT_TO_UBYTE(luminance[i]);
                if (a_flag)
                    *dst++ = FLOAT_TO_UBYTE(alpha[i]);
            }
            break;
        }
        case GL_BYTE:
        {
            GLbyte *dst = (GLbyte *) pixels + start + j * k;
            for (i = 0; i < width; i++)
            {
                if (is_bgr)
                {
                    if (b_flag)
                        *dst++ = FLOAT_TO_BYTE(blue[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_BYTE(green[i]);
                    if (r_flag)
                        *dst++ = FLOAT_TO_BYTE(red[i]);
                }
                else
                {
                    if (r_flag)
                        *dst++ = FLOAT_TO_BYTE(red[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_BYTE(green[i]);
                    if (b_flag)
                        *dst++ = FLOAT_TO_BYTE(blue[i]);
                }
                if (l_flag)
                    *dst++ = FLOAT_TO_BYTE(luminance[i]);
                if (a_flag)
                    *dst++ = FLOAT_TO_BYTE(alpha[i]);
            }
            break;
        }
        case GL_UNSIGNED_SHORT:
        {
            GLushort *dst = (GLushort *) pixels + start + j * k;
            for (i = 0; i < width; i++)
            {
                if (is_bgr)
                {
                    if (b_flag)
                        *dst++ = FLOAT_TO_USHORT(blue[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_USHORT(green[i]);
                    if (r_flag)
                        *dst++ = FLOAT_TO_USHORT(red[i]);
                }
                else
                {
                    if (r_flag)
                        *dst++ = FLOAT_TO_USHORT(red[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_USHORT(green[i]);
                    if (b_flag)
                        *dst++ = FLOAT_TO_USHORT(blue[i]);
                }
                if (l_flag)
                    *dst++ = FLOAT_TO_USHORT(luminance[i]);
                if (a_flag)
                    *dst++ = FLOAT_TO_USHORT(alpha[i]);
            }
            if (ctx->Pack.SwapBytes)
            {
                gl_swap2((GLushort *) pixels + start + j * k, width * n);
            }
            break;
        }
        case GL_SHORT:
        {
            GLshort *dst = (GLshort *) pixels + start + j * k;
            for (i = 0; i < width; i++)
            {
                if (is_bgr)
                {
                    if (b_flag)
                        *dst++ = FLOAT_TO_SHORT(blue[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_SHORT(green[i]);
                    if (r_flag)
                        *dst++ = FLOAT_TO_SHORT(red[i]);
                }
                else
                {
                    if (r_flag)
                        *dst++ = FLOAT_TO_SHORT(red[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_SHORT(green[i]);
                    if (b_flag)
                        *dst++ = FLOAT_TO_SHORT(blue[i]);
                }
                if (l_flag)
                    *dst++ = FLOAT_TO_SHORT(luminance[i]);
                if (a_flag)
                    *dst++ = FLOAT_TO_SHORT(alpha[i]);
            }
            if (ctx->Pack.SwapBytes)
            {
                gl_swap2((GLushort *) pixels + start + j * k, width * n);
            }
            break;
        }
        case GL_UNSIGNED_INT:
        {
            GLuint *dst = (GLuint *) pixels + start + j * k;
            for (i = 0; i < width; i++)
            {
                if (is_bgr)
                {
                    if (b_flag)
                        *dst++ = FLOAT_TO_UINT(blue[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_UINT(green[i]);
                    if (r_flag)
                        *dst++ = FLOAT_TO_UINT(red[i]);
                }
                else
                {
                    if (r_flag)
                        *dst++ = FLOAT_TO_UINT(red[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_UINT(green[i]);
                    if (b_flag)
                        *dst++ = FLOAT_TO_UINT(blue[i]);
                }
                if (l_flag)
                    *dst++ = FLOAT_TO_UINT(luminance[i]);
                if (a_flag)
                    *dst++ = FLOAT_TO_UINT(alpha[i]);
            }
            if (ctx->Pack.SwapBytes)
            {
                gl_swap4((GLuint *) pixels + start + j * k, width * n);
            }
            break;
        }
        case GL_INT:
        {
            GLint *dst = (GLint *) pixels + start + j * k;
            for (i = 0; i < width; i++)
            {
                if (is_bgr)
                {
                    if (b_flag)
                        *dst++ = FLOAT_TO_INT(blue[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_INT(green[i]);
                    if (r_flag)
                        *dst++ = FLOAT_TO_INT(red[i]);
                }
                else
                {
                    if (r_flag)
                        *dst++ = FLOAT_TO_INT(red[i]);
                    if (g_flag)
                        *dst++ = FLOAT_TO_INT(green[i]);
                    if (b_flag)
                        *dst++ = FLOAT_TO_INT(blue[i]);
                }
                if (l_flag)
                    *dst++ = FLOAT_TO_INT(luminance[i]);
                if (a_flag)
                    *dst++ = FLOAT_TO_INT(alpha[i]);
            }
            if (ctx->Pack.SwapBytes)
            {
                gl_swap4((GLuint *) pixels + start + j * k, width * n);
            }
            break;
        }
        case GL_FLOAT:
        {
            GLfloat *dst = (GLfloat *) pixels + start + j * k;
            for (i = 0; i < width; i++)
            {
                if (is_bgr)
                {
                    if (b_flag)
                        *dst++ = blue[i];
                    if (g_flag)
                        *dst++ = green[i];
                    if (r_flag)
                        *dst++ = red[i];
                }
                else
                {
                    if (r_flag)
                        *dst++ = red[i];
                    if (g_flag)
                        *dst++ = green[i];
                    if (b_flag)
                        *dst++ = blue[i];
                }
                if (l_flag)
                    *dst++ = luminance[i];
                if (a_flag)
                    *dst++ = alpha[i];
            }
            if (ctx->Pack.SwapBytes)
            {
                gl_swap4((GLuint *) pixels + start + j * k, width * n);
            }
            break;
        }
        default:
            gl_error(ctx, GL_INVALID_ENUM, "glReadPixels(type)");
        }
    }
    UNDEFARRAY( red );
    UNDEFARRAY( green );
    UNDEFARRAY( blue );
    UNDEFARRAY( alpha );
    UNDEFARRAY( luminance );
}



void gl_ReadPixels( GLcontext *ctx,
                    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type, GLvoid *pixels )
{
    if (INSIDE_BEGIN_END(ctx))
    {
        gl_error(ctx, GL_INVALID_OPERATION, "glReadPixels");
        return;
    }

    (void) (*ctx->Driver.SetBuffer)(ctx, ctx->Pixel.ReadBuffer);

    switch (format)
    {
    case GL_COLOR_INDEX:
        read_index_pixels(ctx, x, y, width, height, type, pixels);
        break;
    case GL_STENCIL_INDEX:
        read_stencil_pixels(ctx, x, y, width, height, type, pixels);
        break;
    case GL_DEPTH_COMPONENT:
        read_depth_pixels(ctx, x, y, width, height, type, pixels);
        break;
    case GL_RED:
    case GL_GREEN:
    case GL_BLUE:
    case GL_ALPHA:
    case GL_RGB:
    case GL_BGR_EXT:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
    case GL_RGBA:
    case GL_BGRA_EXT:
        read_color_pixels(ctx, x, y, width, height, format, type, pixels);
        break;
    default:
        gl_error(ctx, GL_INVALID_ENUM, "glReadPixels(format)");
    }

    (void) (*ctx->Driver.SetBuffer)(ctx, ctx->Color.DrawBuffer);
}
