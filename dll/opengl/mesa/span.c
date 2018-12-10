/* $Id: span.c,v 1.12 1997/08/14 01:12:37 brianp Exp $ */

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
 * $Log: span.c,v $
 * Revision 1.12  1997/08/14 01:12:37  brianp
 * replaced a few for loops with MEMSET calls
 *
 * Revision 1.11  1997/07/24 01:21:56  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.10  1997/05/28 03:26:29  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.9  1997/05/03 00:51:30  brianp
 * new texturing function call: gl_texture_pixels()
 *
 * Revision 1.8  1997/04/16 23:54:11  brianp
 * do per-pixel fog if texturing is enabled
 *
 * Revision 1.7  1997/02/09 19:53:43  brianp
 * now use TEXTURE_xD enable constants
 *
 * Revision 1.6  1997/02/09 18:43:34  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.5  1997/01/28 22:17:44  brianp
 * new RGBA mode logic op support
 *
 * Revision 1.4  1996/09/25 03:22:05  brianp
 * added NO_DRAW_BIT support
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


/*
 * pixel span rasterization:
 * These functions simulate the rasterization pipeline.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <string.h>
#include "alpha.h"
#include "alphabuf.h"
#include "blend.h"
#include "depth.h"
#include "fog.h"
#include "logic.h"
#include "macros.h"
#include "masking.h"
#include "scissor.h"
#include "span.h"
#include "stencil.h"
#include "texture.h"
#include "types.h"
#endif




/*
 * Apply the current polygon stipple pattern to a span of pixels.
 */
static void stipple_polygon_span( GLcontext *ctx,
                                  GLuint n, GLint x, GLint y, GLubyte mask[] )
{
   register GLuint i, m, stipple, highbit=0x80000000;

   stipple = ctx->PolygonStipple[y % 32];
   m = highbit >> (GLuint) (x % 32);

   for (i=0;i<n;i++) {
      if ((m & stipple)==0) {
	 mask[i] = 0;
      }
      m = m >> 1;
      if (m==0) {
	 m = 0x80000000;
      }
   }
}



/*
 * Clip a pixel span to the current buffer/window boundaries.
 * Return:  0 = all pixels clipped
 *          1 = at least one pixel is visible
 */
static GLuint clip_span( GLcontext *ctx,
                         GLint n, GLint x, GLint y, GLubyte mask[] )
{
   GLint i;

   /* Clip to top and bottom */
   if (y<0 || y>=ctx->Buffer->Height) {
      return 0;
   }

   /* Clip to left and right */
   if (x>=0 && x+n<=ctx->Buffer->Width) {
      /* no clipping needed */
      return 1;
   }
   else if (x+n<=0) {
      /* completely off left side */
      return 0;
   }
   else if (x>=ctx->Buffer->Width) {
      /* completely off right side */
      return 0;
   }
   else {
      /* clip-test each pixel, this could be done better */
      for (i=0;i<n;i++) {
         if (x+i<0 || x+i>=ctx->Buffer->Width) {
            mask[i] = 0;
         }
      }
      return 1;
   }
}



/*
 * Write a horizontal span of color index pixels to the frame buffer.
 * Stenciling, Depth-testing, etc. are done as needed.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         z - array of [n] z-values
 *         index - array of [n] color indexes
 *         primitive - either GL_POINT, GL_LINE, GL_POLYGON, or GL_BITMAP
 */
void gl_write_index_span( GLcontext *ctx,
                          GLuint n, GLint x, GLint y, GLdepth z[],
			  GLuint index[], GLenum primitive )
{
   GLubyte mask[MAX_WIDTH];
   GLuint index_save[MAX_WIDTH];

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((ctx->RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if (clip_span(ctx,n,x,y,mask)==0) {
	 return;
      }
   }

   /* Per-pixel fog */
   if (ctx->Fog.Enabled
       && (ctx->Hint.Fog==GL_NICEST || primitive==GL_BITMAP)) {
      gl_fog_index_pixels( ctx, n, z, index );
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if (gl_scissor_span( ctx, n, x, y, mask )==0) {
	 return;
      }
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (gl_stencil_span( ctx, n, x, y, mask )==0) {
	 return;
      }
      /* depth buffering w/ stencil */
      gl_depth_stencil_span( ctx, n, x, y, z, mask );
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      if ((*ctx->Driver.DepthTestSpan)( ctx, n, x, y, z, mask )==0)  return;
   }

   if (ctx->RasterMask & NO_DRAW_BIT) {
      /* write no pixels */
      return;
   }

   if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
      /* Save a copy of the indexes since LogicOp and IndexMask
       * may change them
       */
      MEMCPY( index_save, index, n * sizeof(GLuint) );
   }

   if (ctx->Color.SWLogicOpEnabled) {
      gl_logicop_ci_span( ctx, n, x, y, index, mask );
   }
   if (ctx->Color.SWmasking) {
      gl_mask_index_span( ctx, n, x, y, index );
   }

   /* write pixels */
   (*ctx->Driver.WriteIndexSpan)( ctx, n, x, y, index, mask );


   if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
      /*** Also draw to back buffer ***/
      (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
      MEMCPY( index, index_save, n * sizeof(GLuint) );
      if (ctx->Color.SWLogicOpEnabled) {
         gl_logicop_ci_span( ctx, n, x, y, index, mask );
      }
      if (ctx->Color.SWmasking) {
         gl_mask_index_span( ctx, n, x, y, index );
      }
      (*ctx->Driver.WriteIndexSpan)( ctx, n, x, y, index, mask );
      (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
   }
}




void gl_write_monoindex_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y, GLdepth z[],
			      GLuint index, GLenum primitive )
{
    GLuint i;
    GLubyte mask[MAX_WIDTH];
    GLuint index_save[MAX_WIDTH];

    /* init mask to 1's (all pixels are to be written) */
    MEMSET(mask, 1, n);

    if ((ctx->RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP)
    {
        if (clip_span( ctx,n,x,y,mask)==0) {
            return;
        }
    }

    /* Do the scissor test */
    if (ctx->Scissor.Enabled)
    {
        if (gl_scissor_span( ctx, n, x, y, mask )==0) {
            return;
        }
    }

    /* Polygon Stippling */
    if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON)
    {
        stipple_polygon_span( ctx, n, x, y, mask );
    }

    if (ctx->Stencil.Enabled)
    {
        /* first stencil test */
        if (gl_stencil_span( ctx, n, x, y, mask )==0)
        {
            return;
        }
        /* depth buffering w/ stencil */
        gl_depth_stencil_span( ctx, n, x, y, z, mask );
    }
    else if (ctx->Depth.Test)
    {
        /* regular depth testing */
        if ((*ctx->Driver.DepthTestSpan)( ctx, n, x, y, z, mask )==0)
            return;
    }

    if (ctx->RasterMask & NO_DRAW_BIT)
    {
        /* write no pixels */
        return;
   }

    if ((ctx->Fog.Enabled && (ctx->Hint.Fog==GL_NICEST || primitive==GL_BITMAP))
            || ctx->Color.SWLogicOpEnabled || ctx->Color.SWmasking)
    {
        GLuint ispan[MAX_WIDTH];
        /* index may change, replicate single index into an array */
        for (i=0;i<n;i++)
        {
            ispan[i] = index;
        }

        if (ctx->Fog.Enabled
                && (ctx->Hint.Fog==GL_NICEST || primitive==GL_BITMAP))
        {
            gl_fog_index_pixels( ctx, n, z, ispan );
        }

        if (ctx->RasterMask & FRONT_AND_BACK_BIT)
        {
            MEMCPY( index_save, ispan, n * sizeof(GLuint) );
        }

        if (ctx->Color.SWLogicOpEnabled)
        {
            gl_logicop_ci_span( ctx, n, x, y, ispan, mask );
        }

        if (ctx->Color.SWmasking)
        {
            gl_mask_index_span( ctx, n, x, y, ispan );
        }

        (*ctx->Driver.WriteIndexSpan)( ctx, n, x, y, ispan, mask );

        if (ctx->RasterMask & FRONT_AND_BACK_BIT)
        {
             /*** Also draw to back buffer ***/
             (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
             for (i=0;i<n;i++)
             {
                ispan[i] = index;
             }

             if (ctx->Color.SWLogicOpEnabled) {
                 gl_logicop_ci_span( ctx, n, x, y, ispan, mask );
             }
             if (ctx->Color.SWmasking) {
                 gl_mask_index_span( ctx, n, x, y, ispan );
             }
             (*ctx->Driver.WriteIndexSpan)( ctx, n, x, y, ispan, mask );
             (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
        }
    }
    else
    {
        (*ctx->Driver.WriteMonoindexSpan)( ctx, n, x, y, mask );

        if (ctx->RasterMask & FRONT_AND_BACK_BIT)
        {
            /*** Also draw to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            (*ctx->Driver.WriteMonoindexSpan)( ctx, n, x, y, mask );
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
        }
    }
}



void gl_write_color_span( GLcontext *ctx,
                          GLuint n, GLint x, GLint y, GLdepth z[],
			  GLubyte r[], GLubyte g[],
			  GLubyte b[], GLubyte a[],
			  GLenum primitive )
{
   GLubyte mask[MAX_WIDTH];
   GLboolean write_all = GL_TRUE;
   GLubyte rtmp[MAX_WIDTH], gtmp[MAX_WIDTH], btmp[MAX_WIDTH], atmp[MAX_WIDTH];
   GLubyte *red, *green, *blue, *alpha;

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((ctx->RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if (clip_span( ctx,n,x,y,mask)==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   if ((primitive==GL_BITMAP && ctx->MutablePixels)
       || (ctx->RasterMask & FRONT_AND_BACK_BIT)) {
      /* must make a copy of the colors since they may be modified */
      MEMCPY( rtmp, r, n * sizeof(GLubyte) );
      MEMCPY( gtmp, g, n * sizeof(GLubyte) );
      MEMCPY( btmp, b, n * sizeof(GLubyte) );
      MEMCPY( atmp, a, n * sizeof(GLubyte) );
      red = rtmp;
      green = gtmp;
      blue = btmp;
      alpha = atmp;
   }
   else {
      red   = r;
      green = g;
      blue  = b;
      alpha = a;
   }

   /* Per-pixel fog */
   if (ctx->Fog.Enabled && (ctx->Hint.Fog==GL_NICEST || primitive==GL_BITMAP
                            || ctx->Texture.Enabled)) {
      gl_fog_color_pixels( ctx, n, z, red, green, blue, alpha );
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if (gl_scissor_span( ctx, n, x, y, mask )==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
      write_all = GL_FALSE;
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      if (gl_alpha_test( ctx, n, alpha, mask )==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (gl_stencil_span( ctx, n, x, y, mask )==0) {
	 return;
      }
      /* depth buffering w/ stencil */
      gl_depth_stencil_span( ctx, n, x, y, z, mask );
      write_all = GL_FALSE;
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      GLuint m = (*ctx->Driver.DepthTestSpan)( ctx, n, x, y, z, mask );
      if (m==0) {
         return;
      }
      if (m<n) {
         write_all = GL_FALSE;
      }
   }

   if (ctx->RasterMask & NO_DRAW_BIT) {
      /* write no pixels */
      return;
   }

   /* logic op or blending */
   if (ctx->Color.SWLogicOpEnabled) {
      gl_logicop_rgba_span( ctx, n, x, y, red, green, blue, alpha, mask );
   }
   else if (ctx->Color.BlendEnabled) {
      gl_blend_span( ctx, n, x, y, red, green, blue, alpha, mask );
   }

   /* Color component masking */
   if (ctx->Color.SWmasking) {
      gl_mask_color_span( ctx, n, x, y, red, green, blue, alpha );
   }

   /* write pixels */
   (*ctx->Driver.WriteColorSpan)( ctx, n, x, y, red, green, blue, alpha,
                                  write_all ? NULL : mask );
   if (ctx->RasterMask & ALPHABUF_BIT) {
      gl_write_alpha_span( ctx, n, x, y, alpha, write_all ? NULL : mask );
   }

   if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
      /*** Also render to back buffer ***/
      MEMCPY( rtmp, r, n * sizeof(GLubyte) );
      MEMCPY( gtmp, g, n * sizeof(GLubyte) );
      MEMCPY( btmp, b, n * sizeof(GLubyte) );
      MEMCPY( atmp, a, n * sizeof(GLubyte) );
      (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
      if (ctx->Color.SWLogicOpEnabled) {
         gl_logicop_rgba_span( ctx, n, x, y, red, green, blue, alpha, mask );
      }
      else  if (ctx->Color.BlendEnabled) {
         gl_blend_span( ctx, n, x, y, red, green, blue, alpha, mask );
      }
      if (ctx->Color.SWmasking) {
         gl_mask_color_span( ctx, n, x, y, red, green, blue, alpha );
      }
      (*ctx->Driver.WriteColorSpan)( ctx, n, x, y, red, green, blue, alpha,
                              write_all ? NULL : mask );
      if (ctx->RasterMask & ALPHABUF_BIT) {
         ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
         gl_write_alpha_span( ctx, n, x, y, alpha, write_all ? NULL : mask );
         ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
      }
      (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
   }

}



/*
 * Write a horizontal span of color pixels to the frame buffer.
 * The color is initially constant for the whole span.
 * Alpha-testing, stenciling, depth-testing, and blending are done as needed.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         z - array of [n] z-values
 *         r, g, b, a - the color of the pixels
 *         primitive - either GL_POINT, GL_LINE, GL_POLYGON or GL_BITMAP.
 */
void gl_write_monocolor_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y, GLdepth z[],
			      GLint r, GLint g, GLint b, GLint a,
                              GLenum primitive )
{
   GLuint i;
   GLubyte mask[MAX_WIDTH];
   GLboolean write_all = GL_TRUE;
   GLubyte red[MAX_WIDTH], green[MAX_WIDTH], blue[MAX_WIDTH], alpha[MAX_WIDTH];

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((ctx->RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if (clip_span( ctx,n,x,y,mask)==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if (gl_scissor_span( ctx, n, x, y, mask )==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
      write_all = GL_FALSE;
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      GLubyte alpha[MAX_WIDTH];
      for (i=0;i<n;i++) {
         alpha[i] = a;
      }
      if (gl_alpha_test( ctx, n, alpha, mask )==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (gl_stencil_span( ctx, n, x, y, mask )==0) {
	 return;
      }
      /* depth buffering w/ stencil */
      gl_depth_stencil_span( ctx, n, x, y, z, mask );
      write_all = GL_FALSE;
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      GLuint m = (*ctx->Driver.DepthTestSpan)( ctx, n, x, y, z, mask );
      if (m==0) {
         return;
      }
      if (m<n) {
         write_all = GL_FALSE;
      }
   }

   if (ctx->RasterMask & NO_DRAW_BIT) {
      /* write no pixels */
      return;
   }

   if (ctx->Color.BlendEnabled || ctx->Color.SWLogicOpEnabled
       || ctx->Color.SWmasking) {
      /* assign same color to each pixel */
      for (i=0;i<n;i++) {
	 if (mask[i]) {
	    red[i]   = r;
	    green[i] = g;
	    blue[i]  = b;
	    alpha[i] = a;
	 }
      }

      if (ctx->Color.SWLogicOpEnabled) {
         gl_logicop_rgba_span( ctx, n, x, y, red, green, blue, alpha, mask );
      }
      else if (ctx->Color.BlendEnabled) {
         gl_blend_span( ctx, n, x, y, red, green, blue, alpha, mask );
      }

      /* Color component masking */
      if (ctx->Color.SWmasking) {
         gl_mask_color_span( ctx, n, x, y, red, green, blue, alpha );
      }

      /* write pixels */
      (*ctx->Driver.WriteColorSpan)( ctx, n, x, y, red, green, blue, alpha,
                                     write_all ? NULL : mask );
      if (ctx->RasterMask & ALPHABUF_BIT) {
         gl_write_alpha_span( ctx, n, x, y, alpha, write_all ? NULL : mask );
      }

      if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
         /*** Also draw to back buffer ***/
         for (i=0;i<n;i++) {
            if (mask[i]) {
               red[i]   = r;
               green[i] = g;
               blue[i]  = b;
               alpha[i] = a;
            }
         }
         (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
         if (ctx->Color.SWLogicOpEnabled) {
            gl_logicop_rgba_span( ctx, n, x, y, red, green, blue, alpha, mask);
         }
         else if (ctx->Color.BlendEnabled) {
            gl_blend_span( ctx, n, x, y, red, green, blue, alpha, mask );
         }
         if (ctx->Color.SWmasking) {
            gl_mask_color_span( ctx, n, x, y, red, green, blue, alpha );
         }
         (*ctx->Driver.WriteColorSpan)( ctx, n, x, y, red, green, blue, alpha,
                                        write_all ? NULL : mask );
         (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
            gl_write_alpha_span( ctx, n, x, y, alpha,
                                 write_all ? NULL : mask );
            ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
         }
      }
   }
   else {
      (*ctx->Driver.WriteMonocolorSpan)( ctx, n, x, y, mask );
      if (ctx->RasterMask & ALPHABUF_BIT) {
         gl_write_mono_alpha_span( ctx, n, x, y, a, write_all ? NULL : mask );
      }
      if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
         /* Also draw to back buffer */
         (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
         (*ctx->Driver.WriteMonocolorSpan)( ctx, n, x, y, mask );
         (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
            gl_write_mono_alpha_span( ctx, n, x, y, a,
                                      write_all ? NULL : mask );
            ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
         }
      }
   }
}



/*
 * Write a horizontal span of textured pixels to the frame buffer.
 * The color of each pixel is different.
 * Alpha-testing, stenciling, depth-testing, and blending are done
 * as needed.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         z - array of [n] z-values
 *         s, t - array of (s,t) texture coordinates for each pixel
 *         lambda - array of texture lambda values
 *         red, green, blue, alpha - array of [n] color components
 *         primitive - either GL_POINT, GL_LINE, GL_POLYGON or GL_BITMAP.
 */
void gl_write_texture_span( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, GLdepth z[],
			    GLfloat s[], GLfloat t[], GLfloat u[],
                            GLfloat lambda[],
			    GLubyte r[], GLubyte g[],
			    GLubyte b[], GLubyte a[],
			    GLenum primitive )
{
   GLubyte mask[MAX_WIDTH];
   GLboolean write_all = GL_TRUE;
   GLubyte rtmp[MAX_WIDTH], gtmp[MAX_WIDTH], btmp[MAX_WIDTH], atmp[MAX_WIDTH];
   GLubyte *red, *green, *blue, *alpha;

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((ctx->RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if (clip_span( ctx,n,x,y,mask)==0) {
	 return;
      }
      write_all = GL_FALSE;
   }


   if (primitive==GL_BITMAP || (ctx->RasterMask & FRONT_AND_BACK_BIT)) {
      /* must make a copy of the colors since they may be modified */
      MEMCPY( rtmp, r, n * sizeof(GLubyte) );
      MEMCPY( gtmp, g, n * sizeof(GLubyte) );
      MEMCPY( btmp, b, n * sizeof(GLubyte) );
      MEMCPY( atmp, a, n * sizeof(GLubyte) );
      red = rtmp;
      green = gtmp;
      blue = btmp;
      alpha = atmp;
   }
   else {
      red   = r;
      green = g;
      blue  = b;
      alpha = a;
   }

   /* Texture */
   ASSERT(ctx->Texture.Enabled);
   gl_texture_pixels( ctx, n, s, t, u, lambda, red, green, blue, alpha );

   /* Per-pixel fog */
   if (ctx->Fog.Enabled && (ctx->Hint.Fog==GL_NICEST || primitive==GL_BITMAP
                            || ctx->Texture.Enabled)) {
      gl_fog_color_pixels( ctx, n, z, red, green, blue, alpha );
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if (gl_scissor_span( ctx, n, x, y, mask )==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
      write_all = GL_FALSE;
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      if (gl_alpha_test( ctx, n, alpha, mask )==0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (gl_stencil_span( ctx, n, x, y, mask )==0) {
	 return;
      }
      /* depth buffering w/ stencil */
      gl_depth_stencil_span( ctx, n, x, y, z, mask );
      write_all = GL_FALSE;
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      GLuint m = (*ctx->Driver.DepthTestSpan)( ctx, n, x, y, z, mask );
      if (m==0) {
         return;
      }
      if (m<n) {
         write_all = GL_FALSE;
      }
   }

   if (ctx->RasterMask & NO_DRAW_BIT) {
      /* write no pixels */
      return;
   }

   /* blending */
   if (ctx->Color.SWLogicOpEnabled) {
      gl_logicop_rgba_span( ctx, n, x, y, red, green, blue, alpha, mask );
   }
   else  if (ctx->Color.BlendEnabled) {
      gl_blend_span( ctx, n, x, y, red, green, blue, alpha, mask );
   }

   if (ctx->Color.SWmasking) {
      gl_mask_color_span( ctx, n, x, y, red, green, blue, alpha );
   }

   /* write pixels */
   (*ctx->Driver.WriteColorSpan)( ctx, n, x, y, red, green, blue, alpha,
                                  write_all ? NULL : mask );
   if (ctx->RasterMask & ALPHABUF_BIT) {
      gl_write_alpha_span( ctx, n, x, y, alpha, write_all ? NULL : mask );
   }

   if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
      /* Also draw to back buffer */
      MEMCPY( rtmp, r, n * sizeof(GLubyte) );
      MEMCPY( gtmp, g, n * sizeof(GLubyte) );
      MEMCPY( btmp, b, n * sizeof(GLubyte) );
      MEMCPY( atmp, a, n * sizeof(GLubyte) );
      (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
      if (ctx->Color.SWLogicOpEnabled) {
         gl_logicop_rgba_span( ctx, n, x, y, red, green, blue, alpha, mask );
      }
      else if (ctx->Color.BlendEnabled) {
         gl_blend_span( ctx, n, x, y, red, green, blue, alpha, mask );
      }
      if (ctx->Color.SWmasking) {
         gl_mask_color_span( ctx, n, x, y, red, green, blue, alpha );
      }
      (*ctx->Driver.WriteColorSpan)( ctx, n, x, y, red, green, blue, alpha,
                                     write_all ? NULL : mask );
      (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
      if (ctx->RasterMask & ALPHABUF_BIT) {
         ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
         gl_write_alpha_span( ctx, n, x, y, alpha, write_all ? NULL : mask );
         ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
      }
   }
}



/*
 * Read RGBA pixels from frame buffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 */
void gl_read_color_span( GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
			 GLubyte red[], GLubyte green[],
			 GLubyte blue[], GLubyte alpha[] )
{
   register GLuint i;

   if (y<0 || y>=ctx->Buffer->Height || x>=ctx->Buffer->Width) {
      /* completely above, below, or right */
      for (i=0;i<n;i++) {
	 red[i] = green[i] = blue[i] = alpha[i] = 0;
      }
   }
   else {
      if (x>=0 && x+n<=ctx->Buffer->Width) {
	 /* OK */
	 (*ctx->Driver.ReadColorSpan)( ctx, n, x, y, red, green, blue, alpha );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_read_alpha_span( ctx, n, x, y, alpha );
         }
      }
      else {
	 i = 0;
	 if (x<0) {
	    while (x<0 && n>0) {
	       red[i] = green[i] =  blue[i] = alpha[i] = 0;
	       x++;
	       n--;
	       i++;
	    }
	 }
	 n = MIN2( n, ctx->Buffer->Width - x );
	 (*ctx->Driver.ReadColorSpan)( ctx, n, x, y, red+i, green+i, blue+i, alpha+i);
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_read_alpha_span( ctx, n, x, y, alpha+i );
         }
      }
   }
}




/*
 * Read CI pixels from frame buffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 */
void gl_read_index_span( GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint indx[] )
{
   register GLuint i;

   if (y<0 || y>=ctx->Buffer->Height || x>=ctx->Buffer->Width) {
      /* completely above, below, or right */
      for (i=0;i<n;i++) {
	 indx[i] = 0;
      }
   }
   else {
      if (x>=0 && x+n<=ctx->Buffer->Width) {
	 /* OK */
	 (*ctx->Driver.ReadIndexSpan)( ctx, n, x, y, indx );
      }
      else {
	 i = 0;
	 if (x<0) {
	    while (x<0 && n>0) {
	       indx[i] = 0;
	       x++;
	       n--;
	       i++;
	    }
	 }
	 n = MIN2( n, ctx->Buffer->Width - x );
	 (*ctx->Driver.ReadIndexSpan)( ctx, n, x, y, indx+i );
      }
   }
}


