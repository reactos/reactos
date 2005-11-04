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

   switch (baseImage->Format) {
   case GL_RGB:
   case GL_LUMINANCE:
      t->texelBytes = 2;
      textureFormat = MI1_FMT_16BPP | MI1_PF_16BPP_RGB565;
      break;
   case GL_ALPHA:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_RGBA:
      t->texelBytes = 2;
      textureFormat = MI1_FMT_16BPP | MI1_PF_16BPP_ARGB4444;
      break;
   case GL_COLOR_INDEX:
      textureFormat = MI1_FMT_8CI | MI1_PF_8CI_ARGB4444;
      t->texelBytes = 1;
      break;
   case GL_YCBCR_MESA:
      t->texelBytes = 2;
      textureFormat = MI1_FMT_422 | MI1_PF_422_YCRCB_SWAP_Y
	  | MI1_COLOR_CONV_ENABLE;
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
      t->image[i].internalFormat = baseImage->Format;
      height += t->image[i].image->Height;
   }

   t->Pitch = pitch;
   t->base.totalSize = height*pitch;
   t->max_level = i-1;
   t->dirty = I810_UPLOAD_TEX0 | I810_UPLOAD_TEX1;   
   t->Setup[I810_TEXREG_MI1] = (MI1_MAP_0 | textureFormat | log_pitch); 
   t->Setup[I810_TEXREG_MI2] = (MI2_DIMENSIONS_ARE_LOG2 |
				(log2Height << 16) | log2Width);
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

#define I810_DISABLE		0
#define I810_PASSTHRU		1
#define I810_REPLACE		2
#define I810_MODULATE		3
#define I810_DECAL		4
#define I810_BLEND		5
#define I810_ALPHA_BLEND	6
#define I810_ADD		7
#define I810_MAX_COMBFUNC	8


static GLuint i810_color_combine[][I810_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_ITERATED_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ONE |
	MC_UPDATE_OP |
	MC_OP_ARG1 ),		/* actually passthru */

      /* Passthru
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_ITERATED_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ONE |
	MC_UPDATE_OP |
	MC_OP_ARG1 ),

      /* GL_REPLACE 
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX0_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ONE |
	MC_UPDATE_OP |
	MC_OP_ARG1 ),

      /* GL_MODULATE
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX0_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ITERATED_COLOR |
	MC_UPDATE_OP |
	MC_OP_MODULATE ),

      /* GL_DECAL 
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_COLOR_FACTOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_TEX0_COLOR |
	MC_UPDATE_OP |
	MC_OP_LIN_BLEND_TEX0_ALPHA ),

      /* GL_BLEND 
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_COLOR_FACTOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ITERATED_COLOR |
	MC_UPDATE_OP |
	MC_OP_LIN_BLEND_TEX0_COLOR ),

      /* GL_BLEND according to alpha
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX0_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ITERATED_COLOR |
	MC_UPDATE_OP |
	MC_OP_LIN_BLEND_TEX0_ALPHA ),

      /* GL_ADD 
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_0 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX0_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ITERATED_COLOR |
	MC_UPDATE_OP |
	MC_OP_ADD ),
   },

   /* Unit 1:
    */
   {
      /* Disable combiner stage (Note: disables all subsequent stages)
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_ONE | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ONE |
	MC_UPDATE_OP |
	MC_OP_DISABLE ),

      
      /* Passthru
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_CURRENT_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ONE |
	MC_UPDATE_OP |
	MC_OP_ARG1 ),

      /* GL_REPLACE
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX1_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_ONE |
	MC_UPDATE_OP |
	MC_OP_ARG1 ),

      /* GL_MODULATE
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX1_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_CURRENT_COLOR |
	MC_UPDATE_OP |
	MC_OP_MODULATE ),

      /* GL_DECAL
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_COLOR_FACTOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_TEX1_COLOR |
	MC_UPDATE_OP |
	MC_OP_LIN_BLEND_TEX1_ALPHA ),
      
      /* GL_BLEND 
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_COLOR_FACTOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_CURRENT_COLOR |
	MC_UPDATE_OP |
	MC_OP_LIN_BLEND_TEX1_COLOR ),

      /* GL_BLEND according to alpha
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX1_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_CURRENT_COLOR |
	MC_UPDATE_OP |
	MC_OP_LIN_BLEND_TEX1_ALPHA ),

      /* GL_ADD 
       */
      ( GFX_OP_MAP_COLOR_STAGES |
	MC_STAGE_1 |
	MC_UPDATE_DEST |
	MC_DEST_CURRENT |
	MC_UPDATE_ARG1 |
	MC_ARG1_TEX1_COLOR | 
	MC_UPDATE_ARG2 |
	MC_ARG2_CURRENT_COLOR |
	MC_UPDATE_OP |
	MC_OP_ADD ),
   }
};

static GLuint i810_alpha_combine[][I810_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ITERATED_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX0_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ARG1 ),

      /* Passthru
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ITERATED_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX0_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ARG1 ),

      /* GL_REPLACE 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ITERATED_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX0_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ARG2 ),

      /* GL_MODULATE
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ITERATED_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX0_ALPHA |
	MA_UPDATE_OP |
	MA_OP_MODULATE ),

      /* GL_DECAL 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ALPHA_FACTOR |
	MA_UPDATE_ARG2 |
	MA_ARG2_ALPHA_FACTOR |
	MA_UPDATE_OP |
	MA_OP_ARG1 ),

      /* GL_BLEND 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ALPHA_FACTOR |
	MA_UPDATE_ARG2 |
	MA_ARG2_ITERATED_ALPHA |
	MA_UPDATE_OP |
	MA_OP_LIN_BLEND_TEX0_ALPHA ),

      /* GL_BLEND according to alpha (same as above)
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ALPHA_FACTOR |
	MA_UPDATE_ARG2 |
	MA_ARG2_ITERATED_ALPHA |
	MA_UPDATE_OP |
	MA_OP_LIN_BLEND_TEX0_ALPHA ),

      /* GL_ADD 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_0 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ITERATED_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX0_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ADD ),
   },

   /* Unit 1:
    */
   {
      /* Disable combiner stage
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_CURRENT_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_CURRENT_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ARG1 ),

      /* Passthru
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_CURRENT_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_CURRENT_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ARG1 ),

      /* GL_REPLACE 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_CURRENT_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX1_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ARG2 ),

      /* GL_MODULATE 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_CURRENT_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX1_ALPHA |
	MA_UPDATE_OP |
	MA_OP_MODULATE ),

      /* GL_DECAL 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ALPHA_FACTOR |
	MA_UPDATE_ARG2 |
	MA_ARG2_ALPHA_FACTOR |
	MA_UPDATE_OP |
	MA_OP_ARG1 ),

      /* GL_BLEND 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ALPHA_FACTOR |
	MA_UPDATE_ARG2 |
	MA_ARG2_ITERATED_ALPHA |
	MA_UPDATE_OP |
	MA_OP_LIN_BLEND_TEX1_ALPHA ),

      /* GL_BLEND according to alpha (same as above)
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_ALPHA_FACTOR |
	MA_UPDATE_ARG2 |
	MA_ARG2_ITERATED_ALPHA |
	MA_UPDATE_OP |
	MA_OP_LIN_BLEND_TEX1_ALPHA ),

      /* GL_ADD 
       */
      ( GFX_OP_MAP_ALPHA_STAGES |
	MA_STAGE_1 |
	MA_UPDATE_ARG1 |
	MA_ARG1_CURRENT_ALPHA |
	MA_UPDATE_ARG2 |
	MA_ARG2_TEX1_ALPHA |
	MA_UPDATE_OP |
	MA_OP_ADD ),
   }

};



static void i810UpdateTexEnv( GLcontext *ctx, GLuint unit )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   const struct gl_texture_object *tObj = texUnit->_Current;
   const GLuint format = tObj->Image[0][tObj->BaseLevel]->Format;
   GLuint color_combine, alpha_combine;

   switch (texUnit->EnvMode) {
   case GL_REPLACE:
      if (format == GL_ALPHA) {
	 color_combine = i810_color_combine[unit][I810_PASSTHRU];
	 alpha_combine = i810_alpha_combine[unit][I810_REPLACE];
      } else if (format == GL_LUMINANCE || format == GL_RGB) {
	 color_combine = i810_color_combine[unit][I810_REPLACE];
	 alpha_combine = i810_alpha_combine[unit][I810_PASSTHRU];
      } else {
	 color_combine = i810_color_combine[unit][I810_REPLACE];
	 alpha_combine = i810_alpha_combine[unit][I810_REPLACE];
      }
      break;

   case GL_MODULATE:
      if (format == GL_ALPHA) {
	 color_combine = i810_color_combine[unit][I810_PASSTHRU];
	 alpha_combine = i810_alpha_combine[unit][I810_MODULATE];
      } else {
	 color_combine = i810_color_combine[unit][I810_MODULATE];
	 alpha_combine = i810_alpha_combine[unit][I810_MODULATE];
      }
      break;

   case GL_DECAL:
      switch (format) {
      case GL_RGBA:
	 color_combine = i810_color_combine[unit][I810_ALPHA_BLEND];
	 alpha_combine = i810_alpha_combine[unit][I810_PASSTHRU];
	 break;
      case GL_RGB:
	 color_combine = i810_color_combine[unit][I810_REPLACE];
	 alpha_combine = i810_alpha_combine[unit][I810_PASSTHRU];
	 break;
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_INTENSITY:
	 color_combine = i810_color_combine[unit][I810_PASSTHRU];
	 alpha_combine = i810_alpha_combine[unit][I810_PASSTHRU];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   case GL_BLEND:
      switch (format) {
      case GL_RGB:
      case GL_LUMINANCE:
	 color_combine = i810_color_combine[unit][I810_BLEND];
	 alpha_combine = i810_alpha_combine[unit][I810_PASSTHRU];
	 break;
      case GL_RGBA:
      case GL_LUMINANCE_ALPHA:
	 color_combine = i810_color_combine[unit][I810_BLEND];
	 alpha_combine = i810_alpha_combine[unit][I810_MODULATE];
	 break;
      case GL_ALPHA:
	 color_combine = i810_color_combine[unit][I810_PASSTHRU];
	 alpha_combine = i810_alpha_combine[unit][I810_MODULATE];
	 break;
      case GL_INTENSITY:
	 color_combine = i810_color_combine[unit][I810_BLEND];
	 alpha_combine = i810_alpha_combine[unit][I810_BLEND];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   case GL_ADD:
      switch (format) {
      case GL_RGB:
      case GL_LUMINANCE:
	 color_combine = i810_color_combine[unit][I810_ADD];
	 alpha_combine = i810_alpha_combine[unit][I810_PASSTHRU];
	 break;
      case GL_RGBA:
      case GL_LUMINANCE_ALPHA:
	 color_combine = i810_color_combine[unit][I810_ADD];
	 alpha_combine = i810_alpha_combine[unit][I810_MODULATE];
	 break;
      case GL_ALPHA:
	 color_combine = i810_color_combine[unit][I810_PASSTHRU];
	 alpha_combine = i810_alpha_combine[unit][I810_MODULATE];
	 break;
      case GL_INTENSITY:
	 color_combine = i810_color_combine[unit][I810_ADD];
	 alpha_combine = i810_alpha_combine[unit][I810_ADD];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   default:
      return;
   }

   if (alpha_combine != imesa->Setup[I810_CTXREG_MA0 + unit] ||
       color_combine != imesa->Setup[I810_CTXREG_MC0 + unit]) 
   {
      I810_STATECHANGE( imesa, I810_UPLOAD_CTX );
      imesa->Setup[I810_CTXREG_MA0 + unit] = alpha_combine;
      imesa->Setup[I810_CTXREG_MC0 + unit] = color_combine;
   }	
}




static void i810UpdateTexUnit( GLcontext *ctx, GLuint unit )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   if (texUnit->_ReallyEnabled == TEXTURE_2D_BIT) 
   {
      struct gl_texture_object *tObj = texUnit->_Current;
      i810TextureObjectPtr t = (i810TextureObjectPtr)tObj->DriverData;

      /* Upload teximages (not pipelined)
       */
      if (t->base.dirty_images[0]) {
	 I810_FIREVERTICES(imesa);
	 i810SetTexImages( imesa, tObj );
	 if (!t->base.memBlock) {
	    FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_TRUE );
	    return;
	 }
      }

      if (tObj->Image[0][tObj->BaseLevel]->Border > 0) {
         FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_TRUE );
         return;
      }

      /* Update state if this is a different texture object to last
       * time.
       */
      if (imesa->CurrentTexObj[unit] != t) {
	 I810_STATECHANGE(imesa, (I810_UPLOAD_TEX0<<unit));
	 imesa->CurrentTexObj[unit] = t;
	 t->base.bound |= (1U << unit);
	 
	 driUpdateTextureLRU( (driTextureObject *) t ); /* XXX: should be locked */

      }
      
      /* Update texture environment if texture object image format or 
       * texture environment state has changed.
       */
      if (tObj->Image[0][tObj->BaseLevel]->Format != imesa->TexEnvImageFmt[unit]) {
	 imesa->TexEnvImageFmt[unit] = tObj->Image[0][tObj->BaseLevel]->Format;
	 i810UpdateTexEnv( ctx, unit );
      }
   }
   else if (texUnit->_ReallyEnabled) {
      FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_TRUE );
   }
   else /*if (imesa->CurrentTexObj[unit])*/ {
      imesa->CurrentTexObj[unit] = 0;
      imesa->TexEnvImageFmt[unit] = 0;	
      imesa->dirty &= ~(I810_UPLOAD_TEX0<<unit); 
      imesa->Setup[I810_CTXREG_MA0 + unit] = 
	 i810_alpha_combine[unit][I810_DISABLE];
      imesa->Setup[I810_CTXREG_MC0 + unit] = 
	 i810_color_combine[unit][I810_DISABLE];
      I810_STATECHANGE( imesa, I810_UPLOAD_CTX );
   }
}


void i810UpdateTextureState( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   /*  fprintf(stderr, "%s\n", __FUNCTION__); */
   FALLBACK( imesa, I810_FALLBACK_TEXTURE, GL_FALSE );
   i810UpdateTexUnit( ctx, 0 );
   i810UpdateTexUnit( ctx, 1 );
}



