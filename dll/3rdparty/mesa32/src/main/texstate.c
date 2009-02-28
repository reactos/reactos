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
#if FEATURE_colortable
#include "colortab.h"
#endif
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
   for (i = 0; i < src->Const.MaxTextureImageUnits; i++) {
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

   if (texUnit >= ctx->Const.MaxTextureImageUnits) {
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

   for (i=0; i < ctx->Const.MaxTextureCoordUnits; i++) {
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
   for (unit = 0; unit < ctx->Const.MaxTextureImageUnits; unit++) {
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

      ASSERT(texUnit->Current1D);
      ASSERT(texUnit->Current2D);
      ASSERT(texUnit->Current3D);
      ASSERT(texUnit->CurrentCubeMap);
      ASSERT(texUnit->CurrentRect);
      ASSERT(texUnit->Current1DArray);
      ASSERT(texUnit->Current2DArray);

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
   for (unit = 0; unit < ctx->Const.MaxTextureCoordUnits; unit++) {
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
#if FEATURE_colortable
   _mesa_init_colortable(&ctx->Texture.Palette);
#endif

   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      init_texture_unit( ctx, i );

   /* After we're done initializing the context's texture state the default
    * texture objects' refcounts should be at least MAX_TEXTURE_UNITS + 1.
    */
   assert(ctx->Shared->Default1D->RefCount >= MAX_TEXTURE_UNITS + 1);

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

#if FEATURE_colortable
   {
      GLuint i;
      for (i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++)
         _mesa_free_colortable_data( &ctx->Texture.Unit[i].ColorTable );
   }
#endif
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
