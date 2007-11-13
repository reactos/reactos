/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"

#include "mm.h"
#include "s3v_context.h"
#include "s3v_tex.h"


static void s3vSetTexImages( s3vContextPtr vmesa, 
			      struct gl_texture_object *tObj )
{
   GLuint height, width, pitch, i, /*textureFormat,*/ log_pitch;
   s3vTextureObjectPtr t = (s3vTextureObjectPtr) tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint firstLevel, lastLevel, numLevels;
   GLint log2Width, log2Height;
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vSetTexImages: #%i ***\n", ++times));
#endif

   t->texelBytes = 2; /* FIXME: always 2 ? */

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
      t->image[i].internalFormat = baseImage->_BaseFormat;
      height += t->image[i].image->Height;
      t->TextureBaseAddr[i] = (t->BufAddr + t->image[i].offset +
         _TEXALIGN) & (GLuint)(~_TEXALIGN);
   }

   t->Pitch = pitch;
   t->WidthLog2 = log2Width;
   t->totalSize = height*pitch;
   t->max_level = i-1;
   vmesa->dirty |= S3V_UPLOAD_TEX0 /* | S3V_UPLOAD_TEX1*/;   
   vmesa->restore_primitive = -1;
   DEBUG(("<><>pitch = TexStride = %i\n", pitch));
   DEBUG(("log2Width = %i\n", log2Width));

   s3vUploadTexImages( vmesa, t );
}

static void s3vUpdateTexEnv( GLcontext *ctx, GLuint unit )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   const struct gl_texture_object *tObj = texUnit->_Current;
   const GLuint format = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;
/*
   s3vTextureObjectPtr t = (s3vTextureObjectPtr)tObj->DriverData;
   GLuint tc;
*/
   GLuint alpha = 0;
   GLuint cmd = vmesa->CMD;
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vUpdateTexEnv: %i ***\n", ++times));
#endif

   cmd &= ~TEX_COL_MASK;
   cmd &= ~TEX_BLEND_MAKS;
/*   cmd &= ~ALPHA_BLEND_MASK; */

   DEBUG(("format = "));

   switch (format) {
   case GL_RGB:
      DEBUG_TEX(("GL_RGB\n"));
      cmd |= TEX_COL_ARGB1555;
      break;
   case GL_LUMINANCE:
      DEBUG_TEX(("GL_LUMINANCE\n"));
      cmd |= TEX_COL_ARGB4444;
      alpha = 1; /* FIXME: check */
      break;
   case GL_ALPHA:
      DEBUG_TEX(("GL_ALPHA\n"));
      cmd |= TEX_COL_ARGB4444;
      alpha = 1;
      break;
   case GL_LUMINANCE_ALPHA:
      DEBUG_TEX(("GL_LUMINANCE_ALPHA\n"));
      cmd |= TEX_COL_ARGB4444;
      alpha = 1;
      break;
   case GL_INTENSITY:
      DEBUG_TEX(("GL_INTENSITY\n"));
      cmd |= TEX_COL_ARGB4444;
      alpha = 1;
      break;
   case GL_RGBA:
      DEBUG_TEX(("GL_RGBA\n"));
      cmd |= TEX_COL_ARGB4444;
      alpha = 1;
      break;
   case GL_COLOR_INDEX:
      DEBUG_TEX(("GL_COLOR_INDEX\n"));
      cmd |= TEX_COL_PAL;
      break;
   }

   DEBUG_TEX(("EnvMode = "));

   switch (texUnit->EnvMode) {
   case GL_REPLACE:
      DEBUG_TEX(("GL_REPLACE\n"));
      cmd |= TEX_REFLECT; /* FIXME */
      vmesa->_tri[1] = DO_TEX_UNLIT_TRI; /* FIXME: white tri hack */
      vmesa->_alpha_tex = ALPHA_TEX /* * alpha */;
      break;
   case GL_MODULATE:
      DEBUG_TEX(("GL_MODULATE\n"));
      cmd |= TEX_MODULATE;
      vmesa->_tri[1] = DO_TEX_LIT_TRI;
#if 0
	if (alpha)
		vmesa->_alpha_tex = ALPHA_TEX /* * alpha */;
	else
		vmesa->_alpha_tex = ALPHA_SRC /* * alpha */;
#else
	vmesa->_alpha_tex = ALPHA_TEX ;
#endif
      break;
   case GL_ADD:
      DEBUG_TEX(("DEBUG_TEX\n"));
      /* do nothing ???*/
      break;
   case GL_DECAL:
      DEBUG_TEX(("GL_DECAL\n"));
      cmd |= TEX_DECAL;
      vmesa->_tri[1] = DO_TEX_LIT_TRI;
      vmesa->_alpha_tex = ALPHA_OFF;
      break;
   case GL_BLEND:
      DEBUG_TEX(("GL_BLEND\n"));
      cmd |= TEX_DECAL;
      vmesa->_tri[1] = DO_TEX_LIT_TRI;
      vmesa->_alpha_tex = ALPHA_OFF; /* FIXME: sure? */
      break;
   default:
      fprintf(stderr, "unknown tex env mode");
      return;
   }
  
   DEBUG_TEX(("\n\n    vmesa->CMD was 0x%x\n", vmesa->CMD));   
   DEBUG_TEX((	  "    vmesa->CMD is 0x%x\n\n", cmd ));

   vmesa->_alpha[1] = vmesa->_alpha_tex;
   vmesa->CMD = cmd; /* | MIPMAP_LEVEL(8); */
   vmesa->restore_primitive = -1; 
}

static void s3vUpdateTexUnit( GLcontext *ctx, GLuint unit )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint cmd = vmesa->CMD;
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vUpdateTexUnit: %i ***\n", ++times));
   DEBUG_TEX(("and vmesa->CMD was 0x%x\n", vmesa->CMD));
#endif

   if (texUnit->_ReallyEnabled == TEXTURE_2D_BIT) 
   {
      struct gl_texture_object *tObj = texUnit->_Current;
      s3vTextureObjectPtr t = (s3vTextureObjectPtr)tObj->DriverData;

      /* Upload teximages (not pipelined)
       */
      if (t->dirty_images) {
#if _TEXFLUSH
         DMAFLUSH();
#endif
         s3vSetTexImages( vmesa, tObj ); 
         if (!t->MemBlock) {
#if _TEXFALLBACK
            FALLBACK( vmesa, S3V_FALLBACK_TEXTURE, GL_TRUE );
#endif
            return;
         }
      }

      /* Update state if this is a different texture object to last
       * time.
       */
#if 1
      if (vmesa->CurrentTexObj[unit] != t) {
         vmesa->dirty |= S3V_UPLOAD_TEX0 /* << unit */;
         vmesa->CurrentTexObj[unit] = t;
         s3vUpdateTexLRU( vmesa, t ); /* done too often */
      }
#endif
      
      /* Update texture environment if texture object image format or 
       * texture environment state has changed.
       */
      if (tObj->Image[0][tObj->BaseLevel]->_BaseFormat !=
          vmesa->TexEnvImageFmt[unit]) {
         vmesa->TexEnvImageFmt[unit] = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;
         s3vUpdateTexEnv( ctx, unit );
      }
#if 1
      cmd = vmesa->CMD & ~MIP_MASK;
      vmesa->dirty |= S3V_UPLOAD_TEX0 /* << unit */;
      vmesa->CurrentTexObj[unit] = t;
      vmesa->TexOffset = t->TextureBaseAddr[tObj->BaseLevel];
      vmesa->TexStride = t->Pitch;
      cmd |= MIPMAP_LEVEL(t->WidthLog2);
	
      DEBUG_TEX(("\n\n>>  vmesa->CMD was 0x%x\n", vmesa->CMD));
      DEBUG_TEX((    ">>  vmesa->CMD is 0x%x\n\n", cmd ));
      DEBUG_TEX(("t->WidthLog2 = %i\n", t->WidthLog2));
      DEBUG_TEX(("MIPMAP_LEVEL(t->WidthLog2) = 0x%x\n", MIPMAP_LEVEL(t->WidthLog2)));

      vmesa->CMD = cmd;
      vmesa->restore_primitive = -1;
#endif
   }
   else if (texUnit->_ReallyEnabled) { /* _ReallyEnabled but != TEXTURE0_2D */
#if _TEXFALLBACK
      FALLBACK( vmesa, S3V_FALLBACK_TEXTURE, GL_TRUE );
#endif
   }
   else /*if (vmesa->CurrentTexObj[unit])*/ { /* !_ReallyEnabled */
      vmesa->CurrentTexObj[unit] = 0;
      vmesa->TexEnvImageFmt[unit] = 0;	
      vmesa->dirty &= ~(S3V_UPLOAD_TEX0<<unit); 
   }
}


void s3vUpdateTextureState( GLcontext *ctx )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   (void) vmesa;
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vUpdateTextureState: #%i ***\n", ++times));
#endif

#if _TEXFALLBACK   
   FALLBACK( vmesa, S3V_FALLBACK_TEXTURE, GL_FALSE );
#endif
   s3vUpdateTexUnit( ctx, 0 );
#if 0
   s3vUpdateTexUnit( ctx, 1 );
#endif
}
