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
 * \file swrast/s_bitmap.c
 * \brief glBitmap rendering.
 * \author Brian Paul
 */

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/pixel.h"

#include "s_context.h"
#include "s_span.h"



/**
 * Render a bitmap.
 * Called via ctx->Driver.Bitmap()
 * All parameter error checking will have been done before this is called.
 */
void
_swrast_Bitmap( GLcontext *ctx, GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLint row, col;
   GLuint count = 0;
   SWspan span;

   ASSERT(ctx->RenderMode == GL_RENDER);

   bitmap = _mesa_map_bitmap_pbo(ctx, unpack, bitmap);
   if (!bitmap)
      return;

   RENDER_START(swrast,ctx);

   if (SWRAST_CONTEXT(ctx)->NewState)
      _swrast_validate_derived( ctx );

   INIT_SPAN(span, GL_BITMAP);
   span.end = width;
   span.arrayMask = SPAN_XY;
   _swrast_span_default_attribs(ctx, &span);

   for (row = 0; row < height; row++) {
      const GLubyte *src = (const GLubyte *) _mesa_image_address2d(unpack,
                 bitmap, width, height, GL_COLOR_INDEX, GL_BITMAP, row, 0);

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {
            if (*src & mask) {
               span.array->x[count] = px + col;
               span.array->y[count] = py + row;
               count++;
            }
            if (mask == 128U) {
               src++;
               mask = 1U;
            }
            else {
               mask = mask << 1;
            }
         }

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {
            if (*src & mask) {
               span.array->x[count] = px + col;
               span.array->y[count] = py + row;
               count++;
            }
            if (mask == 1U) {
               src++;
               mask = 128U;
            }
            else {
               mask = mask >> 1;
            }
         }

         /* get ready for next row */
         if (mask != 128)
            src++;
      }

      if (count + width >= MAX_WIDTH || row + 1 == height) {
         /* flush the span */
         span.end = count;
         if (ctx->Visual.rgbMode)
            _swrast_write_rgba_span(ctx, &span);
         else
            _swrast_write_index_span(ctx, &span);
         span.end = 0;
         count = 0;
      }
   }

   RENDER_FINISH(swrast,ctx);

   _mesa_unmap_bitmap_pbo(ctx, unpack);
}


#if 0
/*
 * XXX this is another way to implement Bitmap.  Use horizontal runs of
 * fragments, initializing the mask array to indicate which fragments to
 * draw or skip.
 */
void
_swrast_Bitmap( GLcontext *ctx, GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLint row, col;
   SWspan span;

   ASSERT(ctx->RenderMode == GL_RENDER);
   ASSERT(bitmap);

   RENDER_START(swrast,ctx);

   if (SWRAST_CONTEXT(ctx)->NewState)
      _swrast_validate_derived( ctx );

   INIT_SPAN(span, GL_BITMAP);
   span.end = width;
   span.arrayMask = SPAN_MASK;
   _swrast_span_default_attribs(ctx, &span);

   /*span.arrayMask |= SPAN_MASK;*/  /* we'll init span.mask[] */
   span.x = px;
   span.y = py;
   /*span.end = width;*/

   for (row=0; row<height; row++, span.y++) {
      const GLubyte *src = (const GLubyte *) _mesa_image_address2d(unpack,
                 bitmap, width, height, GL_COLOR_INDEX, GL_BITMAP, row, 0);

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            span.array->mask[col] = (*src & mask) ? GL_TRUE : GL_FALSE;
            if (mask == 128U) {
               src++;
               mask = 1U;
            }
            else {
               mask = mask << 1;
            }
         }

         if (ctx->Visual.rgbMode)
            _swrast_write_rgba_span(ctx, &span);
         else
	    _swrast_write_index_span(ctx, &span);

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col=0; col<width; col++) {
            span.array->mask[col] = (*src & mask) ? GL_TRUE : GL_FALSE;
            if (mask == 1U) {
               src++;
               mask = 128U;
            }
            else {
               mask = mask >> 1;
            }
         }

         if (ctx->Visual.rgbMode)
            _swrast_write_rgba_span(ctx, &span);
         else
            _swrast_write_index_span(ctx, &span);

         /* get ready for next row */
         if (mask != 128)
            src++;
      }
   }

   RENDER_FINISH(swrast,ctx);
}
#endif
