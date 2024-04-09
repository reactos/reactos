/*
 * Mesa 3-D graphics library
 * Version:  7.5
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

#include <precomp.h>

/**
 * Default texture combine environment state.  This is used to initialize
 * a context's texture units and as the basis for converting "classic"
 * texture environmnets to ARB_texture_env_combine style values.
 */
static const struct gl_tex_env_combine_state default_combine_state = {
   GL_MODULATE, GL_MODULATE,
   { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT, GL_CONSTANT },
   { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT, GL_CONSTANT },
   { GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_ALPHA, GL_SRC_ALPHA },
   { GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA },
   0, 0,
   2, 2
};



/**
 * Used by glXCopyContext to copy texture state from one context to another.
 */
void
_mesa_copy_texture_state( const struct gl_context *src, struct gl_context *dst )
{
   GLuint tex;

   ASSERT(src);
   ASSERT(dst);

   dst->Texture._GenFlags = src->Texture._GenFlags;
   dst->Texture._TexGenEnabled = src->Texture._TexGenEnabled;
   dst->Texture._TexMatEnabled = src->Texture._TexMatEnabled;

   dst->Texture.Unit.Enabled = src->Texture.Unit.Enabled;
   dst->Texture.Unit.EnvMode = src->Texture.Unit.EnvMode;
   COPY_4V(dst->Texture.Unit.EnvColor, src->Texture.Unit.EnvColor);
   dst->Texture.Unit.TexGenEnabled = src->Texture.Unit.TexGenEnabled;
   dst->Texture.Unit.GenS = src->Texture.Unit.GenS;
   dst->Texture.Unit.GenT = src->Texture.Unit.GenT;
   dst->Texture.Unit.GenR = src->Texture.Unit.GenR;
   dst->Texture.Unit.GenQ = src->Texture.Unit.GenQ;
   dst->Texture.Unit.LodBias = src->Texture.Unit.LodBias;

   /* GL_EXT_texture_env_combine */
   dst->Texture.Unit.Combine = src->Texture.Unit.Combine;

   /*
    * XXX strictly speaking, we should compare texture names/ids and
    * bind textures in the dest context according to id.  For now, only
    * copy bindings if the contexts share the same pool of textures to
    * avoid refcounting bugs.
    */
   if (dst->Shared == src->Shared) {
      /* copy texture object bindings, not contents of texture objects */
      _mesa_lock_context_textures(dst);

      for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
         _mesa_reference_texobj(&dst->Texture.Unit.CurrentTex[tex],
                                src->Texture.Unit.CurrentTex[tex]);
      }
      _mesa_unlock_context_textures(dst);
   }
}


/*
 * For debugging
 */
void
_mesa_print_texunit_state( struct gl_context *ctx )
{
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit;
   printf("Texture Unit\n");
   printf("  GL_TEXTURE_ENV_MODE = %s\n", _mesa_lookup_enum_by_nr(texUnit->EnvMode));
   printf("  GL_COMBINE_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeRGB));
   printf("  GL_COMBINE_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.ModeA));
   printf("  GL_SOURCE0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[0]));
   printf("  GL_SOURCE1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[1]));
   printf("  GL_SOURCE2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceRGB[2]));
   printf("  GL_SOURCE0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[0]));
   printf("  GL_SOURCE1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[1]));
   printf("  GL_SOURCE2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.SourceA[2]));
   printf("  GL_OPERAND0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[0]));
   printf("  GL_OPERAND1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[1]));
   printf("  GL_OPERAND2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandRGB[2]));
   printf("  GL_OPERAND0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[0]));
   printf("  GL_OPERAND1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[1]));
   printf("  GL_OPERAND2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->Combine.OperandA[2]));
   printf("  GL_RGB_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftRGB);
   printf("  GL_ALPHA_SCALE = %d\n", 1 << texUnit->Combine.ScaleShiftA);
   printf("  GL_TEXTURE_ENV_COLOR = (%f, %f, %f, %f)\n", texUnit->EnvColor[0], texUnit->EnvColor[1], texUnit->EnvColor[2], texUnit->EnvColor[3]);
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
   case GL_RED:
   case GL_RG:
   case GL_RGB:
   case GL_YCBCR_MESA:
      state->SourceA[0] = GL_PREVIOUS;
      break;
      
   default:
      _mesa_problem(NULL,
                    "Invalid texBaseFormat 0x%x in calculate_derived_texenv",
                    texBaseFormat);
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
      case GL_RED:
      case GL_RG:
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
      case GL_RED:
      case GL_RG:
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
                    "Invalid texture env mode 0x%x in calculate_derived_texenv",
                    mode);
      return;
   }
   
   state->ModeRGB = (state->SourceRGB[0] != GL_PREVIOUS)
       ? mode_rgb : GL_REPLACE;
   state->ModeA   = (state->SourceA[0]   != GL_PREVIOUS)
       ? mode_a   : GL_REPLACE;
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
update_texture_matrices( struct gl_context *ctx )
{
   ctx->Texture._TexMatEnabled = 0x0;

   if (_math_matrix_is_dirty(ctx->TextureMatrixStack.Top)) {
	 _math_matrix_analyse( ctx->TextureMatrixStack.Top );

	 if (ctx->Texture.Unit._ReallyEnabled &&
	     ctx->TextureMatrixStack.Top->type != MATRIX_IDENTITY)
	    ctx->Texture._TexMatEnabled = GL_TRUE;
   }
}


/**
 * Examine texture unit's combine/env state to update derived state.
 */
static void
update_tex_combine(struct gl_context *ctx, struct gl_texture_unit *texUnit)
{
   struct gl_tex_env_combine_state *combine;

   /* Set the texUnit->_CurrentCombine field to point to the user's combiner
    * state, or the combiner state which is derived from traditional texenv
    * mode.
    */
   if (texUnit->EnvMode == GL_COMBINE ||
       texUnit->EnvMode == GL_COMBINE4_NV) {
      texUnit->_CurrentCombine = & texUnit->Combine;
   }
   else {
      const struct gl_texture_object *texObj = texUnit->_Current;
      GLenum format = texObj->Image[0][texObj->BaseLevel]->_BaseFormat;

      calculate_derived_texenv(&texUnit->_EnvMode, texUnit->EnvMode, format);
      texUnit->_CurrentCombine = & texUnit->_EnvMode;
   }

   combine = texUnit->_CurrentCombine;

   /* Determine number of source RGB terms in the combiner function */
   switch (combine->ModeRGB) {
   case GL_REPLACE:
      combine->_NumArgsRGB = 1;
      break;
   case GL_ADD:
   case GL_ADD_SIGNED:
      if (texUnit->EnvMode == GL_COMBINE4_NV)
         combine->_NumArgsRGB = 4;
      else
         combine->_NumArgsRGB = 2;
      break;
   case GL_MODULATE:
   case GL_SUBTRACT:
   case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
   case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGBA_EXT:
      combine->_NumArgsRGB = 2;
      break;
   case GL_INTERPOLATE:
   case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
   case GL_MODULATE_SUBTRACT_ATI:
      combine->_NumArgsRGB = 3;
      break;
   default:
      combine->_NumArgsRGB = 0;
      _mesa_problem(ctx, "invalid RGB combine mode in update_texture_state");
      return;
   }

   /* Determine number of source Alpha terms in the combiner function */
   switch (combine->ModeA) {
   case GL_REPLACE:
      combine->_NumArgsA = 1;
      break;
   case GL_ADD:
   case GL_ADD_SIGNED:
      if (texUnit->EnvMode == GL_COMBINE4_NV)
         combine->_NumArgsA = 4;
      else
         combine->_NumArgsA = 2;
      break;
   case GL_MODULATE:
   case GL_SUBTRACT:
      combine->_NumArgsA = 2;
      break;
   case GL_INTERPOLATE:
   case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
   case GL_MODULATE_SUBTRACT_ATI:
      combine->_NumArgsA = 3;
      break;
   default:
      combine->_NumArgsA = 0;
      _mesa_problem(ctx, "invalid Alpha combine mode in update_texture_state");
      break;
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
update_texture_state( struct gl_context *ctx )
{
   /* TODO: only set this if there are actual changes */
   ctx->NewState |= _NEW_TEXTURE;

   ctx->Texture._Enabled = GL_FALSE;
   ctx->Texture._GenFlags = 0x0;
   ctx->Texture._TexMatEnabled = GL_FALSE;
   ctx->Texture._TexGenEnabled = GL_FALSE;

   /*
    * Update texture unit state.
    */
   {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit;
      GLbitfield enabledTargets = texUnit->Enabled;

      texUnit->_ReallyEnabled = 0x0;

      if (enabledTargets) {
         GLuint texIndex;
         /* Look for the highest priority texture target that's enabled (or used
          * by the vert/frag shaders) and "complete".  That's the one we'll use
          * for texturing.  If we're using vert/frag program we're guaranteed
          * that bitcount(enabledBits) <= 1.
          * Note that the TEXTURE_x_INDEX values are in high to low priority.
          */
         for (texIndex = 0; texIndex < NUM_TEXTURE_TARGETS; texIndex++) {
            if (enabledTargets & (1 << texIndex)) {
               struct gl_texture_object *texObj = texUnit->CurrentTex[texIndex];
               if (!texObj->_Complete) {
                  _mesa_test_texobj_completeness(ctx, texObj);
               }
               if (texObj->_Complete) {
                  texUnit->_ReallyEnabled = 1 << texIndex;
                  _mesa_reference_texobj(&texUnit->_Current, texObj);
                  break;
               }
            }
         }

         if (texUnit->_ReallyEnabled) {
            /* if we get here, we know this texture unit is enabled */         
            ctx->Texture._Enabled = GL_TRUE;
            update_tex_combine(ctx, texUnit);
         }
      }
   }


   /* Determine if texture coordinates are actually needed */
   ctx->Texture._EnabledCoord = ctx->Texture._Enabled;

   /* Setup texgen for those texture coordinate sets that are in use */
   if(ctx->Texture._EnabledCoord)
   {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit;

      texUnit->_GenFlags = 0x0;

      if (texUnit->TexGenEnabled) {
	 if (texUnit->TexGenEnabled & S_BIT) {
	    texUnit->_GenFlags |= texUnit->GenS._ModeBit;
	 }
	 if (texUnit->TexGenEnabled & T_BIT) {
	    texUnit->_GenFlags |= texUnit->GenT._ModeBit;
	 }
	 if (texUnit->TexGenEnabled & R_BIT) {
	    texUnit->_GenFlags |= texUnit->GenR._ModeBit;
	 }
	 if (texUnit->TexGenEnabled & Q_BIT) {
	    texUnit->_GenFlags |= texUnit->GenQ._ModeBit;
	 }

	 ctx->Texture._TexGenEnabled = GL_TRUE;
	 ctx->Texture._GenFlags |= texUnit->_GenFlags;
      }

      if (ctx->TextureMatrixStack.Top->type != MATRIX_IDENTITY)
         ctx->Texture._TexMatEnabled = GL_TRUE;
   }
}


/**
 * Update texture-related derived state.
 */
void
_mesa_update_texture( struct gl_context *ctx, GLuint new_state )
{
   if (new_state & _NEW_TEXTURE_MATRIX)
      update_texture_matrices( ctx );

   if (new_state & _NEW_TEXTURE)
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
alloc_proxy_textures( struct gl_context *ctx )
{
   /* NOTE: these values must be in the same order as the TEXTURE_x_INDEX
    * values!
    */
   static const GLenum targets[] = {
      GL_TEXTURE_CUBE_MAP_ARB,
      GL_TEXTURE_2D,
      GL_TEXTURE_1D,
   };
   GLint tgt;

   STATIC_ASSERT(Elements(targets) == NUM_TEXTURE_TARGETS);
   assert(targets[TEXTURE_2D_INDEX] == GL_TEXTURE_2D);
   assert(targets[TEXTURE_CUBE_INDEX] == GL_TEXTURE_CUBE_MAP);

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
init_texture_unit( struct gl_context *ctx)
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit;
   GLuint tex;

   texUnit->EnvMode = GL_MODULATE;
   ASSIGN_4V( texUnit->EnvColor, 0.0, 0.0, 0.0, 0.0 );

   texUnit->Combine = default_combine_state;
   texUnit->_EnvMode = default_combine_state;
   texUnit->_CurrentCombine = & texUnit->_EnvMode;

   texUnit->TexGenEnabled = 0x0;
   texUnit->GenS.Mode = GL_EYE_LINEAR;
   texUnit->GenT.Mode = GL_EYE_LINEAR;
   texUnit->GenR.Mode = GL_EYE_LINEAR;
   texUnit->GenQ.Mode = GL_EYE_LINEAR;
   texUnit->GenS._ModeBit = TEXGEN_EYE_LINEAR;
   texUnit->GenT._ModeBit = TEXGEN_EYE_LINEAR;
   texUnit->GenR._ModeBit = TEXGEN_EYE_LINEAR;
   texUnit->GenQ._ModeBit = TEXGEN_EYE_LINEAR;

   /* Yes, these plane coefficients are correct! */
   ASSIGN_4V( texUnit->GenS.ObjectPlane, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenT.ObjectPlane, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenR.ObjectPlane, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenQ.ObjectPlane, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenS.EyePlane, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenT.EyePlane, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenR.EyePlane, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->GenQ.EyePlane, 0.0, 0.0, 0.0, 0.0 );

   /* initialize current texture object ptrs to the shared default objects */
   for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
      _mesa_reference_texobj(&texUnit->CurrentTex[tex],
                             ctx->Shared->DefaultTex[tex]);
   }
}


/**
 * Initialize texture state for the given context.
 */
GLboolean
_mesa_init_texture(struct gl_context *ctx)
{
   /* Texture group */
   ctx->Texture._Enabled = GL_FALSE;

   init_texture_unit(ctx);

   /* Allocate proxy textures */
   if (!alloc_proxy_textures( ctx ))
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Free dynamically-allocted texture data attached to the given context.
 */
void
_mesa_free_texture_data(struct gl_context *ctx)
{
   GLuint tgt;

   /* unreference current textures */
   /* The _Current texture could account for another reference */
   _mesa_reference_texobj(&ctx->Texture.Unit._Current, NULL);

   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      _mesa_reference_texobj(&ctx->Texture.Unit.CurrentTex[tgt], NULL);
   }

   /* Free proxy texture objects */
   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++)
      ctx->Driver.DeleteTexture(ctx, ctx->Texture.ProxyTex[tgt]);

#if FEATURE_sampler_objects
   _mesa_reference_sampler_object(ctx, &ctx->Texture.Unit.Sampler, NULL);
#endif
}


/**
 * Update the default texture objects in the given context to reference those
 * specified in the shared state and release those referencing the old 
 * shared state.
 */
void
_mesa_update_default_objects_texture(struct gl_context *ctx)
{
   GLuint tex;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit;
   for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
      _mesa_reference_texobj(&texUnit->CurrentTex[tex],
                             ctx->Shared->DefaultTex[tex]);
   }
}
