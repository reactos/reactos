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
#include "macros.h"
#include "simple_list.h"
#include "enums.h"
#include "image.h"
#include "texstore.h"
#include "texformat.h"
#include "teximage.h"
#include "texmem.h"
#include "texobj.h"
#include "swrast/swrast.h"

#include "mm.h"

#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_context.h"
#include "intel_tex.h"
#include "intel_ioctl.h"



static GLboolean
intelValidateClientStorage( intelContextPtr intel, GLenum target,
			    GLint internalFormat,
			    GLint srcWidth, GLint srcHeight, 
			    GLenum format, GLenum type,  const void *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage)

{
   GLcontext *ctx = &intel->ctx;
   int texelBytes;

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


   /* This list is incomplete
    */
   switch ( internalFormat ) {
   case GL_RGBA:
      if ( format == GL_BGRA && type == GL_UNSIGNED_INT_8_8_8_8_REV ) {
	 texImage->TexFormat = &_mesa_texformat_argb8888;
	 texelBytes = 4;
      }
      else
	 return 0;
      break;

   case GL_RGB:
      if ( format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5 ) {
	 texImage->TexFormat = &_mesa_texformat_rgb565;
	 texelBytes = 2;
      }
      else
	 return 0;
      break;

   case GL_YCBCR_MESA:
      if ( format == GL_YCBCR_MESA && 
	   type == GL_UNSIGNED_SHORT_8_8_REV_APPLE ) {
	 texImage->TexFormat = &_mesa_texformat_ycbcr_rev;
	 texelBytes = 2;
      }
      else if ( format == GL_YCBCR_MESA && 
		(type == GL_UNSIGNED_SHORT_8_8_APPLE || 
		 type == GL_UNSIGNED_BYTE)) {
	 texImage->TexFormat = &_mesa_texformat_ycbcr;
	 texelBytes = 2;
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
      if (!intelIsAgpMemory( intel, pixels, srcHeight * srcRowStride ) ||
	  (srcRowStride & 63))
	 return 0;


      /* Have validated that _mesa_transfer_teximage would be a straight
       * memcpy at this point.  NOTE: future calls to TexSubImage will
       * overwrite the client data.  This is explicitly mentioned in the
       * extension spec.
       */
      texImage->Data = (void *)pixels;
      texImage->IsClientData = GL_TRUE;
      texImage->RowStride = srcRowStride / texelBytes;
      return 1;
   }
}

 

static void intelTexImage1D( GLcontext *ctx, GLenum target, GLint level,
			    GLint internalFormat,
			    GLint width, GLint border,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *packing,
			    struct gl_texture_object *texObj,
			    struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   assert(t);
   intelFlush( ctx );
   driSwapOutTextureObject( t );

   texImage->IsClientData = GL_FALSE;

   _mesa_store_teximage1d( ctx, target, level, internalFormat,
			   width, border, format, type,
			   pixels, packing, texObj, texImage );

   t->dirty_images[0] |= (1 << level);
}

static void intelTexSubImage1D( GLcontext *ctx, 
			       GLenum target,
			       GLint level,	
			       GLint xoffset,
				GLsizei width,
			       GLenum format, GLenum type,
			       const GLvoid *pixels,
			       const struct gl_pixelstore_attrib *packing,
			       struct gl_texture_object *texObj,
			       struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   assert(t);
   intelFlush( ctx );
   driSwapOutTextureObject( t );

   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width, 
			     format, type, pixels, packing, texObj,
			     texImage);
}


/* Handles 2D, CUBE, RECT:
 */
static void intelTexImage2D( GLcontext *ctx, GLenum target, GLint level,
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

   assert(t);
   intelFlush( ctx );
   driSwapOutTextureObject( t );
   texImage->IsClientData = GL_FALSE;

   if (intelValidateClientStorage( INTEL_CONTEXT(ctx), target, 
				   internalFormat, 
				   width, height, 
				   format, type, pixels, 
				   packing, texObj, texImage)) {
      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "%s: Using client storage\n", __FUNCTION__); 
   }
   else {
      _mesa_store_teximage2d( ctx, target, level, internalFormat,
			      width, height, border, format, type,
			      pixels, packing, texObj, texImage );

      t->dirty_images[face] |= (1 << level);
   }
}

static void intelTexSubImage2D( GLcontext *ctx, 
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

   if (texImage->IsClientData &&
       (char *)pixels == (char *)texImage->Data + 
       ((xoffset + yoffset * texImage->RowStride) * 
	texImage->TexFormat->TexelBytes)) {

      /* Notification only - no upload required */
   }
   else {
      assert( t ); /* this _should_ be true */
      intelFlush( ctx );
      driSwapOutTextureObject( t );

      _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
				height, format, type, pixels, packing, texObj,
				texImage);

      t->dirty_images[face] |= (1 << level);
   }
}

static void intelCompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
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

   assert(t);
   intelFlush( ctx );
   
   driSwapOutTextureObject( t );
   texImage->IsClientData = GL_FALSE;

   if (INTEL_DEBUG & DEBUG_TEXTURE)
     fprintf(stderr, "%s: Using normal storage\n", __FUNCTION__); 
   
   _mesa_store_compressed_teximage2d(ctx, target, level, internalFormat, width,
				     height, border, imageSize, data, texObj, texImage);
   
   t->dirty_images[face] |= (1 << level);
}


static void intelCompressedTexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
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
   intelFlush( ctx );
   driSwapOutTextureObject( t );
   
   _mesa_store_compressed_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
					height, format, imageSize, data, texObj, texImage);
   
   t->dirty_images[face] |= (1 << level);
}


static void intelTexImage3D( GLcontext *ctx, GLenum target, GLint level,
                            GLint internalFormat,
                            GLint width, GLint height, GLint depth,
                            GLint border,
                            GLenum format, GLenum type, const GLvoid *pixels,
                            const struct gl_pixelstore_attrib *packing,
                            struct gl_texture_object *texObj,
                            struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   assert(t);
   driSwapOutTextureObject( t );
   texImage->IsClientData = GL_FALSE;

   _mesa_store_teximage3d(ctx, target, level, internalFormat,
			  width, height, depth, border,
			  format, type, pixels,
			  &ctx->Unpack, texObj, texImage);
   
   t->dirty_images[0] |= (1 << level);
}


static void
intelTexSubImage3D( GLcontext *ctx, GLenum target, GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, GLenum type,
                   const GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage )
{
   driTextureObject * t = (driTextureObject *) texObj->DriverData;

   assert( t ); /* this _should_ be true */
   driSwapOutTextureObject( t );

   _mesa_store_texsubimage3d(ctx, target, level, xoffset, yoffset, zoffset,
                             width, height, depth,
                             format, type, pixels, packing, texObj, texImage);

   t->dirty_images[0] |= (1 << level);
}




static void intelDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   driTextureObject * t = (driTextureObject *) tObj->DriverData;

   if ( t != NULL ) {
      intelFlush( ctx );
      driDestroyTextureObject( t );
   }
   
   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}


static const struct gl_texture_format *
intelChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			 GLenum format, GLenum type )
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   const GLboolean do32bpt = ( intel->intelScreen->cpp == 4 &&
			       intel->intelScreen->tex.size > 4*1024*1024);

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
      return &_mesa_texformat_a8;

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

   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      return &_mesa_texformat_z16;

   default:
      fprintf(stderr, "unexpected texture format %s in %s\n", 
	      _mesa_lookup_enum_by_nr(internalFormat),
	      __FUNCTION__);
      return NULL;
   }

   return NULL; /* never get here */
}



void intelDestroyTexObj(intelContextPtr intel, intelTextureObjectPtr t)
{
   unsigned   i;

   if ( intel == NULL ) 
      return;

   if ( t->age > intel->dirtyAge )
      intel->dirtyAge = t->age;

   for ( i = 0 ; i < MAX_TEXTURE_UNITS ; i++ ) {
      if ( t == intel->CurrentTexObj[ i ] ) 
	 intel->CurrentTexObj[ i ] = NULL;
   }
}



/* Upload an image from mesa's internal copy.  Image may be 1D, 2D or
 * 3D.  Cubemaps are expanded elsewhere.
 */
static void intelUploadTexImage( intelContextPtr intel,
				 intelTextureObjectPtr t,
				 const struct gl_texture_image *image,
				 const GLuint offset )
{

   if (!image || !image->Data) 
      return;

   if (image->Depth == 1 && image->IsClientData) {
      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "Blit uploading\n");

      /* Do it with a blit.
       */
      intelEmitCopyBlitLocked( intel,
			       image->TexFormat->TexelBytes,
			       image->RowStride, /* ? */
			       intelGetMemoryOffsetMESA( NULL, 0, image->Data ),
			       t->Pitch / image->TexFormat->TexelBytes,
			       intelGetMemoryOffsetMESA( NULL, 0, t->BufAddr + offset ),
			       0, 0,
			       0, 0,
			       image->Width,
			       image->Height);
   }
   else if (image->IsCompressed) {
      GLuint row_len = 0;
      GLubyte *dst = (GLubyte *)(t->BufAddr + offset);
      GLubyte *src = (GLubyte *)image->Data;
      GLuint j;

      /* must always copy whole blocks (8/16 bytes) */
      switch (image->InternalFormat) {
	case GL_COMPRESSED_RGB_FXT1_3DFX:
	case GL_COMPRESSED_RGBA_FXT1_3DFX:
	case GL_RGB_S3TC:
	case GL_RGB4_S3TC:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	  row_len = (image->Width * 2 + 7) & ~7;
	  break;
	case GL_RGBA_S3TC:
	case GL_RGBA4_S3TC:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
	  row_len = (image->Width * 4 + 15) & ~15;
	  break;
	default:
	  fprintf(stderr,"Internal Compressed format not supported %d\n", image->InternalFormat);
	  break;
      }

      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, 
		 "Upload image %dx%dx%d offset %xm row_len %x "
		 "pitch %x depth_pitch %x\n",
		 image->Width, image->Height, image->Depth, offset,
		 row_len, t->Pitch, t->depth_pitch);

      if (row_len) {
	 for (j = 0 ; j < (image->Height + 3)/4 ; j++, dst += (t->Pitch)) {
	   __memcpy(dst, src, row_len );
	   src += row_len;
	 }
      }
   }
   /* Time for another vtbl entry:
    */
   else if (intel->intelScreen->deviceID == PCI_CHIP_I945_G ||
            intel->intelScreen->deviceID == PCI_CHIP_I945_GM ||
            intel->intelScreen->deviceID == PCI_CHIP_I945_GME ||
            intel->intelScreen->deviceID == PCI_CHIP_G33_G ||
            intel->intelScreen->deviceID == PCI_CHIP_Q33_G ||
            intel->intelScreen->deviceID == PCI_CHIP_Q35_G) {
      GLuint row_len = image->Width * image->TexFormat->TexelBytes;
      GLubyte *dst = (GLubyte *)(t->BufAddr + offset);
      GLubyte *src = (GLubyte *)image->Data;
      GLuint d, j;

      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, 
		 "Upload image %dx%dx%d offset %xm row_len %x "
		 "pitch %x depth_pitch %x\n",
		 image->Width, image->Height, image->Depth, offset,
		 row_len, t->Pitch, t->depth_pitch);

      if (row_len == t->Pitch) {
	 memcpy( dst, src, row_len * image->Height * image->Depth );
      }
      else { 
	 GLuint x = 0, y = 0;

	 for (d = 0 ; d < image->Depth ; d++) {
	    GLubyte *dst0 = dst + x + y * t->Pitch;

	    for (j = 0 ; j < image->Height ; j++) {
	       __memcpy(dst0, src, row_len );
	       src += row_len;
	       dst0 += t->Pitch;
	    }

	    x += MIN2(4, row_len); /* Guess: 4 byte minimum alignment */
	    if (x > t->Pitch) {
	       x = 0;
	       y += image->Height;
	    }
	 }
      }

   }
   else {
      GLuint row_len = image->Width * image->TexFormat->TexelBytes;
      GLubyte *dst = (GLubyte *)(t->BufAddr + offset);
      GLubyte *src = (GLubyte *)image->Data;
      GLuint d, j;

      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, 
		 "Upload image %dx%dx%d offset %xm row_len %x "
		 "pitch %x depth_pitch %x\n",
		 image->Width, image->Height, image->Depth, offset,
		 row_len, t->Pitch, t->depth_pitch);

      if (row_len == t->Pitch) {
	 for (d = 0; d < image->Depth; d++) {
	    memcpy( dst, src, t->Pitch * image->Height );
	    dst += t->depth_pitch;
	    src += row_len * image->Height;
	 }
      }
      else { 
	 for (d = 0 ; d < image->Depth ; d++) {
	    for (j = 0 ; j < image->Height ; j++) {
	       __memcpy(dst, src, row_len );
	       src += row_len;
	       dst += t->Pitch;
	    }

	    dst += t->depth_pitch - (t->Pitch * image->Height);
	 }
      }
   }
}



int intelUploadTexImages( intelContextPtr intel, 
			  intelTextureObjectPtr t,
			  GLuint face)
{
   const int numLevels = t->base.lastLevel - t->base.firstLevel + 1;
   const struct gl_texture_image *firstImage = t->image[face][t->base.firstLevel].image;
   int pitch = firstImage->RowStride * firstImage->TexFormat->TexelBytes;

   /* Can we texture out of the existing client data? */
   if ( numLevels == 1 &&
	firstImage->IsClientData &&
	(pitch & 3) == 0) {

      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 fprintf(stderr, "AGP texturing from client memory\n");

      t->TextureOffset = intelAgpOffsetFromVirtual( intel, firstImage->Data );
      t->BufAddr = 0;
      t->dirty = ~0;
      return GL_TRUE;
   }
   else {
      if (INTEL_DEBUG & DEBUG_TEXTURE) 
	 fprintf(stderr, "Uploading client data to agp\n");

      INTEL_FIREVERTICES( intel );
      LOCK_HARDWARE( intel );

      if ( t->base.memBlock == NULL ) {
	 int heap;

	 heap = driAllocateTexture( intel->texture_heaps, intel->nr_heaps,
				    (driTextureObject *) t );
	 if ( heap == -1 ) {
	    UNLOCK_HARDWARE( intel );
	    return GL_FALSE;
	 }

	 /* Set the base offset of the texture image */
	 t->BufAddr = (GLubyte *) (intel->intelScreen->tex.map + 
				   t->base.memBlock->ofs);
	 t->TextureOffset = intel->intelScreen->tex.offset + t->base.memBlock->ofs;
	 t->dirty = ~0;
      }


      /* Let the world know we've used this memory recently.
       */
      driUpdateTextureLRU( (driTextureObject *) t );


      /* Upload any images that are new */
      if (t->base.dirty_images[face]) {
	 int i;

 	 intelWaitForIdle( intel );
	    
	 for (i = 0 ; i < numLevels ; i++) { 
	    int level = i + t->base.firstLevel;

	    if (t->base.dirty_images[face] & (1<<level)) {

	       const struct gl_texture_image *image = t->image[face][i].image;
	       GLuint offset = t->image[face][i].offset;

     	       if (INTEL_DEBUG & DEBUG_TEXTURE)
	          fprintf(stderr, "upload level %d, offset %x\n", 
			  level, offset);

	       intelUploadTexImage( intel, t, image, offset );
	    }
	 }
	 t->base.dirty_images[face] = 0;
	 intel->perf_boxes |= I830_BOX_TEXTURE_LOAD;
      }
      
      UNLOCK_HARDWARE( intel );
      return GL_TRUE;
   }
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
intelNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj = _mesa_new_texture_object(ctx, name, target);
   INTEL_CONTEXT(ctx)->vtbl.alloc_tex_obj( obj );
   return obj;
}


void intelInitTextureFuncs( struct dd_function_table *functions )
{
   functions->NewTextureObject          = intelNewTextureObject;
   functions->ChooseTextureFormat       = intelChooseTextureFormat;
   functions->TexImage1D                = intelTexImage1D;
   functions->TexImage2D                = intelTexImage2D;
   functions->TexImage3D                = intelTexImage3D;
   functions->TexSubImage1D             = intelTexSubImage1D;
   functions->TexSubImage2D             = intelTexSubImage2D;
   functions->TexSubImage3D             = intelTexSubImage3D;
   functions->CopyTexImage1D            = _swrast_copy_teximage1d;
   functions->CopyTexImage2D            = _swrast_copy_teximage2d;
   functions->CopyTexSubImage1D         = _swrast_copy_texsubimage1d;
   functions->CopyTexSubImage2D         = _swrast_copy_texsubimage2d;
   functions->CopyTexSubImage3D         = _swrast_copy_texsubimage3d;
   functions->DeleteTexture             = intelDeleteTexture;
   functions->UpdateTexturePalette      = NULL;
   functions->IsTextureResident         = driIsTextureResident;
   functions->TestProxyTexImage         = _mesa_test_proxy_teximage;
   functions->DeleteTexture             = intelDeleteTexture;
   functions->CompressedTexImage2D      = intelCompressedTexImage2D;
   functions->CompressedTexSubImage2D   = intelCompressedTexSubImage2D;
}
