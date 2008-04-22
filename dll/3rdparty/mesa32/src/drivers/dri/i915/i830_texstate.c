/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "texformat.h"
#include "texstore.h"

#include "mm.h"

#include "intel_screen.h"
#include "intel_ioctl.h"
#include "intel_tex.h"

#include "i830_context.h"
#include "i830_reg.h"

static const GLint initial_offsets[6][2] = { {0,0},
				       {0,2},
				       {1,0},
				       {1,2},
				       {1,1},
				       {1,3} };

static const GLint step_offsets[6][2] = { {0,2},
				    {0,2},
				    {-1,2},
				    {-1,2},
				    {-1,1},
				    {-1,1} };

#define I830_TEX_UNIT_ENABLED(unit)		(1<<unit)

static GLboolean i830SetTexImages( i830ContextPtr i830, 
				  struct gl_texture_object *tObj )
{
   GLuint total_height, pitch, i, textureFormat;
   i830TextureObjectPtr t = (i830TextureObjectPtr) tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint firstLevel, lastLevel, numLevels;

   switch( baseImage->TexFormat->MesaFormat ) {
   case MESA_FORMAT_L8:
      t->intel.texelBytes = 1;
      textureFormat = MAPSURF_8BIT | MT_8BIT_L8;
      break;

   case MESA_FORMAT_I8:
      t->intel.texelBytes = 1;
      textureFormat = MAPSURF_8BIT | MT_8BIT_I8;
      break;

   case MESA_FORMAT_A8:
      t->intel.texelBytes = 1;
      textureFormat = MAPSURF_8BIT | MT_8BIT_I8; /* Kludge -- check with conform, glean */
      break;

   case MESA_FORMAT_AL88:
      t->intel.texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_AY88;
      break;

   case MESA_FORMAT_RGB565:
      t->intel.texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_RGB565;
      break;

   case MESA_FORMAT_ARGB1555:
      t->intel.texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_ARGB1555;
      break;

   case MESA_FORMAT_ARGB4444:
      t->intel.texelBytes = 2;
      textureFormat = MAPSURF_16BIT | MT_16BIT_ARGB4444;
      break;

   case MESA_FORMAT_ARGB8888:
      t->intel.texelBytes = 4;
      textureFormat = MAPSURF_32BIT | MT_32BIT_ARGB8888;
      break;

   case MESA_FORMAT_YCBCR_REV:
      t->intel.texelBytes = 2;
      textureFormat = (MAPSURF_422 | MT_422_YCRCB_NORMAL | 
		       TM0S1_COLORSPACE_CONVERSION);
      break;

   case MESA_FORMAT_YCBCR:
      t->intel.texelBytes = 2;
      textureFormat = (MAPSURF_422 | MT_422_YCRCB_SWAPY | /* ??? */
		       TM0S1_COLORSPACE_CONVERSION);
      break;

   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
     t->intel.texelBytes = 2;
     textureFormat = MAPSURF_COMPRESSED | MT_COMPRESS_FXT1;
     break;

   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGB_DXT1:
     /* 
      * DXTn pitches are Width/4 * blocksize in bytes 
      * for DXT1: blocksize=8 so Width/4*8 = Width * 2 
      * for DXT3/5: blocksize=16 so Width/4*16 = Width * 4
      */
     t->intel.texelBytes = 2;
     textureFormat = (MAPSURF_COMPRESSED | MT_COMPRESS_DXT1);
     break;
   case MESA_FORMAT_RGBA_DXT3:
     t->intel.texelBytes = 4;
     textureFormat = (MAPSURF_COMPRESSED | MT_COMPRESS_DXT2_3);
     break;
   case MESA_FORMAT_RGBA_DXT5:
     t->intel.texelBytes = 4;
     textureFormat = (MAPSURF_COMPRESSED | MT_COMPRESS_DXT4_5);
     break;

   default:
      fprintf(stderr, "%s: bad image format\n", __FUNCTION__);
      abort();
   }

   /* Compute which mipmap levels we really want to send to the hardware.
    * This depends on the base image size, GL_TEXTURE_MIN_LOD,
    * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
    * Yes, this looks overly complicated, but it's all needed.
    */
   driCalculateTextureFirstLastLevel( (driTextureObject *) t );


   /* Figure out the amount of memory required to hold all the mipmap
    * levels.  Choose the smallest pitch to accomodate the largest
    * mipmap:
    */
   firstLevel = t->intel.base.firstLevel;
   lastLevel = t->intel.base.lastLevel;
   numLevels = lastLevel - firstLevel + 1;


   /* All images must be loaded at this pitch.  Count the number of
    * lines required:
    */
   switch (tObj->Target) {
   case GL_TEXTURE_CUBE_MAP: {
      const GLuint dim = tObj->Image[0][firstLevel]->Width;
      GLuint face;

      pitch = dim * t->intel.texelBytes;
      pitch *= 2;		/* double pitch for cube layouts */
      pitch = (pitch + 3) & ~3;
      
      total_height = dim * 4;

      for ( face = 0 ; face < 6 ; face++) {
	 GLuint x = initial_offsets[face][0] * dim;
	 GLuint y = initial_offsets[face][1] * dim;
	 GLuint d = dim;
	 
	 t->intel.base.dirty_images[face] = ~0;

	 assert(tObj->Image[face][firstLevel]->Width == dim);
	 assert(tObj->Image[face][firstLevel]->Height == dim);

	 for (i = 0; i < numLevels; i++) {
	    t->intel.image[face][i].image = tObj->Image[face][firstLevel + i];
	    if (!t->intel.image[face][i].image) {
	       fprintf(stderr, "no image %d %d\n", face, i);
	       break;		/* can't happen */
	    }
	 
	    t->intel.image[face][i].offset = 
	       y * pitch + x * t->intel.texelBytes;
	    t->intel.image[face][i].internalFormat = baseImage->_BaseFormat;

	    d >>= 1;
	    x += step_offsets[face][0] * d;
	    y += step_offsets[face][1] * d;
	 }
      }
      break;
   }
   default:
      pitch = tObj->Image[0][firstLevel]->Width * t->intel.texelBytes;
      pitch = (pitch + 3) & ~3;
      t->intel.base.dirty_images[0] = ~0;

      for ( total_height = i = 0 ; i < numLevels ; i++ ) {
	 t->intel.image[0][i].image = tObj->Image[0][firstLevel + i];
	 if (!t->intel.image[0][i].image) 
	    break;
	 
	 t->intel.image[0][i].offset = total_height * pitch;
	 t->intel.image[0][i].internalFormat = baseImage->_BaseFormat;
	 if (t->intel.image[0][i].image->IsCompressed)
	 {
	   if (t->intel.image[0][i].image->Height > 4)
	     total_height += t->intel.image[0][i].image->Height/4;
	   else
	     total_height += 1;
	 }
	 else
	   total_height += MAX2(2, t->intel.image[0][i].image->Height);
      }
      break;
   }

   t->intel.Pitch = pitch;
   t->intel.base.totalSize = total_height*pitch;
   t->intel.max_level = i-1;
   t->Setup[I830_TEXREG_TM0S1] = 
      (((tObj->Image[0][firstLevel]->Height - 1) << TM0S1_HEIGHT_SHIFT) |
       ((tObj->Image[0][firstLevel]->Width - 1) << TM0S1_WIDTH_SHIFT) |
       textureFormat);
   t->Setup[I830_TEXREG_TM0S2] = 
      (((pitch / 4) - 1) << TM0S2_PITCH_SHIFT) |
      TM0S2_CUBE_FACE_ENA_MASK;
   t->Setup[I830_TEXREG_TM0S3] &= ~TM0S3_MAX_MIP_MASK;
   t->Setup[I830_TEXREG_TM0S3] &= ~TM0S3_MIN_MIP_MASK;
   t->Setup[I830_TEXREG_TM0S3] |= ((numLevels - 1)*4) << TM0S3_MIN_MIP_SHIFT;
   t->intel.dirty = I830_UPLOAD_TEX_ALL;

   return intelUploadTexImages( &i830->intel, &t->intel, 0 );
}


static void i830_import_tex_unit( i830ContextPtr i830, 
			   i830TextureObjectPtr t,
			   GLuint unit )
{
   if(INTEL_DEBUG&DEBUG_TEXTURE)
      fprintf(stderr, "%s unit(%d)\n", __FUNCTION__, unit);
   
   if (i830->intel.CurrentTexObj[unit]) 
      i830->intel.CurrentTexObj[unit]->base.bound &= ~(1U << unit);

   i830->intel.CurrentTexObj[unit] = (intelTextureObjectPtr)t;
   t->intel.base.bound |= (1 << unit);

   I830_STATECHANGE( i830, I830_UPLOAD_TEX(unit) );

   i830->state.Tex[unit][I830_TEXREG_TM0LI] = (_3DSTATE_LOAD_STATE_IMMEDIATE_2 | 
					       (LOAD_TEXTURE_MAP0 << unit) | 4);
   i830->state.Tex[unit][I830_TEXREG_TM0S0] = (TM0S0_USE_FENCE |
					       t->intel.TextureOffset);

   i830->state.Tex[unit][I830_TEXREG_TM0S1] = t->Setup[I830_TEXREG_TM0S1];
   i830->state.Tex[unit][I830_TEXREG_TM0S2] = t->Setup[I830_TEXREG_TM0S2];

   i830->state.Tex[unit][I830_TEXREG_TM0S3] &= TM0S3_LOD_BIAS_MASK;
   i830->state.Tex[unit][I830_TEXREG_TM0S3] |= (t->Setup[I830_TEXREG_TM0S3] &
						~TM0S3_LOD_BIAS_MASK);

   i830->state.Tex[unit][I830_TEXREG_TM0S4] = t->Setup[I830_TEXREG_TM0S4];
   i830->state.Tex[unit][I830_TEXREG_MCS] = (t->Setup[I830_TEXREG_MCS] & 
					     ~MAP_UNIT_MASK);   
   i830->state.Tex[unit][I830_TEXREG_CUBE] = t->Setup[I830_TEXREG_CUBE];
   i830->state.Tex[unit][I830_TEXREG_MCS] |= MAP_UNIT(unit);

   t->intel.dirty &= ~I830_UPLOAD_TEX(unit);
}



static GLboolean enable_tex_common( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr i830 = I830_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   i830TextureObjectPtr t = (i830TextureObjectPtr)tObj->DriverData;

   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   /* Fallback if there's a texture border */
   if ( tObj->Image[0][tObj->BaseLevel]->Border > 0 ) {
      fprintf(stderr, "Texture border\n");
      return GL_FALSE;
   }

   /* Upload teximages (not pipelined)
    */
   if (t->intel.base.dirty_images[0]) {
      if (!i830SetTexImages( i830, tObj )) {
	 return GL_FALSE;
      }
   }

   /* Update state if this is a different texture object to last
    * time.
    */
   if (i830->intel.CurrentTexObj[unit] != &t->intel || 
       (t->intel.dirty & I830_UPLOAD_TEX(unit))) {
      i830_import_tex_unit( i830, t, unit);
   }

   I830_ACTIVESTATE(i830, I830_UPLOAD_TEX(unit), GL_TRUE);

   return GL_TRUE;
}

static GLboolean enable_tex_rect( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr i830 = I830_CONTEXT(ctx);
   GLuint mcs = i830->state.Tex[unit][I830_TEXREG_MCS];

   mcs &= ~TEXCOORDS_ARE_NORMAL;
   mcs |= TEXCOORDS_ARE_IN_TEXELUNITS;

   if ((mcs != i830->state.Tex[unit][I830_TEXREG_MCS])
       || (0 != i830->state.Tex[unit][I830_TEXREG_CUBE])) {
      I830_STATECHANGE(i830, I830_UPLOAD_TEX(unit));
      i830->state.Tex[unit][I830_TEXREG_MCS] = mcs;
      i830->state.Tex[unit][I830_TEXREG_CUBE] = 0;
   }

   return GL_TRUE;
}


static GLboolean enable_tex_2d( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr i830 = I830_CONTEXT(ctx);
   GLuint mcs = i830->state.Tex[unit][I830_TEXREG_MCS];

   mcs &= ~TEXCOORDS_ARE_IN_TEXELUNITS;
   mcs |= TEXCOORDS_ARE_NORMAL;

   if ((mcs != i830->state.Tex[unit][I830_TEXREG_MCS])
       || (0 != i830->state.Tex[unit][I830_TEXREG_CUBE])) {
      I830_STATECHANGE(i830, I830_UPLOAD_TEX(unit));
      i830->state.Tex[unit][I830_TEXREG_MCS] = mcs;
      i830->state.Tex[unit][I830_TEXREG_CUBE] = 0;
   }

   return GL_TRUE;
}

 
static GLboolean enable_tex_cube( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr i830 = I830_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   i830TextureObjectPtr t = (i830TextureObjectPtr)tObj->DriverData;
   GLuint mcs = i830->state.Tex[unit][I830_TEXREG_MCS];
   const GLuint cube = CUBE_NEGX_ENABLE | CUBE_POSX_ENABLE
     | CUBE_NEGY_ENABLE | CUBE_POSY_ENABLE
     | CUBE_NEGZ_ENABLE | CUBE_POSZ_ENABLE;
   GLuint face;

   mcs &= ~TEXCOORDS_ARE_IN_TEXELUNITS;
   mcs |= TEXCOORDS_ARE_NORMAL;

   if ((mcs != i830->state.Tex[unit][I830_TEXREG_MCS])
       || (cube != i830->state.Tex[unit][I830_TEXREG_CUBE])) {
      I830_STATECHANGE(i830, I830_UPLOAD_TEX(unit));
      i830->state.Tex[unit][I830_TEXREG_MCS] = mcs;
      i830->state.Tex[unit][I830_TEXREG_CUBE] = cube;
   }

   /* Upload teximages (not pipelined)
    */
   if ( t->intel.base.dirty_images[0] || t->intel.base.dirty_images[1] ||
        t->intel.base.dirty_images[2] || t->intel.base.dirty_images[3] ||
        t->intel.base.dirty_images[4] || t->intel.base.dirty_images[5] ) {
      i830SetTexImages( i830, tObj );
   }

   /* upload (per face) */
   for (face = 0; face < 6; face++) {
      if (t->intel.base.dirty_images[face]) {
	 if (!intelUploadTexImages( &i830->intel, &t->intel, face )) {
	    return GL_FALSE;
	 }
      }
   }


   return GL_TRUE;
}


static GLboolean disable_tex( GLcontext *ctx, GLuint unit )
{
   i830ContextPtr i830 = I830_CONTEXT(ctx);

   /* This is happening too often.  I need to conditionally send diffuse
    * state to the card.  Perhaps a diffuse dirty flag of some kind.
    * Will need to change this logic if more than 2 texture units are
    * used.  We need to only do this up to the last unit enabled, or unit
    * one if nothing is enabled.
    */

   if ( i830->intel.CurrentTexObj[unit] != NULL ) {
      /* The old texture is no longer bound to this texture unit.
       * Mark it as such.
       */

      i830->intel.CurrentTexObj[unit]->base.bound &= ~(1U << 0);
      i830->intel.CurrentTexObj[unit] = NULL;
   }

   return GL_TRUE;
}

static GLboolean i830UpdateTexUnit( GLcontext *ctx, GLuint unit )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   if (texUnit->_ReallyEnabled &&
       INTEL_CONTEXT(ctx)->intelScreen->tex.size < 2048 * 1024)
      return GL_FALSE;

   switch(texUnit->_ReallyEnabled) {
   case TEXTURE_1D_BIT:
   case TEXTURE_2D_BIT:
      return (enable_tex_common( ctx, unit ) &&
	      enable_tex_2d( ctx, unit ));
   case TEXTURE_RECT_BIT:
      return (enable_tex_common( ctx, unit ) &&
	      enable_tex_rect( ctx, unit ));
   case TEXTURE_CUBE_BIT:
      return (enable_tex_common( ctx, unit ) &&
	      enable_tex_cube( ctx, unit ));
   case 0:
      return disable_tex( ctx, unit );
   default:
      return GL_FALSE;
   }
}


void i830UpdateTextureState( intelContextPtr intel )
{
   i830ContextPtr i830 = I830_CONTEXT(intel);
   GLcontext *ctx = &intel->ctx;
   GLboolean ok;

   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   I830_ACTIVESTATE(i830, I830_UPLOAD_TEX_ALL, GL_FALSE);

   ok = (i830UpdateTexUnit( ctx, 0 ) &&
	 i830UpdateTexUnit( ctx, 1 ) &&
	 i830UpdateTexUnit( ctx, 2 ) &&
	 i830UpdateTexUnit( ctx, 3 ));

   FALLBACK( intel, I830_FALLBACK_TEXTURE, !ok );

   if (ok)
      i830EmitTextureBlend( i830 );
}



