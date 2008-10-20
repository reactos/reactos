/**
 * \file points.c
 * Point operations.
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
#include "context.h"
#include "macros.h"
#include "points.h"
#include "texstate.h"
#include "mtypes.h"


/**
 * Set current point size.
 * \param size  point diameter in pixels
 * \sa glPointSize().
 */
void GLAPIENTRY
_mesa_PointSize( GLfloat size )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size <= 0.0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPointSize" );
      return;
   }

   if (ctx->Point.Size == size)
      return;

   FLUSH_VERTICES(ctx, _NEW_POINT);
   ctx->Point.Size = size;

   if (ctx->Driver.PointSize)
      ctx->Driver.PointSize(ctx, size);
}


#if _HAVE_FULL_GL

/*
 * Added by GL_NV_point_sprite
 */
void GLAPIENTRY
_mesa_PointParameteriNV( GLenum pname, GLint param )
{
   const GLfloat value = (GLfloat) param;
   _mesa_PointParameterfvEXT(pname, &value);
}


/*
 * Added by GL_NV_point_sprite
 */
void GLAPIENTRY
_mesa_PointParameterivNV( GLenum pname, const GLint *params )
{
   GLfloat p[3];
   p[0] = (GLfloat) params[0];
   if (pname == GL_DISTANCE_ATTENUATION_EXT) {
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
   }
   _mesa_PointParameterfvEXT(pname, p);
}



/*
 * Same for both GL_EXT_point_parameters and GL_ARB_point_parameters.
 */
void GLAPIENTRY
_mesa_PointParameterfEXT( GLenum pname, GLfloat param)
{
   _mesa_PointParameterfvEXT(pname, &param);
}



/*
 * Same for both GL_EXT_point_parameters and GL_ARB_point_parameters.
 */
void GLAPIENTRY
_mesa_PointParameterfvEXT( GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_DISTANCE_ATTENUATION_EXT:
         if (ctx->Extensions.EXT_point_parameters) {
            if (TEST_EQ_3V(ctx->Point.Params, params))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_POINT);
            COPY_3V(ctx->Point.Params, params);
            ctx->Point._Attenuated = (ctx->Point.Params[0] != 1.0 ||
                                      ctx->Point.Params[1] != 0.0 ||
                                      ctx->Point.Params[2] != 0.0);

            if (ctx->Point._Attenuated)
               ctx->_TriangleCaps |= DD_POINT_ATTEN;
            else
               ctx->_TriangleCaps &= ~DD_POINT_ATTEN;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      case GL_POINT_SIZE_MIN_EXT:
         if (ctx->Extensions.EXT_point_parameters) {
            if (params[0] < 0.0F) {
               _mesa_error( ctx, GL_INVALID_VALUE,
                            "glPointParameterf[v]{EXT,ARB}(param)" );
               return;
            }
            if (ctx->Point.MinSize == params[0])
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.MinSize = params[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      case GL_POINT_SIZE_MAX_EXT:
         if (ctx->Extensions.EXT_point_parameters) {
            if (params[0] < 0.0F) {
               _mesa_error( ctx, GL_INVALID_VALUE,
                            "glPointParameterf[v]{EXT,ARB}(param)" );
               return;
            }
            if (ctx->Point.MaxSize == params[0])
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.MaxSize = params[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
         if (ctx->Extensions.EXT_point_parameters) {
            if (params[0] < 0.0F) {
               _mesa_error( ctx, GL_INVALID_VALUE,
                            "glPointParameterf[v]{EXT,ARB}(param)" );
               return;
            }
            if (ctx->Point.Threshold == params[0])
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.Threshold = params[0];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      case GL_POINT_SPRITE_R_MODE_NV:
         /* This is one area where ARB_point_sprite and NV_point_sprite
	  * differ.  In ARB_point_sprite the POINT_SPRITE_R_MODE is
	  * always ZERO.  NV_point_sprite adds the S and R modes.
	  */
         if (ctx->Extensions.NV_point_sprite) {
            GLenum value = (GLenum) params[0];
            if (value != GL_ZERO && value != GL_S && value != GL_R) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glPointParameterf[v]{EXT,ARB}(param)");
               return;
            }
            if (ctx->Point.SpriteRMode == value)
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.SpriteRMode = value;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      case GL_POINT_SPRITE_COORD_ORIGIN:
         if (ctx->Extensions.ARB_point_sprite) {
            GLenum value = (GLenum) params[0];
            if (value != GL_LOWER_LEFT && value != GL_UPPER_LEFT) {
               _mesa_error(ctx, GL_INVALID_VALUE,
                           "glPointParameterf[v]{EXT,ARB}(param)");
               return;
            }
            if (ctx->Point.SpriteOrigin == value)
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.SpriteOrigin = value;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glPointParameterf[v]{EXT,ARB}(pname)");
            return;
         }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM,
                      "glPointParameterf[v]{EXT,ARB}(pname)" );
         return;
   }

   if (ctx->Driver.PointParameterfv)
      (*ctx->Driver.PointParameterfv)(ctx, pname, params);
}
#endif



/**
 * Initialize the context point state.
 *
 * \param ctx GL context.
 *
 * Initializes __GLcontextRec::Point and point related constants in
 * __GLcontextRec::Const.
 */
void
_mesa_init_point(GLcontext *ctx)
{
   GLuint i;

   ctx->Point.SmoothFlag = GL_FALSE;
   ctx->Point.Size = 1.0;
   ctx->Point.Params[0] = 1.0;
   ctx->Point.Params[1] = 0.0;
   ctx->Point.Params[2] = 0.0;
   ctx->Point._Attenuated = GL_FALSE;
   ctx->Point.MinSize = 0.0;
   ctx->Point.MaxSize
      = MAX2(ctx->Const.MaxPointSize, ctx->Const.MaxPointSizeAA);
   ctx->Point.Threshold = 1.0;
   ctx->Point.PointSprite = GL_FALSE; /* GL_ARB/NV_point_sprite */
   ctx->Point.SpriteRMode = GL_ZERO; /* GL_NV_point_sprite (only!) */
   ctx->Point.SpriteOrigin = GL_UPPER_LEFT; /* GL_ARB_point_sprite */
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      ctx->Point.CoordReplace[i] = GL_FALSE; /* GL_ARB/NV_point_sprite */
   }
}
