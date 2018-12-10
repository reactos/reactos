/* $Id: masking.c,v 1.5 1997/07/24 01:23:16 brianp Exp $ */

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
 * $Log: masking.c,v $
 * Revision 1.5  1997/07/24 01:23:16  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/05/28 03:25:43  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.3  1996/11/06 04:19:40  brianp
 * now call gl_read_color/index_span() instead of device driver function
 *
 * Revision 1.2  1996/10/25 00:07:46  brianp
 * changed MAX_WIDTH to PB_SIZE in gl_mask_index_pixels()
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * Implement the effect of glColorMask and glIndexMask in software.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <string.h>
#include "alphabuf.h"
#include "context.h"
#include "macros.h"
#include "masking.h"
#include "pb.h"
#include "span.h"
#include "types.h"
#endif



void gl_IndexMask( GLcontext *ctx, GLuint mask )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glIndexMask" );
      return;
   }
   ctx->Color.IndexMask = mask;
   ctx->NewState |= NEW_RASTER_OPS;
}



void gl_ColorMask( GLcontext *ctx, GLboolean red, GLboolean green,
                   GLboolean blue, GLboolean alpha )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glColorMask" );
      return;
   }
   ctx->Color.ColorMask = (red << 3) | (green << 2) | (blue << 1) | alpha;
   ctx->NewState |= NEW_RASTER_OPS;
}




/*
 * Apply glColorMask to a span of RGBA pixels.
 */
void gl_mask_color_span( GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         GLubyte red[], GLubyte green[],
                         GLubyte blue[], GLubyte alpha[] )
{
   GLubyte r[MAX_WIDTH], g[MAX_WIDTH], b[MAX_WIDTH], a[MAX_WIDTH];

   gl_read_color_span( ctx, n, x, y, r, g, b, a );

   if ((ctx->Color.ColorMask & 8) == 0) {
      /* replace source reds with frame buffer reds */
      MEMCPY( red, r, n );
   }
   if ((ctx->Color.ColorMask & 4) == 0) {
      /* replace source greens with frame buffer greens */
      MEMCPY( green, g, n );
   }
   if ((ctx->Color.ColorMask & 2) == 0) {
      /* replace source blues with frame buffer blues */
      MEMCPY( blue, b, n );
   }
   if ((ctx->Color.ColorMask & 1) == 0) {
      /* replace source alphas with frame buffer alphas */
      MEMCPY( alpha, a, n );
   }
}



/*
 * Apply glColorMask to an array of RGBA pixels.
 */
void gl_mask_color_pixels( GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte red[], GLubyte green[],
                           GLubyte blue[], GLubyte alpha[],
                           const GLubyte mask[] )
{
   GLubyte r[PB_SIZE], g[PB_SIZE], b[PB_SIZE], a[PB_SIZE];

   (*ctx->Driver.ReadColorPixels)( ctx, n, x, y, r, g, b, a, mask );
   if (ctx->RasterMask & ALPHABUF_BIT) {
      gl_read_alpha_pixels( ctx, n, x, y, a, mask );
   }

   if ((ctx->Color.ColorMask & 8) == 0) {
      /* replace source reds with frame buffer reds */
      MEMCPY( red, r, n );
   }
   if ((ctx->Color.ColorMask & 4) == 0) {
      /* replace source greens with frame buffer greens */
      MEMCPY( green, g, n );
   }
   if ((ctx->Color.ColorMask & 2) == 0) {
      /* replace source blues with frame buffer blues */
      MEMCPY( blue, b, n );
   }
   if ((ctx->Color.ColorMask & 1) == 0) {
      /* replace source alphas with frame buffer alphas */
      MEMCPY( alpha, a, n );
   }
}



/*
 * Apply glIndexMask to a span of CI pixels.
 */
void gl_mask_index_span( GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint index[] )
{
   GLuint i;
   GLuint fbindexes[MAX_WIDTH];
   GLuint msrc, mdest;

   gl_read_index_span( ctx, n, x, y, fbindexes );

   msrc = ctx->Color.IndexMask;
   mdest = ~msrc;

   for (i=0;i<n;i++) {
      index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
   }
}



/*
 * Apply glIndexMask to an array of CI pixels.
 */
void gl_mask_index_pixels( GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint index[], const GLubyte mask[] )
{
   GLuint i;
   GLuint fbindexes[PB_SIZE];
   GLuint msrc, mdest;

   (*ctx->Driver.ReadIndexPixels)( ctx, n, x, y, fbindexes, mask );

   msrc = ctx->Color.IndexMask;
   mdest = ~msrc;

   for (i=0;i<n;i++) {
      index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
   }
}

