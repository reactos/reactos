/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86$ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "texformat.h"

#include "sis_context.h"
#include "sis_state.h"
#include "sis_tex.h"
#include "sis_tris.h"
#include "sis_alloc.h"

static GLint TransferTexturePitch (GLint dwPitch);

/* Handle texenv stuff, called from validate_texture (renderstart) */
static void
sis_set_texture_env0( GLcontext *ctx, struct gl_texture_object *texObj,
   int unit )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLubyte c[4];

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   struct gl_texture_unit *texture_unit = &ctx->Texture.Unit[unit];

   sisTexObjPtr t = texObj->DriverData;

   switch (texture_unit->EnvMode)
   {
   case GL_REPLACE:
      switch (t->format)
      {
      case GL_ALPHA:
         current->hwTexBlendColor0 = STAGE0_C_CF;
         current->hwTexBlendAlpha0 = STAGE0_A_AS;
         break;
      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor0 = STAGE0_C_CS;
         current->hwTexBlendAlpha0 = STAGE0_A_AF;
         break;
      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
         current->hwTexBlendColor0 = STAGE0_C_CS;
         current->hwTexBlendAlpha0 = STAGE0_A_AS;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   case GL_MODULATE:
      switch (t->format)
      {
      case GL_ALPHA:
         current->hwTexBlendColor0 = STAGE0_C_CF;
         current->hwTexBlendAlpha0 = STAGE0_A_AFAS;
         break;
      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor0 = STAGE0_C_CFCS;
         current->hwTexBlendAlpha0 = STAGE0_A_AF;
         break;
      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
         current->hwTexBlendColor0 = STAGE0_C_CFCS;
         current->hwTexBlendAlpha0 = STAGE0_A_AFAS;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   case GL_DECAL:
      switch (t->format)
      {
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor0 = STAGE0_C_CS;
         current->hwTexBlendAlpha0 = STAGE0_A_AF;
         break;
      case GL_RGBA:
         current->hwTexBlendColor0 = STAGE0_C_CFOMAS_CSAS;
         current->hwTexBlendAlpha0 = STAGE0_A_AF;
         break;
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
         current->hwTexBlendColor0 = STAGE0_C_CF;
         current->hwTexBlendAlpha0 = STAGE0_A_AF;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   case GL_BLEND:
      UNCLAMPED_FLOAT_TO_RGBA_CHAN(c, texture_unit->EnvColor);
      current->hwTexEnvColor = ((GLint) (c[3])) << 24 |
			       ((GLint) (c[0])) << 16 |
			       ((GLint) (c[1])) << 8 |
			       ((GLint) (c[2]));
      switch (t->format)
      {
      case GL_ALPHA:
         current->hwTexBlendColor0 = STAGE0_C_CF;
         current->hwTexBlendAlpha0 = STAGE0_A_AFAS;
         break;
      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor0 = STAGE0_C_CFOMCS_CCCS;
         current->hwTexBlendAlpha0 = STAGE0_A_AF;
         break;
      case GL_INTENSITY:
         current->hwTexBlendColor0 = STAGE0_C_CFOMCS_CCCS;
         current->hwTexBlendAlpha0 = STAGE0_A_AFOMAS_ACAS;
         break;
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
         current->hwTexBlendColor0 = STAGE0_C_CFOMCS_CCCS;
         current->hwTexBlendAlpha0 = STAGE0_A_AFAS;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   default:
      sis_fatal_error("unknown env mode 0x%x\n", texture_unit->EnvMode);
   }

   if ((current->hwTexBlendColor0 != prev->hwTexBlendColor0) ||
       (current->hwTexBlendAlpha0 != prev->hwTexBlendAlpha0) ||
       (current->hwTexEnvColor != prev->hwTexEnvColor))
   {
      prev->hwTexEnvColor = current->hwTexEnvColor;
      prev->hwTexBlendColor0 = current->hwTexBlendColor0;
      prev->hwTexBlendAlpha0 = current->hwTexBlendAlpha0;
      smesa->GlobalFlag |= GFLAG_TEXTUREENV;
   }
}

/* Handle texenv stuff, called from validate_texture (renderstart) */
static void
sis_set_texture_env1( GLcontext *ctx, struct gl_texture_object *texObj,
   int unit)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLubyte c[4];

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   struct gl_texture_unit *texture_unit = &ctx->Texture.Unit[unit];

   sisTexObjPtr t = texObj->DriverData;

   switch (texture_unit->EnvMode)
   {
   case GL_REPLACE:
      switch (t->format)
      {
      case GL_ALPHA:
         current->hwTexBlendColor1 = STAGE1_C_CF;
         current->hwTexBlendAlpha1 = STAGE1_A_AS;
         break;
      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor1 = STAGE1_C_CS;
         current->hwTexBlendAlpha1 = STAGE1_A_AF;
         break;
      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
         current->hwTexBlendColor1 = STAGE1_C_CS;
         current->hwTexBlendAlpha1 = STAGE1_A_AS;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   case GL_MODULATE:
      switch (t->format)
      {
      case GL_ALPHA:
         current->hwTexBlendColor1 = STAGE1_C_CF;
         current->hwTexBlendAlpha1 = STAGE1_A_AFAS;
         break;
      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor1 = STAGE1_C_CFCS;
         current->hwTexBlendAlpha1 = STAGE1_A_AF;
         break;
      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
         current->hwTexBlendColor1 = STAGE1_C_CFCS;
         current->hwTexBlendAlpha1 = STAGE1_A_AFAS;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   case GL_DECAL:
      switch (t->format)
      {
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor1 = STAGE1_C_CS;
         current->hwTexBlendAlpha1 = STAGE1_A_AF;
         break;
      case GL_RGBA:
         current->hwTexBlendColor1 = STAGE1_C_CFOMAS_CSAS;
         current->hwTexBlendAlpha1 = STAGE1_A_AF;
         break;
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
         current->hwTexBlendColor1 = STAGE1_C_CF;
         current->hwTexBlendAlpha1 = STAGE1_A_AF;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   case GL_BLEND:
      UNCLAMPED_FLOAT_TO_RGBA_CHAN(c, texture_unit->EnvColor);
      current->hwTexEnvColor = ((GLint) (c[3])) << 24 |
			       ((GLint) (c[0])) << 16 |
			       ((GLint) (c[1])) << 8 |
			       ((GLint) (c[2]));
      switch (t->format)
      {
      case GL_ALPHA:
         current->hwTexBlendColor1 = STAGE1_C_CF;
         current->hwTexBlendAlpha1 = STAGE1_A_AFAS;
         break;
      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
         current->hwTexBlendColor1 = STAGE1_C_CFOMCS_CCCS;
         current->hwTexBlendAlpha1 = STAGE1_A_AF;
         break;
      case GL_INTENSITY:
         current->hwTexBlendColor1 = STAGE1_C_CFOMCS_CCCS;
         current->hwTexBlendAlpha1 = STAGE1_A_AFOMAS_ACAS;
         break;
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
         current->hwTexBlendColor1 = STAGE1_C_CFOMCS_CCCS;
         current->hwTexBlendAlpha1 = STAGE1_A_AFAS;
         break;
      default:
	 sis_fatal_error("unknown base format 0x%x\n", t->format);
      }
      break;

   default:
      sis_fatal_error("unknown env mode 0x%x\n", texture_unit->EnvMode);
   }

   if ((current->hwTexBlendColor1 != prev->hwTexBlendColor1) ||
       (current->hwTexBlendAlpha1 != prev->hwTexBlendAlpha1) ||
       (current->hwTexEnvColor != prev->hwTexEnvColor))
   {
      prev->hwTexBlendColor1 = current->hwTexBlendColor1;
      prev->hwTexBlendAlpha1 = current->hwTexBlendAlpha1;
      prev->hwTexEnvColor = current->hwTexEnvColor;
      smesa->GlobalFlag |= GFLAG_TEXTUREENV_1;
   }
}

/* Returns 0 if a software fallback is necessary */
static GLboolean
sis_set_texobj_parm( GLcontext *ctx, struct gl_texture_object *texObj,
   int hw_unit )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   int ok = 1;

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   sisTexObjPtr t = texObj->DriverData;

   GLint firstLevel, lastLevel;
   GLint i;

   current->texture[hw_unit].hwTextureMip = 0UL;
   current->texture[hw_unit].hwTextureSet = t->hwformat;

   if ((texObj->MinFilter == GL_NEAREST) || (texObj->MinFilter == GL_LINEAR)) {
      firstLevel = lastLevel = texObj->BaseLevel;
   } else {
      /* Compute which mipmap levels we really want to send to the hardware.
       * This depends on the base image size, GL_TEXTURE_MIN_LOD,
       * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL and GL_TEXTURE_MAX_LEVEL.
       * Yes, this looks overly complicated, but it's all needed.
       */

      firstLevel = texObj->BaseLevel + (GLint)(texObj->MinLod + 0.5);
      firstLevel = MAX2(firstLevel, texObj->BaseLevel);
      lastLevel = texObj->BaseLevel + (GLint)(texObj->MaxLod + 0.5);
      lastLevel = MAX2(lastLevel, texObj->BaseLevel);
      lastLevel = MIN2(lastLevel, texObj->BaseLevel +
         texObj->Image[0][texObj->BaseLevel]->MaxLog2);
      lastLevel = MIN2(lastLevel, texObj->MaxLevel);
      lastLevel = MAX2(firstLevel, lastLevel); /* need at least one level */
   }

   current->texture[hw_unit].hwTextureSet |= (lastLevel << 8);

   switch (texObj->MagFilter)
   {
   case GL_NEAREST:
      current->texture[hw_unit].hwTextureMip |= TEXTURE_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      current->texture[hw_unit].hwTextureMip |= (TEXTURE_FILTER_LINEAR << 3);
      break;
   }

   {
      GLint b;

      /* The mipmap lod biasing is based on experiment.  It seems there's a
       * limit of around +4/-4 to the bias value; we're being conservative.
       */
      b = (GLint) (ctx->Texture.Unit[hw_unit].LodBias * 32.0);
      if (b > 127)
         b = 127;
      else if (b < -128)
         b = -128;

      current->texture[hw_unit].hwTextureMip |= ((b << 4) &
         MASK_TextureMipmapLodBias);
   }

   switch (texObj->MinFilter)
   {
   case GL_NEAREST:
      current->texture[hw_unit].hwTextureMip |= TEXTURE_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      current->texture[hw_unit].hwTextureMip |= TEXTURE_FILTER_LINEAR;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      current->texture[hw_unit].hwTextureMip |=
         TEXTURE_FILTER_NEAREST_MIP_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      current->texture[hw_unit].hwTextureMip |=
         TEXTURE_FILTER_NEAREST_MIP_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      current->texture[hw_unit].hwTextureMip |=
         TEXTURE_FILTER_LINEAR_MIP_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      current->texture[hw_unit].hwTextureMip |=
         TEXTURE_FILTER_LINEAR_MIP_LINEAR;
      break;
   }

   switch (texObj->WrapS)
   {
   case GL_REPEAT:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureWrapU;
      break;
   case GL_MIRRORED_REPEAT:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureMirrorU;
      break;
   case GL_CLAMP:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureClampU;
       /* XXX: GL_CLAMP isn't conformant, but falling back makes the situation
        * worse in other programs at the moment.
        */
      /*ok = 0;*/
      break;
   case GL_CLAMP_TO_EDGE:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureClampU;
      break;
   case GL_CLAMP_TO_BORDER:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureBorderU;
      break;
   }

   switch (texObj->WrapT)
   {
   case GL_REPEAT:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureWrapV;
      break;
   case GL_MIRRORED_REPEAT:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureMirrorV;
      break;
   case GL_CLAMP:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureClampV;
       /* XXX: GL_CLAMP isn't conformant, but falling back makes the situation
        * worse in other programs at the moment.
        */
      /*ok = 0;*/
      break;
   case GL_CLAMP_TO_EDGE:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureClampV;
      break;
   case GL_CLAMP_TO_BORDER:
      current->texture[hw_unit].hwTextureSet |= MASK_TextureBorderV;
      break;
   }

   current->texture[hw_unit].hwTextureBorderColor = 
      ((GLuint) texObj->_BorderChan[3] << 24) + 
      ((GLuint) texObj->_BorderChan[0] << 16) + 
      ((GLuint) texObj->_BorderChan[1] << 8) + 
      ((GLuint) texObj->_BorderChan[2]);

   if (current->texture[hw_unit].hwTextureBorderColor !=
       prev->texture[hw_unit].hwTextureBorderColor) 
   {
      prev->texture[hw_unit].hwTextureBorderColor =
         current->texture[hw_unit].hwTextureBorderColor; 
      if (hw_unit == 1)
         smesa->GlobalFlag |= GFLAG_TEXBORDERCOLOR_1; 
      else
         smesa->GlobalFlag |= GFLAG_TEXBORDERCOLOR;
   }

   current->texture[hw_unit].hwTextureSet |=
      texObj->Image[0][firstLevel]->WidthLog2 << 4;
   current->texture[hw_unit].hwTextureSet |=
      texObj->Image[0][firstLevel]->HeightLog2;

   if (hw_unit == 0)
      smesa->GlobalFlag |= GFLAG_TEXTUREADDRESS;
   else
      smesa->GlobalFlag |= GFLAG_TEXTUREADDRESS_1;

   for (i = firstLevel; i <= lastLevel; i++)
   {
      GLuint texOffset = 0;
      GLuint texPitch = TransferTexturePitch( t->image[i].pitch );

      switch (t->image[i].memType)
      {
      case VIDEO_TYPE:
         texOffset = ((unsigned long)t->image[i].Data - (unsigned long)smesa->FbBase);
         break;
      case AGP_TYPE:
         texOffset = ((unsigned long)t->image[i].Data - (unsigned long)smesa->AGPBase) +
            (unsigned long) smesa->AGPAddr;
         current->texture[hw_unit].hwTextureMip |=
            (MASK_TextureLevel0InSystem << i);
         break;
      }

      switch (i)
      {
      case 0:
         prev->texture[hw_unit].texOffset0 = texOffset;
         prev->texture[hw_unit].texPitch01 = texPitch << 16;
         break;
      case 1:
         prev->texture[hw_unit].texOffset1 = texOffset;
         prev->texture[hw_unit].texPitch01 |= texPitch;
         break;
      case 2:
         prev->texture[hw_unit].texOffset2 = texOffset;
         prev->texture[hw_unit].texPitch23 = texPitch << 16;
         break;
      case 3:
         prev->texture[hw_unit].texOffset3 = texOffset;
         prev->texture[hw_unit].texPitch23 |= texPitch;
         break;
      case 4:
         prev->texture[hw_unit].texOffset4 = texOffset;
         prev->texture[hw_unit].texPitch45 = texPitch << 16;
         break;
      case 5:
         prev->texture[hw_unit].texOffset5 = texOffset;
         prev->texture[hw_unit].texPitch45 |= texPitch;
         break;
      case 6:
         prev->texture[hw_unit].texOffset6 = texOffset;
         prev->texture[hw_unit].texPitch67 = texPitch << 16;
         break;
      case 7:
         prev->texture[hw_unit].texOffset7 = texOffset;
         prev->texture[hw_unit].texPitch67 |= texPitch;
         break;
      case 8:
         prev->texture[hw_unit].texOffset8 = texOffset;
         prev->texture[hw_unit].texPitch89 = texPitch << 16;
         break;
      case 9:
         prev->texture[hw_unit].texOffset9 = texOffset;
         prev->texture[hw_unit].texPitch89 |= texPitch;
         break;
      case 10:
         prev->texture[hw_unit].texOffset10 = texOffset;
         prev->texture[hw_unit].texPitch10 = texPitch << 16;
         break;
      case 11:
         prev->texture[hw_unit].texOffset11 = texOffset;
         prev->texture[hw_unit].texPitch10 |= texPitch;
         break;
      }
   }

   if (current->texture[hw_unit].hwTextureSet != 
      prev->texture[hw_unit].hwTextureSet)
   {
      prev->texture[hw_unit].hwTextureSet =
         current->texture[hw_unit].hwTextureSet;
      if (hw_unit == 1)
         smesa->GlobalFlag |= CFLAG_TEXTURERESET_1;
      else
         smesa->GlobalFlag |= CFLAG_TEXTURERESET;
   }
   if (current->texture[hw_unit].hwTextureMip != 
      prev->texture[hw_unit].hwTextureMip)
   {
      prev->texture[hw_unit].hwTextureMip =
         current->texture[hw_unit].hwTextureMip;
      if (hw_unit == 1)
         smesa->GlobalFlag |= GFLAG_TEXTUREMIPMAP_1;
      else
         smesa->GlobalFlag |= GFLAG_TEXTUREMIPMAP;
   }

   return ok;
}

/* Disable a texture unit, called from validate_texture */
static void
sis_reset_texture_env (GLcontext *ctx, int hw_unit)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   if (hw_unit == 1)
   {
      current->hwTexBlendColor1 = STAGE1_C_CF;
      current->hwTexBlendAlpha1 = STAGE1_A_AF;
      
      if ((current->hwTexBlendColor1 != prev->hwTexBlendColor1) ||
          (current->hwTexBlendAlpha1 != prev->hwTexBlendAlpha1) ||
          (current->hwTexEnvColor != prev->hwTexEnvColor))
      {
         prev->hwTexBlendColor1 = current->hwTexBlendColor1;
         prev->hwTexBlendAlpha1 = current->hwTexBlendAlpha1;
         prev->hwTexEnvColor = current->hwTexEnvColor;
         smesa->GlobalFlag |= GFLAG_TEXTUREENV_1;
      }
   } else {
      current->hwTexBlendColor0 = STAGE0_C_CF;
      current->hwTexBlendAlpha0 = STAGE0_A_AF;
      
      if ((current->hwTexBlendColor0 != prev->hwTexBlendColor0) ||
          (current->hwTexBlendAlpha0 != prev->hwTexBlendAlpha0) ||
          (current->hwTexEnvColor != prev->hwTexEnvColor))
      {
         prev->hwTexBlendColor0 = current->hwTexBlendColor0;
         prev->hwTexBlendAlpha0 = current->hwTexBlendAlpha0;
         prev->hwTexEnvColor = current->hwTexEnvColor;
         smesa->GlobalFlag |= GFLAG_TEXTUREENV;
      }
   }
}

static void updateTextureUnit( GLcontext *ctx, int unit )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *texObj = texUnit->_Current;
   GLint fallbackbit;
   
   if (unit == 0)
      fallbackbit = SIS_FALLBACK_TEXTURE0;
   else
      fallbackbit = SIS_FALLBACK_TEXTURE1;

   if (texUnit->_ReallyEnabled & (TEXTURE_1D_BIT | TEXTURE_2D_BIT)) {
      if (smesa->TexStates[unit] & NEW_TEXTURING) {
         GLboolean ok;

         ok = sis_set_texobj_parm (ctx, texObj, unit);
         FALLBACK( smesa, fallbackbit, !ok );
      }
      if (smesa->TexStates[unit] & NEW_TEXTURE_ENV) {
         if (unit == 0)
            sis_set_texture_env0( ctx, texObj, unit );
         else
            sis_set_texture_env1( ctx, texObj, unit );
      }
      smesa->TexStates[unit] = 0;
   } else if ( texUnit->_ReallyEnabled ) {
      /* fallback */
      FALLBACK( smesa, fallbackbit, 1 );
   } else {
      sis_reset_texture_env( ctx, unit );
      FALLBACK( smesa, fallbackbit, 0 );
   }
}


void sisUpdateTextureState( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT( ctx );
   int i;
   __GLSiSHardware *current = &smesa->current;

#if 1
   /* TODO : if unmark these, error in multitexture */ /* XXX */
   for (i = 0; i < SIS_MAX_TEXTURES; i++)
      smesa->TexStates[i] |= (NEW_TEXTURING | NEW_TEXTURE_ENV);
#endif

   updateTextureUnit( ctx, 0 );
   updateTextureUnit( ctx, 1 );

   /* XXX Issues with the 2nd unit but not the first being enabled? */
   if ( ctx->Texture.Unit[0]._ReallyEnabled &
        (TEXTURE_1D_BIT | TEXTURE_2D_BIT) ||
        ctx->Texture.Unit[1]._ReallyEnabled &
        (TEXTURE_1D_BIT | TEXTURE_2D_BIT) )
   {
      current->hwCapEnable |= MASK_TextureEnable;
      current->hwCapEnable &= ~MASK_TextureNumUsed;
      if (ctx->Texture.Unit[1]._ReallyEnabled)
         current->hwCapEnable |= 0x00002000;
      else
         current->hwCapEnable |= 0x00001000;
   } else {
      current->hwCapEnable &= ~MASK_TextureEnable;
   }
}

static GLint
BitScanForward( GLshort w )
{
   GLint i;

   for (i = 0; i < 16; i++) {
      if (w & (1 << i))
         break;
   }
   return i;
}

static GLint
TransferTexturePitch( GLint dwPitch )
{
   GLint dwRet, i;

   i = BitScanForward( (GLshort)dwPitch );
   dwRet = dwPitch >> i;
   dwRet |= i << 9;
   return dwRet;
}
