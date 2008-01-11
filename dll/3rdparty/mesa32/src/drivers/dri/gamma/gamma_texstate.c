/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_texstate.c,v 1.5 2002/11/05 17:46:07 tsi Exp $ */

#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"

#include "mm.h"
#include "gamma_context.h"

static void gammaSetTexImages( gammaContextPtr gmesa, 
			      struct gl_texture_object *tObj )
{
   GLuint height, width, pitch, i, log_pitch;
   gammaTextureObjectPtr t = (gammaTextureObjectPtr) tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint firstLevel, lastLevel, numLevels;
   GLint log2Width, log2Height;

   /* fprintf(stderr, "%s\n", __FUNCTION__);  */

   t->texelBytes = 2;

   /* Compute which mipmap levels we really want to send to the hardware.
    * This depends on the base image size, GL_TEXTURE_MIN_LOD,
    * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
    * Yes, this looks overly complicated, but it's all needed.
    */
   if (tObj->MinFilter == GL_LINEAR || tObj->MinFilter == GL_NEAREST) {
      firstLevel = lastLevel = tObj->BaseLevel;
   }
   else {
      firstLevel = tObj->BaseLevel + (GLint) (tObj->MinLod + 0.5);
      firstLevel = MAX2(firstLevel, tObj->BaseLevel);
      lastLevel = tObj->BaseLevel + (GLint) (tObj->MaxLod + 0.5);
      lastLevel = MAX2(lastLevel, tObj->BaseLevel);
      lastLevel = MIN2(lastLevel, tObj->BaseLevel + baseImage->MaxLog2);
      lastLevel = MIN2(lastLevel, tObj->MaxLevel);
      lastLevel = MAX2(firstLevel, lastLevel); /* need at least one level */
   }

   /* save these values */
   t->firstLevel = firstLevel;
   t->lastLevel = lastLevel;

   numLevels = lastLevel - firstLevel + 1;

   log2Width = tObj->Image[0][firstLevel]->WidthLog2;
   log2Height = tObj->Image[0][firstLevel]->HeightLog2;


   /* Figure out the amount of memory required to hold all the mipmap
    * levels.  Choose the smallest pitch to accomodate the largest
    * mipmap:
    */
   width = tObj->Image[0][firstLevel]->Width * t->texelBytes;
   for (pitch = 32, log_pitch=2 ; pitch < width ; pitch *= 2 )
      log_pitch++;
   
   /* All images must be loaded at this pitch.  Count the number of
    * lines required:
    */
   for ( height = i = 0 ; i < numLevels ; i++ ) {
      t->image[i].image = tObj->Image[0][firstLevel + i];
      t->image[i].offset = height * pitch;
      t->image[i].internalFormat = baseImage->Format;
      height += t->image[i].image->Height;
      t->TextureBaseAddr[i] = /* ??? */
	(unsigned long)(t->image[i].offset + t->BufAddr) << 5;

   }

   t->Pitch = pitch;
   t->totalSize = height*pitch;
   t->max_level = i-1;
   gmesa->dirty |= GAMMA_UPLOAD_TEX0 /* | GAMMA_UPLOAD_TEX1*/;   

   gammaUploadTexImages( gmesa, t );
}

static void gammaUpdateTexEnv( GLcontext *ctx, GLuint unit )
{
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   const struct gl_texture_object *tObj = texUnit->_Current;
   const GLuint format = tObj->Image[0][tObj->BaseLevel]->Format;
   gammaTextureObjectPtr t = (gammaTextureObjectPtr)tObj->DriverData;
   GLuint tc;

   /* fprintf(stderr, "%s\n", __FUNCTION__);  */

   tc = t->TextureColorMode & ~(TCM_BaseFormatMask | TCM_ApplicationMask);

   switch (format) {
   case GL_RGB:
      tc |= TCM_BaseFormat_RGB;
      break;
   case GL_LUMINANCE:
      tc |= TCM_BaseFormat_Lum;
      break;
   case GL_ALPHA:
      tc |= TCM_BaseFormat_Alpha;
      break;
   case GL_LUMINANCE_ALPHA:
      tc |= TCM_BaseFormat_LumAlpha;
      break;
   case GL_INTENSITY:
      tc |= TCM_BaseFormat_Intensity;
      break;
   case GL_RGBA:
      tc |= TCM_BaseFormat_RGBA;
      break;
   case GL_COLOR_INDEX:
      break;
   }

   switch (texUnit->EnvMode) {
   case GL_REPLACE:
      tc |= TCM_Replace;
      break;
   case GL_MODULATE:
      tc |= TCM_Modulate;
      break;
   case GL_ADD:
      /* do nothing ???*/
      break;
   case GL_DECAL:
      tc |= TCM_Decal;
      break;
   case GL_BLEND:
      tc |= TCM_Blend;
      break;
   default:
      fprintf(stderr, "unknown tex env mode");
      return;
   }

   t->TextureColorMode = tc;
}




static void gammaUpdateTexUnit( GLcontext *ctx, GLuint unit )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   /* fprintf(stderr, "%s\n", __FUNCTION__);  */

   if (texUnit->_ReallyEnabled == TEXTURE_2D_BIT) 
   {
      struct gl_texture_object *tObj = texUnit->_Current;
      gammaTextureObjectPtr t = (gammaTextureObjectPtr)tObj->DriverData;

      /* Upload teximages (not pipelined)
       */
      if (t->dirty_images) {
	 gammaSetTexImages( gmesa, tObj );
	 if (!t->MemBlock) {
	    FALLBACK( gmesa, GAMMA_FALLBACK_TEXTURE, GL_TRUE );
	    return;
	 }
      }

#if 0
      if (tObj->Image[0][tObj->BaseLevel]->Border > 0) {
         FALLBACK( gmesa, GAMMA_FALLBACK_TEXTURE, GL_TRUE );
         return;
      }
#endif

      /* Update state if this is a different texture object to last
       * time.
       */
      if (gmesa->CurrentTexObj[unit] != t) {
	 gmesa->dirty |= GAMMA_UPLOAD_TEX0 /* << unit */;
	 gmesa->CurrentTexObj[unit] = t;
	 gammaUpdateTexLRU( gmesa, t ); /* done too often */
      }
      
      /* Update texture environment if texture object image format or 
       * texture environment state has changed.
       */
      if (tObj->Image[0][tObj->BaseLevel]->Format != gmesa->TexEnvImageFmt[unit]) {
	 gmesa->TexEnvImageFmt[unit] = tObj->Image[0][tObj->BaseLevel]->Format;
	 gammaUpdateTexEnv( ctx, unit );
      }
   }
   else if (texUnit->_ReallyEnabled) {
      FALLBACK( gmesa, GAMMA_FALLBACK_TEXTURE, GL_TRUE );
   }
   else /*if (gmesa->CurrentTexObj[unit])*/ {
      gmesa->CurrentTexObj[unit] = 0;
      gmesa->TexEnvImageFmt[unit] = 0;	
      gmesa->dirty &= ~(GAMMA_UPLOAD_TEX0<<unit); 
   }
}


void gammaUpdateTextureState( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   /* fprintf(stderr, "%s\n", __FUNCTION__);  */
   FALLBACK( gmesa, GAMMA_FALLBACK_TEXTURE, GL_FALSE );
   gammaUpdateTexUnit( ctx, 0 );
#if 0
   gammaUpdateTexUnit( ctx, 1 );
#endif
}



