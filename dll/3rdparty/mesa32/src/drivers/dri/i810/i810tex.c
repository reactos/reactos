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
/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810tex.c,v 1.9 2002/10/30 12:51:33 alanh Exp $ */

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "simple_list.h"
#include "enums.h"
#include "texstore.h"
#include "texformat.h"
#include "teximage.h"
#include "texmem.h"
#include "texobj.h"
#include "swrast/swrast.h"
#include "colormac.h"
#include "texobj.h"
#include "mm.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810tex.h"
#include "i810state.h"
#include "i810ioctl.h"


/*
 * Compute the 'S2.4' lod bias factor from the floating point OpenGL bias.
 */
static GLuint i810ComputeLodBias(GLfloat bias)
{
   int b = (int) (bias * 16.0) + 12;
   if (b > 63)
      b = 63;
   else if (b < -64)
      b = -64;
   return (GLuint) (b & MLC_LOD_BIAS_MASK);
}


static void i810SetTexWrapping(i810TextureObjectPtr tex,
			       GLenum swrap, GLenum twrap)
{
   tex->Setup[I810_TEXREG_MCS] &= ~(MCS_U_STATE_MASK| MCS_V_STATE_MASK);

   switch( swrap ) {
   case GL_REPEAT:
      tex->Setup[I810_TEXREG_MCS] |= MCS_U_WRAP;
      break;
   case GL_CLAMP:
   case GL_CLAMP_TO_EDGE:
      tex->Setup[I810_TEXREG_MCS] |= MCS_U_CLAMP;
      break;
   case GL_MIRRORED_REPEAT:
      tex->Setup[I810_TEXREG_MCS] |= MCS_U_MIRROR;
      break;
   default:
      _mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
   }

   switch( twrap ) {
   case GL_REPEAT:
      tex->Setup[I810_TEXREG_MCS] |= MCS_V_WRAP;
      break;
   case GL_CLAMP:
   case GL_CLAMP_TO_EDGE:
      tex->Setup[I810_TEXREG_MCS] |= MCS_V_CLAMP;
      break;
   case GL_MIRRORED_REPEAT:
      tex->Setup[I810_TEXREG_MCS] |= MCS_V_MIRROR;
      break;
   default:
      _mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
   }
}


static void i810SetTexFilter(i810ContextPtr imesa, 
			     i810TextureObjectPtr t, 
			     GLenum minf, GLenum magf,
                             GLfloat bias)
{
   t->Setup[I810_TEXREG_MF] &= ~(MF_MIN_MASK|
				 MF_MAG_MASK|
				 MF_MIP_MASK);
   t->Setup[I810_TEXREG_MLC] &= ~(MLC_LOD_BIAS_MASK);

   switch (minf) {
   case GL_NEAREST:
      t->Setup[I810_TEXREG_MF] |= MF_MIN_NEAREST | MF_MIP_NONE;
      break;
   case GL_LINEAR:
      t->Setup[I810_TEXREG_MF] |= MF_MIN_LINEAR | MF_MIP_NONE;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      t->Setup[I810_TEXREG_MF] |= MF_MIN_NEAREST | MF_MIP_NEAREST;
      if (magf == GL_LINEAR) {
         /*bias -= 0.5;*/  /* this doesn't work too good */
      }
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      t->Setup[I810_TEXREG_MF] |= MF_MIN_LINEAR | MF_MIP_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      if (IS_I815(imesa)) 
	 t->Setup[I810_TEXREG_MF] |= MF_MIN_NEAREST | MF_MIP_LINEAR;
      else 
	 t->Setup[I810_TEXREG_MF] |= MF_MIN_NEAREST | MF_MIP_DITHER;
      /*
      if (magf == GL_LINEAR) {
         bias -= 0.5;
      }
      */
      bias -= 0.5; /* always biasing here looks better */
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      if (IS_I815(imesa))
	 t->Setup[I810_TEXREG_MF] |= MF_MIN_LINEAR | MF_MIP_LINEAR;
      else 
	 t->Setup[I810_TEXREG_MF] |= MF_MIN_LINEAR | MF_MIP_DITHER;
      break;
   default:
      return;
   }

   switch (magf) {
   case GL_NEAREST: 
      t->Setup[I810_TEXREG_MF] |= MF_MAG_NEAREST; 
      break;
   case GL_LINEAR: 
      t->Setup[I810_TEXREG_MF] |= MF_MAG_LINEAR; 
      break;
   default: 
      return;
   }

   t->Setup[I810_TEXREG_MLC] |= i810ComputeLodBias(bias);
}


static void
i810SetTexBorderColor( i810TextureObjectPtr t, GLubyte color[4] )
{
   /* Need a fallback.
    */
}


static i810TextureObjectPtr
i810AllocTexObj( GLcontext *ctx, struct gl_texture_object *texObj )
{
   i810TextureObjectPtr t;
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   t = CALLOC_STRUCT( i810_texture_object_t );
   texObj->DriverData = t;
   if ( t != NULL ) {
      GLfloat bias = ctx->Texture.Unit[ctx->Texture.CurrentUnit].LodBias;
      /* Initialize non-image-dependent parts of the state:
       */
      t->base.tObj = texObj;
      t->Setup[I810_TEXREG_MI0] = GFX_OP_MAP_INFO;
      t->Setup[I810_TEXREG_MI1] = MI1_MAP_0; 
      t->Setup[I810_TEXREG_MI2] = MI2_DIMENSIONS_ARE_LOG2;
      t->Setup[I810_TEXREG_MLC] = (GFX_OP_MAP_LOD_CTL | 
				   MLC_MAP_0 |
				   /*MLC_DITHER_WEIGHT_FULL |*/
				   MLC_DITHER_WEIGHT_12 |
				   MLC_UPDATE_LOD_BIAS |
				   0x0);
      t->Setup[I810_TEXREG_MCS] = (GFX_OP_MAP_COORD_SETS |
				   MCS_COORD_0 |
				   MCS_UPDATE_NORMALIZED |
				   MCS_NORMALIZED_COORDS |
				   MCS_UPDATE_V_STATE |
				   MCS_V_WRAP |
				   MCS_UPDATE_U_STATE |
				   MCS_U_WRAP);
      t->Setup[I810_TEXREG_MF] = (GFX_OP_MAP_FILTER |
				  MF_MAP_0 |
				  MF_UPDATE_ANISOTROPIC |
				  MF_UPDATE_MIP_FILTER |
				  MF_UPDATE_MAG_FILTER |
				  MF_UPDATE_MIN_FILTER);
      
      make_empty_list( & t->base );

      i810SetTexWrapping( t, texObj->WrapS, texObj->WrapT );
      /*i830SetTexMaxAnisotropy( t, texObj->MaxAnisotropy );*/
      i810SetTexFilter( imesa, t, texObj->MinFilter, texObj->MagFilter, bias );
      i810SetTexBorderColor( t, texObj->_BorderChan );
   }

   return t;
}


static void i810TexParameter( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj,
			      GLenum pname, const GLfloat *params )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   i810TextureObjectPtr t = (i810TextureObjectPtr) tObj->DriverData;

   if (!t)
      return;

   if ( target != GL_TEXTURE_2D )
      return;

   /* Can't do the update now as we don't know whether to flush
    * vertices or not.  Setting imesa->new_state means that
    * i810UpdateTextureState() will be called before any triangles are
    * rendered.  If a statechange has occurred, it will be detected at
    * that point, and buffered vertices flushed.  
    */
   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
      {
         GLfloat bias = ctx->Texture.Unit[ctx->Texture.CurrentUnit].LodBias;
         i810SetTexFilter( imesa, t, tObj->MinFilter, tObj->MagFilter, bias );
      }
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      i810SetTexWrapping( t, tObj->WrapS, tObj->WrapT );
      break;
  
   case GL_TEXTURE_BORDER_COLOR:
      i810SetTexBorderColor( t, tObj->_BorderChan );
      break;

   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      /* This isn't the most efficient solution but there doesn't appear to
       * be a nice alternative for Radeon.  Since there's no LOD clamping,
       * we just have to rely on loading the right subset of mipmap levels
       * to simulate a clamped LOD.
       */
      I810_FIREVERTICES( I810_CONTEXT(ctx) );
      driSwapOutTextureObject( (driTextureObject *) t );
      break;

   default:
      return;
   }

   if (t == imesa->CurrentTexObj[0]) {
      I810_STATECHANGE( imesa, I810_UPLOAD_TEX0 );
   }

   if (t == imesa->CurrentTexObj[1]) {
      I810_STATECHANGE( imesa, I810_UPLOAD_TEX1 );
   }
}


/**
 * Setup hardware bits for new texture environment settings.
 * 
 * \todo
 * Determine whether or not \c param can be used instead of
 * \c texUnit->EnvColor in the \c GL_TEXTURE_ENV_COLOR case.
 */
static void i810TexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   const GLuint unit = ctx->Texture.CurrentUnit;
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   /* Only one env color.  Need a fallback if env colors are different
    * and texture setup references env color in both units.  
    */
   switch (pname) {
   case GL_TEXTURE_ENV_COLOR: {
      GLubyte c[4];
      GLuint envColor;

      UNCLAMPED_FLOAT_TO_RGBA_CHAN( c, texUnit->EnvColor );
      envColor = PACK_COLOR_8888( c[3], c[0], c[1], c[2] );

      if (imesa->Setup[I810_CTXREG_CF1] != envColor) {
	 I810_STATECHANGE(imesa, I810_UPLOAD_CTX);
	 imesa->Setup[I810_CTXREG_CF1] = envColor;
      }
      break;
   }

   case GL_TEXTURE_ENV_MODE:
      imesa->TexEnvImageFmt[unit] = 0; /* force recalc of env state */
      break;

   case GL_TEXTURE_LOD_BIAS: {
      if ( texUnit->_Current != NULL ) {
	 const struct gl_texture_object *tObj = texUnit->_Current;
	 i810TextureObjectPtr t = (i810TextureObjectPtr) tObj->DriverData;

	 t->Setup[I810_TEXREG_MLC] &= ~(MLC_LOD_BIAS_MASK);
	 t->Setup[I810_TEXREG_MLC] |= i810ComputeLodBias(*param);
      }
      break;
   }

   default:
      break;
   }
} 



#if 0
static void i810TexImage1D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint border,
			    GLenum format, GLenum type, 
			    const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *pack,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   i810TextureObjectPtr t = (i810TextureObjectPtr) texObj->DriverData;
   if (t) {
      i810SwapOutTexObj( imesa, t );
   }
}

static void i810TexSubImage1D( GLcontext *ctx, 
			       GLenum target,
			       GLint level,	
			       GLint xoffset,
			       GLsizei width,
			       GLenum format, GLenum type,
			       const GLvoid *pixels,
			       const struct gl_pixelstore_attrib *pack,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage )
{
}
#endif


static void i810TexImage2D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint height, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   driTextureObject *t = (driTextureObject *) texObj->DriverData;
   if (t) {
      I810_FIREVERTICES( I810_CONTEXT(ctx) );
      driSwapOutTextureObject( t );
   }
   else {
      t = (driTextureObject *) i810AllocTexObj( ctx, texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );
}

static void i810TexSubImage2D( GLcontext *ctx, 
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
   driTextureObject *t = (driTextureObject *)texObj->DriverData;
   if (t) {
     I810_FIREVERTICES( I810_CONTEXT(ctx) );
     driSwapOutTextureObject( t );
   }
   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);
}


static void i810BindTexture( GLcontext *ctx, GLenum target,
			     struct gl_texture_object *tObj )
{
   assert( (target != GL_TEXTURE_2D) || (tObj->DriverData != NULL) );
}


static void i810DeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   driTextureObject * t = (driTextureObject *) tObj->DriverData;
   if (t) {
      i810ContextPtr imesa = I810_CONTEXT( ctx );
      if (imesa)
         I810_FIREVERTICES( imesa );
      driDestroyTextureObject( t );
   }
   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}

/**
 * Choose a Mesa texture format to match the requested format.
 * 
 * The i810 only supports 5 texture modes that are useful to Mesa.  That
 * makes this routine pretty simple.
 */
static const struct gl_texture_format *
i810ChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			 GLenum format, GLenum type )
{
   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
   case GL_COMPRESSED_RGBA:
      if ( ((format == GL_BGRA) && (type == GL_UNSIGNED_SHORT_1_5_5_5_REV))
	   || ((format == GL_RGBA) && (type == GL_UNSIGNED_SHORT_5_5_5_1))
	   || (internalFormat == GL_RGB5_A1) ) {
	 return &_mesa_texformat_argb1555;
      }
      return &_mesa_texformat_argb4444;

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return &_mesa_texformat_rgb565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_al88;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA ||
	  type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

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
i810NewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   i810AllocTexObj( ctx, obj );
   return obj;
}

void i810InitTextureFuncs( struct dd_function_table *functions )
{
   functions->ChooseTextureFormat = i810ChooseTextureFormat;
   functions->TexImage2D = i810TexImage2D;
   functions->TexSubImage2D = i810TexSubImage2D;
   functions->BindTexture = i810BindTexture;
   functions->NewTextureObject = i810NewTextureObject;
   functions->DeleteTexture = i810DeleteTexture;
   functions->TexParameter = i810TexParameter;
   functions->TexEnv = i810TexEnv;
   functions->IsTextureResident = driIsTextureResident;
}
