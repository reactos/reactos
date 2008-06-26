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

#include "mtypes.h"
#include "image.h"
#include "texstore.h"
#include "texformat.h"
#include "teximage.h"
#include "texobj.h"
#include "swrast/swrast.h"


#include "intel_context.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"


static GLuint target_to_face( GLenum target )
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return ((GLuint) target - 
	      (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X);
   default:
      return 0;
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
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   _mesa_store_teximage1d( ctx, target, level, internalFormat,
			   width, border, format, type,
			   pixels, packing, texObj, texImage );

   intelObj->dirty_images[0] |= (1 << level);
   intelObj->dirty |= 1;
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
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width, 
			     format, type, pixels, packing, texObj,
			     texImage);

   intelObj->dirty_images[0] |= (1 << level);
   intelObj->dirty |= 1;
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
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   GLuint face = target_to_face(target);

   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );

   intelObj->dirty_images[face] |= (1 << level);
   intelObj->dirty |= 1 << face;
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
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   GLuint face = target_to_face(target);

   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);

   intelObj->dirty_images[face] |= (1 << level);
   intelObj->dirty |= 1 << face;
}

static void intelCompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
                              GLint internalFormat,
                              GLint width, GLint height, GLint border,
                              GLsizei imageSize, const GLvoid *data,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage )
{
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   GLuint face = target_to_face(target);

   _mesa_store_compressed_teximage2d(ctx, target, level, internalFormat, width,
				     height, border, imageSize, data, texObj, texImage);
   
   intelObj->dirty_images[face] |= (1 << level);
   intelObj->dirty |= 1 << face;
}


static void intelCompressedTexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
                                 GLint xoffset, GLint yoffset,
                                 GLsizei width, GLsizei height,
                                 GLenum format,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage )
{
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   GLuint face = target_to_face(target);

   _mesa_store_compressed_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
					height, format, imageSize, data, texObj, texImage);
   
   intelObj->dirty_images[face] |= (1 << level);
   intelObj->dirty |= 1 << face;
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
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   _mesa_store_teximage3d(ctx, target, level, internalFormat,
			  width, height, depth, border,
			  format, type, pixels,
			  &ctx->Unpack, texObj, texImage);
   
   intelObj->dirty_images[0] |= (1 << level);
   intelObj->dirty |= 1 << 0;
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
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   _mesa_store_texsubimage3d(ctx, target, level, xoffset, yoffset, zoffset,
                             width, height, depth,
                             format, type, pixels, packing, texObj, texImage);

   intelObj->dirty_images[0] |= (1 << level);
   intelObj->dirty |= 1 << 0;
}




static struct gl_texture_object *intelNewTextureObject( GLcontext *ctx, 
							GLuint name, 
							GLenum target )
{
   struct intel_texture_object *obj = CALLOC_STRUCT(intel_texture_object);

   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

static GLboolean intelIsTextureResident(GLcontext *ctx,
                                      struct gl_texture_object *texObj)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   
   return 
      intelObj->mt && 
      intelObj->mt->region && 
      intel_is_region_resident(intel, intelObj->mt->region);
#endif
   return 1;
}



static void intelTexParameter( GLcontext *ctx, 
			       GLenum target,
			       struct gl_texture_object *texObj,
			       GLenum pname, 
			       const GLfloat *params )
{
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
 
   switch (pname) {
      /* Anything which can affect the calculation of firstLevel and
       * lastLevel, as changes to these may invalidate the miptree.
       */
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
   case GL_TEXTURE_BASE_LEVEL:
   case GL_TEXTURE_MAX_LEVEL:
   case GL_TEXTURE_MIN_LOD:
   case GL_TEXTURE_MAX_LOD:
      intelObj->dirty |= 1;
      break;

   default:
      break;
   }
}


static void
intel_delete_texture_object( GLcontext *ctx, struct gl_texture_object *texObj )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   if (intelObj->mt)
      intel_miptree_destroy(intel, intelObj->mt);

   _mesa_delete_texture_object( ctx, texObj );
}

void intelInitTextureFuncs( struct dd_function_table *functions )
{
   functions->NewTextureObject          = intelNewTextureObject;
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
   functions->DeleteTexture             = intel_delete_texture_object;
   functions->UpdateTexturePalette      = NULL;
   functions->IsTextureResident = intelIsTextureResident;
   functions->TestProxyTexImage         = _mesa_test_proxy_teximage;
   functions->CompressedTexImage2D      = intelCompressedTexImage2D;
   functions->CompressedTexSubImage2D   = intelCompressedTexSubImage2D;
   functions->TexParameter              = intelTexParameter;
}





