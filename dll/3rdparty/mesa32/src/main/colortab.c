/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colortab.h"
#include "context.h"
#include "image.h"
#include "macros.h"
#include "state.h"


/**
 * Given an internalFormat token passed to glColorTable,
 * return the corresponding base format.
 * Return -1 if invalid token.
 */
static GLint
base_colortab_format( GLenum format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      default:
         return -1;  /* error */
   }
}



/**
 * Examine table's format and set the component sizes accordingly.
 */
static void
set_component_sizes( struct gl_color_table *table )
{
   /* assuming the ubyte table */
   const GLubyte sz = 8;

   switch (table->_BaseFormat) {
      case GL_ALPHA:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = sz;
         table->IntensitySize = 0;
         table->LuminanceSize = 0;
         break;
      case GL_LUMINANCE:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = 0;
         table->IntensitySize = 0;
         table->LuminanceSize = sz;
         break;
      case GL_LUMINANCE_ALPHA:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = sz;
         table->IntensitySize = 0;
         table->LuminanceSize = sz;
         break;
      case GL_INTENSITY:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = 0;
         table->IntensitySize = sz;
         table->LuminanceSize = 0;
         break;
      case GL_RGB:
         table->RedSize = sz;
         table->GreenSize = sz;
         table->BlueSize = sz;
         table->AlphaSize = 0;
         table->IntensitySize = 0;
         table->LuminanceSize = 0;
         break;
      case GL_RGBA:
         table->RedSize = sz;
         table->GreenSize = sz;
         table->BlueSize = sz;
         table->AlphaSize = sz;
         table->IntensitySize = 0;
         table->LuminanceSize = 0;
         break;
      default:
         _mesa_problem(NULL, "unexpected format in set_component_sizes");
   }
}



/**
 * Update/replace all or part of a color table.  Helper function
 * used by _mesa_ColorTable() and _mesa_ColorSubTable().
 * The table->Table buffer should already be allocated.
 * \param start first entry to update
 * \param count number of entries to update
 * \param format format of user-provided table data
 * \param type datatype of user-provided table data
 * \param data user-provided table data
 * \param [rgba]Scale - RGBA scale factors
 * \param [rgba]Bias - RGBA bias factors
 */
static void
store_colortable_entries(GLcontext *ctx, struct gl_color_table *table,
			 GLsizei start, GLsizei count,
			 GLenum format, GLenum type, const GLvoid *data,
			 GLfloat rScale, GLfloat rBias,
			 GLfloat gScale, GLfloat gBias,
			 GLfloat bScale, GLfloat bBias,
			 GLfloat aScale, GLfloat aBias)
{
   if (ctx->Unpack.BufferObj->Name) {
      /* Get/unpack the color table data from a PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(1, &ctx->Unpack, count, 1, 1,
                                     format, type, data)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glColor[Sub]Table(bad PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              ctx->Unpack.BufferObj);
      if (!buf) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glColor[Sub]Table(PBO mapped)");
         return;
      }
      data = ADD_POINTERS(buf, data);
   }


   {
      /* convert user-provided data to GLfloat values */
      GLfloat tempTab[MAX_COLOR_TABLE_SIZE * 4];
      GLfloat *tableF;
      GLint i;

      _mesa_unpack_color_span_float(ctx,
                                    count,         /* number of pixels */
                                    table->_BaseFormat, /* dest format */
                                    tempTab,       /* dest address */
                                    format, type,  /* src format/type */
                                    data,          /* src data */
                                    &ctx->Unpack,
                                    IMAGE_CLAMP_BIT); /* transfer ops */

      /* the destination */
      tableF = table->TableF;

      /* Apply scale & bias & clamp now */
      switch (table->_BaseFormat) {
         case GL_INTENSITY:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j] = CLAMP(tempTab[i] * rScale + rBias, 0.0F, 1.0F);
            }
            break;
         case GL_LUMINANCE:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j] = CLAMP(tempTab[i] * rScale + rBias, 0.0F, 1.0F);
            }
            break;
         case GL_ALPHA:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j] = CLAMP(tempTab[i] * aScale + aBias, 0.0F, 1.0F);
            }
            break;
         case GL_LUMINANCE_ALPHA:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j*2+0] = CLAMP(tempTab[i*2+0] * rScale + rBias, 0.0F, 1.0F);
               tableF[j*2+1] = CLAMP(tempTab[i*2+1] * aScale + aBias, 0.0F, 1.0F);
            }
            break;
         case GL_RGB:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j*3+0] = CLAMP(tempTab[i*3+0] * rScale + rBias, 0.0F, 1.0F);
               tableF[j*3+1] = CLAMP(tempTab[i*3+1] * gScale + gBias, 0.0F, 1.0F);
               tableF[j*3+2] = CLAMP(tempTab[i*3+2] * bScale + bBias, 0.0F, 1.0F);
            }
            break;
         case GL_RGBA:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j*4+0] = CLAMP(tempTab[i*4+0] * rScale + rBias, 0.0F, 1.0F);
               tableF[j*4+1] = CLAMP(tempTab[i*4+1] * gScale + gBias, 0.0F, 1.0F);
               tableF[j*4+2] = CLAMP(tempTab[i*4+2] * bScale + bBias, 0.0F, 1.0F);
               tableF[j*4+3] = CLAMP(tempTab[i*4+3] * aScale + aBias, 0.0F, 1.0F);
            }
            break;
         default:
            _mesa_problem(ctx, "Bad format in store_colortable_entries");
            return;
         }
   }

   /* update the ubyte table */
   {
      const GLint comps = _mesa_components_in_format(table->_BaseFormat);
      const GLfloat *tableF = table->TableF + start * comps;
      GLubyte *tableUB = table->TableUB + start * comps;
      GLint i;
      for (i = 0; i < count * comps; i++) {
         CLAMPED_FLOAT_TO_UBYTE(tableUB[i], tableF[i]);
      }
   }

   if (ctx->Unpack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }
}



void GLAPIENTRY
_mesa_ColorTable( GLenum target, GLenum internalFormat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *data )
{
   static const GLfloat one[4] = { 1.0, 1.0, 1.0, 1.0 };
   static const GLfloat zero[4] = { 0.0, 0.0, 0.0, 0.0 };
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj = NULL;
   struct gl_color_table *table = NULL;
   GLboolean proxy = GL_FALSE;
   GLint baseFormat;
   const GLfloat *scale = one, *bias = zero;
   GLint comps;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex */

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->Current3D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
            return;
         }
         texObj = texUnit->CurrentCubeMap;
         table = &texObj->Palette;
         break;
      case GL_PROXY_TEXTURE_1D:
         texObj = ctx->Texture.Proxy1D;
         table = &texObj->Palette;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_2D:
         texObj = ctx->Texture.Proxy2D;
         table = &texObj->Palette;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_3D:
         texObj = ctx->Texture.Proxy3D;
         table = &texObj->Palette;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
            return;
         }
         texObj = ctx->Texture.ProxyCubeMap;
         table = &texObj->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_PRECONVOLUTION];
         scale = ctx->Pixel.ColorTableScale[COLORTABLE_PRECONVOLUTION];
         bias = ctx->Pixel.ColorTableBias[COLORTABLE_PRECONVOLUTION];
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_PRECONVOLUTION];
         proxy = GL_TRUE;
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         scale = ctx->Pixel.TextureColorTableScale;
         bias = ctx->Pixel.TextureColorTableBias;
         break;
      case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
            return;
         }
         table = &(texUnit->ProxyColorTable);
         proxy = GL_TRUE;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCONVOLUTION];
         scale = ctx->Pixel.ColorTableScale[COLORTABLE_POSTCONVOLUTION];
         bias = ctx->Pixel.ColorTableBias[COLORTABLE_POSTCONVOLUTION];
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_POSTCONVOLUTION];
         proxy = GL_TRUE;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCOLORMATRIX];
         scale = ctx->Pixel.ColorTableScale[COLORTABLE_POSTCOLORMATRIX];
         bias = ctx->Pixel.ColorTableBias[COLORTABLE_POSTCOLORMATRIX];
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_POSTCOLORMATRIX];
         proxy = GL_TRUE;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
         return;
   }

   assert(table);

   if (!_mesa_is_legal_format_and_type(ctx, format, type) ||
       format == GL_INTENSITY) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glColorTable(format or type)");
      return;
   }

   baseFormat = base_colortab_format(internalFormat);
   if (baseFormat < 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(internalFormat)");
      return;
   }

   if (width < 0 || (width != 0 && _mesa_bitcount(width) != 1)) {
      /* error */
      if (proxy) {
         table->Size = 0;
         table->InternalFormat = (GLenum) 0;
         table->_BaseFormat = (GLenum) 0;
      }
      else {
         _mesa_error(ctx, GL_INVALID_VALUE, "glColorTable(width=%d)", width);
      }
      return;
   }

   if (width > (GLsizei) ctx->Const.MaxColorTableSize) {
      if (proxy) {
         table->Size = 0;
         table->InternalFormat = (GLenum) 0;
         table->_BaseFormat = (GLenum) 0;
      }
      else {
         _mesa_error(ctx, GL_TABLE_TOO_LARGE, "glColorTable(width)");
      }
      return;
   }

   table->Size = width;
   table->InternalFormat = internalFormat;
   table->_BaseFormat = (GLenum) baseFormat;

   comps = _mesa_components_in_format(table->_BaseFormat);
   assert(comps > 0);  /* error should have been caught sooner */

   if (!proxy) {
      _mesa_free_colortable_data(table);

      if (width > 0) {
         table->TableF = (GLfloat *) _mesa_malloc(comps * width * sizeof(GLfloat));
         table->TableUB = (GLubyte *) _mesa_malloc(comps * width * sizeof(GLubyte));

	 if (!table->TableF || !table->TableUB) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glColorTable");
	    return;
	 }

	 store_colortable_entries(ctx, table,
				  0, width,  /* start, count */
				  format, type, data,
				  scale[0], bias[0],
				  scale[1], bias[1],
				  scale[2], bias[2],
				  scale[3], bias[3]);
      }
   } /* proxy */

   /* do this after the table's Type and Format are set */
   set_component_sizes(table);

   if (texObj || target == GL_SHARED_TEXTURE_PALETTE_EXT) {
      /* texture object palette, texObj==NULL means the shared palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
      }
   }

   ctx->NewState |= _NEW_PIXEL;
}



void GLAPIENTRY
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *data )
{
   static const GLfloat one[4] = { 1.0, 1.0, 1.0, 1.0 };
   static const GLfloat zero[4] = { 0.0, 0.0, 0.0, 0.0 };
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj = NULL;
   struct gl_color_table *table = NULL;
   const GLfloat *scale = one, *bias = zero;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->Current3D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
            return;
         }
         texObj = texUnit->CurrentCubeMap;
         table = &texObj->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_PRECONVOLUTION];
         scale = ctx->Pixel.ColorTableScale[COLORTABLE_PRECONVOLUTION];
         bias = ctx->Pixel.ColorTableBias[COLORTABLE_PRECONVOLUTION];
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         scale = ctx->Pixel.TextureColorTableScale;
         bias = ctx->Pixel.TextureColorTableBias;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCONVOLUTION];
         scale = ctx->Pixel.ColorTableScale[COLORTABLE_POSTCONVOLUTION];
         bias = ctx->Pixel.ColorTableBias[COLORTABLE_POSTCONVOLUTION];
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCOLORMATRIX];
         scale = ctx->Pixel.ColorTableScale[COLORTABLE_POSTCOLORMATRIX];
         bias = ctx->Pixel.ColorTableBias[COLORTABLE_POSTCOLORMATRIX];
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
         return;
   }

   assert(table);

   if (!_mesa_is_legal_format_and_type(ctx, format, type) ||
       format == GL_INTENSITY) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glColorSubTable(format or type)");
      return;
   }

   if (count < 1) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glColorSubTable(count)");
      return;
   }

   /* error should have been caught sooner */
   assert(_mesa_components_in_format(table->_BaseFormat) > 0);

   if (start + count > (GLint) table->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glColorSubTable(count)");
      return;
   }

   if (!table->TableF || !table->TableUB) {
      /* a GL_OUT_OF_MEMORY error would have been recorded previously */
      return;
   }

   store_colortable_entries(ctx, table, start, count,
			    format, type, data,
                            scale[0], bias[0],
                            scale[1], bias[1],
                            scale[2], bias[2],
                            scale[3], bias[3]);

   if (texObj || target == GL_SHARED_TEXTURE_PALETTE_EXT) {
      /* per-texture object palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
      }
   }

   ctx->NewState |= _NEW_PIXEL;
}



void GLAPIENTRY
_mesa_CopyColorTable(GLenum target, GLenum internalformat,
                     GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   /* Select buffer to read from */
   ctx->Driver.CopyColorTable( ctx, target, internalformat, x, y, width );
}



void GLAPIENTRY
_mesa_CopyColorSubTable(GLenum target, GLsizei start,
                        GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   ctx->Driver.CopyColorSubTable( ctx, target, start, x, y, width );
}



void GLAPIENTRY
_mesa_GetColorTable( GLenum target, GLenum format,
                     GLenum type, GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *table = NULL;
   GLfloat rgba[MAX_COLOR_TABLE_SIZE][4];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   switch (target) {
      case GL_TEXTURE_1D:
         table = &texUnit->Current1D->Palette;
         break;
      case GL_TEXTURE_2D:
         table = &texUnit->Current2D->Palette;
         break;
      case GL_TEXTURE_3D:
         table = &texUnit->Current3D->Palette;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
            return;
         }
         table = &texUnit->CurrentCubeMap->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_PRECONVOLUTION];
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCONVOLUTION];
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCOLORMATRIX];
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
         return;
   }

   ASSERT(table);

   if (table->Size <= 0) {
      return;
   }

   switch (table->_BaseFormat) {
   case GL_ALPHA:
      {
         GLuint i;
         for (i = 0; i < table->Size; i++) {
            rgba[i][RCOMP] = 0;
            rgba[i][GCOMP] = 0;
            rgba[i][BCOMP] = 0;
            rgba[i][ACOMP] = table->TableF[i];
         }
      }
      break;
   case GL_LUMINANCE:
      {
         GLuint i;
         for (i = 0; i < table->Size; i++) {
            rgba[i][RCOMP] =
            rgba[i][GCOMP] =
            rgba[i][BCOMP] = table->TableF[i];
            rgba[i][ACOMP] = 1.0F;
         }
      }
      break;
   case GL_LUMINANCE_ALPHA:
      {
         GLuint i;
         for (i = 0; i < table->Size; i++) {
            rgba[i][RCOMP] =
            rgba[i][GCOMP] =
            rgba[i][BCOMP] = table->TableF[i*2+0];
            rgba[i][ACOMP] = table->TableF[i*2+1];
         }
      }
      break;
   case GL_INTENSITY:
      {
         GLuint i;
         for (i = 0; i < table->Size; i++) {
            rgba[i][RCOMP] =
            rgba[i][GCOMP] =
            rgba[i][BCOMP] =
            rgba[i][ACOMP] = table->TableF[i];
         }
      }
      break;
   case GL_RGB:
      {
         GLuint i;
         for (i = 0; i < table->Size; i++) {
            rgba[i][RCOMP] = table->TableF[i*3+0];
            rgba[i][GCOMP] = table->TableF[i*3+1];
            rgba[i][BCOMP] = table->TableF[i*3+2];
            rgba[i][ACOMP] = 1.0F;
         }
      }
      break;
   case GL_RGBA:
      _mesa_memcpy(rgba, table->TableF, 4 * table->Size * sizeof(GLfloat));
      break;
   default:
      _mesa_problem(ctx, "bad table format in glGetColorTable");
      return;
   }

   if (ctx->Pack.BufferObj->Name) {
      /* pack color table into PBO */
      GLubyte *buf;
      if (!_mesa_validate_pbo_access(1, &ctx->Pack, table->Size, 1, 1,
                                     format, type, data)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetColorTable(invalid PBO access)");
         return;
      }
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetColorTable(PBO is mapped)");
         return;
      }
      data = ADD_POINTERS(buf, data);
   }

   _mesa_pack_rgba_span_float(ctx, table->Size, rgba,
                              format, type, data, &ctx->Pack, 0x0);

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}



void GLAPIENTRY
_mesa_ColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (target) {
      case GL_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(ctx->Pixel.ColorTableScale[COLORTABLE_PRECONVOLUTION], params);
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(ctx->Pixel.ColorTableBias[COLORTABLE_PRECONVOLUTION], params);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameter(target)");
            return;
         }
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(ctx->Pixel.TextureColorTableScale, params);
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(ctx->Pixel.TextureColorTableBias, params);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(ctx->Pixel.ColorTableScale[COLORTABLE_POSTCONVOLUTION], params);
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(ctx->Pixel.ColorTableBias[COLORTABLE_POSTCONVOLUTION], params);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(ctx->Pixel.ColorTableScale[COLORTABLE_POSTCOLORMATRIX], params);
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(ctx->Pixel.ColorTableBias[COLORTABLE_POSTCOLORMATRIX], params);
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameter(target)");
         return;
   }

   ctx->NewState |= _NEW_PIXEL;
}



void GLAPIENTRY
_mesa_ColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GLfloat fparams[4];
   if (pname == GL_COLOR_TABLE_SGI ||
       pname == GL_TEXTURE_COLOR_TABLE_SGI ||
       pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
       pname == GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI) {
      /* four values */
      fparams[0] = (GLfloat) params[0];
      fparams[1] = (GLfloat) params[1];
      fparams[2] = (GLfloat) params[2];
      fparams[3] = (GLfloat) params[3];
   }
   else {
      /* one values */
      fparams[0] = (GLfloat) params[0];
   }
   _mesa_ColorTableParameterfv(target, pname, fparams);
}



void GLAPIENTRY
_mesa_GetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *table = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
      case GL_TEXTURE_1D:
         table = &texUnit->Current1D->Palette;
         break;
      case GL_TEXTURE_2D:
         table = &texUnit->Current2D->Palette;
         break;
      case GL_TEXTURE_3D:
         table = &texUnit->Current3D->Palette;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetColorTableParameterfv(target)");
            return;
         }
         table = &texUnit->CurrentCubeMap->Palette;
         break;
      case GL_PROXY_TEXTURE_1D:
         table = &ctx->Texture.Proxy1D->Palette;
         break;
      case GL_PROXY_TEXTURE_2D:
         table = &ctx->Texture.Proxy2D->Palette;
         break;
      case GL_PROXY_TEXTURE_3D:
         table = &ctx->Texture.Proxy3D->Palette;
         break;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetColorTableParameterfv(target)");
            return;
         }
         table = &ctx->Texture.ProxyCubeMap->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_PRECONVOLUTION];
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(params, ctx->Pixel.ColorTableScale[COLORTABLE_PRECONVOLUTION]);
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(params, ctx->Pixel.ColorTableBias[COLORTABLE_PRECONVOLUTION]);
            return;
         }
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_PRECONVOLUTION];
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(params, ctx->Pixel.TextureColorTableScale);
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(params, ctx->Pixel.TextureColorTableBias);
            return;
         }
         break;
      case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
            return;
         }
         table = &(texUnit->ProxyColorTable);
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCONVOLUTION];
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(params, ctx->Pixel.ColorTableScale[COLORTABLE_POSTCONVOLUTION]);
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(params, ctx->Pixel.ColorTableBias[COLORTABLE_POSTCONVOLUTION]);
            return;
         }
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_POSTCONVOLUTION];
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCOLORMATRIX];
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            COPY_4V(params, ctx->Pixel.ColorTableScale[COLORTABLE_POSTCOLORMATRIX]);
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            COPY_4V(params, ctx->Pixel.ColorTableBias[COLORTABLE_POSTCOLORMATRIX]);
            return;
         }
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_POSTCOLORMATRIX];
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameterfv(target)");
         return;
   }

   assert(table);

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
         *params = (GLfloat) table->InternalFormat;
         break;
      case GL_COLOR_TABLE_WIDTH:
         *params = (GLfloat) table->Size;
         break;
      case GL_COLOR_TABLE_RED_SIZE:
         *params = (GLfloat) table->RedSize;
         break;
      case GL_COLOR_TABLE_GREEN_SIZE:
         *params = (GLfloat) table->GreenSize;
         break;
      case GL_COLOR_TABLE_BLUE_SIZE:
         *params = (GLfloat) table->BlueSize;
         break;
      case GL_COLOR_TABLE_ALPHA_SIZE:
         *params = (GLfloat) table->AlphaSize;
         break;
      case GL_COLOR_TABLE_LUMINANCE_SIZE:
         *params = (GLfloat) table->LuminanceSize;
         break;
      case GL_COLOR_TABLE_INTENSITY_SIZE:
         *params = (GLfloat) table->IntensitySize;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameterfv(pname)" );
         return;
   }
}



void GLAPIENTRY
_mesa_GetColorTableParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *table = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
      case GL_TEXTURE_1D:
         table = &texUnit->Current1D->Palette;
         break;
      case GL_TEXTURE_2D:
         table = &texUnit->Current2D->Palette;
         break;
      case GL_TEXTURE_3D:
         table = &texUnit->Current3D->Palette;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetColorTableParameteriv(target)");
            return;
         }
         table = &texUnit->CurrentCubeMap->Palette;
         break;
      case GL_PROXY_TEXTURE_1D:
         table = &ctx->Texture.Proxy1D->Palette;
         break;
      case GL_PROXY_TEXTURE_2D:
         table = &ctx->Texture.Proxy2D->Palette;
         break;
      case GL_PROXY_TEXTURE_3D:
         table = &ctx->Texture.Proxy3D->Palette;
         break;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glGetColorTableParameteriv(target)");
            return;
         }
         table = &ctx->Texture.ProxyCubeMap->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_PRECONVOLUTION];
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            GLfloat *scale = ctx->Pixel.ColorTableScale[COLORTABLE_PRECONVOLUTION];
            params[0] = (GLint) scale[0];
            params[1] = (GLint) scale[1];
            params[2] = (GLint) scale[2];
            params[3] = (GLint) scale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            GLfloat *bias = ctx->Pixel.ColorTableBias[COLORTABLE_PRECONVOLUTION];
            params[0] = (GLint) bias[0];
            params[1] = (GLint) bias[1];
            params[2] = (GLint) bias[2];
            params[3] = (GLint) bias[3];
            return;
         }
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_PRECONVOLUTION];
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = (GLint) ctx->Pixel.TextureColorTableScale[0];
            params[1] = (GLint) ctx->Pixel.TextureColorTableScale[1];
            params[2] = (GLint) ctx->Pixel.TextureColorTableScale[2];
            params[3] = (GLint) ctx->Pixel.TextureColorTableScale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = (GLint) ctx->Pixel.TextureColorTableBias[0];
            params[1] = (GLint) ctx->Pixel.TextureColorTableBias[1];
            params[2] = (GLint) ctx->Pixel.TextureColorTableBias[2];
            params[3] = (GLint) ctx->Pixel.TextureColorTableBias[3];
            return;
         }
         break;
      case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
            return;
         }
         table = &(texUnit->ProxyColorTable);
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCONVOLUTION];
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            GLfloat *scale = ctx->Pixel.ColorTableScale[COLORTABLE_POSTCONVOLUTION];
            params[0] = (GLint) scale[0];
            params[1] = (GLint) scale[1];
            params[2] = (GLint) scale[2];
            params[3] = (GLint) scale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            GLfloat *bias = ctx->Pixel.ColorTableBias[COLORTABLE_POSTCONVOLUTION];
            params[0] = (GLint) bias[0];
            params[1] = (GLint) bias[1];
            params[2] = (GLint) bias[2];
            params[3] = (GLint) bias[3];
            return;
         }
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_POSTCONVOLUTION];
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ColorTable[COLORTABLE_POSTCOLORMATRIX];
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            GLfloat *scale = ctx->Pixel.ColorTableScale[COLORTABLE_POSTCOLORMATRIX];
            params[0] = (GLint) scale[0];
            params[0] = (GLint) scale[1];
            params[0] = (GLint) scale[2];
            params[0] = (GLint) scale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            GLfloat *bias = ctx->Pixel.ColorTableScale[COLORTABLE_POSTCOLORMATRIX];
            params[0] = (GLint) bias[0];
            params[1] = (GLint) bias[1];
            params[2] = (GLint) bias[2];
            params[3] = (GLint) bias[3];
            return;
         }
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyColorTable[COLORTABLE_POSTCOLORMATRIX];
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameteriv(target)");
         return;
   }

   assert(table);

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
         *params = table->InternalFormat;
         break;
      case GL_COLOR_TABLE_WIDTH:
         *params = table->Size;
         break;
      case GL_COLOR_TABLE_RED_SIZE:
         *params = table->RedSize;
         break;
      case GL_COLOR_TABLE_GREEN_SIZE:
         *params = table->GreenSize;
         break;
      case GL_COLOR_TABLE_BLUE_SIZE:
         *params = table->BlueSize;
         break;
      case GL_COLOR_TABLE_ALPHA_SIZE:
         *params = table->AlphaSize;
         break;
      case GL_COLOR_TABLE_LUMINANCE_SIZE:
         *params = table->LuminanceSize;
         break;
      case GL_COLOR_TABLE_INTENSITY_SIZE:
         *params = table->IntensitySize;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameteriv(pname)" );
         return;
   }
}

/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/


void
_mesa_init_colortable( struct gl_color_table *p )
{
   p->TableF = NULL;
   p->TableUB = NULL;
   p->Size = 0;
   p->InternalFormat = GL_RGBA;
}



void
_mesa_free_colortable_data( struct gl_color_table *p )
{
   if (p->TableF) {
      _mesa_free(p->TableF);
      p->TableF = NULL;
   }
   if (p->TableUB) {
      _mesa_free(p->TableUB);
      p->TableUB = NULL;
   }
}


/*
 * Initialize all colortables for a context.
 */
void
_mesa_init_colortables( GLcontext * ctx )
{
   GLuint i;
   for (i = 0; i < COLORTABLE_MAX; i++) {
      _mesa_init_colortable(&ctx->ColorTable[i]);
      _mesa_init_colortable(&ctx->ProxyColorTable[i]);
   }
}


/*
 * Free all colortable data for a context
 */
void
_mesa_free_colortables_data( GLcontext *ctx )
{
   GLuint i;
   for (i = 0; i < COLORTABLE_MAX; i++) {
      _mesa_free_colortable_data(&ctx->ColorTable[i]);
      _mesa_free_colortable_data(&ctx->ProxyColorTable[i]);
   }
}
