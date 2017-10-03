/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 **************************************************************************/


/**
 * Code to convert compressed/paletted texture images to ordinary images.
 * See the GL_OES_compressed_paletted_texture spec at
 * http://khronos.org/registry/gles/extensions/OES/OES_compressed_paletted_texture.txt
 *
 * XXX this makes it impossible to add hardware support...
 */


#include <precomp.h>

#include "texpal.h"

#if FEATURE_ES


static const struct cpal_format_info {
   GLenum cpal_format;
   GLenum format;
   GLenum type;
   GLuint palette_size;
   GLuint size;
} formats[] = {
   { GL_PALETTE4_RGB8_OES,     GL_RGB,  GL_UNSIGNED_BYTE,           16, 3 },
   { GL_PALETTE4_RGBA8_OES,    GL_RGBA, GL_UNSIGNED_BYTE,           16, 4 },
   { GL_PALETTE4_R5_G6_B5_OES, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,    16, 2 },
   { GL_PALETTE4_RGBA4_OES,    GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4,  16, 2 },
   { GL_PALETTE4_RGB5_A1_OES,  GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,  16, 2 },
   { GL_PALETTE8_RGB8_OES,     GL_RGB,  GL_UNSIGNED_BYTE,          256, 3 },
   { GL_PALETTE8_RGBA8_OES,    GL_RGBA, GL_UNSIGNED_BYTE,          256, 4 },
   { GL_PALETTE8_R5_G6_B5_OES, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,   256, 2 },
   { GL_PALETTE8_RGBA4_OES,    GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 256, 2 },
   { GL_PALETTE8_RGB5_A1_OES,  GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 256, 2 }
};


/**
 * Get a color/entry from the palette.
 */
static GLuint
get_palette_entry(const struct cpal_format_info *info, const GLubyte *palette,
                  GLuint index, GLubyte *pixel)
{
   memcpy(pixel, palette + info->size * index, info->size);
   return info->size;
}


/**
 * Convert paletted texture to color texture.
 */
static void
paletted_to_color(const struct cpal_format_info *info, const GLubyte *palette,
                  const void *indices, GLuint num_pixels, GLubyte *image)
{
   GLubyte *pix = image;
   GLuint remain, i;

   if (info->palette_size == 16) {
      /* 4 bits per index */
      const GLubyte *ind = (const GLubyte *) indices;

      /* two pixels per iteration */
      remain = num_pixels % 2;
      for (i = 0; i < num_pixels / 2; i++) {
         pix += get_palette_entry(info, palette, (ind[i] >> 4) & 0xf, pix);
         pix += get_palette_entry(info, palette, ind[i] & 0xf, pix);
      }
      if (remain) {
         get_palette_entry(info, palette, (ind[i] >> 4) & 0xf, pix);
      }
   }
   else {
      /* 8 bits per index */
      const GLubyte *ind = (const GLubyte *) indices;
      for (i = 0; i < num_pixels; i++)
         pix += get_palette_entry(info, palette, ind[i], pix);
   }
}

unsigned
_mesa_cpal_compressed_size(int level, GLenum internalFormat,
			   unsigned width, unsigned height)
{
   const struct cpal_format_info *info;
   const int num_levels = -level + 1;
   int lvl;
   unsigned w, h, expect_size;

   if (internalFormat < GL_PALETTE4_RGB8_OES
       || internalFormat > GL_PALETTE8_RGB5_A1_OES) {
      return 0;
   }

   info = &formats[internalFormat - GL_PALETTE4_RGB8_OES];
   ASSERT(info->cpal_format == internalFormat);

   expect_size = info->palette_size * info->size;
   for (lvl = 0; lvl < num_levels; lvl++) {
      w = width >> lvl;
      if (!w)
         w = 1;
      h = height >> lvl;
      if (!h)
         h = 1;

      if (info->palette_size == 16)
         expect_size += (w * h  + 1) / 2;
      else
         expect_size += w * h;
   }

   return expect_size;
}

void
_mesa_cpal_compressed_format_type(GLenum internalFormat, GLenum *format,
				  GLenum *type)
{
   const struct cpal_format_info *info;

   if (internalFormat < GL_PALETTE4_RGB8_OES
       || internalFormat > GL_PALETTE8_RGB5_A1_OES) {
      return;
   }

   info = &formats[internalFormat - GL_PALETTE4_RGB8_OES];
   *format = info->format;
   *type = info->type;
}

/**
 * Convert a call to glCompressedTexImage2D() where internalFormat is a
 *  compressed palette format into a regular GLubyte/RGBA glTexImage2D() call.
 */
void
_mesa_cpal_compressed_teximage2d(GLenum target, GLint level,
				 GLenum internalFormat,
				 GLsizei width, GLsizei height,
				 GLsizei imageSize, const void *palette)
{
   const struct cpal_format_info *info;
   GLint lvl, num_levels;
   const GLubyte *indices;
   GLint saved_align, align;
   GET_CURRENT_CONTEXT(ctx);

   /* By this point, the internalFormat should have been validated.
    */
   assert(internalFormat >= GL_PALETTE4_RGB8_OES
	  && internalFormat <= GL_PALETTE8_RGB5_A1_OES);

   info = &formats[internalFormat - GL_PALETTE4_RGB8_OES];

   num_levels = -level + 1;

   /* first image follows the palette */
   indices = (const GLubyte *) palette + info->palette_size * info->size;

   saved_align = ctx->Unpack.Alignment;
   align = saved_align;

   for (lvl = 0; lvl < num_levels; lvl++) {
      GLsizei w, h;
      GLuint num_texels;
      GLubyte *image = NULL;

      w = width >> lvl;
      if (!w)
         w = 1;
      h = height >> lvl;
      if (!h)
         h = 1;
      num_texels = w * h;
      if (w * info->size % align) {
         _mesa_PixelStorei(GL_UNPACK_ALIGNMENT, 1);
         align = 1;
      }

      /* allocate and fill dest image buffer */
      if (palette) {
         image = (GLubyte *) malloc(num_texels * info->size);
         paletted_to_color(info, palette, indices, num_texels, image);
      }

      _mesa_TexImage2D(target, lvl, info->format, w, h, 0,
                       info->format, info->type, image);
      if (image)
         free(image);

      /* advance index pointer to point to next src mipmap */
      if (info->palette_size == 16)
         indices += (num_texels + 1) / 2;
      else
         indices += num_texels;
   }

   if (saved_align != align)
      _mesa_PixelStorei(GL_UNPACK_ALIGNMENT, saved_align);
}

#endif
