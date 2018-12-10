/* $Id: drawpix.c,v 1.16 1998/02/03 23:45:02 brianp Exp $ */

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
 * $Log: drawpix.c,v $
 * Revision 1.16  1998/02/03 23:45:02  brianp
 * added casts to prevent warnings with Amiga StormC compiler
 *
 * Revision 1.15  1997/08/26 02:10:11  brianp
 * another bug fix related to glPixelStore()
 *
 * Revision 1.14  1997/08/24 02:31:23  brianp
 * bug fix: glPixelStore() params weren't ignored during display list execute
 *
 * Revision 1.13  1997/07/24 01:25:01  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.12  1997/06/20 02:18:23  brianp
 * replaced Current.IntColor with Current.ByteColor
 *
 * Revision 1.11  1997/05/28 03:24:22  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.10  1997/04/24 00:18:56  brianp
 * added some missing UNDEFARRAY()s.  Reported by Randy Frank.
 *
 * Revision 1.9  1997/04/20 20:28:49  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.8  1997/04/11 23:24:25  brianp
 * move call to gl_update_state() into gl_DrawPixels() from drawpixels()
 *
 * Revision 1.7  1997/03/18 01:55:47  brianp
 * only generate feedback/selection if raster position is valid
 *
 * Revision 1.6  1997/02/10 20:26:57  brianp
 * fixed memory leak in quickdraw_rgb()
 *
 * Revision 1.5  1997/02/03 20:30:31  brianp
 * added a few DEFARRAY macros for BeOS
 *
 * Revision 1.4  1996/10/16 00:58:53  brianp
 * renamed gl_drawpixels() to drawpixels()
 *
 * Revision 1.3  1996/09/27 01:26:25  brianp
 * added missing default cases to switches
 *
 * Revision 1.2  1996/09/15 14:17:30  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "drawpix.h"
#include "feedback.h"
#include "dlist.h"
#include "macros.h"
#include "pixel.h"
#include "span.h"
#include "stencil.h"
#include "types.h"
#endif



/* TODO:  apply texture mapping to fragments */



static void draw_index_pixels( GLcontext* ctx, GLsizei width, GLsizei height,
			       GLenum type, const GLvoid *pixels )
{
   GLint x, y, desty;
   GLuint i, j;
   GLdepth zspan[MAX_WIDTH];
   GLboolean zoom;

   zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;

   /* Position, depth of pixels */
   x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
   y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
   desty = y;
   if (ctx->Depth.Test) {
      GLdepth zval = (GLdepth) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
	 zspan[i] = zval;
      }
   }

   /* process the image row by row */
   for (i=0;i<height;i++,y++) {
      GLuint ispan[MAX_WIDTH];

      /* convert to uints */
      switch (type) {
	 case GL_UNSIGNED_BYTE:
	    {
	       GLubyte *src = (GLubyte *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) *src++;
	       }
	    }
	    break;
	 case GL_BYTE:
	    {
	       GLbyte *src = (GLbyte *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) *src++;
	       }
	    }
	    break;
	 case GL_UNSIGNED_SHORT:
	    {
	       GLushort *src = (GLushort *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) *src++;
	       }
	    }
	    break;
	 case GL_SHORT:
	    {
	       GLshort *src = (GLshort *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) *src++;
	       }
	    }
	    break;
	 case GL_UNSIGNED_INT:
	    {
	       GLuint *src = (GLuint *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = *src++;
	       }
	    }
	    break;
	 case GL_INT:
	    {
	       GLint *src = (GLint *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) *src++;
	       }
	    }
	    break;
	 case GL_BITMAP:
	    /* TODO */
	    break;
	 case GL_FLOAT:
	    {
	       GLfloat *src = (GLfloat *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  ispan[j] = (GLuint) (GLint) *src++;
	       }
	    }
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_ENUM, "Internal: draw_index_pixels" );
      }

      /* apply shift and offset */
      if (ctx->Pixel.IndexOffset || ctx->Pixel.IndexShift) {
	 if (ctx->Pixel.IndexShift>=0) {
	    for (j=0;j<width;j++) {
	       ispan[j] = (ispan[j] << ctx->Pixel.IndexShift)
		          + ctx->Pixel.IndexOffset;
	    }
	 }
	 else {
	    for (j=0;j<width;j++) {
	       ispan[j] = (ispan[j] >> -ctx->Pixel.IndexShift)
		          + ctx->Pixel.IndexOffset;
	    }
	 }
      }

      if (ctx->Visual->RGBAflag) {
	 /* Convert index to RGBA and write to frame buffer */
	 GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
	 GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
	 for (j=0;j<width;j++) {
	    red[j]   = (GLint) (ctx->Pixel.MapItoR[ispan[j]] * ctx->Visual->RedScale);
	    green[j] = (GLint) (ctx->Pixel.MapItoG[ispan[j]] * ctx->Visual->GreenScale);
	    blue[j]  = (GLint) (ctx->Pixel.MapItoB[ispan[j]] * ctx->Visual->BlueScale);
	    alpha[j] = (GLint) (ctx->Pixel.MapItoA[ispan[j]] * ctx->Visual->AlphaScale);
	 }
         if (zoom) {
            gl_write_zoomed_color_span( ctx, width, x, y, zspan,
                                        red, green, blue, alpha, desty );
         }
         else {
            gl_write_color_span( ctx, width, x, y, zspan,
                                 red, green, blue, alpha, GL_BITMAP );
         }
      }
      else {
	 /* optionally apply index map then write to frame buffer */
	 if (ctx->Pixel.MapColorFlag) {
	    for (j=0;j<width;j++) {
	       ispan[j] = ctx->Pixel.MapItoI[ispan[j]];
	    }
	 }
         if (zoom) {
            gl_write_zoomed_index_span( ctx, width, x, y, zspan, ispan, desty );
         }
         else {
            gl_write_index_span( ctx, width, x, y, zspan, ispan, GL_BITMAP );
         }
      }
   }

}



static void draw_stencil_pixels( GLcontext* ctx, GLsizei width, GLsizei height,
			         GLenum type, const GLvoid *pixels )
{
   GLint x, y, desty;
   GLuint i, j;
   GLboolean zoom;

   zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;

   /* Position, depth of pixels */
   x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
   y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
   desty = y;

   /* process the image row by row */
   for (i=0;i<height;i++,y++) {
      GLubyte stencil[MAX_WIDTH];

      /* convert to ubytes */
      switch (type) {
	 case GL_UNSIGNED_BYTE:
	    {
	       GLubyte *src = (GLubyte *) pixels + i * width;
	       MEMCPY( stencil, src, width );
	    }
	    break;
	 case GL_BYTE:
	    {
	       GLbyte *src = (GLbyte *) pixels + i * width;
	       MEMCPY( stencil, src, width );
	    }
	    break;
	 case GL_UNSIGNED_SHORT:
	    {
	       GLushort *src = (GLushort *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  stencil[j] = (GLubyte) ((*src++) & 0xff);
	       }
	    }
	    break;
	 case GL_SHORT:
	    {
	       GLshort *src = (GLshort *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  stencil[j] = (GLubyte) ((*src++) & 0xff);
	       }
	    }
	    break;
	 case GL_UNSIGNED_INT:
	    {
	       GLuint *src = (GLuint *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  stencil[j] = (GLubyte) ((*src++) & 0xff);
	       }
	    }
	    break;
	 case GL_INT:
	    {
	       GLint *src = (GLint *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  stencil[j] = (GLubyte) ((*src++) & 0xff);
	       }
	    }
	    break;
	 case GL_BITMAP:
	    /* TODO */
	    break;
	 case GL_FLOAT:
	    {
	       GLfloat *src = (GLfloat *) pixels + i * width;
	       for (j=0;j<width;j++) {
		  stencil[j] = (GLubyte) (((GLint) *src++) & 0xff);
	       }
	    }
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_ENUM, "Internal: draw_stencil_pixels" );
      }

      /* apply shift and offset */
      if (ctx->Pixel.IndexOffset || ctx->Pixel.IndexShift) {
	 if (ctx->Pixel.IndexShift>=0) {
	    for (j=0;j<width;j++) {
	       stencil[j] = (stencil[j] << ctx->Pixel.IndexShift)
		          + ctx->Pixel.IndexOffset;
	    }
	 }
	 else {
	    for (j=0;j<width;j++) {
	       stencil[j] = (stencil[j] >> -ctx->Pixel.IndexShift)
		          + ctx->Pixel.IndexOffset;
	    }
	 }
      }

      /* mapping */
      if (ctx->Pixel.MapStencilFlag) {
	 for (j=0;j<width;j++) {
	    stencil[j] = ctx->Pixel.MapStoS[ stencil[j] ];
	 }
      }

      /* write stencil values to stencil buffer */
      if (zoom) {
         gl_write_zoomed_stencil_span( ctx, (GLuint) width, x, y, stencil, desty );
      }
      else {
         gl_write_stencil_span( ctx, (GLuint) width, x, y, stencil );
      }
   }
}



static void draw_depth_pixels( GLcontext* ctx, GLsizei width, GLsizei height,
			       GLenum type, const GLvoid *pixels )
{
   GLint x, y, desty;
   GLubyte red[MAX_WIDTH], green[MAX_WIDTH], blue[MAX_WIDTH], alpha[MAX_WIDTH];
   GLuint ispan[MAX_WIDTH];
   GLboolean bias_or_scale;
   GLboolean zoom;

   bias_or_scale = ctx->Pixel.DepthBias!=0.0 || ctx->Pixel.DepthScale!=1.0;
   zoom = ctx->Pixel.ZoomX!=1.0 || ctx->Pixel.ZoomY!=1.0;

   /* Position, depth of pixels */
   x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
   y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
   desty = y;

   /* Color or index */
   if (ctx->Visual->RGBAflag) {
      GLint r, g, b, a;
      r = (GLint) (ctx->Current.RasterColor[0] * ctx->Visual->RedScale);
      g = (GLint) (ctx->Current.RasterColor[1] * ctx->Visual->GreenScale);
      b = (GLint) (ctx->Current.RasterColor[2] * ctx->Visual->BlueScale);
      a = (GLint) (ctx->Current.RasterColor[3] * ctx->Visual->AlphaScale);
      MEMSET( red,   r, width );
      MEMSET( green, g, width );
      MEMSET( blue,  b, width );
      MEMSET( alpha, a, width );
   }
   else {
      GLuint i;
      for (i=0;i<width;i++) {
	 ispan[i] = ctx->Current.RasterIndex;
      }
   }

   if (type==GL_UNSIGNED_INT && !bias_or_scale && !zoom && ctx->Visual->RGBAflag)
   {
      /* Special case: directly write 32-bit depth values */
      GLuint i, j;
      /* Compute shift value to scale 32-bit uints down to depth values. */
      GLuint shift = 0;
      GLuint max = MAX_DEPTH;
      while ((max&0x80000000)==0) {
         max = max << 1;
         shift++;
      }
      for (j=0;j<height;j++,y++) {
         GLdepth zspan[MAX_WIDTH];
         GLuint *zptr = (GLuint *) pixels + j * width;
         for (i=0;i<width;i++) {
            zspan[i] = zptr[i] >> shift;
         }
         gl_write_color_span( ctx, width, x, y, zspan,
                              red, green, blue, alpha, GL_BITMAP );
      }
   }
   else {
      /* General case (slower) */
      GLuint i, j;

      /* process image row by row */
      for (i=0;i<height;i++,y++) {
         GLfloat depth[MAX_WIDTH];
         GLdepth zspan[MAX_WIDTH];

         switch (type) {
            case GL_UNSIGNED_BYTE:
               {
                  GLubyte *src = (GLubyte *) pixels + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = UBYTE_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_BYTE:
               {
                  GLbyte *src = (GLbyte *) pixels + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = BYTE_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_UNSIGNED_SHORT:
               {
                  GLushort *src = (GLushort *) pixels + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = USHORT_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_SHORT:
               {
                  GLshort *src = (GLshort *) pixels + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = SHORT_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_UNSIGNED_INT:
               {
                  GLuint *src = (GLuint *) pixels + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = UINT_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_INT:
               {
                  GLint *src = (GLint *) pixels + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = INT_TO_FLOAT( *src++ );
                  }
               }
               break;
            case GL_FLOAT:
               {
                  GLfloat *src = (GLfloat *) pixels + i * width;
                  for (j=0;j<width;j++) {
                     depth[j] = *src++;
                  }
               }
               break;
            default:
               gl_problem(ctx, "Bad type in draw_depth_pixels");
               return;
         }

         /* apply depth scale and bias */
         if (ctx->Pixel.DepthScale!=1.0 || ctx->Pixel.DepthBias!=0.0) {
            for (j=0;j<width;j++) {
               depth[j] = depth[j] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
            }
         }

         /* clamp depth values to [0,1] and convert from floats to integers */
         for (j=0;j<width;j++) {
            zspan[j] = (GLdepth) (CLAMP( depth[j], 0.0F, 1.0F ) * DEPTH_SCALE);
         }

         if (ctx->Visual->RGBAflag) {
            if (zoom) {
               gl_write_zoomed_color_span( ctx, width, x, y, zspan,
                                           red, green, blue, alpha, desty );
            }
            else {
               gl_write_color_span( ctx, width, x, y, zspan,
                                    red, green, blue, alpha, GL_BITMAP );
            }
         }
         else {
            if (zoom) {
               gl_write_zoomed_index_span( ctx, width, x, y, zspan,
                                           ispan, GL_BITMAP );
            }
            else {
               gl_write_index_span( ctx, width, x, y, zspan, ispan, GL_BITMAP );
            }
         }

      }
   }
}



static void draw_color_pixels(GLcontext* ctx, GLsizei width, GLsizei height,
        GLenum format, GLenum type, const GLvoid *pixels)
{
    GLuint i, j;
    GLint x, y, desty;
    GLdepth zspan[MAX_WIDTH];
    GLboolean scale_or_bias, quick_draw;
    GLboolean zoom;

    zoom = ctx->Pixel.ZoomX != 1.0 || ctx->Pixel.ZoomY != 1.0;

    /* Position, depth of pixels */
    x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
    y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
    desty = y;
    if (ctx->Depth.Test)
    {
        /* fill in array of z values */
        GLdepth z = (GLdepth) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
        for (i = 0; i < width; i++)
        {
            zspan[i] = z;
        }
    }

    /* Determine if scaling and/or biasing is needed */
    if (ctx->Pixel.RedScale != 1.0F || ctx->Pixel.RedBias != 0.0F
            || ctx->Pixel.GreenScale != 1.0F || ctx->Pixel.GreenBias != 0.0F
            || ctx->Pixel.BlueScale != 1.0F || ctx->Pixel.BlueBias != 0.0F
            || ctx->Pixel.AlphaScale != 1.0F || ctx->Pixel.AlphaBias != 0.0F)
    {
        scale_or_bias = GL_TRUE;
    }
    else
    {
        scale_or_bias = GL_FALSE;
    }

    /* Determine if we can directly call the device driver function */
    if (ctx->RasterMask == 0 && !zoom && x >= 0 && y >= 0
            && x + width <= ctx->Buffer->Width
            && y + height <= ctx->Buffer->Height)
    {
        quick_draw = GL_TRUE;
    }
    else
    {
        quick_draw = GL_FALSE;
    }

    /* First check for common cases */
    if (type == GL_UNSIGNED_BYTE
            && (format == GL_RGB || format == GL_LUMINANCE || format == GL_BGR_EXT)
            && !ctx->Pixel.MapColorFlag
            && !scale_or_bias && ctx->Visual->EightBitColor)
    {
        DEFARRAY(GLubyte, alpha, MAX_WIDTH);
        GLubyte *src = (GLubyte *) pixels;
        /* constant alpha */
        MEMSET(alpha, (GLint ) ctx->Visual->AlphaScale, width);
        if (format == GL_RGB)
        {
            /* 8-bit RGB pixels */
            DEFARRAY(GLubyte, red, MAX_WIDTH);
            DEFARRAY(GLubyte, green, MAX_WIDTH);
            DEFARRAY(GLubyte, blue, MAX_WIDTH);
            for (i = 0; i < height; i++, y++)
            {
                for (j = 0; j < width; j++)
                {
                    red[j] = *src++;
                    green[j] = *src++;
                    blue[j] = *src++;
                }
                if (quick_draw)
                {
                    (*ctx->Driver.WriteColorSpan)(ctx, width, x, y, red, green,
                            blue, alpha, NULL);
                }
                else if (zoom)
                {
                    gl_write_zoomed_color_span(ctx, (GLuint) width, x, y, zspan,
                            red, green, blue, alpha, desty);
                }
                else
                {
                    gl_write_color_span(ctx, (GLuint) width, x, y, zspan, red,
                            green, blue, alpha, GL_BITMAP);
                }
            }
            UNDEFARRAY( red );
            UNDEFARRAY( green );
            UNDEFARRAY( blue );
        }
        else if (format == GL_BGR_EXT)
        {
            /* 8-bit BGR pixels */
            DEFARRAY(GLubyte, red, MAX_WIDTH);
            DEFARRAY(GLubyte, green, MAX_WIDTH);
            DEFARRAY(GLubyte, blue, MAX_WIDTH);
            for (i = 0; i < height; i++, y++)
            {
                for (j = 0; j < width; j++)
                {
                    blue[j] = *src++;
                    green[j] = *src++;
                    red[j] = *src++;
                }
                if (quick_draw)
                {
                    (*ctx->Driver.WriteColorSpan)(ctx, width, x, y, red, green,
                            blue, alpha, NULL);
                }
                else if (zoom)
                {
                    gl_write_zoomed_color_span(ctx, (GLuint) width, x, y, zspan,
                            red, green, blue, alpha, desty);
                }
                else
                {
                    gl_write_color_span(ctx, (GLuint) width, x, y, zspan, red,
                            green, blue, alpha, GL_BITMAP);
                }
            }
            UNDEFARRAY( red );
            UNDEFARRAY( green );
            UNDEFARRAY( blue );
        }
        else
        {
            /* 8-bit Luminance pixels */
            GLubyte *lum = (GLubyte *) pixels;
            for (i = 0; i < height; i++, y++, lum += width)
            {
                if (quick_draw)
                {
                    (*ctx->Driver.WriteColorSpan)(ctx, width, x, y, lum, lum,
                            lum, alpha, NULL);
                }
                else if (zoom)
                {
                    gl_write_zoomed_color_span(ctx, (GLuint) width, x, y, zspan,
                            lum, lum, lum, alpha, desty);
                }
                else
                {
                    gl_write_color_span(ctx, (GLuint) width, x, y, zspan, lum,
                            lum, lum, alpha, GL_BITMAP);
                }
            }
        }
        UNDEFARRAY( alpha );
    }
    else
    {
        /* General solution */
        GLboolean r_flag, g_flag, b_flag, a_flag, l_flag;
        GLuint components;
        GLboolean is_bgr;

        r_flag = g_flag = b_flag = a_flag = l_flag = GL_FALSE;
        is_bgr = GL_FALSE;
        switch (format)
        {
        case GL_RED:
            r_flag = GL_TRUE;
            components = 1;
            break;
        case GL_GREEN:
            g_flag = GL_TRUE;
            components = 1;
            break;
        case GL_BLUE:
            b_flag = GL_TRUE;
            components = 1;
            break;
        case GL_ALPHA:
            a_flag = GL_TRUE;
            components = 1;
            break;
        case GL_RGB:
            r_flag = g_flag = b_flag = GL_TRUE;
            components = 3;
            break;
        case GL_BGR_EXT:
            is_bgr = GL_TRUE;
            r_flag = g_flag = b_flag = GL_TRUE;
            components = 3;
            break;
        case GL_LUMINANCE:
            l_flag = GL_TRUE;
            components = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            l_flag = a_flag = GL_TRUE;
            components = 2;
            break;
        case GL_RGBA:
            r_flag = g_flag = b_flag = a_flag = GL_TRUE;
            components = 4;
            break;
        case GL_BGRA_EXT:
            is_bgr = GL_TRUE;
            r_flag = g_flag = b_flag = a_flag = GL_TRUE;
            components = 4;
            break;
        default:
            gl_problem(ctx, "Bad type in draw_color_pixels");
            return;
        }

        /* process the image row by row */
        for (i = 0; i < height; i++, y++)
        {
            DEFARRAY(GLfloat, rf, MAX_WIDTH);
            DEFARRAY(GLfloat, gf, MAX_WIDTH);
            DEFARRAY(GLfloat, bf, MAX_WIDTH);
            DEFARRAY(GLfloat, af, MAX_WIDTH);
            DEFARRAY(GLubyte, red, MAX_WIDTH);
            DEFARRAY(GLubyte, green, MAX_WIDTH);
            DEFARRAY(GLubyte, blue, MAX_WIDTH);
            DEFARRAY(GLubyte, alpha, MAX_WIDTH);

            /* convert to floats */
            switch (type)
            {
            case GL_UNSIGNED_BYTE:
            {
                GLubyte *src = (GLubyte *) pixels + i * width * components;
                for (j = 0; j < width; j++)
                {
                    if (l_flag)
                    {
                        rf[j] = gf[j] = bf[j] = UBYTE_TO_FLOAT(*src++);
                    }
                    else if (is_bgr)
                    {
                        bf[j] = b_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
                        rf[j] = r_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
                    }
                    else
                    {
                        rf[j] = r_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
                        bf[j] = b_flag ? UBYTE_TO_FLOAT(*src++) : 0.0;
                    }
                    af[j] = a_flag ? UBYTE_TO_FLOAT(*src++) : 1.0;
                }
            }
                break;
            case GL_BYTE:
            {
                GLbyte *src = (GLbyte *) pixels + i * width * components;
                for (j = 0; j < width; j++)
                {
                    if (l_flag)
                    {
                        rf[j] = gf[j] = bf[j] = BYTE_TO_FLOAT(*src++);
                    }
                    else if (is_bgr)
                    {
                        bf[j] = b_flag ? BYTE_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? BYTE_TO_FLOAT(*src++) : 0.0;
                        rf[j] = r_flag ? BYTE_TO_FLOAT(*src++) : 0.0;
                    }
                    else
                    {
                        rf[j] = r_flag ? BYTE_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? BYTE_TO_FLOAT(*src++) : 0.0;
                        bf[j] = b_flag ? BYTE_TO_FLOAT(*src++) : 0.0;
                    }
                    af[j] = a_flag ? BYTE_TO_FLOAT(*src++) : 1.0;
                }
            }
                break;
            case GL_BITMAP:
                /* special case */
                break;
            case GL_UNSIGNED_SHORT:
            {
                GLushort *src = (GLushort *) pixels + i * width * components;
                for (j = 0; j < width; j++)
                {
                    if (l_flag)
                    {
                        rf[j] = gf[j] = bf[j] = USHORT_TO_FLOAT(*src++);
                    }
                    else if (is_bgr)
                    {
                        bf[j] = b_flag ? USHORT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? USHORT_TO_FLOAT(*src++) : 0.0;
                        rf[j] = r_flag ? USHORT_TO_FLOAT(*src++) : 0.0;
                    }
                    else
                    {
                        rf[j] = r_flag ? USHORT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? USHORT_TO_FLOAT(*src++) : 0.0;
                        bf[j] = b_flag ? USHORT_TO_FLOAT(*src++) : 0.0;
                    }
                    af[j] = a_flag ? USHORT_TO_FLOAT(*src++) : 1.0;
                }
            }
                break;
            case GL_SHORT:
            {
                GLshort *src = (GLshort *) pixels + i * width * components;
                for (j = 0; j < width; j++)
                {
                    if (l_flag)
                    {
                        rf[j] = gf[j] = bf[j] = SHORT_TO_FLOAT(*src++);
                    }
                    else if (is_bgr)
                    {
                        bf[j] = b_flag ? SHORT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? SHORT_TO_FLOAT(*src++) : 0.0;
                        rf[j] = r_flag ? SHORT_TO_FLOAT(*src++) : 0.0;
                    }
                    else
                    {
                        rf[j] = r_flag ? SHORT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? SHORT_TO_FLOAT(*src++) : 0.0;
                        bf[j] = b_flag ? SHORT_TO_FLOAT(*src++) : 0.0;
                    }
                    af[j] = a_flag ? SHORT_TO_FLOAT(*src++) : 1.0;
                }
            }
                break;
            case GL_UNSIGNED_INT:
            {
                GLuint *src = (GLuint *) pixels + i * width * components;
                for (j = 0; j < width; j++)
                {
                    if (l_flag)
                    {
                        rf[j] = gf[j] = bf[j] = UINT_TO_FLOAT(*src++);
                    }
                    else if (is_bgr)
                    {
                        bf[j] = b_flag ? UINT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? UINT_TO_FLOAT(*src++) : 0.0;
                        rf[j] = r_flag ? UINT_TO_FLOAT(*src++) : 0.0;
                    }
                    else
                    {
                        rf[j] = r_flag ? UINT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? UINT_TO_FLOAT(*src++) : 0.0;
                        bf[j] = b_flag ? UINT_TO_FLOAT(*src++) : 0.0;
                    }
                    af[j] = a_flag ? UINT_TO_FLOAT(*src++) : 1.0;
                }
            }
                break;
            case GL_INT:
            {
                GLint *src = (GLint *) pixels + i * width * components;
                for (j = 0; j < width; j++)
                {
                    if (l_flag)
                    {
                        rf[j] = gf[j] = bf[j] = INT_TO_FLOAT(*src++);
                    }
                    else if (is_bgr)
                    {
                        bf[j] = b_flag ? INT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? INT_TO_FLOAT(*src++) : 0.0;
                        rf[j] = r_flag ? INT_TO_FLOAT(*src++) : 0.0;
                    }
                    else
                    {
                        rf[j] = r_flag ? INT_TO_FLOAT(*src++) : 0.0;
                        gf[j] = g_flag ? INT_TO_FLOAT(*src++) : 0.0;
                        bf[j] = b_flag ? INT_TO_FLOAT(*src++) : 0.0;
                    }
                    af[j] = a_flag ? INT_TO_FLOAT(*src++) : 1.0;
                }
            }
                break;
            case GL_FLOAT:
            {
                GLfloat *src = (GLfloat *) pixels + i * width * components;
                for (j = 0; j < width; j++)
                {
                    if (l_flag)
                    {
                        rf[j] = gf[j] = bf[j] = *src++;
                    }
                    else if (is_bgr)
                    {
                        bf[j] = b_flag ? *src++ : 0.0;
                        gf[j] = g_flag ? *src++ : 0.0;
                        rf[j] = r_flag ? *src++ : 0.0;
                    }
                    else
                    {
                        rf[j] = r_flag ? *src++ : 0.0;
                        gf[j] = g_flag ? *src++ : 0.0;
                        bf[j] = b_flag ? *src++ : 0.0;
                    }
                    af[j] = a_flag ? *src++ : 1.0;
                }
            }
                break;
            default:
                gl_error(ctx, GL_INVALID_ENUM, "glDrawPixels");
                return;
            }

            /* apply scale and bias */
            if (scale_or_bias)
            {
                for (j = 0; j < width; j++)
                {
                    GLfloat r, g, b, a;
                    r = rf[j] * ctx->Pixel.RedScale + ctx->Pixel.RedBias;
                    g = gf[j] * ctx->Pixel.GreenScale + ctx->Pixel.GreenBias;
                    b = bf[j] * ctx->Pixel.BlueScale + ctx->Pixel.BlueBias;
                    a = af[j] * ctx->Pixel.AlphaScale + ctx->Pixel.AlphaBias;
                    rf[j] = CLAMP(r, 0.0, 1.0);
                    gf[j] = CLAMP(g, 0.0, 1.0);
                    bf[j] = CLAMP(b, 0.0, 1.0);
                    af[j] = CLAMP(a, 0.0, 1.0);
                }
            }

            /* apply pixel mappings */
            if (ctx->Pixel.MapColorFlag)
            {
                GLfloat rscale = ctx->Pixel.MapRtoRsize - 1;
                GLfloat gscale = ctx->Pixel.MapGtoGsize - 1;
                GLfloat bscale = ctx->Pixel.MapBtoBsize - 1;
                GLfloat ascale = ctx->Pixel.MapAtoAsize - 1;
                for (j = 0; j < width; j++)
                {
                    rf[j] = ctx->Pixel.MapRtoR[(GLint) (rf[j] * rscale)];
                    gf[j] = ctx->Pixel.MapGtoG[(GLint) (gf[j] * gscale)];
                    bf[j] = ctx->Pixel.MapBtoB[(GLint) (bf[j] * bscale)];
                    af[j] = ctx->Pixel.MapAtoA[(GLint) (af[j] * ascale)];
                }
            }

            /* convert to integers */
            for (j = 0; j < width; j++)
            {
                red[j] = (GLint) (rf[j] * ctx->Visual->RedScale);
                green[j] = (GLint) (gf[j] * ctx->Visual->GreenScale);
                blue[j] = (GLint) (bf[j] * ctx->Visual->BlueScale);
                alpha[j] = (GLint) (af[j] * ctx->Visual->AlphaScale);
            }

            /* write to frame buffer */
            if (quick_draw)
            {
                (*ctx->Driver.WriteColorSpan)(ctx, width, x, y, red, green,
                        blue, alpha, NULL);
            }
            else if (zoom)
            {
                gl_write_zoomed_color_span(ctx, width, x, y, zspan, red, green,
                        blue, alpha, desty);
            }
            else
            {
                gl_write_color_span(ctx, (GLuint) width, x, y, zspan, red,
                        green, blue, alpha, GL_BITMAP);
            }

            UNDEFARRAY(rf);
            UNDEFARRAY(gf);
            UNDEFARRAY(bf);
            UNDEFARRAY(af);
            UNDEFARRAY(red);
            UNDEFARRAY(green);
            UNDEFARRAY(blue);
            UNDEFARRAY(alpha);
        }
    }

}



/*
 * Do a glDrawPixels( w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels ) optimized
 * for the case of no pixel mapping, no scale, no bias, no zoom, default
 * storage mode, no raster ops, and no pixel clipping.
 * Return:  GL_TRUE if success
 *          GL_FALSE if conditions weren't met for optimized drawing
 */
static GLboolean quickdraw_rgb( GLcontext* ctx, GLsizei width, GLsizei height,
                                const void *pixels )
{
   DEFARRAY( GLubyte, red, MAX_WIDTH );
   DEFARRAY( GLubyte, green, MAX_WIDTH );
   DEFARRAY( GLubyte, blue, MAX_WIDTH );
   DEFARRAY( GLubyte, alpha, MAX_WIDTH );
   GLint i, j;
   GLint x, y;
   GLint bytes_per_row;
   GLboolean result;

   bytes_per_row = width * 3 + (width % ctx->Unpack.Alignment);

   if (!ctx->Current.RasterPosValid) {
      /* This is success, actually. */
      result = GL_TRUE;
   }
   else {
      x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);

      if (x<0 || y<0
          || x+width>ctx->Buffer->Width || y+height>ctx->Buffer->Height) {
         result = GL_FALSE;  /* can't handle this situation */
      }
      else {
         /* constant alpha */
         for (j=0;j<width;j++) {
            alpha[j] = (GLint) ctx->Visual->AlphaScale;
         }

         /* write directly to device driver */
         for (i=0;i<height;i++) {
            /* each row of pixel data starts at 4-byte boundary */
            GLubyte *src = (GLubyte *) pixels + i * bytes_per_row;
            for (j=0;j<width;j++) {
               red[j]   = *src++;
               green[j] = *src++;
               blue[j]  = *src++;
            }
            (*ctx->Driver.WriteColorSpan)( ctx, width, x, y+i,
                                           red, green, blue, alpha, NULL);
         }
         result = GL_TRUE;
      }
   }

   UNDEFARRAY( red );
   UNDEFARRAY( green );
   UNDEFARRAY( blue );
   UNDEFARRAY( alpha );

   return result;
}



/*
 * Implements general glDrawPixels operation.
 */
static void drawpixels( GLcontext* ctx, GLsizei width, GLsizei height,
                        GLenum format, GLenum type, const GLvoid *pixels )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawPixels" );
      return;
   }

   if (ctx->RenderMode==GL_RENDER) {
      if (!ctx->Current.RasterPosValid) {
	 return;
      }
      switch (format) {
	 case GL_COLOR_INDEX:
            draw_index_pixels( ctx, width, height, type, pixels );
	    break;
	 case GL_STENCIL_INDEX:
	    draw_stencil_pixels( ctx, width, height, type, pixels );
	    break;
	 case GL_DEPTH_COMPONENT:
	    draw_depth_pixels( ctx, width, height, type, pixels );
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
            draw_color_pixels( ctx, width, height, format, type, pixels );
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_ENUM, "glDrawPixels" );
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      if (ctx->Current.RasterPosValid) {
         GLfloat color[4], texcoord[4], invq;
         color[0] = ctx->Current.ByteColor[0] * ctx->Visual->InvRedScale;
         color[1] = ctx->Current.ByteColor[1] * ctx->Visual->InvGreenScale;
         color[2] = ctx->Current.ByteColor[2] * ctx->Visual->InvBlueScale;
         color[3] = ctx->Current.ByteColor[3] * ctx->Visual->InvAlphaScale;
         invq = 1.0F / ctx->Current.TexCoord[3];
         texcoord[0] = ctx->Current.TexCoord[0] * invq;
         texcoord[1] = ctx->Current.TexCoord[1] * invq;
         texcoord[2] = ctx->Current.TexCoord[2] * invq;
         texcoord[3] = ctx->Current.TexCoord[3];
         FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
         gl_feedback_vertex( ctx, ctx->Current.RasterPos[0],
                             ctx->Current.RasterPos[1],
                             ctx->Current.RasterPos[2],
                             ctx->Current.RasterPos[3],
                             color, ctx->Current.Index, texcoord );
      }
   }
   else if (ctx->RenderMode==GL_SELECT) {
      if (ctx->Current.RasterPosValid) {
         gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
      }
   }
}



/*
 * Compile OR Execute a glDrawPixels!
 */
void gl_DrawPixels( GLcontext* ctx, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, const GLvoid *pixels )
{
   GLvoid *image;

   if (width<0 || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDrawPixels" );
      return;
   }

   if (ctx->NewState) {
      gl_update_state(ctx);
   }

   /* Let the device driver take a crack at glDrawPixels */
   if (!ctx->CompileFlag && ctx->Driver.DrawPixels) {
      GLint x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      GLint y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);
      if ((*ctx->Driver.DrawPixels)( ctx, x, y, width, height,
                                      format, type, GL_FALSE, pixels )) {
         /* Device driver did the job */
         return;
      }
   }

   if (format==GL_RGB && type==GL_UNSIGNED_BYTE && ctx->FastDrawPixels
       && !ctx->CompileFlag && ctx->RenderMode==GL_RENDER
       && ctx->RasterMask==0 && ctx->CallDepth==0) {
      /* optimized path */
      if (quickdraw_rgb( ctx, width, height, pixels )) {
         /* success */
         return;
      }
   }

   /* take the general path */

   /* THIS IS A REAL HACK - FIX IN MESA 2.5
    * If we're inside glCallList then we don't have to unpack the pixels again.
    */
   if (ctx->CallDepth == 0) {
      image = gl_unpack_pixels( ctx, width, height, format, type, pixels );
      if (!image) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glDrawPixels" );
         return;
      }
   }
   else {
      image = (GLvoid *) pixels;
   }

   if (ctx->CompileFlag) {
      gl_save_DrawPixels( ctx, width, height, format, type, image );
   }
   if (ctx->ExecuteFlag) {
      drawpixels( ctx, width, height, format, type, image );
      if (!ctx->CompileFlag) {
         /* may discard unpacked image now */
         if (image!=pixels)
            free( image );
      }
   }
}
