/**************************************************************************

Copyright 2001 2d3d Inc., Delray Beach, FL

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_texstate.c,v 1.3 2002/12/10 01:26:53 dawes Exp $ */

/**
 * \file i830_texstate.c
 * 
 * Heavily based on the I810 driver, which was written by Keith Whitwell.
 *
 * \author Jeff Hartmann <jhartmann@2d3d.com>
 * \author Keith Whitwell <keithw@tungstengraphics.com>
 */

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "texformat.h"
#include "texstore.h"

#include "mm.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_context.h"
#include "i830_tex.h"
#include "i830_state.h"
#include "i830_ioctl.h"

#define I830_TEX_UNIT_ENABLED(unit)		(1<<unit)

static void i830SetTexImages( i830ContextPtr imesa, 
			      struct gl_texture_object *tObj )
{
   GLuint total_height, pitch, i, textureFormat;
   i830TextureObjectPtr t = (i830TextureObjectPtr) tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint numLevels;

   switch( baseImage->TexFormat->MesaFormat ) {
   case MESA_FORMAT_L8:
      t->texelBytes = 1;
      textureFormat = MAPSURF_8BIT | MT_8BIT_L8;
      break;

   case MESA_FORMAT_I8:
      t->texelBytes = 1;
      textureFormat = MAPSURF_8BIT | MT_8BIT_I8;
      break;

   case MESA_FORMAT_AL88:
      t->texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_AY88;
      break;

   case MESA_FORMAT_RGB565:
      t->texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_RGB565;
      break;

   case MESA_FORMAT_ARGB1555:
      t->texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_ARGB1555;
      break;

   case MESA_FORMAT_ARGB4444:
      t->texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_ARGB4444;
      break;

   case MESA_FORMAT_ARGB8888:
      t->texelBytes = 4;
      textureFormat = MAPSURF_32BIT | MT_32BIT_ARGB8888;
      break;

   case MESA_FORMAT_YCBCR_REV:
      t->texelBytes = 2;
      textureFormat = (MAPSURF_422 | MT_422_YCRCB_NORMAL | 
		       TM0S1_COLORSPACE_CONVERSION);
      break;

   case MESA_FORMAT_YCBCR:
      t->texelBytes = 2;
      textureFormat = (MAPSURF_422 | MT_422_YCRCB_SWAPY | /* ??? */
		       TM0S1_COLORSPACE_CONVERSION);
      break;
      
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
     t->texelBytes = 2;
     textureFormat = (MAPSURF_COMPRESSED | MT_COMPRESS_FXT1);
     break;
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGB_DXT1:
     /*
      * DXTn pitches are Width/4 * blocksize in bytes
      * for DXT1: blocksize=8 so Width/4*8 = Width * 2
      * for DXT3/5: blocksize=16 so Width/4*16 = Width * 4
      */
     t->texelBytes = 2;
     textureFormat = (MAPSURF_COMPRESSED | MT_COMPRESS_DXT1);
     break;
   case MESA_FORMAT_RGBA_DXT3:
     t->texelBytes = 4;
     textureFormat = (MAPSURF_COMPRESSED | MT_COMPRESS_DXT2_3);
     break;
   case MESA_FORMAT_RGBA_DXT5:
     t->texelBytes = 4;
     textureFormat = (MAPSURF_COMPRESSED | MT_COMPRESS_DXT4_5);
     break;
   default:
      fprintf(stderr, "%s: bad image format\n", __FUNCTION__);
      free( t );
      return;
   }

   /* Compute which mipmap levels we really want to send to the hardware.
    */

   driCalculateTextureFirstLastLevel( (driTextureObject *) t );


   /* Figure out the amount of memory required to hold all the mipmap
    * levels.  Choose the smallest pitch to accomodate the largest
    * mipmap:
    */
   numLevels = t->base.lastLevel - t->base.firstLevel + 1;

   /* Pitch would be subject to additional rules if texture memory were
    * tiled.  Currently it isn't. 
    */
   if (0) {
      pitch = 128;
      while (pitch < tObj->Image[0][t->base.firstLevel]->Width * t->texelBytes)
	 pitch *= 2;
   }
   else {
      pitch = tObj->Image[0][t->base.firstLevel]->Width * t->texelBytes;
      pitch = (pitch + 3) & ~3;
   }


   /* All images must be loaded at this pitch.  Count the number of
    * lines required:
    */
   for ( total_height = i = 0 ; i < numLevels ; i++ ) {
      t->image[0][i].image = tObj->Image[0][t->base.firstLevel + i];
      if (!t->image[0][i].image) 
	 break;
      
      t->image[0][i].offset = total_height * pitch;
      if (t->image[0][i].image->IsCompressed)
	{
	  if (t->image[0][i].image->Height > 4)
	    total_height += t->image[0][i].image->Height/4;
	  else
	    total_height += 1;
	}
      else
	total_height += t->image[0][i].image->Height;
      t->image[0][i].internalFormat = baseImage->Format;
   }

   t->Pitch = pitch;
   t->base.totalSize = total_height*pitch;
   t->Setup[I830_TEXREG_TM0S1] = 
      (((tObj->Image[0][t->base.firstLevel]->Height - 1) << TM0S1_HEIGHT_SHIFT) |
       ((tObj->Image[0][t->base.firstLevel]->Width - 1) << TM0S1_WIDTH_SHIFT) |
       textureFormat);
   t->Setup[I830_TEXREG_TM0S2] = 
      ((((pitch / 4) - 1) << TM0S2_PITCH_SHIFT));   
   t->Setup[I830_TEXREG_TM0S3] &= ~TM0S3_MAX_MIP_MASK;
   t->Setup[I830_TEXREG_TM0S3] &= ~TM0S3_MIN_MIP_MASK;
   t->Setup[I830_TEXREG_TM0S3] |= ((numLevels - 1)*4) << TM0S3_MIN_MIP_SHIFT;
   t->dirty = I830_UPLOAD_TEX0 | I830_UPLOAD_TEX1 
	| I830_UPLOAD_TEX2 | I830_UPLOAD_TEX3;

   LOCK_HARDWARE( imesa );
   i830UploadTexImagesLocked( imesa, t );
   UNLOCK_HARDWARE( imesa );
}

/* ================================================================
 * Texture combine functions
 */


/**
 * Calculate the hardware instuctions to setup the current texture enviromnemt
 * settings.  Since \c gl_texture_unit::_CurrentCombine is used, both
 * "classic" texture enviroments and GL_ARB_texture_env_combine type texture
 * environments are treated identically.
 *
 * \todo
 * This function should return \c GLboolean.  When \c GL_FALSE is returned,
 * it means that an environment is selected that the hardware cannot do.  This
 * is the way the Radeon and R200 drivers work.
 * 
 * \todo
 * Looking at i830_3d_regs.h, it seems the i830 can do part of
 * GL_ATI_texture_env_combine3.  It can handle using \c GL_ONE and
 * \c GL_ZERO as combine inputs (which the code already supports).  It can
 * also handle the \c GL_MODULATE_ADD_ATI mode.  Is it worth investigating
 * partial support for the extension?
 * 
 * \todo
 * Some thought needs to be put into the way combiners work.  The driver
 * treats the hardware as if there's a specific combine unit tied to each
 * texture unit.  That's why there's the special case for a disabled texture
 * unit.  That's not the way the hardware works.  In reality, there are 4
 * texture units and four general instruction slots.  Each instruction slot
 * can use any texture as an input.  There's no need for this wierd "no-op"
 * stuff.  If texture units 0 and 3 are enabled, the  instructions to combine
 * them should be in slots 0 and 1, not 0 and 3 with two no-ops inbetween.
 */

static void i830UpdateTexEnv( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   const GLuint numColorArgs = texUnit->_CurrentCombine->_NumArgsRGB;
   const GLuint numAlphaArgs = texUnit->_CurrentCombine->_NumArgsA;

   GLboolean need_constant_color = GL_FALSE;
   GLuint blendop;
   GLuint ablendop;
   GLuint args_RGB[3];
   GLuint args_A[3];
   GLuint rgb_shift = texUnit->Combine.ScaleShiftRGB;
   GLuint alpha_shift = texUnit->Combine.ScaleShiftA;
   int i;
   unsigned used;
   static const GLuint tex_blend_rgb[3] = {
      TEXPIPE_COLOR | TEXBLEND_ARG1 | TEXBLENDARG_MODIFY_PARMS,
      TEXPIPE_COLOR | TEXBLEND_ARG2 | TEXBLENDARG_MODIFY_PARMS,
      TEXPIPE_COLOR | TEXBLEND_ARG0 | TEXBLENDARG_MODIFY_PARMS,
   };
   static const GLuint tex_blend_a[3] = {
      TEXPIPE_ALPHA | TEXBLEND_ARG1 | TEXBLENDARG_MODIFY_PARMS,
      TEXPIPE_ALPHA | TEXBLEND_ARG2 | TEXBLENDARG_MODIFY_PARMS,
      TEXPIPE_ALPHA | TEXBLEND_ARG0 | TEXBLENDARG_MODIFY_PARMS,
   };
   static const GLuint op_rgb[4] = {
      0,
      TEXBLENDARG_INV_ARG,
      TEXBLENDARG_REPLICATE_ALPHA,
      TEXBLENDARG_REPLICATE_ALPHA | TEXBLENDARG_INV_ARG,
   };



   imesa->TexBlendWordsUsed[unit] = 0;

   if(I830_DEBUG&DEBUG_TEXTURE)
       fprintf(stderr, "[%s:%u] env. mode = %s\n",  __FUNCTION__, __LINE__, 
	       _mesa_lookup_enum_by_nr(texUnit->EnvMode));


   if ( !texUnit->_ReallyEnabled ) {
      imesa->TexBlend[unit][0] = (STATE3D_MAP_BLEND_OP_CMD(unit) |
				  TEXPIPE_COLOR |
				  ENABLE_TEXOUTPUT_WRT_SEL |
				  TEXOP_OUTPUT_CURRENT |
				  DISABLE_TEX_CNTRL_STAGE |
				  TEXOP_SCALE_1X |
				  TEXOP_MODIFY_PARMS |
				  TEXBLENDOP_ARG1);
      imesa->TexBlend[unit][1] = (STATE3D_MAP_BLEND_OP_CMD(unit) |
				  TEXPIPE_ALPHA |
				  ENABLE_TEXOUTPUT_WRT_SEL |
				  TEXOP_OUTPUT_CURRENT |
				  TEXOP_SCALE_1X |
				  TEXOP_MODIFY_PARMS |
				  TEXBLENDOP_ARG1);
      imesa->TexBlend[unit][2] = (STATE3D_MAP_BLEND_ARG_CMD(unit) |
				  TEXPIPE_COLOR |
				  TEXBLEND_ARG1 |
				  TEXBLENDARG_MODIFY_PARMS |
				  TEXBLENDARG_CURRENT);
      imesa->TexBlend[unit][3] = (STATE3D_MAP_BLEND_ARG_CMD(unit) |
				  TEXPIPE_ALPHA |
				  TEXBLEND_ARG1 |
				  TEXBLENDARG_MODIFY_PARMS |
				  TEXBLENDARG_CURRENT);
      imesa->TexBlendWordsUsed[unit] = 4;
   }
   else {
      switch(texUnit->_CurrentCombine->ModeRGB) {
	  case GL_REPLACE: 
	 blendop = TEXBLENDOP_ARG1;
      break;
	  case GL_MODULATE: 
	 blendop = TEXBLENDOP_MODULATE;
	 break;
	  case GL_ADD: 
	 blendop = TEXBLENDOP_ADD;
	 break;
	  case GL_ADD_SIGNED:
	 blendop = TEXBLENDOP_ADDSIGNED; 
	 break;
	  case GL_INTERPOLATE:
	 blendop = TEXBLENDOP_BLEND; 
	 break;
	  case GL_SUBTRACT: 
	 blendop = TEXBLENDOP_SUBTRACT;
	 break;
	  case GL_DOT3_RGB_EXT:
	  case GL_DOT3_RGBA_EXT:
	 /* The EXT version of the DOT3 extension does not support the
	  * scale factor, but the ARB version (and the version in OpenGL
	  * 1.3) does.
	  */
	 rgb_shift = 0;
	 alpha_shift = 0;
	 /* FALLTHROUGH */

	  case GL_DOT3_RGB:
	  case GL_DOT3_RGBA:
	 blendop = TEXBLENDOP_DOT3;
	 break;
	  default: 
	 return;
      }

      blendop |= (rgb_shift << TEXOP_SCALE_SHIFT);

      switch(texUnit->_CurrentCombine->ModeA) {
	  case GL_REPLACE: 
	 ablendop = TEXBLENDOP_ARG1;
	 break;
	  case GL_MODULATE: 
	 ablendop = TEXBLENDOP_MODULATE;
	 break;
	  case GL_ADD: 
	 ablendop = TEXBLENDOP_ADD;
	 break;
	  case GL_ADD_SIGNED:
	 ablendop = TEXBLENDOP_ADDSIGNED; 
	 break;
	  case GL_INTERPOLATE:
	 ablendop = TEXBLENDOP_BLEND; 
	 break;
	  case GL_SUBTRACT: 
	 ablendop = TEXBLENDOP_SUBTRACT;
	 break;
	  default:
	 return;
      }

      if ( (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT)
	   || (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) ) {
	 ablendop = TEXBLENDOP_DOT3;
      }

      ablendop |= (alpha_shift << TEXOP_SCALE_SHIFT);

      /* Handle RGB args */
      for( i = 0 ; i < numColorArgs ; i++ ) {
	 const int op = texUnit->_CurrentCombine->OperandRGB[i] - GL_SRC_COLOR;

	 assert( (op >= 0) && (op <= 3) );
	 switch(texUnit->_CurrentCombine->SourceRGB[i]) {
	     case GL_TEXTURE: 
	    args_RGB[i] = TEXBLENDARG_TEXEL0 + unit;
	    break;
	     case GL_TEXTURE0:
	     case GL_TEXTURE1: 
	     case GL_TEXTURE2: 
	     case GL_TEXTURE3:
	    args_RGB[i] = TEXBLENDARG_TEXEL0
		+ (texUnit->_CurrentCombine->SourceRGB[i] & 0x03);
	    break;
	     case GL_CONSTANT:
	    args_RGB[i] = TEXBLENDARG_FACTOR_N; 
	    need_constant_color = GL_TRUE;
	    break;
	     case GL_PRIMARY_COLOR:
	    args_RGB[i] = TEXBLENDARG_DIFFUSE;
	    break;
	     case GL_PREVIOUS:
	    args_RGB[i] = TEXBLENDARG_CURRENT; 
	    break;
	     case GL_ONE:
	    args_RGB[i] = TEXBLENDARG_ONE;
	    break;
	     case GL_ZERO:
	    args_RGB[i] = TEXBLENDARG_ONE | TEXBLENDARG_INV_ARG;
	    break;
	     default: 
	    return;
	 }

	 /* Xor is used so that GL_ONE_MINUS_SRC_COLOR with GL_ZERO
	  * works correctly.
	  */
	 args_RGB[i] ^= op_rgb[op];
      }

      /* Handle A args */
      for( i = 0 ; i < numAlphaArgs ; i++ ) {
	 const int op = texUnit->_CurrentCombine->OperandA[i] - GL_SRC_ALPHA;

	 assert( (op >= 0) && (op <= 1) );
	 switch(texUnit->_CurrentCombine->SourceA[i]) {
	     case GL_TEXTURE: 
	    args_A[i] = TEXBLENDARG_TEXEL0 + unit;
	    break;
	     case GL_TEXTURE0:
	     case GL_TEXTURE1: 
	     case GL_TEXTURE2: 
	     case GL_TEXTURE3:
	    args_A[i] = TEXBLENDARG_TEXEL0 
		+ (texUnit->_CurrentCombine->SourceA[i] & 0x03);
	    break;
	     case GL_CONSTANT:
	    args_A[i] = TEXBLENDARG_FACTOR_N; 
	    need_constant_color = GL_TRUE;
	    break;
	     case GL_PRIMARY_COLOR:
	    args_A[i] = TEXBLENDARG_DIFFUSE; 
	    break;
	     case GL_PREVIOUS:
	    args_A[i] = TEXBLENDARG_CURRENT; 
	    break;
	     case GL_ONE:
	    args_A[i] = TEXBLENDARG_ONE;
	    break;
	     case GL_ZERO:
	    args_A[i] = TEXBLENDARG_ONE | TEXBLENDARG_INV_ARG;
	    break;
	     default: 
	    return;
	 }

	 /* We cheat. :) The register values for this are the same as for
	  * RGB.  Xor is used so that GL_ONE_MINUS_SRC_ALPHA with GL_ZERO
	  * works correctly.
	  */
	 args_A[i] ^= op_rgb[op];
      }

      /* Native Arg1 == Arg0 in GL_EXT_texture_env_combine spec */
      /* Native Arg2 == Arg1 in GL_EXT_texture_env_combine spec */
      /* Native Arg0 == Arg2 in GL_EXT_texture_env_combine spec */

      /* Build color pipeline */

      used = 0;
      imesa->TexBlend[unit][used++] = (STATE3D_MAP_BLEND_OP_CMD(unit) |
				       TEXPIPE_COLOR |
				       ENABLE_TEXOUTPUT_WRT_SEL |
				       TEXOP_OUTPUT_CURRENT |
				       DISABLE_TEX_CNTRL_STAGE |
				       TEXOP_MODIFY_PARMS |
				       blendop);

      imesa->TexBlend[unit][used++] = (STATE3D_MAP_BLEND_OP_CMD(unit) |
				       TEXPIPE_ALPHA |
				       ENABLE_TEXOUTPUT_WRT_SEL |
				       TEXOP_OUTPUT_CURRENT |
				       TEXOP_MODIFY_PARMS |
				       ablendop);

      for ( i = 0 ; i < numColorArgs ; i++ ) {
	 imesa->TexBlend[unit][used++] = (STATE3D_MAP_BLEND_ARG_CMD(unit) |
					  tex_blend_rgb[i] |
					  args_RGB[i]);
      }

      for ( i = 0 ; i < numAlphaArgs ; i++ ) {
	 imesa->TexBlend[unit][used++] = (STATE3D_MAP_BLEND_ARG_CMD(unit) |
					  tex_blend_a[i] |
					  args_A[i]);
      }


      if ( need_constant_color ) {
	 GLubyte r, g, b, a;
	 const GLfloat * const fc = texUnit->EnvColor;

	 FLOAT_COLOR_TO_UBYTE_COLOR(r, fc[RCOMP]);
	 FLOAT_COLOR_TO_UBYTE_COLOR(g, fc[GCOMP]);
	 FLOAT_COLOR_TO_UBYTE_COLOR(b, fc[BCOMP]);
	 FLOAT_COLOR_TO_UBYTE_COLOR(a, fc[ACOMP]);

	 imesa->TexBlend[unit][used++] = STATE3D_COLOR_FACTOR_CMD(unit);
	 imesa->TexBlend[unit][used++] = ((a << 24) | (r << 16) | (g << 8) | b);
      }

      imesa->TexBlendWordsUsed[unit] = used;
   }

   I830_STATECHANGE( imesa, I830_UPLOAD_TEXBLEND_N(unit) );
}


/* This is bogus -- can't load the same texture object on two units.
 */
static void i830TexSetUnit( i830TextureObjectPtr t, GLuint unit )
{
   if(I830_DEBUG&DEBUG_TEXTURE)
      fprintf(stderr, "%s unit(%d)\n", __FUNCTION__, unit);
   
   t->Setup[I830_TEXREG_TM0LI] = (STATE3D_LOAD_STATE_IMMEDIATE_2 | 
				  (LOAD_TEXTURE_MAP0 << unit) | 4);

   I830_SET_FIELD(t->Setup[I830_TEXREG_MCS], MAP_UNIT_MASK, MAP_UNIT(unit));

   t->current_unit = unit;
   t->base.bound |= (1U << unit);
}

#define TEXCOORDTYPE_MASK	(~((1<<13)|(1<<12)|(1<<11)))


static GLboolean enable_tex_common( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   i830TextureObjectPtr t = (i830TextureObjectPtr)tObj->DriverData;

   /* Fallback if there's a texture border */
   if ( tObj->Image[0][tObj->BaseLevel]->Border > 0 ) {
      return GL_FALSE;
   }

   /* Upload teximages (not pipelined)
    */
   if (t->base.dirty_images[0]) {
      i830SetTexImages( imesa, tObj );
      if (!t->base.memBlock) {
	 return GL_FALSE;
      }
   }

   /* Update state if this is a different texture object to last
    * time.
    */
   if (imesa->CurrentTexObj[unit] != t) {

      if ( imesa->CurrentTexObj[unit] != NULL ) {
	 /* The old texture is no longer bound to this texture unit.
	  * Mark it as such.
	  */

	 imesa->CurrentTexObj[unit]->base.bound &= ~(1U << unit);
      }

      I830_STATECHANGE( imesa, I830_UPLOAD_TEX_N(unit) );
      imesa->CurrentTexObj[unit] = t;
      i830TexSetUnit(t, unit);
   }

   /* Update texture environment if texture object image format or 
    * texture environment state has changed. 
    *
    * KW: doesn't work -- change from tex0 only to tex0+tex1 gets
    * missed (need to update last stage flag?).  Call
    * i830UpdateTexEnv always.
    */
   if (tObj->Image[0][tObj->BaseLevel]->Format !=
       imesa->TexEnvImageFmt[unit]) {
      imesa->TexEnvImageFmt[unit] = tObj->Image[0][tObj->BaseLevel]->Format;
   }
   i830UpdateTexEnv( ctx, unit );
   imesa->TexEnabledMask |= I830_TEX_UNIT_ENABLED(unit);

   return GL_TRUE;
}

static GLboolean enable_tex_rect( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   i830TextureObjectPtr t = (i830TextureObjectPtr)tObj->DriverData;
   GLuint mcs = t->Setup[I830_TEXREG_MCS];

   mcs &= ~TEXCOORDS_ARE_NORMAL;
   mcs |= TEXCOORDS_ARE_IN_TEXELUNITS;

   if (mcs != t->Setup[I830_TEXREG_MCS]) {
      I830_STATECHANGE( imesa, I830_UPLOAD_TEX_N(unit) );
      t->Setup[I830_TEXREG_MCS] = mcs;
   }

   return GL_TRUE;
}


static GLboolean enable_tex_2d( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   i830TextureObjectPtr t = (i830TextureObjectPtr)tObj->DriverData;
   GLuint mcs = t->Setup[I830_TEXREG_MCS];

   mcs &= ~TEXCOORDS_ARE_IN_TEXELUNITS;
   mcs |= TEXCOORDS_ARE_NORMAL;

   if (mcs != t->Setup[I830_TEXREG_MCS]) {
      I830_STATECHANGE( imesa, I830_UPLOAD_TEX_N(unit) );
      t->Setup[I830_TEXREG_MCS] = mcs;
   }

   return GL_TRUE;
}

 
static GLboolean disable_tex( GLcontext *ctx, int unit )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);

   /* This is happening too often.  I need to conditionally send diffuse
    * state to the card.  Perhaps a diffuse dirty flag of some kind.
    * Will need to change this logic if more than 2 texture units are
    * used.  We need to only do this up to the last unit enabled, or unit
    * one if nothing is enabled.
    */

   if ( imesa->CurrentTexObj[unit] != NULL ) {
      /* The old texture is no longer bound to this texture unit.
       * Mark it as such.
       */

      imesa->CurrentTexObj[unit]->base.bound &= ~(1U << unit);
      imesa->CurrentTexObj[unit] = NULL;
   }

   imesa->TexEnvImageFmt[unit] = 0;
   imesa->dirty &= ~(I830_UPLOAD_TEX_N(unit));
   
   i830UpdateTexEnv( ctx, unit );

   return GL_TRUE;
}

static GLboolean i830UpdateTexUnit( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   imesa->TexEnabledMask &= ~(I830_TEX_UNIT_ENABLED(unit));

   if (texUnit->_ReallyEnabled == TEXTURE_2D_BIT) {
      return (enable_tex_common( ctx, unit ) &&
	      enable_tex_2d( ctx, unit ));
   }
   else if (texUnit->_ReallyEnabled == TEXTURE_RECT_BIT) {      
      return (enable_tex_common( ctx, unit ) &&
	      enable_tex_rect( ctx, unit ));
   }
   else if (texUnit->_ReallyEnabled) {
      return GL_FALSE;
   }
   else {
      return disable_tex( ctx, unit );
   }
}



void i830UpdateTextureState( GLcontext *ctx )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   int i;
   int last_stage = 0;
   GLboolean ok;

   for ( i = 0 ; i < ctx->Const.MaxTextureUnits ; i++ ) {
      if ( (ctx->Texture.Unit[i]._ReallyEnabled == TEXTURE_2D_BIT)
	   || (ctx->Texture.Unit[i]._ReallyEnabled == TEXTURE_RECT_BIT) ) {
	 last_stage = i;
      }
   }

   ok = GL_TRUE;
   for ( i = 0 ; i <= last_stage ; i++ ) {
      ok = ok && i830UpdateTexUnit( ctx, i );
   }

   FALLBACK( imesa, I830_FALLBACK_TEXTURE, !ok );


   /* Make sure last stage is set correctly */
   imesa->TexBlend[last_stage][0] |= TEXOP_LAST_STAGE;
}
