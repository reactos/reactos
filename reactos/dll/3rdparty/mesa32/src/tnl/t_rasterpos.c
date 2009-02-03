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


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "feedback.h"
#include "light.h"
#include "main/macros.h"
#include "rastpos.h"
#include "simple_list.h"
#include "main/mtypes.h"

#include "math/m_matrix.h"
#include "tnl/tnl.h"



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
 * Compute lighting for the raster position.  Both RGB and CI modes computed.
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
   /*const*/ GLfloat (*base)[3] = ctx->Light._BaseColor;
   const struct gl_light *light;
   GLfloat diffuseColor[4], specularColor[4];  /* for RGB mode only */
   GLfloat diffuseCI = 0.0, specularCI = 0.0;  /* for CI mode only */

   _mesa_validate_all_lighting_tables( ctx );

   COPY_3V(diffuseColor, base[0]);
   diffuseColor[3] = CLAMP( 
      ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE][3], 0.0F, 1.0F );
   ASSIGN_4V(specularColor, 0.0, 0.0, 0.0, 1.0);

   foreach (light, &ctx->Light.EnabledList) {
      GLfloat attenuation = 1.0;
      GLfloat VP[3]; /* vector from vertex to light pos */
      GLfloat n_dot_VP;
      GLfloat diffuseContrib[3], specularContrib[3];

      if (!(light->_Flags & LIGHT_POSITIONAL)) {
         /* light at infinity */
	 COPY_3V(VP, light->_VP_inf_norm);
	 attenuation = light->_VP_inf_spot_attenuation;
      }
      else {
         /* local/positional light */
	 GLfloat d;

         /* VP = vector from vertex pos to light[i].pos */
	 SUB_3V(VP, light->_Position, vertex);
         /* d = length(VP) */
	 d = (GLfloat) LEN_3FV( VP );
	 if (d > 1.0e-6) {
            /* normalize VP */
	    GLfloat invd = 1.0F / d;
	    SELF_SCALE_SCALAR_3V(VP, invd);
	 }

         /* atti */
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

      /* Ambient + diffuse */
      COPY_3V(diffuseContrib, light->_MatAmbient[0]);
      ACC_SCALE_SCALAR_3V(diffuseContrib, n_dot_VP, light->_MatDiffuse[0]);
      diffuseCI += n_dot_VP * light->_dli * attenuation;

      /* Specular */
      {
         const GLfloat *h;
         GLfloat n_dot_h;

         ASSIGN_3V(specularContrib, 0.0, 0.0, 0.0);

	 if (ctx->Light.Model.LocalViewer) {
	    GLfloat v[3];
	    COPY_3V(v, vertex);
	    NORMALIZE_3FV(v);
	    SUB_3V(VP, VP, v);
            NORMALIZE_3FV(VP);
	    h = VP;
	 }
	 else if (light->_Flags & LIGHT_POSITIONAL) {
	    ACC_3V(VP, ctx->_EyeZDir);
            NORMALIZE_3FV(VP);
	    h = VP;
	 }
         else {
	    h = light->_h_inf_norm;
	 }

	 n_dot_h = DOT3(normal, h);

	 if (n_dot_h > 0.0F) {
	    GLfloat spec_coef;
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
               /*assert(light->_sli > 0.0);*/
               specularCI += spec_coef * light->_sli * attenuation;
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
		   + diffuseCI * (1.0F-specularCI) * d_a
		   + specularCI * s_a);
      if (i > ind[MAT_INDEX_SPECULAR]) {
	 i = ind[MAT_INDEX_SPECULAR];
      }
      *Rindex = i;
   }
}


/**
 * Do texgen needed for glRasterPos.
 * \param ctx  rendering context
 * \param vObj  object-space vertex coordinate
 * \param vEye  eye-space vertex coordinate
 * \param normal  vertex normal
 * \param unit  texture unit number
 * \param texcoord  incoming texcoord and resulting texcoord
 */
static void
compute_texgen(GLcontext *ctx, const GLfloat vObj[4], const GLfloat vEye[4],
               const GLfloat normal[3], GLuint unit, GLfloat texcoord[4])
{
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   /* always compute sphere map terms, just in case */
   GLfloat u[3], two_nu, rx, ry, rz, m, mInv;
   COPY_3V(u, vEye);
   NORMALIZE_3FV(u);
   two_nu = 2.0F * DOT3(normal, u);
   rx = u[0] - normal[0] * two_nu;
   ry = u[1] - normal[1] * two_nu;
   rz = u[2] - normal[2] * two_nu;
   m = rx * rx + ry * ry + (rz + 1.0F) * (rz + 1.0F);
   if (m > 0.0F)
      mInv = 0.5F * _mesa_inv_sqrtf(m);
   else
      mInv = 0.0F;

   if (texUnit->TexGenEnabled & S_BIT) {
      switch (texUnit->GenModeS) {
         case GL_OBJECT_LINEAR:
            texcoord[0] = DOT4(vObj, texUnit->ObjectPlaneS);
            break;
         case GL_EYE_LINEAR:
            texcoord[0] = DOT4(vEye, texUnit->EyePlaneS);
            break;
         case GL_SPHERE_MAP:
            texcoord[0] = rx * mInv + 0.5F;
            break;
         case GL_REFLECTION_MAP:
            texcoord[0] = rx;
            break;
         case GL_NORMAL_MAP:
            texcoord[0] = normal[0];
            break;
         default:
            _mesa_problem(ctx, "Bad S texgen in compute_texgen()");
            return;
      }
   }

   if (texUnit->TexGenEnabled & T_BIT) {
      switch (texUnit->GenModeT) {
         case GL_OBJECT_LINEAR:
            texcoord[1] = DOT4(vObj, texUnit->ObjectPlaneT);
            break;
         case GL_EYE_LINEAR:
            texcoord[1] = DOT4(vEye, texUnit->EyePlaneT);
            break;
         case GL_SPHERE_MAP:
            texcoord[1] = ry * mInv + 0.5F;
            break;
         case GL_REFLECTION_MAP:
            texcoord[1] = ry;
            break;
         case GL_NORMAL_MAP:
            texcoord[1] = normal[1];
            break;
         default:
            _mesa_problem(ctx, "Bad T texgen in compute_texgen()");
            return;
      }
   }

   if (texUnit->TexGenEnabled & R_BIT) {
      switch (texUnit->GenModeR) {
         case GL_OBJECT_LINEAR:
            texcoord[2] = DOT4(vObj, texUnit->ObjectPlaneR);
            break;
         case GL_EYE_LINEAR:
            texcoord[2] = DOT4(vEye, texUnit->EyePlaneR);
            break;
         case GL_REFLECTION_MAP:
            texcoord[2] = rz;
            break;
         case GL_NORMAL_MAP:
            texcoord[2] = normal[2];
            break;
         default:
            _mesa_problem(ctx, "Bad R texgen in compute_texgen()");
            return;
      }
   }

   if (texUnit->TexGenEnabled & Q_BIT) {
      switch (texUnit->GenModeQ) {
         case GL_OBJECT_LINEAR:
            texcoord[3] = DOT4(vObj, texUnit->ObjectPlaneQ);
            break;
         case GL_EYE_LINEAR:
            texcoord[3] = DOT4(vEye, texUnit->EyePlaneQ);
            break;
         default:
            _mesa_problem(ctx, "Bad Q texgen in compute_texgen()");
            return;
      }
   }
}


/**
 * glRasterPos transformation.  Typically called via ctx->Driver.RasterPos().
 * XXX some of this code (such as viewport xform, clip testing and setting
 * of ctx->Current.Raster* fields) could get lifted up into the
 * main/rasterpos.c code.
 *
 * \param vObj  vertex position in object space
 */
void
_tnl_RasterPos(GLcontext *ctx, const GLfloat vObj[4])
{
   if (ctx->VertexProgram._Enabled) {
      /* XXX implement this */
      _mesa_problem(ctx, "Vertex programs not implemented for glRasterPos");
      return;
   }
   else {
      GLfloat eye[4], clip[4], ndc[3], d;
      GLfloat *norm, eyenorm[3];
      GLfloat *objnorm = ctx->Current.Attrib[VERT_ATTRIB_NORMAL];

      /* apply modelview matrix:  eye = MV * obj */
      TRANSFORM_POINT( eye, ctx->ModelviewMatrixStack.Top->m, vObj );
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
      /* wincoord = viewport_mapping(ndc) */
      ctx->Current.RasterPos[0] = (ndc[0] * ctx->Viewport._WindowMap.m[MAT_SX]
                                   + ctx->Viewport._WindowMap.m[MAT_TX]);
      ctx->Current.RasterPos[1] = (ndc[1] * ctx->Viewport._WindowMap.m[MAT_SY]
                                   + ctx->Viewport._WindowMap.m[MAT_TY]);
      ctx->Current.RasterPos[2] = (ndc[2] * ctx->Viewport._WindowMap.m[MAT_SZ]
                                   + ctx->Viewport._WindowMap.m[MAT_TZ])
                                  / ctx->DrawBuffer->_DepthMaxF;
      ctx->Current.RasterPos[3] = clip[3];

      /* compute raster distance */
      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
         ctx->Current.RasterDistance = ctx->Current.Attrib[VERT_ATTRIB_FOG][0];
      else
         ctx->Current.RasterDistance =
                        SQRTF( eye[0]*eye[0] + eye[1]*eye[1] + eye[2]*eye[2] );

      /* compute transformed normal vector (for lighting or texgen) */
      if (ctx->_NeedEyeCoords) {
         const GLfloat *inv = ctx->ModelviewMatrixStack.Top->inv;
         TRANSFORM_NORMAL( eyenorm, objnorm, inv );
         norm = eyenorm;
      }
      else {
         norm = objnorm;
      }

      /* update raster color */
      if (ctx->Light.Enabled) {
         /* lighting */
         shade_rastpos( ctx, vObj, norm,
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
            ctx->Current.RasterIndex
               = ctx->Current.Attrib[VERT_ATTRIB_COLOR_INDEX][0];
         }
      }

      /* texture coords */
      {
         GLuint u;
         for (u = 0; u < ctx->Const.MaxTextureCoordUnits; u++) {
            GLfloat tc[4];
            COPY_4V(tc, ctx->Current.Attrib[VERT_ATTRIB_TEX0 + u]);
            if (ctx->Texture.Unit[u].TexGenEnabled) {
               compute_texgen(ctx, vObj, eye, norm, u, tc);
            }
            TRANSFORM_POINT(ctx->Current.RasterTexCoords[u],
                            ctx->TextureMatrixStack[u].Top->m, tc);
         }
      }

      ctx->Current.RasterPosValid = GL_TRUE;
   }

   if (ctx->RenderMode == GL_SELECT) {
      _mesa_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }
}
