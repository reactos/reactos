/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


/*
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



/*
 * Examine table's format and set the component sizes accordingly.
 */
static void
set_component_sizes( struct gl_color_table *table )
{
   GLubyte sz;

   switch (table->Type) {
   case GL_UNSIGNED_BYTE:
      sz = 8 * sizeof(GLubyte);
      break;
   case GL_UNSIGNED_SHORT:
      sz = 8 * sizeof(GLushort);
      break;
   case GL_FLOAT:
      /* Don't actually return 32 here since that causes the conformance
       * tests to blow up.  Conform thinks the component is an integer,
       * not a float.
       */
      sz = 8;  /** 8 * sizeof(GLfloat); **/
      break;
   default:
      _mesa_problem(NULL, "bad color table type in set_component_sizes 0x%x", table->Type);
      return;
   }

   switch (table->Format) {
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


   if (table->Type == GL_FLOAT) {
      /* convert user-provided data to GLfloat values */
      GLfloat tempTab[MAX_COLOR_TABLE_SIZE * 4];
      GLfloat *tableF;
      GLint i;

      _mesa_unpack_color_span_float(ctx,
                                    count,         /* number of pixels */
                                    table->Format, /* dest format */
                                    tempTab,       /* dest address */
                                    format, type,  /* src format/type */
                                    data,          /* src data */
                                    &ctx->Unpack,
                                    IMAGE_CLAMP_BIT); /* transfer ops */

      /* the destination */
      tableF = (GLfloat *) table->Table;

      /* Apply scale & bias & clamp now */
      switch (table->Format) {
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
   else {
      /* non-float (GLchan) */
      const GLint comps = _mesa_components_in_format(table->Format);
      GLchan *dest = (GLchan *) table->Table + start * comps;
      _mesa_unpack_color_span_chan(ctx, count,         /* number of entries */
				   table->Format,      /* dest format */
				   dest,               /* dest address */
                                   format, type, data, /* src data */
				   &ctx->Unpack,
				   0);                 /* transfer ops */
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
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj = NULL;
   struct gl_color_table *table = NULL;
   GLboolean proxy = GL_FALSE;
   GLint baseFormat;
   GLfloat rScale = 1.0, gScale = 1.0, bScale = 1.0, aScale = 1.0;
   GLfloat rBias  = 0.0, gBias  = 0.0, bBias  = 0.0, aBias  = 0.0;
   GLenum tableType = CHAN_TYPE;
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
	 tableType = GL_FLOAT;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable;
	 tableType = GL_FLOAT;
         rScale = ctx->Pixel.ColorTableScale[0];
         gScale = ctx->Pixel.ColorTableScale[1];
         bScale = ctx->Pixel.ColorTableScale[2];
         aScale = ctx->Pixel.ColorTableScale[3];
         rBias = ctx->Pixel.ColorTableBias[0];
         gBias = ctx->Pixel.ColorTableBias[1];
         bBias = ctx->Pixel.ColorTableBias[2];
         aBias = ctx->Pixel.ColorTableBias[3];
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable;
         proxy = GL_TRUE;
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
            return;
         }
         table = &(texUnit->ColorTable);
	 tableType = GL_FLOAT;
         rScale = ctx->Pixel.TextureColorTableScale[0];
         gScale = ctx->Pixel.TextureColorTableScale[1];
         bScale = ctx->Pixel.TextureColorTableScale[2];
         aScale = ctx->Pixel.TextureColorTableScale[3];
         rBias = ctx->Pixel.TextureColorTableBias[0];
         gBias = ctx->Pixel.TextureColorTableBias[1];
         bBias = ctx->Pixel.TextureColorTableBias[2];
         aBias = ctx->Pixel.TextureColorTableBias[3];
         break;
      case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
            return;
         }
         table = &(texUnit->ProxyColorTable);
	 tableType = GL_FLOAT;
         proxy = GL_TRUE;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
	 tableType = GL_FLOAT;
         rScale = ctx->Pixel.PCCTscale[0];
         gScale = ctx->Pixel.PCCTscale[1];
         bScale = ctx->Pixel.PCCTscale[2];
         aScale = ctx->Pixel.PCCTscale[3];
         rBias = ctx->Pixel.PCCTbias[0];
         gBias = ctx->Pixel.PCCTbias[1];
         bBias = ctx->Pixel.PCCTbias[2];
         aBias = ctx->Pixel.PCCTbias[3];
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyPostConvolutionColorTable;
	 tableType = GL_FLOAT;
         proxy = GL_TRUE;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
	 tableType = GL_FLOAT;
         rScale = ctx->Pixel.PCMCTscale[0];
         gScale = ctx->Pixel.PCMCTscale[1];
         bScale = ctx->Pixel.PCMCTscale[2];
         aScale = ctx->Pixel.PCMCTscale[3];
         rBias = ctx->Pixel.PCMCTbias[0];
         gBias = ctx->Pixel.PCMCTbias[1];
         bBias = ctx->Pixel.PCMCTbias[2];
         aBias = ctx->Pixel.PCMCTbias[3];
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyPostColorMatrixColorTable;
	 tableType = GL_FLOAT;
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
   if (baseFormat < 0 || baseFormat == GL_COLOR_INDEX) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(internalFormat)");
      return;
   }

   if (width < 0 || (width != 0 && _mesa_bitcount(width) != 1)) {
      /* error */
      if (proxy) {
         table->Size = 0;
         table->IntFormat = (GLenum) 0;
         table->Format = (GLenum) 0;
      }
      else {
         _mesa_error(ctx, GL_INVALID_VALUE, "glColorTable(width=%d)", width);
      }
      return;
   }

   if (width > (GLsizei) ctx->Const.MaxColorTableSize) {
      if (proxy) {
         table->Size = 0;
         table->IntFormat = (GLenum) 0;
         table->Format = (GLenum) 0;
      }
      else {
         _mesa_error(ctx, GL_TABLE_TOO_LARGE, "glColorTable(width)");
      }
      return;
   }

   table->Size = width;
   table->IntFormat = internalFormat;
   table->Format = (GLenum) baseFormat;
   table->Type = (tableType == GL_FLOAT) ? GL_FLOAT : CHAN_TYPE;

   comps = _mesa_components_in_format(table->Format);
   assert(comps > 0);  /* error should have been caught sooner */

   if (!proxy) {
      /* free old table, if any */
      if (table->Table) {
         FREE(table->Table);
         table->Table = NULL;
      }

      if (width > 0) {
         if (table->Type == GL_FLOAT) {
	    table->Table = MALLOC(comps * width * sizeof(GLfloat));
	 }
	 else {
            table->Table = MALLOC(comps * width * sizeof(GLchan));
	 }

	 if (!table->Table) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glColorTable");
	    return;
	 }

	 store_colortable_entries(ctx, table,
				  0, width,  /* start, count */
				  format, type, data,
				  rScale, rBias,
				  gScale, gBias,
				  bScale, bBias,
				  aScale, aBias);
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
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj = NULL;
   struct gl_color_table *table = NULL;
   GLfloat rScale = 1.0, gScale = 1.0, bScale = 1.0, aScale = 1.0;
   GLfloat rBias  = 0.0, gBias  = 0.0, bBias  = 0.0, aBias  = 0.0;
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
         table = &ctx->ColorTable;
         rScale = ctx->Pixel.ColorTableScale[0];
         gScale = ctx->Pixel.ColorTableScale[1];
         bScale = ctx->Pixel.ColorTableScale[2];
         aScale = ctx->Pixel.ColorTableScale[3];
         rBias = ctx->Pixel.ColorTableBias[0];
         gBias = ctx->Pixel.ColorTableBias[1];
         bBias = ctx->Pixel.ColorTableBias[2];
         aBias = ctx->Pixel.ColorTableBias[3];
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         rScale = ctx->Pixel.TextureColorTableScale[0];
         gScale = ctx->Pixel.TextureColorTableScale[1];
         bScale = ctx->Pixel.TextureColorTableScale[2];
         aScale = ctx->Pixel.TextureColorTableScale[3];
         rBias = ctx->Pixel.TextureColorTableBias[0];
         gBias = ctx->Pixel.TextureColorTableBias[1];
         bBias = ctx->Pixel.TextureColorTableBias[2];
         aBias = ctx->Pixel.TextureColorTableBias[3];
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
         rScale = ctx->Pixel.PCCTscale[0];
         gScale = ctx->Pixel.PCCTscale[1];
         bScale = ctx->Pixel.PCCTscale[2];
         aScale = ctx->Pixel.PCCTscale[3];
         rBias = ctx->Pixel.PCCTbias[0];
         gBias = ctx->Pixel.PCCTbias[1];
         bBias = ctx->Pixel.PCCTbias[2];
         aBias = ctx->Pixel.PCCTbias[3];
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         rScale = ctx->Pixel.PCMCTscale[0];
         gScale = ctx->Pixel.PCMCTscale[1];
         bScale = ctx->Pixel.PCMCTscale[2];
         aScale = ctx->Pixel.PCMCTscale[3];
         rBias = ctx->Pixel.PCMCTbias[0];
         gBias = ctx->Pixel.PCMCTbias[1];
         bBias = ctx->Pixel.PCMCTbias[2];
         aBias = ctx->Pixel.PCMCTbias[3];
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
   assert(_mesa_components_in_format(table->Format) > 0);

   if (start + count > (GLint) table->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glColorSubTable(count)");
      return;
   }

   if (!table->Table) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glColorSubTable");
      return;
   }

   store_colortable_entries(ctx, table, start, count,
			    format, type, data,
			    rScale, rBias,
			    gScale, gBias,
			    bScale, bBias,
			    aScale, aBias);

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
   GLchan rgba[MAX_COLOR_TABLE_SIZE][4];
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
         table = &ctx->ColorTable;
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
         return;
   }

   ASSERT(table);

   switch (table->Format) {
      case GL_ALPHA:
         if (table->Type == GL_FLOAT) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = 0;
               rgba[i][GCOMP] = 0;
               rgba[i][BCOMP] = 0;
               rgba[i][ACOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = 0;
               rgba[i][GCOMP] = 0;
               rgba[i][BCOMP] = 0;
               rgba[i][ACOMP] = tableUB[i];
            }
         }
         break;
      case GL_LUMINANCE:
         if (table->Type == GL_FLOAT) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
               rgba[i][GCOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
               rgba[i][BCOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i];
               rgba[i][GCOMP] = tableUB[i];
               rgba[i][BCOMP] = tableUB[i];
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         break;
      case GL_LUMINANCE_ALPHA:
         if (table->Type == GL_FLOAT) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = IROUND_POS(tableF[i*2+0] * CHAN_MAXF);
               rgba[i][GCOMP] = IROUND_POS(tableF[i*2+0] * CHAN_MAXF);
               rgba[i][BCOMP] = IROUND_POS(tableF[i*2+0] * CHAN_MAXF);
               rgba[i][ACOMP] = IROUND_POS(tableF[i*2+1] * CHAN_MAXF);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i*2+0];
               rgba[i][GCOMP] = tableUB[i*2+0];
               rgba[i][BCOMP] = tableUB[i*2+0];
               rgba[i][ACOMP] = tableUB[i*2+1];
            }
         }
         break;
      case GL_INTENSITY:
         if (table->Type == GL_FLOAT) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
               rgba[i][GCOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
               rgba[i][BCOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
               rgba[i][ACOMP] = IROUND_POS(tableF[i] * CHAN_MAXF);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i];
               rgba[i][GCOMP] = tableUB[i];
               rgba[i][BCOMP] = tableUB[i];
               rgba[i][ACOMP] = tableUB[i];
            }
         }
         break;
      case GL_RGB:
         if (table->Type == GL_FLOAT) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = IROUND_POS(tableF[i*3+0] * CHAN_MAXF);
               rgba[i][GCOMP] = IROUND_POS(tableF[i*3+1] * CHAN_MAXF);
               rgba[i][BCOMP] = IROUND_POS(tableF[i*3+2] * CHAN_MAXF);
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i*3+0];
               rgba[i][GCOMP] = tableUB[i*3+1];
               rgba[i][BCOMP] = tableUB[i*3+2];
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         break;
      case GL_RGBA:
         if (table->Type == GL_FLOAT) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = IROUND_POS(tableF[i*4+0] * CHAN_MAXF);
               rgba[i][GCOMP] = IROUND_POS(tableF[i*4+1] * CHAN_MAXF);
               rgba[i][BCOMP] = IROUND_POS(tableF[i*4+2] * CHAN_MAXF);
               rgba[i][ACOMP] = IROUND_POS(tableF[i*4+3] * CHAN_MAXF);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i*4+0];
               rgba[i][GCOMP] = tableUB[i*4+1];
               rgba[i][BCOMP] = tableUB[i*4+2];
               rgba[i][ACOMP] = tableUB[i*4+3];
            }
         }
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

   _mesa_pack_rgba_span_chan(ctx, table->Size, (const GLchan (*)[4]) rgba,
                        format, type, data, &ctx->Pack, GL_FALSE);

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
            ctx->Pixel.ColorTableScale[0] = params[0];
            ctx->Pixel.ColorTableScale[1] = params[1];
            ctx->Pixel.ColorTableScale[2] = params[2];
            ctx->Pixel.ColorTableScale[3] = params[3];
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            ctx->Pixel.ColorTableBias[0] = params[0];
            ctx->Pixel.ColorTableBias[1] = params[1];
            ctx->Pixel.ColorTableBias[2] = params[2];
            ctx->Pixel.ColorTableBias[3] = params[3];
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
            ctx->Pixel.TextureColorTableScale[0] = params[0];
            ctx->Pixel.TextureColorTableScale[1] = params[1];
            ctx->Pixel.TextureColorTableScale[2] = params[2];
            ctx->Pixel.TextureColorTableScale[3] = params[3];
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            ctx->Pixel.TextureColorTableBias[0] = params[0];
            ctx->Pixel.TextureColorTableBias[1] = params[1];
            ctx->Pixel.TextureColorTableBias[2] = params[2];
            ctx->Pixel.TextureColorTableBias[3] = params[3];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            ctx->Pixel.PCCTscale[0] = params[0];
            ctx->Pixel.PCCTscale[1] = params[1];
            ctx->Pixel.PCCTscale[2] = params[2];
            ctx->Pixel.PCCTscale[3] = params[3];
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            ctx->Pixel.PCCTbias[0] = params[0];
            ctx->Pixel.PCCTbias[1] = params[1];
            ctx->Pixel.PCCTbias[2] = params[2];
            ctx->Pixel.PCCTbias[3] = params[3];
         }
         else {
            _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            ctx->Pixel.PCMCTscale[0] = params[0];
            ctx->Pixel.PCMCTscale[1] = params[1];
            ctx->Pixel.PCMCTscale[2] = params[2];
            ctx->Pixel.PCMCTscale[3] = params[3];
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            ctx->Pixel.PCMCTbias[0] = params[0];
            ctx->Pixel.PCMCTbias[1] = params[1];
            ctx->Pixel.PCMCTbias[2] = params[2];
            ctx->Pixel.PCMCTbias[3] = params[3];
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
         table = &ctx->ColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = ctx->Pixel.ColorTableScale[0];
            params[1] = ctx->Pixel.ColorTableScale[1];
            params[2] = ctx->Pixel.ColorTableScale[2];
            params[3] = ctx->Pixel.ColorTableScale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = ctx->Pixel.ColorTableBias[0];
            params[1] = ctx->Pixel.ColorTableBias[1];
            params[2] = ctx->Pixel.ColorTableBias[2];
            params[3] = ctx->Pixel.ColorTableBias[3];
            return;
         }
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable;
         break;
      case GL_TEXTURE_COLOR_TABLE_SGI:
         if (!ctx->Extensions.SGI_texture_color_table) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
            return;
         }
         table = &(texUnit->ColorTable);
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = ctx->Pixel.TextureColorTableScale[0];
            params[1] = ctx->Pixel.TextureColorTableScale[1];
            params[2] = ctx->Pixel.TextureColorTableScale[2];
            params[3] = ctx->Pixel.TextureColorTableScale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = ctx->Pixel.TextureColorTableBias[0];
            params[1] = ctx->Pixel.TextureColorTableBias[1];
            params[2] = ctx->Pixel.TextureColorTableBias[2];
            params[3] = ctx->Pixel.TextureColorTableBias[3];
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
         table = &ctx->PostConvolutionColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = ctx->Pixel.PCCTscale[0];
            params[1] = ctx->Pixel.PCCTscale[1];
            params[2] = ctx->Pixel.PCCTscale[2];
            params[3] = ctx->Pixel.PCCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = ctx->Pixel.PCCTbias[0];
            params[1] = ctx->Pixel.PCCTbias[1];
            params[2] = ctx->Pixel.PCCTbias[2];
            params[3] = ctx->Pixel.PCCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyPostConvolutionColorTable;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = ctx->Pixel.PCMCTscale[0];
            params[1] = ctx->Pixel.PCMCTscale[1];
            params[2] = ctx->Pixel.PCMCTscale[2];
            params[3] = ctx->Pixel.PCMCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = ctx->Pixel.PCMCTbias[0];
            params[1] = ctx->Pixel.PCMCTbias[1];
            params[2] = ctx->Pixel.PCMCTbias[2];
            params[3] = ctx->Pixel.PCMCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyPostColorMatrixColorTable;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameterfv(target)");
         return;
   }

   assert(table);

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
         *params = (GLfloat) table->IntFormat;
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
         table = &ctx->ColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = (GLint) ctx->Pixel.ColorTableScale[0];
            params[1] = (GLint) ctx->Pixel.ColorTableScale[1];
            params[2] = (GLint) ctx->Pixel.ColorTableScale[2];
            params[3] = (GLint) ctx->Pixel.ColorTableScale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = (GLint) ctx->Pixel.ColorTableBias[0];
            params[1] = (GLint) ctx->Pixel.ColorTableBias[1];
            params[2] = (GLint) ctx->Pixel.ColorTableBias[2];
            params[3] = (GLint) ctx->Pixel.ColorTableBias[3];
            return;
         }
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable;
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
         table = &ctx->PostConvolutionColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = (GLint) ctx->Pixel.PCCTscale[0];
            params[1] = (GLint) ctx->Pixel.PCCTscale[1];
            params[2] = (GLint) ctx->Pixel.PCCTscale[2];
            params[3] = (GLint) ctx->Pixel.PCCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = (GLint) ctx->Pixel.PCCTbias[0];
            params[1] = (GLint) ctx->Pixel.PCCTbias[1];
            params[2] = (GLint) ctx->Pixel.PCCTbias[2];
            params[3] = (GLint) ctx->Pixel.PCCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyPostConvolutionColorTable;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = (GLint) ctx->Pixel.PCMCTscale[0];
            params[1] = (GLint) ctx->Pixel.PCMCTscale[1];
            params[2] = (GLint) ctx->Pixel.PCMCTscale[2];
            params[3] = (GLint) ctx->Pixel.PCMCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = (GLint) ctx->Pixel.PCMCTbias[0];
            params[1] = (GLint) ctx->Pixel.PCMCTbias[1];
            params[2] = (GLint) ctx->Pixel.PCMCTbias[2];
            params[3] = (GLint) ctx->Pixel.PCMCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyPostColorMatrixColorTable;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameteriv(target)");
         return;
   }

   assert(table);

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
         *params = table->IntFormat;
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
   p->Type = CHAN_TYPE;
   p->Table = NULL;
   p->Size = 0;
   p->IntFormat = GL_RGBA;
}



void
_mesa_free_colortable_data( struct gl_color_table *p )
{
   if (p->Table) {
      FREE(p->Table);
      p->Table = NULL;
   }
}


/*
 * Initialize all colortables for a context.
 */
void _mesa_init_colortables( GLcontext * ctx )
{
   /* Color tables */
   _mesa_init_colortable(&ctx->ColorTable);
   _mesa_init_colortable(&ctx->ProxyColorTable);
   _mesa_init_colortable(&ctx->PostConvolutionColorTable);
   _mesa_init_colortable(&ctx->ProxyPostConvolutionColorTable);
   _mesa_init_colortable(&ctx->PostColorMatrixColorTable);
   _mesa_init_colortable(&ctx->ProxyPostColorMatrixColorTable);
}


/*
 * Free all colortable data for a context
 */
void _mesa_free_colortables_data( GLcontext *ctx )
{
   _mesa_free_colortable_data(&ctx->ColorTable);
   _mesa_free_colortable_data(&ctx->ProxyColorTable);
   _mesa_free_colortable_data(&ctx->PostConvolutionColorTable);
   _mesa_free_colortable_data(&ctx->ProxyPostConvolutionColorTable);
   _mesa_free_colortable_data(&ctx->PostColorMatrixColorTable);
   _mesa_free_colortable_data(&ctx->ProxyPostColorMatrixColorTable);
}
