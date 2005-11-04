/*
 * GLX Hardware Device Driver for Intel i810
 * Copyright (C) 1999 Keith Whitwell
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
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Adapted for use in the I830M driver:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 */

#ifndef I830TEX_INC
#define I830TEX_INC

#include "mtypes.h"
#include "i830_context.h"
#include "i830_3d_reg.h"
#include "texmem.h"

#define I830_TEX_MAXLEVELS 10

struct i830_texture_object_t 
{
   driTextureObject    base;

   int texelBytes;
   int Pitch;
   int Height;
   char *BufAddr;   
   GLenum palette_format;
   GLuint palette[256];
   struct {
      const struct gl_texture_image *image;
      int offset;       /* into BufAddr */
      int height;
      int internalFormat;
   } image[6][I830_TEX_MAXLEVELS];

   /* Support for multitexture.
    */

   GLuint current_unit;
   GLuint Setup[I830_TEX_SETUP_SIZE];
   GLuint dirty;

   GLfloat  max_anisotropy;
};

void i830UpdateTextureState( GLcontext *ctx );
void i830InitTextureFuncs( struct dd_function_table *functions );

void i830DestroyTexObj( i830ContextPtr imesa, i830TextureObjectPtr t );
int i830UploadTexImagesLocked( i830ContextPtr imesa, i830TextureObjectPtr t );

#endif
