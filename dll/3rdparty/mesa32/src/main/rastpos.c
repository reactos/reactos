/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
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


/**
 * \file rastpos.c
 * Raster position operations.
 */

#include "glheader.h"
#include "context.h"
#include "feedback.h"
#include "macros.h"
#include "rastpos.h"
#include "state.h"


/**
 * Helper function for all the RasterPos functions.
 */
static void
rasterpos(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat p[4];

   p[0] = x;
   p[1] = y;
   p[2] = z;
   p[3] = w;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   FLUSH_CURRENT(ctx, 0);

   if (ctx->NewState)
      _mesa_update_state( ctx );

   ctx->Driver.RasterPos(ctx, p);
}


void GLAPIENTRY
_mesa_RasterPos2d(GLdouble x, GLdouble y)
{
   rasterpos((GLfloat)x, (GLfloat)y, (GLfloat)0.0, (GLfloat)1.0);
}

void GLAPIENTRY
_mesa_RasterPos2f(GLfloat x, GLfloat y)
{
   rasterpos(x, y, 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos2i(GLint x, GLint y)
{
   rasterpos((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos2s(GLshort x, GLshort y)
{
   rasterpos(x, y, 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   rasterpos(x, y, z, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3i(GLint x, GLint y, GLint z)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   rasterpos(x, y, z, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   rasterpos(x, y, z, w);
}

void GLAPIENTRY
_mesa_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   rasterpos(x, y, z, w);
}

void GLAPIENTRY
_mesa_RasterPos2dv(const GLdouble *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos2fv(const GLfloat *v)
{
   rasterpos(v[0], v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos2iv(const GLint *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos2sv(const GLshort *v)
{
   rasterpos(v[0], v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3dv(const GLdouble *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3fv(const GLfloat *v)
{
   rasterpos(v[0], v[1], v[2], 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3iv(const GLint *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos3sv(const GLshort *v)
{
   rasterpos(v[0], v[1], v[2], 1.0F);
}

void GLAPIENTRY
_mesa_RasterPos4dv(const GLdouble *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_RasterPos4fv(const GLfloat *v)
{
   rasterpos(v[0], v[1], v[2], v[3]);
}

void GLAPIENTRY
_mesa_RasterPos4iv(const GLint *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_RasterPos4sv(const GLshort *v)
{
   rasterpos(v[0], v[1], v[2], v[3]);
}


/**********************************************************************/
/***           GL_ARB_window_pos / GL_MESA_window_pos               ***/
/**********************************************************************/

#if FEATURE_drawpix
/**
 * All glWindowPosMESA and glWindowPosARB commands call this function to
 * update the current raster position.
 */
static void
window_pos3f(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat z2;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   FLUSH_CURRENT(ctx, 0);

   z2 = CLAMP(z, 0.0F, 1.0F) * (ctx->Viewport.Far - ctx->Viewport.Near)
      + ctx->Viewport.Near;

   /* set raster position */
   ctx->Current.RasterPos[0] = x;
   ctx->Current.RasterPos[1] = y;
   ctx->Current.RasterPos[2] = z2;
   ctx->Current.RasterPos[3] = 1.0F;

   ctx->Current.RasterPosValid = GL_TRUE;

   if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
      ctx->Current.RasterDistance = ctx->Current.Attrib[VERT_ATTRIB_FOG][0];
   else
      ctx->Current.RasterDistance = 0.0;

   /* raster color = current color or index */
   if (ctx->Visual.rgbMode) {
      ctx->Current.RasterColor[0]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][0], 0.0F, 1.0F);
      ctx->Current.RasterColor[1]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][1], 0.0F, 1.0F);
      ctx->Current.RasterColor[2]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][2], 0.0F, 1.0F);
      ctx->Current.RasterColor[3]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3], 0.0F, 1.0F);
      ctx->Current.RasterSecondaryColor[0]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][0], 0.0F, 1.0F);
      ctx->Current.RasterSecondaryColor[1]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][1], 0.0F, 1.0F);
      ctx->Current.RasterSecondaryColor[2]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][2], 0.0F, 1.0F);
      ctx->Current.RasterSecondaryColor[3]
         = CLAMP(ctx->Current.Attrib[VERT_ATTRIB_COLOR1][3], 0.0F, 1.0F);
   }
   else {
      ctx->Current.RasterIndex
         = ctx->Current.Attrib[VERT_ATTRIB_COLOR_INDEX][0];
   }

   /* raster texcoord = current texcoord */
   {
      GLuint texSet;
      for (texSet = 0; texSet < ctx->Const.MaxTextureCoordUnits; texSet++) {
         COPY_4FV( ctx->Current.RasterTexCoords[texSet],
                  ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texSet] );
      }
   }

   if (ctx->RenderMode==GL_SELECT) {
      _mesa_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }
}


/* This is just to support the GL_MESA_window_pos version */
static void
window_pos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   window_pos3f(x, y, z);
   ctx->Current.RasterPos[3] = w;
}


void GLAPIENTRY
_mesa_WindowPos2dMESA(GLdouble x, GLdouble y)
{
   window_pos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos2fMESA(GLfloat x, GLfloat y)
{
   window_pos4f(x, y, 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos2iMESA(GLint x, GLint y)
{
   window_pos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos2sMESA(GLshort x, GLshort y)
{
   window_pos4f(x, y, 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
   window_pos4f(x, y, z, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos3iMESA(GLint x, GLint y, GLint z)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   window_pos4f(x, y, z, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_WindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   window_pos4f(x, y, z, w);
}

void GLAPIENTRY
_mesa_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
{
   window_pos4f(x, y, z, w);
}

void GLAPIENTRY
_mesa_WindowPos2dvMESA(const GLdouble *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos2fvMESA(const GLfloat *v)
{
   window_pos4f(v[0], v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos2ivMESA(const GLint *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos2svMESA(const GLshort *v)
{
   window_pos4f(v[0], v[1], 0.0F, 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos3dvMESA(const GLdouble *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos3fvMESA(const GLfloat *v)
{
   window_pos4f(v[0], v[1], v[2], 1.0);
}

void GLAPIENTRY
_mesa_WindowPos3ivMESA(const GLint *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos3svMESA(const GLshort *v)
{
   window_pos4f(v[0], v[1], v[2], 1.0F);
}

void GLAPIENTRY
_mesa_WindowPos4dvMESA(const GLdouble *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 
			 (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_WindowPos4fvMESA(const GLfloat *v)
{
   window_pos4f(v[0], v[1], v[2], v[3]);
}

void GLAPIENTRY
_mesa_WindowPos4ivMESA(const GLint *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 
			 (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_WindowPos4svMESA(const GLshort *v)
{
   window_pos4f(v[0], v[1], v[2], v[3]);
}

#endif

#if 0

/*
 * OpenGL implementation of glWindowPos*MESA()
 */
void glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GLfloat fx, fy;

   /* Push current matrix mode and viewport attributes */
   glPushAttrib( GL_TRANSFORM_BIT | GL_VIEWPORT_BIT );

   /* Setup projection parameters */
   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
   glLoadIdentity();

   glDepthRange( z, z );
   glViewport( (int) x - 1, (int) y - 1, 2, 2 );

   /* set the raster (window) position */
   fx = x - (int) x;
   fy = y - (int) y;
   glRasterPos4f( fx, fy, 0.0, w );

   /* restore matrices, viewport and matrix mode */
   glPopMatrix();
   glMatrixMode( GL_PROJECTION );
   glPopMatrix();

   glPopAttrib();
}

#endif


/**********************************************************************/
/** \name Initialization                                              */
/**********************************************************************/
/*@{*/

/**
 * Initialize the context current raster position information.
 *
 * \param ctx GL context.
 *
 * Initialize the current raster position information in
 * __GLcontextRec::Current, and adds the extension entry points to the
 * dispatcher.
 */
void _mesa_init_rastpos( GLcontext * ctx )
{
   int i;

   ASSIGN_4V( ctx->Current.RasterPos, 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterDistance = 0.0;
   ASSIGN_4V( ctx->Current.RasterColor, 1.0, 1.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.RasterSecondaryColor, 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterIndex = 1.0;
   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++)
      ASSIGN_4V( ctx->Current.RasterTexCoords[i], 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterPosValid = GL_TRUE;
}

/*@}*/
