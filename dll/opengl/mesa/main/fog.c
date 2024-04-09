/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <precomp.h>

void GLAPIENTRY
_mesa_Fogf(GLenum pname, GLfloat param)
{
   GLfloat fparam[4];
   fparam[0] = param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
   _mesa_Fogfv(pname, fparam);
}


void GLAPIENTRY
_mesa_Fogi(GLenum pname, GLint param )
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
   _mesa_Fogfv(pname, fparam);
}


void GLAPIENTRY
_mesa_Fogiv(GLenum pname, const GLint *params )
{
   GLfloat p[4];
   switch (pname) {
      case GL_FOG_MODE:
      case GL_FOG_DENSITY:
      case GL_FOG_START:
      case GL_FOG_END:
      case GL_FOG_INDEX:
      case GL_FOG_COORDINATE_SOURCE_EXT:
	 p[0] = (GLfloat) *params;
	 break;
      case GL_FOG_COLOR:
	 p[0] = INT_TO_FLOAT( params[0] );
	 p[1] = INT_TO_FLOAT( params[1] );
	 p[2] = INT_TO_FLOAT( params[2] );
	 p[3] = INT_TO_FLOAT( params[3] );
	 break;
      default:
         /* Error will be caught later in _mesa_Fogfv */
         ASSIGN_4V(p, 0.0F, 0.0F, 0.0F, 0.0F);
   }
   _mesa_Fogfv(pname, p);
}


/**
 * Update the gl_fog_attrib::_Scale field.
 */
static void
update_fog_scale(struct gl_context *ctx)
{
   if (ctx->Fog.End == ctx->Fog.Start)
      ctx->Fog._Scale = 1.0f;
   else
      ctx->Fog._Scale = 1.0f / (ctx->Fog.End - ctx->Fog.Start);
}


void GLAPIENTRY
_mesa_Fogfv( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLenum m;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_FOG_MODE:
         m = (GLenum) (GLint) *params;
	 switch (m) {
	 case GL_LINEAR:
	 case GL_EXP:
	 case GL_EXP2:
	    break;
	 default:
	    _mesa_error( ctx, GL_INVALID_ENUM, "glFog" );
            return;
	 }
	 if (ctx->Fog.Mode == m)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.Mode = m;
	 break;
      case GL_FOG_DENSITY:
	 if (*params<0.0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glFog" );
            return;
	 }
	 if (ctx->Fog.Density == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.Density = *params;
	 break;
      case GL_FOG_START:
         if (ctx->Fog.Start == *params)
            return;
         FLUSH_VERTICES(ctx, _NEW_FOG);
         ctx->Fog.Start = *params;
         update_fog_scale(ctx);
         break;
      case GL_FOG_END:
         if (ctx->Fog.End == *params)
            return;
         FLUSH_VERTICES(ctx, _NEW_FOG);
         ctx->Fog.End = *params;
         update_fog_scale(ctx);
         break;
      case GL_FOG_INDEX:
 	 if (ctx->Fog.Index == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
 	 ctx->Fog.Index = *params;
	 break;
      case GL_FOG_COLOR:
	 if (TEST_EQ_4V(ctx->Fog.Color, params))
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.Color[0] = params[0];
	 ctx->Fog.Color[1] = params[1];
	 ctx->Fog.Color[2] = params[2];
	 ctx->Fog.Color[3] = params[3];
         break;
      case GL_FOG_COORDINATE_SOURCE_EXT: {
	 GLenum p = (GLenum) (GLint) *params;
         if (!ctx->Extensions.EXT_fog_coord ||
             (p != GL_FOG_COORDINATE_EXT && p != GL_FRAGMENT_DEPTH_EXT)) {
	    _mesa_error(ctx, GL_INVALID_ENUM, "glFog");
	    return;
	 }
	 if (ctx->Fog.FogCoordinateSource == p)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.FogCoordinateSource = p;
	 break;
      }
      case GL_FOG_DISTANCE_MODE_NV: {
	 GLenum p = (GLenum) (GLint) *params;
         if (!ctx->Extensions.NV_fog_distance ||
             (p != GL_EYE_RADIAL_NV && p != GL_EYE_PLANE && p != GL_EYE_PLANE_ABSOLUTE_NV)) {
	    _mesa_error(ctx, GL_INVALID_ENUM, "glFog");
	    return;
	 }
	 if (ctx->Fog.FogDistanceMode == p)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_FOG);
	 ctx->Fog.FogDistanceMode = p;
	 break;
      }
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glFog" );
         return;
   }

   if (ctx->Driver.Fogfv) {
      (*ctx->Driver.Fogfv)( ctx, pname, params );
   }
}


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

void _mesa_init_fog( struct gl_context * ctx )
{
   /* Fog group */
   ctx->Fog.Enabled = GL_FALSE;
   ctx->Fog.Mode = GL_EXP;
   ASSIGN_4V( ctx->Fog.Color, 0.0, 0.0, 0.0, 0.0 );
   ctx->Fog.Index = 0.0;
   ctx->Fog.Density = 1.0;
   ctx->Fog.Start = 0.0;
   ctx->Fog.End = 1.0;
   ctx->Fog.FogCoordinateSource = GL_FRAGMENT_DEPTH_EXT;
   ctx->Fog._Scale = 1.0f;
   ctx->Fog.FogDistanceMode = GL_EYE_PLANE_ABSOLUTE_NV;
}
