/* $Id: light.c,v 1.14 1997/07/24 01:24:11 brianp Exp $ */

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
 * $Log: light.c,v $
 * Revision 1.14  1997/07/24 01:24:11  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.13  1997/06/20 04:15:43  brianp
 * optimized changing of SHININESS (Henk Kok)
 *
 * Revision 1.12  1997/05/28 03:25:26  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.11  1997/05/01 01:38:57  brianp
 * now use NORMALIZE_3FV() macro from mmath.h
 *
 * Revision 1.10  1997/04/20 20:28:49  brianp
 * replaced abort() with gl_problem()
 *
 * Revision 1.9  1997/04/07 02:59:17  brianp
 * small optimization to setting of shininess and spot exponent
 *
 * Revision 1.8  1997/04/01 04:09:31  brianp
 * misc code clean-ups.  moved shading code to shade.c
 *
 * Revision 1.7  1997/03/11 00:37:39  brianp
 * spotlight factor now effects ambient lighting
 *
 * Revision 1.6  1996/12/18 20:02:07  brianp
 * glColorMaterial() and glMaterial() should finally work right!
 *
 * Revision 1.5  1996/12/07 10:22:41  brianp
 * gl_Materialfv() now calls gl_set_material() if GL_COLOR_MATERIAL disabled
 * implemented gl_GetLightiv()
 *
 * Revision 1.4  1996/11/08 04:39:23  brianp
 * new gl_compute_spot_exp_table() contributed by Randy Frank
 *
 * Revision 1.3  1996/09/27 01:27:55  brianp
 * removed unused variables
 *
 * Revision 1.2  1996/09/15 14:18:10  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "context.h"
#include "light.h"
#include "dlist.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "types.h"
#include "vb.h"
#include "xform.h"
#endif



void gl_ShadeModel( GLcontext *ctx, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glShadeModel" );
      return;
   }

   switch (mode) {
      case GL_FLAT:
      case GL_SMOOTH:
         if (ctx->Light.ShadeModel!=mode) {
            ctx->Light.ShadeModel = mode;
            ctx->NewState |= NEW_RASTER_OPS;
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glShadeModel" );
   }
}



void gl_Lightfv( GLcontext *ctx,
                 GLenum light, GLenum pname, const GLfloat *params,
                 GLint nparams )
{
   GLint l;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glShadeModel" );
      return;
   }

   l = (GLint) (light - GL_LIGHT0);

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glLight" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( ctx->Light.Light[l].Ambient, params );
         break;
      case GL_DIFFUSE:
         COPY_4V( ctx->Light.Light[l].Diffuse, params );
         break;
      case GL_SPECULAR:
         COPY_4V( ctx->Light.Light[l].Specular, params );
         break;
      case GL_POSITION:
	 /* transform position by ModelView matrix */
	 TRANSFORM_POINT( ctx->Light.Light[l].Position, ctx->ModelViewMatrix,
                          params );
         break;
      case GL_SPOT_DIRECTION:
	 /* transform direction by inverse modelview */
         {
            GLfloat direction[4];
            direction[0] = params[0];
            direction[1] = params[1];
            direction[2] = params[2];
            direction[3] = 0.0;
            if (ctx->NewModelViewMatrix) {
               gl_analyze_modelview_matrix( ctx );
            }
            gl_transform_vector( ctx->Light.Light[l].Direction,
                                 direction, ctx->ModelViewInv);
         }
         break;
      case GL_SPOT_EXPONENT:
         if (params[0]<0.0 || params[0]>128.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         if (ctx->Light.Light[l].SpotExponent != params[0]) {
            ctx->Light.Light[l].SpotExponent = params[0];
            gl_compute_spot_exp_table( &ctx->Light.Light[l] );
         }
         break;
      case GL_SPOT_CUTOFF:
         if ((params[0]<0.0 || params[0]>90.0) && params[0]!=180.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].SpotCutoff = params[0];
         ctx->Light.Light[l].CosCutoff = cos(params[0]*DEG2RAD);
         break;
      case GL_CONSTANT_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].ConstantAttenuation = params[0];
         break;
      case GL_LINEAR_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].LinearAttenuation = params[0];
         break;
      case GL_QUADRATIC_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].QuadraticAttenuation = params[0];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glLight" );
         break;
   }

   ctx->NewState |= NEW_LIGHTING;
}



void gl_GetLightfv( GLcontext *ctx,
                    GLenum light, GLenum pname, GLfloat *params )
{
   GLint l = (GLint) (light - GL_LIGHT0);

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( params, ctx->Light.Light[l].Ambient );
         break;
      case GL_DIFFUSE:
         COPY_4V( params, ctx->Light.Light[l].Diffuse );
         break;
      case GL_SPECULAR:
         COPY_4V( params, ctx->Light.Light[l].Specular );
         break;
      case GL_POSITION:
         COPY_4V( params, ctx->Light.Light[l].Position );
         break;
      case GL_SPOT_DIRECTION:
         COPY_3V( params, ctx->Light.Light[l].Direction );
         break;
      case GL_SPOT_EXPONENT:
         params[0] = ctx->Light.Light[l].SpotExponent;
         break;
      case GL_SPOT_CUTOFF:
         params[0] = ctx->Light.Light[l].SpotCutoff;
         break;
      case GL_CONSTANT_ATTENUATION:
         params[0] = ctx->Light.Light[l].ConstantAttenuation;
         break;
      case GL_LINEAR_ATTENUATION:
         params[0] = ctx->Light.Light[l].LinearAttenuation;
         break;
      case GL_QUADRATIC_ATTENUATION:
         params[0] = ctx->Light.Light[l].QuadraticAttenuation;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
         break;
   }
}



void gl_GetLightiv( GLcontext *ctx, GLenum light, GLenum pname, GLint *params )
{
   GLint l = (GLint) (light - GL_LIGHT0);

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[3]);
         break;
      case GL_DIFFUSE:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[3]);
         break;
      case GL_SPECULAR:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[3]);
         break;
      case GL_POSITION:
         params[0] = ctx->Light.Light[l].Position[0];
         params[1] = ctx->Light.Light[l].Position[1];
         params[2] = ctx->Light.Light[l].Position[2];
         params[3] = ctx->Light.Light[l].Position[3];
         break;
      case GL_SPOT_DIRECTION:
         params[0] = ctx->Light.Light[l].Direction[0];
         params[1] = ctx->Light.Light[l].Direction[1];
         params[2] = ctx->Light.Light[l].Direction[2];
         break;
      case GL_SPOT_EXPONENT:
         params[0] = ctx->Light.Light[l].SpotExponent;
         break;
      case GL_SPOT_CUTOFF:
         params[0] = ctx->Light.Light[l].SpotCutoff;
         break;
      case GL_CONSTANT_ATTENUATION:
         params[0] = ctx->Light.Light[l].ConstantAttenuation;
         break;
      case GL_LINEAR_ATTENUATION:
         params[0] = ctx->Light.Light[l].LinearAttenuation;
         break;
      case GL_QUADRATIC_ATTENUATION:
         params[0] = ctx->Light.Light[l].QuadraticAttenuation;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
         break;
   }
}



/**********************************************************************/
/***                        Light Model                             ***/
/**********************************************************************/


void gl_LightModelfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         COPY_4V( ctx->Light.Model.Ambient, params );
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
         if (params[0]==0.0)
            ctx->Light.Model.LocalViewer = GL_FALSE;
         else
            ctx->Light.Model.LocalViewer = GL_TRUE;
         break;
      case GL_LIGHT_MODEL_TWO_SIDE:
         if (params[0]==0.0)
            ctx->Light.Model.TwoSide = GL_FALSE;
         else
            ctx->Light.Model.TwoSide = GL_TRUE;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glLightModel" );
         break;
   }
   ctx->NewState |= NEW_LIGHTING;
}




/********** MATERIAL **********/


/*
 * Given a face and pname value (ala glColorMaterial), compute a bitmask
 * of the targeted material values.
 */
GLuint gl_material_bitmask( GLenum face, GLenum pname )
{
   GLuint bitmask = 0;

   /* Make a bitmask indicating what material attribute(s) we're updating */
   switch (pname) {
      case GL_EMISSION:
         bitmask |= FRONT_EMISSION_BIT | BACK_EMISSION_BIT;
         break;
      case GL_AMBIENT:
         bitmask |= FRONT_AMBIENT_BIT | BACK_AMBIENT_BIT;
         break;
      case GL_DIFFUSE:
         bitmask |= FRONT_DIFFUSE_BIT | BACK_DIFFUSE_BIT;
         break;
      case GL_SPECULAR:
         bitmask |= FRONT_SPECULAR_BIT | BACK_SPECULAR_BIT;
         break;
      case GL_SHININESS:
         bitmask |= FRONT_SHININESS_BIT | BACK_SHININESS_BIT;
         break;
      case GL_AMBIENT_AND_DIFFUSE:
         bitmask |= FRONT_AMBIENT_BIT | BACK_AMBIENT_BIT;
         bitmask |= FRONT_DIFFUSE_BIT | BACK_DIFFUSE_BIT;
         break;
      case GL_COLOR_INDEXES:
         bitmask |= FRONT_INDEXES_BIT  | BACK_INDEXES_BIT;
         break;
      default:
         gl_problem(NULL, "Bad param in gl_material_bitmask");
         return 0;
   }

   ASSERT( face==GL_FRONT || face==GL_BACK || face==GL_FRONT_AND_BACK );

   if (face==GL_FRONT) {
      bitmask &= FRONT_MATERIAL_BITS;
   }
   else if (face==GL_BACK) {
      bitmask &= BACK_MATERIAL_BITS;
   }

   return bitmask;
}



/*
 * This is called by glColor() when GL_COLOR_MATERIAL is enabled and
 * called by glMaterial() when GL_COLOR_MATERIAL is disabled.
 */
void gl_set_material( GLcontext *ctx, GLuint bitmask, const GLfloat *params )
{
   struct gl_material *mat;

   if (INSIDE_BEGIN_END(ctx)) {
      struct vertex_buffer *VB = ctx->VB;
      /* Save per-vertex material changes in the Vertex Buffer.
       * The update_material function will eventually update the global
       * ctx->Light.Material values.
       */
      mat = VB->Material[VB->Count];
      VB->MaterialMask[VB->Count] |= bitmask;
      VB->MonoMaterial = GL_FALSE;
   }
   else {
      /* just update the global material property */
      mat = ctx->Light.Material;
      ctx->NewState |= NEW_LIGHTING;
   }

   if (bitmask & FRONT_AMBIENT_BIT) {
      COPY_4V( mat[0].Ambient, params );
   }
   if (bitmask & BACK_AMBIENT_BIT) {
      COPY_4V( mat[1].Ambient, params );
   }
   if (bitmask & FRONT_DIFFUSE_BIT) {
      COPY_4V( mat[0].Diffuse, params );
   }
   if (bitmask & BACK_DIFFUSE_BIT) {
      COPY_4V( mat[1].Diffuse, params );
   }
   if (bitmask & FRONT_SPECULAR_BIT) {
      COPY_4V( mat[0].Specular, params );
   }
   if (bitmask & BACK_SPECULAR_BIT) {
      COPY_4V( mat[1].Specular, params );
   }
   if (bitmask & FRONT_EMISSION_BIT) {
      COPY_4V( mat[0].Emission, params );
   }
   if (bitmask & BACK_EMISSION_BIT) {
      COPY_4V( mat[1].Emission, params );
   }
   if (bitmask & FRONT_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      if (mat[0].Shininess != shininess) {
         mat[0].Shininess = shininess;
         gl_compute_material_shine_table( &mat[0] );
      }
   }
   if (bitmask & BACK_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      if (mat[1].Shininess != shininess) {
         mat[1].Shininess = shininess;
         gl_compute_material_shine_table( &mat[1] );
      }
   }
   if (bitmask & FRONT_INDEXES_BIT) {
      mat[0].AmbientIndex = params[0];
      mat[0].DiffuseIndex = params[1];
      mat[0].SpecularIndex = params[2];
   }
   if (bitmask & BACK_INDEXES_BIT) {
      mat[1].AmbientIndex = params[0];
      mat[1].DiffuseIndex = params[1];
      mat[1].SpecularIndex = params[2];
   }
}



void gl_ColorMaterial( GLcontext *ctx, GLenum face, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glColorMaterial" );
      return;
   }
   switch (face) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
         ctx->Light.ColorMaterialFace = face;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glColorMaterial(face)" );
         return;
   }
   switch (mode) {
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_AMBIENT_AND_DIFFUSE:
         ctx->Light.ColorMaterialMode = mode;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glColorMaterial(mode)" );
         return;
   }

   ctx->Light.ColorMaterialBitmask = gl_material_bitmask( face, mode );
}



/*
 * This is only called via the api_function_table struct or by the
 * display list executor.
 */
void gl_Materialfv( GLcontext *ctx,
                    GLenum face, GLenum pname, const GLfloat *params )
{
   GLuint bitmask;

   /* error checking */
   if (face!=GL_FRONT && face!=GL_BACK && face!=GL_FRONT_AND_BACK) {
      gl_error( ctx, GL_INVALID_ENUM, "glMaterial(face)" );
      return;
   }
   switch (pname) {
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_SHININESS:
      case GL_AMBIENT_AND_DIFFUSE:
      case GL_COLOR_INDEXES:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glMaterial(pname)" );
         return;
   }

   /* convert face and pname to a bitmask */
   bitmask = gl_material_bitmask( face, pname );

   if (ctx->Light.ColorMaterialEnabled) {
      /* The material values specified by glColorMaterial() can't be */
      /* updated by glMaterial() while GL_COLOR_MATERIAL is enabled! */
      bitmask &= ~ctx->Light.ColorMaterialBitmask;
   }

   gl_set_material( ctx, bitmask, params );
}




void gl_GetMaterialfv( GLcontext *ctx,
                       GLenum face, GLenum pname, GLfloat *params )
{
   GLuint f;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetMaterialfv" );
      return;
   }
   if (face==GL_FRONT) {
      f = 0;
   }
   else if (face==GL_BACK) {
      f = 1;
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(face)" );
      return;
   }
   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( params, ctx->Light.Material[f].Ambient );
         break;
      case GL_DIFFUSE:
         COPY_4V( params, ctx->Light.Material[f].Diffuse );
	 break;
      case GL_SPECULAR:
         COPY_4V( params, ctx->Light.Material[f].Specular );
	 break;
      case GL_EMISSION:
	 COPY_4V( params, ctx->Light.Material[f].Emission );
	 break;
      case GL_SHININESS:
	 *params = ctx->Light.Material[f].Shininess;
	 break;
      case GL_COLOR_INDEXES:
	 params[0] = ctx->Light.Material[f].AmbientIndex;
	 params[1] = ctx->Light.Material[f].DiffuseIndex;
	 params[2] = ctx->Light.Material[f].SpecularIndex;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(pname)" );
   }
}



void gl_GetMaterialiv( GLcontext *ctx,
                       GLenum face, GLenum pname, GLint *params )
{
   GLuint f;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetMaterialiv" );
      return;
   }
   if (face==GL_FRONT) {
      f = 0;
   }
   else if (face==GL_BACK) {
      f = 1;
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialiv(face)" );
      return;
   }
   switch (pname) {
      case GL_AMBIENT:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[3] );
         break;
      case GL_DIFFUSE:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[3] );
	 break;
      case GL_SPECULAR:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[3] );
	 break;
      case GL_EMISSION:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[3] );
	 break;
      case GL_SHININESS:
         *params = ROUNDF( ctx->Light.Material[f].Shininess );
	 break;
      case GL_COLOR_INDEXES:
	 params[0] = ROUNDF( ctx->Light.Material[f].AmbientIndex );
	 params[1] = ROUNDF( ctx->Light.Material[f].DiffuseIndex );
	 params[2] = ROUNDF( ctx->Light.Material[f].SpecularIndex );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(pname)" );
   }
}




/**********************************************************************/
/*****                  Lighting computation                      *****/
/**********************************************************************/


/*
 * Notes:
 *   When two-sided lighting is enabled we compute the color (or index)
 *   for both the front and back side of the primitive.  Then, when the
 *   orientation of the facet is later learned, we can determine which
 *   color (or index) to use for rendering.
 *
 * Variables:
 *   n = normal vector
 *   V = vertex position
 *   P = light source position
 *   Pe = (0,0,0,1)
 *
 * Precomputed:
 *   IF P[3]==0 THEN
 *       // light at infinity
 *       IF local_viewer THEN
 *           VP_inf_norm = unit vector from V to P      // Precompute
 *       ELSE 
 *           // eye at infinity
 *           h_inf_norm = Normalize( VP + <0,0,1> )     // Precompute
 *       ENDIF
 *   ENDIF
 *
 * Functions:
 *   Normalize( v ) = normalized vector v
 *   Magnitude( v ) = length of vector v
 */



/*
 * Whenever the spotlight exponent for a light changes we must call
 * this function to recompute the exponent lookup table.
 */
void gl_compute_spot_exp_table( struct gl_light *l )
{
   int i;
   double exponent = l->SpotExponent;
   double tmp;
   int clamp = 0;

   l->SpotExpTable[0][0] = 0.0;

   for (i=EXP_TABLE_SIZE-1;i>0;i--) {
      if (clamp == 0) {
         tmp = pow(i/(double)(EXP_TABLE_SIZE-1), exponent);
         if (tmp < FLT_MIN*100.0) {
            tmp = 0.0;
            clamp = 1;
         }
      }
      l->SpotExpTable[i][0] = tmp;
   }
   for (i=0;i<EXP_TABLE_SIZE-1;i++) {
      l->SpotExpTable[i][1] = l->SpotExpTable[i+1][0] - l->SpotExpTable[i][0];
   }
   l->SpotExpTable[EXP_TABLE_SIZE-1][1] = 0.0;
}



/*
 * Whenever the shininess of a material changes we must call this
 * function to recompute the exponential lookup table.
 */
void gl_compute_material_shine_table( struct gl_material *m )
{
   int i;

   m->ShineTable[0] = 0.0F;
   for (i=1;i<SHINE_TABLE_SIZE;i++) {
#if 0
      double x = pow( i/(double)(SHINE_TABLE_SIZE-1), exponent );
      if (x<1.0e-10) {
         m->ShineTable[i] = 0.0F;
      }
      else {
         m->ShineTable[i] = x;
      }
#else
      /* just invalidate the table */
      m->ShineTable[i] = -1.0;
#endif
   }
}



/*
 * Examine current lighting parameters to determine if the optimized lighting
 * function can be used.
 * Also, precompute some lighting values such as the products of light
 * source and material ambient, diffuse and specular coefficients.
 */
void gl_update_lighting( GLcontext *ctx )
{
   GLint i, side;
   struct gl_light *prev_enabled, *light;

   if (!ctx->Light.Enabled) {
      /* If lighting is not enabled, we can skip all this. */
      return;
   }

   /* Setup linked list of enabled light sources */
   prev_enabled = NULL;
   ctx->Light.FirstEnabled = NULL;
   for (i=0;i<MAX_LIGHTS;i++) {
      ctx->Light.Light[i].NextEnabled = NULL;
      if (ctx->Light.Light[i].Enabled) {
         if (prev_enabled) {
            prev_enabled->NextEnabled = &ctx->Light.Light[i];
         }
         else {
            ctx->Light.FirstEnabled = &ctx->Light.Light[i];
         }
         prev_enabled = &ctx->Light.Light[i];
      }
   }

   /* base color = material_emission + global_ambient * material_ambient */
   for (side=0; side<2; side++) {
      ctx->Light.BaseColor[side][0] = ctx->Light.Material[side].Emission[0]
         + ctx->Light.Model.Ambient[0] * ctx->Light.Material[side].Ambient[0];
      ctx->Light.BaseColor[side][1] = ctx->Light.Material[side].Emission[1]
         + ctx->Light.Model.Ambient[1] * ctx->Light.Material[side].Ambient[1];
      ctx->Light.BaseColor[side][2] = ctx->Light.Material[side].Emission[2]
         + ctx->Light.Model.Ambient[2] * ctx->Light.Material[side].Ambient[2];
      ctx->Light.BaseColor[side][3]
         = MIN2( ctx->Light.Material[side].Diffuse[3], 1.0F );
   }


   /* Precompute some lighting stuff */
   for (light = ctx->Light.FirstEnabled; light; light = light->NextEnabled) {
      for (side=0; side<2; side++) {
         struct gl_material *mat = &ctx->Light.Material[side];
         /* Add each light's ambient component to base color */
         ctx->Light.BaseColor[side][0] += light->Ambient[0] * mat->Ambient[0];
         ctx->Light.BaseColor[side][1] += light->Ambient[1] * mat->Ambient[1];
         ctx->Light.BaseColor[side][2] += light->Ambient[2] * mat->Ambient[2];
         /* compute product of light's ambient with front material ambient */
         light->MatAmbient[side][0] = light->Ambient[0] * mat->Ambient[0];
         light->MatAmbient[side][1] = light->Ambient[1] * mat->Ambient[1];
         light->MatAmbient[side][2] = light->Ambient[2] * mat->Ambient[2];
         /* compute product of light's diffuse with front material diffuse */
         light->MatDiffuse[side][0] = light->Diffuse[0] * mat->Diffuse[0];
         light->MatDiffuse[side][1] = light->Diffuse[1] * mat->Diffuse[1];
         light->MatDiffuse[side][2] = light->Diffuse[2] * mat->Diffuse[2];
         /* compute product of light's specular with front material specular */
         light->MatSpecular[side][0] = light->Specular[0] * mat->Specular[0];
         light->MatSpecular[side][1] = light->Specular[1] * mat->Specular[1];
         light->MatSpecular[side][2] = light->Specular[2] * mat->Specular[2];

         /* VP (VP) = Normalize( Position ) */
         COPY_3V( light->VP_inf_norm, light->Position );
         NORMALIZE_3FV( light->VP_inf_norm );

         /* h_inf_norm = Normalize( V_to_P + <0,0,1> ) */
         COPY_3V( light->h_inf_norm, light->VP_inf_norm );
         light->h_inf_norm[2] += 1.0F;
         NORMALIZE_3FV( light->h_inf_norm );

         COPY_3V( light->NormDirection, light->Direction );
         NORMALIZE_3FV( light->NormDirection );

         /* Compute color index diffuse and specular light intensities */
         light->dli = 0.30F * light->Diffuse[0]
                    + 0.59F * light->Diffuse[1]
                    + 0.11F * light->Diffuse[2];
         light->sli = 0.30F * light->Specular[0]
                    + 0.59F * light->Specular[1]
                    + 0.11F * light->Specular[2];

      } /* loop over materials */
   } /* loop over lights */

   /* Determine if the fast lighting function can be used */
   ctx->Light.Fast = GL_TRUE;
   if (    ctx->Light.BaseColor[0][0]<0.0F
        || ctx->Light.BaseColor[0][1]<0.0F
        || ctx->Light.BaseColor[0][2]<0.0F
        || ctx->Light.BaseColor[0][3]<0.0F
        || ctx->Light.BaseColor[1][0]<0.0F
        || ctx->Light.BaseColor[1][1]<0.0F
        || ctx->Light.BaseColor[1][2]<0.0F
        || ctx->Light.BaseColor[1][3]<0.0F
        || ctx->Light.Model.LocalViewer
        || ctx->Light.ColorMaterialEnabled) {
      ctx->Light.Fast = GL_FALSE;
   }
   else {
      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) {
         if (   light->Position[3]!=0.0F
             || light->SpotCutoff!=180.0F
             || light->MatDiffuse[0][0]<0.0F
             || light->MatDiffuse[0][1]<0.0F
             || light->MatDiffuse[0][2]<0.0F
             || light->MatSpecular[0][0]<0.0F
             || light->MatSpecular[0][1]<0.0F
             || light->MatSpecular[0][2]<0.0F
             || light->MatDiffuse[1][0]<0.0F
             || light->MatDiffuse[1][1]<0.0F
             || light->MatDiffuse[1][2]<0.0F
             || light->MatSpecular[1][0]<0.0F
             || light->MatSpecular[1][1]<0.0F
             || light->MatSpecular[1][2]<0.0F) {
            ctx->Light.Fast = GL_FALSE;
            break;
         }
      }
   }
}
