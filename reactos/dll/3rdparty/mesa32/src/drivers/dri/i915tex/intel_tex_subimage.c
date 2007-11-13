
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
#include "texobj.h"
#include "texstore.h"
#include "enums.h"

#include "intel_context.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static void
intelTexSubimage(GLcontext * ctx,
                 GLint dims,
                 GLenum target, GLint level,
                 GLint xoffset, GLint yoffset, GLint zoffset,
                 GLint width, GLint height, GLint depth,
                 GLenum format, GLenum type, const void *pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   GLuint dstRowStride;

   DBG("%s target %s level %d offset %d,%d %dx%d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target),
       level, xoffset, yoffset, width, height);

   intelFlush(ctx);

   pixels =
      _mesa_validate_pbo_teximage(ctx, dims, width, height, depth, format,
                                  type, pixels, packing, "glTexSubImage2D");
   if (!pixels)
      return;

   if (intelImage->mt)
      intel_region_idle(intel->intelScreen, intelImage->mt->region);

   LOCK_HARDWARE(intel);

   /* Map buffer if necessary.  Need to lock to prevent other contexts
    * from uploading the buffer under us.
    */
   if (intelImage->mt) 
      texImage->Data = intel_miptree_image_map(intel,
                                               intelImage->mt,
                                               intelImage->face,
                                               intelImage->level,
                                               &dstRowStride,
                                               texImage->ImageOffsets);

   assert(dstRowStride);

   if (!texImage->TexFormat->StoreImage(ctx, dims, texImage->_BaseFormat,
                                        texImage->TexFormat,
                                        texImage->Data,
                                        xoffset, yoffset, zoffset,
                                        dstRowStride,
                                        texImage->ImageOffsets,
                                        width, height, depth,
                                        format, type, pixels, packing)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   }

#if 0
   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      _mesa_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
#endif

   _mesa_unmap_teximage_pbo(ctx, packing);

   if (intelImage->mt) {
      intel_miptree_image_unmap(intel, intelImage->mt);
      texImage->Data = NULL;
   }

   UNLOCK_HARDWARE(intel);
}





void
intelTexSubImage3D(GLcontext * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{

   intelTexSubimage(ctx, 3,
                    target, level,
                    xoffset, yoffset, zoffset,
                    width, height, depth,
                    format, type, pixels, packing, texObj, texImage);

}



void
intelTexSubImage2D(GLcontext * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset, GLint yoffset,
                   GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{

   intelTexSubimage(ctx, 2,
                    target, level,
                    xoffset, yoffset, 0,
                    width, height, 1,
                    format, type, pixels, packing, texObj, texImage);

}


void
intelTexSubImage1D(GLcontext * ctx,
                   GLenum target,
                   GLint level,
                   GLint xoffset,
                   GLsizei width,
                   GLenum format, GLenum type,
                   const GLvoid * pixels,
                   const struct gl_pixelstore_attrib *packing,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage)
{
   intelTexSubimage(ctx, 1,
                    target, level,
                    xoffset, 0, 0,
                    width, 1, 1,
                    format, type, pixels, packing, texObj, texImage);

}
