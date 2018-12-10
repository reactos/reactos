/* $Id: colortab.c,v 1.6 1997/10/16 02:26:18 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.5
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: colortab.c,v $
 * Revision 1.6  1997/10/16 02:26:18  brianp
 * call Driver.UpdateTexturePalette(ctx,NULL) when shared palette changed
 *
 * Revision 1.5  1997/10/16 01:59:08  brianp
 * added GL_EXT_shared_texture_palette extension
 *
 * Revision 1.4  1997/10/15 23:37:05  brianp
 * removed dirty flag stuff
 *
 * Revision 1.3  1997/10/13 23:50:03  brianp
 * added call to Driver.UpdateTexturePalette
 *
 * Revision 1.2  1997/09/29 23:27:44  brianp
 * updated for new device driver texture functions
 *
 * Revision 1.1  1997/09/27 00:12:46  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "colortab.h"
#include "context.h"
#include "macros.h"
#endif



/*
 * Return GL_TRUE if k is a power of two, else return GL_FALSE.
 */
static GLboolean power_of_two( GLint k )
{
   GLint i, m = 1;
   for (i=0; i<32; i++) {
      if (k == m)
         return GL_TRUE;
      m = m << 1;
   }
   return GL_FALSE;
}


static GLint decode_internal_format( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case 2:
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
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case 4:
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


void gl_ColorTable( GLcontext *ctx, GLenum target,
                    GLenum internalFormat, struct gl_image *table )
{
   struct gl_texture_object *texObj;
   GLboolean proxy = GL_FALSE;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetBooleanv" );
      return;
   }

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = ctx->Texture.Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = ctx->Texture.Current2D;
         break;
      case GL_PROXY_TEXTURE_1D:
         texObj = ctx->Texture.Proxy1D;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_2D:
         texObj = ctx->Texture.Proxy2D;
         proxy = GL_TRUE;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glColorTableEXT(target)");
         return;
   }

   /* internalformat = just like glTexImage */

   if (table->Width < 1 || table->Width > MAX_TEXTURE_PALETTE_SIZE
       || !power_of_two(table->Width)) {
      gl_error(ctx, GL_INVALID_VALUE, "glColorTableEXT(width)");
      if (proxy) {
         texObj->PaletteSize = 0;
         texObj->PaletteIntFormat = 0;
         texObj->PaletteFormat = 0;
      }
      return;
   }

  /* per-texture object palette */
  texObj->PaletteSize = table->Width;
  texObj->PaletteIntFormat = internalFormat;
  texObj->PaletteFormat = decode_internal_format(internalFormat);
  if (!proxy) {
     MEMCPY(texObj->Palette, table->Data, table->Width*table->Components);
     if (ctx->Driver.UpdateTexturePalette) {
        (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
     }
   }
}



void gl_ColorSubTable( GLcontext *ctx, GLenum target,
                       GLsizei start, struct gl_image *data )
{
   /* XXX TODO */
   gl_problem(ctx, "glColorSubTableEXT not implemented");
}



void gl_GetColorTable( GLcontext *ctx, GLenum target, GLenum format,
                       GLenum type, GLvoid *table )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetBooleanv" );
      return;
   }

   switch (target) {
      case GL_TEXTURE_1D:
         break;
      case GL_TEXTURE_2D:
         break;
      case GL_TEXTURE_3D_EXT:
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableEXT(target)");
         return;
   }

   gl_problem(ctx, "glGetColorTableEXT not implemented!");
}



void gl_GetColorTableParameterfv( GLcontext *ctx, GLenum target,
                                  GLenum pname, GLfloat *params )
{
   GLint iparams[10];

   gl_GetColorTableParameteriv( ctx, target, pname, iparams );
   *params = (GLfloat) iparams[0];
}



void gl_GetColorTableParameteriv( GLcontext *ctx, GLenum target,
                                  GLenum pname, GLint *params )
{
   struct gl_texture_object *texObj;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetColorTableParameter" );
      return;
   }

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = ctx->Texture.Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = ctx->Texture.Current2D;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
         return;
   }

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT_EXT:
         if (texObj)
            *params = texObj->PaletteIntFormat;
         break;
      case GL_COLOR_TABLE_WIDTH_EXT:
         if (texObj)
            *params = texObj->PaletteSize;
         break;
      case GL_COLOR_TABLE_RED_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_GREEN_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_BLUE_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_ALPHA_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_LUMINANCE_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_INTENSITY_SIZE_EXT:
         *params = 8;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter" );
         return;
   }
}


