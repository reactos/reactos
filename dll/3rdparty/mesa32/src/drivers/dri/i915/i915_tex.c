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

#include "i915_context.h"
#include "i915_reg.h"






/**
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */

intelTextureObjectPtr i915AllocTexObj( struct gl_texture_object *texObj )
{
   i915TextureObjectPtr t = CALLOC_STRUCT( i915_texture_object );
   if ( !t ) 
      return NULL;

   texObj->DriverData = t;
   t->intel.base.tObj = texObj;
   t->intel.dirty = I915_UPLOAD_TEX_ALL;
   make_empty_list( &t->intel.base );
   return &t->intel;
}


static void i915TexParameter( GLcontext *ctx, GLenum target,
			     struct gl_texture_object *tObj,
			     GLenum pname, const GLfloat *params )
{
   i915TextureObjectPtr t = (i915TextureObjectPtr) tObj->DriverData;
 
   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_WRAP_R:
   case GL_TEXTURE_BORDER_COLOR:
      t->intel.dirty = I915_UPLOAD_TEX_ALL;
      break;

   case GL_TEXTURE_COMPARE_MODE:
      t->intel.dirty = I915_UPLOAD_TEX_ALL;
      break;
   case GL_TEXTURE_COMPARE_FUNC:
      t->intel.dirty = I915_UPLOAD_TEX_ALL;
      break;

   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      /* The i915 and its successors can do a lot of this without
       * reloading the textures.  A project for someone?
       */
      intelFlush( ctx );
      driSwapOutTextureObject( (driTextureObject *) t );
      t->intel.dirty = I915_UPLOAD_TEX_ALL;
      break;

   default:
      return;
   }
}


static void i915TexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
   i915ContextPtr i915 = I915_CONTEXT( ctx );
   GLuint unit = ctx->Texture.CurrentUnit;

   switch (pname) {
   case GL_TEXTURE_ENV_COLOR: 	/* Should be a tracked param */
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
      i915->tex_program.translated = 0; 
      break;

   case GL_TEXTURE_LOD_BIAS: {
      int b = (int) ((*param) * 16.0);
      if (b > 255) b = 255;
      if (b < -256) b = -256;
      I915_STATECHANGE(i915, I915_UPLOAD_TEX(unit));
      i915->state.Tex[unit][I915_TEXREG_SS2] &= ~SS2_LOD_BIAS_MASK;
      i915->state.Tex[unit][I915_TEXREG_SS2] |= 
	 ((b << SS2_LOD_BIAS_SHIFT) & SS2_LOD_BIAS_MASK);
      break;
   }

   default:
      break;
   }
}


static void i915BindTexture( GLcontext *ctx, GLenum target,
			    struct gl_texture_object *texObj )
{
   i915TextureObjectPtr tex;
   
   if (!texObj->DriverData)
      i915AllocTexObj( texObj );
   
   tex = (i915TextureObjectPtr)texObj->DriverData;

   if (tex->lastTarget != texObj->Target) {
      tex->intel.dirty = I915_UPLOAD_TEX_ALL;
      tex->lastTarget = texObj->Target;
   }

   /* Need this if image format changes between bound textures.
    * Could try and shortcircuit by checking for differences in
    * state between incoming and outgoing textures:
    */
   I915_CONTEXT(ctx)->tex_program.translated = 0; 
}



void i915InitTextureFuncs( struct dd_function_table *functions )
{
   functions->BindTexture = i915BindTexture;
   functions->TexEnv = i915TexEnv;
   functions->TexParameter = i915TexParameter;
}
