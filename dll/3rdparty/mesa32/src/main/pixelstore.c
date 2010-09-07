/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

/**
 * \file pixelstore.c
 * glPixelStore functions.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "macros.h"
#include "pixelstore.h"
#include "mtypes.h"


void GLAPIENTRY
_mesa_PixelStorei( GLenum pname, GLint param )
{
   /* NOTE: this call can't be compiled into the display list */
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_PACK_SWAP_BYTES:
	 if (param == (GLint)ctx->Pack.SwapBytes)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.SwapBytes = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_LSB_FIRST:
	 if (param == (GLint)ctx->Pack.LsbFirst)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_ROW_LENGTH:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.RowLength == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.RowLength = param;
	 break;
      case GL_PACK_IMAGE_HEIGHT:
         if (param<0) {
            _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.ImageHeight == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.ImageHeight = param;
         break;
      case GL_PACK_SKIP_PIXELS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipPixels == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipPixels = param;
	 break;
      case GL_PACK_SKIP_ROWS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipRows == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipRows = param;
	 break;
      case GL_PACK_SKIP_IMAGES:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipImages == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipImages = param;
	 break;
      case GL_PACK_ALIGNMENT:
         if (param!=1 && param!=2 && param!=4 && param!=8) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.Alignment == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.Alignment = param;
	 break;
      case GL_PACK_INVERT_MESA:
         if (!ctx->Extensions.MESA_pack_invert) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glPixelstore(pname)" );
            return;
         }
         if (ctx->Pack.Invert == param)
            return;
         FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.Invert = param;
         break;

      case GL_UNPACK_SWAP_BYTES:
	 if (param == (GLint)ctx->Unpack.SwapBytes)
	    return;
	 if ((GLint)ctx->Unpack.SwapBytes == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SwapBytes = param ? GL_TRUE : GL_FALSE;
         break;
      case GL_UNPACK_LSB_FIRST:
	 if (param == (GLint)ctx->Unpack.LsbFirst)
	    return;
	 if ((GLint)ctx->Unpack.LsbFirst == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.RowLength == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.RowLength = param;
	 break;
      case GL_UNPACK_IMAGE_HEIGHT:
         if (param<0) {
            _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.ImageHeight == param)
	    return;

	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.ImageHeight = param;
         break;
      case GL_UNPACK_SKIP_PIXELS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipPixels == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipPixels = param;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipRows == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipRows = param;
	 break;
      case GL_UNPACK_SKIP_IMAGES:
	 if (param < 0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipImages == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipImages = param;
	 break;
      case GL_UNPACK_ALIGNMENT:
         if (param!=1 && param!=2 && param!=4 && param!=8) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore" );
	    return;
	 }
	 if (ctx->Unpack.Alignment == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.Alignment = param;
	 break;
      case GL_UNPACK_CLIENT_STORAGE_APPLE:
         if (param == (GLint)ctx->Unpack.ClientStorage)
            return;
         FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Unpack.ClientStorage = param ? GL_TRUE : GL_FALSE;
         break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glPixelStore" );
	 return;
   }
}


void GLAPIENTRY
_mesa_PixelStoref( GLenum pname, GLfloat param )
{
   _mesa_PixelStorei( pname, (GLint) param );
}



/**
 * Initialize the context's pixel store state.
 */
void
_mesa_init_pixelstore( GLcontext *ctx )
{
   /* Pixel transfer */
   ctx->Pack.Alignment = 4;
   ctx->Pack.RowLength = 0;
   ctx->Pack.ImageHeight = 0;
   ctx->Pack.SkipPixels = 0;
   ctx->Pack.SkipRows = 0;
   ctx->Pack.SkipImages = 0;
   ctx->Pack.SwapBytes = GL_FALSE;
   ctx->Pack.LsbFirst = GL_FALSE;
   ctx->Pack.ClientStorage = GL_FALSE;
   ctx->Pack.Invert = GL_FALSE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->Pack.BufferObj = ctx->Array.NullBufferObj;
#endif
   ctx->Unpack.Alignment = 4;
   ctx->Unpack.RowLength = 0;
   ctx->Unpack.ImageHeight = 0;
   ctx->Unpack.SkipPixels = 0;
   ctx->Unpack.SkipRows = 0;
   ctx->Unpack.SkipImages = 0;
   ctx->Unpack.SwapBytes = GL_FALSE;
   ctx->Unpack.LsbFirst = GL_FALSE;
   ctx->Unpack.ClientStorage = GL_FALSE;
   ctx->Unpack.Invert = GL_FALSE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->Unpack.BufferObj = ctx->Array.NullBufferObj;
#endif

   /*
    * _mesa_unpack_image() returns image data in this format.  When we
    * execute image commands (glDrawPixels(), glTexImage(), etc) from
    * within display lists we have to be sure to set the current
    * unpacking parameters to these values!
    */
   ctx->DefaultPacking.Alignment = 1;
   ctx->DefaultPacking.RowLength = 0;
   ctx->DefaultPacking.SkipPixels = 0;
   ctx->DefaultPacking.SkipRows = 0;
   ctx->DefaultPacking.ImageHeight = 0;
   ctx->DefaultPacking.SkipImages = 0;
   ctx->DefaultPacking.SwapBytes = GL_FALSE;
   ctx->DefaultPacking.LsbFirst = GL_FALSE;
   ctx->DefaultPacking.ClientStorage = GL_FALSE;
   ctx->DefaultPacking.Invert = GL_FALSE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
#endif
}
