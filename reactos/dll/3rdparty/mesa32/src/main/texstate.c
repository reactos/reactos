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
 * \file texstate.c
 *
 * Texture state handling.
 */

#include "glheader.h"
#include "colormac.h"
#include "colortab.h"
#include "context.h"
#include "enums.h"
#include "macros.h"
#include "texcompress.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "texenvprogram.h"
#include "mtypes.h"
#include "math/m_xform.h"


#define ENUM_TO_FLOAT(X) ((GLfloat)(GLint)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(GLint)(X))


/**
 * Default texture combine environment state.  This is used to initialize
 * a context's texture units and as the basis for converting "classic"
 * texture environmnets to ARB_texture_env_combine style values.
 */
static const struct gl_tex_env_combine_state default_combine_state = {
   GL_MODULATE, GL_MODULATE,
   { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT },
   { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT },
   { GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_ALPHA },
   { GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA },
   0, 0,
   2, 2
};



/**
 * Used by glXCopyContext to copy texture state from one context to another.
 */
void
_mesa_copy_texture_state( const GLcontext *src, GLcontext *dst )
{
   GLuint i;

   ASSERT(src);
   ASSERT(dst);

   dst->Texture.CurrentUnit = src->Texture.CurrentUnit;
   dst->Texture._GenFlags = src->Texture._GenFlags;
   dst->Texture._TexGenEnabled = src->Texture._TexGenEnabled;
   dst->Texture._TexMatEnabled = src->Texture._TexMatEnabled;
   dst->Texture.SharedPalette = src->Texture.SharedPalette;

   /* per-unit state */
   for (i = 0; i < src->Const.MaxTextureUnits; i++) {
      dst->Texture.Unit[i].Enabled = src->Texture.Unit[i].Enabled;
      dst->Texture.Unit[i].EnvMode = src->Texture.Unit[i].EnvMode;
      COPY_4V(dst->Texture.Unit[i].EnvColor, src->Texture.Unit[i].EnvColor);
      dst->Texture.Unit[i].TexGenEnabled = src->Texture.Unit[i].TexGenEnabled;
      dst->Texture.Unit[i].GenModeS = src->Texture.Unit[i].GenModeS;
      dst->Texture.Unit[i].GenModeT = src->Texture.Unit[i].GenModeT;
      dst->Texture.Unit[i].GenModeR = src->Texture.Unit[i].GenModeR;
      dst->Texture.Unit[i].GenModeQ = src->Texture.Unit[i].GenModeQ;
      dst->Texture.Unit[i]._GenBitS = src->Texture.Unit[i]._GenBitS;
      dst->Texture.Unit[i]._GenBitT = src->Texture.Unit[i]._GenBitT;
      dst->Texture.Unit[i]._GenBitR = src->Texture.Unit[i]._GenBitR;
      dst->Texture.Unit[i]._GenBitQ = src->Texture.Unit[i]._GenBitQ;
      dst->Texture.Unit[i]._GenFlags = src->Texture.Unit[i]._GenFlags;
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneS, src->Texture.Unit[i].ObjectPlaneS);
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneT, src->Texture.Unit[i].ObjectPlaneT);
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneR, src->Texture.Unit[i].ObjectPlaneR);
      COPY_4V(dst->Texture.Unit[i].ObjectPlaneQ, src->Texture.Unit[i].ObjectPlaneQ);
      COPY_4V(dst->Texture.Unit[i].EyePlaneS, src->Texture.Unit[i].EyePlaneS);
      COPY_4V(dst->Texture.Unit[i].EyePlaneT, src->Texture.Unit[i].EyePlaneT);
      COPY_4V(dst->Texture.Unit[i].EyePlaneR, src->Texture.Unit[i].EyePlaneR);
      COPY_4V(dst->Texture.Unit[i].EyePlaneQ, src->Texture.Unit[i].EyePlaneQ);
      dst->Texture.Unit[i].LodBias = src->Texture.Unit[i].LodBias;

      /* GL_EXT_texture_env_combine */
      dst->Texture.Unit[i].Combine.ModeRGB = src->Texture.Unit[i].Combine.ModeRGB;
      dst->Texture.Unit[i].Combine.ModeA = src->Texture.Unit[i].Combine.ModeA;
      COPY_3V(dst->Texture.Unit[i].Combine.SourceRGB, src->Texture.Unit[i].Combine.SourceRGB);
      COPY_3V(dst->Texture.Unit[i].Combine.SourceA, src->Texture.Unit[i].Combine.SourceA);
      COPY_3V(dst->Texture.Unit[i].Combine.OperandRGB, src->Texture.Unit[i].Combine.OperandRGB);
      COPY_3V(dst->Texture.Unit[i].Combine.OperandA, src->Texture.Unit[i].Combine.OperandA);
      dst->Texture.Unit[i].Combine.ScaleShiftRGB = src->Texture.Unit[i].Combine.ScaleShiftRGB;
      dst->Texture.Unit[i].Combine.ScaleShiftA = src->Texture.Unit[i].Combine.ScaleShiftA;

      /* copy texture object bindings, not contents of texture objects */
      _mesa_lock_context_textures(dst);

      _mesa_reference_texobj(&dst->Texture.Unit[i].Current1D,
                             src->Texture.Unit[i].Current1D);
      _mesa_reference_texobj(&dst->Texture.Unit[i].Current2D,
                             src->Texture.Unit[i].Current2D);
      _mesa_reference_texobj(&dst->Texture.Unit[i].Current3D,
                             src->Texture.Unit[i].Current3D);
      _mesa_reference_texobj(&dst->Texture.Unit[i].CurrentCubeMap,
                             src->Texture.Unit[i].CurrentCubeMap);
      _mesa_reference_texobj(&dst->Texture.Unit[i].CurrentRect,
                             src->Texture.Unit[i].CurrentRect);
      _mesa_reference_texobj(&dst->Texture.Unit[i].Current1DArray,
                             src->Texture.Unit[i].Current1DArray);
      _mesa_reference_texobj(&dst->Texture.Unit[i].Current2DArray,
                             src->Texture.Unit[i].Current2DArray);

      _mesa_unlock_context_textures(dst);
   }
}


/*
 * For debugging
 */
void
_mesa_print_texunit_state( GLcontext *ctx, GLuint unit )
{
   const struct gl_texture_unit *texUnit = ctx->Texture.Unit + unit;
   _mesa_printf("Texture Unit %d\n", unit);
   _mesa_printf("  GL_TEXTURE_ENV_MODE = %s\n", _mesa_lookup_enum_by_nr(texUnit->EnvMode));
   _mesa_printf("  GL_COMBINE_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeRGB));
   _mesa_printf("  GL_COMBINE_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeA));
   _mesa_printf("  GL_SOURCE0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[0]));
   _mesa_printf("  GL_SOURCE1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[1]));
   _mesa_printf("  GL_SOURCE2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[2]));
   _mesa_printf("  GL_SOURCE0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[0]));
   _mesa_printf("  GL_SOURCE1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[1]));
   _mesa_printf("  GL_SOURCE2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[2]));
   _mesa_printf("  GL_OPERAND0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[0]));
   _mesa_printf("  GL_OPERAND1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[1]));
   _mesa_printf("  GL_OPERAND2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[2]));
   _mesa_printf("  GL_OPERAND0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[0]));
   _mesa_printf("  GL_OPERAND1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[1]));
   _mesa_printf("  GL_OPERAND2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[2]));
   _mesa_printf("  GL_RGB_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftRGB);
   _mesa_printf("  GL_ALPHA_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftA);
   _mesa_printf("  GL_TEXTURE_ENV_COLOR = (%f, %f, %f, %f)\n", texUnit->EnvColor[0], texUnit->EnvColor[1], texUnit->EnvColor[2], texUnit->EnvColor[3]);
}



/**********************************************************************/
/*                       Texture Environment                          */
/**********************************************************************/

/**
 * Convert "classic" texture environment to ARB_texture_env_combine style
 * environments.
 * 
 * \param state  texture_env_combine state vector to be filled-in.
 * \param mode   Classic texture environment mode (i.e., \c GL_REPLACE,
 *               \c GL_BLEND, \c GL_DECAL, etc.).
 * \param texBaseFormat  Base format of the texture associated with the
 *               texture unit.
 */
static void
calculate_derived_texenv( struct gl_tex_env_combine_state *state,
			  GLenum mode, GLenum texBaseFormat )
{
   GLenum mode_rgb;
   GLenum mode_a;

   *state = default_combine_state;

   switch (texBaseFormat) {
   case GL_ALPHA:
      state->SourceRGB[0] = GL_PREVIOUS;
      break;

   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_RGBA:
      break;

   case GL_LUMINANCE:
   case GL_RGB:
   case GL_YCBCR_MESA:
      state->SourceA[0] = GL_PREVIOUS;
      break;
      
   default:
      _mesa_problem(NULL, "Invalid texBaseFormat in calculate_derived_texenv");
      return;
   }

   if (mode == GL_REPLACE_EXT)
      mode = GL_REPLACE;

   switch (mode) {
   case GL_REPLACE:
   case GL_MODULATE:
      mode_rgb = (texBaseFormat == GL_ALPHA) ? GL_REPLACE : mode;
      mode_a   = mode;
      break;
   
   case GL_DECAL:
      mode_rgb = GL_INTERPOLATE;
      mode_a   = GL_REPLACE;

      state->SourceA[0] = GL_PREVIOUS;

      /* Having alpha / luminance / intensity textures replace using the
       * incoming fragment color matches the definition in NV_texture_shader.
       * The 1.5 spec simply marks these as "undefined".
       */
      switch (texBaseFormat) {
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_INTENSITY:
	 state->SourceRGB[0] = GL_PREVIOUS;
	 break;
      case GL_RGB:
      case GL_YCBCR_MESA:
	 mode_rgb = GL_REPLACE;
	 break;
      case GL_RGBA:
	 state->SourceRGB[2] = GL_TEXTURE;
	 break;
      }
      break;

   case GL_BLEND:
      mode_rgb = GL_INTERPOLATE;
      mode_a   = GL_MODULATE;

      switch (texBaseFormat) {
      case GL_ALPHA:
	 mode_rgb = GL_REPLACE;
	 break;
      case GL_INTENSITY:
	 mode_a = GL_INTERPOLATE;
	 state->SourceA[0] = GL_CONSTANT;
	 state->OperandA[2] = GL_SRC_ALPHA;
	 /* FALLTHROUGH */
      case GL_LUMINANCE:
      case GL_RGB:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
      case GL_YCBCR_MESA:
	 state->SourceRGB[2] = GL_TEXTURE;
	 state->SourceA[2]   = GL_TEXTURE;
	 state->SourceRGB[0] = GL_CONSTANT;
	 state->OperandRGB[2] = GL_SRC_COLOR;
	 break;
      }
      break;

   case GL_ADD:
      mode_rgb = (texBaseFormat == GL_ALPHA) ? GL_REPLACE : GL_ADD;
      mode_a   = (texBaseFormat == GL_INTENSITY) ? GL_ADD : GL_MODULATE;
      break;

   default:
      _mesa_problem(NULL,
                    "Invalid texture env mode in calculate_derived_texenv");
      return;
   }
   
   state->ModeRGB = (state->SourceRGB[0] != GL_PREVIOUS)
       ? mode_rgb : GL_REPLACE;
   state->ModeA   = (state->SourceA[0]   != GL_PREVIOUS)
       ? mode_a   : GL_REPLACE;
}


void GLAPIENTRY
_mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
{
   GLuint maxUnit;
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexEnvfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

#define TE_ERROR(errCode, msg, value)				\
   _mesa_error(ctx, errCode, msg, _mesa_lookup_enum_by_nr(value));

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         {
            GLenum mode = (GLenum) (GLint) *param;
            if (mode == GL_REPLACE_EXT)
               mode = GL_REPLACE;
	    if (texUnit->EnvMode == mode)
	       return;
            if (mode == GL_MODULATE ||
                mode == GL_BLEND ||
                mode == GL_DECAL ||
                mode == GL_REPLACE ||
                (mode == GL_ADD && ctx->Extensions.EXT_texture_env_add) ||
                (mode == GL_COMBINE &&
                 (ctx->Extensions.EXT_texture_env_combine ||
                  ctx->Extensions.ARB_texture_env_combine))) {
               /* legal */
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texUnit->EnvMode = mode;
            }
            else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
               return;
            }
         }
         break;
      case GL_TEXTURE_ENV_COLOR:
         {
            GLfloat tmp[4];
            tmp[0] = CLAMP( param[0], 0.0F, 1.0F );
            tmp[1] = CLAMP( param[1], 0.0F, 1.0F );
            tmp[2] = CLAMP( param[2], 0.0F, 1.0F );
            tmp[3] = CLAMP( param[3], 0.0F, 1.0F );
            if (TEST_EQ_4V(tmp, texUnit->EnvColor))
               return;
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            COPY_4FV(texUnit->EnvColor, tmp);
         }
         break;
      case GL_COMBINE_RGB:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum mode = (GLenum) (GLint) *param;
	    if (texUnit->Combine.ModeRGB == mode)
	       return;
	    switch (mode) {
	    case GL_REPLACE:
	    case GL_MODULATE:
	    case GL_ADD:
	    case GL_ADD_SIGNED:
	    case GL_INTERPOLATE:
               /* OK */
	       break;
            case GL_SUBTRACT:
               if (!ctx->Extensions.ARB_texture_env_combine) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
                  return;
               }
               break;
	    case GL_DOT3_RGB_EXT:
	    case GL_DOT3_RGBA_EXT:
	       if (!ctx->Extensions.EXT_texture_env_dot3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    case GL_DOT3_RGB:
	    case GL_DOT3_RGBA:
	       if (!ctx->Extensions.ARB_texture_env_dot3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    case GL_MODULATE_ADD_ATI:
	    case GL_MODULATE_SIGNED_ADD_ATI:
	    case GL_MODULATE_SUBTRACT_ATI:
	       if (!ctx->Extensions.ATI_texture_env_combine3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ModeRGB = mode;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
         break;
      case GL_COMBINE_ALPHA:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum mode = (GLenum) (GLint) *param;
	    if (texUnit->Combine.ModeA == mode)
	       return;
            switch (mode) {
	    case GL_REPLACE:
	    case GL_MODULATE:
	    case GL_ADD:
	    case GL_ADD_SIGNED:
	    case GL_INTERPOLATE:
	       /* OK */
	       break;
	    case GL_SUBTRACT:
	       if (!ctx->Extensions.ARB_texture_env_combine) {
		  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    case GL_MODULATE_ADD_ATI:
	    case GL_MODULATE_SIGNED_ADD_ATI:
	    case GL_MODULATE_SUBTRACT_ATI:
	       if (!ctx->Extensions.ATI_texture_env_combine3) {
                  TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
		  return;
	       }
	       break;
	    default:
	       TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", mode);
	       return;
	    }
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ModeA = mode;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_SOURCE0_RGB:
      case GL_SOURCE1_RGB:
      case GL_SOURCE2_RGB:
	 if (ctx->Extensions.EXT_texture_env_combine ||
	     ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum source = (GLenum) (GLint) *param;
	    const GLuint s = pname - GL_SOURCE0_RGB;
	    if (texUnit->Combine.SourceRGB[s] == source)
	       return;
            if (source == GL_TEXTURE ||
                source == GL_CONSTANT ||
                source == GL_PRIMARY_COLOR ||
                source == GL_PREVIOUS ||
                (ctx->Extensions.ARB_texture_env_crossbar &&
                 source >= GL_TEXTURE0 &&
                 source < GL_TEXTURE0 + ctx->Const.MaxTextureUnits) ||
                (ctx->Extensions.ATI_texture_env_combine3 &&
                 (source == GL_ZERO || source == GL_ONE))) {
               /* legal */
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.SourceRGB[s] = source;
            }
            else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", source);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_SOURCE0_ALPHA:
      case GL_SOURCE1_ALPHA:
      case GL_SOURCE2_ALPHA:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum source = (GLenum) (GLint) *param;
	    const GLuint s = pname - GL_SOURCE0_ALPHA;
	    if (texUnit->Combine.SourceA[s] == source)
	       return;
            if (source == GL_TEXTURE ||
                source == GL_CONSTANT ||
                source == GL_PRIMARY_COLOR ||
                source == GL_PREVIOUS ||
                (ctx->Extensions.ARB_texture_env_crossbar &&
                 source >= GL_TEXTURE0 &&
                 source < GL_TEXTURE0 + ctx->Const.MaxTextureUnits) ||
		(ctx->Extensions.ATI_texture_env_combine3 &&
                 (source == GL_ZERO || source == GL_ONE))) {
               /* legal */
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.SourceA[s] = source;
            }
            else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", source);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
	 if (ctx->Extensions.EXT_texture_env_combine ||
	     ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    const GLuint s = pname - GL_OPERAND0_RGB;
	    if (texUnit->Combine.OperandRGB[s] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_COLOR:
	    case GL_ONE_MINUS_SRC_COLOR:
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandRGB[s] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandA[pname-GL_OPERAND0_ALPHA] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA:
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandA[pname-GL_OPERAND0_ALPHA] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND2_RGB:
	 if (ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandRGB[2] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_COLOR:           /* ARB combine only */
	    case GL_ONE_MINUS_SRC_COLOR: /* ARB combine only */
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandRGB[2] = operand;
               break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else if (ctx->Extensions.EXT_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandRGB[2] == operand)
	       return;
	    /* operand must be GL_SRC_ALPHA which is the initial value - thus
	       don't need to actually compare the operand to the possible value */
	    else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_OPERAND2_ALPHA:
	 if (ctx->Extensions.ARB_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandA[2] == operand)
	       return;
	    switch (operand) {
	    case GL_SRC_ALPHA:
	    case GL_ONE_MINUS_SRC_ALPHA: /* ARB combine only */
	       FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	       texUnit->Combine.OperandA[2] = operand;
	       break;
	    default:
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else if (ctx->Extensions.EXT_texture_env_combine) {
	    const GLenum operand = (GLenum) (GLint) *param;
	    if (texUnit->Combine.OperandA[2] == operand)
	       return;
	    /* operand must be GL_SRC_ALPHA which is the initial value - thus
	       don't need to actually compare the operand to the possible value */
	    else {
               TE_ERROR(GL_INVALID_ENUM, "glTexEnv(param=%s)", operand);
	       return;
	    }
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_RGB_SCALE:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLuint newshift;
	    if (*param == 1.0) {
	       newshift = 0;
	    }
	    else if (*param == 2.0) {
	       newshift = 1;
	    }
	    else if (*param == 4.0) {
	       newshift = 2;
	    }
	    else {
	       _mesa_error( ctx, GL_INVALID_VALUE,
                            "glTexEnv(GL_RGB_SCALE not 1, 2 or 4)" );
	       return;
	    }
	    if (texUnit->Combine.ScaleShiftRGB == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ScaleShiftRGB = newshift;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      case GL_ALPHA_SCALE:
	 if (ctx->Extensions.EXT_texture_env_combine ||
             ctx->Extensions.ARB_texture_env_combine) {
	    GLuint newshift;
	    if (*param == 1.0) {
	       newshift = 0;
	    }
	    else if (*param == 2.0) {
	       newshift = 1;
	    }
	    else if (*param == 4.0) {
	       newshift = 2;
	    }
	    else {
	       _mesa_error( ctx, GL_INVALID_VALUE,
                            "glTexEnv(GL_ALPHA_SCALE not 1, 2 or 4)" );
	       return;
	    }
	    if (texUnit->Combine.ScaleShiftA == newshift)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->Combine.ScaleShiftA = newshift;
	 }
	 else {
            TE_ERROR(GL_INVALID_ENUM, "glTexEnv(pname=%s)", pname);
	    return;
	 }
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
	 return;
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      /* GL_EXT_texture_lod_bias */
      if (!ctx->Extensions.EXT_texture_lod_bias) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glTexEnv(target=0x%x)", target );
	 return;
      }
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
         const GLenum value = (GLenum) param[0];
         if (value == GL_TRUE || value == GL_FALSE) {
            /* It's kind of weird to set point state via glTexEnv,
             * but that's what the spec calls for.
             */
            const GLboolean state = (GLboolean) value;
            if (ctx->Point.CoordReplace[ctx->Texture.CurrentUnit] == state)
               return;
            FLUSH_VERTICES(ctx, _NEW_POINT);
            ctx->Point.CoordReplace[ctx->Texture.CurrentUnit] = state;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexEnv(param=0x%x)", value);
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
                  _mesa_lookup_enum_by_nr((GLenum) (GLint) *param));

   /* Tell device driver about the new texture environment */
   if (ctx->Driver.TexEnv) {
      (*ctx->Driver.TexEnv)( ctx, target, pname, param );
   }
}


void GLAPIENTRY
_mesa_TexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   _mesa_TexEnvfv( target, pname, &param );
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


void GLAPIENTRY
_mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
   GLuint maxUnit;
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   maxUnit = (target == GL_POINT_SPRITE_NV && pname == GL_COORD_REPLACE_NV)
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnvfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
         case GL_TEXTURE_ENV_MODE:
            *params = ENUM_TO_FLOAT(texUnit->EnvMode);
            break;
         case GL_TEXTURE_ENV_COLOR:
            COPY_4FV( params, texUnit->EnvColor );
            break;
         case GL_COMBINE_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->Combine.ModeRGB;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_COMBINE_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLfloat) texUnit->Combine.ModeA;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE0_RGB:
         case GL_SOURCE1_RGB:
         case GL_SOURCE2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned rgb_idx = pname - GL_SOURCE0_RGB;
               *params = (GLfloat) texUnit->Combine.SourceRGB[rgb_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_SOURCE0_ALPHA:
         case GL_SOURCE1_ALPHA:
         case GL_SOURCE2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned alpha_idx = pname - GL_SOURCE0_ALPHA;
               *params = (GLfloat) texUnit->Combine.SourceA[alpha_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND0_RGB:
         case GL_OPERAND1_RGB:
         case GL_OPERAND2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_rgb = pname - GL_OPERAND0_RGB;
               *params = (GLfloat) texUnit->Combine.OperandRGB[op_rgb];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_OPERAND0_ALPHA:
         case GL_OPERAND1_ALPHA:
         case GL_OPERAND2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_alpha = pname - GL_OPERAND0_ALPHA;
               *params = (GLfloat) texUnit->Combine.OperandA[op_alpha];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
            }
            break;
         case GL_RGB_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->Combine.ScaleShiftRGB == 0)
                  *params = 1.0;
               else if (texUnit->Combine.ScaleShiftRGB == 1)
                  *params = 2.0;
               else
                  *params = 4.0;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
               return;
            }
            break;
         case GL_ALPHA_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->Combine.ScaleShiftA == 0)
                  *params = 1.0;
               else if (texUnit->Combine.ScaleShiftA == 1)
                  *params = 2.0;
               else
                  *params = 4.0;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)");
               return;
            }
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname=0x%x)", pname);
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      /* GL_EXT_texture_lod_bias */
      if (!ctx->Extensions.EXT_texture_lod_bias) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
	 return;
      }
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
      ? ctx->Const.MaxTextureCoordUnits : ctx->Const.MaxTextureImageUnits;
   if (ctx->Texture.CurrentUnit >= maxUnit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexEnviv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   if (target == GL_TEXTURE_ENV) {
      switch (pname) {
         case GL_TEXTURE_ENV_MODE:
            *params = (GLint) texUnit->EnvMode;
            break;
         case GL_TEXTURE_ENV_COLOR:
            params[0] = FLOAT_TO_INT( texUnit->EnvColor[0] );
            params[1] = FLOAT_TO_INT( texUnit->EnvColor[1] );
            params[2] = FLOAT_TO_INT( texUnit->EnvColor[2] );
            params[3] = FLOAT_TO_INT( texUnit->EnvColor[3] );
            break;
         case GL_COMBINE_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->Combine.ModeRGB;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_COMBINE_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               *params = (GLint) texUnit->Combine.ModeA;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE0_RGB:
         case GL_SOURCE1_RGB:
         case GL_SOURCE2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned rgb_idx = pname - GL_SOURCE0_RGB;
               *params = (GLint) texUnit->Combine.SourceRGB[rgb_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_SOURCE0_ALPHA:
         case GL_SOURCE1_ALPHA:
         case GL_SOURCE2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned alpha_idx = pname - GL_SOURCE0_ALPHA;
               *params = (GLint) texUnit->Combine.SourceA[alpha_idx];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND0_RGB:
         case GL_OPERAND1_RGB:
         case GL_OPERAND2_RGB:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_rgb = pname - GL_OPERAND0_RGB;
               *params = (GLint) texUnit->Combine.OperandRGB[op_rgb];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_OPERAND0_ALPHA:
         case GL_OPERAND1_ALPHA:
         case GL_OPERAND2_ALPHA:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
	       const unsigned op_alpha = pname - GL_OPERAND0_ALPHA;
               *params = (GLint) texUnit->Combine.OperandA[op_alpha];
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
            }
            break;
         case GL_RGB_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->Combine.ScaleShiftRGB == 0)
                  *params = 1;
               else if (texUnit->Combine.ScaleShiftRGB == 1)
                  *params = 2;
               else
                  *params = 4;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
               return;
            }
            break;
         case GL_ALPHA_SCALE:
            if (ctx->Extensions.EXT_texture_env_combine ||
                ctx->Extensions.ARB_texture_env_combine) {
               if (texUnit->Combine.ScaleShiftA == 0)
                  *params = 1;
               else if (texUnit->Combine.ScaleShiftA == 1)
                  *params = 2;
               else
                  *params = 4;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)");
               return;
            }
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname=0x%x)",
                        pname);
      }
   }
   else if (target == GL_TEXTURE_FILTER_CONTROL_EXT) {
      /* GL_EXT_texture_lod_bias */
      if (!ctx->Extensions.EXT_texture_lod_bias) {
	 _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
	 return;
      }
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




/**********************************************************************/
/*                       Texture Parameters                           */
/**********************************************************************/

/**
 * Check if a coordinate wrap mode is supported for the texture target.
 * \return GL_TRUE if legal, GL_FALSE otherwise
 */
static GLboolean 
validate_texture_wrap_mode(GLcontext * ctx, GLenum target, GLenum wrap)
{
   const struct gl_extensions * const e = & ctx->Extensions;

   if (wrap == GL_CLAMP || wrap == GL_CLAMP_TO_EDGE ||
       (wrap == GL_CLAMP_TO_BORDER && e->ARB_texture_border_clamp)) {
      /* any texture target */
      return GL_TRUE;
   }
   else if (target != GL_TEXTURE_RECTANGLE_NV &&
	    (wrap == GL_REPEAT ||
	     (wrap == GL_MIRRORED_REPEAT &&
	      e->ARB_texture_mirrored_repeat) ||
	     (wrap == GL_MIRROR_CLAMP_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (wrap == GL_MIRROR_CLAMP_TO_EDGE_EXT &&
	      (e->ATI_texture_mirror_once || e->EXT_texture_mirror_clamp)) ||
	     (wrap == GL_MIRROR_CLAMP_TO_BORDER_EXT &&
	      (e->EXT_texture_mirror_clamp)))) {
      /* non-rectangle texture */
      return GL_TRUE;
   }

   _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
   return GL_FALSE;
}


void GLAPIENTRY
_mesa_TexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   _mesa_TexParameterfv(target, pname, &param);
}


void GLAPIENTRY
_mesa_TexParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
   const GLenum eparam = (GLenum) (GLint) params[0];
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexParameter %s %s %.1f(%s)...\n",
                  _mesa_lookup_enum_by_nr(target),
                  _mesa_lookup_enum_by_nr(pname),
                  *params,
		  _mesa_lookup_enum_by_nr(eparam));

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexParameterfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->Current3D;
         break;
      case GL_TEXTURE_CUBE_MAP:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->CurrentCubeMap;
         break;
      case GL_TEXTURE_RECTANGLE_NV:
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->CurrentRect;
         break;
      case GL_TEXTURE_1D_ARRAY_EXT:
         if (!ctx->Extensions.MESA_texture_array) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->Current1DArray;
         break;
      case GL_TEXTURE_2D_ARRAY_EXT:
         if (!ctx->Extensions.MESA_texture_array) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
            return;
         }
         texObj = texUnit->Current2DArray;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         /* A small optimization */
         if (texObj->MinFilter == eparam)
            return;
         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MinFilter = eparam;
         }
         else if ((eparam==GL_NEAREST_MIPMAP_NEAREST ||
                   eparam==GL_LINEAR_MIPMAP_NEAREST ||
                   eparam==GL_NEAREST_MIPMAP_LINEAR ||
                   eparam==GL_LINEAR_MIPMAP_LINEAR) &&
                  texObj->Target != GL_TEXTURE_RECTANGLE_NV) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MinFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_MAG_FILTER:
         /* A small optimization */
         if (texObj->MagFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->MagFilter = eparam;
         }
         else {
            _mesa_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_S:
         if (texObj->WrapS == eparam)
            return;
         if (validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapS = eparam;
         }
         else {
            return;
         }
         break;
      case GL_TEXTURE_WRAP_T:
         if (texObj->WrapT == eparam)
            return;
         if (validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapT = eparam;
         }
         else {
            return;
         }
         break;
      case GL_TEXTURE_WRAP_R:
         if (texObj->WrapR == eparam)
            return;
         if (validate_texture_wrap_mode(ctx, texObj->Target, eparam)) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->WrapR = eparam;
         }
         else {
	    return;
         }
         break;
      case GL_TEXTURE_BORDER_COLOR:
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BorderColor[RCOMP] = params[0];
         texObj->BorderColor[GCOMP] = params[1];
         texObj->BorderColor[BCOMP] = params[2];
         texObj->BorderColor[ACOMP] = params[3];
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[RCOMP], params[0]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[GCOMP], params[1]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[BCOMP], params[2]);
         UNCLAMPED_FLOAT_TO_CHAN(texObj->_BorderChan[ACOMP], params[3]);
         break;
      case GL_TEXTURE_MIN_LOD:
         if (texObj->MinLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MinLod = params[0];
         break;
      case GL_TEXTURE_MAX_LOD:
         if (texObj->MaxLod == params[0])
            return;
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLod = params[0];
         break;
      case GL_TEXTURE_BASE_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)");
            return;
         }
         if (target == GL_TEXTURE_RECTANGLE_ARB && params[0] != 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)");
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->BaseLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_MAX_LEVEL:
         if (params[0] < 0.0) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)");
            return;
         }
         if (target == GL_TEXTURE_RECTANGLE_ARB) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glTexParameter(param)");
            return;
         }
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->MaxLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_PRIORITY:
         FLUSH_VERTICES(ctx, _NEW_TEXTURE);
         texObj->Priority = CLAMP( params[0], 0.0F, 1.0F );
         break;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
	    if (params[0] < 1.0) {
	       _mesa_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
	       return;
	    }
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            /* clamp to max, that's what NVIDIA does */
            texObj->MaxAnisotropy = MIN2(params[0],
                                         ctx->Const.MaxTextureMaxAnisotropy);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_MAX_ANISOTROPY_EXT)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->CompareFlag = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_SGIX)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            GLenum op = (GLenum) params[0];
            if (op == GL_TEXTURE_LEQUAL_R_SGIX ||
                op == GL_TEXTURE_GEQUAL_R_SGIX) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareOperator = op;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM, "glTexParameter(param)");
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                    "glTexParameter(pname=GL_TEXTURE_COMPARE_OPERATOR_SGIX)");
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->ShadowAmbient = CLAMP(params[0], 0.0F, 1.0F);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_SHADOW_AMBIENT_SGIX)");
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            texObj->GenerateMipmap = params[0] ? GL_TRUE : GL_FALSE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_GENERATE_MIPMAP_SGIS)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum mode = (GLenum) params[0];
            if (mode == GL_NONE || mode == GL_COMPARE_R_TO_TEXTURE_ARB) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareMode = mode;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_MODE_ARB: 0x%x)", mode);
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_MODE_ARB)");
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            const GLenum func = (GLenum) params[0];
            if (func == GL_LEQUAL || func == GL_GEQUAL) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else if (ctx->Extensions.EXT_shadow_funcs &&
                     (func == GL_EQUAL ||
                      func == GL_NOTEQUAL ||
                      func == GL_LESS ||
                      func == GL_GREATER ||
                      func == GL_ALWAYS ||
                      func == GL_NEVER)) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->CompareFunc = func;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                           "glTexParameter(bad GL_TEXTURE_COMPARE_FUNC_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_TEXTURE_COMPARE_FUNC_ARB)");
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            const GLenum result = (GLenum) params[0];
            if (result == GL_LUMINANCE || result == GL_INTENSITY
                || result == GL_ALPHA) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->DepthMode = result;
            }
            else {
               _mesa_error(ctx, GL_INVALID_ENUM,
                          "glTexParameter(bad GL_DEPTH_TEXTURE_MODE_ARB)");
               return;
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexParameter(pname=GL_DEPTH_TEXTURE_MODE_ARB)");
            return;
         }
         break;
      case GL_TEXTURE_LOD_BIAS:
         /* NOTE: this is really part of OpenGL 1.4, not EXT_texture_lod_bias*/
         if (ctx->Extensions.EXT_texture_lod_bias) {
            if (texObj->LodBias != params[0]) {
               FLUSH_VERTICES(ctx, _NEW_TEXTURE);
               texObj->LodBias = params[0];
            }
         }
         break;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glTexParameter(pname=0x%x)", pname);
         return;
   }

   texObj->_Complete = GL_FALSE;

   if (ctx->Driver.TexParameter) {
      (*ctx->Driver.TexParameter)( ctx, target, texObj, pname, params );
   }
}


void GLAPIENTRY
_mesa_TexParameteri( GLenum target, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   if (pname == GL_TEXTURE_PRIORITY)
      fparam[0] = INT_TO_FLOAT(param);
   else
      fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   _mesa_TexParameterfv(target, pname, fparam);
}


void GLAPIENTRY
_mesa_TexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   if (pname == GL_TEXTURE_BORDER_COLOR) {
      fparam[0] = INT_TO_FLOAT(params[0]);
      fparam[1] = INT_TO_FLOAT(params[1]);
      fparam[2] = INT_TO_FLOAT(params[2]);
      fparam[3] = INT_TO_FLOAT(params[3]);
   }
   else {
      if (pname == GL_TEXTURE_PRIORITY)
         fparam[0] = INT_TO_FLOAT(params[0]);
      else
         fparam[0] = (GLfloat) params[0];
      fparam[1] = fparam[2] = fparam[3] = 0.0F;
   }
   _mesa_TexParameterfv(target, pname, fparam);
}


void GLAPIENTRY
_mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params )
{
   GLint iparam;
   _mesa_GetTexLevelParameteriv( target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}


static GLuint
tex_image_dimensions(GLcontext *ctx, GLenum target)
{
   switch (target) {
      case GL_TEXTURE_1D:
      case GL_PROXY_TEXTURE_1D:
         return 1;
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_2D:
         return 2;
      case GL_TEXTURE_3D:
      case GL_PROXY_TEXTURE_3D:
         return 3;
      case GL_TEXTURE_CUBE_MAP:
      case GL_PROXY_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         return ctx->Extensions.ARB_texture_cube_map ? 2 : 0;
      case GL_TEXTURE_RECTANGLE_NV:
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle ? 2 : 0;
      case GL_TEXTURE_1D_ARRAY_EXT:
      case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array ? 2 : 0;
      case GL_TEXTURE_2D_ARRAY_EXT:
      case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
         return ctx->Extensions.MESA_texture_array ? 3 : 0;
      default:
         _mesa_problem(ctx, "bad target in _mesa_tex_target_dimensions()");
         return 0;
   }
}


void GLAPIENTRY
_mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params )
{
   const struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   const struct gl_texture_image *img = NULL;
   GLuint dimensions;
   GLboolean isProxy;
   GLint maxLevels;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexLevelParameteriv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   /* this will catch bad target values */
   dimensions = tex_image_dimensions(ctx, target);  /* 1, 2 or 3 */
   if (dimensions == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(target)");
      return;
   }

   maxLevels = _mesa_max_texture_levels(ctx, target);
   if (maxLevels == 0) {
      /* should not happen since <target> was just checked above */
      _mesa_problem(ctx, "maxLevels=0 in _mesa_GetTexLevelParameter");
      return;
   }

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   _mesa_lock_texture(ctx, texObj);

   img = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!img || !img->TexFormat) {
      /* undefined texture image */
      if (pname == GL_TEXTURE_COMPONENTS)
         *params = 1;
      else
         *params = 0;
      goto out;
   }

   isProxy = _mesa_is_proxy_texture(target);

   switch (pname) {
      case GL_TEXTURE_WIDTH:
         *params = img->Width;
         break;
      case GL_TEXTURE_HEIGHT:
         *params = img->Height;
         break;
      case GL_TEXTURE_DEPTH:
         *params = img->Depth;
         break;
      case GL_TEXTURE_INTERNAL_FORMAT:
         *params = img->InternalFormat;
         break;
      case GL_TEXTURE_BORDER:
         *params = img->Border;
         break;
      case GL_TEXTURE_RED_SIZE:
         if (img->_BaseFormat == GL_RGB || img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->RedBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_GREEN_SIZE:
         if (img->_BaseFormat == GL_RGB || img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->GreenBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_BLUE_SIZE:
         if (img->_BaseFormat == GL_RGB || img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->BlueBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_ALPHA_SIZE:
         if (img->_BaseFormat == GL_ALPHA ||
             img->_BaseFormat == GL_LUMINANCE_ALPHA ||
             img->_BaseFormat == GL_RGBA)
            *params = img->TexFormat->AlphaBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_INTENSITY_SIZE:
         if (img->_BaseFormat != GL_INTENSITY)
            *params = 0;
         else if (img->TexFormat->IntensityBits > 0)
            *params = img->TexFormat->IntensityBits;
         else /* intensity probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         break;
      case GL_TEXTURE_LUMINANCE_SIZE:
         if (img->_BaseFormat != GL_LUMINANCE &&
             img->_BaseFormat != GL_LUMINANCE_ALPHA)
            *params = 0;
         else if (img->TexFormat->LuminanceBits > 0)
            *params = img->TexFormat->LuminanceBits;
         else /* luminance probably stored as rgb texture */
            *params = MIN2(img->TexFormat->RedBits, img->TexFormat->GreenBits);
         break;
      case GL_TEXTURE_INDEX_SIZE_EXT:
         if (img->_BaseFormat == GL_COLOR_INDEX)
            *params = img->TexFormat->IndexBits;
         else
            *params = 0;
         break;
      case GL_TEXTURE_DEPTH_SIZE_ARB:
         if (ctx->Extensions.SGIX_depth_texture ||
             ctx->Extensions.ARB_depth_texture)
            *params = img->TexFormat->DepthBits;
         else
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         break;
      case GL_TEXTURE_STENCIL_SIZE_EXT:
         if (ctx->Extensions.EXT_packed_depth_stencil) {
            *params = img->TexFormat->StencilBits;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      /* GL_ARB_texture_compression */
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE:
         if (ctx->Extensions.ARB_texture_compression) {
            if (img->IsCompressed && !isProxy) {
               /* Don't use ctx->Driver.CompressedTextureSize() since that
                * may returned a padded hardware size.
                */
               *params = _mesa_compressed_texture_size(ctx, img->Width,
                                                   img->Height, img->Depth,
                                                   img->TexFormat->MesaFormat);
            }
            else {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glGetTexLevelParameter[if]v(pname)");
            }
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_COMPRESSED:
         if (ctx->Extensions.ARB_texture_compression) {
            *params = (GLint) img->IsCompressed;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      /* GL_ARB_texture_float */
      case GL_TEXTURE_RED_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->RedBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_GREEN_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->GreenBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_BLUE_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->BlueBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_ALPHA_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->AlphaBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_LUMINANCE_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->LuminanceBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_INTENSITY_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->IntensityBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;
      case GL_TEXTURE_DEPTH_TYPE_ARB:
         if (ctx->Extensions.ARB_texture_float) {
            *params = img->TexFormat->DepthBits ? img->TexFormat->DataType : GL_NONE;
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetTexLevelParameter[if]v(pname)");
         }
         break;

      default:
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetTexLevelParameter[if]v(pname)");
   }

 out:
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *obj;
   GLboolean error = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexParameterfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)");
      return;
   }

   _mesa_lock_texture(ctx, obj);
   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
	 *params = ENUM_TO_FLOAT(obj->MagFilter);
	 break;
      case GL_TEXTURE_MIN_FILTER:
         *params = ENUM_TO_FLOAT(obj->MinFilter);
         break;
      case GL_TEXTURE_WRAP_S:
         *params = ENUM_TO_FLOAT(obj->WrapS);
         break;
      case GL_TEXTURE_WRAP_T:
         *params = ENUM_TO_FLOAT(obj->WrapT);
         break;
      case GL_TEXTURE_WRAP_R:
         *params = ENUM_TO_FLOAT(obj->WrapR);
         break;
      case GL_TEXTURE_BORDER_COLOR:
         params[0] = CLAMP(obj->BorderColor[0], 0.0F, 1.0F);
         params[1] = CLAMP(obj->BorderColor[1], 0.0F, 1.0F);
         params[2] = CLAMP(obj->BorderColor[2], 0.0F, 1.0F);
         params[3] = CLAMP(obj->BorderColor[3], 0.0F, 1.0F);
         break;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = ENUM_TO_FLOAT(resident);
         }
         break;
      case GL_TEXTURE_PRIORITY:
         *params = obj->Priority;
         break;
      case GL_TEXTURE_MIN_LOD:
         *params = obj->MinLod;
         break;
      case GL_TEXTURE_MAX_LOD:
         *params = obj->MaxLod;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         *params = (GLfloat) obj->BaseLevel;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         *params = (GLfloat) obj->MaxLevel;
         break;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = obj->MaxAnisotropy;
         }
	 else
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareFlag;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLfloat) obj->CompareOperator;
         }
	 else 
	    error = 1;
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            *params = obj->ShadowAmbient;
         }
	 else 
	    error = 1;
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLfloat) obj->GenerateMipmap;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareMode;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLfloat) obj->CompareFunc;
         }
	 else 
	    error = 1;
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLfloat) obj->DepthMode;
         }
	 else 
	    error = 1;
         break;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = obj->LodBias;
         }
	 else 
	    error = 1;
         break;
      default:
	 error = 1;
	 break;
   }
   if (error)
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname=0x%x)",
		  pname);

   _mesa_unlock_texture(ctx, obj);
}


void GLAPIENTRY
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *obj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetTexParameteriv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   obj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(target)");
      return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->MagFilter;
         return;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->MinFilter;
         return;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->WrapS;
         return;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->WrapT;
         return;
      case GL_TEXTURE_WRAP_R:
         *params = (GLint) obj->WrapR;
         return;
      case GL_TEXTURE_BORDER_COLOR:
         {
            GLfloat b[4];
            b[0] = CLAMP(obj->BorderColor[0], 0.0F, 1.0F);
            b[1] = CLAMP(obj->BorderColor[1], 0.0F, 1.0F);
            b[2] = CLAMP(obj->BorderColor[2], 0.0F, 1.0F);
            b[3] = CLAMP(obj->BorderColor[3], 0.0F, 1.0F);
            params[0] = FLOAT_TO_INT(b[0]);
            params[1] = FLOAT_TO_INT(b[1]);
            params[2] = FLOAT_TO_INT(b[2]);
            params[3] = FLOAT_TO_INT(b[3]);
         }
         return;
      case GL_TEXTURE_RESIDENT:
         {
            GLboolean resident;
            if (ctx->Driver.IsTextureResident)
               resident = ctx->Driver.IsTextureResident(ctx, obj);
            else
               resident = GL_TRUE;
            *params = (GLint) resident;
         }
         return;
      case GL_TEXTURE_PRIORITY:
         *params = FLOAT_TO_INT(obj->Priority);
         return;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->MinLod;
         return;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->MaxLod;
         return;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         return;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         return;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
         if (ctx->Extensions.EXT_texture_filter_anisotropic) {
            *params = (GLint) obj->MaxAnisotropy;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareFlag;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
         if (ctx->Extensions.SGIX_shadow) {
            *params = (GLint) obj->CompareOperator;
            return;
         }
         break;
      case GL_SHADOW_AMBIENT_SGIX: /* aka GL_TEXTURE_COMPARE_FAIL_VALUE_ARB */
         if (ctx->Extensions.SGIX_shadow_ambient) {
            *params = (GLint) FLOAT_TO_INT(obj->ShadowAmbient);
            return;
         }
         break;
      case GL_GENERATE_MIPMAP_SGIS:
         if (ctx->Extensions.SGIS_generate_mipmap) {
            *params = (GLint) obj->GenerateMipmap;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_MODE_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareMode;
            return;
         }
         break;
      case GL_TEXTURE_COMPARE_FUNC_ARB:
         if (ctx->Extensions.ARB_shadow) {
            *params = (GLint) obj->CompareFunc;
            return;
         }
         break;
      case GL_DEPTH_TEXTURE_MODE_ARB:
         if (ctx->Extensions.ARB_depth_texture) {
            *params = (GLint) obj->DepthMode;
            return;
         }
         break;
      case GL_TEXTURE_LOD_BIAS:
         if (ctx->Extensions.EXT_texture_lod_bias) {
            *params = (GLint) obj->LodBias;
            return;
         }
         break;
      default:
         ; /* silence warnings */
   }
   /* If we get here, pname was an unrecognized enum */
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname=0x%x)", pname);
}




/**********************************************************************/
/*                    Texture Coord Generation                        */
/**********************************************************************/

#if FEATURE_texgen
void GLAPIENTRY
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&(VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexGen %s %s %.1f(%s)...\n",
                  _mesa_lookup_enum_by_nr(coord),
                  _mesa_lookup_enum_by_nr(pname),
                  *params,
		  _mesa_lookup_enum_by_nr((GLenum) (GLint) *params));

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexGen(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLbitfield bits;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bits = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       bits = TEXGEN_EYE_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       bits = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       bits = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_SPHERE_MAP:
	       bits = TEXGEN_SPHERE_MAP;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeS == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeS = mode;
	    texUnit->_GenBitS = bits;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneS, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            COPY_4FV(texUnit->ObjectPlaneS, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneS, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneS, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLbitfield bitt;
	    switch (mode) {
               case GL_OBJECT_LINEAR:
                  bitt = TEXGEN_OBJ_LINEAR;
                  break;
               case GL_EYE_LINEAR:
                  bitt = TEXGEN_EYE_LINEAR;
                  break;
               case GL_REFLECTION_MAP_NV:
                  bitt = TEXGEN_REFLECTION_MAP_NV;
                  break;
               case GL_NORMAL_MAP_NV:
                  bitt = TEXGEN_NORMAL_MAP_NV;
                  break;
               case GL_SPHERE_MAP:
                  bitt = TEXGEN_SPHERE_MAP;
                  break;
               default:
                  _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
                  return;
	    }
	    if (texUnit->GenModeT == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeT = mode;
	    texUnit->_GenBitT = bitt;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneT, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            COPY_4FV(texUnit->ObjectPlaneT, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
	    if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneT, tmp))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneT, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLbitfield bitr;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bitr = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_REFLECTION_MAP_NV:
	       bitr = TEXGEN_REFLECTION_MAP_NV;
	       break;
	    case GL_NORMAL_MAP_NV:
	       bitr = TEXGEN_NORMAL_MAP_NV;
	       break;
	    case GL_EYE_LINEAR:
	       bitr = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeR == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeR = mode;
	    texUnit->_GenBitR = bitr;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneR, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->ObjectPlaneR, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneR, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneR, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    GLbitfield bitq;
	    switch (mode) {
	    case GL_OBJECT_LINEAR:
	       bitq = TEXGEN_OBJ_LINEAR;
	       break;
	    case GL_EYE_LINEAR:
	       bitq = TEXGEN_EYE_LINEAR;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	    if (texUnit->GenModeQ == mode)
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    texUnit->GenModeQ = mode;
	    texUnit->_GenBitQ = bitq;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    if (TEST_EQ_4V(texUnit->ObjectPlaneQ, params))
		return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
            COPY_4FV(texUnit->ObjectPlaneQ, params);
	 }
	 else if (pname==GL_EYE_PLANE) {
	    GLfloat tmp[4];
            /* Transform plane equation by the inverse modelview matrix */
            if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top)) {
               _math_matrix_analyse( ctx->ModelviewMatrixStack.Top );
            }
            _mesa_transform_vector( tmp, params, ctx->ModelviewMatrixStack.Top->inv );
	    if (TEST_EQ_4V(texUnit->EyePlaneQ, tmp))
	       return;
	    FLUSH_VERTICES(ctx, _NEW_TEXTURE);
	    COPY_4FV(texUnit->EyePlaneQ, tmp);
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexGenfv(coord)" );
	 return;
   }

   if (ctx->Driver.TexGen)
      ctx->Driver.TexGen( ctx, coord, pname, params );
}


void GLAPIENTRY
_mesa_TexGeniv(GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
      p[1] = p[2] = p[3] = 0.0F;
   }
   else {
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
      p[3] = (GLfloat) params[3];
   }
   _mesa_TexGenfv(coord, pname, p);
}


void GLAPIENTRY
_mesa_TexGend(GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p = (GLfloat) param;
   _mesa_TexGenfv( coord, pname, &p );
}


void GLAPIENTRY
_mesa_TexGendv(GLenum coord, GLenum pname, const GLdouble *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   if (pname == GL_TEXTURE_GEN_MODE) {
      p[1] = p[2] = p[3] = 0.0F;
   }
   else {
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
      p[3] = (GLfloat) params[3];
   }
   _mesa_TexGenfv( coord, pname, p );
}


void GLAPIENTRY
_mesa_TexGenf( GLenum coord, GLenum pname, GLfloat param )
{
   _mesa_TexGenfv(coord, pname, &param);
}


void GLAPIENTRY
_mesa_TexGeni( GLenum coord, GLenum pname, GLint param )
{
   _mesa_TexGeniv( coord, pname, &param );
}



void GLAPIENTRY
_mesa_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
{
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexGendv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(coord)" );
	 return;
   }
}



void GLAPIENTRY
_mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexGenfv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(coord)" );
	 return;
   }
}



void GLAPIENTRY
_mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
   const struct gl_texture_unit *texUnit;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetTexGeniv(current unit)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   switch (coord) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeS;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneS[0];
            params[1] = (GLint) texUnit->ObjectPlaneS[1];
            params[2] = (GLint) texUnit->ObjectPlaneS[2];
            params[3] = (GLint) texUnit->ObjectPlaneS[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneS[0];
            params[1] = (GLint) texUnit->EyePlaneS[1];
            params[2] = (GLint) texUnit->EyePlaneS[2];
            params[3] = (GLint) texUnit->EyePlaneS[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeT;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneT[0];
            params[1] = (GLint) texUnit->ObjectPlaneT[1];
            params[2] = (GLint) texUnit->ObjectPlaneT[2];
            params[3] = (GLint) texUnit->ObjectPlaneT[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneT[0];
            params[1] = (GLint) texUnit->EyePlaneT[1];
            params[2] = (GLint) texUnit->EyePlaneT[2];
            params[3] = (GLint) texUnit->EyePlaneT[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeR;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneR[0];
            params[1] = (GLint) texUnit->ObjectPlaneR[1];
            params[2] = (GLint) texUnit->ObjectPlaneR[2];
            params[3] = (GLint) texUnit->ObjectPlaneR[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneR[0];
            params[1] = (GLint) texUnit->EyePlaneR[1];
            params[2] = (GLint) texUnit->EyePlaneR[2];
            params[3] = (GLint) texUnit->EyePlaneR[3];
	 }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeQ;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            params[0] = (GLint) texUnit->ObjectPlaneQ[0];
            params[1] = (GLint) texUnit->ObjectPlaneQ[1];
            params[2] = (GLint) texUnit->ObjectPlaneQ[2];
            params[3] = (GLint) texUnit->ObjectPlaneQ[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            params[0] = (GLint) texUnit->EyePlaneQ[0];
            params[1] = (GLint) texUnit->EyePlaneQ[1];
            params[2] = (GLint) texUnit->EyePlaneQ[2];
            params[3] = (GLint) texUnit->EyePlaneQ[3];
         }
	 else {
	    _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(coord)" );
	 return;
   }
}
#endif


/* GL_ARB_multitexture */
void GLAPIENTRY
_mesa_ActiveTextureARB(GLenum texture)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint texUnit = texture - GL_TEXTURE0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glActiveTexture %s\n",
                  _mesa_lookup_enum_by_nr(texture));

   /* XXX error-check against max(coordunits, imageunits) */
   if (texUnit >= ctx->Const.MaxTextureUnits) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glActiveTexture(texture)");
      return;
   }

   if (ctx->Texture.CurrentUnit == texUnit)
      return;

   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   ctx->Texture.CurrentUnit = texUnit;
   if (ctx->Transform.MatrixMode == GL_TEXTURE) {
      /* update current stack pointer */
      ctx->CurrentStack = &ctx->TextureMatrixStack[texUnit];
   }

   if (ctx->Driver.ActiveTexture) {
      (*ctx->Driver.ActiveTexture)( ctx, (GLuint) texUnit );
   }
}


/* GL_ARB_multitexture */
void GLAPIENTRY
_mesa_ClientActiveTextureARB(GLenum texture)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint texUnit = texture - GL_TEXTURE0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (texUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClientActiveTexture(texture)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_ARRAY);
   ctx->Array.ActiveTexture = texUnit;
}



/**********************************************************************/
/*****                    State management                        *****/
/**********************************************************************/


/**
 * \note This routine refers to derived texture attribute values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE_MATRIX.  On changes to _NEW_TEXTURE, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_textures(), below.
 *
 * \param ctx GL context.
 */
static void
update_texture_matrices( GLcontext *ctx )
{
   GLuint i;

   ctx->Texture._TexMatEnabled = 0;

   for (i=0; i < ctx->Const.MaxTextureUnits; i++) {
      if (_math_matrix_is_dirty(ctx->TextureMatrixStack[i].Top)) {
	 _math_matrix_analyse( ctx->TextureMatrixStack[i].Top );

	 if (ctx->Texture.Unit[i]._ReallyEnabled &&
	     ctx->TextureMatrixStack[i].Top->type != MATRIX_IDENTITY)
	    ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(i);

	 if (ctx->Driver.TextureMatrix)
	    ctx->Driver.TextureMatrix( ctx, i, ctx->TextureMatrixStack[i].Top);
      }
   }
}


/**
 * Update texture object's _Function field.  We need to do this
 * whenever any of the texture object's shadow-related fields change
 * or when we start/stop using a fragment program.
 *
 * This function could be expanded someday to update additional per-object
 * fields that depend on assorted state changes.
 */
static void
update_texture_compare_function(GLcontext *ctx,
                                struct gl_texture_object *tObj)
{
   /* XXX temporarily disable this test since it breaks the GLSL
    * shadow2D(), etc. functions.
    */
   if (0 /*ctx->FragmentProgram._Current*/) {
      /* Texel/coordinate comparison is ignored for programs.
       * See GL_ARB_fragment_program/shader spec for details.
       */
      tObj->_Function = GL_NONE;
   }
   else if (tObj->CompareFlag) {
      /* GL_SGIX_shadow */
      if (tObj->CompareOperator == GL_TEXTURE_LEQUAL_R_SGIX) {
         tObj->_Function = GL_LEQUAL;
      }
      else {
         ASSERT(tObj->CompareOperator == GL_TEXTURE_GEQUAL_R_SGIX);
         tObj->_Function = GL_GEQUAL;
      }
   }
   else if (tObj->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB) {
      /* GL_ARB_shadow */
      tObj->_Function = tObj->CompareFunc;
   }
   else {
      tObj->_Function = GL_NONE;  /* pass depth through as grayscale */
   }
}


/**
 * Helper function for determining which texture object (1D, 2D, cube, etc)
 * should actually be used.
 */
static void
texture_override(GLcontext *ctx,
                 struct gl_texture_unit *texUnit, GLbitfield enableBits,
                 struct gl_texture_object *texObj, GLuint textureBit)
{
   if (!texUnit->_ReallyEnabled && (enableBits & textureBit)) {
      if (!texObj->_Complete) {
         _mesa_test_texobj_completeness(ctx, texObj);
      }
      if (texObj->_Complete) {
         texUnit->_ReallyEnabled = textureBit;
         texUnit->_Current = texObj;
         update_texture_compare_function(ctx, texObj);
      }
   }
}


/**
 * \note This routine refers to derived texture matrix values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE.  On changes to _NEW_TEXTURE_MATRIX, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_texture_matrices, above.
 *
 * \param ctx GL context.
 */
static void
update_texture_state( GLcontext *ctx )
{
   GLuint unit;
   struct gl_fragment_program *fprog = NULL;
   struct gl_vertex_program *vprog = NULL;

   if (ctx->Shader.CurrentProgram &&
       ctx->Shader.CurrentProgram->LinkStatus) {
      fprog = ctx->Shader.CurrentProgram->FragmentProgram;
      vprog = ctx->Shader.CurrentProgram->VertexProgram;
   }
   else {
      if (ctx->FragmentProgram._Enabled) {
         fprog = ctx->FragmentProgram.Current;
      }
      if (ctx->VertexProgram._Enabled) {
         /* XXX enable this if/when non-shader vertex programs get
          * texture fetches:
         vprog = ctx->VertexProgram.Current;
         */
      }
   }

   ctx->NewState |= _NEW_TEXTURE; /* TODO: only set this if there are 
				   * actual changes. 
				   */

   ctx->Texture._EnabledUnits = 0;
   ctx->Texture._GenFlags = 0;
   ctx->Texture._TexMatEnabled = 0;
   ctx->Texture._TexGenEnabled = 0;

   /*
    * Update texture unit state.
    */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
      GLbitfield enableBits;

      texUnit->_Current = NULL;
      texUnit->_ReallyEnabled = 0;
      texUnit->_GenFlags = 0;

      /* Get the bitmask of texture enables.
       * enableBits will be a mask of the TEXTURE_*_BIT flags indicating
       * which texture targets are enabled (fixed function) or referenced
       * by a fragment shader/program.  When multiple flags are set, we'll
       * settle on the one with highest priority (see texture_override below).
       */
      if (fprog || vprog) {
         enableBits = 0x0;
         if (fprog)
            enableBits |= fprog->Base.TexturesUsed[unit];
         if (vprog)
            enableBits |= vprog->Base.TexturesUsed[unit];
      }
      else {
         if (!texUnit->Enabled)
            continue;
         enableBits = texUnit->Enabled;
      }

      /* Look for the highest-priority texture target that's enabled and
       * complete.  That's the one we'll use for texturing.  If we're using
       * a fragment program we're guaranteed that bitcount(enabledBits) <= 1.
       */
      texture_override(ctx, texUnit, enableBits,
                       texUnit->Current2DArray, TEXTURE_2D_ARRAY_BIT);
      texture_override(ctx, texUnit, enableBits,
                       texUnit->Current1DArray, TEXTURE_1D_ARRAY_BIT);
      texture_override(ctx, texUnit, enableBits,
                       texUnit->CurrentCubeMap, TEXTURE_CUBE_BIT);
      texture_override(ctx, texUnit, enableBits,
                       texUnit->Current3D, TEXTURE_3D_BIT);
      texture_override(ctx, texUnit, enableBits,
                       texUnit->CurrentRect, TEXTURE_RECT_BIT);
      texture_override(ctx, texUnit, enableBits,
                       texUnit->Current2D, TEXTURE_2D_BIT);
      texture_override(ctx, texUnit, enableBits,
                       texUnit->Current1D, TEXTURE_1D_BIT);

      if (!texUnit->_ReallyEnabled) {
         continue;
      }

      if (texUnit->_ReallyEnabled)
         ctx->Texture._EnabledUnits |= (1 << unit);

      if (texUnit->EnvMode == GL_COMBINE) {
	 texUnit->_CurrentCombine = & texUnit->Combine;
      }
      else {
         const struct gl_texture_object *texObj = texUnit->_Current;
         GLenum format = texObj->Image[0][texObj->BaseLevel]->_BaseFormat;
         if (format == GL_COLOR_INDEX) {
            format = GL_RGBA;  /* a bit of a hack */
         }
         else if (format == GL_DEPTH_COMPONENT
                  || format == GL_DEPTH_STENCIL_EXT) {
            format = texObj->DepthMode;
         }
	 calculate_derived_texenv(&texUnit->_EnvMode, texUnit->EnvMode, format);
	 texUnit->_CurrentCombine = & texUnit->_EnvMode;
      }

      switch (texUnit->_CurrentCombine->ModeRGB) {
      case GL_REPLACE:
	 texUnit->_CurrentCombine->_NumArgsRGB = 1;
	 break;
      case GL_MODULATE:
      case GL_ADD:
      case GL_ADD_SIGNED:
      case GL_SUBTRACT:
      case GL_DOT3_RGB:
      case GL_DOT3_RGBA:
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
	 texUnit->_CurrentCombine->_NumArgsRGB = 2;
	 break;
      case GL_INTERPOLATE:
      case GL_MODULATE_ADD_ATI:
      case GL_MODULATE_SIGNED_ADD_ATI:
      case GL_MODULATE_SUBTRACT_ATI:
	 texUnit->_CurrentCombine->_NumArgsRGB = 3;
	 break;
      default:
	 texUnit->_CurrentCombine->_NumArgsRGB = 0;
         _mesa_problem(ctx, "invalid RGB combine mode in update_texture_state");
         return;
      }

      switch (texUnit->_CurrentCombine->ModeA) {
      case GL_REPLACE:
	 texUnit->_CurrentCombine->_NumArgsA = 1;
	 break;
      case GL_MODULATE:
      case GL_ADD:
      case GL_ADD_SIGNED:
      case GL_SUBTRACT:
	 texUnit->_CurrentCombine->_NumArgsA = 2;
	 break;
      case GL_INTERPOLATE:
      case GL_MODULATE_ADD_ATI:
      case GL_MODULATE_SIGNED_ADD_ATI:
      case GL_MODULATE_SUBTRACT_ATI:
	 texUnit->_CurrentCombine->_NumArgsA = 3;
	 break;
      default:
	 texUnit->_CurrentCombine->_NumArgsA = 0;
         _mesa_problem(ctx, "invalid Alpha combine mode in update_texture_state");
	 break;
      }
   }

   /* Determine which texture coordinate sets are actually needed */
   if (fprog) {
      const GLuint coordMask = (1 << MAX_TEXTURE_COORD_UNITS) - 1;
      ctx->Texture._EnabledCoordUnits
         = (fprog->Base.InputsRead >> FRAG_ATTRIB_TEX0) & coordMask;
   }
   else {
      ctx->Texture._EnabledCoordUnits = ctx->Texture._EnabledUnits;
   }

   /* Setup texgen for those texture coordinate sets that are in use */
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

      if (!(ctx->Texture._EnabledCoordUnits & (1 << unit)))
	 continue;

      if (texUnit->TexGenEnabled) {
	 if (texUnit->TexGenEnabled & S_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitS;
	 }
	 if (texUnit->TexGenEnabled & T_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitT;
	 }
	 if (texUnit->TexGenEnabled & Q_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitQ;
	 }
	 if (texUnit->TexGenEnabled & R_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitR;
	 }

	 ctx->Texture._TexGenEnabled |= ENABLE_TEXGEN(unit);
	 ctx->Texture._GenFlags |= texUnit->_GenFlags;
      }

      if (ctx->TextureMatrixStack[unit].Top->type != MATRIX_IDENTITY)
	 ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(unit);
   }
}


/**
 * Update texture-related derived state.
 */
void
_mesa_update_texture( GLcontext *ctx, GLuint new_state )
{
   if (new_state & _NEW_TEXTURE_MATRIX)
      update_texture_matrices( ctx );

   if (new_state & (_NEW_TEXTURE | _NEW_PROGRAM))
      update_texture_state( ctx );
}


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

/**
 * Allocate the proxy textures for the given context.
 * 
 * \param ctx the context to allocate proxies for.
 * 
 * \return GL_TRUE on success, or GL_FALSE on failure
 * 
 * If run out of memory part way through the allocations, clean up and return
 * GL_FALSE.
 */
static GLboolean
alloc_proxy_textures( GLcontext *ctx )
{
   static const GLenum targets[] = {
      GL_TEXTURE_1D,
      GL_TEXTURE_2D,
      GL_TEXTURE_3D,
      GL_TEXTURE_CUBE_MAP_ARB,
      GL_TEXTURE_RECTANGLE_NV,
      GL_TEXTURE_1D_ARRAY_EXT,
      GL_TEXTURE_2D_ARRAY_EXT
   };
   GLint tgt;

   ASSERT(Elements(targets) == NUM_TEXTURE_TARGETS);

   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      if (!(ctx->Texture.ProxyTex[tgt]
            = ctx->Driver.NewTextureObject(ctx, 0, targets[tgt]))) {
         /* out of memory, free what we did allocate */
         while (--tgt >= 0) {
            ctx->Driver.DeleteTexture(ctx, ctx->Texture.ProxyTex[tgt]);
         }
         return GL_FALSE;
      }
   }

   assert(ctx->Texture.ProxyTex[0]->RefCount == 1); /* sanity check */
   return GL_TRUE;
}


/**
 * Initialize a texture unit.
 *
 * \param ctx GL context.
 * \param unit texture unit number to be initialized.
 */
static void
init_texture_unit( GLcontext *ctx, GLuint unit )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   texUnit->EnvMode = GL_MODULATE;
   ASSIGN_4V( texUnit->EnvColor, 0.0, 0.0, 0.0, 0.0 );

   texUnit->Combine = default_combine_state;
   texUnit->_EnvMode = default_combine_state;
   texUnit->_CurrentCombine = & texUnit->_EnvMode;

   texUnit->TexGenEnabled = 0;
   texUnit->GenModeS = GL_EYE_LINEAR;
   texUnit->GenModeT = GL_EYE_LINEAR;
   texUnit->GenModeR = GL_EYE_LINEAR;
   texUnit->GenModeQ = GL_EYE_LINEAR;
   texUnit->_GenBitS = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitT = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitR = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitQ = TEXGEN_EYE_LINEAR;

   /* Yes, these plane coefficients are correct! */
   ASSIGN_4V( texUnit->ObjectPlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneQ, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneQ, 0.0, 0.0, 0.0, 0.0 );

   /* initialize current texture object ptrs to the shared default objects */
   _mesa_reference_texobj(&texUnit->Current1D, ctx->Shared->Default1D);
   _mesa_reference_texobj(&texUnit->Current2D, ctx->Shared->Default2D);
   _mesa_reference_texobj(&texUnit->Current3D, ctx->Shared->Default3D);
   _mesa_reference_texobj(&texUnit->CurrentCubeMap, ctx->Shared->DefaultCubeMap);
   _mesa_reference_texobj(&texUnit->CurrentRect, ctx->Shared->DefaultRect);
   _mesa_reference_texobj(&texUnit->Current1DArray, ctx->Shared->Default1DArray);
   _mesa_reference_texobj(&texUnit->Current2DArray, ctx->Shared->Default2DArray);
}


/**
 * Initialize texture state for the given context.
 */
GLboolean
_mesa_init_texture(GLcontext *ctx)
{
   GLuint i;

   assert(MAX_TEXTURE_LEVELS >= MAX_3D_TEXTURE_LEVELS);
   assert(MAX_TEXTURE_LEVELS >= MAX_CUBE_TEXTURE_LEVELS);

   /* Texture group */
   ctx->Texture.CurrentUnit = 0;      /* multitexture */
   ctx->Texture._EnabledUnits = 0;
   ctx->Texture.SharedPalette = GL_FALSE;
   _mesa_init_colortable(&ctx->Texture.Palette);

   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      init_texture_unit( ctx, i );

   /* After we're done initializing the context's texture state the default
    * texture objects' refcounts should be at least MAX_TEXTURE_UNITS + 1.
    */
   assert(ctx->Shared->Default1D->RefCount >= MAX_TEXTURE_UNITS + 1);

   _mesa_TexEnvProgramCacheInit( ctx );

   /* Allocate proxy textures */
   if (!alloc_proxy_textures( ctx ))
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Free dynamically-allocted texture data attached to the given context.
 */
void
_mesa_free_texture_data(GLcontext *ctx)
{
   GLuint u, tgt;

   /* unreference current textures */
   for (u = 0; u < MAX_TEXTURE_IMAGE_UNITS; u++) {
      struct gl_texture_unit *unit = ctx->Texture.Unit + u;
      _mesa_reference_texobj(&unit->Current1D, NULL);
      _mesa_reference_texobj(&unit->Current2D, NULL);
      _mesa_reference_texobj(&unit->Current3D, NULL);
      _mesa_reference_texobj(&unit->CurrentCubeMap, NULL);
      _mesa_reference_texobj(&unit->CurrentRect, NULL);
      _mesa_reference_texobj(&unit->Current1DArray, NULL);
      _mesa_reference_texobj(&unit->Current2DArray, NULL);
   }

   /* Free proxy texture objects */
   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++)
      ctx->Driver.DeleteTexture(ctx, ctx->Texture.ProxyTex[tgt]);

   for (u = 0; u < MAX_TEXTURE_IMAGE_UNITS; u++)
      _mesa_free_colortable_data( &ctx->Texture.Unit[u].ColorTable );

   _mesa_TexEnvProgramCacheDestroy( ctx );
}


/**
 * Update the default texture objects in the given context to reference those
 * specified in the shared state and release those referencing the old 
 * shared state.
 */
void
_mesa_update_default_objects_texture(GLcontext *ctx)
{
   GLuint i;

   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];

      _mesa_reference_texobj(&texUnit->Current1D, ctx->Shared->Default1D);
      _mesa_reference_texobj(&texUnit->Current2D, ctx->Shared->Default2D);
      _mesa_reference_texobj(&texUnit->Current3D, ctx->Shared->Default3D);
      _mesa_reference_texobj(&texUnit->CurrentCubeMap, ctx->Shared->DefaultCubeMap);
      _mesa_reference_texobj(&texUnit->CurrentRect, ctx->Shared->DefaultRect);
      _mesa_reference_texobj(&texUnit->Current1DArray, ctx->Shared->Default1DArray);
      _mesa_reference_texobj(&texUnit->Current2DArray, ctx->Shared->Default2DArray);
   }
}
