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

/**
 * \file prog_statevars.c
 * Program state variable management.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "prog_statevars.h"
#include "prog_parameter.h"
#include "nvvertparse.h"


/**
 * Use the list of tokens in the state[] array to find global GL state
 * and return it in <value>.  Usually, four values are returned in <value>
 * but matrix queries may return as many as 16 values.
 * This function is used for ARB vertex/fragment programs.
 * The program parser will produce the state[] values.
 */
static void
_mesa_fetch_state(GLcontext *ctx, const gl_state_index state[],
                  GLfloat *value)
{
   switch (state[0]) {
   case STATE_MATERIAL:
      {
         /* state[1] is either 0=front or 1=back side */
         const GLuint face = (GLuint) state[1];
         const struct gl_material *mat = &ctx->Light.Material;
         ASSERT(face == 0 || face == 1);
         /* we rely on tokens numbered so that _BACK_ == _FRONT_+ 1 */
         ASSERT(MAT_ATTRIB_FRONT_AMBIENT + 1 == MAT_ATTRIB_BACK_AMBIENT);
         /* XXX we could get rid of this switch entirely with a little
          * work in arbprogparse.c's parse_state_single_item().
          */
         /* state[2] is the material attribute */
         switch (state[2]) {
         case STATE_AMBIENT:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_AMBIENT + face]);
            return;
         case STATE_DIFFUSE:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_DIFFUSE + face]);
            return;
         case STATE_SPECULAR:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_SPECULAR + face]);
            return;
         case STATE_EMISSION:
            COPY_4V(value, mat->Attrib[MAT_ATTRIB_FRONT_EMISSION + face]);
            return;
         case STATE_SHININESS:
            value[0] = mat->Attrib[MAT_ATTRIB_FRONT_SHININESS + face][0];
            value[1] = 0.0F;
            value[2] = 0.0F;
            value[3] = 1.0F;
            return;
         default:
            _mesa_problem(ctx, "Invalid material state in fetch_state");
            return;
         }
      }
   case STATE_LIGHT:
      {
         /* state[1] is the light number */
         const GLuint ln = (GLuint) state[1];
         /* state[2] is the light attribute */
         switch (state[2]) {
         case STATE_AMBIENT:
            COPY_4V(value, ctx->Light.Light[ln].Ambient);
            return;
         case STATE_DIFFUSE:
            COPY_4V(value, ctx->Light.Light[ln].Diffuse);
            return;
         case STATE_SPECULAR:
            COPY_4V(value, ctx->Light.Light[ln].Specular);
            return;
         case STATE_POSITION:
            COPY_4V(value, ctx->Light.Light[ln].EyePosition);
            return;
         case STATE_ATTENUATION:
            value[0] = ctx->Light.Light[ln].ConstantAttenuation;
            value[1] = ctx->Light.Light[ln].LinearAttenuation;
            value[2] = ctx->Light.Light[ln].QuadraticAttenuation;
            value[3] = ctx->Light.Light[ln].SpotExponent;
            return;
         case STATE_SPOT_DIRECTION:
            COPY_3V(value, ctx->Light.Light[ln].EyeDirection);
            value[3] = ctx->Light.Light[ln]._CosCutoff;
            return;
         case STATE_SPOT_CUTOFF:
            value[0] = ctx->Light.Light[ln].SpotCutoff;
            return;
         case STATE_HALF_VECTOR:
            {
               static const GLfloat eye_z[] = {0, 0, 1};
               GLfloat p[3];
               /* Compute infinite half angle vector:
                *   halfVector = normalize(normalize(lightPos) + (0, 0, 1))
		* light.EyePosition.w should be 0 for infinite lights.
                */
               COPY_3V(p, ctx->Light.Light[ln].EyePosition);
               NORMALIZE_3FV(p);
	       ADD_3V(value, p, eye_z);
	       NORMALIZE_3FV(value);
	       value[3] = 1.0;
            }
            return;
	 case STATE_POSITION_NORMALIZED:
            COPY_4V(value, ctx->Light.Light[ln].EyePosition);
	    NORMALIZE_3FV( value );
            return;
         default:
            _mesa_problem(ctx, "Invalid light state in fetch_state");
            return;
         }
      }
   case STATE_LIGHTMODEL_AMBIENT:
      COPY_4V(value, ctx->Light.Model.Ambient);
      return;
   case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
         /* front */
         GLint i;
         for (i = 0; i < 3; i++) {
            value[i] = ctx->Light.Model.Ambient[i]
               * ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT][i]
               + ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION][i];
         }
	 value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE][3];
      }
      else {
         /* back */
         GLint i;
         for (i = 0; i < 3; i++) {
            value[i] = ctx->Light.Model.Ambient[i]
               * ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_AMBIENT][i]
               + ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_EMISSION][i];
         }
	 value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_BACK_DIFFUSE][3];
      }
      return;
   case STATE_LIGHTPROD:
      {
         const GLuint ln = (GLuint) state[1];
         const GLuint face = (GLuint) state[2];
         GLint i;
         ASSERT(face == 0 || face == 1);
         switch (state[3]) {
            case STATE_AMBIENT:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Ambient[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT+face][3];
               return;
            case STATE_DIFFUSE:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Diffuse[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_DIFFUSE+face][3];
               return;
            case STATE_SPECULAR:
               for (i = 0; i < 3; i++) {
                  value[i] = ctx->Light.Light[ln].Specular[i] *
                     ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SPECULAR+face][i];
               }
               /* [3] = material alpha */
               value[3] = ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SPECULAR+face][3];
               return;
            default:
               _mesa_problem(ctx, "Invalid lightprod state in fetch_state");
               return;
         }
      }
   case STATE_TEXGEN:
      {
         /* state[1] is the texture unit */
         const GLuint unit = (GLuint) state[1];
         /* state[2] is the texgen attribute */
         switch (state[2]) {
         case STATE_TEXGEN_EYE_S:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneS);
            return;
         case STATE_TEXGEN_EYE_T:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneT);
            return;
         case STATE_TEXGEN_EYE_R:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneR);
            return;
         case STATE_TEXGEN_EYE_Q:
            COPY_4V(value, ctx->Texture.Unit[unit].EyePlaneQ);
            return;
         case STATE_TEXGEN_OBJECT_S:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneS);
            return;
         case STATE_TEXGEN_OBJECT_T:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneT);
            return;
         case STATE_TEXGEN_OBJECT_R:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneR);
            return;
         case STATE_TEXGEN_OBJECT_Q:
            COPY_4V(value, ctx->Texture.Unit[unit].ObjectPlaneQ);
            return;
         default:
            _mesa_problem(ctx, "Invalid texgen state in fetch_state");
            return;
         }
      }
   case STATE_TEXENV_COLOR:
      {
         /* state[1] is the texture unit */
         const GLuint unit = (GLuint) state[1];
         COPY_4V(value, ctx->Texture.Unit[unit].EnvColor);
      }
      return;
   case STATE_FOG_COLOR:
      COPY_4V(value, ctx->Fog.Color);
      return;
   case STATE_FOG_PARAMS:
      value[0] = ctx->Fog.Density;
      value[1] = ctx->Fog.Start;
      value[2] = ctx->Fog.End;
      value[3] = (ctx->Fog.End == ctx->Fog.Start)
         ? 1.0f : (GLfloat)(1.0 / (ctx->Fog.End - ctx->Fog.Start));
      return;
   case STATE_CLIPPLANE:
      {
         const GLuint plane = (GLuint) state[1];
         COPY_4V(value, ctx->Transform.EyeUserPlane[plane]);
      }
      return;
   case STATE_POINT_SIZE:
      value[0] = ctx->Point.Size;
      value[1] = ctx->Point.MinSize;
      value[2] = ctx->Point.MaxSize;
      value[3] = ctx->Point.Threshold;
      return;
   case STATE_POINT_ATTENUATION:
      value[0] = ctx->Point.Params[0];
      value[1] = ctx->Point.Params[1];
      value[2] = ctx->Point.Params[2];
      value[3] = 1.0F;
      return;
   case STATE_MODELVIEW_MATRIX:
   case STATE_PROJECTION_MATRIX:
   case STATE_MVP_MATRIX:
   case STATE_TEXTURE_MATRIX:
   case STATE_PROGRAM_MATRIX:
   case STATE_COLOR_MATRIX:
      {
         /* state[0] = modelview, projection, texture, etc. */
         /* state[1] = which texture matrix or program matrix */
         /* state[2] = first row to fetch */
         /* state[3] = last row to fetch */
         /* state[4] = transpose, inverse or invtrans */
         const GLmatrix *matrix;
         const gl_state_index mat = state[0];
         const GLuint index = (GLuint) state[1];
         const GLuint firstRow = (GLuint) state[2];
         const GLuint lastRow = (GLuint) state[3];
         const gl_state_index modifier = state[4];
         const GLfloat *m;
         GLuint row, i;
         ASSERT(firstRow >= 0);
         ASSERT(firstRow < 4);
         ASSERT(lastRow >= 0);
         ASSERT(lastRow < 4);
         if (mat == STATE_MODELVIEW_MATRIX) {
            matrix = ctx->ModelviewMatrixStack.Top;
         }
         else if (mat == STATE_PROJECTION_MATRIX) {
            matrix = ctx->ProjectionMatrixStack.Top;
         }
         else if (mat == STATE_MVP_MATRIX) {
            matrix = &ctx->_ModelProjectMatrix;
         }
         else if (mat == STATE_TEXTURE_MATRIX) {
            matrix = ctx->TextureMatrixStack[index].Top;
         }
         else if (mat == STATE_PROGRAM_MATRIX) {
            matrix = ctx->ProgramMatrixStack[index].Top;
         }
         else if (mat == STATE_COLOR_MATRIX) {
            matrix = ctx->ColorMatrixStack.Top;
         }
         else {
            _mesa_problem(ctx, "Bad matrix name in _mesa_fetch_state()");
            return;
         }
         if (modifier == STATE_MATRIX_INVERSE ||
             modifier == STATE_MATRIX_INVTRANS) {
            /* Be sure inverse is up to date:
	     */
            _math_matrix_alloc_inv( (GLmatrix *) matrix );
	    _math_matrix_analyse( (GLmatrix*) matrix );
            m = matrix->inv;
         }
         else {
            m = matrix->m;
         }
         if (modifier == STATE_MATRIX_TRANSPOSE ||
             modifier == STATE_MATRIX_INVTRANS) {
            for (i = 0, row = firstRow; row <= lastRow; row++) {
               value[i++] = m[row * 4 + 0];
               value[i++] = m[row * 4 + 1];
               value[i++] = m[row * 4 + 2];
               value[i++] = m[row * 4 + 3];
            }
         }
         else {
            for (i = 0, row = firstRow; row <= lastRow; row++) {
               value[i++] = m[row + 0];
               value[i++] = m[row + 4];
               value[i++] = m[row + 8];
               value[i++] = m[row + 12];
            }
         }
      }
      return;
   case STATE_DEPTH_RANGE:
      value[0] = ctx->Viewport.Near;                     /* near       */
      value[1] = ctx->Viewport.Far;                      /* far        */
      value[2] = ctx->Viewport.Far - ctx->Viewport.Near; /* far - near */
      value[3] = 1.0;
      return;
   case STATE_FRAGMENT_PROGRAM:
      {
         /* state[1] = {STATE_ENV, STATE_LOCAL} */
         /* state[2] = parameter index          */
         const int idx = (int) state[2];
         switch (state[1]) {
            case STATE_ENV:
               COPY_4V(value, ctx->FragmentProgram.Parameters[idx]);
               break;
            case STATE_LOCAL:
               COPY_4V(value, ctx->FragmentProgram.Current->Base.LocalParams[idx]);
               break;
            default:
               _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
               return;
         }
      }
      return;

   case STATE_VERTEX_PROGRAM:
      {
         /* state[1] = {STATE_ENV, STATE_LOCAL} */
         /* state[2] = parameter index          */
         const int idx = (int) state[2];
         switch (state[1]) {
            case STATE_ENV:
               COPY_4V(value, ctx->VertexProgram.Parameters[idx]);
               break;
            case STATE_LOCAL:
               COPY_4V(value, ctx->VertexProgram.Current->Base.LocalParams[idx]);
               break;
            default:
               _mesa_problem(ctx, "Bad state switch in _mesa_fetch_state()");
               return;
         }
      }
      return;

   case STATE_NORMAL_SCALE:
      ASSIGN_4V(value, ctx->_ModelViewInvScale, 0, 0, 1);
      return;

   case STATE_INTERNAL:
      switch (state[1]) {
      case STATE_NORMAL_SCALE:
         ASSIGN_4V(value, ctx->_ModelViewInvScale, 0, 0, 1);
         return;
      case STATE_TEXRECT_SCALE:
         {
            const int unit = (int) state[2];
            const struct gl_texture_object *texObj
               = ctx->Texture.Unit[unit]._Current;
            if (texObj) {
               struct gl_texture_image *texImage = texObj->Image[0][0];
               ASSIGN_4V(value, (GLfloat) (1.0 / texImage->Width),
                         (GLfloat)(1.0 / texImage->Height),
                         0.0f, 1.0f);
            }
         }
         return;
      case STATE_FOG_PARAMS_OPTIMIZED:
         /* for simpler per-vertex/pixel fog calcs. POW (for EXP/EXP2 fog)
          * might be more expensive than EX2 on some hw, plus it needs
          * another constant (e) anyway. Linear fog can now be done with a
          * single MAD.
          * linear: fogcoord * -1/(end-start) + end/(end-start)
          * exp: 2^-(density/ln(2) * fogcoord)
          * exp2: 2^-((density/(ln(2)^2) * fogcoord)^2)
          */
         value[0] = (ctx->Fog.End == ctx->Fog.Start)
            ? 1.0f : (GLfloat)(-1.0F / (ctx->Fog.End - ctx->Fog.Start));
         value[1] = ctx->Fog.End * -value[0];
         value[2] = ctx->Fog.Density * ONE_DIV_LN2;
         value[3] = ctx->Fog.Density * ONE_DIV_SQRT_LN2;
         return;
      case STATE_SPOT_DIR_NORMALIZED: {
         /* here, state[2] is the light number */
         /* pre-normalize spot dir */
         const GLuint ln = (GLuint) state[2];
         COPY_3V(value, ctx->Light.Light[ln].EyeDirection);
         NORMALIZE_3FV(value);
         value[3] = ctx->Light.Light[ln]._CosCutoff;
         return;
      }
      case STATE_PT_SCALE:
         value[0] = ctx->Pixel.RedScale;
         value[1] = ctx->Pixel.GreenScale;
         value[2] = ctx->Pixel.BlueScale;
         value[3] = ctx->Pixel.AlphaScale;
         break;
      case STATE_PT_BIAS:
         value[0] = ctx->Pixel.RedBias;
         value[1] = ctx->Pixel.GreenBias;
         value[2] = ctx->Pixel.BlueBias;
         value[3] = ctx->Pixel.AlphaBias;
         break;
      case STATE_PCM_SCALE:
         COPY_4V(value, ctx->Pixel.PostColorMatrixScale);
         break;
      case STATE_PCM_BIAS:
         COPY_4V(value, ctx->Pixel.PostColorMatrixBias);
         break;
      case STATE_SHADOW_AMBIENT:
         {
            const int unit = (int) state[2];
            const struct gl_texture_object *texObj
               = ctx->Texture.Unit[unit]._Current;
            if (texObj) {
               value[0] = texObj->ShadowAmbient;
               value[1] = texObj->ShadowAmbient;
               value[2] = texObj->ShadowAmbient;
               value[3] = texObj->ShadowAmbient;
            }
         }
         return;

      default:
         /* unknown state indexes are silently ignored
          *  should be handled by the driver.
          */
         return;
      }
      return;

   default:
      _mesa_problem(ctx, "Invalid state in _mesa_fetch_state");
      return;
   }
}


/**
 * Return a bitmask of the Mesa state flags (_NEW_* values) which would
 * indicate that the given context state may have changed.
 * The bitmask is used during validation to determine if we need to update
 * vertex/fragment program parameters (like "state.material.color") when
 * some GL state has changed.
 */
GLbitfield
_mesa_program_state_flags(const gl_state_index state[STATE_LENGTH])
{
   switch (state[0]) {
   case STATE_MATERIAL:
   case STATE_LIGHT:
   case STATE_LIGHTMODEL_AMBIENT:
   case STATE_LIGHTMODEL_SCENECOLOR:
   case STATE_LIGHTPROD:
      return _NEW_LIGHT;

   case STATE_TEXGEN:
   case STATE_TEXENV_COLOR:
      return _NEW_TEXTURE;

   case STATE_FOG_COLOR:
   case STATE_FOG_PARAMS:
      return _NEW_FOG;

   case STATE_CLIPPLANE:
      return _NEW_TRANSFORM;

   case STATE_POINT_SIZE:
   case STATE_POINT_ATTENUATION:
      return _NEW_POINT;

   case STATE_MODELVIEW_MATRIX:
      return _NEW_MODELVIEW;
   case STATE_PROJECTION_MATRIX:
      return _NEW_PROJECTION;
   case STATE_MVP_MATRIX:
      return _NEW_MODELVIEW | _NEW_PROJECTION;
   case STATE_TEXTURE_MATRIX:
      return _NEW_TEXTURE_MATRIX;
   case STATE_PROGRAM_MATRIX:
      return _NEW_TRACK_MATRIX;
   case STATE_COLOR_MATRIX:
      return _NEW_COLOR_MATRIX;

   case STATE_DEPTH_RANGE:
      return _NEW_VIEWPORT;

   case STATE_FRAGMENT_PROGRAM:
   case STATE_VERTEX_PROGRAM:
      return _NEW_PROGRAM;

   case STATE_NORMAL_SCALE:
      return _NEW_MODELVIEW;

   case STATE_INTERNAL:
      switch (state[1]) {
      case STATE_TEXRECT_SCALE:
      case STATE_SHADOW_AMBIENT:
	 return _NEW_TEXTURE;
      case STATE_FOG_PARAMS_OPTIMIZED:
	 return _NEW_FOG;
      default:
         /* unknown state indexes are silently ignored and
         *  no flag set, since it is handled by the driver.
         */
	 return 0;
      }

   default:
      _mesa_problem(NULL, "unexpected state[0] in make_state_flags()");
      return 0;
   }
}


static void
append(char *dst, const char *src)
{
   while (*dst)
      dst++;
   while (*src)
     *dst++ = *src++;
   *dst = 0;
}


static void
append_token(char *dst, gl_state_index k)
{
   switch (k) {
   case STATE_MATERIAL:
      append(dst, "material");
      break;
   case STATE_LIGHT:
      append(dst, "light");
      break;
   case STATE_LIGHTMODEL_AMBIENT:
      append(dst, "lightmodel.ambient");
      break;
   case STATE_LIGHTMODEL_SCENECOLOR:
      break;
   case STATE_LIGHTPROD:
      append(dst, "lightprod");
      break;
   case STATE_TEXGEN:
      append(dst, "texgen");
      break;
   case STATE_FOG_COLOR:
      append(dst, "fog.color");
      break;
   case STATE_FOG_PARAMS:
      append(dst, "fog.params");
      break;
   case STATE_CLIPPLANE:
      append(dst, "clip");
      break;
   case STATE_POINT_SIZE:
      append(dst, "point.size");
      break;
   case STATE_POINT_ATTENUATION:
      append(dst, "point.attenuation");
      break;
   case STATE_MODELVIEW_MATRIX:
      append(dst, "matrix.modelview");
      break;
   case STATE_PROJECTION_MATRIX:
      append(dst, "matrix.projection");
      break;
   case STATE_MVP_MATRIX:
      append(dst, "matrix.mvp");
      break;
   case STATE_TEXTURE_MATRIX:
      append(dst, "matrix.texture");
      break;
   case STATE_PROGRAM_MATRIX:
      append(dst, "matrix.program");
      break;
   case STATE_COLOR_MATRIX:
      append(dst, "matrix.color");
      break;
   case STATE_MATRIX_INVERSE:
      append(dst, ".inverse");
      break;
   case STATE_MATRIX_TRANSPOSE:
      append(dst, ".transpose");
      break;
   case STATE_MATRIX_INVTRANS:
      append(dst, ".invtrans");
      break;
   case STATE_AMBIENT:
      append(dst, ".ambient");
      break;
   case STATE_DIFFUSE:
      append(dst, ".diffuse");
      break;
   case STATE_SPECULAR:
      append(dst, ".specular");
      break;
   case STATE_EMISSION:
      append(dst, ".emission");
      break;
   case STATE_SHININESS:
      append(dst, "lshininess");
      break;
   case STATE_HALF_VECTOR:
      append(dst, ".half");
      break;
   case STATE_POSITION:
      append(dst, ".position");
      break;
   case STATE_ATTENUATION:
      append(dst, ".attenuation");
      break;
   case STATE_SPOT_DIRECTION:
      append(dst, ".spot.direction");
      break;
   case STATE_SPOT_CUTOFF:
      append(dst, ".spot.cutoff");
      break;
   case STATE_TEXGEN_EYE_S:
      append(dst, "eye.s");
      break;
   case STATE_TEXGEN_EYE_T:
      append(dst, "eye.t");
      break;
   case STATE_TEXGEN_EYE_R:
      append(dst, "eye.r");
      break;
   case STATE_TEXGEN_EYE_Q:
      append(dst, "eye.q");
      break;
   case STATE_TEXGEN_OBJECT_S:
      append(dst, "object.s");
      break;
   case STATE_TEXGEN_OBJECT_T:
      append(dst, "object.t");
      break;
   case STATE_TEXGEN_OBJECT_R:
      append(dst, "object.r");
      break;
   case STATE_TEXGEN_OBJECT_Q:
      append(dst, "object.q");
      break;
   case STATE_TEXENV_COLOR:
      append(dst, "texenv");
      break;
   case STATE_DEPTH_RANGE:
      append(dst, "depth.range");
      break;
   case STATE_VERTEX_PROGRAM:
   case STATE_FRAGMENT_PROGRAM:
      break;
   case STATE_ENV:
      append(dst, "env");
      break;
   case STATE_LOCAL:
      append(dst, "local");
      break;
   case STATE_NORMAL_SCALE:
      append(dst, "normalScale");
      break;
   case STATE_INTERNAL:
   case STATE_POSITION_NORMALIZED:
      append(dst, "(internal)");
      break;
   case STATE_PT_SCALE:
      append(dst, "PTscale");
      break;
   case STATE_PT_BIAS:
      append(dst, "PTbias");
      break;
   case STATE_PCM_SCALE:
      append(dst, "PCMscale");
      break;
   case STATE_PCM_BIAS:
      append(dst, "PCMbias");
      break;
   case STATE_SHADOW_AMBIENT:
      append(dst, "ShadowAmbient");
      break;
   default:
      ;
   }
}

static void
append_face(char *dst, GLint face)
{
   if (face == 0)
      append(dst, "front.");
   else
      append(dst, "back.");
}

static void
append_index(char *dst, GLint index)
{
   char s[20];
   _mesa_sprintf(s, "[%d]", index);
   append(dst, s);
}

/**
 * Make a string from the given state vector.
 * For example, return "state.matrix.texture[2].inverse".
 * Use _mesa_free() to deallocate the string.
 */
const char *
_mesa_program_state_string(const gl_state_index state[STATE_LENGTH])
{
   char str[1000] = "";
   char tmp[30];

   append(str, "state.");
   append_token(str, (gl_state_index) state[0]);

   switch (state[0]) {
   case STATE_MATERIAL:
      append_face(str, state[1]);
      append_token(str, (gl_state_index) state[2]);
      break;
   case STATE_LIGHT:
      append_index(str, state[1]); /* light number [i]. */
      append_token(str, (gl_state_index) state[2]); /* coefficients */
      break;
   case STATE_LIGHTMODEL_AMBIENT:
      append(str, "lightmodel.ambient");
      break;
   case STATE_LIGHTMODEL_SCENECOLOR:
      if (state[1] == 0) {
         append(str, "lightmodel.front.scenecolor");
      }
      else {
         append(str, "lightmodel.back.scenecolor");
      }
      break;
   case STATE_LIGHTPROD:
      append_index(str, state[1]); /* light number [i]. */
      append_face(str, state[2]);
      append_token(str, (gl_state_index) state[3]);
      break;
   case STATE_TEXGEN:
      append_index(str, state[1]); /* tex unit [i] */
      append_token(str, (gl_state_index) state[2]); /* plane coef */
      break;
   case STATE_TEXENV_COLOR:
      append_index(str, state[1]); /* tex unit [i] */
      append(str, "color");
      break;
   case STATE_CLIPPLANE:
      append_index(str, state[1]); /* plane [i] */
      append(str, ".plane");
      break;
   case STATE_MODELVIEW_MATRIX:
   case STATE_PROJECTION_MATRIX:
   case STATE_MVP_MATRIX:
   case STATE_TEXTURE_MATRIX:
   case STATE_PROGRAM_MATRIX:
   case STATE_COLOR_MATRIX:
      {
         /* state[0] = modelview, projection, texture, etc. */
         /* state[1] = which texture matrix or program matrix */
         /* state[2] = first row to fetch */
         /* state[3] = last row to fetch */
         /* state[4] = transpose, inverse or invtrans */
         const gl_state_index mat = (gl_state_index) state[0];
         const GLuint index = (GLuint) state[1];
         const GLuint firstRow = (GLuint) state[2];
         const GLuint lastRow = (GLuint) state[3];
         const gl_state_index modifier = (gl_state_index) state[4];
         if (index ||
             mat == STATE_TEXTURE_MATRIX ||
             mat == STATE_PROGRAM_MATRIX)
            append_index(str, index);
         if (modifier)
            append_token(str, modifier);
         if (firstRow == lastRow)
            _mesa_sprintf(tmp, ".row[%d]", firstRow);
         else
            _mesa_sprintf(tmp, ".row[%d..%d]", firstRow, lastRow);
         append(str, tmp);
      }
      break;
   case STATE_POINT_SIZE:
      break;
   case STATE_POINT_ATTENUATION:
      break;
   case STATE_FOG_PARAMS:
      break;
   case STATE_FOG_COLOR:
      break;
   case STATE_DEPTH_RANGE:
      break;
   case STATE_FRAGMENT_PROGRAM:
   case STATE_VERTEX_PROGRAM:
      /* state[1] = {STATE_ENV, STATE_LOCAL} */
      /* state[2] = parameter index          */
      append_token(str, (gl_state_index) state[1]);
      append_index(str, state[2]);
      break;
   case STATE_INTERNAL:
      break;
   default:
      _mesa_problem(NULL, "Invalid state in _mesa_program_state_string");
      break;
   }

   return _mesa_strdup(str);
}


/**
 * Loop over all the parameters in a parameter list.  If the parameter
 * is a GL state reference, look up the current value of that state
 * variable and put it into the parameter's Value[4] array.
 * This would be called at glBegin time when using a fragment program.
 */
void
_mesa_load_state_parameters(GLcontext *ctx,
                            struct gl_program_parameter_list *paramList)
{
   GLuint i;

   if (!paramList)
      return;

   /*assert(ctx->Driver.NeedFlush == 0);*/

   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Type == PROGRAM_STATE_VAR) {
         _mesa_fetch_state(ctx,
			   (gl_state_index *) paramList->Parameters[i].StateIndexes,
                           paramList->ParameterValues[i]);
      }
   }
}


/**
 * Copy the 16 elements of a matrix into four consecutive program
 * registers starting at 'pos'.
 */
static void
load_matrix(GLfloat registers[][4], GLuint pos, const GLfloat mat[16])
{
   GLuint i;
   for (i = 0; i < 4; i++) {
      registers[pos + i][0] = mat[0 + i];
      registers[pos + i][1] = mat[4 + i];
      registers[pos + i][2] = mat[8 + i];
      registers[pos + i][3] = mat[12 + i];
   }
}


/**
 * As above, but transpose the matrix.
 */
static void
load_transpose_matrix(GLfloat registers[][4], GLuint pos,
                      const GLfloat mat[16])
{
   MEMCPY(registers[pos], mat, 16 * sizeof(GLfloat));
}


/**
 * Load current vertex program's parameter registers with tracked
 * matrices (if NV program).  This only needs to be done per
 * glBegin/glEnd, not per-vertex.
 */
void
_mesa_load_tracked_matrices(GLcontext *ctx)
{
   GLuint i;

   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
      /* point 'mat' at source matrix */
      GLmatrix *mat;
      if (ctx->VertexProgram.TrackMatrix[i] == GL_MODELVIEW) {
         mat = ctx->ModelviewMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_PROJECTION) {
         mat = ctx->ProjectionMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_TEXTURE) {
         mat = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_COLOR) {
         mat = ctx->ColorMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i]==GL_MODELVIEW_PROJECTION_NV) {
         /* XXX verify the combined matrix is up to date */
         mat = &ctx->_ModelProjectMatrix;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] >= GL_MATRIX0_NV &&
               ctx->VertexProgram.TrackMatrix[i] <= GL_MATRIX7_NV) {
         GLuint n = ctx->VertexProgram.TrackMatrix[i] - GL_MATRIX0_NV;
         ASSERT(n < MAX_PROGRAM_MATRICES);
         mat = ctx->ProgramMatrixStack[n].Top;
      }
      else {
         /* no matrix is tracked, but we leave the register values as-is */
         assert(ctx->VertexProgram.TrackMatrix[i] == GL_NONE);
         continue;
      }

      /* load the matrix values into sequential registers */
      if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_IDENTITY_NV) {
         load_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
      }
      else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_INVERSE_NV) {
         _math_matrix_analyse(mat); /* update the inverse */
         ASSERT(!_math_matrix_is_dirty(mat));
         load_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
      }
      else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_TRANSPOSE_NV) {
         load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
      }
      else {
         assert(ctx->VertexProgram.TrackMatrixTransform[i]
                == GL_INVERSE_TRANSPOSE_NV);
         _math_matrix_analyse(mat); /* update the inverse */
         ASSERT(!_math_matrix_is_dirty(mat));
         load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
      }
   }
}
