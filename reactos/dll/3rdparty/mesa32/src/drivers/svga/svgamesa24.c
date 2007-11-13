/* $Id: svgamesa24.c,v 1.12.36.1 2006/11/02 12:02:17 alanh Exp $ */

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
#include "svgamesa24.h"
#include "swrast/swrast.h"


#if 0
/* this doesn't compile with GCC on RedHat 6.1 */
static inline int RGB2BGR24(int c)
{
	asm("rorw  $8, %0\n"	 
	    "rorl $16, %0\n"	 
	    "rorw  $8, %0\n"	 
	    "shrl  $8, %0\n"	 
      : "=q"(c):"0"(c));
    return c;
}
#else
static unsigned long RGB2BGR24(unsigned long color)
{
   return (color & 0xff00)|(color>>16)|((color & 0xff)<<16);
}
#endif

static void __svga_drawpixel24(int x, int y, GLubyte r, GLubyte g, GLubyte b)
{
    unsigned long offset;

    _RGB *rgbBuffer=(void *)SVGABuffer.DrawBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;

    rgbBuffer[offset].r=r;
    rgbBuffer[offset].g=g;
    rgbBuffer[offset].b=b;
}

static unsigned long __svga_getpixel24(int x, int y)
{
    unsigned long offset;

    _RGB *rgbBuffer=(void *)SVGABuffer.ReadBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;
    return rgbBuffer[offset].r<<16 | rgbBuffer[offset].g<<8 | rgbBuffer[offset].b;
}

void __clear_color24( GLcontext *ctx, const GLfloat color[4] )
{
   GLubyte col[3];
   CLAMPED_FLOAT_TO_UBYTE(col[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(col[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(col[2], color[2]);
   SVGAMesa->clear_red = col[0];
   SVGAMesa->clear_green = col[1];
   SVGAMesa->clear_blue = col[2];
/*   SVGAMesa->clear_truecolor = red<<16 | green<<8 | blue; */
}

void __clear24( GLcontext *ctx, GLbitfield mask )
{
   int i,j;
   int x = ctx->DrawBuffer->_Xmin;
   int y = ctx->DrawBuffer->_Ymin;
   int width = ctx->DrawBuffer->_Xmax - x;
   int height = ctx->DrawBuffer->_Ymax - y;
   GLboolean all = (width == ctx->DrawBuffer->Width && height == ctx->DrawBuffer->height)
   
   if (mask & DD_FRONT_LEFT_BIT) {
      if (all) {
         _RGB *rgbBuffer=(void *)SVGABuffer.FrontBuffer;
         for (i=0;i<SVGABuffer.BufferSize / 3;i++) {
            rgbBuffer[i].r=SVGAMesa->clear_red;
            rgbBuffer[i].g=SVGAMesa->clear_green;
            rgbBuffer[i].b=SVGAMesa->clear_blue;
         } 
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.FrontBuffer;
         for (i=x;i<width;i++)    
            for (j=y;j<height;j++)    
               __svga_drawpixel24( i, j, SVGAMesa->clear_red,
                                   SVGAMesa->clear_green,
                                   SVGAMesa->clear_blue);
         SVGABuffer.DrawBuffer = tmp;
      }	
      mask &= ~DD_FRONT_LEFT_BIT;
   }
   if (mask & DD_BACK_LEFT_BIT) {
      if (all) {
         _RGB *rgbBuffer=(void *)SVGABuffer.BackBuffer;
         for (i=0;i<SVGABuffer.BufferSize / 3;i++) {
            rgbBuffer[i].r=SVGAMesa->clear_red;
            rgbBuffer[i].g=SVGAMesa->clear_green;
            rgbBuffer[i].b=SVGAMesa->clear_blue;
         } 
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.BackBuffer;
         for (i=x;i<width;i++)    
            for (j=y;j<height;j++)    
               __svga_drawpixel24( i, j, SVGAMesa->clear_red,
                                   SVGAMesa->clear_green,
                                   SVGAMesa->clear_blue);
         SVGABuffer.DrawBuffer = tmp;
      }	
      mask &= ~DD_BACK_LEFT_BIT;
   }

   if (mask)
      _swrast_Clear( ctx, mask );
}

void __write_rgba_span24( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   if (mask) {
      /* draw some pixels */
      for (i=0; i<n; i++, x++) {
         if (mask[i]) {
         __svga_drawpixel24( x, y, rgba[i][RCOMP],
	                           rgba[i][GCOMP],
				   rgba[i][BCOMP]);
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0; i<n; i++, x++) {
         __svga_drawpixel24( x, y, rgba[i][RCOMP],
	                           rgba[i][GCOMP],
				   rgba[i][BCOMP]);
      }
   }
}

void __write_mono_rgba_span24( const GLcontext *ctx,
                               GLuint n, GLint x, GLint y,
                               const GLchan color[4], const GLubyte mask[])
{
   int i;
   for (i=0; i<n; i++, x++) {
      if (mask[i]) {
         __svga_drawpixel24( x, y, color[RCOMP], color[GCOMP], color[BCOMP]);
      }
   }
}

void __read_rgba_span24( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         GLubyte rgba[][4] )
{
   int i;
   for (i=0; i<n; i++, x++) {
    *((GLint*)rgba[i]) = RGB2BGR24(__svga_getpixel24( x, y));
   }
}

void __write_rgba_pixels24( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel24( x[i], y[i], rgba[i][RCOMP],
	                                 rgba[i][GCOMP],
				         rgba[i][BCOMP]);
      }
   }
}

void __write_mono_rgba_pixels24( const GLcontext *ctx,
                                 GLuint n,
                                 const GLint x[], const GLint y[],
                                 const GLchan color[4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel24( x[i], y[i],
                             color[RCOMP], color[GCOMP], color[BCOMP] );
      }
   }
}

void __read_rgba_pixels24( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++,x++) {
    *((GLint*)rgba[i]) = RGB2BGR24(__svga_getpixel24( x[i], y[i]));    
   }
}

#else


/* silence compiler warning */
extern void _mesa_svga24_dummy_function(void);
void _mesa_svga24_dummy_function(void)
{
}


#endif
