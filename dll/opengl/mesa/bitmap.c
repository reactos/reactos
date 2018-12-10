/* $Id: bitmap.c,v 1.9 1998/02/03 23:45:02 brianp Exp $ */

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
 * $Log: bitmap.c,v $
 * Revision 1.9  1998/02/03 23:45:02  brianp
 * added casts to prevent warnings with Amiga StormC compiler
 *
 * Revision 1.8  1997/10/02 03:06:42  brianp
 * added #include <assert.h>
 *
 * Revision 1.7  1997/09/27 00:15:39  brianp
 * changed parameters to gl_unpack_image()
 *
 * Revision 1.6  1997/07/24 01:24:45  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.5  1997/06/20 02:18:09  brianp
 * replaced Current.IntColor with Current.ByteColor
 *
 * Revision 1.4  1997/05/28 03:23:48  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1996/11/06 04:23:18  brianp
 * replaced 0 with GL_COLOR_INDEX in gl_unpack_bitmap()
 *
 * Revision 1.2  1996/09/15 14:18:10  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"
#include "context.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "pb.h"
#include "types.h"
#endif



/*
 * Unpack a bitmap image
 */
struct gl_image *gl_unpack_bitmap( GLcontext* ctx,
                                   GLsizei width, GLsizei height,
                                   const GLubyte *bitmap )
{
   return gl_unpack_image( ctx, width, height,
                           GL_COLOR_INDEX, GL_BITMAP, bitmap );
}




/*
 * Do actual rendering of a bitmap.
 */
void gl_render_bitmap( GLcontext* ctx,
                       GLsizei width, GLsizei height,
                       GLfloat xorig, GLfloat yorig,
                       GLfloat xmove, GLfloat ymove,
                       const struct gl_image *bitmap )
{
   struct pixel_buffer *PB = ctx->PB;
   GLint bx, by;      /* bitmap position */
   GLint px, py, pz;  /* pixel position */
   GLubyte *ptr;

   assert(bitmap);
   assert(bitmap->Type == GL_BITMAP);
   assert(bitmap->Format == GL_COLOR_INDEX);

   if (ctx->NewState) {
      gl_update_state(ctx);
      PB_INIT( PB, GL_BITMAP );
   }

   if (ctx->Visual->RGBAflag) {
      GLint r, g, b, a;
      r = (GLint) (ctx->Current.RasterColor[0] * ctx->Visual->RedScale);
      g = (GLint) (ctx->Current.RasterColor[1] * ctx->Visual->GreenScale);
      b = (GLint) (ctx->Current.RasterColor[2] * ctx->Visual->BlueScale);
      a = (GLint) (ctx->Current.RasterColor[3] * ctx->Visual->AlphaScale);
      PB_SET_COLOR( ctx, PB, r, g, b, a );
   }
   else {
      PB_SET_INDEX( ctx, PB, ctx->Current.RasterIndex );
   }

   px = (GLint) ( (ctx->Current.RasterPos[0] - xorig) + 0.0F );
   py = (GLint) ( (ctx->Current.RasterPos[1] - yorig) + 0.0F );
   pz = (GLint) ( ctx->Current.RasterPos[2] * DEPTH_SCALE );
   ptr = (GLubyte *) bitmap->Data;

   for (by=0;by<height;by++) {
      GLubyte bitmask;

      /* do a row */
      bitmask = 128;
      for (bx=0;bx<width;bx++) {
         if (*ptr&bitmask) {
            PB_WRITE_PIXEL( PB, px+bx, py+by, pz );
         }
         bitmask = bitmask >> 1;
         if (bitmask==0) {
            ptr++;
            bitmask = 128;
         }
      }

      PB_CHECK_FLUSH( ctx, PB )

      /* get ready for next row */
      if (bitmask!=128)  ptr++;
   }

   gl_flush_pb(ctx);
}




/*
 * Execute a glBitmap command:
 *   1. check for errors
 *   2. feedback/render/select
 *   3. advance raster position
 */
void gl_Bitmap( GLcontext* ctx,
                GLsizei width, GLsizei height,
	        GLfloat xorig, GLfloat yorig,
	        GLfloat xmove, GLfloat ymove,
                const struct gl_image *bitmap )
{
   if (width<0 || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glBitmap" );
      return;
   }
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBitmap" );
      return;
   }
   if (ctx->Current.RasterPosValid==GL_FALSE) {
      /* do nothing */
      return;
   }

   if (ctx->RenderMode==GL_RENDER) {
      GLboolean completed = GL_FALSE;
      if (ctx->Driver.Bitmap) {
         /* let device driver try to render the bitmap */
         completed = (*ctx->Driver.Bitmap)( ctx, width, height, xorig, yorig,
                                            xmove, ymove, bitmap );
      }
      if (!completed) {
         /* use generic function */
         gl_render_bitmap( ctx, width, height, xorig, yorig,
                           xmove, ymove, bitmap );
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
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
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_BITMAP_TOKEN );
      /* TODO: Verify XYZW values are correct: */
      gl_feedback_vertex( ctx, ctx->Current.RasterPos[0] - xorig,
			  ctx->Current.RasterPos[1] - yorig,
			  ctx->Current.RasterPos[2],
			  ctx->Current.RasterPos[3],
			  color, ctx->Current.Index, texcoord );
   }
   else if (ctx->RenderMode==GL_SELECT) {
      /* Bitmaps don't generate selection hits.  See appendix B of 1.1 spec. */
   }

   /* update raster position */
   ctx->Current.RasterPos[0] += xmove;
   ctx->Current.RasterPos[1] += ymove;
}


