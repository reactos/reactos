/**
 * \file rastpos.c
 * Raster position operations.
 */

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
/*#include "clip.h"*/
#include "colormac.h"
#include "context.h"
#include "feedback.h"
#include "light.h"
#include "macros.h"
#include "rastpos.h"
#include "state.h"
#include "simple_list.h"
#include "mtypes.h"

#include "math/m_matrix.h"


/**
 * Clip a point against the view volume.
 *
 * \param v vertex vector describing the point to clip.
 * 
 * \return zero if outside view volume, or one if inside.
 */
static GLuint
viewclip_point( const GLfloat v[] )
{
   if (   v[0] > v[3] || v[0] < -v[3]
       || v[1] > v[3] || v[1] < -v[3]
       || v[2] > v[3] || v[2] < -v[3] ) {
      return 0;
   }
   else {
      return 1;
   }
}


/**
 * Clip a point against the far/near Z clipping planes.
 *
 * \param v vertex vector describing the point to clip.
 * 
 * \return zero if outside view volume, or one if inside.
 */
static GLuint
viewclip_point_z( const GLfloat v[] )
{
   if (v[2] > v[3] || v[2] < -v[3] ) {
      return 0;
   }
   else {
      return 1;
   }
}


/**
 * Clip a point against the user clipping planes.
 * 
 * \param ctx GL context.
 * \param v vertex vector describing the point to clip.
 * 
 * \return zero if the point was clipped, or one otherwise.
 */
static GLuint
userclip_point( GLcontext *ctx, const GLfloat v[] )
{
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLfloat dot = v[0] * ctx->Transform._ClipUserPlane[p][0]
		     + v[1] * ctx->Transform._ClipUserPlane[p][1]
		     + v[2] * ctx->Transform._ClipUserPlane[p][2]
		     + v[3] * ctx->Transform._ClipUserPlane[p][3];
         if (dot < 0.0F) {
            return 0;
         }
      }
   }

   return 1;
}


/**
 * This has been split off to allow the normal shade routines to
 * get a little closer to the vertex buffer, and to use the
 * GLvector objects directly.
 * \param ctx the context
 * \param vertex vertex location
 * \param normal normal vector
 * \param Rcolor returned color
 * \param Rspec returned specular color (if separate specular enabled)
 * \param Rindex returned color index
 */
static void
shade_rastpos(GLcontext *ctx,
              const GLfloat vertex[4],
              const GLfloat normal[3],
              GLfloat Rcolor[4],
              GLfloat Rspec[4],
              GLfloat *Rindex)
{
   GLfloat (*base)[3] = ctx->Light._BaseColor;
   struct gl_light *light;
   GLfloat diffuseColor[4], specularColor[4];
   GLfloat diffuse = 0, specular = 0;

   if (!ctx->_ShineTable[0] || !ctx->_ShineTable[1])
      _mesa_validate_all_lighting_tables( ctx );

   COPY_3V(diffuseColor, base[0]);
   diffuseColor[3] = CLAMP( 
      ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE][3], 0.0F, 1.0F );
   ASSIGN_4V(specularColor, 0.0, 0.0, 0.0, 0.0);

   foreach (light, &ctx->Light.EnabledList) {
      GLfloat n_dot_h;
      GLfloat attenuation = 1.0;
      GLfloat VP[3];
      GLfloat n_dot_VP;
      GLfloat *h;
      GLfloat diffuseContrib[3], specularContrib[3];
      GLboolean normalized;

      if (!(light->_Flags & LIGHT_POSITIONAL)) {
	 COPY_3V(VP, light->_VP_inf_norm);
	 attenuation = light->_VP_inf_spot_attenuation;
      }
      else {
	 GLfloat d;

	 SUB_3V(VP, light->_Position, vertex);
	 d = (GLfloat) LEN_3FV( VP );

	 if ( d > 1e-6) {
	    GLfloat invd = 1.0F / d;
	    SELF_SCALE_SCALAR_3V(VP, invd);
	 }
	 attenuation = 1.0F / (light->ConstantAttenuation + d *
			       (light->LinearAttenuation + d *
				light->QuadraticAttenuation));

	 if (light->_Flags & LIGHT_SPOT) {
	    GLfloat PV_dot_dir = - DOT3(VP, light->_NormDirection);

	    if (PV_dot_dir<light->_CosCutoff) {
	       continue;
	    }
	    else {
	       double x = PV_dot_dir * (EXP_TABLE_SIZE-1);
	       int k = (int) x;
	       GLfloat spot = (GLfloat) (light->_SpotExpTable[k][0]
			       + (x-k)*light->_SpotExpTable[k][1]);
	       attenuation *= spot;
	    }
	 }
      }

      if (attenuation < 1e-3)
	 continue;

      n_dot_VP = DOT3( normal, VP );

      if (n_dot_VP < 0.0F) {
	 ACC_SCALE_SCALAR_3V(diffuseColor, attenuation, light->_MatAmbient[0]);
	 continue;
      }

      COPY_3V(diffuseContrib, light->_MatAmbient[0]);
      ACC_SCALE_SCALAR_3V(diffuseContrib, n_dot_VP, light->_MatDiffuse[0]);
      diffuse += n_dot_VP * light->_dli * attenuation;
      ASSIGN_3V(specularContrib, 0.0, 0.0, 0.0);

      {
	 if (ctx->Light.Model.LocalViewer) {
	    GLfloat v[3];
	    COPY_3V(v, vertex);
	    NORMALIZE_3FV(v);
	    SUB_3V(VP, VP, v);
	    h = VP;
	    normalized = 0;
	 }
	 else if (light->_Flags & LIGHT_POSITIONAL) {
	    h = VP;
	    ACC_3V(h, ctx->_EyeZDir);
	    normalized = 0;
	 }
         else {
	    h = light->_h_inf_norm;
	    normalized = 1;
	 }

	 n_dot_h = DOT3(normal, h);

	 if (n_dot_h > 0.0F) {
	    GLfloat (*mat)[4] = ctx->Light.Material.Attrib;
	    GLfloat spec_coef;
	    GLfloat shininess = mat[MAT_ATTRIB_FRONT_SHININESS][0];

	    if (!normalized) {
	       n_dot_h *= n_dot_h;
	       n_dot_h /= LEN_SQUARED_3FV( h );
	       shininess *= .5;
	    }

	    GET_SHINE_TAB_ENTRY( ctx->_ShineTable[0], n_dot_h, spec_coef );

	    if (spec_coef > 1.0e-10) {
               if (ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) {
                  ACC_SCALE_SCALAR_3V( specularContrib, spec_coef,
                                       light->_MatSpecular[0]);
               }
               else {
                  ACC_SCALE_SCALAR_3V( diffuseContrib, spec_coef,
                                       light->_MatSpecular[0]);
               }
	       specular += spec_coef * light->_sli * attenuation;
	    }
	 }
      }

      ACC_SCALE_SCALAR_3V( diffuseColor, attenuation, diffuseContrib );
      ACC_SCALE_SCALAR_3V( specularColor, attenuation, specularContrib );
   }

   if (ctx->Visual.rgbMode) {
      Rcolor[0] = CLAMP(diffuseColor[0], 0.0F, 1.0F);
      Rcolor[1] = CLAMP(diffuseColor[1], 0.0F, 1.0F);
      Rcolor[2] = CLAMP(diffuseColor[2], 0.0F, 1.0F);
      Rcolor[3] = CLAMP(diffuseColor[3], 0.0F, 1.0F);
      Rspec[0] = CLAMP(specularColor[0], 0.0F, 1.0F);
      Rspec[1] = CLAMP(specularColor[1], 0.0F, 1.0F);
      Rspec[2] = CLAMP(specularColor[2], 0.0F, 1.0F);
      Rspec[3] = CLAMP(specularColor[3], 0.0F, 1.0F);
   }
   else {
      GLfloat *ind = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_INDEXES];
      GLfloat d_a = ind[MAT_INDEX_DIFFUSE] - ind[MAT_INDEX_AMBIENT];
      GLfloat s_a = ind[MAT_INDEX_SPECULAR] - ind[MAT_INDEX_AMBIENT];
      GLfloat i = (ind[MAT_INDEX_AMBIENT]
		   + diffuse * (1.0F-specular) * d_a
		   + specular * s_a);
      if (i > ind[MAT_INDEX_SPECULAR]) {
	 i = ind[MAT_INDEX_SPECULAR];
      }
      *Rindex = i;
   }
}


/**
 * Set the raster position for pixel operations.
 *
 * All glRasterPos command call this function to update the current
 * raster position.
 * 
 * \param ctx GL context.
 * \param x x coordinate for the raster position.
 * \param y y coordinate for the raster position.
 * \param z z coordinate for the raster position.
 * \param w w coordinate for the raster position.
 * 
 * \sa Called by _mesa_RasterPos4f().
 *
 * Flushes the vertices, transforms and clips the vertex coordinates, and
 * finally sets the current raster position and associated data in
 * __GLcontextRec::Current.  When in selection mode calls
 * _mesa_update_hitflag() with the current raster position.
 */
static void
raster_pos4f(GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GLfloat v[4], eye[4], clip[4], ndc[3], d;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   FLUSH_CURRENT(ctx, 0);

   if (ctx->NewState)
      _mesa_update_state( ctx );

   if (ctx->VertexProgram.Enabled) {
      /* XXX implement this */
      _mesa_problem(ctx, "Vertex programs not implemented for glRasterPos");
      return;
   }
   else {
      ASSIGN_4V( v, x, y, z, w );
      TRANSFORM_POINT( eye, ctx->ModelviewMatrixStack.Top->m, v );

      /* raster color */
      if (ctx->Light.Enabled) {
         GLfloat *norm, eyenorm[3];
         GLfloat *objnorm = ctx->Current.Attrib[VERT_ATTRIB_NORMAL];

         if (ctx->_NeedEyeCoords) {
            GLfloat *inv = ctx->ModelviewMatrixStack.Top->inv;
            TRANSFORM_NORMAL( eyenorm, objnorm, inv );
            norm = eyenorm;
         }
         else {
            norm = objnorm;
         }

         shade_rastpos( ctx, v, norm,
                        ctx->Current.RasterColor,
                        ctx->Current.RasterSecondaryColor,
                        &ctx->Current.RasterIndex );
      }
      else {
         /* use current color or index */
         if (ctx->Visual.rgbMode) {
            COPY_4FV(ctx->Current.RasterColor,
                     ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
            COPY_4FV(ctx->Current.RasterSecondaryColor,
                     ctx->Current.Attrib[VERT_ATTRIB_COLOR1]);
         }
         else {
            ctx->Current.RasterIndex = ctx->Current.Index;
         }
      }

      /* compute raster distance */
      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
         ctx->Current.RasterDistance = ctx->Current.Attrib[VERT_ATTRIB_FOG][0];
      else
         ctx->Current.RasterDistance =
                         SQRTF( eye[0]*eye[0] + eye[1]*eye[1] + eye[2]*eye[2] );

      /* apply projection matrix:  clip = Proj * eye */
      TRANSFORM_POINT( clip, ctx->ProjectionMatrixStack.Top->m, eye );

      /* clip to view volume */
      if (ctx->Transform.RasterPositionUnclipped) {
         /* GL_IBM_rasterpos_clip: only clip against Z */
         if (viewclip_point_z(clip) == 0) {
            ctx->Current.RasterPosValid = GL_FALSE;
            return;
         }
      }
      else if (viewclip_point(clip) == 0) {
         /* Normal OpenGL behaviour */
         ctx->Current.RasterPosValid = GL_FALSE;
         return;
      }

      /* clip to user clipping planes */
      if (ctx->Transform.ClipPlanesEnabled && !userclip_point(ctx, clip)) {
         ctx->Current.RasterPosValid = GL_FALSE;
         return;
      }

      /* ndc = clip / W */
      d = (clip[3] == 0.0F) ? 1.0F : 1.0F / clip[3];
      ndc[0] = clip[0] * d;
      ndc[1] = clip[1] * d;
      ndc[2] = clip[2] * d;

      ctx->Current.RasterPos[0] = (ndc[0] * ctx->Viewport._WindowMap.m[MAT_SX] +
                                   ctx->Viewport._WindowMap.m[MAT_TX]);
      ctx->Current.RasterPos[1] = (ndc[1] * ctx->Viewport._WindowMap.m[MAT_SY] +
                                   ctx->Viewport._WindowMap.m[MAT_TY]);
      ctx->Current.RasterPos[2] = (ndc[2] * ctx->Viewport._WindowMap.m[MAT_SZ] +
                                   ctx->Viewport._WindowMap.m[MAT_TZ]) / ctx->DepthMaxF;
      ctx->Current.RasterPos[3] = clip[3];
      ctx->Current.RasterPosValid = GL_TRUE;

      {
         GLuint texSet;
         for (texSet = 0; texSet < ctx->Const.MaxTextureCoordUnits; texSet++) {
            COPY_4FV( ctx->Current.RasterTexCoords[texSet],
                     ctx->Current.Attrib[VERT_ATTRIB_TEX0 + texSet] );
         }
      }

   }

   if (ctx->RenderMode==GL_SELECT) {
      _mesa_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }
}


/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2d(GLdouble x, GLdouble y)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2f(GLfloat x, GLfloat y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2i(GLint x, GLint y)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2s(GLshort x, GLshort y)
{
   _mesa_RasterPos4f(x, y, 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3i(GLint x, GLint y, GLint z)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   _mesa_RasterPos4f(x, y, z, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

/** Calls raster_pos4f() */
void GLAPIENTRY
_mesa_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   raster_pos4f(ctx, x, y, z, w);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   _mesa_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   _mesa_RasterPos4f(x, y, z, w);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2dv(const GLdouble *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2iv(const GLint *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos2sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3dv(const GLdouble *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3iv(const GLint *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos3sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos4dv(const GLdouble *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos4fv(const GLfloat *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos4iv(const GLint *v)
{
   _mesa_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 
		     (GLfloat) v[2], (GLfloat) v[3]);
}

/** Calls _mesa_RasterPos4f() */
void GLAPIENTRY
_mesa_RasterPos4sv(const GLshort *v)
{
   _mesa_RasterPos4f(v[0], v[1], v[2], v[3]);
}


/**********************************************************************/
/***           GL_ARB_window_pos / GL_MESA_window_pos               ***/
/**********************************************************************/

#if FEATURE_windowpos
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
      ctx->Current.RasterIndex = ctx->Current.Index;
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
   ctx->Current.RasterIndex = 1.0;
   for (i=0; i<MAX_TEXTURE_UNITS; i++)
      ASSIGN_4V( ctx->Current.RasterTexCoords[i], 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterPosValid = GL_TRUE;

  /*
    * For XFree86/DRI: tell libGL to add these functions to the dispatcher.
    * Basically, we should add all extension functions above offset 577.
    * This enables older libGL libraries to work with newer drivers that
    * have newer extensions.
    */
   /* GL_ARB_window_pos aliases with GL_MESA_window_pos */
   _glapi_add_entrypoint("glWindowPos2dARB", 513);
   _glapi_add_entrypoint("glWindowPos2dvARB", 514);
   _glapi_add_entrypoint("glWindowPos2fARB", 515);
   _glapi_add_entrypoint("glWindowPos2fvARB", 516);
   _glapi_add_entrypoint("glWindowPos2iARB", 517);
   _glapi_add_entrypoint("glWindowPos2ivARB", 518);
   _glapi_add_entrypoint("glWindowPos2sARB", 519);
   _glapi_add_entrypoint("glWindowPos2svARB", 520);
   _glapi_add_entrypoint("glWindowPos3dARB", 521);
   _glapi_add_entrypoint("glWindowPos3dvARB", 522);
   _glapi_add_entrypoint("glWindowPos3fARB", 523);
   _glapi_add_entrypoint("glWindowPos3fvARB", 524);
   _glapi_add_entrypoint("glWindowPos3iARB", 525);
   _glapi_add_entrypoint("glWindowPos3ivARB", 526);
   _glapi_add_entrypoint("glWindowPos3sARB", 527);
   _glapi_add_entrypoint("glWindowPos3svARB", 528);
}

/*@}*/
