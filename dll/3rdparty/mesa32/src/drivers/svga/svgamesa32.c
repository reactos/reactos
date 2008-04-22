/* $Id: svgamesa32.c,v 1.12.36.1 2006/11/02 12:02:17 alanh Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.0
 * Copyright (C) 1995-2002  Brian Paul
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
#include "svgamesa32.h"
#include "swrast/swrast.h"


#if 0
/* this doesn't compile with GCC on RedHat 6.1 */
static inline int RGB2BGR32(int c)
{
	asm("rorw  $8, %0\n"	 
	    "rorl $16, %0\n"	 
	    "rorw  $8, %0\n"	 
	    "shrl  $8, %0\n"	 
      : "=q"(c):"0"(c));
    return c;
}
#else
static unsigned long RGB2BGR32(unsigned long color)
{
   return (color & 0xff00)|(color>>16)|((color & 0xff)<<16);
}
#endif

static void __svga_drawpixel32(int x, int y, unsigned long c)
{
    unsigned long offset;

    GLint *intBuffer=(void *)SVGABuffer.DrawBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;
    intBuffer[offset]=c;
}

static unsigned long __svga_getpixel32(int x, int y)
{
    unsigned long offset;

    const GLint *intBuffer=(void *)SVGABuffer.ReadBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;
    return intBuffer[offset];
}

void __clear_color32( GLcontext *ctx, const GLfloat color[4] )
{
   GLubyte col[3];
   CLAMPED_FLOAT_TO_UBYTE(col[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(col[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(col[2], color[2]);
   SVGAMesa->clear_truecolor = (col[0] << 16) | (col[1] << 8) | col[2];
}

void __clear32( GLcontext *ctx, GLbitfield mask )
{
   int i,j;
   int x = ctx->DrawBuffer->_Xmin;
   int y = ctx->DrawBuffer->_Ymin;
   int width = ctx->DrawBuffer->_Xmax - x;
   int height = ctx->DrawBuffer->_Ymax - y;
   GLboolean all = (width == ctx->DrawBuffer->Width && height == ctx->DrawBuffer->height)
   
   if (mask & DD_FRONT_LEFT_BIT) {
      if (all) {
         GLint *intBuffer=(void *)SVGABuffer.FrontBuffer;
         for (i=0;i<SVGABuffer.BufferSize / 4;i++)
            intBuffer[i]=SVGAMesa->clear_truecolor;
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.FrontBuffer;
         for (i=x;i<width;i++)    
            for (j=y;j<height;j++)    
               __svga_drawpixel32(i,j,SVGAMesa->clear_truecolor);
         SVGABuffer.DrawBuffer = tmp;
      }
      mask &= ~DD_FRONT_LEFT_BIT;
   }
   if (mask & DD_BACK_LEFT_BIT) {
      if (all) {
         GLint *intBuffer=(void *)SVGABuffer.BackBuffer;
         for (i=0;i<SVGABuffer.BufferSize / 4;i++)
            intBuffer[i]=SVGAMesa->clear_truecolor;
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.BackBuffer;
         for (i=x;i<width;i++)    
            for (j=y;j<height;j++)    
               __svga_drawpixel32(i,j,SVGAMesa->clear_truecolor);
         SVGABuffer.DrawBuffer = tmp;
      }
      mask &= ~DD_BACK_LEFT_BIT;
   }

   if (mask)
      _swrast_Clear( ctx, mask );
}

void __write_rgba_span32( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   if (mask) {
      /* draw some pixels */
      for (i=0; i<n; i++, x++) {
         if (mask[i]) {
         __svga_drawpixel32( x, y, RGB2BGR32(*((GLint*)rgba[i])));
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0; i<n; i++, x++) {
         __svga_drawpixel32( x, y, RGB2BGR32(*((GLint*)rgba[i])));
      }
   }
}

void __write_mono_rgba_span32( const GLcontext *ctx,
                               GLuint n, GLint x, GLint y,
                               const GLchan color[4], const GLubyte mask[])
{
   int i;
   GLuint truecolor = color[RCOMP]<<16 | color[GCOMP]<<8 | color[BCOMP];
   for (i=0; i<n; i++, x++) {
      if (mask[i]) {
         __svga_drawpixel32( x, y, truecolor);
      }
   }
}

void __read_rgba_span32( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         GLubyte rgba[][4] )
{
   int i;
   for (i=0; i<n; i++, x++) {
     *((GLint*)rgba[i]) = RGB2BGR32(__svga_getpixel32( x, y ));
   }
}

void __write_rgba_pixels32( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel32( x[i], y[i], RGB2BGR32(*((GLint*)rgba[i])));
      }
   }
}

void __write_mono_rgba_pixels32( const GLcontext *ctx,
                                 GLuint n,
                                 const GLint x[], const GLint y[],
                                 const GLchan color[4], const GLubyte mask[] )
{
   GLuint truecolor = color[RCOMP]<<16 | color[GCOMP]<<8 | color[BCOMP];
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel32( x[i], y[i], truecolor );
      }
   }
}

void __read_rgba_pixels32( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++,x++) {
    *((GLint*)rgba[i]) = RGB2BGR32(__svga_getpixel32( x[i], y[i] ));
   }
}


#else


/* silence compiler warning */
extern void _mesa_svga32_dummy_function(void);
void _mesa_svga32_dummy_function(void)
{
}


#endif

