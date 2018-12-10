/* $Id: copypix.c,v 1.7 1998/02/03 23:45:02 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
 * Copyright (C) 1995-1996  Brian Paul
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
 * $Log: copypix.c,v $
 * Revision 1.7  1998/02/03 23:45:02  brianp
 * added casts to prevent warnings with Amiga StormC compiler
 *
 * Revision 1.6  1997/07/24 01:24:45  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.5  1997/06/20 02:20:04  brianp
 * replaced Current.IntColor with Current.ByteColor
 *
 * Revision 1.4  1997/05/28 03:23:48  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1996/09/15 14:18:10  brianp
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
#include <string.h>
#include "context.h"
#include "copypix.h"
#include "depth.h"
#include "feedback.h"
#include "dlist.h"
#include "macros.h"
#include "pixel.h"
#include "span.h"
#include "stencil.h"
#include "types.h"
#endif



static void copy_rgb_pixels( GLcontext* ctx,
                             GLint srcx, GLint srcy, GLint width, GLint height,
                             GLint destx, GLint desty )
{
   DEFARRAY( GLdepth, zspan, MAX_WIDTH );
   DEFARRAY( GLubyte, red, MAX_WIDTH );
   DEFARRAY( GLubyte, green, MAX_WIDTH );
   DEFARRAY( GLubyte, blue, MAX_WIDTH );
   DEFARRAY( GLubyte, alpha, MAX_WIDTH );
   GLboolean scale_or_bias, quick_draw, zoom;
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean setbuffer;

   if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
      zoom = GL_FALSE;
   }
   else {
      zoom = GL_TRUE;
   }

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   scale_or_bias = ctx->Pixel.RedScale!=1.0 || ctx->Pixel.RedBias!=0.0
                || ctx->Pixel.GreenScale!=1.0 || ctx->Pixel.GreenBias!=0.0
		|| ctx->Pixel.BlueScale!=1.0 || ctx->Pixel.BlueBias!=0.0
		|| ctx->Pixel.AlphaScale!=1.0 || ctx->Pixel.AlphaBias!=0.0;

   if (ctx->Depth.Test) {
      /* fill in array of z values */
      GLint z = (GLint) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   if (ctx->RasterMask==0 && !zoom
       && destx>=0 && destx+width<=ctx->Buffer->Width) {
      quick_draw = GL_TRUE;
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* If read and draw buffer are different we must do buffer switching */
   setbuffer = ctx->Pixel.ReadBuffer!=ctx->Color.DrawBuffer;

   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      /* read */

      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );
      }
      gl_read_color_span( ctx, width, srcx, sy, red, green, blue, alpha );

      if (scale_or_bias) {
         GLfloat rbias = ctx->Pixel.RedBias   * ctx->Visual->RedScale;
         GLfloat gbias = ctx->Pixel.GreenBias * ctx->Visual->GreenScale;
         GLfloat bbias = ctx->Pixel.BlueBias  * ctx->Visual->BlueScale;
         GLfloat abias = ctx->Pixel.AlphaBias * ctx->Visual->AlphaScale;
         GLint rmax = (GLint) ctx->Visual->RedScale;
         GLint gmax = (GLint) ctx->Visual->GreenScale;
         GLint bmax = (GLint) ctx->Visual->BlueScale;
         GLint amax = (GLint) ctx->Visual->AlphaScale;
         for (i=0;i<width;i++) {
            GLint r = red[i]   * ctx->Pixel.RedScale   + rbias;
            GLint g = green[i] * ctx->Pixel.GreenScale + gbias;
            GLint b = blue[i]  * ctx->Pixel.BlueScale  + bbias;
            GLint a = alpha[i] * ctx->Pixel.AlphaScale + abias;
            red[i]   = CLAMP( r, 0, rmax );
            green[i] = CLAMP( g, 0, gmax );
            blue[i]  = CLAMP( b, 0, bmax );
            alpha[i] = CLAMP( a, 0, amax );
         }
      }

      if (ctx->Pixel.MapColorFlag) {
         GLfloat r = (ctx->Pixel.MapRtoRsize-1) * ctx->Visual->InvRedScale;
         GLfloat g = (ctx->Pixel.MapGtoGsize-1) * ctx->Visual->InvGreenScale;
         GLfloat b = (ctx->Pixel.MapBtoBsize-1) * ctx->Visual->InvBlueScale;
         GLfloat a = (ctx->Pixel.MapAtoAsize-1) * ctx->Visual->InvAlphaScale;
         for (i=0;i<width;i++) {
            GLint ir = red[i] * r;
            GLint ig = green[i] * g;
            GLint ib = blue[i] * b;
            GLint ia = alpha[i] * a;
            red[i]   = (GLint) (ctx->Pixel.MapRtoR[ir]*ctx->Visual->RedScale);
            green[i] = (GLint) (ctx->Pixel.MapGtoG[ig]*ctx->Visual->GreenScale);
            blue[i]  = (GLint) (ctx->Pixel.MapBtoB[ib]*ctx->Visual->BlueScale);
            alpha[i] = (GLint) (ctx->Pixel.MapAtoA[ia]*ctx->Visual->AlphaScale);
         }
      }

      /* write */
      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
      }
      if (quick_draw && dy>=0 && dy<ctx->Buffer->Height) {
         (*ctx->Driver.WriteColorSpan)( ctx, width, destx, dy,
                                 red, green, blue, alpha, NULL);
      }
      else if (zoom) {
         gl_write_zoomed_color_span( ctx, width, destx, dy, zspan,
                                     red, green, blue, alpha, desty );
      }
      else {
         gl_write_color_span( ctx, width, destx, dy, zspan,
                              red, green, blue, alpha, GL_BITMAP );
      }
   }
   UNDEFARRAY( zspan );
   UNDEFARRAY( red );
   UNDEFARRAY( green );
   UNDEFARRAY( blue );
   UNDEFARRAY( alpha );
}



static void copy_ci_pixels( GLcontext* ctx,
                            GLint srcx, GLint srcy, GLint width, GLint height,
                            GLint destx, GLint desty )
{
   GLdepth zspan[MAX_WIDTH];
   GLuint indx[MAX_WIDTH];
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean setbuffer, zoom;

   if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
      zoom = GL_FALSE;
   }
   else {
      zoom = GL_TRUE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->Depth.Test) {
      /* fill in array of z values */
      GLint z = (GLint) (ctx->Current.RasterPos[2] * DEPTH_SCALE);
      for (i=0;i<width;i++) {
         zspan[i] = z;
      }
   }

   /* If read and draw buffer are different we must do buffer switching */
   setbuffer = ctx->Pixel.ReadBuffer!=ctx->Color.DrawBuffer;

   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      /* read */
      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.ReadBuffer );
      }
      gl_read_index_span( ctx, width, srcx, sy, indx );

      /* shift, offset */
      if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset) {
         if (ctx->Pixel.IndexShift<0) {
            for (i=0;i<width;i++) {
               indx[i] = (indx[i] >> -ctx->Pixel.IndexShift)
                         + ctx->Pixel.IndexOffset;
            }
         }
         else {
            for (i=0;i<width;i++) {
               indx[i] = (indx[i] << ctx->Pixel.IndexShift)
                         + ctx->Pixel.IndexOffset;
            }
         }
      }

      /* mapping */
      if (ctx->Pixel.MapColorFlag) {
         for (i=0;i<width;i++) {
            if (indx[i] < ctx->Pixel.MapItoIsize) {
               indx[i] = ctx->Pixel.MapItoI[ indx[i] ];
            }
         }
      }

      /* write */
      if (setbuffer) {
         (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
      }
      if (zoom) {
         gl_write_zoomed_index_span( ctx, width, destx, dy, zspan, indx, desty );
      }
      else {
         gl_write_index_span( ctx, width, destx, dy, zspan, indx, GL_BITMAP );
      }
   }
}



/*
 * TODO: Optimize!!!!
 */
static void copy_depth_pixels( GLcontext* ctx, GLint srcx, GLint srcy,
                               GLint width, GLint height,
                               GLint destx, GLint desty )
{
   GLfloat depth[MAX_WIDTH];
   GLdepth zspan[MAX_WIDTH];
   GLuint indx[MAX_WIDTH];
   GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
   GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean zoom;

   if (!ctx->Buffer->Depth) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
      zoom = GL_FALSE;
   }
   else {
      zoom = GL_TRUE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   /* setup colors or indexes */
   if (ctx->Visual->RGBAflag) {
      GLubyte r, g, b, a;
      r = ctx->Current.ByteColor[0];
      g = ctx->Current.ByteColor[1];
      b = ctx->Current.ByteColor[2];
      a = ctx->Current.ByteColor[3];
      MEMSET( red,   (int) r, width );
      MEMSET( green, (int) g, width );
      MEMSET( blue,  (int) b, width );
      MEMSET( alpha, (int) a, width );
   }
   else {
      for (i=0;i<width;i++) {
         indx[i] = ctx->Current.Index;
      }
   }

   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      /* read */
      (*ctx->Driver.ReadDepthSpanFloat)( ctx, width, srcx, sy, depth );
      /* scale, bias, clamp */
      for (i=0;i<width;i++) {
         GLfloat d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         zspan[i] = (GLint) (CLAMP( d, 0.0, 1.0 ) * DEPTH_SCALE);
      }
      /* write */
      if (ctx->Visual->RGBAflag) {
         if (zoom) {
            gl_write_zoomed_color_span( ctx, width, destx, dy, zspan,
                                        red, green, blue, alpha, desty );
         }
         else {
            gl_write_color_span( ctx, width, destx, dy, zspan,
                                 red, green, blue, alpha, GL_BITMAP );
         }
      }
      else {
         if (zoom) {
            gl_write_zoomed_index_span( ctx, width, destx, dy,
                                        zspan, indx, desty);
         }
         else {
            gl_write_index_span( ctx, width, destx, dy,
                                 zspan, indx, GL_BITMAP );
         }
      }
   }
}



static void copy_stencil_pixels( GLcontext* ctx, GLint srcx, GLint srcy,
                                 GLint width, GLint height,
                                 GLint destx, GLint desty )
{
   GLubyte stencil[MAX_WIDTH];
   GLint sy, dy, stepy;
   GLint i, j;
   GLboolean zoom;

   if (!ctx->Buffer->Stencil) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   if (ctx->Pixel.ZoomX==1.0F && ctx->Pixel.ZoomY==1.0F) {
      zoom = GL_FALSE;
   }
   else {
      zoom = GL_TRUE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   for (j=0; j<height; j++, sy+=stepy, dy+=stepy) {
      /* read */
      gl_read_stencil_span( ctx, width, srcx, sy, stencil );
      /* shift, offset */
      if (ctx->Pixel.IndexShift<0) {
         for (i=0;i<width;i++) {
            stencil[i] = (stencil[i] >> -ctx->Pixel.IndexShift)
                         + ctx->Pixel.IndexOffset;
         }
      }
      else {
         for (i=0;i<width;i++) {
            stencil[i] = (stencil[i] << ctx->Pixel.IndexShift)
			 + ctx->Pixel.IndexOffset;
         }
      }
      /* mapping */
      if (ctx->Pixel.MapStencilFlag) {
         for (i=0;i<width;i++) {
            if ((GLint) stencil[i] < ctx->Pixel.MapStoSsize) {
               stencil[i] = ctx->Pixel.MapStoS[ stencil[i] ];
            }
         }
      }
      /* write */
      if (zoom) {
         gl_write_zoomed_stencil_span( ctx, width, destx, dy, stencil, desty );
      }
      else {
         gl_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }
}




void gl_CopyPixels( GLcontext* ctx, GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		    GLenum type )
{
   GLint destx, desty;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyPixels" );
      return;
   }

   if (width<0 || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyPixels" );
      return;
   }

   if (ctx->NewState) {
      gl_update_state(ctx);
   }

   if (ctx->RenderMode==GL_RENDER) {
      /* Destination of copy: */
      if (!ctx->Current.RasterPosValid) {
	 return;
      }
      destx = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      desty = (GLint) (ctx->Current.RasterPos[1] + 0.5F);

      if (type==GL_COLOR && ctx->Visual->RGBAflag) {
         copy_rgb_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type==GL_COLOR && !ctx->Visual->RGBAflag) {
         copy_ci_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type==GL_DEPTH) {
         copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else if (type==GL_STENCIL) {
         copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else {
	 gl_error( ctx, GL_INVALID_ENUM, "glCopyPixels" );
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      GLfloat color[4];
      color[0] = ctx->Current.ByteColor[0] * ctx->Visual->InvRedScale;
      color[1] = ctx->Current.ByteColor[1] * ctx->Visual->InvGreenScale;
      color[2] = ctx->Current.ByteColor[2] * ctx->Visual->InvBlueScale;
      color[3] = ctx->Current.ByteColor[3] * ctx->Visual->InvAlphaScale;
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_COPY_PIXEL_TOKEN );
      gl_feedback_vertex( ctx, ctx->Current.RasterPos[0],
			  ctx->Current.RasterPos[1],
			  ctx->Current.RasterPos[2],
			  ctx->Current.RasterPos[3],
			  color, ctx->Current.Index,
			  ctx->Current.TexCoord );
   }
   else if (ctx->RenderMode==GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }

}


