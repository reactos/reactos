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
 *
 */

#ifndef I810TEX_INC
#define I810TEX_INC

#include "mtypes.h"
#include "mm.h"

#include "i810context.h"
#include "i810_3d_reg.h"
#include "texmem.h"

#define I810_TEX_MAXLEVELS 11

/* For shared texture space managment, these texture objects may also
 * be used as proxies for regions of texture memory containing other
 * client's textures.  Such proxy textures (not to be confused with GL
 * proxy textures) are subject to the same LRU aging we use for our
 * own private textures, and thus we have a mechanism where we can
 * fairly decide between kicking out our own textures and those of
 * other clients.
 *
 * Non-local texture objects have a valid MemBlock to describe the
 * region managed by the other client, and can be identified by
 * 't->globj == 0' 
 */
struct i810_texture_object_t {
   driTextureObject base;
     
   int Pitch;
   int Height;
   int texelBytes;
   char *BufAddr;
   
   GLuint max_level;

   struct { 
      const struct gl_texture_image *image;
      int offset;		/* into BufAddr */
      int height;
      int internalFormat;
   } image[I810_TEX_MAXLEVELS];

   GLuint Setup[I810_TEX_SETUP_SIZE];
   GLuint dirty;

};		

void i810UpdateTextureState( GLcontext *ctx );
void i810InitTextureFuncs( struct dd_function_table *functions );

void i810DestroyTexObj( i810ContextPtr imesa, i810TextureObjectPtr t );
int i810UploadTexImagesLocked( i810ContextPtr imesa, i810TextureObjectPtr t );

#endif
