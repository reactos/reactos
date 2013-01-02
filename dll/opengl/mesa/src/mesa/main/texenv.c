/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file texenv.c
 *
 * glTexEnv-related functions
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/state.h"
#include "main/texenv.h"
#include "main/texstate.h"


#define TE_ERROR(errCode, msg, value)				\
   _mesa_error(ctx, errCode, msg, _mesa_lookup_enum_by_nr(value));


/** Set texture env mode */
static void
set_env_mode(struct gl_context *ctx,
             struct gl_texture_unit *texUnit,
             GLenum mode)
{
   GLboolean legal;

   if (texUnit->EnvMode == mode)
      return;

   switch (mode) {
   case GL_MODULATE:
   case GL_BLEND:
   case GL_DECAL:
   case GL_REPLACE:
   case GL_ADD:
   case GL_COMBINE:
      legal = GL_TRUE;
      break;
   case GL_REPLACE_EXT:
      mode = GL_REPLACE; /* GL_REPLACE_EXT != GL_REPLACE */
      legal = GL_TRUE;
      break;
   case GL_COMBINE4_NV:
      legal = ctx->Extensions.NV_texture_env_combine4;
      break;
   default:
      legal = GL_FALSE;
   }

   if (legal) {
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      texUnit->EnvMode = mode;
   }
   else {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
   }
}


static void
set_env_color(struct gl_context *ctx,
              struct gl_texture_unit *texUnit,
              const GLfloat *color)
{
   if (TEST_EQ_4V(color, texUnit->EnvColorUnclamped))
      return;
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   COPY_4FV(texUnit->EnvColorUnclamped, color);
   texUnit->EnvColor[0] = CLAMP(color[0], 0.0F, 1.0F);
   texUnit->EnvColor[1] = CLAMP(color[1], 0.0F, 1.0F);
   texUnit->EnvColor[2] = CLAMP(color[2], 0.0F, 1.0F);
   texUnit->EnvColor[3] = CLAMP(color[3], 0.0F, 1.0F);
}


/** Set an RGB or A combiner mode/function */
static void
set_combiner_mode(struct gl_context *ctx,
                  struct gl_texture_unit *texUnit,
                  GLenum pname, GLenum mode)
{
   GLboolean legal;

   switch (mode) {
   case GL_REPLACE:
   case GL_MODULATE:
   case GL_ADD:
   case GL_ADD_SIGNED:
   case GL_INTERPOLATE:
      legal = GL_TRUE;
      break;
   case GL_SUBTRACT:
      legal = ctx->Extensions.ARB_texture_env_combine;
      break;
   case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGBA_EXT:
      legal = (ctx->Extensions.EXT_texture_env_dot3 &&
               pname == GL_COMBINE_RGB);
      break;
   case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
      legal = (ctx->Extensions.ARB_texture_env_dot3 &&
               pname == GL_COMBINE_RGB);
      break;
   case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
   case GL_MODULATE_SUBTRACT_ATI:
      legal = ctx->Extensions.ATI_texture_env_combine3;
      break;
   case GL_BUMP_ENVMAP_ATI:
      legal = (ctx->Extensions.ATI_envmap_bumpmap &&
               pname == GL_COMBINE_RGB);
      break;
   default:
      legal = GL_FALSE;
   }

   if (!legal) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
      return;
   }

   switch (pname) {
   case GL_COMBINE_RGB:
      if (texUnit->Combine.ModeRGB == mode)
         return;
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      texUnit->Combine.ModeRGB = mode;
      break;

   case GL_COMBINE_ALPHA:
      if (texUnit->Combine.ModeA == mode)
         return;
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      texUnit->Combine.ModeA = mode;
      break;
   default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
   }
}



/** Set an RGB or A combiner source term */
static void
set_combiner_source(struct gl_context *ctx,
                    struct gl_texture_unit *texUnit,
                    GLenum pname, GLenum param)
{
   GLuint term;
   GLboolean alpha, legal;

   /*
    * Translate pname to (term, alpha).
    *
    * The enums were given sequential values for a reason.
    */
   switch (pname) {
   case GL_SOURCE0_RGB:
   case GL_SOURCE1_RGB:
   case GL_SOURCE2_RGB:
   case GL_SOURCE3_RGB_NV:
      term = pname - GL_SOURCE0_RGB;
      alpha = GL_FALSE;
      break;
   case GL_SOURCE0_ALPHA:
   case GL_SOURCE1_ALPHA:
   case GL_SOURCE2_ALPHA:
   case GL_SOURCE3_ALPHA_NV:
      term = pname - GL_SOURCE0_ALPHA;
      alpha = GL_TRUE;
      break;
   default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
      return;
   }

   if ((term == 3) && !ctx->Extensions.NV_texture_env_combine4) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
      return;
   }

   assert(term < MAX_COMBINER_TERMS);

   /*
    * Error-check param (the source term)
    */
   switch (param) {
   case GL_TEXTURE:
   case GL_CONSTANT:
   case GL_PRIMARY_COLOR:
   case GL_PREVIOUS:
      legal = GL_TRUE;
      break;
   case GL_TEXTURE0:
   case GL_TEXTURE1:
   case GL_TEXTURE2:
   case GL_TEXTURE3:
   case GL_TEXTURE4:
   case GL_TEXTURE5:
   case GL_TEXTURE6:
   case GL_TEXTURE7:
      legal = (ctx->Extensions.ARB_texture_env_crossbar &&
               param - GL_TEXTURE0 < ctx->Const.MaxTextureUnits);
      break;
   case GL_ZERO:
      legal = (ctx->Extensions.ATI_texture_env_combine3 ||
               ctx->Extensions.NV_texture_env_combine4);
      break;
   case GL_ONE:
      legal = ctx->Extensions.ATI_texture_env_combine3;
      break;
   default:
      legal = GL_FALSE;
   }

   if (!legal) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", param);
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   if (alpha)
      texUnit->Combine.SourceA[term] = param;
   else
      texUnit->Combine.SourceRGB[term] = param;
}


/** Set an RGB or A combiner operand term */
static void
set_combiner_operand(struct gl_context *ctx,
                     struct gl_texture_unit *texUnit,
                     GLenum pname, GLenum param)
{
   GLuint term;
   GLboolean alpha, legal;

   /* The enums were given sequential values for a reason.
    */
   switch (pname) {
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND3_RGB_NV:
      term = pname - GL_OPERAND0_RGB;
      alpha = GL_FALSE;
      break;
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
   case GL_OPERAND3_ALPHA_NV:
      term = pname - GL_OPERAND0_ALPHA;
      alpha = GL_TRUE;
      break;
   default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
      return;
   }

   if ((term == 3) && !ctx->Extensions.NV_texture_env_combine4) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
      return;
   }

   assert(term < MAX_COMBINER_TERMS);

   /*
    * Error-check param (the source operand)
    */
   switch (param) {
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
      /* The color input can only be used with GL_OPERAND[01]_RGB in the EXT
       * version.  In the ARB and NV versions they can be used for any RGB
       * operand.
       */
      legal = !alpha
	 && ((term < 2) || ctx->Extensions.ARB_texture_env_combine
	     || ctx->Extensions.NV_texture_env_combine4);
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      /* GL_ONE_MINUS_SRC_ALPHA can only be used with
       * GL_OPERAND[01]_(RGB|ALPHA) in the EXT version.  In the ARB and NV
       * versions it can be used for any operand.
       */
      legal = (term < 2) || ctx->Extensions.ARB_texture_env_combine
	 || ctx->Extensions.NV_texture_env_combine4;
      break;
   case GL_SRC_ALPHA:
      legal = GL_TRUE;
      break;
   default:
      legal = GL_FALSE;
   }

   if (!legal) {
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", param);
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   if (alpha)
      texUnit->Combine.OperandA[term] = param;
   else
      texUnit->Combine.OperandRGB[term] = param;
}


static void
set_combiner_scale(struct gl_context *ctx,
                   struct gl_texture_unit *texUnit,
                   GLenum pname, GLfloat scale)
{
   GLuint shift;

   if (scale == 1.0F) {
      shift = 0;
   }
   else if (scale == 2.0F) {
      shift = 1;
   }
   else if (scale == 4.0F) {
      shift = 2;
   }
   else {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glTexEnv(GL_RGB_SCALE not 1, 2 or 4)" );
      return;
   }

   switch (pname) {
   case GL_RGB_SCALE:
      if (texUnit->Combine.ScaleShiftRGB == shift)
         return;
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      texUnit->Combine.ScaleShiftRGB = shift;
      break;
   case GL_ALPHA_SCALE:
      if (texUnit->Combine.ScaleShiftA == shift)
         return;
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      texUnit->Combine.ScaleShiftA = shift;
      break;
   default:
      TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
   }
}



void GLAPIENTRY
_mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
{
   const GLint iparam0 = (GLint) param[0];
   struct gl_texture_unit *texUnit;
   GLuint maxUnit;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxCombinedTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexEnvfv(current unit)");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         set_env_mode(ctx, texUnit, (GLenum) iparam0);
         break;
      case GL_TEXTURE_ENV_COLOR:
         set_env_color(ctx, texUnit, param);
         break;
      case GL_COMBINE_RGB:
      case GL_COMBINE_ALPHA:
         set_combiner_mode(ctx, texUnit, pname, (GLenum) iparam0);
	 break;
      case GL_SOURCE0_RGB:
      case GL_SOURCE1_RGB:
      case GL_SOURCE2_RGB:
      case GL_SOURCE3_RGB_NV:
      case GL_SOURCE0_ALPHA:
      case GL_SOURCE1_ALPHA:
      case GL_SOURCE2_ALPHA:
      case GL_SOURCE3_ALPHA_NV:
         set_combiner_source(ctx, texUnit, pname, (GLenum) iparam0);
	 break;
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
      case GL_OPERAND2_RGB:
      case GL_OPERAND3_RGB_NV:
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
      case GL_OPERAND2_ALPHA:
      case GL_OPERAND3_ALPHA_NV:
         set_combiner_operand(ctx, texUnit, pname, (GLenum) iparam0);
	 break;
      case GL_RGB_SCALE:
      case GL_ALPHA_SCALE:
         set_combiner_scale(ctx, texUnit, pname, param[0]);
	 break;
      case GL_BUMP_TARGET_ATI:
         if (!ctx->Extensions.ATI_envmap_bumpmap) {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname=0x%x)", pname );
	    return;
	 }
	 if ((iparam0 < GL_TEXTURE0) ||
             (iparam0 > GL_TEXTURE31)) {
	    /* spec doesn't say this but it seems logical */
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(param=0x%x)", iparam0);
	    return;
	 }
	 if (!((1 << (iparam0 - GL_TEXTURE0)) & ctx->Const.SupportedBumpUnits)) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glTexEnv(param=0x%x)", iparam0);
	    return;
	 }
	 else {
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->BumpTarget = iparam0;
	 }
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
	 return;
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
	 if (texUnit->LodBias == param[0])
	    return;
	 FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texUnit->LodBias = param[0];
      }
      else {
         TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	 return;
      }
   }
   else if (target == GL_POINT_SPRITE_NV) {
      /* GL_ARB_point_sprite / GL_NV_point_sprite */
      if (!ctx->Extensions.NV_point_sprite
	  && !ctx->Extensions.ARB_point_sprite) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(target=0x%x)", target );
	 return;
      }
      if (pname == GL_COORD_REPLACE_NV) {
         if (iparam0 == GL_TRUE || iparam0 == GL_FALSE) {
            /* It's kind of weird to set point state via glTexEnv,
             * but that's what the spec calls for.
             */
            const GLboolean state = (GLboolean) iparam0;
            if (ctx->Point.CoordReplace[ctx->Texture.CurrentUnit] == state)
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.CoordReplace[ctx->Texture.CurrentUnit] = state;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexEnv(param=0x%x)", iparam0);
            return;
         }
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname=0x%x)", pname );
         return;
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(target=0x%x)",target );
      return;
   }

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexEnv %s %s %.1f(%s) ...\n",
                  _mesa_lookup_enum_by_nr(target),
                  _mesa_lookup_enum_by_nr(pname),
                  *param,
                  _mesa_lookup_enum_by_nr((GLenum) iparam0));

   /* Tell device driver about the new texture environment */
   if (ctx->Driver.TexEnv) {
      (*ctx->Driver.TexEnv)( ctx, target, pname, param );
   }
}


void GLAPIENTRY
_mesa_TexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0;
   _mesa_TexEnvfv( target, pname, p );
}



void GLAPIENTRY
_mesa_TexEnvi( GLenum target, GLenum pname, GLint param )
{
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
   _mesa_TexEnvfv( target, pname, p );
}


void GLAPIENTRY
_mesa_TexEnviv( GLenum target, GLenum pname, const GLint *param )
{
   GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT( param[0] );
      p[1] = INT_TO_FLOAT( param[1] );
      p[2] = INT_TO_FLOAT( param[2] );
      p[3] = INT_TO_FLOAT( param[3] );
   }
   else {
      p[0] = (GLfloat) param[0];
      p[1] = p[2] = p[3] = 0;  /* init to zero, just to be safe */
   }
   _mesa_TexEnvfv( target, pname, p );
}



/**
 * Helper for glGetTexEnvi/f()
 * \return  value of queried pname or -1 if error.
 */
static GLint
get_texenvi(struct gl_context *ctx, const struct gl_texture_unit *texUnit,
            GLenum pname)
{
   switch (pname) {
   case GL_TEXTURE_ENV_MODE:
      return texUnit->EnvMode;
      break;
   case GL_COMBINE_RGB:
      return texUnit->Combine.ModeRGB;
   case GL_COMBINE_ALPHA:
      return texUnit->Combine.ModeA;
   case GL_SOURCE0_RGB:
   case GL_SOURCE1_RGB:
   case GL_SOURCE2_RGB: {
      const unsigned rgb_idx = pname - GL_SOURCE0_RGB;
      return texUnit->Combine.SourceRGB[rgb_idx];
   }
   case GL_SOURCE3_RGB_NV:
      if (ctx->Extensions.NV_texture_env_combine4) {
         return texUnit->Combine.SourceRGB[3];
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
      }
      break;
   case GL_SOURCE0_ALPHA:
   case GL_SOURCE1_ALPHA:
   case GL_SOURCE2_ALPHA: {
      const unsigned alpha_idx = pname - GL_SOURCE0_ALPHA;
      return texUnit->Combine.SourceA[alpha_idx];
   }
   case GL_SOURCE3_ALPHA_NV:
      if (ctx->Extensions.NV_texture_env_combine4) {
         return texUnit->Combine.SourceA[3];
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
      }
      break;
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB: {
      const unsigned op_rgb = pname - GL_OPERAND0_RGB;
      return texUnit->Combine.OperandRGB[op_rgb];
   }
   case GL_OPERAND3_RGB_NV:
      if (ctx->Extensions.NV_texture_env_combine4) {
         return texUnit->Combine.OperandRGB[3];
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
      }
      break;
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA: {
      const unsigned op_alpha = pname - GL_OPERAND0_ALPHA;
      return texUnit->Combine.OperandA[op_alpha];
   }
   case GL_OPERAND3_ALPHA_NV:
      if (ctx->Extensions.NV_texture_env_combine4) {
         return texUnit->Combine.OperandA[3];
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
      }
      break;
   case GL_RGB_SCALE:
      return 1 << texUnit->Combine.ScaleShiftRGB;
   case GL_ALPHA_SCALE:
      return 1 << texUnit->Combine.ScaleShiftA;
   case GL_BUMP_TARGET_ATI:
      /* spec doesn't say so, but I think this should be queryable */
      if (ctx->Extensions.ATI_envmap_bumpmap) {
         return texUnit->BumpTarget;
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
      }
      break;

   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
      break;
   }

   return -1; /* error */
}



void GLAPIENTRY
_mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
   GLuint maxUnit;
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxCombinedTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnvfv(current unit)");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   if (target == GL_TEXTURE_ENV) {
      if (pname == GL_TEXTURE_ENV_COLOR) {
         if(ctx->NewState & (_NEW_BUFFERS | _NEW_FRAG_CLAMP))
            _mesa_update_state(ctx);
         if(ctx->Color._ClampFragmentColor)
            COPY_4FV( params, texUnit->EnvColor );
         else
            COPY_4FV( params, texUnit->EnvColorUnclamped );
      }
      else {
         GLint val = get_texenvi(ctx, texUnit, pname);
         if (val >= 0) {
            *params = (GLfloat) val;
         }
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
         *params = texUnit->LodBias;
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
	 return;
      }
   }
   else if (target == GL_POINT_SPRITE_NV) {
      /* GL_ARB_point_sprite / GL_NV_point_sprite */
      if (!ctx->Extensions.NV_point_sprite
	  && !ctx->Extensions.ARB_point_sprite) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
         return;
      }
      if (pname == GL_COORD_REPLACE_NV) {
         *params = (GLfloat) ctx->Point.CoordReplace[ctx->Texture.CurrentUnit];
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
         return;
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
      return;
   }
}


void GLAPIENTRY
_mesa_GetTexEnviv( GLenum target, GLenum pname, GLint *params )
{
   GLuint maxUnit;
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxCombinedTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnviv(current unit)");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   if (target == GL_TEXTURE_ENV) {
      if (pname == GL_TEXTURE_ENV_COLOR) {
         params[0] = FLOAT_TO_INT( texUnit->EnvColor[0] );
         params[1] = FLOAT_TO_INT( texUnit->EnvColor[1] );
         params[2] = FLOAT_TO_INT( texUnit->EnvColor[2] );
         params[3] = FLOAT_TO_INT( texUnit->EnvColor[3] );
      }
      else {
         GLint val = get_texenvi(ctx, texUnit, pname);
         if (val >= 0) {
            *params = val;
         }
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
         *params = (GLint) texUnit->LodBias;
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
	 return;
      }
   }
   else if (target == GL_POINT_SPRITE_NV) {
      /* GL_ARB_point_sprite / GL_NV_point_sprite */
      if (!ctx->Extensions.NV_point_sprite
	  && !ctx->Extensions.ARB_point_sprite) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
         return;
      }
      if (pname == GL_COORD_REPLACE_NV) {
         *params = (GLint) ctx->Point.CoordReplace[ctx->Texture.CurrentUnit];
      }
      else {
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
         return;
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
      return;
   }
}


/**
 * Why does ATI_envmap_bumpmap require new entrypoints? Should just
 * reuse TexEnv ones...
 */
void GLAPIENTRY
_mesa_TexBumpParameterivATI( GLenum pname, const GLint *param )
{
   GLfloat p[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ATI_envmap_bumpmap) {
      /* This isn't an "official" error case, but let's tell the user
       * that something's wrong.
       */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBumpParameterivATI");
      return;
   }

   if (pname == GL_BUMP_ROT_MATRIX_ATI) {
      /* hope that conversion is correct here */
      p[0] = INT_TO_FLOAT( param[0] );
      p[1] = INT_TO_FLOAT( param[1] );
      p[2] = INT_TO_FLOAT( param[2] );
      p[3] = INT_TO_FLOAT( param[3] );
   }
   else {
      p[0] = (GLfloat) param[0];
      p[1] = p[2] = p[3] = 0.0F;  /* init to zero, just to be safe */
   }
   _mesa_TexBumpParameterfvATI( pname, p );
}


void GLAPIENTRY
_mesa_TexBumpParameterfvATI( GLenum pname, const GLfloat *param )
{
   struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ATI_envmap_bumpmap) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBumpParameterfvATI");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   if (pname == GL_BUMP_ROT_MATRIX_ATI) {
      if (TEST_EQ_4V(param, texUnit->RotMatrix))
         return;
      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
      COPY_4FV(texUnit->RotMatrix, param);
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexBumpParameter(pname)" );
      return;
   }
   /* Drivers might want to know about this, instead of dedicated function
      just shove it into TexEnv where it really belongs anyway */
   if (ctx->Driver.TexEnv) {
      (*ctx->Driver.TexEnv)( ctx, 0, pname, param );
   }
}


void GLAPIENTRY
_mesa_GetTexBumpParameterivATI( GLenum pname, GLint *param )
{
   const struct gl_texture_unit *texUnit;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ATI_envmap_bumpmap) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexBumpParameterivATI");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   if (pname == GL_BUMP_ROT_MATRIX_SIZE_ATI) {
      /* spec leaves open to support larger matrices.
         Don't think anyone would ever want to use it
         (and apps almost certainly would not understand it and
         thus fail to submit matrices correctly) so hardcode this. */
      *param = 4;
   }
   else if (pname == GL_BUMP_ROT_MATRIX_ATI) {
      /* hope that conversion is correct here */
      param[0] = FLOAT_TO_INT(texUnit->RotMatrix[0]);
      param[1] = FLOAT_TO_INT(texUnit->RotMatrix[1]);
      param[2] = FLOAT_TO_INT(texUnit->RotMatrix[2]);
      param[3] = FLOAT_TO_INT(texUnit->RotMatrix[3]);
   }
   else if (pname == GL_BUMP_NUM_TEX_UNITS_ATI) {
      GLint count = 0;
      for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
         if (ctx->Const.SupportedBumpUnits & (1 << i)) {
            count++;
         }
      }
      *param = count;
   }
   else if (pname == GL_BUMP_TEX_UNITS_ATI) {
      for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
         if (ctx->Const.SupportedBumpUnits & (1 << i)) {
            *param++ = i + GL_TEXTURE0;
         }
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexBumpParameter(pname)" );
      return;
   }
}


void GLAPIENTRY
_mesa_GetTexBumpParameterfvATI( GLenum pname, GLfloat *param )
{
   const struct gl_texture_unit *texUnit;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ATI_envmap_bumpmap) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexBumpParameterfvATI");
      return;
   }

   texUnit = _mesa_get_current_tex_unit(ctx);

   if (pname == GL_BUMP_ROT_MATRIX_SIZE_ATI) {
      /* spec leaves open to support larger matrices.
         Don't think anyone would ever want to use it
         (and apps might not understand it) so hardcode this. */
      *param = 4.0F;
   }
   else if (pname == GL_BUMP_ROT_MATRIX_ATI) {
      param[0] = texUnit->RotMatrix[0];
      param[1] = texUnit->RotMatrix[1];
      param[2] = texUnit->RotMatrix[2];
      param[3] = texUnit->RotMatrix[3];
   }
   else if (pname == GL_BUMP_NUM_TEX_UNITS_ATI) {
      GLint count = 0;
      for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
         if (ctx->Const.SupportedBumpUnits & (1 << i)) {
            count++;
         }
      }
      *param = (GLfloat) count;
   }
   else if (pname == GL_BUMP_TEX_UNITS_ATI) {
      for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
         if (ctx->Const.SupportedBumpUnits & (1 << i)) {
            *param++ = (GLfloat) (i + GL_TEXTURE0);
         }
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexBumpParameter(pname)" );
      return;
   }
}
