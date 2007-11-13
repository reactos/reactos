/**************************************************************************

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
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86$ */

/*
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_alloc.h"
#include "sis_tex.h"

#include "swrast/swrast.h"
#include "imports.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"
#include "texobj.h"

#include "xmlpool.h"

#define ALIGN(value, align) (GLubyte *)((long)(value + align - 1) & ~(align - 1))

#define TEXTURE_HW_ALIGNMENT 4
#define TEXTURE_HW_PLUS (4 + 4)

static sisTexObjPtr
sisAllocTexObj( struct gl_texture_object *texObj )
{
   sisTexObjPtr t;

   t = (sisTexObjPtr) CALLOC_STRUCT( sis_tex_obj );
   texObj->DriverData = t;
   return t;
}

static void
sisAllocTexImage( sisContextPtr smesa, sisTexObjPtr t, int level,
		  const struct gl_texture_image *image )
{
   char *addr;
   int size, texel_size;

   if (t->format == 0) {
      t->format = image->_BaseFormat;
      switch (image->TexFormat->MesaFormat)
      {
      case MESA_FORMAT_ARGB8888:
         t->hwformat = TEXEL_ARGB_8888_32;
         break;
      case MESA_FORMAT_ARGB4444:
         t->hwformat = TEXEL_ARGB_4444_16;
         break;
      case MESA_FORMAT_ARGB1555:
         t->hwformat = TEXEL_ARGB_1555_16;
         break;
      case MESA_FORMAT_RGB565:
         t->hwformat = TEXEL_RGB_565_16;
         break;
      case MESA_FORMAT_RGB332:
         t->hwformat = TEXEL_RGB_332_8;
         break;
      case MESA_FORMAT_I8:
         t->hwformat = TEXEL_I8;
         break;
      case MESA_FORMAT_A8:
         t->hwformat = TEXEL_A8;
         break;
      case MESA_FORMAT_L8:
         t->hwformat = TEXEL_L8;
         break;
      case MESA_FORMAT_AL88:
         t->hwformat = TEXEL_AL88;
         break;
      case MESA_FORMAT_YCBCR:
         t->hwformat = TEXEL_YUV422;
         break;
      case MESA_FORMAT_YCBCR_REV:
         t->hwformat = TEXEL_VUY422;
         break;
      default:
         sis_fatal_error("Bad texture format 0x%x.\n",
			 image->TexFormat->MesaFormat);
      }
   }
   assert(t->format == image->_BaseFormat);

   texel_size = image->TexFormat->TexelBytes;
   size = image->Width * image->Height * texel_size + TEXTURE_HW_PLUS;

   addr = sisAllocFB( smesa, size, &t->image[level].handle );
   if (addr == NULL) {
      addr = sisAllocAGP( smesa, size, &t->image[level].handle );
      if (addr == NULL)
         sis_fatal_error("Failure to allocate texture memory.\n");
      t->image[level].memType = AGP_TYPE;
   }
   else
      t->image[level].memType = VIDEO_TYPE;
   
   t->image[level].Data = ALIGN(addr, TEXTURE_HW_ALIGNMENT);
   t->image[level].pitch = image->Width * texel_size;
   t->image[level].size = image->Width * image->Height * texel_size;
   t->numImages++;
}

static void
sisFreeTexImage( sisContextPtr smesa, sisTexObjPtr t, int level )
{
   assert(level >= 0);
   assert(level < SIS_MAX_TEXTURE_LEVELS);
   if (t->image[level].Data == NULL)
      return;

   switch (t->image[level].memType)
   {
   case VIDEO_TYPE:
      sisFreeFB( smesa, t->image[level].handle );
      break;
   case AGP_TYPE:
      sisFreeAGP( smesa, t->image[level].handle );
      break;
   }
   t->image[level].Data = NULL;
   t->image[level].handle = NULL;
   /* If there are no textures loaded any more, reset the hw format so the 
    * object can be reused for new formats
    */
   t->numImages--;
   if (t->numImages == 0) {
      t->format = 0;
      t->hwformat = 0;
   }
}

static void 
sisTexEnv( GLcontext *ctx, GLenum target, GLenum pname, const GLfloat *param )
{
  sisContextPtr smesa = SIS_CONTEXT(ctx);

  smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
}

static void
sisTexParameter( GLcontext *ctx, GLenum target,
                 struct gl_texture_object *texObj, GLenum pname,
                 const GLfloat *params )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void
sisBindTexture( GLcontext *ctx, GLenum target,
                struct gl_texture_object *texObj )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;

   if ( target == GL_TEXTURE_2D || target == GL_TEXTURE_1D ) {
      if ( texObj->DriverData == NULL ) {
         sisAllocTexObj( texObj );
      }
   }

   t = texObj->DriverData;
   if (!t)
      return;

   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format) {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void
sisDeleteTexture( GLcontext * ctx, struct gl_texture_object *texObj )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;
   int i;

   smesa->clearTexCache = GL_TRUE;

   t = texObj->DriverData;
   if (t == NULL) {
      /* 
       * this shows the texture is default object and never be a 
       * argument of sisTexImage*
       */
      return;
   }
   for (i = 0; i < SIS_MAX_TEXTURE_LEVELS; i++) {
      sisFreeTexImage( smesa, t, i );
   }

   FREE(t);
   texObj->DriverData = NULL;
   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, texObj);
}

static GLboolean sisIsTextureResident( GLcontext * ctx,
			 		 struct gl_texture_object *texObj )
{
  return (texObj->DriverData != NULL);
}

static const struct gl_texture_format *
sisChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			  GLenum format, GLenum type )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   const GLboolean do32bpt =
       (smesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_32);
   const GLboolean force16bpt =
       (smesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FORCE_16);

   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      switch ( type ) {
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
	 return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return &_mesa_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return &_mesa_texformat_argb1555;
      default:
         return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;
      }

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      switch ( type ) {
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return &_mesa_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return &_mesa_texformat_argb1555;
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
	 return &_mesa_texformat_rgb565;
      default:
         return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;
      }

   case GL_RGBA8:
   case GL_RGBA12:
   case GL_RGBA16:
      return !force16bpt ?
	  &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_RGB10_A2:
      return !force16bpt ?
	  &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555;

   case GL_RGBA4:
   case GL_RGBA2:
      return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGB5:
   case GL_RGB4:
      return &_mesa_texformat_rgb565;

   case GL_R3_G3_B2:
      return &_mesa_texformat_rgb332;

   case GL_ALPHA:
   case GL_ALPHA4:		/* FIXME: This could use its own texstore */
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:		/* FIXME: This could use its own texstore */
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:	/* FIXME: This could use its own texstore */
   case GL_LUMINANCE6_ALPHA2:	/* FIXME: This could use its own texstore */
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:	/* FIXME: This could use its own texstore */
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
      if (type == GL_UNSIGNED_SHORT_8_8_APPLE ||
          type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   default:
      _mesa_problem(ctx, "unexpected format in sisDDChooseTextureFormat: %d",
         internalFormat);
      return NULL;
   }
}

static void sisTexImage1D( GLcontext *ctx, GLenum target, GLint level,
			     GLint internalFormat,
			     GLint width, GLint border,
			     GLenum format, GLenum type, const GLvoid *pixels,
			     const struct gl_pixelstore_attrib *packing,
			     struct gl_texture_object *texObj,
			     struct gl_texture_image *texImage )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   /* Note, this will call sisChooseTextureFormat */
   _mesa_store_teximage1d( ctx, target, level, internalFormat,
			   width, border, format, type,
			   pixels, packing, texObj, texImage );

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   WaitEngIdle(smesa);
   memcpy(t->image[level].Data, texImage->Data, t->image[level].size);
   
   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}


static void sisTexSubImage1D( GLcontext *ctx,
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
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;
   GLuint copySize;
   GLint texelBytes;
   const char *src;
   GLubyte *dst;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width,
			     format, type, pixels, packing, texObj,
			     texImage);

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   WaitEngIdle(smesa);
   texelBytes = texImage->TexFormat->TexelBytes;

   copySize = width * texelBytes;
   src = (char *)texImage->Data + xoffset * texelBytes;
   dst = t->image[level].Data + xoffset * texelBytes;

   memcpy( dst, src, copySize );

   smesa->clearTexCache = GL_TRUE;

   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void sisTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			     GLint internalFormat,
			     GLint width, GLint height, GLint border,
			     GLenum format, GLenum type, const GLvoid *pixels,
			     const struct gl_pixelstore_attrib *packing,
			     struct gl_texture_object *texObj,
			     struct gl_texture_image *texImage )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   /* Note, this will call sisChooseTextureFormat */
   _mesa_store_teximage2d(ctx, target, level, internalFormat,
                          width, height, border, format, type, pixels,
                          &ctx->Unpack, texObj, texImage);

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   WaitEngIdle(smesa);
   memcpy(t->image[level].Data, texImage->Data, t->image[level].size);
   
   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void sisTexSubImage2D( GLcontext *ctx,
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
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;
   GLuint copySize;
   GLint texelBytes;
   const char *src;
   GLubyte *dst;
   int j;
   GLuint soffset;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
			     height, format, type, pixels, packing, texObj,
			     texImage);

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   WaitEngIdle(smesa);
   texelBytes = texImage->TexFormat->TexelBytes;

   copySize = width * texelBytes;
   src = (char *)texImage->Data + (xoffset + yoffset * texImage->Width) *
      texelBytes;
   dst = t->image[level].Data + (xoffset + yoffset * texImage->Width) *
      texelBytes;
   soffset = texImage->Width * texelBytes;

   for (j = yoffset; j < yoffset + height; j++) {
      memcpy( dst, src, copySize );
      src += soffset;
      dst += soffset;
   }

   smesa->clearTexCache = GL_TRUE;

   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
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
sisNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   return obj;
}


void sisInitTextureFuncs( struct dd_function_table *functions )
{
   functions->TexEnv			= sisTexEnv;
   functions->ChooseTextureFormat	= sisChooseTextureFormat;
   functions->TexImage1D		= sisTexImage1D;
   functions->TexSubImage1D		= sisTexSubImage1D;
   functions->TexImage2D		= sisTexImage2D;
   functions->TexSubImage2D		= sisTexSubImage2D;
   functions->TexParameter		= sisTexParameter;
   functions->BindTexture		= sisBindTexture;
   functions->NewTextureObject		= sisNewTextureObject;
   functions->DeleteTexture		= sisDeleteTexture;
   functions->IsTextureResident	= sisIsTextureResident;
}
