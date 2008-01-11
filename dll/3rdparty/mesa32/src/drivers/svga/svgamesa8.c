/* $Id: svgamesa8.c,v 1.9.10.1 2006/11/02 12:02:17 alanh Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1995-2000  Brian Paul
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
 * SVGA driver for Mesa.
 * Original author:  Brian Paul
 * Additional authors:  Slawomir Szczyrba <steev@hot.pl>  (Mesa 3.2)
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#ifdef SVGA


#include "svgapix.h"
#include "svgamesa8.h"
#include "swrast/swrast.h"


static void __svga_drawpixel8(int x, int y, unsigned long c)
{
    unsigned long offset;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->linewidth + x;
    SVGABuffer.DrawBuffer[offset]=c;
}

static unsigned long __svga_getpixel8(int x, int y)
{
    unsigned long offset;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->linewidth + x;
    return SVGABuffer.ReadBuffer[offset];
}

void __clear_index8( GLcontext *ctx, GLuint index )
{
   SVGAMesa->clear_index = index;
}

void __clear8( GLcontext *ctx, GLbitfield mask )
{
   int i,j;
   int x = ctx->DrawBuffer->_Xmin;
   int y = ctx->DrawBuffer->_Ymin;
   int width = ctx->DrawBuffer->_Xmax - x;
   int height = ctx->DrawBuffer->_Ymax - y;
   GLboolean all = (width == ctx->DrawBuffer->Width && height == ctx->DrawBuffer->height)
   
   if (mask & DD_FRONT_LEFT_BIT) {
      if (all) { 
         memset(SVGABuffer.FrontBuffer, SVGAMesa->clear_index, SVGABuffer.BufferSize);
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.FrontBuffer;
         for (i=x;i<width;i++)
            for (j=y;j<height;j++)
               __svga_drawpixel8(i,j,SVGAMesa->clear_index);
         SVGABuffer.DrawBuffer = tmp;
      }
      mask &= ~DD_FRONT_LEFT_BIT;
   }
   if (mask & DD_BACK_LEFT_BIT) {
      if (all) { 
         memset(SVGABuffer.BackBuffer, SVGAMesa->clear_index, SVGABuffer.BufferSize);
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.BackBuffer;
         for (i=x;i<width;i++)
            for (j=y;j<height;j++)
               __svga_drawpixel8(i,j,SVGAMesa->clear_index);
         SVGABuffer.DrawBuffer = tmp;
      }
      mask &= ~DD_BACK_LEFT_BIT;
   }

   if (mask)
      _swrast_Clear( ctx, mask );
}

void __write_ci32_span8( const GLcontext *ctx, struct gl_renderbuffer *rb,
                         GLuint n, GLint x, GLint y,
                         const GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         __svga_drawpixel8( x, y, index[i]);
      }
   }
}

void __write_ci8_span8( const GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y,
                        const GLubyte index[], const GLubyte mask[] )
{
   int i;

   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         __svga_drawpixel8( x, y, index[i]);
      }
   }
}

void __write_mono_ci_span8( const GLcontext *ctx, struct gl_renderbuffer *rb,
                            GLuint n, GLint x, GLint y,
                            GLuint colorIndex, const GLubyte mask[] )
{
   int i;
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         __svga_drawpixel8( x, y, colorIndex);
      }
   }
}

void __read_ci32_span8( const GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y, GLuint index[])
{
   int i;
   for (i=0; i<n; i++,x++) {
      index[i] = __svga_getpixel8( x, y);
   }
}

void __write_ci32_pixels8( const GLcontext *ctx, struct gl_renderbuffer *rb,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel8( x[i], y[i], index[i]);
      }
   }
}


void __write_mono_ci_pixels8( const GLcontext *ctx, struct gl_renderbuffer *rb,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint colorIndex, const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel8( x[i], y[i], colorIndex);
      }
   }
}

void __read_ci32_pixels8( const GLcontext *ctx, struct gl_renderbuffer *rb,
                          GLuint n, const GLint x[], const GLint y[],
                          GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++,x++) {
      index[i] = __svga_getpixel8( x[i], y[i]);
   }
}


#else


/* silence compiler warning */
extern void _mesa_svga8_dummy_function(void);
void _mesa_svga8_dummy_function(void)
{
}


#endif
