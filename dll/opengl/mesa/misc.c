/* $Id: misc.c,v 1.23 1997/12/06 18:07:13 brianp Exp $ */

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
 * $Log: misc.c,v $
 * Revision 1.23  1997/12/06 18:07:13  brianp
 * changed version to 2.6
 *
 * Revision 1.22  1997/12/05 04:38:55  brianp
 * added ClearColorAndDepth() function pointer (David Bucciarelli)
 *
 * Revision 1.21  1997/10/29 01:29:09  brianp
 * added GL_EXT_point_parameters extension from Daniel Barrero
 *
 * Revision 1.20  1997/10/16 02:32:19  brianp
 * added GL_EXT_shared_texture_palette extension
 *
 * Revision 1.19  1997/10/14 00:17:48  brianp
 * added 3DFX_set_global_palette to extension string for 3Dfx
 *
 * Revision 1.18  1997/09/27 00:13:44  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.17  1997/08/13 01:26:40  brianp
 * changed version string to 2.4
 *
 * Revision 1.16  1997/07/24 01:23:16  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.15  1997/06/20 02:21:36  brianp
 * don't clear buffers if RenderMode != GL_RENDER
 *
 * Revision 1.14  1997/05/28 03:25:43  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.13  1997/04/12 12:31:18  brianp
 * removed gl_Rectf()
 *
 * Revision 1.12  1997/03/21 01:58:54  brianp
 * now call Driver.RendererString() in gl_GetString()
 *
 * Revision 1.11  1997/02/10 20:40:51  brianp
 * added GL_MESA_resize_buffers to extensions string
 *
 * Revision 1.10  1997/02/09 18:44:35  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.9  1997/01/08 20:55:02  brianp
 * added GL_EXT_texture_object
 *
 * Revision 1.8  1996/11/05 01:41:45  brianp
 * fixed potential scissor/clear color buffer bug
 *
 * Revision 1.7  1996/10/30 03:14:02  brianp
 * incremented version to 2.1
 *
 * Revision 1.6  1996/10/11 03:42:17  brianp
 * added GL_EXT_polygon_offset to extensions string
 *
 * Revision 1.5  1996/10/02 02:51:44  brianp
 * created clear_color_buffers() which handles draw mode GL_FRONT_AND_BACK
 *
 * Revision 1.4  1996/09/25 03:22:14  brianp
 * glDrawBuffer(GL_NONE) works now
 *
 * Revision 1.3  1996/09/24 00:16:10  brianp
 * set NewState flag in glRead/DrawBuffer() and glHint()
 * fixed display list bug in gl_Hint()
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
#include <stdlib.h>
#include <string.h>
#include "accum.h"
#include "alphabuf.h"
#include "context.h"
#include "depth.h"
#include "macros.h"
#include "masking.h"
#include "misc.h"
#include "stencil.h"
#include "types.h"
#endif



void gl_ClearIndex( GLcontext *ctx, GLfloat c )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClearIndex" );
      return;
   }
   ctx->Color.ClearIndex = (GLuint) c;
   if (!ctx->Visual->RGBAflag) {
      /* it's OK to call glClearIndex in RGBA mode but it should be a NOP */
      (*ctx->Driver.ClearIndex)( ctx, ctx->Color.ClearIndex );
   }
}



void gl_ClearColor( GLcontext *ctx, GLclampf red, GLclampf green,
                    GLclampf blue, GLclampf alpha )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClearColor" );
      return;
   }

   ctx->Color.ClearColor[0] = CLAMP( red,   0.0F, 1.0F );
   ctx->Color.ClearColor[1] = CLAMP( green, 0.0F, 1.0F );
   ctx->Color.ClearColor[2] = CLAMP( blue,  0.0F, 1.0F );
   ctx->Color.ClearColor[3] = CLAMP( alpha, 0.0F, 1.0F );

   if (ctx->Visual->RGBAflag) {
      GLubyte r = (GLint) (ctx->Color.ClearColor[0] * ctx->Visual->RedScale);
      GLubyte g = (GLint) (ctx->Color.ClearColor[1] * ctx->Visual->GreenScale);
      GLubyte b = (GLint) (ctx->Color.ClearColor[2] * ctx->Visual->BlueScale);
      GLubyte a = (GLint) (ctx->Color.ClearColor[3] * ctx->Visual->AlphaScale);
      (*ctx->Driver.ClearColor)( ctx, r, g, b, a );
   }
}




/*
 * Clear the color buffer when glColorMask or glIndexMask is in effect.
 */
static void clear_color_buffer_with_masking( GLcontext *ctx )
{
   GLint x, y, height, width;

   /* Compute region to clear */
   if (ctx->Scissor.Enabled) {
      x = ctx->Buffer->Xmin;
      y = ctx->Buffer->Ymin;
      height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
      width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
   }
   else {
      x = 0;
      y = 0;
      height = ctx->Buffer->Height;
      width  = ctx->Buffer->Width;
   }

   if (ctx->Visual->RGBAflag) {
      /* RGBA mode */
      GLubyte red[MAX_WIDTH], green[MAX_WIDTH];
      GLubyte blue[MAX_WIDTH], alpha[MAX_WIDTH];
      GLubyte r = ctx->Color.ClearColor[0] * ctx->Visual->RedScale;
      GLubyte g = ctx->Color.ClearColor[1] * ctx->Visual->GreenScale;
      GLubyte b = ctx->Color.ClearColor[2] * ctx->Visual->BlueScale;
      GLubyte a = ctx->Color.ClearColor[3] * ctx->Visual->AlphaScale;
      GLint i;
      for (i=0;i<height;i++,y++) {
         MEMSET( red,   (int) r, width );
         MEMSET( green, (int) g, width );
         MEMSET( blue,  (int) b, width );
         MEMSET( alpha, (int) a, width );
         gl_mask_color_span( ctx, width, x, y, red, green, blue, alpha );
         (*ctx->Driver.WriteColorSpan)( ctx,
                                 width, x, y, red, green, blue, alpha, NULL );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_write_alpha_span( ctx, width, x, y, alpha, NULL );
         }
      }
   }
   else {
      /* Color index mode */
      GLuint indx[MAX_WIDTH];
      GLubyte mask[MAX_WIDTH];
      GLint i, j;
      MEMSET( mask, 1, width );
      for (i=0;i<height;i++,y++) {
         for (j=0;j<width;j++) {
            indx[j] = ctx->Color.ClearIndex;
         }
         gl_mask_index_span( ctx, width, x, y, indx );
         (*ctx->Driver.WriteIndexSpan)( ctx, width, x, y, indx, mask );
      }
   }
}



/*
 * Clear the front and/or back color buffers.  Also clear the alpha
 * buffer(s) if present.
 */
static void clear_color_buffers( GLcontext *ctx )
{
   if (ctx->Color.SWmasking) {
      clear_color_buffer_with_masking( ctx );
   }
   else {
      GLint x = ctx->Buffer->Xmin;
      GLint y = ctx->Buffer->Ymin;
      GLint height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
      GLint width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
      (*ctx->Driver.Clear)( ctx, !ctx->Scissor.Enabled,
                            x, y, width, height );
      if (ctx->RasterMask & ALPHABUF_BIT) {
         /* front and/or back alpha buffers will be cleared here */
         gl_clear_alpha_buffers( ctx );
      }
   }

   if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
      /*** Also clear the back buffer ***/
      (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
      if (ctx->Color.SWmasking) {
         clear_color_buffer_with_masking( ctx );
      }
      else {
         GLint x = ctx->Buffer->Xmin;
         GLint y = ctx->Buffer->Ymin;
         GLint height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
         GLint width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
         (*ctx->Driver.Clear)( ctx, !ctx->Scissor.Enabled,
                               x, y, width, height );
      }
      (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
   }
}



void gl_Clear( GLcontext *ctx, GLbitfield mask )
{
#ifdef PROFILE
   GLdouble t0 = gl_time();
#endif

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glClear" );
      return;
   }

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NewState) {
         gl_update_state( ctx );
      }

      /* See if we can call device driver function to clear both the
       * color and depth buffers.
       */
      if (ctx->Driver.ClearColorAndDepth &&
          (mask & GL_COLOR_BUFFER_BIT) && (mask & GL_DEPTH_BUFFER_BIT)) {
         GLint x = ctx->Buffer->Xmin;
         GLint y = ctx->Buffer->Ymin;
         GLint height = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
         GLint width  = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
         (*ctx->Driver.ClearColorAndDepth)( ctx, !ctx->Scissor.Enabled,
                                            x, y, width, height );
         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also clear the back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            (*ctx->Driver.ClearColorAndDepth)( ctx, !ctx->Scissor.Enabled,
                                               x, y, width, height );
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
         }
      }
      else {
         /* normal procedure for clearing buffers */
         if (mask & GL_COLOR_BUFFER_BIT)  clear_color_buffers( ctx );
         if (mask & GL_DEPTH_BUFFER_BIT)  (*ctx->Driver.ClearDepthBuffer)(ctx);
         if (mask & GL_ACCUM_BUFFER_BIT)   gl_clear_accum_buffer( ctx );
         if (mask & GL_STENCIL_BUFFER_BIT) gl_clear_stencil_buffer( ctx );
      }

#ifdef PROFILE
      ctx->ClearTime += gl_time() - t0;
      ctx->ClearCount++;
#endif
   }
}



const GLubyte *gl_GetString( GLcontext *ctx, GLenum name )
{
   static char renderer[1000];
   static char *vendor = "Brian Paul & ReactOS Developers";
   static char *version = "1.1";
   static char *extensions = "GL_EXT_paletted_texture GL_EXT_bgra GL_WIN_swap_hint";

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetString" );
      return (GLubyte *) 0;
   }

   switch (name) {
      case GL_VENDOR:
         return (GLubyte *) vendor;
      case GL_RENDERER:
         strcpy(renderer, "Mesa");
         if (ctx->Driver.RendererString) {
            strcat(renderer, " ");
            strcat(renderer, (*ctx->Driver.RendererString)());
         }
         return (GLubyte *) renderer;
      case GL_VERSION:
         return (GLubyte *) version;
      case GL_EXTENSIONS:
         return (GLubyte *) extensions;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetString" );
         return (GLubyte *) 0;
   }
}



void gl_Finish( GLcontext *ctx )
{
   /* Don't compile into display list */
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glFinish" );
      return;
   }
   if (ctx->Driver.Finish) {
      (*ctx->Driver.Finish)( ctx );
   }
}



void gl_Flush( GLcontext *ctx )
{
   /* Don't compile into display list */
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glFlush" );
      return;
   }
   if (ctx->Driver.Flush) {
      (*ctx->Driver.Flush)( ctx );
   }
}



void gl_Hint( GLcontext *ctx, GLenum target, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glHint" );
      return;
   }
   if (mode!=GL_DONT_CARE && mode!=GL_FASTEST && mode!=GL_NICEST) {
      gl_error( ctx, GL_INVALID_ENUM, "glHint(mode)" );
      return;
   }
   switch (target) {
      case GL_FOG_HINT:
         ctx->Hint.Fog = mode;
         break;
      case GL_LINE_SMOOTH_HINT:
         ctx->Hint.LineSmooth = mode;
         break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
         ctx->Hint.PerspectiveCorrection = mode;
         break;
      case GL_POINT_SMOOTH_HINT:
         ctx->Hint.PointSmooth = mode;
         break;
      case GL_POLYGON_SMOOTH_HINT:
         ctx->Hint.PolygonSmooth = mode;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glHint(target)" );
   }
   ctx->NewState |= NEW_ALL;   /* just to be safe */
}



void gl_DrawBuffer( GLcontext *ctx, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
      return;
   }
   switch (mode) {
      case GL_FRONT:
      case GL_FRONT_LEFT:
      case GL_FRONT_AND_BACK:
         if ( (*ctx->Driver.SetBuffer)( ctx, GL_FRONT ) == GL_FALSE ) {
            gl_error( ctx, GL_INVALID_ENUM, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawBuffer = mode;
         ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
	 ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_BACK:
      case GL_BACK_LEFT:
         if ( (*ctx->Driver.SetBuffer)( ctx, GL_BACK ) == GL_FALSE) {
            gl_error( ctx, GL_INVALID_ENUM, "glDrawBuffer" );
            return;
         }
         ctx->Color.DrawBuffer = mode;
         ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
	 ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_NONE:
         ctx->Color.DrawBuffer = mode;
         ctx->Buffer->Alpha = NULL;
         ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_LEFT:
      case GL_RIGHT:
      case GL_AUX0:
         gl_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer" );
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glDrawBuffer" );
   }
}



void gl_ReadBuffer( GLcontext *ctx, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
      return;
   }
   switch (mode) {
      case GL_FRONT:
      case GL_FRONT_LEFT:
         if ( (*ctx->Driver.SetBuffer)( ctx, GL_FRONT ) == GL_FALSE) {
            gl_error( ctx, GL_INVALID_ENUM, "glReadBuffer" );
            return;
         }
         ctx->Pixel.ReadBuffer = mode;
         ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
         ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_BACK:
      case GL_BACK_LEFT:
         if ( (*ctx->Driver.SetBuffer)( ctx, GL_BACK ) == GL_FALSE) {
            gl_error( ctx, GL_INVALID_ENUM, "glReadBuffer" );
            return;
         }
         ctx->Pixel.ReadBuffer = mode;
         ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
         ctx->NewState |= NEW_RASTER_OPS;
         break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_LEFT:
      case GL_RIGHT:
      case GL_AUX0:
         gl_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glReadBuffer" );
   }

   /* Remember, the draw buffer is the default state */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DrawBuffer );
}
