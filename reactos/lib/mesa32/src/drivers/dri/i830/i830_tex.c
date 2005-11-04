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

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_tex.c,v 1.5 2003/05/07 21:56:31 dawes Exp $ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 *
 * Heavily based on the I810 driver, which was written by:
 *   Keith Whitwell <keithw@tungstengraphics.com>
 */

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "simple_list.h"
#include "enums.h"
#include "texstore.h"
#include "teximage.h"
#include "texformat.h"
#include "texmem.h"
#include "texobj.h"
#include "swrast/swrast.h"
#include "texobj.h"
#include "mm.h"

#include "i830_screen.h"
#include "i830_dri.h"
#include "i830_context.h"
#include "i830_tex.h"
#include "i830_state.h"
#include "i830_ioctl.h"

/*
 * Compute the 'S2.4' lod bias factor from the floating point OpenGL bias.
 */
static void i830ComputeLodBias( i830ContextPtr imesa, unsigned unit,
				GLfloat bias )
{
   int b;

   b = (int) (bias * 16.0);
   if(b > 63) b = 63;
   else if (b < -64) b = -64;
   imesa->LodBias[ unit ] = ((b << TM0S3_LOD_BIAS_SHIFT) & 
			     TM0S3_LOD_BIAS_MASK);
}


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
			       GLenum swrap, GLenum twrap)
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
      _mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
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
      _mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
   }
}

static void i830SetTexMaxAnisotropy( i830TextureObjectPtr t, GLfloat max )
{
   t->max_anisotropy = max;
}


/**
 * Set the texture magnification and minification modes.
 * 
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 * \param bias LOD bias for this texture unit.
 */

static void i830SetTexFilter( i830TextureObjectPtr t,
			      GLenum minf, GLenum magf )
{
   int minFilt = 0, mipFilt = 0, magFilt = 0;

   if(I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if ( t->max_anisotropy > 1.0 ) {
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
	 _mesa_problem(NULL, "%s: Unsupported min. filter %d", __FUNCTION__,
		       (int) minf );
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
	 _mesa_problem(NULL, "%s: Unsupported mag. filter %d", __FUNCTION__,
		       (int) magf );
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
   if(I830_DEBUG&DEBUG_DRI)
      fprintf(stderr, "%s\n", __FUNCTION__);

    t->Setup[I830_TEXREG_TM0S4] = 
        I830PACKCOLOR8888(color[0],color[1],color[2],color[3]);
}


/**
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */

static i830TextureObjectPtr i830AllocTexObj( struct gl_texture_object *texObj )
{
   i830TextureObjectPtr t;

   t = CALLOC_STRUCT( i830_texture_object_t );
   texObj->DriverData = t;
   if ( t != NULL ) {
      /* Initialize non-image-dependent parts of the state:
       */
      t->base.tObj = texObj;

      t->Setup[I830_TEXREG_TM0LI] = STATE3D_LOAD_STATE_IMMEDIATE_2;
      t->Setup[I830_TEXREG_TM0S0] = TM0S0_USE_FENCE;
      t->Setup[I830_TEXREG_TM0S1] = 0;
      t->Setup[I830_TEXREG_TM0S2] = 0;
      t->Setup[I830_TEXREG_TM0S3] = 0;

      t->Setup[I830_TEXREG_NOP0] = 0;
      t->Setup[I830_TEXREG_NOP1] = 0;
      t->Setup[I830_TEXREG_NOP2] = 0;

      t->Setup[I830_TEXREG_MCS] = (STATE3D_MAP_COORD_SET_CMD |
				   MAP_UNIT(0) |
				   ENABLE_TEXCOORD_PARAMS |
				   TEXCOORDS_ARE_NORMAL |
				   TEXCOORDTYPE_CARTESIAN |
				   ENABLE_ADDR_V_CNTL |
				   TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_WRAP) |
				   ENABLE_ADDR_U_CNTL |
				   TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_WRAP));

      make_empty_list( & t->base );

      i830SetTexWrapping( t, texObj->WrapS, texObj->WrapT );
      i830SetTexMaxAnisotropy( t, texObj->MaxAnisotropy );
      i830SetTexFilter( t, texObj->MinFilter, texObj->MagFilter );
      i830SetTexBorderColor( t, texObj->_BorderChan );
   }

   return t;
}


static void i830TexParameter( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj,
			      GLenum pname, const GLfloat *params )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   i830TextureObjectPtr t = (i830TextureObjectPtr) tObj->DriverData;
   GLuint unit = ctx->Texture.CurrentUnit;

   if (!t)
      return;

   if ( target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE_NV )
      return;

   /* Can't do the update now as we don't know whether to flush
    * vertices or not.  Setting imesa->NewGLState means that
    * i830UpdateTextureState() will be called before any triangles are
    * rendered.  If a statechange has occurred, it will be detected at
    * that point, and buffered vertices flushed.  
    */
   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      i830SetTexMaxAnisotropy( t, tObj->MaxAnisotropy );
      i830SetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
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
      I830_FIREVERTICES( I830_CONTEXT(ctx) );
      driSwapOutTextureObject( (driTextureObject *) t );
      break;

   default:
      return;
   }

   if (t == imesa->CurrentTexObj[unit]) {
      I830_STATECHANGE( imesa, I830_UPLOAD_TEX0 );
   }
}


static void i830TexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   GLuint unit = ctx->Texture.CurrentUnit;

   /* Only one env color.  Need a fallback if env colors are different
    * and texture setup references env color in both units.  
    */
   switch (pname) {
   case GL_TEXTURE_ENV_COLOR:
   case GL_TEXTURE_ENV_MODE:
   case GL_COMBINE_RGB_EXT:
   case GL_COMBINE_ALPHA_EXT:
   case GL_SOURCE0_RGB_EXT:
   case GL_SOURCE1_RGB_EXT:
   case GL_SOURCE2_RGB_EXT:
   case GL_SOURCE0_ALPHA_EXT:
   case GL_SOURCE1_ALPHA_EXT:
   case GL_SOURCE2_ALPHA_EXT:
   case GL_OPERAND0_RGB_EXT:
   case GL_OPERAND1_RGB_EXT:
   case GL_OPERAND2_RGB_EXT:
   case GL_OPERAND0_ALPHA_EXT:
   case GL_OPERAND1_ALPHA_EXT:
   case GL_OPERAND2_ALPHA_EXT:
   case GL_RGB_SCALE_EXT:
   case GL_ALPHA_SCALE:
      imesa->TexEnvImageFmt[unit] = 0; /* force recalc of env state */
      break;

   case GL_TEXTURE_LOD_BIAS_EXT:
      i830ComputeLodBias( imesa, unit, *param );
      I830_STATECHANGE( imesa, I830_UPLOAD_TEX_N(unit) );
      break;

   default:
      break;
   }
} 

static void i830TexImage2D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint height, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;
   if (t) {
      I830_FIREVERTICES( I830_CONTEXT(ctx) );
      driSwapOutTextureObject( t );
   }
   else {
      t = (driTextureObject *) i830AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }

   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );
}

static void i830TexSubImage2D( GLcontext *ctx, 
			       GLenum target,
			       GLint level,	
			       GLint xoffset, GLint yoffset,
			       GLsizei width, GLsizei height,
			       GLenum format, GLenum type,
			       const GLvoid *pixels,
			       const struct gl_pixelstore_attrib *packing,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;
   if (t) {
      I830_FIREVERTICES( I830_CONTEXT(ctx) );
      driSwapOutTextureObject( t );
   }
   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);

}



static void i830CompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
                              GLint internalFormat,
                              GLint width, GLint height, GLint border,
                              GLsizei imageSize, const GLvoid *data,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;
   GLuint face;

   /* which cube face or ordinary 2D image */
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      face = (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
      ASSERT(face < 6);
      break;
   default:
      face = 0;
   }

   if ( t != NULL ) {
      driSwapOutTextureObject( t );
   }
   else {
      t = (driTextureObject *) i830AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
         return;
      }
   }

   texImage->IsClientData = GL_FALSE;
   
   if (I830_DEBUG & DEBUG_TEXTURE)
     fprintf(stderr, "%s: Using normal storage\n", __FUNCTION__);
   
   _mesa_store_compressed_teximage2d(ctx, target, level, internalFormat, width,
				     height, border, imageSize, data, texObj, texImage);
   
   t->dirty_images[face] |= (1 << level);
}


static void i830CompressedTexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
                                 GLint xoffset, GLint yoffset,
                                 GLsizei width, GLsizei height,
                                 GLenum format,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;
   GLuint face;


   /* which cube face or ordinary 2D image */
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      face = (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
      ASSERT(face < 6);
      break;
   default:
      face = 0;
   }

   assert( t ); /* this _should_ be true */
   if ( t ) {
      driSwapOutTextureObject( t );
   }
   else {
      t = (driTextureObject *) i830AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage2D");
         return;
      }
   }

   _mesa_store_compressed_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
                            height, format, imageSize, data, texObj, texImage);

   t->dirty_images[face] |= (1 << level);
}


static void i830BindTexture( GLcontext *ctx, GLenum target,
			     struct gl_texture_object *tObj )
{
   assert( (target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE_NV) ||
           (tObj->DriverData != NULL) );
}


static void i830DeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   driTextureObject * t = (driTextureObject *) tObj->DriverData;
   if ( t != NULL ) {
      i830ContextPtr imesa = I830_CONTEXT( ctx );

      if ( imesa ) {
         I830_FIREVERTICES( imesa );
      }

      driDestroyTextureObject( t );
   }
   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}


static const struct gl_texture_format *
i830ChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			 GLenum format, GLenum type )
{
   i830ContextPtr imesa = I830_CONTEXT( ctx );
   const GLboolean do32bpt = ( imesa->i830Screen->cpp == 4 &&
			       imesa->i830Screen->textureSize > 4*1024*1024);

   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      if ( format == GL_BGRA ) {
	 if ( type == GL_UNSIGNED_INT_8_8_8_8_REV ) {
	    return &_mesa_texformat_argb8888;
	 }
         else if ( type == GL_UNSIGNED_SHORT_4_4_4_4_REV ) {
            return &_mesa_texformat_argb4444;
	 }
         else if ( type == GL_UNSIGNED_SHORT_1_5_5_5_REV ) {
	    return &_mesa_texformat_argb1555;
	 }
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      if ( format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5 ) {
	 return &_mesa_texformat_rgb565;
      }
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_RGBA4:
   case GL_RGBA2:
      return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return &_mesa_texformat_rgb565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_al88;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return &_mesa_texformat_al88;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA ||
	  type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   case GL_COMPRESSED_RGB_FXT1_3DFX:
     return &_mesa_texformat_rgb_fxt1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
     return &_mesa_texformat_rgba_fxt1;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
     return &_mesa_texformat_rgb_dxt1;

   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
     return &_mesa_texformat_rgba_dxt1;

   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
     return &_mesa_texformat_rgba_dxt3;

   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;

   default:
      fprintf(stderr, "unexpected texture format in %s\n", __FUNCTION__);
      return NULL;
   }

   return NULL; /* never get here */
}

/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 */
static struct gl_texture_object *
i830NewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   i830AllocTexObj( obj );
   return obj;
}

void i830InitTextureFuncs( struct dd_function_table *functions )
{
   functions->NewTextureObject          = i830NewTextureObject;
   functions->DeleteTexture             = i830DeleteTexture;
   functions->ChooseTextureFormat       = i830ChooseTextureFormat;
   functions->TexImage2D                = i830TexImage2D;
   functions->TexSubImage2D             = i830TexSubImage2D;
   functions->BindTexture               = i830BindTexture;
   functions->TexParameter              = i830TexParameter;
   functions->TexEnv                    = i830TexEnv;
   functions->IsTextureResident         = driIsTextureResident;
   functions->CompressedTexImage2D      = i830CompressedTexImage2D;
   functions->CompressedTexSubImage2D   = i830CompressedTexSubImage2D;
}
