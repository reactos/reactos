/*
 * GLX Hardware Device Driver for Intel i810
 * Copyright (C) 1999 Keith Whitwell
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
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "texformat.h"
#include "simple_list.h"
#include "enums.h"

#include "mm.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810tex.h"
#include "i810state.h"
#include "i810ioctl.h"




static void i810SetTexImages( i810ContextPtr imesa, 
			      struct gl_texture_object *tObj )
{
   GLuint height, width, pitch, i, textureFormat, log_pitch;
   i810TextureObjectPtr t = (i810TextureObjectPtr) tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint numLevels;
   GLint log2Width, log2Height;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   t->texelBytes = 2;
   switch (baseImage->TexFormat->MesaFormat) {
   case MESA_FORMAT_ARGB1555:
      textureFormat = MI1_FMT_16BPP | MI1_PF_16BPP_ARGB1555;
      break;
   case MESA_FORMAT_ARGB4444:
      textureFormat = MI1_FMT_16BPP | MI1_PF_16BPP_ARGB4444;
      break;
   case MESA_FORMAT_RGB565:
      textureFormat = MI1_FMT_16BPP | MI1_PF_16BPP_RGB565;
      break;
   case MESA_FORMAT_AL88:
      textureFormat = MI1_FMT_16BPP | MI1_PF_16BPP_AY88;
      break;
   case MESA_FORMAT_YCBCR:
      textureFormat = MI1_FMT_422 | MI1_PF_422_YCRCB_SWAP_Y
	  | MI1_COLOR_CONV_ENABLE;
      break;
   case MESA_FORMAT_YCBCR_REV:
      textureFormat = MI1_FMT_422 | MI1_PF_422_YCRCB
	  | MI1_COLOR_CONV_ENABLE;
      break;
   case MESA_FORMAT_CI8:
      textureFormat = MI1_FMT_8CI | MI1_PF_8CI_ARGB4444;
      t->texelBytes = 1;
      break;

   default:
      fprintf(stderr, "i810SetTexImages: bad image->Format\n" );
      return;
   }

   driCalculateTextureFirstLastLevel( (driTextureObject *) t );

   numLevels = t->base.lastLevel - t->base.firstLevel + 1;

   log2Width = tObj->Image[0][t->base.firstLevel]->WidthLog2;
   log2Height = tObj->Image[0][t->base.firstLevel]->HeightLog2;

   /* Figure out the amount of memory required to hold all the mipmap
    * levels.  Choose the smallest pitch to accomodate the largest
    * mipmap:
    */
   width = tObj->Image[0][t->base.firstLevel]->Width * t->texelBytes;
   for (pitch = 32, log_pitch=2 ; pitch < width ; pitch *= 2 )
      log_pitch++;
   
   /* All images must be loaded at this pitch.  Count the number of
    * lines required:
    */
   for ( height = i = 0 ; i < numLevels ; i++ ) {
      t->image[i].image = tObj->Image[0][t->base.firstLevel + i];
      t->image[i].offset = height * pitch;
      t->image[i].internalFormat = baseImage->_BaseFormat;
      height += t->image[i].image->Height;
   }

   t->Pitch = pitch;
   t->base.totalSize = height*pitch;
   t->max_level = i-1;
   t->dirty = I810_UPLOAD_TEX0 | I810_UPLOAD_TEX1;   
   t->Setup[I810_TEXREG_MI1] = (MI1_MAP_0 | textureFormat | log_pitch); 
   t->Setup[I810_TEXREG_MLL] = (GFX_OP_MAP_LOD_LIMITS |
				MLL_MAP_0  |
				MLL_UPDATE_MAX_MIP | 
				MLL_UPDATE_MIN_MIP |
				((numLevels - 1) << MLL_MIN_MIP_SHIFT));

   LOCK_HARDWARE( imesa );
   i810UploadTexImagesLocked( imesa, t );
   UNLOCK_HARDWARE( imesa );
}

/* ================================================================
 * Texture combine functions
 */


static void set_color_stage( unsigned color, int stage,
			      i810ContextPtr imesa )
{
   if ( color != imesa->Setup[I810_CTXREG_MC0 + stage] ) {
      I810_STATECHANGE( imesa, I810_UPLOAD_CTX );
      imesa->Setup[I810_CTXREG_MC0 + stage] = color;
   }
}


static void set_alpha_stage( unsigned alpha, int stage,
				    i810ContextPtr imesa )
{
   if ( alpha != imesa->Setup[I810_CTXREG_MA0 + stage] ) {
      I810_STATECHANGE( imesa, I810_UPLOAD_CTX );
      imesa->Setup[I810_CTXREG_MA0 + stage] = alpha;
   }
}


static const unsigned operand_modifiers[] = {
   0,                       MC_ARG_INVERT,
   MC_ARG_REPLICATE_ALPHA,  MC_ARG_INVERT | MC_ARG_REPLICATE_ALPHA
};

/**
 * Configure the hardware bits for the specified texture environment.
 *
 * Configures the hardware bits for the texture environment state for the
 * specified texture unit.  As combine stages are added, the values pointed
 * to by \c color_stage and \c alpha_stage are incremented.
 *
 * \param ctx          GL context pointer.
 * \param unit         Texture unit to be added.
 * \param color_stage  Next available hardware color combine stage.
 * \param alpha_stage  Next available hardware alpha combine stage.
 *
 * \returns
 * If the combine mode for the specified texture unit could be added without
 * requiring a software fallback, \c GL_TRUE is returned.  Otherwise,
 * \c GL_FALSE is returned.
 *
 * \todo
 * If the mode is (GL_REPLACE, GL_PREVIOUS), treat it as though the texture
 * stage is disabled.  That is, don't emit any combine stages.
 *
 * \todo
 * Add support for ATI_texture_env_combine3 modes.  This will require using
 * two combine stages.
 *
 * \todo
 * Add support for the missing \c GL_INTERPOLATE modes.  This will require
 * using all three combine stages.  There is a comment in the function
 * describing how this might work.
 *
 * \todo
 * If, after all the combine stages have been emitted, a texture is never
 * actually used, disable the texture unit.  That should save texture some
 * memory bandwidth.  This won't happen in this function, but this seems like
 * a reasonable place to make note of it.
 */
static GLboolean
i810UpdateTexEnvCombine( GLcontext *ctx, GLuint unit, 
			 int * color_stage, int * alpha_stage )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint color_arg[3] = {
      MC_ARG_ONE,            MC_ARG_ONE,            MC_ARG_ONE
   };
   GLuint alpha_arg[3] = {
      MA_ARG_ITERATED_ALPHA, MA_ARG_ITERATED_ALPHA, MA_ARG_ITERATED_ALPHA
   };
   GLuint i;
   GLuint color_combine, alpha_combine;
   const GLuint numColorArgs = texUnit->_CurrentCombine->_NumArgsRGB;
   const GLuint numAlphaArgs = texUnit->_CurrentCombine->_NumArgsA;
   GLuint RGBshift = texUnit->_CurrentCombine->ScaleShiftRGB;
   GLuint Ashift = texUnit->_CurrentCombine->ScaleShiftA;


   if ( !texUnit->_ReallyEnabled ) {
      return GL_TRUE;
   }

      
   if ((*color_stage >= 3) || (*alpha_stage >= 3)) {
      return GL_FALSE;
   }


   /* Step 1:
    * Extract the color and alpha combine function arguments.
    */

   for ( i = 0 ; i < numColorArgs ; i++ ) {
      unsigned op = texUnit->_CurrentCombine->OperandRGB[i] - GL_SRC_COLOR;
      assert(op >= 0);
      assert(op <= 3);
      switch ( texUnit->_CurrentCombine->SourceRGB[i] ) {
      case GL_TEXTURE0:
	 color_arg[i] = MC_ARG_TEX0_COLOR;
	 break;
      case GL_TEXTURE1:
	 color_arg[i] = MC_ARG_TEX1_COLOR;
	 break;
      case GL_TEXTURE:
	 color_arg[i] = (unit == 0) 
	   ? MC_ARG_TEX0_COLOR : MC_ARG_TEX1_COLOR;
	 break;
      case GL_CONSTANT:
	 color_arg[i] = MC_ARG_COLOR_FACTOR;
	 break;
      case GL_PRIMARY_COLOR:
	 color_arg[i] = MC_ARG_ITERATED_COLOR;
	 break;
      case GL_PREVIOUS:
	 color_arg[i] = (unit == 0)
	   ? MC_ARG_ITERATED_COLOR : MC_ARG_CURRENT_COLOR;
	 break;
      case GL_ZERO:
	 /* Toggle the low bit of the op value.  The is the 'invert' bit,
	  * and it acts to convert GL_ZERO+op to the equivalent GL_ONE+op.
	  */
	 op ^= 1;

	 /*FALLTHROUGH*/

      case GL_ONE:
	 color_arg[i] = MC_ARG_ONE;
	 break;
      default:
	 return GL_FALSE;
      }

      color_arg[i] |= operand_modifiers[op];
   }


   for ( i = 0 ; i < numAlphaArgs ; i++ ) {
      unsigned op = texUnit->_CurrentCombine->OperandA[i] - GL_SRC_ALPHA;
      assert(op >= 0);
      assert(op <= 1);
      switch ( texUnit->_CurrentCombine->SourceA[i] ) {
      case GL_TEXTURE0:
	 alpha_arg[i] = MA_ARG_TEX0_ALPHA;
	 break;
      case GL_TEXTURE1:
	 alpha_arg[i] = MA_ARG_TEX1_ALPHA;
	 break;
      case GL_TEXTURE:
	 alpha_arg[i] = (unit == 0)
	   ? MA_ARG_TEX0_ALPHA : MA_ARG_TEX1_ALPHA;
	 break;
      case GL_CONSTANT:
	 alpha_arg[i] = MA_ARG_ALPHA_FACTOR;
	 break;
      case GL_PRIMARY_COLOR:
	 alpha_arg[i] = MA_ARG_ITERATED_ALPHA;
	 break;
      case GL_PREVIOUS:
	 alpha_arg[i] = (unit == 0)
	   ? MA_ARG_ITERATED_ALPHA : MA_ARG_CURRENT_ALPHA;
	 break;
      case GL_ZERO:
	 /* Toggle the low bit of the op value.  The is the 'invert' bit,
	  * and it acts to convert GL_ZERO+op to the equivalent GL_ONE+op.
	  */
	 op ^= 1;

	 /*FALLTHROUGH*/

      case GL_ONE:
	 if (i != 2) {
	    return GL_FALSE;
	 }

	 alpha_arg[i] = MA_ARG_ONE;
	 break;
      default:
	 return GL_FALSE;
      }

      alpha_arg[i] |= operand_modifiers[op];
   }


   /* Step 2:
    * Build up the color and alpha combine functions.
    */
   switch ( texUnit->_CurrentCombine->ModeRGB ) {
   case GL_REPLACE:
      color_combine = MC_OP_ARG1;
      break;
   case GL_MODULATE:
      color_combine = MC_OP_MODULATE + RGBshift;
      RGBshift = 0;
      break;
   case GL_ADD:
      color_combine = MC_OP_ADD;
      break;
   case GL_ADD_SIGNED:
      color_combine = MC_OP_ADD_SIGNED;
      break;
   case GL_SUBTRACT:
      color_combine = MC_OP_SUBTRACT;
      break;
   case GL_INTERPOLATE:
      /* For interpolation, the i810 hardware has some limitations.  It
       * can't handle using the secondary or diffuse color (diffuse alpha
       * is okay) for the third argument.
       *
       * It is possible to emulate the missing modes by using multiple
       * combine stages.  Unfortunately it requires all three stages to
       * emulate a single interpolate stage.  The (arg0*arg2) portion is
       * done in stage zero and writes to MC_DEST_ACCUMULATOR.  The
       * (arg1*(1-arg2)) portion is done in stage 1, and the final stage is
       * (MC_ARG1_ACCUMULATOR | MC_ARG2_CURRENT_COLOR | MC_OP_ADD).
       * 
       * It can also be done without using the accumulator by rearranging
       * the equation as (arg1 + (arg2 * (arg0 - arg1))).  Too bad the i810
       * doesn't support the MODULATE_AND_ADD mode that the i830 supports.
       * If it did, the interpolate could be done in only two stages.
       */
	 
      if ( (color_arg[2] & MC_ARG_INVERT) != 0 ) {
	 unsigned temp = color_arg[0];

	 color_arg[0] = color_arg[1];
	 color_arg[1] = temp;
	 color_arg[2] &= ~MC_ARG_INVERT;
      }

      switch (color_arg[2]) {
      case (MC_ARG_ONE):
      case (MC_ARG_ONE | MC_ARG_REPLICATE_ALPHA):
	 color_combine = MC_OP_ARG1;
	 color_arg[1] = MC_ARG_ONE;
	 break;

      case (MC_ARG_COLOR_FACTOR):
	 return GL_FALSE;

      case (MC_ARG_COLOR_FACTOR | MC_ARG_REPLICATE_ALPHA):
	 color_combine = MC_OP_LIN_BLEND_ALPHA_FACTOR;
	 break;

      case (MC_ARG_ITERATED_COLOR):
	 return GL_FALSE;

      case (MC_ARG_ITERATED_COLOR | MC_ARG_REPLICATE_ALPHA):
	 color_combine = MC_OP_LIN_BLEND_ITER_ALPHA;
	 break;

      case (MC_ARG_SPECULAR_COLOR):
      case (MC_ARG_SPECULAR_COLOR | MC_ARG_REPLICATE_ALPHA):
	 return GL_FALSE;

      case (MC_ARG_TEX0_COLOR):
	 color_combine = MC_OP_LIN_BLEND_TEX0_COLOR;
	 break;

      case (MC_ARG_TEX0_COLOR | MC_ARG_REPLICATE_ALPHA):
	 color_combine = MC_OP_LIN_BLEND_TEX0_ALPHA;
	 break;

      case (MC_ARG_TEX1_COLOR):
	 color_combine = MC_OP_LIN_BLEND_TEX1_COLOR;
	 break;

      case (MC_ARG_TEX1_COLOR | MC_ARG_REPLICATE_ALPHA):
	 color_combine = MC_OP_LIN_BLEND_TEX1_ALPHA;
	 break;

      default:
	 return GL_FALSE;
      }
      break;

   default:
      return GL_FALSE;
   }

   
   switch ( texUnit->_CurrentCombine->ModeA ) {
   case GL_REPLACE:
      alpha_combine = MA_OP_ARG1;
      break;
   case GL_MODULATE:
      alpha_combine = MA_OP_MODULATE + Ashift;
      Ashift = 0;
      break;
   case GL_ADD:
      alpha_combine = MA_OP_ADD;
      break;
   case GL_ADD_SIGNED:
      alpha_combine = MA_OP_ADD_SIGNED;
      break;
   case GL_SUBTRACT:
      alpha_combine = MA_OP_SUBTRACT;
      break;
   case GL_INTERPOLATE:
      if ( (alpha_arg[2] & MA_ARG_INVERT) != 0 ) {
	 unsigned temp = alpha_arg[0];

	 alpha_arg[0] = alpha_arg[1];
	 alpha_arg[1] = temp;
	 alpha_arg[2] &= ~MA_ARG_INVERT;
      }

      switch (alpha_arg[2]) {
      case MA_ARG_ONE:
	 alpha_combine = MA_OP_ARG1;
	 alpha_arg[1] = MA_ARG_ITERATED_ALPHA;
	 break;

      case MA_ARG_ALPHA_FACTOR:
	 alpha_combine = MA_OP_LIN_BLEND_ALPHA_FACTOR;
	 break;

      case MA_ARG_ITERATED_ALPHA:
	 alpha_combine = MA_OP_LIN_BLEND_ITER_ALPHA;
	 break;

      case MA_ARG_TEX0_ALPHA:
	 alpha_combine = MA_OP_LIN_BLEND_TEX0_ALPHA;
	 break;

      case MA_ARG_TEX1_ALPHA:
	 alpha_combine = MA_OP_LIN_BLEND_TEX1_ALPHA;
	 break;

      default:
	 return GL_FALSE;
      }
      break;

   default:
      return GL_FALSE;
   }


   color_combine |= GFX_OP_MAP_COLOR_STAGES | (*color_stage << MC_STAGE_SHIFT)
     | MC_UPDATE_DEST | MC_DEST_CURRENT
     | MC_UPDATE_ARG1 | (color_arg[0] << MC_ARG1_SHIFT)
     | MC_UPDATE_ARG2 | (color_arg[1] << MC_ARG2_SHIFT)
     | MC_UPDATE_OP;

   alpha_combine |= GFX_OP_MAP_ALPHA_STAGES | (*alpha_stage << MA_STAGE_SHIFT)
     | MA_UPDATE_ARG1 | (alpha_arg[0] << MA_ARG1_SHIFT)
     | MA_UPDATE_ARG2 | (alpha_arg[1] << MA_ARG2_SHIFT)
     | MA_UPDATE_OP;

   set_color_stage( color_combine, *color_stage, imesa );
   set_alpha_stage( alpha_combine, *alpha_stage, imesa );
   (*color_stage)++;
   (*alpha_stage)++;


   /* Step 3:
    * Apply the scale factor.
    */
   /* The only operation where the i810 directly supports adding a post-
    * scale factor is modulate.  For all the other modes the post-scale is
    * emulated by inserting and extra modulate stage.  For the modulate
    * case, the scaling is handled above when color_combine / alpha_combine
    * are initially set.
    */

   if ( RGBshift != 0 ) {
      const unsigned color_scale = GFX_OP_MAP_COLOR_STAGES
	| (*color_stage << MC_STAGE_SHIFT)
	| MC_UPDATE_DEST | MC_DEST_CURRENT
	| MC_UPDATE_ARG1 | (MC_ARG_CURRENT_COLOR << MC_ARG1_SHIFT)
	| MC_UPDATE_ARG2 | (MC_ARG_ONE           << MC_ARG2_SHIFT)
	| MC_UPDATE_OP   | (MC_OP_MODULATE + RGBshift);

      if ( *color_stage >= 3 ) {
	 return GL_FALSE;
      }

      set_color_stage( color_scale, *color_stage, imesa );
      (*color_stage)++;
   }

   
   if ( Ashift != 0 ) {
      const unsigned alpha_scale = GFX_OP_MAP_ALPHA_STAGES
	| (*alpha_stage << MA_STAGE_SHIFT)
	| MA_UPDATE_ARG1 | (MA_ARG_CURRENT_ALPHA << MA_ARG1_SHIFT)
	| MA_UPDATE_ARG2 | (MA_ARG_ONE           << MA_ARG2_SHIFT)
	| MA_UPDATE_OP   | (MA_OP_MODULATE + Ashift);

      if ( *alpha_stage >= 3 ) {
	 return GL_FALSE;
      }

      set_alpha_stage( alpha_scale, *alpha_stage, imesa );
      (*alpha_stage)++;
   }

   return GL_TRUE;
}

static GLboolean enable_tex_common( GLcontext *ctx, GLuint unit )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   i810TextureObjectPtr t = (i810TextureObjectPtr)tObj->DriverData;

   if (tObj->Image[0][tObj->BaseLevel]->Border > 0) {
     return GL_FALSE;
   }

  /* Upload teximages (not pipelined)
   */
  if (t->base.dirty_images[0]) {
    I810_FIREVERTICES(imesa);
    i810SetTexImages( imesa, tObj );
    if (!t->base.memBlock) {
      return GL_FALSE;
    }
  }
   
  /* Update state if this is a different texture object to last
   * time.
   */
  if (imesa->CurrentTexObj[unit] != t) {
    I810_STATECHANGE(imesa, (I810_UPLOAD_TEX0<<unit));
    imesa->CurrentTexObj[unit] = t;
    t->base.bound |= (1U << unit);
    
    /* XXX: should be locked */
    driUpdateTextureLRU( (driTextureObject *) t );
  }
  
  imesa->TexEnvImageFmt[unit] = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;
  return GL_TRUE;
}

static GLboolean enable_tex_rect( GLcontext *ctx, GLuint unit )
{
  i810ContextPtr imesa = I810_CONTEXT(ctx);
  struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
  struct gl_texture_object *tObj = texUnit->_Current;
  i810TextureObjectPtr t = (i810TextureObjectPtr)tObj->DriverData;
  GLint Width, Height;

  Width = tObj->Image[0][t->base.firstLevel]->Width - 1;
  Height = tObj->Image[0][t->base.firstLevel]->Height - 1;

  I810_STATECHANGE(imesa, (I810_UPLOAD_TEX0<<unit));
  t->Setup[I810_TEXREG_MCS] &= ~MCS_NORMALIZED_COORDS;
  t->Setup[I810_TEXREG_MCS] |= MCS_UPDATE_NORMALIZED; 
  t->Setup[I810_TEXREG_MI2] = (MI2_DIMENSIONS_ARE_EXACT |
			       (Height << MI2_HEIGHT_SHIFT) | Width);
  
  return GL_TRUE;
}

static GLboolean enable_tex_2d( GLcontext *ctx, GLuint unit )
{
  i810ContextPtr imesa = I810_CONTEXT(ctx);
  struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
  struct gl_texture_object *tObj = texUnit->_Current;
  i810TextureObjectPtr t = (i810TextureObjectPtr)tObj->DriverData;
  GLint log2Width, log2Height;


  log2Width = tObj->Image[0][t->base.firstLevel]->WidthLog2;
  log2Height = tObj->Image[0][t->base.firstLevel]->HeightLog2;

  I810_STATECHANGE(imesa, (I810_UPLOAD_TEX0<<unit));
  t->Setup[I810_TEXREG_MCS] |= MCS_NORMALIZED_COORDS | MCS_UPDATE_NORMALIZED; 
  t->Setup[I810_TEXREG_MI2] = (MI2_DIMENSIONS_ARE_LOG2 |
			       (log2Height << MI2_HEIGHT_SHIFT) | log2Width);
  
  return GL_TRUE;
}

static void disable_tex( GLcontext *ctx, GLuint unit )
{
  i810ContextPtr imesa = I810_CONTEXT(ctx);

  imesa->CurrentTexObj[unit] = 0;
  imesa->TexEnvImageFmt[unit] = 0;	
  imesa->dirty &= ~(I810_UPLOAD_TEX0<<unit); 
  
}

/**
 * Update hardware state for a texture unit.
 *
 * \todo
 * 1D textures should be supported!  Just use a 2D texture with the second
 * texture coordinate value fixed at 0.0.
 */
static void i810UpdateTexUnit( GLcontext *ctx, GLuint unit, 
			      int * next_color_stage, int * next_alpha_stage )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLboolean ret;
   
   switch(texUnit->_ReallyEnabled) {
   case TEXTURE_2D_BIT:
     ret = enable_tex_common( ctx, unit);
     ret &= enable_tex_2d(ctx, unit);
     if (ret == GL_FALSE) {
       FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_TRUE );
     }
     break;
   case TEXTURE_RECT_BIT:
     ret = enable_tex_common( ctx, unit);
     ret &= enable_tex_rect(ctx, unit);
     if (ret == GL_FALSE) {
       FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_TRUE );
     }
     break;
   case 0:
     disable_tex(ctx, unit);
     break;
   }


   if (!i810UpdateTexEnvCombine( ctx, unit, 
				 next_color_stage, next_alpha_stage )) {
     FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_TRUE );
   }

   return;
}


void i810UpdateTextureState( GLcontext *ctx )
{
   static const unsigned color_pass[3] = {
      GFX_OP_MAP_COLOR_STAGES | MC_STAGE_0 | MC_UPDATE_DEST | MC_DEST_CURRENT
	| MC_UPDATE_ARG1 | (MC_ARG_ITERATED_COLOR << MC_ARG1_SHIFT)
	| MC_UPDATE_ARG2 | (MC_ARG_ONE            << MC_ARG2_SHIFT)
	| MC_UPDATE_OP   | MC_OP_ARG1,
      GFX_OP_MAP_COLOR_STAGES | MC_STAGE_1 | MC_UPDATE_DEST | MC_DEST_CURRENT
	| MC_UPDATE_ARG1 | (MC_ARG_CURRENT_COLOR  << MC_ARG1_SHIFT)
	| MC_UPDATE_ARG2 | (MC_ARG_ONE            << MC_ARG2_SHIFT)
	| MC_UPDATE_OP   | MC_OP_ARG1,
      GFX_OP_MAP_COLOR_STAGES | MC_STAGE_2 | MC_UPDATE_DEST | MC_DEST_CURRENT
	| MC_UPDATE_ARG1 | (MC_ARG_CURRENT_COLOR  << MC_ARG1_SHIFT)
	| MC_UPDATE_ARG2 | (MC_ARG_ONE            << MC_ARG2_SHIFT)
	| MC_UPDATE_OP   | MC_OP_ARG1
   };
   static const unsigned alpha_pass[3] = {
      GFX_OP_MAP_ALPHA_STAGES | MA_STAGE_0
	| MA_UPDATE_ARG1 | (MA_ARG_ITERATED_ALPHA << MA_ARG1_SHIFT)
	| MA_UPDATE_ARG2 | (MA_ARG_ITERATED_ALPHA << MA_ARG2_SHIFT)
	| MA_UPDATE_OP   | MA_OP_ARG1,
      GFX_OP_MAP_ALPHA_STAGES | MA_STAGE_1
	| MA_UPDATE_ARG1 | (MA_ARG_CURRENT_ALPHA  << MA_ARG1_SHIFT)
	| MA_UPDATE_ARG2 | (MA_ARG_CURRENT_ALPHA  << MA_ARG2_SHIFT)
	| MA_UPDATE_OP   | MA_OP_ARG1,
      GFX_OP_MAP_ALPHA_STAGES | MA_STAGE_2
	| MA_UPDATE_ARG1 | (MA_ARG_CURRENT_ALPHA  << MA_ARG1_SHIFT)
	| MA_UPDATE_ARG2 | (MA_ARG_CURRENT_ALPHA  << MA_ARG2_SHIFT)
	| MA_UPDATE_OP   | MA_OP_ARG1
   };
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   int next_color_stage = 0;
   int next_alpha_stage = 0;


   /*  fprintf(stderr, "%s\n", __FUNCTION__); */
   FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_FALSE );

   i810UpdateTexUnit( ctx, 0, & next_color_stage, & next_alpha_stage );
   i810UpdateTexUnit( ctx, 1, & next_color_stage, & next_alpha_stage );

   /* There needs to be at least one combine stage emitted that just moves
    * the incoming primary color to the current color register.  In addition,
    * there number be the same number of color and alpha stages emitted.
    * Finally, if there are less than 3 combine stages, a MC_OP_DISABLE stage
    * must be emitted.
    */

   while ( (next_color_stage == 0) ||
	   (next_color_stage < next_alpha_stage) ) {
      set_color_stage( color_pass[ next_color_stage ], next_color_stage,
		       imesa );
      next_color_stage++;
   }

   assert( next_color_stage <= 3 );

   while ( next_alpha_stage < next_color_stage ) {
      set_alpha_stage( alpha_pass[ next_alpha_stage ], next_alpha_stage,
		       imesa );
      next_alpha_stage++;
   }

   assert( next_alpha_stage <= 3 );
   assert( next_color_stage == next_alpha_stage );

   if ( next_color_stage < 3 ) {
      const unsigned color = GFX_OP_MAP_COLOR_STAGES
	| (next_color_stage << MC_STAGE_SHIFT)
	| MC_UPDATE_DEST | MC_DEST_CURRENT
	| MC_UPDATE_ARG1 | (MC_ARG_ONE << MC_ARG1_SHIFT)
	| MC_UPDATE_ARG2 | (MC_ARG_ONE << MC_ARG2_SHIFT)
	| MC_UPDATE_OP   | (MC_OP_DISABLE);

      const unsigned alpha = GFX_OP_MAP_ALPHA_STAGES
	| (next_color_stage << MC_STAGE_SHIFT)
	| MA_UPDATE_ARG1 | (MA_ARG_CURRENT_ALPHA << MA_ARG1_SHIFT)
	| MA_UPDATE_ARG2 | (MA_ARG_CURRENT_ALPHA << MA_ARG2_SHIFT)
	| MA_UPDATE_OP   | (MA_OP_ARG1);

      set_color_stage( color, next_color_stage, imesa );
      set_alpha_stage( alpha, next_alpha_stage, imesa );
   }
}
