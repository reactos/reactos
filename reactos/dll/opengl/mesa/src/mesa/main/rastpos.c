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
#include "mfeatures.h"
#include "mtypes.h"
#include "rastpos.h"
#include "state.h"
#include "main/dispatch.h"


#if FEATURE_rastpos


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


static void GLAPIENTRY
_mesa_RasterPos2d(GLdouble x, GLdouble y)
{
   rasterpos((GLfloat)x, (GLfloat)y, (GLfloat)0.0, (GLfloat)1.0);
}

static void GLAPIENTRY
_mesa_RasterPos2f(GLfloat x, GLfloat y)
{
   rasterpos(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos2i(GLint x, GLint y)
{
   rasterpos((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos2s(GLshort x, GLshort y)
{
   rasterpos(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   rasterpos(x, y, z, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3i(GLint x, GLint y, GLint z)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   rasterpos(x, y, z, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
_mesa_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   rasterpos(x, y, z, w);
}

static void GLAPIENTRY
_mesa_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   rasterpos((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
_mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   rasterpos(x, y, z, w);
}

static void GLAPIENTRY
_mesa_RasterPos2dv(const GLdouble *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos2fv(const GLfloat *v)
{
   rasterpos(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos2iv(const GLint *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos2sv(const GLshort *v)
{
   rasterpos(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3dv(const GLdouble *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3fv(const GLfloat *v)
{
   rasterpos(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3iv(const GLint *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos3sv(const GLshort *v)
{
   rasterpos(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY
_mesa_RasterPos4dv(const GLdouble *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
_mesa_RasterPos4fv(const GLfloat *v)
{
   rasterpos(v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
_mesa_RasterPos4iv(const GLint *v)
{
   rasterpos((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
_mesa_RasterPos4sv(const GLshort *v)
{
   rasterpos(v[0], v[1], v[2], v[3]);
}


/**********************************************************************/
/***           GL_ARB_window_pos / GL_MESA_window_pos               ***/
/**********************************************************************/


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

   /* raster texcoord = current texcoord */
   {
      GLuint texSet;
      for (texSet = 0; texSet < ctx->Const.MaxTextureCoordUnits; texSet++) {
         assert(texSet < Elements(ctx->Current.RasterTexCoords));
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


static void GLAPIENTRY
_mesa_WindowPos2dMESA(GLdouble x, GLdouble y)
{
   window_pos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos2fMESA(GLfloat x, GLfloat y)
{
   window_pos4f(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos2iMESA(GLint x, GLint y)
{
   window_pos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos2sMESA(GLshort x, GLshort y)
{
   window_pos4f(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
   window_pos4f(x, y, z, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos3iMESA(GLint x, GLint y, GLint z)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   window_pos4f(x, y, z, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
_mesa_WindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   window_pos4f(x, y, z, w);
}

static void GLAPIENTRY
_mesa_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
{
   window_pos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
_mesa_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
{
   window_pos4f(x, y, z, w);
}

static void GLAPIENTRY
_mesa_WindowPos2dvMESA(const GLdouble *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos2fvMESA(const GLfloat *v)
{
   window_pos4f(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos2ivMESA(const GLint *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos2svMESA(const GLshort *v)
{
   window_pos4f(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos3dvMESA(const GLdouble *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos3fvMESA(const GLfloat *v)
{
   window_pos4f(v[0], v[1], v[2], 1.0);
}

static void GLAPIENTRY
_mesa_WindowPos3ivMESA(const GLint *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos3svMESA(const GLshort *v)
{
   window_pos4f(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY
_mesa_WindowPos4dvMESA(const GLdouble *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 
			 (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
_mesa_WindowPos4fvMESA(const GLfloat *v)
{
   window_pos4f(v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
_mesa_WindowPos4ivMESA(const GLint *v)
{
   window_pos4f((GLfloat) v[0], (GLfloat) v[1], 
			 (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
_mesa_WindowPos4svMESA(const GLshort *v)
{
   window_pos4f(v[0], v[1], v[2], v[3]);
}


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


void
_mesa_init_rastpos_dispatch(struct _glapi_table *disp)
{
   SET_RasterPos2f(disp, _mesa_RasterPos2f);
   SET_RasterPos2fv(disp, _mesa_RasterPos2fv);
   SET_RasterPos2i(disp, _mesa_RasterPos2i);
   SET_RasterPos2iv(disp, _mesa_RasterPos2iv);
   SET_RasterPos2d(disp, _mesa_RasterPos2d);
   SET_RasterPos2dv(disp, _mesa_RasterPos2dv);
   SET_RasterPos2s(disp, _mesa_RasterPos2s);
   SET_RasterPos2sv(disp, _mesa_RasterPos2sv);
   SET_RasterPos3d(disp, _mesa_RasterPos3d);
   SET_RasterPos3dv(disp, _mesa_RasterPos3dv);
   SET_RasterPos3f(disp, _mesa_RasterPos3f);
   SET_RasterPos3fv(disp, _mesa_RasterPos3fv);
   SET_RasterPos3i(disp, _mesa_RasterPos3i);
   SET_RasterPos3iv(disp, _mesa_RasterPos3iv);
   SET_RasterPos3s(disp, _mesa_RasterPos3s);
   SET_RasterPos3sv(disp, _mesa_RasterPos3sv);
   SET_RasterPos4d(disp, _mesa_RasterPos4d);
   SET_RasterPos4dv(disp, _mesa_RasterPos4dv);
   SET_RasterPos4f(disp, _mesa_RasterPos4f);
   SET_RasterPos4fv(disp, _mesa_RasterPos4fv);
   SET_RasterPos4i(disp, _mesa_RasterPos4i);
   SET_RasterPos4iv(disp, _mesa_RasterPos4iv);
   SET_RasterPos4s(disp, _mesa_RasterPos4s);
   SET_RasterPos4sv(disp, _mesa_RasterPos4sv);

   /* 197. GL_MESA_window_pos */
   SET_WindowPos2dMESA(disp, _mesa_WindowPos2dMESA);
   SET_WindowPos2dvMESA(disp, _mesa_WindowPos2dvMESA);
   SET_WindowPos2fMESA(disp, _mesa_WindowPos2fMESA);
   SET_WindowPos2fvMESA(disp, _mesa_WindowPos2fvMESA);
   SET_WindowPos2iMESA(disp, _mesa_WindowPos2iMESA);
   SET_WindowPos2ivMESA(disp, _mesa_WindowPos2ivMESA);
   SET_WindowPos2sMESA(disp, _mesa_WindowPos2sMESA);
   SET_WindowPos2svMESA(disp, _mesa_WindowPos2svMESA);
   SET_WindowPos3dMESA(disp, _mesa_WindowPos3dMESA);
   SET_WindowPos3dvMESA(disp, _mesa_WindowPos3dvMESA);
   SET_WindowPos3fMESA(disp, _mesa_WindowPos3fMESA);
   SET_WindowPos3fvMESA(disp, _mesa_WindowPos3fvMESA);
   SET_WindowPos3iMESA(disp, _mesa_WindowPos3iMESA);
   SET_WindowPos3ivMESA(disp, _mesa_WindowPos3ivMESA);
   SET_WindowPos3sMESA(disp, _mesa_WindowPos3sMESA);
   SET_WindowPos3svMESA(disp, _mesa_WindowPos3svMESA);
   SET_WindowPos4dMESA(disp, _mesa_WindowPos4dMESA);
   SET_WindowPos4dvMESA(disp, _mesa_WindowPos4dvMESA);
   SET_WindowPos4fMESA(disp, _mesa_WindowPos4fMESA);
   SET_WindowPos4fvMESA(disp, _mesa_WindowPos4fvMESA);
   SET_WindowPos4iMESA(disp, _mesa_WindowPos4iMESA);
   SET_WindowPos4ivMESA(disp, _mesa_WindowPos4ivMESA);
   SET_WindowPos4sMESA(disp, _mesa_WindowPos4sMESA);
   SET_WindowPos4svMESA(disp, _mesa_WindowPos4svMESA);
}


#endif /* FEATURE_rastpos */


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
 * __struct gl_contextRec::Current, and adds the extension entry points to the
 * dispatcher.
 */
void _mesa_init_rastpos( struct gl_context * ctx )
{
   int i;

   ASSIGN_4V( ctx->Current.RasterPos, 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterDistance = 0.0;
   ASSIGN_4V( ctx->Current.RasterColor, 1.0, 1.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.RasterSecondaryColor, 0.0, 0.0, 0.0, 1.0 );
   for (i = 0; i < Elements(ctx->Current.RasterTexCoords); i++)
      ASSIGN_4V( ctx->Current.RasterTexCoords[i], 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterPosValid = GL_TRUE;
}

/*@}*/
