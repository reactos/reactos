/* $Id: rastpos.c,v 1.5 1997/07/24 01:23:44 brianp Exp $ */

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
 * $Log: rastpos.c,v $
 * Revision 1.5  1997/07/24 01:23:44  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.4  1997/06/20 02:25:54  brianp
 * replaced Current.IntColor with Current.ByteColor
 *
 * Revision 1.3  1997/05/28 03:26:18  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.2  1997/05/01 01:39:59  brianp
 * replaced sqrt() with GL_SQRT()
 *
 * Revision 1.1  1997/04/01 04:17:13  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "clip.h"
#include "feedback.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "shade.h"
#include "types.h"
#include "xform.h"
#endif


/*
 * Caller:  context->API.RasterPos4f
 */
void gl_RasterPos4f( GLcontext *ctx,
                     GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GLfloat v[4], eye[4], clip[4], ndc[3], d;

   ASSIGN_4V( v, x, y, z, w );

   if (ctx->NewModelViewMatrix) {
      gl_analyze_modelview_matrix(ctx);
   }
   if (ctx->NewProjectionMatrix) {
      gl_analyze_projection_matrix(ctx);
   }
   if (ctx->NewTextureMatrix) {
      gl_analyze_texture_matrix(ctx);
   }

   /* transform v to eye coords:  eye = ModelView * v */
   TRANSFORM_POINT( eye, ctx->ModelViewMatrix, v );

   /* raster color */
   if (ctx->Light.Enabled) {
      GLfloat eyenorm[3];
      TRANSFORM_NORMAL( eyenorm[0], eyenorm[1], eyenorm[2], ctx->Current.Normal,
                        ctx->ModelViewInv );
      if (ctx->Visual->RGBAflag) {
         GLubyte color[4];
         gl_color_shade_vertices( ctx, 0, 1, &eye, &eyenorm, &color );
         ctx->Current.RasterColor[0] = color[0] * ctx->Visual->InvRedScale;
         ctx->Current.RasterColor[1] = color[1] * ctx->Visual->InvGreenScale;
         ctx->Current.RasterColor[2] = color[2] * ctx->Visual->InvBlueScale;
         ctx->Current.RasterColor[3] = color[3] * ctx->Visual->InvAlphaScale;
      }
      else {
	 gl_index_shade_vertices( ctx, 0, 1, &eye, &eyenorm,
                                  &ctx->Current.RasterIndex );
      }
   }
   else {
      /* use current color or index */
      if (ctx->Visual->RGBAflag) {
         GLfloat *rc = ctx->Current.RasterColor;
         rc[0] = ctx->Current.ByteColor[0] * ctx->Visual->InvRedScale;
         rc[1] = ctx->Current.ByteColor[1] * ctx->Visual->InvGreenScale;
         rc[2] = ctx->Current.ByteColor[2] * ctx->Visual->InvBlueScale;
         rc[3] = ctx->Current.ByteColor[3] * ctx->Visual->InvAlphaScale;
      }
      else {
	 ctx->Current.RasterIndex = ctx->Current.Index;
      }
   }

   /* clip to user clipping planes */
   if (gl_userclip_point(ctx, eye)==0) {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* compute raster distance */
   ctx->Current.RasterDistance =
                      GL_SQRT( eye[0]*eye[0] + eye[1]*eye[1] + eye[2]*eye[2] );

   /* apply projection matrix:  clip = Proj * eye */
   TRANSFORM_POINT( clip, ctx->ProjectionMatrix, eye );

   /* clip to view volume */
   if (gl_viewclip_point( clip )==0) {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* ndc = clip / W */
   ASSERT( clip[3]!=0.0 );
   d = 1.0F / clip[3];
   ndc[0] = clip[0] * d;
   ndc[1] = clip[1] * d;
   ndc[2] = clip[2] * d;

   ctx->Current.RasterPos[0] = ndc[0] * ctx->Viewport.Sx + ctx->Viewport.Tx;
   ctx->Current.RasterPos[1] = ndc[1] * ctx->Viewport.Sy + ctx->Viewport.Ty;
   ctx->Current.RasterPos[2] = (ndc[2] * ctx->Viewport.Sz + ctx->Viewport.Tz)
                               / DEPTH_SCALE;
   ctx->Current.RasterPos[3] = clip[3];
   ctx->Current.RasterPosValid = GL_TRUE;

   /* FOG??? */

   if (ctx->Texture.Enabled) {
      COPY_4V( ctx->Current.RasterTexCoord, ctx->Current.TexCoord );
   }

   if (ctx->RenderMode==GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }

}



/*
 * This is a MESA extension function.  Pretty much just like glRasterPos
 * except we don't apply the modelview or projection matrices; specify a
 * window coordinate directly.
 * Caller:  context->API.WindowPos4fMESA pointer.
 */
void gl_windowpos( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   /* set raster position */
   ctx->Current.RasterPos[0] = x;
   ctx->Current.RasterPos[1] = y;
   ctx->Current.RasterPos[2] = CLAMP( z, 0.0F, 1.0F );
   ctx->Current.RasterPos[3] = w;

   ctx->Current.RasterPosValid = GL_TRUE;

   /* raster color */
   if (ctx->Light.Enabled) {
      GLfloat eye[4];
      GLfloat eyenorm[3];
      COPY_4V( eye, ctx->Current.RasterPos );
      if (ctx->NewModelViewMatrix) {
	 gl_analyze_modelview_matrix(ctx);
      }
      TRANSFORM_NORMAL( eyenorm[0], eyenorm[1], eyenorm[2],
                        ctx->Current.Normal,
                        ctx->ModelViewInv );
      if (ctx->Visual->RGBAflag) {
         GLubyte color[4];
         gl_color_shade_vertices( ctx, 0, 1, &eye, &eyenorm, &color );
         ASSIGN_4V( ctx->Current.RasterColor, 
                    (GLfloat) color[0] * ctx->Visual->InvRedScale,
                    (GLfloat) color[1] * ctx->Visual->InvGreenScale,
                    (GLfloat) color[2] * ctx->Visual->InvBlueScale,
                    (GLfloat) color[3] * ctx->Visual->InvAlphaScale );
      }
      else {
	 gl_index_shade_vertices( ctx, 0, 1, &eye, &eyenorm,
                                  &ctx->Current.RasterIndex );
      }
   }
   else {
      /* use current color or index */
      if (ctx->Visual->RGBAflag) {
         ASSIGN_4V( ctx->Current.RasterColor,
                    ctx->Current.ByteColor[0] * ctx->Visual->InvRedScale,
                    ctx->Current.ByteColor[1] * ctx->Visual->InvGreenScale,
                    ctx->Current.ByteColor[2] * ctx->Visual->InvBlueScale,
                    ctx->Current.ByteColor[3] * ctx->Visual->InvAlphaScale );
      }
      else {
	 ctx->Current.RasterIndex = ctx->Current.Index;
      }
   }

   ctx->Current.RasterDistance = 0.0;

   if (ctx->Texture.Enabled) {
      COPY_4V( ctx->Current.RasterTexCoord, ctx->Current.TexCoord );
   }

   if (ctx->RenderMode==GL_SELECT) {
      gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }
}
