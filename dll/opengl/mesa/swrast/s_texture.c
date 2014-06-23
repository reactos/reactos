/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2011 VMware, Inc.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Functions for mapping/unmapping texture images.
 */

#include <precomp.h>

/**
 * Allocate a new swrast_texture_image (a subclass of gl_texture_image).
 * Called via ctx->Driver.NewTextureImage().
 */
struct gl_texture_image *
_swrast_new_texture_image( struct gl_context *ctx )
{
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(swrast_texture_image);
}


/**
 * Free a swrast_texture_image (a subclass of gl_texture_image).
 * Called via ctx->Driver.DeleteTextureImage().
 */
void
_swrast_delete_texture_image(struct gl_context *ctx,
                             struct gl_texture_image *texImage)
{
   /* Nothing special for the subclass yet */
   _mesa_delete_texture_image(ctx, texImage);
}


/**
 * Called via ctx->Driver.AllocTextureImageBuffer()
 */
GLboolean
_swrast_alloc_texture_image_buffer(struct gl_context *ctx,
                                   struct gl_texture_image *texImage,
                                   gl_format format, GLsizei width,
                                   GLsizei height, GLsizei depth)
{
   struct swrast_texture_image *swImg = swrast_texture_image(texImage);
   GLuint bytes = _mesa_format_image_size(format, width, height, depth);
   GLuint i;

   /* This _should_ be true (revisit if these ever fail) */
   assert(texImage->Width == width);
   assert(texImage->Height == height);
   assert(texImage->Depth == depth);

   assert(!swImg->Buffer);
   swImg->Buffer = _mesa_align_malloc(bytes, 512);
   if (!swImg->Buffer)
      return GL_FALSE;

   /* RowStride and ImageOffsets[] describe how to address texels in 'Data' */
   swImg->RowStride = width;

   /* Allocate the ImageOffsets array and initialize to typical values.
    * We allocate the array for 1D/2D textures too in order to avoid special-
    * case code in the texstore routines.
    */
   swImg->ImageOffsets = (GLuint *) malloc(depth * sizeof(GLuint));
   if (!swImg->ImageOffsets)
      return GL_FALSE;

   for (i = 0; i < depth; i++) {
      swImg->ImageOffsets[i] = i * width * height;
   }

   _swrast_init_texture_image(texImage, width, height, depth);

   return GL_TRUE;
}


/**
 * Code that overrides ctx->Driver.AllocTextureImageBuffer may use this to
 * initialize the fields of swrast_texture_image without allocating the image
 * buffer or initializing ImageOffsets or RowStride.
 *
 * Returns GL_TRUE on success, GL_FALSE on memory allocation failure.
 */
void
_swrast_init_texture_image(struct gl_texture_image *texImage, GLsizei width,
                           GLsizei height, GLsizei depth)
{
   struct swrast_texture_image *swImg = swrast_texture_image(texImage);

   if ((width == 1 || _mesa_is_pow_two(texImage->Width2)) &&
       (height == 1 || _mesa_is_pow_two(texImage->Height2)) &&
       (depth == 1 || _mesa_is_pow_two(texImage->Depth2)))
      swImg->_IsPowerOfTwo = GL_TRUE;
   else
      swImg->_IsPowerOfTwo = GL_FALSE;

   /* Compute Width/Height/DepthScale for mipmap lod computation */
   swImg->WidthScale = (GLfloat) texImage->Width;
   swImg->HeightScale = (GLfloat) texImage->Height;
   swImg->DepthScale = (GLfloat) texImage->Depth;
}


/**
 * Called via ctx->Driver.FreeTextureImageBuffer()
 */
void
_swrast_free_texture_image_buffer(struct gl_context *ctx,
                                  struct gl_texture_image *texImage)
{
   struct swrast_texture_image *swImage = swrast_texture_image(texImage);
   if (swImage->Buffer) {
      _mesa_align_free(swImage->Buffer);
      swImage->Buffer = NULL;
   }

   if (swImage->ImageOffsets) {
      free(swImage->ImageOffsets);
      swImage->ImageOffsets = NULL;
   }
}


/**
 * Error checking for debugging only.
 */
static void
_mesa_check_map_teximage(struct gl_texture_image *texImage,
                         GLuint slice, GLuint x, GLuint y, GLuint w, GLuint h)
{

   if (texImage->TexObject->Target == GL_TEXTURE_1D)
      assert(y == 0 && h == 1);

   assert(x < texImage->Width || texImage->Width == 0);
   assert(y < texImage->Height || texImage->Height == 0);
   assert(x + w <= texImage->Width);
   assert(y + h <= texImage->Height);
}

/**
 * Map a 2D slice of a texture image into user space.
 * (x,y,w,h) defines a region of interest (ROI).  Reading/writing texels
 * outside of the ROI is undefined.
 *
 * \param texImage  the texture image
 * \param slice  the 3D image slice or array texture slice
 * \param x, y, w, h  region of interest
 * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT
 * \param mapOut  returns start of mapping of region of interest
 * \param rowStrideOut  returns row stride (in bytes)
 */
void
_swrast_map_teximage(struct gl_context *ctx,
                     struct gl_texture_image *texImage,
                     GLuint slice,
                     GLuint x, GLuint y, GLuint w, GLuint h,
                     GLbitfield mode,
                     GLubyte **mapOut,
                     GLint *rowStrideOut)
{
   struct swrast_texture_image *swImage = swrast_texture_image(texImage);
   GLubyte *map;
   GLint stride, texelSize;
   GLuint bw, bh;

   _mesa_check_map_teximage(texImage, slice, x, y, w, h);

   texelSize = _mesa_get_format_bytes(texImage->TexFormat);
   stride = _mesa_format_row_stride(texImage->TexFormat, texImage->Width);
   _mesa_get_format_block_size(texImage->TexFormat, &bw, &bh);

   assert(x % bw == 0);
   assert(y % bh == 0);

   if (!swImage->Buffer) {
      /* probably ran out of memory when allocating tex mem */
      *mapOut = NULL;
      return;
   }
      
   map = swImage->Buffer;

   /* apply x/y offset to map address */
   map += stride * (y / bh) + texelSize * (x / bw);

   *mapOut = map;
   *rowStrideOut = stride;
}

void
_swrast_unmap_teximage(struct gl_context *ctx,
                       struct gl_texture_image *texImage,
                       GLuint slice)
{
   /* nop */
}


void
_swrast_map_texture(struct gl_context *ctx, struct gl_texture_object *texObj)
{
   const GLuint faces = texObj->Target == GL_TEXTURE_CUBE_MAP ? 6 : 1;
   GLuint face, level;

   for (face = 0; face < faces; face++) {
      for (level = texObj->BaseLevel; level < MAX_TEXTURE_LEVELS; level++) {
         struct gl_texture_image *texImage = texObj->Image[face][level];
         if (texImage) {
            struct swrast_texture_image *swImage =
               swrast_texture_image(texImage);

            /* XXX we'll eventually call _swrast_map_teximage() here */
            swImage->Map = swImage->Buffer;
         }
      }
   }
}


void
_swrast_unmap_texture(struct gl_context *ctx, struct gl_texture_object *texObj)
{
   const GLuint faces = texObj->Target == GL_TEXTURE_CUBE_MAP ? 6 : 1;
   GLuint face, level;

   for (face = 0; face < faces; face++) {
      for (level = texObj->BaseLevel; level < MAX_TEXTURE_LEVELS; level++) {
         struct gl_texture_image *texImage = texObj->Image[face][level];
         if (texImage) {
            struct swrast_texture_image *swImage
               = swrast_texture_image(texImage);

            /* XXX we'll eventually call _swrast_unmap_teximage() here */
            swImage->Map = NULL;
         }
      }
   }
}


/**
 * Map all textures for reading prior to software rendering.
 */
void
_swrast_map_textures(struct gl_context *ctx)
{
   if (ctx->Texture._Enabled) {
      struct gl_texture_object *texObj = ctx->Texture.Unit._Current;
      
      _swrast_map_texture(ctx, texObj);
   }
}


/**
 * Unmap all textures for reading prior to software rendering.
 */
void
_swrast_unmap_textures(struct gl_context *ctx)
{
   if(ctx->Texture._Enabled) {
      struct gl_texture_object *texObj = ctx->Texture.Unit._Current;
      
      _swrast_unmap_texture(ctx, texObj);
   }
}


/**
 * Called via ctx->Driver.AllocTextureStorage()
 * Just have to allocate memory for the texture images.
 */
GLboolean
_swrast_AllocTextureStorage(struct gl_context *ctx,
                            struct gl_texture_object *texObj,
                            GLsizei levels, GLsizei width,
                            GLsizei height, GLsizei depth)
{
   const GLint numFaces = (texObj->Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLint face, level;

   for (face = 0; face < numFaces; face++) {
      for (level = 0; level < levels; level++) {
         struct gl_texture_image *texImage = texObj->Image[face][level];
         if (!_swrast_alloc_texture_image_buffer(ctx, texImage,
                                                 texImage->TexFormat,
                                                 texImage->Width,
                                                 texImage->Height,
                                                 texImage->Depth)) {
            return GL_FALSE;
         }
      }
   }

   return GL_TRUE;
}

