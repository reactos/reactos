/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_tex.c,v 1.2 2002/11/05 17:46:08 tsi Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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
 *   Keith Whitwell <keith@tungstengraphics.com>
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
#include "texmem.h"
#include "teximage.h"
#include "texobj.h"

#include "r200_context.h"
#include "r200_state.h"
#include "r200_ioctl.h"
#include "r200_swtcl.h"
#include "r200_tex.h"

#include "xmlpool.h"



/**
 * Set the texture wrap modes.
 * 
 * \param t Texture object whose wrap modes are to be set
 * \param swrap Wrap mode for the \a s texture coordinate
 * \param twrap Wrap mode for the \a t texture coordinate
 */

static void r200SetTexWrap( r200TexObjPtr t, GLenum swrap, GLenum twrap, GLenum rwrap )
{
   GLboolean  is_clamp = GL_FALSE;
   GLboolean  is_clamp_to_border = GL_FALSE;

   t->pp_txfilter &= ~(R200_CLAMP_S_MASK | R200_CLAMP_T_MASK | R200_BORDER_MODE_D3D);

   switch ( swrap ) {
   case GL_REPEAT:
      t->pp_txfilter |= R200_CLAMP_S_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txfilter |= R200_CLAMP_S_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txfilter |= R200_CLAMP_S_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txfilter |= R200_CLAMP_S_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txfilter |= R200_CLAMP_S_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad S wrap mode in %s", __FUNCTION__);
   }

   switch ( twrap ) {
   case GL_REPEAT:
      t->pp_txfilter |= R200_CLAMP_T_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txfilter |= R200_CLAMP_T_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txfilter |= R200_CLAMP_T_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txfilter |= R200_CLAMP_T_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txfilter |= R200_CLAMP_T_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txfilter |= R200_CLAMP_T_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txfilter |= R200_CLAMP_T_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txfilter |= R200_CLAMP_T_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad T wrap mode in %s", __FUNCTION__);
   }

   t->pp_txformat_x &= ~R200_CLAMP_Q_MASK;

   switch ( rwrap ) {
   case GL_REPEAT:
      t->pp_txformat_x |= R200_CLAMP_Q_WRAP;
      break;
   case GL_CLAMP:
      t->pp_txformat_x |= R200_CLAMP_Q_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_CLAMP_TO_EDGE:
      t->pp_txformat_x |= R200_CLAMP_Q_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_BORDER:
      t->pp_txformat_x |= R200_CLAMP_Q_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   case GL_MIRRORED_REPEAT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR;
      break;
   case GL_MIRROR_CLAMP_EXT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR_CLAMP_GL;
      is_clamp = GL_TRUE;
      break;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR_CLAMP_LAST;
      break;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      t->pp_txformat_x |= R200_CLAMP_Q_MIRROR_CLAMP_GL;
      is_clamp_to_border = GL_TRUE;
      break;
   default:
      _mesa_problem(NULL, "bad R wrap mode in %s", __FUNCTION__);
   }

   if ( is_clamp_to_border ) {
      t->pp_txfilter |= R200_BORDER_MODE_D3D;
   }

   t->border_fallback = (is_clamp && is_clamp_to_border);
}

static void r200SetTexMaxAnisotropy( r200TexObjPtr t, GLfloat max )
{
   t->pp_txfilter &= ~R200_MAX_ANISO_MASK;

   if ( max == 1.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_1_TO_1;
   } else if ( max <= 2.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_2_TO_1;
   } else if ( max <= 4.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_4_TO_1;
   } else if ( max <= 8.0 ) {
      t->pp_txfilter |= R200_MAX_ANISO_8_TO_1;
   } else {
      t->pp_txfilter |= R200_MAX_ANISO_16_TO_1;
   }
}

/**
 * Set the texture magnification and minification modes.
 * 
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 */

static void r200SetTexFilter( r200TexObjPtr t, GLenum minf, GLenum magf )
{
   GLuint anisotropy = (t->pp_txfilter & R200_MAX_ANISO_MASK);

   t->pp_txfilter &= ~(R200_MIN_FILTER_MASK | R200_MAG_FILTER_MASK);
   t->pp_txformat_x &= ~R200_VOLUME_FILTER_MASK;

   if ( anisotropy == R200_MAX_ANISO_1_TO_1 ) {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_LINEAR_MIP_NEAREST;
	 break;
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_NEAREST_MIP_LINEAR;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_LINEAR_MIP_LINEAR;
	 break;
      }
   } else {
      switch ( minf ) {
      case GL_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= R200_MIN_FILTER_ANISO_NEAREST_MIP_LINEAR;
	 break;
      }
   }

   /* Note we don't have 3D mipmaps so only use the mag filter setting
    * to set the 3D texture filter mode.
    */
   switch ( magf ) {
   case GL_NEAREST:
      t->pp_txfilter |= R200_MAG_FILTER_NEAREST;
      t->pp_txformat_x |= R200_VOLUME_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      t->pp_txfilter |= R200_MAG_FILTER_LINEAR;
      t->pp_txformat_x |= R200_VOLUME_FILTER_LINEAR;
      break;
   }
}

static void r200SetTexBorderColor( r200TexObjPtr t, GLubyte c[4] )
{
   t->pp_border_color = r200PackColor( 4, c[0], c[1], c[2], c[3] );
}


/**
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */

static r200TexObjPtr r200AllocTexObj( struct gl_texture_object *texObj )
{
   r200TexObjPtr t;

   t = CALLOC_STRUCT( r200_tex_obj );
   texObj->DriverData = t;
   if ( t != NULL ) {
      if ( R200_DEBUG & DEBUG_TEXTURE ) {
	 fprintf( stderr, "%s( %p, %p )\n", __FUNCTION__, (void *)texObj, 
		  (void *)t );
      }

      /* Initialize non-image-dependent parts of the state:
       */
      t->base.tObj = texObj;
      t->border_fallback = GL_FALSE;

      make_empty_list( & t->base );

      r200SetTexWrap( t, texObj->WrapS, texObj->WrapT, texObj->WrapR );
      r200SetTexMaxAnisotropy( t, texObj->MaxAnisotropy );
      r200SetTexFilter( t, texObj->MinFilter, texObj->MagFilter );
      r200SetTexBorderColor( t, texObj->_BorderChan );
   }

   return t;
}

/* try to find a format which will only need a memcopy */
static const struct gl_texture_format *
r200Choose8888TexFormat( GLenum srcFormat, GLenum srcType )
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && !littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && littleEndian)) {
      return &_mesa_texformat_rgba8888;
   }
   else if ((srcFormat == GL_RGBA && srcType == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       (srcFormat == GL_RGBA && srcType == GL_UNSIGNED_BYTE && littleEndian) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_INT_8_8_8_8) ||
       (srcFormat == GL_ABGR_EXT && srcType == GL_UNSIGNED_BYTE && !littleEndian)) {
      return &_mesa_texformat_rgba8888_rev;
   }
   else return _dri_texformat_argb8888;
}

static const struct gl_texture_format *
r200ChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                           GLenum format, GLenum type )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
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
         return do32bpt ?
	    r200Choose8888TexFormat(format, type) : _dri_texformat_argb4444;
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
	  r200Choose8888TexFormat(format, type) : _dri_texformat_argb4444;

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
   /* can't use a8 format since interpreting hw I8 as a8 would result
      in wrong rgb values (same as alpha value instead of 0). */
      return _dri_texformat_al88;

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
      _mesa_problem(ctx,
         "unexpected internalFormat 0x%x in r200ChooseTextureFormat",
         (int) internalFormat);
      return NULL;
   }

   return NULL; /* never get here */
}


static GLboolean
r200ValidateClientStorage( GLcontext *ctx, GLenum target,
			   GLint internalFormat,
			   GLint srcWidth, GLint srcHeight, 
                           GLenum format, GLenum type,  const void *pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage)

{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (0)
      fprintf(stderr, "intformat %s format %s type %s\n",
	      _mesa_lookup_enum_by_nr( internalFormat ),
	      _mesa_lookup_enum_by_nr( format ),
	      _mesa_lookup_enum_by_nr( type ));

   if (!ctx->Unpack.ClientStorage)
      return 0;

   if (ctx->_ImageTransferState ||
       texImage->IsCompressed ||
       texObj->GenerateMipmap)
      return 0;


   /* This list is incomplete, may be different on ppc???
    */
   switch ( internalFormat ) {
   case GL_RGBA:
      if ( format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV ) {
	 texImage->TexFormat = _dri_texformat_argb8888;
      }
      else
	 return 0;
      break;

   case GL_RGB:
      if ( format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5 ) {
	 texImage->TexFormat = _dri_texformat_rgb565;
      }
      else
	 return 0;
      break;

   case GL_YCBCR_MESA:
      if ( format == GL_YCBCR_MESA && 
	   type == GL_UNSIGNED_SHORT_8_8_REV_APPLE ) {
	 texImage->TexFormat = &_mesa_texformat_ycbcr_rev;
      }
      else if ( format == GL_YCBCR_MESA && 
		(type == GL_UNSIGNED_SHORT_8_8_APPLE || 
		 type == GL_UNSIGNED_BYTE)) {
	 texImage->TexFormat = &_mesa_texformat_ycbcr;
      }
      else
	 return 0;
      break;

   default:
      return 0;
   }

   /* Could deal with these packing issues, but currently don't:
    */
   if (packing->SkipPixels || 
       packing->SkipRows || 
       packing->SwapBytes ||
       packing->LsbFirst) {
      return 0;
   }

   {      
      GLint srcRowStride = _mesa_image_row_stride(packing, srcWidth,
						  format, type);

      
      if (0)
	 fprintf(stderr, "%s: srcRowStride %d/%x\n", 
		 __FUNCTION__, srcRowStride, srcRowStride);

      /* Could check this later in upload, pitch restrictions could be
       * relaxed, but would need to store the image pitch somewhere,
       * as packing details might change before image is uploaded:
       */
      if (!r200IsGartMemory( rmesa, pixels, srcHeight * srcRowStride ) ||
	  (srcRowStride & 63))
	 return 0;


      /* Have validated that _mesa_transfer_teximage would be a straight
       * memcpy at this point.  NOTE: future calls to TexSubImage will
       * overwrite the client data.  This is explicitly mentioned in the
       * extension spec.
       */
      texImage->Data = (void *)pixels;
      texImage->IsClientData = GL_TRUE;
      texImage->RowStride = srcRowStride / texImage->TexFormat->TexelBytes;

      return 1;
   }
}


static void r200TexImage1D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) r200AllocTexObj( texObj );
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


static void r200TexSubImage1D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) r200AllocTexObj( texObj );
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


static void r200TexImage2D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) r200AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }

   texImage->IsClientData = GL_FALSE;

   if (r200ValidateClientStorage( ctx, target, 
				  internalFormat, 
				  width, height, 
				  format, type, pixels, 
				  packing, texObj, texImage)) {
      if (R200_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "%s: Using client storage\n", __FUNCTION__); 
   }
   else {
      if (R200_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "%s: Using normal storage\n", __FUNCTION__); 

      /* Normal path: copy (to cached memory) and eventually upload
       * via another copy to GART memory and then a blit...  Could
       * eliminate one copy by going straight to (permanent) GART.
       *
       * Note, this will call r200ChooseTextureFormat.
       */
      _mesa_store_teximage2d(ctx, target, level, internalFormat,
			     width, height, border, format, type, pixels,
			     &ctx->Unpack, texObj, texImage);
      
      t->dirty_images[face] |= (1 << level);
   }
}


static void r200TexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) r200AllocTexObj( texObj );
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


static void r200CompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) r200AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
         return;
      }
   }

   texImage->IsClientData = GL_FALSE;
/* can't call this, different parameters. Would never evaluate to true anyway currently
   if (r200ValidateClientStorage( ctx, target, 
				  internalFormat,
				  width, height,
				  format, type, pixels,
				  packing, texObj, texImage)) {
      if (R200_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "%s: Using client storage\n", __FUNCTION__);
   }
   else */{
      if (R200_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "%s: Using normal storage\n", __FUNCTION__);

      /* Normal path: copy (to cached memory) and eventually upload
       * via another copy to GART memory and then a blit...  Could
       * eliminate one copy by going straight to (permanent) GART.
       *
       * Note, this will call r200ChooseTextureFormat.
       */
      _mesa_store_compressed_teximage2d(ctx, target, level, internalFormat, width,
                                 height, border, imageSize, data, texObj, texImage);

      t->dirty_images[face] |= (1 << level);
   }
}


static void r200CompressedTexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
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
      t = (driTextureObject *) r200AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage2D");
         return;
      }
   }

   _mesa_store_compressed_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
                            height, format, imageSize, data, texObj, texImage);

   t->dirty_images[face] |= (1 << level);
}


#if ENABLE_HW_3D_TEXTURE
static void r200TexImage3D( GLcontext *ctx, GLenum target, GLint level,
                            GLint internalFormat,
                            GLint width, GLint height, GLint depth,
                            GLint border,
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
      t = (driTextureObject *) r200AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
         return;
      }
   }

   texImage->IsClientData = GL_FALSE;

#if 0
   if (r200ValidateClientStorage( ctx, target, 
				  internalFormat, 
				  width, height, 
				  format, type, pixels, 
				  packing, texObj, texImage)) {
      if (R200_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "%s: Using client storage\n", __FUNCTION__); 
   }
   else
#endif
   {
      if (R200_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "%s: Using normal storage\n", __FUNCTION__); 

      /* Normal path: copy (to cached memory) and eventually upload
       * via another copy to GART memory and then a blit...  Could
       * eliminate one copy by going straight to (permanent) GART.
       *
       * Note, this will call r200ChooseTextureFormat.
       */
      _mesa_store_teximage3d(ctx, target, level, internalFormat,
			     width, height, depth, border,
                             format, type, pixels,
			     &ctx->Unpack, texObj, texImage);
      
      t->dirty_images[0] |= (1 << level);
   }
}
#endif


#if ENABLE_HW_3D_TEXTURE
static void
r200TexSubImage3D( GLcontext *ctx, GLenum target, GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, GLenum type,
                   const GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   assert( t ); /* this _should_ be true */
   if ( t ) {
      driSwapOutTextureObject( t );
   }
   else {
      t = (driTextureObject *) r200AllocTexObj( texObj );
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage3D");
         return;
      }
      texObj->DriverData = t;
   }

   _mesa_store_texsubimage3d(ctx, target, level, xoffset, yoffset, zoffset,
                             width, height, depth,
                             format, type, pixels, packing, texObj, texImage);

   t->dirty_images[0] |= (1 << level);
}
#endif



static void r200TexEnv( GLcontext *ctx, GLenum target,
			  GLenum pname, const GLfloat *param )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   if ( R200_DEBUG & DEBUG_STATE ) {
      fprintf( stderr, "%s( %s )\n",
	       __FUNCTION__, _mesa_lookup_enum_by_nr( pname ) );
   }

   /* This is incorrect: Need to maintain this data for each of
    * GL_TEXTURE_{123}D, GL_TEXTURE_RECTANGLE_NV, etc, and switch
    * between them according to _ReallyEnabled.
    */
   switch ( pname ) {
   case GL_TEXTURE_ENV_COLOR: {
      GLubyte c[4];
      GLuint envColor;
      UNCLAMPED_FLOAT_TO_RGBA_CHAN( c, texUnit->EnvColor );
      envColor = r200PackColor( 4, c[0], c[1], c[2], c[3] );
      if ( rmesa->hw.tf.cmd[TF_TFACTOR_0 + unit] != envColor ) {
	 R200_STATECHANGE( rmesa, tf );
	 rmesa->hw.tf.cmd[TF_TFACTOR_0 + unit] = envColor;
      }
      break;
   }

   case GL_TEXTURE_LOD_BIAS_EXT: {
      GLfloat bias, min;
      GLuint b;
      const int fixed_one = 0x8000000;

      /* The R200's LOD bias is a signed 2's complement value with a
       * range of -16.0 <= bias < 16.0. 
       *
       * NOTE: Add a small bias to the bias for conform mipsel.c test.
       */
      bias = *param + .01;
      min = driQueryOptionb (&rmesa->optionCache, "no_neg_lod_bias") ?
	  0.0 : -16.0;
      bias = CLAMP( bias, min, 16.0 );
      b = (int)(bias * fixed_one) & R200_LOD_BIAS_MASK;
      
      if ( (rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT_X] & R200_LOD_BIAS_MASK) != b ) {
	 R200_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT_X] &= ~R200_LOD_BIAS_MASK;
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT_X] |= b;
      }
      break;
   }
   case GL_COORD_REPLACE_ARB:
      if (ctx->Point.PointSprite) {
	 R200_STATECHANGE( rmesa, spr );
	 if ((GLenum)param[0]) {
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |= R200_PS_GEN_TEX_0 << unit;
	 } else {
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] &= ~(R200_PS_GEN_TEX_0 << unit);
	 }
      }
      break;
   default:
      return;
   }
}


/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void r200TexParameter( GLcontext *ctx, GLenum target,
				struct gl_texture_object *texObj,
				GLenum pname, const GLfloat *params )
{
   r200TexObjPtr t = (r200TexObjPtr) texObj->DriverData;

   if ( R200_DEBUG & (DEBUG_STATE|DEBUG_TEXTURE) ) {
      fprintf( stderr, "%s( %s )\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr( pname ) );
   }

   switch ( pname ) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      r200SetTexMaxAnisotropy( t, texObj->MaxAnisotropy );
      r200SetTexFilter( t, texObj->MinFilter, texObj->MagFilter );
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
   case GL_TEXTURE_WRAP_R:
      r200SetTexWrap( t, texObj->WrapS, texObj->WrapT, texObj->WrapR );
      break;

   case GL_TEXTURE_BORDER_COLOR:
      r200SetTexBorderColor( t, texObj->_BorderChan );
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



static void r200BindTexture( GLcontext *ctx, GLenum target,
			       struct gl_texture_object *texObj )
{
   if ( R200_DEBUG & (DEBUG_STATE|DEBUG_TEXTURE) ) {
      fprintf( stderr, "%s( %p ) unit=%d\n", __FUNCTION__, (void *)texObj,
	       ctx->Texture.CurrentUnit );
   }

   if ( (target == GL_TEXTURE_1D)
	|| (target == GL_TEXTURE_2D) 
#if ENABLE_HW_3D_TEXTURE
	|| (target == GL_TEXTURE_3D)
#endif
	|| (target == GL_TEXTURE_CUBE_MAP)
	|| (target == GL_TEXTURE_RECTANGLE_NV) ) {
      assert( texObj->DriverData != NULL );
   }
}


static void r200DeleteTexture( GLcontext *ctx,
				 struct gl_texture_object *texObj )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   if ( R200_DEBUG & (DEBUG_STATE|DEBUG_TEXTURE) ) {
      fprintf( stderr, "%s( %p (target = %s) )\n", __FUNCTION__, (void *)texObj,
	       _mesa_lookup_enum_by_nr( texObj->Target ) );
   }

   if ( t != NULL ) {
      if ( rmesa ) {
         R200_FIREVERTICES( rmesa );
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
static void r200TexGen( GLcontext *ctx,
			  GLenum coord,
			  GLenum pname,
			  const GLfloat *params )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   rmesa->recheck_texgen[unit] = GL_TRUE;
}


/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 * Fixup MaxAnisotropy according to user preference.
 */
static struct gl_texture_object *
r200NewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   if (!obj)
      return NULL;
   obj->MaxAnisotropy = rmesa->initialMaxAnisotropy;
   r200AllocTexObj( obj );
   return obj;
}


void r200InitTextureFuncs( struct dd_function_table *functions )
{
   /* Note: we only plug in the functions we implement in the driver
    * since _mesa_init_driver_functions() was already called.
    */
   functions->ChooseTextureFormat	= r200ChooseTextureFormat;
   functions->TexImage1D		= r200TexImage1D;
   functions->TexImage2D		= r200TexImage2D;
#if ENABLE_HW_3D_TEXTURE
   functions->TexImage3D		= r200TexImage3D;
#else
   functions->TexImage3D		= _mesa_store_teximage3d;
#endif
   functions->TexSubImage1D		= r200TexSubImage1D;
   functions->TexSubImage2D		= r200TexSubImage2D;
#if ENABLE_HW_3D_TEXTURE
   functions->TexSubImage3D		= r200TexSubImage3D;
#else
   functions->TexSubImage3D		= _mesa_store_texsubimage3d;
#endif
   functions->NewTextureObject		= r200NewTextureObject;
   functions->BindTexture		= r200BindTexture;
   functions->DeleteTexture		= r200DeleteTexture;
   functions->IsTextureResident		= driIsTextureResident;

   functions->TexEnv			= r200TexEnv;
   functions->TexParameter		= r200TexParameter;
   functions->TexGen			= r200TexGen;

   functions->CompressedTexImage2D	= r200CompressedTexImage2D;
   functions->CompressedTexSubImage2D	= r200CompressedTexSubImage2D;

   driInitTextureFormats();

#if 000
   /* moved or obsolete code */
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   driInitTextureObjects( ctx, & rmesa->swapped,
			  DRI_TEXMGR_DO_TEXTURE_1D
			  | DRI_TEXMGR_DO_TEXTURE_2D );

   /* Hack: r200NewTextureObject is not yet installed when the
    * default textures are created. Therefore set MaxAnisotropy of the
    * default 2D texture now. */
   ctx->Shared->Default2D->MaxAnisotropy = driQueryOptionf (&rmesa->optionCache,
							    "def_max_anisotropy");
#endif
}
