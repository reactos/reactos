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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "fog.h"
#include "mtypes.h"



void GLAPIENTRY
_mesa_Fogf(GLenum pname, GLfloat param)
{
   _mesa_Fogfv(pname, &param);
}


void GLAPIENTRY
_mesa_Fogi(GLenum pname, GLint param )
{
   GLfloat fparam = (GLfloat) param;
   _mesa_Fogfv(pname, &fparam);
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
         ;
   }
   _mesa_Fogfv(pname, p);
}


#define UPDATE_FOG_SCALE(ctx) do {\
      if (ctx->Fog.End == ctx->Fog.Start)\
         ctx->Fog._Scale = 1.0f;\
      else\
         ctx->Fog._Scale = 1.0f / (ctx->Fog.End - ctx->Fog.Start);\
   } while(0)


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
         UPDATE_FOG_SCALE(ctx);
         break;
      case GL_FOG_END:
         if (ctx->Fog.End == *params)
            return;
         FLUSH_VERTICES(ctx, _NEW_FOG);
         ctx->Fog.End = *params;
         UPDATE_FOG_SCALE(ctx);
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
	 ctx->Fog.Color[0] = CLAMP(params[0], 0.0F, 1.0F);
	 ctx->Fog.Color[1] = CLAMP(params[1], 0.0F, 1.0F);
	 ctx->Fog.Color[2] = CLAMP(params[2], 0.0F, 1.0F);
	 ctx->Fog.Color[3] = CLAMP(params[3], 0.0F, 1.0F);
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

void _mesa_init_fog( GLcontext * ctx )
{
   /* Fog group */
   ctx->Fog.Enabled = GL_FALSE;
   ctx->Fog.Mode = GL_EXP;
   ASSIGN_4V( ctx->Fog.Color, 0.0, 0.0, 0.0, 0.0 );
   ctx->Fog.Index = 0.0;
   ctx->Fog.Density = 1.0;
   ctx->Fog.Start = 0.0;
   ctx->Fog.End = 1.0;
   ctx->Fog.ColorSumEnabled = GL_FALSE;
   ctx->Fog.FogCoordinateSource = GL_FRAGMENT_DEPTH_EXT;
   ctx->Fog._Scale = 1.0f;
}
