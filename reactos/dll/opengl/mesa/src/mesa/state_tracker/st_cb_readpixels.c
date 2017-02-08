/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "main/imports.h"
#include "main/readpix.h"

#include "st_atom.h"
#include "st_context.h"
#include "st_cb_bitmap.h"
#include "st_cb_readpixels.h"


/**
 * The only special thing we need to do for the state tracker's
 * glReadPixels is to validate state (to be sure we have up-to-date
 * framebuffer surfaces) and flush the bitmap cache prior to reading.
 */
static void
st_readpixels(struct gl_context *ctx, GLint x, GLint y,
              GLsizei width, GLsizei height,
              GLenum format, GLenum type,
              const struct gl_pixelstore_attrib *pack,
              GLvoid *dest)
{
   struct st_context *st = st_context(ctx);

   st_validate_state(st);
   st_flush_bitmap_cache(st);
   _mesa_readpixels(ctx, x, y, width, height, format, type, pack, dest);
}


void st_init_readpixels_functions(struct dd_function_table *functions)
{
   functions->ReadPixels = st_readpixels;
}
