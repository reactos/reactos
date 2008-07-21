/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_tex.c,v 1.6 2002/09/16 18:05:20 eich Exp $ */
/*
Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * Authors:
 *    Gareth Hughes <gareth@valinux.com>
 *    Brian Paul <brianp@valinux.com>
 */

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "image.h"
#include "simple_list.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"
#include "texobj.h"


#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_swtcl.h"
#include "radeon_tex.h"

#include "xmlpool.h"



/**
 * Set the texture wrap modes.
 * 
 * \param t Texture object whose wrap modes are to be set
 * \param swrap Wrap mode for the \a s texture coordinate
 * \param twrap Wrap mode for the \a t texture coordinate
 */

static void radeonSetTexWrap( radeonTexObjPtr t, GLenum swrap, GLenum twrap )
{
   GLboolean  is_clamp = GL_FALSE;
   GLboolean  is_clamp_to_border = GL_FALSE;

   t->pp_txfilter &= ~(RADEON_CLAMP_S_MASK | RADEON_CLAMP_T_MASK | RADEON_BORDER_MODE_D3D);

   switch ( swrap ) {
   case GL_REPEAT:
      t->pp_txfilter |= RADEON_CLAMP_S_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txfilter |= RADEON_CLAMP_S_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txfilter |= RADEON_CLAMP_S_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txfilter |= RADEON_CLAMP_S_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txfilter |= RADEON_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
   }

   switch ( twrap ) {
   case GL_REPEAT:
      t->pp_txfilter |= RADEON_CLAMP_T_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txfilter |= RADEON_CLAMP_T_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txfilter |= RADEON_CLAMP_T_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txfilter |= RADEON_CLAMP_T_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txfilter |= RADEON_CLAMP_T_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txfilter |= RADEON_CLAMP_T_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txfilter |= RADEON_CLAMP_T_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txfilter |= RADEON_CLAMP_T_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
   }

   if ( is_clamp_to_border ) {
      t->pp_txfilter |= RADEON_BORDER_MODE_D3D;
   }

   t->border_fallback = (is_clamp && is_clamp_to_border);
}

static void radeonSetTexMaxAnisotropy( radeonTexObjPtr t, GLfloat max )
{
   t->pp_txfilter &= ~RADEON_MAX_ANISO_MASK;

   if ( max == 1.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_1_TO_1;
   } else if ( max <= 2.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_2_TO_1;
   } else if ( max <= 4.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_4_TO_1;
   } else if ( max <= 8.0 ) {
      t->pp_txfilter |= RADEON_MAX_ANISO_8_TO_1;
   } else {
      t->pp_txfilter |= RADEON_MAX_ANISO_16_TO_1;
   }
}

/**
 * Set the texture magnification and minification modes.
 * 
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 */

static void radeonSetTexFilter( radeonTexObjPtr t, GLenum minf, GLenum magf )
{
   GLuint anisotropy = (t->pp_txfilter & RADEON_MAX_ANISO_MASK);

   t->pp_txfilter &= ~(RADEON_MIN_FILTER_MASK | RADEON_MAG_FILTER_MASK);

   /* r100 chips can't handle mipmaps/aniso for cubemap/volume textures */
   if ( t->base.tObj->Target == GL_TEXTURE_CUBE_MAP ) {
      switch ( minf ) {
      case GL_NEAREST:
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR;
	 break;
      default:
	 break;
      }
   }
   else if ( anisotropy == RADEON_MAX_ANISO_1_TO_1 ) {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_NEAREST;
	 break;
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_LINEAR;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_LINEAR;
	 break;
      }
   } else {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_ANISO_NEAREST_MIP_LINEAR;
	 break;
      }
   }

   switch ( magf ) {
   case GL_NEAREST:
      t->pp_txfilter |= RADEON_MAG_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      t->pp_txfilter |= RADEON_MAG_FILTER_LINEAR;
      break;
   }
}

static void radeonSetTexBorderColor( radeonTexObjPtr t, GLubyte c[4] )
{
   t->pp_border_color = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
}


/**
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */

static radeonTexObjPtr radeonAllocTexObj( struct gl_texture_object *texObj )
{
   radeonTexObjPtr t;

   t = CALLOC_STRUCT( radeon_tex_obj );
   texObj->DriverData = t;
   if ( t != NULL ) {
      if ( RADEON_DEBUG & DEBUG_TEXTURE ) {
	 fprintf( stderr, "%s( %p, %p )\n", __FUNCTION__, (void *)texObj, (void *)t );
      }

      /* Initialize non-image-dependent parts of the state:
       */
      t->base.tObj = texObj;
      t->border_fallback = GL_FALSE;

      t->pp_txfilter = RADEON_BORDER_MODE_OGL;
      t->pp_txformat = (RADEON_TXFORMAT_ENDIAN_NO_SWAP |
			RADEON_TXFORMAT_PERSPECTIVE_ENABLE);

      make_empty_list( & t->base );

      radeonSetTexWrap( t, texObj->WrapS, texObj->WrapT );
      radeonSetTexMaxAnisotropy( t, texObj->MaxAnisotropy );
      radeonSetTexFilter( t, texObj->MinFilter, texObj->MagFilter );
      radeonSetTexBorderColor( t, texObj->_BorderChan );
   }

   return t;
}


static const struct gl_texture_format *
radeonChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                           GLenum format, GLenum type )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   const GLboolean do32bpt =
       ( rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32 );
   const GLboolean force16bpt =
       ( rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16 );
   (void) format;

   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      switch ( type ) {
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
	 return do32bpt ? _dri_texformat_argb8888 : _dri_texformat_argb1555;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return _dri_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return _dri_texformat_argb1555;
      default:
         return do32bpt ? _dri_texformat_argb8888 : _dri_texformat_argb4444;
      }

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      switch ( type ) {
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return _dri_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return _dri_texformat_argb1555;
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
	 return _dri_texformat_rgb565;
      default:
         return do32bpt ? _dri_texformat_argb8888 : _dri_texformat_rgb565;
      }

   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return !force16bpt ?
	  _dri_texformat_argb8888 : _dri_texformat_argb4444;

   case GL_RGBA4:
   case GL_RGBA2:
      return _dri_texformat_argb4444;

   case GL_RGB5_A1:
      return _dri_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return !force16bpt ? _dri_texformat_argb8888 : _dri_texformat_rgb565;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return _dri_texformat_rgb565;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return _dri_texformat_a8;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return _dri_texformat_l8;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return _dri_texformat_al88;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return _dri_texformat_i8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
          type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

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
      _mesa_problem(ctx, "unexpected texture format in %s", __FUNCTION__);
      return NULL;
   }

   return NULL; /* never get here */
}


static void radeonTexImage1D( GLcontext *ctx, GLenum target, GLint level,
                              GLint internalFormat,
                              GLint width, GLint border,
                              GLenum format, GLenum type, const GLvoid *pixels,
                              const struct gl_pixelstore_attrib *packing,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   if ( t ) {
      driSwapOutTextureObject( t );
   }
   else {
      t = (driTextureObject *) radeonAllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
         return;
      }
   }

   /* Note, this will call ChooseTextureFormat */
   _mesa_store_teximage1d(ctx, target, level, internalFormat,
                          width, border, format, type, pixels,
                          &ctx->Unpack, texObj, texImage);

   t->dirty_images[0] |= (1 << level);
}


static void radeonTexSubImage1D( GLcontext *ctx, GLenum target, GLint level,
                                 GLint xoffset,
                                 GLsizei width,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels,
                                 const struct gl_pixelstore_attrib *packing,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   assert( t ); /* this _should_ be true */
   if ( t ) {
      driSwapOutTextureObject( t );
   }
   else {
      t = (driTextureObject *) radeonAllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D");
         return;
      }
   }

   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width,
			     format, type, pixels, packing, texObj,
			     texImage);

   t->dirty_images[0] |= (1 << level);
}


static void radeonTexImage2D( GLcontext *ctx, GLenum target, GLint level,
                              GLint internalFormat,
                              GLint width, GLint height, GLint border,
                              GLenum format, GLenum type, const GLvoid *pixels,
                              const struct gl_pixelstore_attrib *packing,
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
      t = (driTextureObject *) radeonAllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }

   /* Note, this will call ChooseTextureFormat */
   _mesa_store_teximage2d(ctx, target, level, internalFormat,
                          width, height, border, format, type, pixels,
                          &ctx->Unpack, texObj, texImage);

   t->dirty_images[face] |= (1 << level);
}


static void radeonTexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
                                 GLint xoffset, GLint yoffset,
                                 GLsizei width, GLsizei height,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels,
                                 const struct gl_pixelstore_attrib *packing,
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
      t = (driTextureObject *) radeonAllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
         return;
      }
   }

   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
			     height, format, type, pixels, packing, texObj,
			     texImage);

   t->dirty_images[face] |= (1 << level);
}

static void radeonCompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) radeonAllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
         return;
      }
   }

   /* Note, this will call ChooseTextureFormat */
   _mesa_store_compressed_teximage2d(ctx, target, level, internalFormat, width,
                                 height, border, imageSize, data, texObj, texImage);

   t->dirty_images[face] |= (1 << level);
}


static void radeonCompressedTexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) radeonAllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage2D");
         return;
      }
   }

   _mesa_store_compressed_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
                                 height, format, imageSize, data, texObj, texImage);

   t->dirty_images[face] |= (1 << level);
}

#define SCALED_FLOAT_TO_BYTE( x, scale ) \
		(((GLuint)((255.0F / scale) * (x))) / 2)

static void radeonTexEnv( GLcontext *ctx, GLenum target,
			  GLenum pname, const GLfloat *param )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   if ( RADEON_DEBUG & DEBUG_STATE ) {
      fprintf( stderr, "%s( %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( pname ) );
   }

   switch ( pname ) {
   case GL_TEXTURE_ENV_COLOR: {
      GLubyte c[4];
      GLuint envColor;
      UNCLAMPED_FLOAT_TO_RGBA_CHAN( c, texUnit->EnvColor );
      envColor = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
      if ( rmesa->hw.tex[unit].cmd[TEX_PP_TFACTOR] != envColor ) {
	 RADEON_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TFACTOR] = envColor;
      }
      break;
   }

   case GL_TEXTURE_LOD_BIAS_EXT: {
      GLfloat bias, min;
      GLuint b;

      /* The Radeon's LOD bias is a signed 2's complement value with a
       * range of -1.0 <= bias < 4.0.  We break this into two linear
       * functions, one mapping [-1.0,0.0] to [-128,0] and one mapping
       * [0.0,4.0] to [0,127].
       */
      min = driQueryOptionb (&rmesa->optionCache, "no_neg_lod_bias") ?
	  0.0 : -1.0;
      bias = CLAMP( *param, min, 4.0 );
      if ( bias == 0 ) {
	 b = 0;
      } else if ( bias > 0 ) {
	 b = ((GLuint)SCALED_FLOAT_TO_BYTE( bias, 4.0 )) << RADEON_LOD_BIAS_SHIFT;
      } else {
	 b = ((GLuint)SCALED_FLOAT_TO_BYTE( bias, 1.0 )) << RADEON_LOD_BIAS_SHIFT;
      }
      if ( (rmesa->hw.tex[unit].cmd[TEX_PP_TXFILTER] & RADEON_LOD_BIAS_MASK) != b ) {
	 RADEON_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFILTER] &= ~RADEON_LOD_BIAS_MASK;
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFILTER] |= (b & RADEON_LOD_BIAS_MASK);
      }
      break;
   }

   default:
      return;
   }
}


/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void radeonTexParameter( GLcontext *ctx, GLenum target,
				struct gl_texture_object *texObj,
				GLenum pname, const GLfloat *params )
{
   radeonTexObjPtr t = (radeonTexObjPtr) texObj->DriverData;

   if ( RADEON_DEBUG & (DEBUG_STATE|DEBUG_TEXTURE) ) {
      fprintf( stderr, "%s( %s )\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr( pname ) );
   }

   switch ( pname ) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      radeonSetTexMaxAnisotropy( t, texObj->MaxAnisotropy );
      radeonSetTexFilter( t, texObj->MinFilter, texObj->MagFilter );
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      radeonSetTexWrap( t, texObj->WrapS, texObj->WrapT );
      break;

   case GL_TEXTURE_BORDER_COLOR:
      radeonSetTexBorderColor( t, texObj->_BorderChan );
      break;

   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      /* This isn't the most efficient solution but there doesn't appear to
       * be a nice alternative.  Since there's no LOD clamping,
       * we just have to rely on loading the right subset of mipmap levels
       * to simulate a clamped LOD.
       */
      driSwapOutTextureObject( (driTextureObject *) t );
      break;

   default:
      return;
   }

   /* Mark this texobj as dirty (one bit per tex unit)
    */
   t->dirty_state = TEX_ALL;
}


static void radeonBindTexture( GLcontext *ctx, GLenum target,
			       struct gl_texture_object *texObj )
{
   if ( RADEON_DEBUG & (DEBUG_STATE|DEBUG_TEXTURE) ) {
      fprintf( stderr, "%s( %p ) unit=%d\n", __FUNCTION__, (void *)texObj,
	       ctx->Texture.CurrentUnit );
   }

   assert( (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D &&
            target != GL_TEXTURE_RECTANGLE_NV && target != GL_TEXTURE_CUBE_MAP) ||
           (texObj->DriverData != NULL) );
}


static void radeonDeleteTexture( GLcontext *ctx,
				 struct gl_texture_object *texObj )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   if ( RADEON_DEBUG & (DEBUG_STATE|DEBUG_TEXTURE) ) {
      fprintf( stderr, "%s( %p (target = %s) )\n", __FUNCTION__, (void *)texObj,
	       _mesa_lookup_enum_by_nr( texObj->Target ) );
   }

   if ( t != NULL ) {
      if ( rmesa ) {
         RADEON_FIREVERTICES( rmesa );
      }

      driDestroyTextureObject( t );
   }

   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, texObj);
}

/* Need:  
 *  - Same GEN_MODE for all active bits
 *  - Same EyePlane/ObjPlane for all active bits when using Eye/Obj
 *  - STRQ presumably all supported (matrix means incoming R values
 *    can end up in STQ, this has implications for vertex support,
 *    presumably ok if maos is used, though?)
 *  
 * Basically impossible to do this on the fly - just collect some
 * basic info & do the checks from ValidateState().
 */
static void radeonTexGen( GLcontext *ctx,
			  GLenum coord,
			  GLenum pname,
			  const GLfloat *params )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   rmesa->recheck_texgen[unit] = GL_TRUE;
}

/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 */
static struct gl_texture_object *
radeonNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   if (!obj)
      return NULL;
   obj->MaxAnisotropy = rmesa->initialMaxAnisotropy;
   radeonAllocTexObj( obj );
   return obj;
}


void radeonInitTextureFuncs( struct dd_function_table *functions )
{
   functions->ChooseTextureFormat	= radeonChooseTextureFormat;
   functions->TexImage1D		= radeonTexImage1D;
   functions->TexImage2D		= radeonTexImage2D;
   functions->TexSubImage1D		= radeonTexSubImage1D;
   functions->TexSubImage2D		= radeonTexSubImage2D;

   functions->NewTextureObject		= radeonNewTextureObject;
   functions->BindTexture		= radeonBindTexture;
   functions->DeleteTexture		= radeonDeleteTexture;
   functions->IsTextureResident		= driIsTextureResident;

   functions->TexEnv			= radeonTexEnv;
   functions->TexParameter		= radeonTexParameter;
   functions->TexGen			= radeonTexGen;

   functions->CompressedTexImage2D	= radeonCompressedTexImage2D;
   functions->CompressedTexSubImage2D	= radeonCompressedTexSubImage2D;

   driInitTextureFormats();
}
