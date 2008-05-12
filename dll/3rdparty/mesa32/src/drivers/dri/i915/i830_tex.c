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
#include "mtypes.h"
#include "imports.h"
#include "simple_list.h"
#include "enums.h"
#include "image.h"
#include "texstore.h"
#include "texformat.h"
#include "texmem.h"
#include "swrast/swrast.h"

#include "mm.h"

#include "intel_ioctl.h"

#include "i830_context.h"
#include "i830_reg.h"




/**
 * Set the texture wrap modes.
 * 
 * The i830M (and related graphics cores) do not support GL_CLAMP.  The Intel
 * drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 * 
 * \param t Texture object whose wrap modes are to be set
 * \param swrap Wrap mode for the \a s texture coordinate
 * \param twrap Wrap mode for the \a t texture coordinate
 */
static void i830SetTexWrapping(i830TextureObjectPtr tex,
			       GLenum swrap, 
			       GLenum twrap)
{
   tex->Setup[I830_TEXREG_MCS] &= ~(TEXCOORD_ADDR_U_MASK|TEXCOORD_ADDR_V_MASK);

   switch( swrap ) {
   case GL_REPEAT:
      tex->Setup[I830_TEXREG_MCS] |= TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_WRAP);
      break;
   case GL_CLAMP:
   case GL_CLAMP_TO_EDGE:
      tex->Setup[I830_TEXREG_MCS] |= TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_CLAMP);
      break;
   case GL_CLAMP_TO_BORDER:
      tex->Setup[I830_TEXREG_MCS] |= 
			TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_CLAMP_BORDER);
      break;
   case GL_MIRRORED_REPEAT:
      tex->Setup[I830_TEXREG_MCS] |= 
			TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_MIRROR);
      break;
   default:
      break;
   }

   switch( twrap ) {
   case GL_REPEAT:
      tex->Setup[I830_TEXREG_MCS] |= TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_WRAP);
      break;
   case GL_CLAMP:
   case GL_CLAMP_TO_EDGE:
      tex->Setup[I830_TEXREG_MCS] |= TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_CLAMP);
      break;
   case GL_CLAMP_TO_BORDER:
      tex->Setup[I830_TEXREG_MCS] |= 
			TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_CLAMP_BORDER);
      break;
   case GL_MIRRORED_REPEAT:
      tex->Setup[I830_TEXREG_MCS] |=
			TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_MIRROR);
      break;
   default:
      break;
   }
}


/**
 * Set the texture magnification and minification modes.
 * 
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 * \param bias LOD bias for this texture unit.
 */

static void i830SetTexFilter( i830TextureObjectPtr t, GLenum minf, GLenum magf,
			      GLfloat maxanisotropy )
{
   int minFilt = 0, mipFilt = 0, magFilt = 0;

   if(INTEL_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if ( maxanisotropy > 1.0 ) {
      minFilt = FILTER_ANISOTROPIC;
      magFilt = FILTER_ANISOTROPIC;
   }
   else {
      switch (minf) {
      case GL_NEAREST:
	 minFilt = FILTER_NEAREST;
	 mipFilt = MIPFILTER_NONE;
	 break;
      case GL_LINEAR:
	 minFilt = FILTER_LINEAR;
	 mipFilt = MIPFILTER_NONE;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
	 minFilt = FILTER_NEAREST;
	 mipFilt = MIPFILTER_NEAREST;
	 break;
      case GL_LINEAR_MIPMAP_NEAREST:
	 minFilt = FILTER_LINEAR;
	 mipFilt = MIPFILTER_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
	 minFilt = FILTER_NEAREST;
	 mipFilt = MIPFILTER_LINEAR;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
	 minFilt = FILTER_LINEAR;
	 mipFilt = MIPFILTER_LINEAR;
	 break;
      default:
	 break;
      }

      switch (magf) {
      case GL_NEAREST:
	 magFilt = FILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 magFilt = FILTER_LINEAR;
	 break;
      default:
	 break;
      }  
   }

   t->Setup[I830_TEXREG_TM0S3] &= ~TM0S3_MIN_FILTER_MASK;
   t->Setup[I830_TEXREG_TM0S3] &= ~TM0S3_MIP_FILTER_MASK;
   t->Setup[I830_TEXREG_TM0S3] &= ~TM0S3_MAG_FILTER_MASK;
   t->Setup[I830_TEXREG_TM0S3] |= ((minFilt << TM0S3_MIN_FILTER_SHIFT) |
				   (mipFilt << TM0S3_MIP_FILTER_SHIFT) |
				   (magFilt << TM0S3_MAG_FILTER_SHIFT));
}

static void i830SetTexBorderColor(i830TextureObjectPtr t, GLubyte color[4])
{
   if(INTEL_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

    t->Setup[I830_TEXREG_TM0S4] = 
        INTEL_PACKCOLOR8888(color[0],color[1],color[2],color[3]);
}


/**
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */

intelTextureObjectPtr i830AllocTexObj( struct gl_texture_object *texObj )
{
   i830TextureObjectPtr t = CALLOC_STRUCT( i830_texture_object );
   if ( !t ) 
      return NULL;

   texObj->DriverData = t;
   t->intel.base.tObj = texObj;
   t->intel.dirty = I830_UPLOAD_TEX_ALL;
   make_empty_list( &t->intel.base );

   t->Setup[I830_TEXREG_TM0LI] = 0; /* not used */
   t->Setup[I830_TEXREG_TM0S0] = 0;
   t->Setup[I830_TEXREG_TM0S1] = 0;
   t->Setup[I830_TEXREG_TM0S2] = 0;
   t->Setup[I830_TEXREG_TM0S3] = 0;
   t->Setup[I830_TEXREG_MCS] = (_3DSTATE_MAP_COORD_SET_CMD |
				MAP_UNIT(0) |
				ENABLE_TEXCOORD_PARAMS |
				TEXCOORDS_ARE_NORMAL |
				TEXCOORDTYPE_CARTESIAN |
				ENABLE_ADDR_V_CNTL |
				TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_WRAP) |
				ENABLE_ADDR_U_CNTL |
				TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_WRAP));

   
   i830SetTexWrapping( t, texObj->WrapS, texObj->WrapT );
   i830SetTexFilter( t, texObj->MinFilter, texObj->MagFilter, 
		     texObj->MaxAnisotropy );
   i830SetTexBorderColor( t, texObj->_BorderChan );

   return &t->intel;
}


static void i830TexParameter( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj,
			      GLenum pname, const GLfloat *params )
{
   i830TextureObjectPtr t = (i830TextureObjectPtr) tObj->DriverData;
   if (!t)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      i830SetTexFilter( t, tObj->MinFilter, tObj->MagFilter,
			tObj->MaxAnisotropy);
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      i830SetTexWrapping( t, tObj->WrapS, tObj->WrapT );
      break;
  
   case GL_TEXTURE_BORDER_COLOR:
      i830SetTexBorderColor( t, tObj->_BorderChan );
      break;

   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      /* The i830 and its successors can do a lot of this without
       * reloading the textures.  A project for someone?
       */
      intelFlush( ctx );
      driSwapOutTextureObject( (driTextureObject *) t );
      break;

   default:
      return;
   }

   t->intel.dirty = I830_UPLOAD_TEX_ALL;
}


static void i830TexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
   i830ContextPtr i830 = I830_CONTEXT( ctx );
   GLuint unit = ctx->Texture.CurrentUnit;

   switch (pname) {
   case GL_TEXTURE_ENV_COLOR: 
#if 0
   {
      GLubyte r, g, b, a;
      GLuint col;
      
      UNCLAMPED_FLOAT_TO_UBYTE(r, param[RCOMP]);
      UNCLAMPED_FLOAT_TO_UBYTE(g, param[GCOMP]);
      UNCLAMPED_FLOAT_TO_UBYTE(b, param[BCOMP]);
      UNCLAMPED_FLOAT_TO_UBYTE(a, param[ACOMP]);

      col = ((a << 24) | (r << 16) | (g << 8) | b);

      if (col != i830->state.TexEnv[unit][I830_TEXENVREG_COL1]) {
	 I830_STATECHANGE(i830, I830_UPLOAD_TEXENV);
	 i830->state.TexEnv[unit][I830_TEXENVREG_COL1] = col;
      }

      break;
   }
#endif
   case GL_TEXTURE_ENV_MODE:
   case GL_COMBINE_RGB:
   case GL_COMBINE_ALPHA:
   case GL_SOURCE0_RGB:
   case GL_SOURCE1_RGB:
   case GL_SOURCE2_RGB:
   case GL_SOURCE0_ALPHA:
   case GL_SOURCE1_ALPHA:
   case GL_SOURCE2_ALPHA:
   case GL_OPERAND0_RGB:
   case GL_OPERAND1_RGB:
   case GL_OPERAND2_RGB:
   case GL_OPERAND0_ALPHA:
   case GL_OPERAND1_ALPHA:
   case GL_OPERAND2_ALPHA:
   case GL_RGB_SCALE:
   case GL_ALPHA_SCALE:
      break;

   case GL_TEXTURE_LOD_BIAS: {
      int b = (int) ((*param) * 16.0);
      if (b > 63) b = 63;
      if (b < -64) b = -64;
      I830_STATECHANGE(i830, I830_UPLOAD_TEX(unit));
      i830->state.Tex[unit][I830_TEXREG_TM0S3] &= ~TM0S3_LOD_BIAS_MASK;
      i830->state.Tex[unit][I830_TEXREG_TM0S3] |= 
	 ((b << TM0S3_LOD_BIAS_SHIFT) & TM0S3_LOD_BIAS_MASK);
      break;
   }

   default:
      break;
   }
}

static void i830BindTexture( GLcontext *ctx, GLenum target,
			    struct gl_texture_object *texObj )
{
   i830TextureObjectPtr tex;
   
   if (!texObj->DriverData)
      i830AllocTexObj( texObj );
   
   tex = (i830TextureObjectPtr)texObj->DriverData;
}



void i830InitTextureFuncs( struct dd_function_table *functions )
{
   functions->BindTexture 		= i830BindTexture;
   functions->TexEnv                    = i830TexEnv;
   functions->TexParameter              = i830TexParameter;
}
