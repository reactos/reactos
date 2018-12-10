/* $Id: fog.c,v 1.9 1997/07/24 01:25:01 brianp Exp $ */

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
 * $Log: fog.c,v $
 * Revision 1.9  1997/07/24 01:25:01  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.8  1997/06/20 04:14:47  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.7  1997/05/28 03:24:54  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.6  1997/05/22 03:03:47  brianp
 * don't apply fog to alpha values
 *
 * Revision 1.5  1997/04/20 20:28:49  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.4  1996/11/04 02:30:15  brianp
 * optimized gl_fog_color_pixels() and gl_fog_index_pixels()
 *
 * Revision 1.3  1996/09/27 01:26:52  brianp
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
#include <math.h>
#include <stdlib.h>
#include "context.h"
#include "fog.h"
#include "dlist.h"
#include "macros.h"
#include "types.h"
#endif



void gl_Fogfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   GLenum m;

   switch (pname) {
      case GL_FOG_MODE:
         m = (GLenum) (GLint) *params;
	 if (m==GL_LINEAR || m==GL_EXP || m==GL_EXP2) {
	    ctx->Fog.Mode = m;
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glFog" );
	 }
	 break;
      case GL_FOG_DENSITY:
	 if (*params<0.0) {
	    gl_error( ctx, GL_INVALID_VALUE, "glFog" );
	 }
	 else {
	    ctx->Fog.Density = *params;
	 }
	 break;
      case GL_FOG_START:
#ifndef GL_VERSION_1_1
         if (*params<0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glFog(GL_FOG_START)" );
            return;
         }
#endif
	 ctx->Fog.Start = *params;
	 break;
      case GL_FOG_END:
#ifndef GL_VERSION_1_1
         if (*params<0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glFog(GL_FOG_END)" );
            return;
         }
#endif
	 ctx->Fog.End = *params;
	 break;
      case GL_FOG_INDEX:
	 ctx->Fog.Index = *params;
	 break;
      case GL_FOG_COLOR:
	 ctx->Fog.Color[0] = params[0];
	 ctx->Fog.Color[1] = params[1];
	 ctx->Fog.Color[2] = params[2];
	 ctx->Fog.Color[3] = params[3];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glFog" );
   }
}




/*
 * Compute the fogged color for an array of vertices.
 * Input:  n - number of vertices
 *         v - array of vertices
 *         color - the original vertex colors
 * Output:  color - the fogged colors
 */
void gl_fog_color_vertices( GLcontext *ctx,
                            GLuint n, GLfloat v[][4], GLubyte color[][4] )
{
   GLuint i;
   GLfloat d;
   GLfloat fogr = ctx->Fog.Color[0] * ctx->Visual->RedScale;
   GLfloat fogg = ctx->Fog.Color[1] * ctx->Visual->GreenScale;
   GLfloat fogb = ctx->Fog.Color[2] * ctx->Visual->BlueScale;
   GLfloat end = ctx->Fog.End;

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
         for (i=0;i<n;i++) {
            GLfloat f = (end - ABSF(v[i][2])) * d;
            f = CLAMP( f, 0.0F, 1.0F );
            color[i][0] = f * color[i][0] + (1.0F-f) * fogr;
            color[i][1] = f * color[i][1] + (1.0F-f) * fogg;
            color[i][2] = f * color[i][2] + (1.0F-f) * fogb;
         }
	 break;
      case GL_EXP:
         d = -ctx->Fog.Density;
         for (i=0;i<n;i++) {
            GLfloat f = exp( d * ABSF(v[i][2]) );
            f = CLAMP( f, 0.0F, 1.0F );
            color[i][0] = f * color[i][0] + (1.0F-f) * fogr;
            color[i][1] = f * color[i][1] + (1.0F-f) * fogg;
            color[i][2] = f * color[i][2] + (1.0F-f) * fogb;
         }
	 break;
      case GL_EXP2:
         d = -(ctx->Fog.Density*ctx->Fog.Density);
         for (i=0;i<n;i++) {
            GLfloat z = ABSF(v[i][2]);
            GLfloat f = exp( d * z*z );
            f = CLAMP( f, 0.0F, 1.0F );
            color[i][0] = f * color[i][0] + (1.0F-f) * fogr;
            color[i][1] = f * color[i][1] + (1.0F-f) * fogg;
            color[i][2] = f * color[i][2] + (1.0F-f) * fogb;
         }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_color_vertices");
         return;
   }
}



/*
 * Compute the fogged color indexes for an array of vertices.
 * Input:  n - number of vertices
 *         v - array of vertices
 * In/Out: indx - array of vertex color indexes
 */
void gl_fog_index_vertices( GLcontext *ctx,
                            GLuint n, GLfloat v[][4], GLuint indx[] )
{
   /* NOTE: the extensive use of casts generates better/faster code for MIPS */
   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            GLfloat fogindex = ctx->Fog.Index;
            GLfloat fogend = ctx->Fog.End;
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat f = (fogend - ABSF(v[i][2])) * d;
               f = CLAMP( f, 0.0F, 1.0F );
               indx[i] = (GLint)
                         ((GLfloat) (GLint) indx[i] + (1.0F-f) * fogindex);
            }
         }
	 break;
      case GL_EXP:
         {
            GLfloat d = -ctx->Fog.Density;
            GLfloat fogindex = ctx->Fog.Index;
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat f = exp( d * ABSF(v[i][2]) );
               f = CLAMP( f, 0.0F, 1.0F );
               indx[i] = (GLint)
                         ((GLfloat) (GLint) indx[i] + (1.0F-f) * fogindex);
            }
         }
	 break;
      case GL_EXP2:
         {
            GLfloat d = -(ctx->Fog.Density*ctx->Fog.Density);
            GLfloat fogindex = ctx->Fog.Index;
            GLuint i;
            for (i=0;i<n;i++) {
               GLfloat z = ABSF(v[i][2]);
               GLfloat f = exp( -d * z*z );
               f = CLAMP( f, 0.0F, 1.0F );
               indx[i] = (GLint)
                         ((GLfloat) (GLint) indx[i] + (1.0F-f) * fogindex);
            }
         }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_index_vertices");
         return;
   }
}




/*
 * Apply fog to an array of RGBA pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         red, green, blue, alpha - pixel colors
 * Output:  red, green, blue, alpha - fogged pixel colors
 */
void gl_fog_color_pixels( GLcontext *ctx,
                          GLuint n, const GLdepth z[], GLubyte red[],
			  GLubyte green[], GLubyte blue[], GLubyte alpha[] )
{
   GLfloat c = ctx->ProjectionMatrix[10];
   GLfloat d = ctx->ProjectionMatrix[14];
   GLuint i;

   GLfloat fog_red   = ctx->Fog.Color[0] * ctx->Visual->RedScale;
   GLfloat fog_green = ctx->Fog.Color[1] * ctx->Visual->GreenScale;
   GLfloat fog_blue  = ctx->Fog.Color[2] * ctx->Visual->BlueScale;

   GLfloat tz = ctx->Viewport.Tz;
   GLfloat szInv = 1.0F / ctx->Viewport.Sz;

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat fogEnd = ctx->Fog.End;
            GLfloat fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f, g;
               if (eyez < 0.0)  eyez = -eyez;
               f = (fogEnd - eyez) * fogScale;
               f = CLAMP( f, 0.0F, 1.0F );
               g = 1.0F - f;
               red[i]   = (GLint) (f * (GLfloat) red[i]   + g * fog_red);
               green[i] = (GLint) (f * (GLfloat) green[i] + g * fog_green);
               blue[i]  = (GLint) (f * (GLfloat) blue[i]  + g * fog_blue);
            }
         }
	 break;
      case GL_EXP:
	 for (i=0;i<n;i++) {
	    GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
	    GLfloat eyez = -d / (c+ndcz);
            GLfloat f, g;
	    if (eyez < 0.0)  eyez = -eyez;
	    f = exp( -ctx->Fog.Density * eyez );
	    f = CLAMP( f, 0.0F, 1.0F );
            g = 1.0F - f;
            red[i]   = (GLint) (f * (GLfloat) red[i]   + g * fog_red);
            green[i] = (GLint) (f * (GLfloat) green[i] + g * fog_green);
            blue[i]  = (GLint) (f * (GLfloat) blue[i]  + g * fog_blue);
	 }
	 break;
      case GL_EXP2:
         {
            GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f, g;
               if (eyez < 0.0)  eyez = -eyez;
               f = exp( negDensitySquared * eyez*eyez );
               f = CLAMP( f, 0.0F, 1.0F );
               g = 1.0F - f;
               red[i]   = (GLint) (f * (GLfloat) red[i]   + g * fog_red);
               green[i] = (GLint) (f * (GLfloat) green[i] + g * fog_green);
               blue[i]  = (GLint) (f * (GLfloat) blue[i]  + g * fog_blue);
            }
         }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_color_pixels");
         return;
   }
}




/*
 * Apply fog to an array of color index pixels.
 * Input:  n - number of pixels
 *         z - array of integer depth values
 *         index - pixel color indexes
 * Output:  index - fogged pixel color indexes
 */
void gl_fog_index_pixels( GLcontext *ctx,
                          GLuint n, const GLdepth z[], GLuint index[] )
{
   GLfloat c = ctx->ProjectionMatrix[10];
   GLfloat d = ctx->ProjectionMatrix[14];
   GLuint i;

   GLfloat tz = ctx->Viewport.Tz;
   GLfloat szInv = 1.0F / ctx->Viewport.Sz;

   switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            GLfloat fogEnd = ctx->Fog.End;
            GLfloat fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f;
               if (eyez < 0.0)  eyez = -eyez;
               f = (fogEnd - eyez) * fogScale;
               f = CLAMP( f, 0.0F, 1.0F );
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
            }
	 }
	 break;
      case GL_EXP:
         for (i=0;i<n;i++) {
	    GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
	    GLfloat eyez = -d / (c+ndcz);
            GLfloat f;
	    if (eyez < 0.0)  eyez = -eyez;
	    f = exp( -ctx->Fog.Density * eyez );
	    f = CLAMP( f, 0.0F, 1.0F );
	    index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
	 }
	 break;
      case GL_EXP2:
         {
            GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            for (i=0;i<n;i++) {
               GLfloat ndcz = ((GLfloat) z[i] - tz) * szInv;
               GLfloat eyez = -d / (c+ndcz);
               GLfloat f;
               if (eyez < 0.0)  eyez = -eyez;
               f = exp( negDensitySquared * eyez*eyez );
               f = CLAMP( f, 0.0F, 1.0F );
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F-f) * ctx->Fog.Index);
            }
	 }
	 break;
      default:
         gl_problem(ctx, "Bad fog mode in gl_fog_index_pixels");
         return;
   }
}

