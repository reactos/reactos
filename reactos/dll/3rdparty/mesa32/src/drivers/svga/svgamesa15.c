/* $Id: svgamesa15.c,v 1.11.36.1 2006/11/02 12:02:17 alanh Exp $ */

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
#include "svgamesa15.h"
#include "swrast/swrast.h"


static void __svga_drawpixel15(int x, int y, unsigned long c)
{
    unsigned long offset;
    GLshort *shortBuffer=(void *)SVGABuffer.DrawBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;
    shortBuffer[offset]=c;
}

static unsigned long __svga_getpixel15(int x, int y)
{
    unsigned long offset;
    GLshort *shortBuffer=(void *)SVGABuffer.ReadBuffer;
    y = SVGAInfo->height-y-1;
    offset = y * SVGAInfo->width + x;
    return shortBuffer[offset];
}

void __clear_color15( GLcontext *ctx, const GLfloat color[4] )
{
   GLubyte col[3];
   CLAMPED_FLOAT_TO_UBYTE(col[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(col[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(col[2], color[2]);
   SVGAMesa->clear_hicolor=(col[0]>>3)<<10 | (col[1]>>3)<<5 | (col[2]>>3);  
/*   SVGAMesa->clear_hicolor=(red)<<10 | (green)<<5 | (blue);*/
}   

void __clear15( GLcontext *ctx, GLbitfield mask )
{
   int i, j;
   int x = ctx->DrawBuffer->_Xmin;
   int y = ctx->DrawBuffer->_Ymin;
   int width = ctx->DrawBuffer->_Xmax - x;
   int height = ctx->DrawBuffer->_Ymax - y;
   GLboolean all = (width == ctx->DrawBuffer->Width && height == ctx->DrawBuffer->height)

   if (mask & DD_FRONT_LEFT_BIT) {
      GLshort *shortBuffer=(void *)SVGABuffer.FrontBuffer;
      if (all) {
         for (i=0;i<SVGABuffer.BufferSize / 2;i++)
            shortBuffer[i]=SVGAMesa->clear_hicolor;
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.FrontBuffer;
         for (i=x;i<width;i++)
            for (j=y;j<height;j++)
               __svga_drawpixel15(i,j,SVGAMesa->clear_hicolor);
         SVGABuffer.DrawBuffer = tmp;
      }
      mask &= ~DD_FRONT_LEFT_BIT;
   }
   if (mask & DD_BACK_LEFT_BIT) {
      GLshort *shortBuffer=(void *)SVGABuffer.BackBuffer;
      if (all) {
         for (i=0;i<SVGABuffer.BufferSize / 2;i++)
            shortBuffer[i]=SVGAMesa->clear_hicolor;
      }
      else {
         GLubyte *tmp = SVGABuffer.DrawBuffer;
         SVGABuffer.DrawBuffer = SVGABuffer.BackBuffer;
         for (i=x;i<width;i++)
            for (j=y;j<height;j++)
               __svga_drawpixel15(i,j,SVGAMesa->clear_hicolor);
         SVGABuffer.DrawBuffer = tmp;
      }
      mask &= ~DD_BACK_LEFT_BIT;
   }

   if (mask)
      _swrast_Clear( ctx, mask );
}

void __write_rgba_span15( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   if (mask) {
      /* draw some pixels */
      for (i=0; i<n; i++, x++) {
         if (mask[i]) {
         __svga_drawpixel15( x, y, (rgba[i][RCOMP]>>3)<<10 | \
			           (rgba[i][GCOMP]>>3)<<5 |  \
			           (rgba[i][BCOMP]>>3));
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0; i<n; i++, x++) {
         __svga_drawpixel15( x, y, (rgba[i][RCOMP]>>3)<<10 | \
			           (rgba[i][GCOMP]>>3)<<5  | \
			           (rgba[i][BCOMP]>>3));
      }
   }
}

void __write_mono_rgba_span15( const GLcontext *ctx,
                               GLuint n, GLint x, GLint y,
                               const GLchan color[4], const GLubyte mask[])
{
   GLushort hicolor = (color[RCOMP] >> 3) << 10 |
                      (color[GCOMP] >> 3) << 5 |
                      (color[BCOMP] >> 3); 
   int i;
   for (i=0; i<n; i++, x++) {
      if (mask[i]) {
         __svga_drawpixel15( x, y, hicolor);
      }
   }
}

void __read_rgba_span15( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         GLubyte rgba[][4] )
{
   int i,pix;
   for (i=0; i<n; i++, x++) {
    pix = __svga_getpixel15( x, y);
    rgba[i][RCOMP] = ((pix>>10)<<3) & 0xff;
    rgba[i][GCOMP] = ((pix>> 5)<<3) & 0xff;
    rgba[i][BCOMP] = ((pix    )<<3) & 0xff;
   }
}

void __write_rgba_pixels15( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLubyte rgba[][4], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel15( x[i], y[i], (rgba[i][RCOMP]>>3)<<10 | \
			                 (rgba[i][GCOMP]>>3)<<5  | \
			                 (rgba[i][BCOMP]>>3));
      }
   }
}


void __write_mono_rgba_pixels15( const GLcontext *ctx,
                                 GLuint n,
                                 const GLint x[], const GLint y[],
                                 const GLchan color[4], const GLubyte mask[] )
{
   GLushort hicolor = (color[RCOMP] >> 3) << 10 |
                      (color[GCOMP] >> 3) << 5 |
                      (color[BCOMP] >> 3); 
   int i;
   /* use current rgb color */
   for (i=0; i<n; i++) {
      if (mask[i]) {
         __svga_drawpixel15( x[i], y[i], hicolor );
      }
   }
}

void __read_rgba_pixels15( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4], const GLubyte mask[] )
{
   int i,pix;
   for (i=0; i<n; i++,x++) {
    pix = __svga_getpixel15( x[i], y[i] );
    rgba[i][RCOMP] = ((pix>>10)<<3) & 0xff;
    rgba[i][GCOMP] = ((pix>> 5)<<3) & 0xff;
    rgba[i][BCOMP] = ((pix    )<<3) & 0xff;
   }
}

#else


/* silence compiler warning */
extern void _mesa_svga15_dummy_function(void);
void _mesa_svga15_dummy_function(void)
{
}


#endif
