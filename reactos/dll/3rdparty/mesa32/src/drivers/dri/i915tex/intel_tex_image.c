
#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "enums.h"
#include "colortab.h"
#include "convolve.h"
#include "context.h"
#include "simple_list.h"
#include "texcompress.h"
#include "texformat.h"
#include "texobj.h"
#include "texstore.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_ioctl.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/* Functions to store texture images.  Where possible, mipmap_tree's
 * will be created or further instantiated with image data, otherwise
 * images will be stored in malloc'd memory.  A validation step is
 * required to pull those images into a mipmap tree, or otherwise
 * decide a fallback is required.
 */


static int
logbase2(int n)
{
   GLint i = 1;
   GLint log2 = 0;

   while (n > i) {
      i *= 2;
      log2++;
   }

   return log2;
}


/* Otherwise, store it in memory if (Border != 0) or (any dimension ==
 * 1).
 *    
 * Otherwise, if max_level >= level >= min_level, create tree with
 * space for textures from min_level down to max_level.
 *
 * Otherwise, create tree with space for textures from (level
 * 0)..(1x1).  Consider pruning this tree at a validation if the
 * saving is worth it.
 */
static void
guess_and_alloc_mipmap_tree(struct intel_context *intel,
                            struct intel_texture_object *intelObj,
                            struct intel_texture_image *intelImage)
{
   GLuint firstLevel;
   GLuint lastLevel;
   GLuint width = intelImage->base.Width;
   GLuint height = intelImage->base.Height;
   GLuint depth = intelImage->base.Depth;
   GLuint l2width, l2height, l2depth;
   GLuint i, comp_byte = 0;

   DBG("%s\n", __FUNCTION__);

   if (intelImage->base.Border)
      return;

   if (intelImage->level > intelObj->base.BaseLevel &&
       (intelImage->base.Width == 1 ||
        (intelObj->base.Target != GL_TEXTURE_1D &&
         intelImage->base.Height == 1) ||
        (intelObj->base.Target == GL_TEXTURE_3D &&
         intelImage->base.Depth == 1)))
      return;

   /* If this image disrespects BaseLevel, allocate from level zero.
    * Usually BaseLevel == 0, so it's unlikely to happen.
    */
   if (intelImage->level < intelObj->base.BaseLevel)
      firstLevel = 0;
   else
      firstLevel = intelObj->base.BaseLevel;


   /* Figure out image dimensions at start level. 
    */
   for (i = intelImage->level; i > firstLevel; i--) {
      width <<= 1;
      if (height != 1)
         height <<= 1;
      if (depth != 1)
         depth <<= 1;
   }

   /* Guess a reasonable value for lastLevel.  This is probably going
    * to be wrong fairly often and might mean that we have to look at
    * resizable buffers, or require that buffers implement lazy
    * pagetable arrangements.
    */
   if ((intelObj->base.MinFilter == GL_NEAREST ||
        intelObj->base.MinFilter == GL_LINEAR) &&
       intelImage->level == firstLevel) {
      lastLevel = firstLevel;
   }
   else {
      l2width = logbase2(width);
      l2height = logbase2(height);
      l2depth = logbase2(depth);
      lastLevel = firstLevel + MAX2(MAX2(l2width, l2height), l2depth);
   }

   assert(!intelObj->mt);
   if (intelImage->base.IsCompressed)
      comp_byte = intel_compressed_num_bytes(intelImage->base.TexFormat->MesaFormat);
   intelObj->mt = intel_miptree_create(intel,
                                       intelObj->base.Target,
                                       intelImage->base.InternalFormat,
                                       firstLevel,
                                       lastLevel,
                                       width,
                                       height,
                                       depth,
                                       intelImage->base.TexFormat->TexelBytes,
                                       comp_byte);

   DBG("%s - success\n", __FUNCTION__);
}




static GLuint
target_to_face(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return ((GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X);
   default:
      return 0;
   }
}

/* There are actually quite a few combinations this will work for,
 * more than what I've listed here.
 */
static GLboolean
check_pbo_format(GLint internalFormat,
                 GLenum format, GLenum type,
                 const struct gl_texture_format *mesa_format)
{
   switch (internalFormat) {
   case 4:
   case GL_RGBA:
      return (format == GL_BGRA &&
              (type == GL_UNSIGNED_BYTE ||
               type == GL_UNSIGNED_INT_8_8_8_8_REV) &&
              mesa_format == &_mesa_texformat_argb8888);
   case 3:
   case GL_RGB:
      return (format == GL_RGB &&
              type == GL_UNSIGNED_SHORT_5_6_5 &&
              mesa_format == &_mesa_texformat_rgb565);
   case GL_YCBCR_MESA:
      return (type == GL_UNSIGNED_SHORT_8_8_MESA || type == GL_UNSIGNED_BYTE);
   default:
      return GL_FALSE;
   }
}


/* XXX: Do this for TexSubImage also:
 */
static GLboolean
try_pbo_upload(struct intel_context *intel,
               struct intel_texture_image *intelImage,
               const struct gl_pixelstore_attrib *unpack,
               GLint internalFormat,
               GLint width, GLint height,
               GLenum format, GLenum type, const void *pixels)
{
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset, src_stride;
   GLuint dst_offset, dst_stride;

   if (!pbo ||
       intel->ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      _mesa_printf("%s: failure 1\n", __FUNCTION__);
      return GL_FALSE;
   }

   src_offset = (GLuint) pixels;

   if (unpack->RowLength > 0)
      src_stride = unpack->RowLength;
   else
      src_stride = width;

   dst_offset = intel_miptree_image_offset(intelImage->mt,
                                           intelImage->face,
                                           intelImage->level);

   dst_stride = intelImage->mt->pitch;

   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);
   {
      struct _DriBufferObject *src_buffer =
         intel_bufferobj_buffer(intel, pbo, INTEL_READ);
      struct _DriBufferObject *dst_buffer =
         intel_region_buffer(intel->intelScreen, intelImage->mt->region,
                             INTEL_WRITE_FULL);


      intelEmitCopyBlit(intel,
                        intelImage->mt->cpp,
                        src_stride, src_buffer, src_offset,
                        dst_stride, dst_buffer, dst_offset,
                        0, 0, 0, 0, width, height,
			GL_COPY);

      intel_batchbuffer_flush(intel->batch);
   }
   UNLOCK_HARDWARE(intel);

   return GL_TRUE;
}



static GLboolean
try_pbo_zcopy(struct intel_context *intel,
              struct intel_texture_image *intelImage,
              const struct gl_pixelstore_attrib *unpack,
              GLint internalFormat,
              GLint width, GLint height,
              GLenum format, GLenum type, const void *pixels)
{
   struct intel_buffer_object *pbo = intel_buffer_object(unpack->BufferObj);
   GLuint src_offset, src_stride;
   GLuint dst_offset, dst_stride;

   if (!pbo ||
       intel->ctx._ImageTransferState ||
       unpack->SkipPixels || unpack->SkipRows) {
      _mesa_printf("%s: failure 1\n", __FUNCTION__);
      return GL_FALSE;
   }

   src_offset = (GLuint) pixels;

   if (unpack->RowLength > 0)
      src_stride = unpack->RowLength;
   else
      src_stride = width;

   dst_offset = intel_miptree_image_offset(intelImage->mt,
                                           intelImage->face,
                                           intelImage->level);

   dst_stride = intelImage->mt->pitch;

   if (src_stride != dst_stride || dst_offset != 0 || src_offset != 0) {
      _mesa_printf("%s: failure 2\n", __FUNCTION__);
      return GL_FALSE;
   }

   intel_region_attach_pbo(intel->intelScreen, intelImage->mt->region, pbo);

   return GL_TRUE;
}






static void
intelTexImage(GLcontext * ctx,
              GLint dims,
              GLenum target, GLint level,
              GLint internalFormat,
              GLint width, GLint height, GLint depth,
              GLint border,
              GLenum format, GLenum type, const void *pixels,
              const struct gl_pixelstore_attrib *unpack,
              struct gl_texture_object *texObj,
              struct gl_texture_image *texImage, GLsizei imageSize, int compressed)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   GLint postConvWidth = width;
   GLint postConvHeight = height;
   GLint texelBytes, sizeInBytes;
   GLuint dstRowStride;


   DBG("%s target %s level %d %dx%dx%d border %d\n", __FUNCTION__,
       _mesa_lookup_enum_by_nr(target), level, width, height, depth, border);

   intelFlush(ctx);

   intelImage->face = target_to_face(target);
   intelImage->level = level;

   if (ctx->_ImageTransferState & IMAGE_CONVOLUTION_BIT) {
      _mesa_adjust_image_for_convolution(ctx, dims, &postConvWidth,
                                         &postConvHeight);
   }

   /* choose the texture format */
   texImage->TexFormat = intelChooseTextureFormat(ctx, internalFormat,
                                                  format, type);

   _mesa_set_fetch_functions(texImage, dims);

   if (texImage->TexFormat->TexelBytes == 0) {
      /* must be a compressed format */
      texelBytes = 0;
      texImage->IsCompressed = GL_TRUE;
      texImage->CompressedSize =
	 ctx->Driver.CompressedTextureSize(ctx, texImage->Width,
					   texImage->Height, texImage->Depth,
					   texImage->TexFormat->MesaFormat);
   } else {
      texelBytes = texImage->TexFormat->TexelBytes;
      
      /* Minimum pitch of 32 bytes */
      if (postConvWidth * texelBytes < 32) {
	 postConvWidth = 32 / texelBytes;
	 texImage->RowStride = postConvWidth;
      }
      
      assert(texImage->RowStride == postConvWidth);
   }

   /* Release the reference to a potentially orphaned buffer.   
    * Release any old malloced memory.
    */
   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
      assert(!texImage->Data);
   }
   else if (texImage->Data) {
      _mesa_align_free(texImage->Data);
   }

   /* If this is the only texture image in the tree, could call
    * bmBufferData with NULL data to free the old block and avoid
    * waiting on any outstanding fences.
    */
   if (intelObj->mt &&
       intelObj->mt->first_level == level &&
       intelObj->mt->last_level == level &&
       intelObj->mt->target != GL_TEXTURE_CUBE_MAP_ARB &&
       !intel_miptree_match_image(intelObj->mt, &intelImage->base,
                                  intelImage->face, intelImage->level)) {

      DBG("release it\n");
      intel_miptree_release(intel, &intelObj->mt);
      assert(!intelObj->mt);
   }

   if (!intelObj->mt) {
      guess_and_alloc_mipmap_tree(intel, intelObj, intelImage);
      if (!intelObj->mt) {
	 DBG("guess_and_alloc_mipmap_tree: failed\n");
      }
   }

   assert(!intelImage->mt);

   if (intelObj->mt &&
       intel_miptree_match_image(intelObj->mt, &intelImage->base,
                                 intelImage->face, intelImage->level)) {

      intel_miptree_reference(&intelImage->mt, intelObj->mt);
      assert(intelImage->mt);
   }

   if (!intelImage->mt)
      DBG("XXX: Image did not fit into tree - storing in local memory!\n");

   /* PBO fastpaths:
    */
   if (dims <= 2 &&
       intelImage->mt &&
       intel_buffer_object(unpack->BufferObj) &&
       check_pbo_format(internalFormat, format,
                        type, intelImage->base.TexFormat)) {

      DBG("trying pbo upload\n");

      /* Attempt to texture directly from PBO data (zero copy upload).
       *
       * Currently disable as it can lead to worse as well as better
       * performance (in particular when intel_region_cow() is
       * required).
       */
      if (intelObj->mt == intelImage->mt &&
          intelObj->mt->first_level == level &&
          intelObj->mt->last_level == level) {

         if (try_pbo_zcopy(intel, intelImage, unpack,
                           internalFormat,
                           width, height, format, type, pixels)) {

            DBG("pbo zcopy upload succeeded\n");
            return;
         }
      }


      /* Otherwise, attempt to use the blitter for PBO image uploads.
       */
      if (try_pbo_upload(intel, intelImage, unpack,
                         internalFormat,
                         width, height, format, type, pixels)) {
         DBG("pbo upload succeeded\n");
         return;
      }

      DBG("pbo upload failed\n");
   }



   /* intelCopyTexImage calls this function with pixels == NULL, with
    * the expectation that the mipmap tree will be set up but nothing
    * more will be done.  This is where those calls return:
    */
   if (compressed) {
      pixels = _mesa_validate_pbo_compressed_teximage(ctx, imageSize, pixels,
						      unpack,
						      "glCompressedTexImage");
   } else {
      pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, 1,
					   format, type,
					   pixels, unpack, "glTexImage");
   }
   if (!pixels)
      return;


   if (intelImage->mt)
      intel_region_idle(intel->intelScreen, intelImage->mt->region);

   LOCK_HARDWARE(intel);

   if (intelImage->mt) {
      texImage->Data = intel_miptree_image_map(intel,
                                               intelImage->mt,
                                               intelImage->face,
                                               intelImage->level,
                                               &dstRowStride,
                                               intelImage->base.ImageOffsets);
   }
   else {
      /* Allocate regular memory and store the image there temporarily.   */
      if (texImage->IsCompressed) {
         sizeInBytes = texImage->CompressedSize;
         dstRowStride =
            _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat, width);
         assert(dims != 3);
      }
      else {
         dstRowStride = postConvWidth * texelBytes;
         sizeInBytes = depth * dstRowStride * postConvHeight;
      }

      texImage->Data = malloc(sizeInBytes);
   }

   DBG("Upload image %dx%dx%d row_len %x "
       "pitch %x\n",
       width, height, depth, width * texelBytes, dstRowStride);

   /* Copy data.  Would like to know when it's ok for us to eg. use
    * the blitter to copy.  Or, use the hardware to do the format
    * conversion and copy:
    */
   if (compressed) {
     memcpy(texImage->Data, pixels, imageSize);
   } else if (!texImage->TexFormat->StoreImage(ctx, dims, 
					       texImage->_BaseFormat, 
					       texImage->TexFormat, 
					       texImage->Data, 0, 0, 0, /* dstX/Y/Zoffset */
					       dstRowStride,
					       texImage->ImageOffsets,
					       width, height, depth,
					       format, type, pixels, unpack)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
   }

   _mesa_unmap_teximage_pbo(ctx, unpack);

   if (intelImage->mt) {
      intel_miptree_image_unmap(intel, intelImage->mt);
      texImage->Data = NULL;
   }

   UNLOCK_HARDWARE(intel);

#if 0
   /* GL_SGIS_generate_mipmap -- this can be accelerated now.
    */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      intel_generate_mipmap(ctx, target,
                            &ctx->Texture.Unit[ctx->Texture.CurrentUnit],
                            texObj);
   }
#endif
}

void
intelTexImage3D(GLcontext * ctx,
                GLenum target, GLint level,
                GLint internalFormat,
                GLint width, GLint height, GLint depth,
                GLint border,
                GLenum format, GLenum type, const void *pixels,
                const struct gl_pixelstore_attrib *unpack,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage)
{
   intelTexImage(ctx, 3, target, level,
                 internalFormat, width, height, depth, border,
                 format, type, pixels, unpack, texObj, texImage, 0, 0);
}


void
intelTexImage2D(GLcontext * ctx,
                GLenum target, GLint level,
                GLint internalFormat,
                GLint width, GLint height, GLint border,
                GLenum format, GLenum type, const void *pixels,
                const struct gl_pixelstore_attrib *unpack,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage)
{
   intelTexImage(ctx, 2, target, level,
                 internalFormat, width, height, 1, border,
                 format, type, pixels, unpack, texObj, texImage, 0, 0);
}

void
intelTexImage1D(GLcontext * ctx,
                GLenum target, GLint level,
                GLint internalFormat,
                GLint width, GLint border,
                GLenum format, GLenum type, const void *pixels,
                const struct gl_pixelstore_attrib *unpack,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage)
{
   intelTexImage(ctx, 1, target, level,
                 internalFormat, width, 1, 1, border,
                 format, type, pixels, unpack, texObj, texImage, 0, 0);
}

void intelCompressedTexImage2D( GLcontext *ctx, GLenum target, GLint level,
				GLint internalFormat,
				GLint width, GLint height, GLint border,
				GLsizei imageSize, const GLvoid *data,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage )
{
   intelTexImage(ctx, 2, target, level,
		 internalFormat, width, height, 1, border,
		 0, 0, data, &ctx->Unpack, texObj, texImage, imageSize, 1);
}

/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
static void
intel_get_tex_image(GLcontext * ctx, GLenum target, GLint level,
		    GLenum format, GLenum type, GLvoid * pixels,
		    struct gl_texture_object *texObj,
		    struct gl_texture_image *texImage, int compressed)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   /* Map */
   if (intelImage->mt) {
      /* Image is stored in hardware format in a buffer managed by the
       * kernel.  Need to explicitly map and unmap it.
       */
      intelImage->base.Data =
         intel_miptree_image_map(intel,
                                 intelImage->mt,
                                 intelImage->face,
                                 intelImage->level,
                                 &intelImage->base.RowStride,
                                 intelImage->base.ImageOffsets);
      intelImage->base.RowStride /= intelImage->mt->cpp;
   }
   else {
      /* Otherwise, the image should actually be stored in
       * intelImage->base.Data.  This is pretty confusing for
       * everybody, I'd much prefer to separate the two functions of
       * texImage->Data - storage for texture images in main memory
       * and access (ie mappings) of images.  In other words, we'd
       * create a new texImage->Map field and leave Data simply for
       * storage.
       */
      assert(intelImage->base.Data);
   }


   if (compressed) {
      _mesa_get_compressed_teximage(ctx, target, level, pixels,
				    texObj, texImage);
   } else {
      _mesa_get_teximage(ctx, target, level, format, type, pixels,
			 texObj, texImage);
   }
     

   /* Unmap */
   if (intelImage->mt) {
      intel_miptree_image_unmap(intel, intelImage->mt);
      intelImage->base.Data = NULL;
   }
}

void
intelGetTexImage(GLcontext * ctx, GLenum target, GLint level,
                 GLenum format, GLenum type, GLvoid * pixels,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
   intel_get_tex_image(ctx, target, level, format, type, pixels,
		       texObj, texImage, 0);


}

void
intelGetCompressedTexImage(GLcontext *ctx, GLenum target, GLint level,
			   GLvoid *pixels,
			   const struct gl_texture_object *texObj,
			   const struct gl_texture_image *texImage)
{
   intel_get_tex_image(ctx, target, level, 0, 0, pixels,
		       texObj, texImage, 1);

}

void
intelSetTexOffset(__DRIcontext *pDRICtx, GLint texname,
		  unsigned long long offset, GLint depth, GLuint pitch)
{
   struct intel_context *intel = (struct intel_context*)
      ((__DRIcontextPrivate*)pDRICtx->private)->driverPrivate;
   struct gl_texture_object *tObj = _mesa_lookup_texture(&intel->ctx, texname);
   struct intel_texture_object *intelObj = intel_texture_object(tObj);

   if (!intelObj)
      return;

   if (intelObj->mt)
      intel_miptree_release(intel, &intelObj->mt);

   intelObj->imageOverride = GL_TRUE;
   intelObj->depthOverride = depth;
   intelObj->pitchOverride = pitch;

   if (offset)
      intelObj->textureOffset = offset;
}
