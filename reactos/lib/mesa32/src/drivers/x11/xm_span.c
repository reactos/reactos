/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
/* $XFree86: xc/extras/Mesa/src/X/xm_span.c,v 1.3 2002/02/27 21:07:54 tsi Exp $ */

#include "glxheader.h"
#include "colormac.h"
#include "context.h"
#include "depth.h"
#include "drawpix.h"
#include "extensions.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"
#include "state.h"
#include "xmesaP.h"

#include "swrast/swrast.h"


/*
 * The following functions are used to trap XGetImage() calls which
 * generate BadMatch errors if the drawable isn't mapped.
 */

#ifndef XFree86Server
static int caught_xgetimage_error = 0;
static int (*old_xerror_handler)( XMesaDisplay *dpy, XErrorEvent *ev );
static unsigned long xgetimage_serial;

/*
 * This is the error handler which will be called if XGetImage fails.
 */
static int xgetimage_error_handler( XMesaDisplay *dpy, XErrorEvent *ev )
{
   if (ev->serial==xgetimage_serial && ev->error_code==BadMatch) {
      /* caught the expected error */
      caught_xgetimage_error = 0;
   }
   else {
      /* call the original X error handler, if any.  otherwise ignore */
      if (old_xerror_handler) {
         (*old_xerror_handler)( dpy, ev );
      }
   }
   return 0;
}


/*
 * Call this right before XGetImage to setup error trap.
 */
static void catch_xgetimage_errors( XMesaDisplay *dpy )
{
   xgetimage_serial = NextRequest( dpy );
   old_xerror_handler = XSetErrorHandler( xgetimage_error_handler );
   caught_xgetimage_error = 0;
}


/*
 * Call this right after XGetImage to check if an error occured.
 */
static int check_xgetimage_errors( void )
{
   /* restore old handler */
   (void) XSetErrorHandler( old_xerror_handler );
   /* return 0=no error, 1=error caught */
   return caught_xgetimage_error;
}
#endif


/*
 * Read a pixel from an X drawable.
 */
static unsigned long read_pixel( XMesaDisplay *dpy,
                                 XMesaDrawable d, int x, int y )
{
   unsigned long p;
#ifndef XFree86Server
   XMesaImage *pixel = NULL;
   int error;

   catch_xgetimage_errors( dpy );
   pixel = XGetImage( dpy, d, x, y, 1, 1, AllPlanes, ZPixmap );
   error = check_xgetimage_errors();
   if (pixel && !error) {
      p = XMesaGetPixel( pixel, 0, 0 );
   }
   else {
      p = 0;
   }
   if (pixel) {
      XMesaDestroyImage( pixel );
   }
#else
   (*dpy->GetImage)(d, x, y, 1, 1, ZPixmap, ~0L, (pointer)&p);
#endif
   return p;
}



/*
 * The Mesa library needs to be able to draw pixels in a number of ways:
 *   1. RGB vs Color Index
 *   2. as horizontal spans (polygons, images) vs random locations (points,
 *      lines)
 *   3. different color per-pixel or same color for all pixels
 *
 * Furthermore, the X driver needs to support rendering to 3 possible
 * "buffers", usually one, but sometimes two at a time:
 *   1. The front buffer as an X window
 *   2. The back buffer as a Pixmap
 *   3. The back buffer as an XImage
 *
 * Finally, if the back buffer is an XImage, we can avoid using XPutPixel and
 * optimize common cases such as 24-bit and 8-bit modes.
 *
 * By multiplication, there's at least 48 possible combinations of the above.
 *
 * Below are implementations of the most commonly used combinations.  They are
 * accessed through function pointers which get initialized here and are used
 * directly from the Mesa library.  The 8 function pointers directly correspond
 * to the first 3 cases listed above.
 *
 *
 * The function naming convention is:
 *
 *   write_[span|pixels]_[mono]_[format]_[pixmap|ximage]
 *
 * New functions optimized for specific cases can be added without too much
 * trouble.  An example might be the 24-bit TrueColor mode 8A8R8G8B which is
 * found on IBM RS/6000 X servers.
 */




/**********************************************************************/
/*** Write COLOR SPAN functions                                     ***/
/**********************************************************************/


#define RGBA_SPAN_ARGS	const GLcontext *ctx,				\
			GLuint n, GLint x, GLint y,			\
			CONST GLubyte rgba[][4], const GLubyte mask[]

#define RGB_SPAN_ARGS	const GLcontext *ctx,				\
			GLuint n, GLint x, GLint y,			\
			CONST GLubyte rgb[][3], const GLubyte mask[]


/* NOTE: if mask==NULL, draw all pixels */


/*
 * Write a span of PF_TRUECOLOR pixels to a pixmap.
 */
static void write_span_TRUECOLOR_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;

   (void)DitherValues;		/* Muffle compiler */

   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUECOLOR( p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
            XMesaSetForeground( dpy, gc, p );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         unsigned long p;
         PACK_TRUECOLOR( p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         XMesaPutPixel( rowimg, i, 0, p );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_TRUECOLOR pixels to a pixmap.
 */
static void write_span_rgb_TRUECOLOR_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUECOLOR( p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
            XMesaSetForeground( dpy, gc, p );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         unsigned long p;
         PACK_TRUECOLOR( p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
         XMesaPutPixel( rowimg, i, 0, p );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_TRUEDITHER pixels to a pixmap.
 */
static void write_span_TRUEDITHER_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
            XMesaSetForeground( dpy, gc, p );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         unsigned long p;
         PACK_TRUEDITHER(p, x+i, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
         XMesaPutPixel( rowimg, i, 0, p );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_TRUEDITHER pixels to a pixmap (no alpha).
 */
static void write_span_rgb_TRUEDITHER_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
            XMesaSetForeground( dpy, gc, p );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         unsigned long p;
         PACK_TRUEDITHER(p, x+i, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
         XMesaPutPixel( rowimg, i, 0, p );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}



/*
 * Write a span of PF_8A8B8G8R pixels to a pixmap.
 */
static void write_span_8A8B8G8R_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
                         PACK_8A8B8G8R(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP], rgba[i][ACOMP]) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLuint *ptr4 = (GLuint *) rowimg->data;
      for (i=0;i<n;i++) {
         *ptr4++ = PACK_8A8B8G8R( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP], rgba[i][ACOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_8A8B8G8R pixels to a pixmap (no alpha).
 */
static void write_span_rgb_8A8B8G8R_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
                   PACK_8B8G8R(rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLuint *ptr4 = (GLuint *) rowimg->data;
      for (i=0;i<n;i++) {
         *ptr4++ = PACK_8B8G8R(rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_8R8G8B pixels to a pixmap.
 */
static void write_span_8R8G8B_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, PACK_8R8G8B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ));
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLuint *ptr4 = (GLuint *) rowimg->data;
      for (i=0;i<n;i++) {
         *ptr4++ = PACK_8R8G8B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_8R8G8B24 pixels to a pixmap.
 */
static void write_span_8R8G8B24_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      register GLuint i;
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
               PACK_8R8G8B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ));
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLuint *ptr4 = (GLuint *) rowimg->data;
      register GLuint pixel;
      static const GLuint shift[4] = {0, 8, 16, 24};
      register GLuint i = 0;
      int w = n;
      while (w > 3) {
         pixel  = rgba[i][BCOMP] /* << shift[0]*/;
         pixel |= rgba[i][GCOMP]    << shift[1];
         pixel |= rgba[i++][RCOMP]  << shift[2];
         pixel |= rgba[i][BCOMP]    << shift[3];
         *ptr4++ = pixel;

         pixel  = rgba[i][GCOMP] /* << shift[0]*/;
         pixel |= rgba[i++][RCOMP]  << shift[1];
         pixel |= rgba[i][BCOMP]    << shift[2];
         pixel |= rgba[i][GCOMP]    << shift[3];
         *ptr4++ = pixel;

         pixel  = rgba[i++][RCOMP]/* << shift[0]*/;
         pixel |= rgba[i][BCOMP]     << shift[1];
         pixel |= rgba[i][GCOMP]     << shift[2];
         pixel |= rgba[i++][RCOMP]   << shift[3];
         *ptr4++ = pixel;

         w -= 4;
      }
      switch (w) {
         case 3:
            pixel = 0;
            pixel |= rgba[i][BCOMP] /*<< shift[0]*/;
            pixel |= rgba[i][GCOMP]   << shift[1];
            pixel |= rgba[i++][RCOMP] << shift[2];
            pixel |= rgba[i][BCOMP]   << shift[3];
            *ptr4++ = pixel;
            pixel = 0;
            pixel |= rgba[i][GCOMP] /*<< shift[0]*/;
            pixel |= rgba[i++][RCOMP] << shift[1];
            pixel |= rgba[i][BCOMP]   << shift[2];
            pixel |= rgba[i][GCOMP]   << shift[3];
            *ptr4++ = pixel;
            pixel = 0xffffff00 & *ptr4;
            pixel |= rgba[i][RCOMP] /*<< shift[0]*/;
            *ptr4 = pixel;
            break;
         case 2:
            pixel = 0;
            pixel |= rgba[i][BCOMP] /*<< shift[0]*/;
            pixel |= rgba[i][GCOMP]   << shift[1];
            pixel |= rgba[i++][RCOMP] << shift[2];
            pixel |= rgba[i][BCOMP]   << shift[3];
            *ptr4++ = pixel;
            pixel = 0xffff0000 & *ptr4;
            pixel |= rgba[i][GCOMP] /*<< shift[0]*/;
            pixel |= rgba[i][RCOMP]   << shift[1];
            *ptr4 = pixel;
            break;
         case 1:
            pixel = 0xff000000 & *ptr4;
            pixel |= rgba[i][BCOMP] /*<< shift[0]*/;
            pixel |= rgba[i][GCOMP] << shift[1];
            pixel |= rgba[i][RCOMP] << shift[2];
            *ptr4 = pixel;
            break;
         case 0:
            break;
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_8R8G8B pixels to a pixmap (no alpha).
 */
static void write_span_rgb_8R8G8B_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, PACK_8R8G8B( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ));
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLuint *ptr4 = (GLuint *) rowimg->data;
      for (i=0;i<n;i++) {
         *ptr4++ = PACK_8R8G8B( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_8R8G8B24 pixels to a pixmap (no alpha).
 */
static void write_span_rgb_8R8G8B24_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      register GLuint i;
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
                  PACK_8R8G8B( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ));
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLuint *ptr4 = (GLuint *) rowimg->data;
      register GLuint pixel;
      static const GLuint shift[4] = {0, 8, 16, 24};
      unsigned w = n;
      register GLuint i = 0;
      while (w > 3) {
         pixel = 0;
         pixel |= rgb[i][BCOMP]/* << shift[0]*/;
         pixel |= rgb[i][GCOMP] << shift[1];
         pixel |= rgb[i++][RCOMP] << shift[2];
         pixel |= rgb[i][BCOMP] <<shift[3];
         *ptr4++ = pixel;

         pixel = 0;
         pixel |= rgb[i][GCOMP]/* << shift[0]*/;
         pixel |= rgb[i++][RCOMP] << shift[1];
         pixel |= rgb[i][BCOMP] << shift[2];
         pixel |= rgb[i][GCOMP] << shift[3];
         *ptr4++ = pixel;

         pixel = 0;
         pixel |= rgb[i++][RCOMP]/* << shift[0]*/;
         pixel |= rgb[i][BCOMP] << shift[1];
         pixel |= rgb[i][GCOMP] << shift[2];
         pixel |= rgb[i++][RCOMP] << shift[3];
         *ptr4++ = pixel;
         w -= 4;
      }
      switch (w) {
         case 3:
            pixel = 0;
            pixel |= rgb[i][BCOMP]/* << shift[0]*/;
            pixel |= rgb[i][GCOMP] << shift[1];
            pixel |= rgb[i++][RCOMP] << shift[2];
            pixel |= rgb[i][BCOMP] << shift[3];
            *ptr4++ = pixel;
            pixel = 0;
            pixel |= rgb[i][GCOMP]/* << shift[0]*/;
            pixel |= rgb[i++][RCOMP] << shift[1];
            pixel |= rgb[i][BCOMP] << shift[2];
            pixel |= rgb[i][GCOMP] << shift[3];
            *ptr4++ = pixel;
            pixel = *ptr4;
            pixel &= 0xffffff00;
            pixel |= rgb[i++][RCOMP]/* << shift[0]*/;
            *ptr4++ = pixel;
            break;
         case 2:
            pixel = 0;
            pixel |= rgb[i][BCOMP]/* << shift[0]*/;
            pixel |= rgb[i][GCOMP] << shift[1];
            pixel |= rgb[i++][RCOMP] << shift[2];
            pixel |= rgb[i][BCOMP]  << shift[3];
            *ptr4++ = pixel;
            pixel = *ptr4;
            pixel &= 0xffff0000;
            pixel |= rgb[i][GCOMP]/* << shift[0]*/;
            pixel |= rgb[i++][RCOMP] << shift[1];
            *ptr4++ = pixel;
            break;
         case 1:
            pixel = *ptr4;
            pixel &= 0xff000000;
            pixel |= rgb[i][BCOMP]/* << shift[0]*/;
            pixel |= rgb[i][GCOMP] << shift[1];
            pixel |= rgb[i++][RCOMP] << shift[2];
            *ptr4++ = pixel;
            break;
         case 0:
            break;
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_5R6G5B pixels to a pixmap.
 */
static void write_span_5R6G5B_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, PACK_5R6G5B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ));
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLushort *ptr2 = (GLushort *) rowimg->data;
      for (i=0;i<n;i++) {
         ptr2[i] = PACK_5R6G5B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_DITHER_5R6G5B pixels to a pixmap.
 */
static void write_span_DITHER_5R6G5B_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
            XMesaSetForeground( dpy, gc, p );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLushort *ptr2 = (GLushort *) rowimg->data;
      for (i=0;i<n;i++) {
         PACK_TRUEDITHER( ptr2[i], x+i, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_5R6G5B pixels to a pixmap (no alpha).
 */
static void write_span_rgb_5R6G5B_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, PACK_5R6G5B( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ));
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLushort *ptr2 = (GLushort *) rowimg->data;
      for (i=0;i<n;i++) {
         ptr2[i] = PACK_5R6G5B( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_DITHER_5R6G5B pixels to a pixmap (no alpha).
 */
static void write_span_rgb_DITHER_5R6G5B_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
            XMesaSetForeground( dpy, gc, p );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLushort *ptr2 = (GLushort *) rowimg->data;
      for (i=0;i<n;i++) {
         PACK_TRUEDITHER( ptr2[i], x+i, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}



/*
 * Write a span of PF_DITHER pixels to a pixmap.
 */
static void write_span_DITHER_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   XDITHER_SETUP(y);
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, XDITHER(x, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0, XDITHER(x+i, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_DITHER pixels to a pixmap (no alpha).
 */
static void write_span_rgb_DITHER_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   XDITHER_SETUP(y);
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, XDITHER(x, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0, XDITHER(x+i, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_1BIT pixels to a pixmap.
 */
static void write_span_1BIT_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   SETUP_1BIT;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
                            DITHER_1BIT( x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0,
                    DITHER_1BIT( x+i, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_1BIT pixels to a pixmap (no alpha).
 */
static void write_span_rgb_1BIT_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   SETUP_1BIT;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
              DITHER_1BIT(x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      /* draw all pixels */
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0,
          DITHER_1BIT(x+i, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_HPCR pixels to a pixmap.
 */
static void write_span_HPCR_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
                            DITHER_HPCR( x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLubyte *ptr = (GLubyte *) xmesa->xm_buffer->rowimage->data;
      for (i=0;i<n;i++) {
         ptr[i] = DITHER_HPCR( (x+i), y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_HPCR pixels to a pixmap (no alpha).
 */
static void write_span_rgb_HPCR_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc,
              DITHER_HPCR(x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      register GLubyte *ptr = (GLubyte *) xmesa->xm_buffer->rowimage->data;
      for (i=0;i<n;i++) {
         ptr[i] = DITHER_HPCR( (x+i), y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_LOOKUP pixels to a pixmap.
 */
static void write_span_LOOKUP_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   LOOKUP_SETUP;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0, LOOKUP(rgba[i][RCOMP],rgba[i][GCOMP],rgba[i][BCOMP]) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_LOOKUP pixels to a pixmap (no alpha).
 */
static void write_span_rgb_LOOKUP_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   LOOKUP_SETUP;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, LOOKUP( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0, LOOKUP(rgb[i][RCOMP],rgb[i][GCOMP],rgb[i][BCOMP]) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}



/*
 * Write a span of PF_GRAYSCALE pixels to a pixmap.
 */
static void write_span_GRAYSCALE_pixmap( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0, GRAY_RGB(rgba[i][RCOMP],rgba[i][GCOMP],rgba[i][BCOMP]) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_GRAYSCALE pixels to a pixmap (no alpha).
 */
static void write_span_rgb_GRAYSCALE_pixmap( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, GRAY_RGB( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      XMesaImage *rowimg = xmesa->xm_buffer->rowimage;
      for (i=0;i<n;i++) {
         XMesaPutPixel( rowimg, i, 0, GRAY_RGB(rgb[i][RCOMP],rgb[i][GCOMP],rgb[i][BCOMP]) );
      }
      XMesaPutImage( dpy, buffer, gc, rowimg, 0, 0, x, y, n, 1 );
   }
}


/*
 * Write a span of PF_TRUECOLOR pixels to an XImage.
 */
static void write_span_TRUECOLOR_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUECOLOR( p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
            XMesaPutPixel( img, x, y, p );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         unsigned long p;
         PACK_TRUECOLOR( p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         XMesaPutPixel( img, x, y, p );
      }
   }
}


/*
 * Write a span of PF_TRUECOLOR pixels to an XImage (no alpha).
 */
static void write_span_rgb_TRUECOLOR_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUECOLOR( p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
            XMesaPutPixel( img, x, y, p );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         unsigned long p;
         PACK_TRUECOLOR( p, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
         XMesaPutPixel( img, x, y, p );
      }
   }
}


/*
 * Write a span of PF_TRUEDITHER pixels to an XImage.
 */
static void write_span_TRUEDITHER_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
            XMesaPutPixel( img, x, y, p );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         unsigned long p;
         PACK_TRUEDITHER(p, x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
         XMesaPutPixel( img, x, y, p );
      }
   }
}


/*
 * Write a span of PF_TRUEDITHER pixels to an XImage (no alpha).
 */
static void write_span_rgb_TRUEDITHER_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
            XMesaPutPixel( img, x, y, p );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         unsigned long p;
         PACK_TRUEDITHER(p, x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
         XMesaPutPixel( img, x, y, p );
      }
   }
}


/*
 * Write a span of PF_8A8B8G8R-format pixels to an ximage.
 */
static void write_span_8A8B8G8R_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = PACK_8A8B8G8R( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP], rgba[i][ACOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++) {
         ptr[i] = PACK_8A8B8G8R( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP], rgba[i][ACOMP] );
      }
   }
}


/*
 * Write a span of PF_8A8B8G8R-format pixels to an ximage (no alpha).
 */
static void write_span_rgb_8A8B8G8R_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = PACK_8A8B8G8R( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP], 255 );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++) {
         ptr[i] = PACK_8A8B8G8R( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP], 255 );
      }
   }
}


/*
 * Write a span of PF_8R8G8B-format pixels to an ximage.
 */
static void write_span_8R8G8B_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = PACK_8R8G8B(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
         }
      }
   }
   else {
      for (i=0;i<n;i++) {
         ptr[i] = PACK_8R8G8B(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
   }
}


/*
 * Write a span of PF_8R8G8B24-format pixels to an ximage.
 */
static void write_span_8R8G8B24_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = (GLubyte *) PIXELADDR3( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLuint *ptr4 = (GLuint *) ptr;
            register GLuint pixel = *ptr4;
            switch (3 & (int)(ptr - (GLubyte*)ptr4)) {
               case 0:
                  pixel &= 0xff000000;
                  pixel |= rgba[i][BCOMP];
                  pixel |= rgba[i][GCOMP] << 8;
                  pixel |= rgba[i][RCOMP] << 16;
                  *ptr4 = pixel;
                  break;
               case 3:
                  pixel &= 0x00ffffff;
                  pixel |= rgba[i][BCOMP] << 24;
                  *ptr4++ = pixel;
                  pixel = *ptr4 && 0xffff0000;
                  pixel |= rgba[i][GCOMP];
                  pixel |= rgba[i][RCOMP] << 8;
                  *ptr4 = pixel;
                  break;
               case 2:
                  pixel &= 0x0000ffff;
                  pixel |= rgba[i][BCOMP] << 16;
                  pixel |= rgba[i][GCOMP] << 24;
                  *ptr4++ = pixel;
                  pixel = *ptr4 && 0xffffff00;
                  pixel |= rgba[i][RCOMP];
                  *ptr4 = pixel;
                  break;
               case 1:
                  pixel &= 0x000000ff;
                  pixel |= rgba[i][BCOMP] << 8;
                  pixel |= rgba[i][GCOMP] << 16;
                  pixel |= rgba[i][RCOMP] << 24;
                  *ptr4 = pixel;
                  break;
            }
         }
	 ptr += 3;
      }
   }
   else {
      /* write all pixels */
      int w = n;
      GLuint *ptr4 = (GLuint *) ptr;
      register GLuint pixel = *ptr4;
      int index = (int)(ptr - (GLubyte *)ptr4);
      register GLuint i = 0;
      switch (index) {
         case 0:
            break;
         case 1:
            pixel &= 0x00ffffff;
            pixel |= rgba[i][BCOMP] << 24;
            *ptr4++ = pixel;
            pixel = *ptr4 && 0xffff0000;
            pixel |= rgba[i][GCOMP];
            pixel |= rgba[i++][RCOMP] << 8;
            *ptr4 = pixel;
            if (0 == --w)
               break;
         case 2:
            pixel &= 0x0000ffff;
            pixel |= rgba[i][BCOMP] << 16;
            pixel |= rgba[i][GCOMP] << 24;
            *ptr4++ = pixel;
            pixel = *ptr4 && 0xffffff00;
            pixel |= rgba[i++][RCOMP];
            *ptr4 = pixel;
            if (0 == --w)
               break;
         case 3:
            pixel &= 0x000000ff;
            pixel |= rgba[i][BCOMP] << 8;
            pixel |= rgba[i][GCOMP] << 16;
            pixel |= rgba[i++][RCOMP] << 24;
            *ptr4++ = pixel;
            if (0 == --w)
               break;
            break;
      }
      while (w > 3) {
         pixel = rgba[i][BCOMP];
         pixel |= rgba[i][GCOMP] << 8;
         pixel |= rgba[i++][RCOMP] << 16;
         pixel |= rgba[i][BCOMP] << 24;
         *ptr4++ = pixel;
         pixel = rgba[i][GCOMP];
         pixel |= rgba[i++][RCOMP] << 8;
         pixel |= rgba[i][BCOMP] << 16;
         pixel |= rgba[i][GCOMP] << 24;
         *ptr4++ = pixel;
         pixel = rgba[i++][RCOMP];
         pixel |= rgba[i][BCOMP] << 8;
         pixel |= rgba[i][GCOMP] << 16;
         pixel |= rgba[i++][RCOMP] << 24;
         *ptr4++ = pixel;
         w -= 4;
      }
      switch (w) {
         case 0:
            break;
         case 1:
            pixel = *ptr4 & 0xff000000;
            pixel |= rgba[i][BCOMP];
            pixel |= rgba[i][GCOMP] << 8;
            pixel |= rgba[i][RCOMP] << 16;
            *ptr4 = pixel;
            break;
         case 2:
            pixel = rgba[i][BCOMP];
            pixel |= rgba[i][GCOMP] << 8;
            pixel |= rgba[i++][RCOMP] << 16;
            pixel |= rgba[i][BCOMP] << 24;
            *ptr4++ = pixel;
            pixel = *ptr4 & 0xffff0000;
            pixel |= rgba[i][GCOMP];
            pixel |= rgba[i][RCOMP] << 8;
            *ptr4 = pixel;
            break;
         case 3:
            pixel = rgba[i][BCOMP];
            pixel |= rgba[i][GCOMP] << 8;
            pixel |= rgba[i++][RCOMP] << 16;
            pixel |= rgba[i][BCOMP] << 24;
            *ptr4++ = pixel;
            pixel = rgba[i][GCOMP];
            pixel |= rgba[i++][RCOMP] << 8;
            pixel |= rgba[i][BCOMP] << 16;
            pixel |= rgba[i][GCOMP] << 24;
            *ptr4++ = pixel;
            pixel = *ptr4 & 0xffffff00;
            pixel |= rgba[i][RCOMP];
            *ptr4 = pixel;
            break;
      }
   }
}


/*
 * Write a span of PF_8R8G8B-format pixels to an ximage (no alpha).
 */
static void write_span_rgb_8R8G8B_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = PACK_8R8G8B(rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++) {
         ptr[i] = PACK_8R8G8B(rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
      }
   }
}


/*
 * Write a span of PF_8R8G8B24-format pixels to an ximage (no alpha).
 */
static void write_span_rgb_8R8G8B24_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = (GLubyte *) PIXELADDR3( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            *ptr++ = rgb[i][BCOMP];
            *ptr++ = rgb[i][GCOMP];
            *ptr++ = rgb[i][RCOMP];
         }
         else {
            ptr += 3;
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++) {
         *ptr++ = rgb[i][BCOMP];
         *ptr++ = rgb[i][GCOMP];
         *ptr++ = rgb[i][RCOMP];
      }
   }
}


/*
 * Write a span of PF_5R6G5B-format pixels to an ximage.
 */
static void write_span_5R6G5B_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = PACK_5R6G5B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
#if defined(__i386__) /* word stores don't have to be on 4-byte boundaries */
      GLuint *ptr32 = (GLuint *) ptr;
      GLuint extraPixel = (n & 1);
      n -= extraPixel;
      for (i = 0; i < n; i += 2) {
         GLuint p0, p1;
         p0 = PACK_5R6G5B(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
         p1 = PACK_5R6G5B(rgba[i+1][RCOMP], rgba[i+1][GCOMP], rgba[i+1][BCOMP]);
         *ptr32++ = (p1 << 16) | p0;
      }
      if (extraPixel) {
         ptr[n] = PACK_5R6G5B(rgba[n][RCOMP], rgba[n][GCOMP], rgba[n][BCOMP]);
      }
#else
      for (i = 0; i < n; i++) {
         ptr[i] = PACK_5R6G5B(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
#endif
   }
}


/*
 * Write a span of PF_DITHER_5R6G5B-format pixels to an ximage.
 */
static void write_span_DITHER_5R6G5B_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x, y );
   const GLint y2 = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            PACK_TRUEDITHER( ptr[i], x, y2, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
#if defined(__i386__) /* word stores don't have to be on 4-byte boundaries */
      GLuint *ptr32 = (GLuint *) ptr;
      GLuint extraPixel = (n & 1);
      n -= extraPixel;
      for (i = 0; i < n; i += 2, x += 2) {
         GLuint p0, p1;
         PACK_TRUEDITHER( p0, x, y2, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         PACK_TRUEDITHER( p1, x+1, y2, rgba[i+1][RCOMP], rgba[i+1][GCOMP], rgba[i+1][BCOMP] );
         *ptr32++ = (p1 << 16) | p0;
      }
      if (extraPixel) {
         PACK_TRUEDITHER( ptr[n], x+n, y2, rgba[n][RCOMP], rgba[n][GCOMP], rgba[n][BCOMP]);
      }
#else
      for (i = 0; i < n; i++, x++) {
         PACK_TRUEDITHER( ptr[i], x, y2, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
      }
#endif
   }
}


/*
 * Write a span of PF_5R6G5B-format pixels to an ximage (no alpha).
 */
static void write_span_rgb_5R6G5B_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = PACK_5R6G5B( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
#if defined(__i386__) /* word stores don't have to be on 4-byte boundaries */
      GLuint *ptr32 = (GLuint *) ptr;
      GLuint extraPixel = (n & 1);
      n -= extraPixel;
      for (i = 0; i < n; i += 2) {
         GLuint p0, p1;
         p0 = PACK_5R6G5B(rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]);
         p1 = PACK_5R6G5B(rgb[i+1][RCOMP], rgb[i+1][GCOMP], rgb[i+1][BCOMP]);
         *ptr32++ = (p1 << 16) | p0;
      }
      if (extraPixel) {
         ptr[n] = PACK_5R6G5B(rgb[n][RCOMP], rgb[n][GCOMP], rgb[n][BCOMP]);
      }
#else
      for (i=0;i<n;i++) {
         ptr[i] = PACK_5R6G5B( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
#endif
   }
}


/*
 * Write a span of PF_DITHER_5R6G5B-format pixels to an ximage (no alpha).
 */
static void write_span_rgb_DITHER_5R6G5B_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            PACK_TRUEDITHER( ptr[i], x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
#if defined(__i386__) /* word stores don't have to be on 4-byte boundaries */
      GLuint *ptr32 = (GLuint *) ptr;
      GLuint extraPixel = (n & 1);
      n -= extraPixel;
      for (i = 0; i < n; i += 2, x += 2) {
         GLuint p0, p1;
         PACK_TRUEDITHER( p0, x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
         PACK_TRUEDITHER( p1, x+1, y, rgb[i+1][RCOMP], rgb[i+1][GCOMP], rgb[i+1][BCOMP] );
         *ptr32++ = (p1 << 16) | p0;
      }
      if (extraPixel) {
         PACK_TRUEDITHER( ptr[n], x+n, y, rgb[n][RCOMP], rgb[n][GCOMP], rgb[n][BCOMP]);
      }
#else
      for (i=0;i<n;i++,x++) {
         PACK_TRUEDITHER( ptr[i], x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
#endif
   }
}


/*
 * Write a span of PF_DITHER pixels to an XImage.
 */
static void write_span_DITHER_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   int yy = FLIP(xmesa->xm_buffer, y);
   XDITHER_SETUP(yy);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel( img, x, yy, XDITHER( x, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, yy, XDITHER( x, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
      }
   }
}


/*
 * Write a span of PF_DITHER pixels to an XImage (no alpha).
 */
static void write_span_rgb_DITHER_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   int yy = FLIP(xmesa->xm_buffer, y);
   XDITHER_SETUP(yy);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel( img, x, yy, XDITHER( x, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, yy, XDITHER( x, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
      }
   }
}



/*
 * Write a span of 8-bit PF_DITHER pixels to an XImage.
 */
static void write_span_DITHER8_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   XDITHER_SETUP(y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            ptr[i] = (GLubyte) XDITHER( x, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         }
      }
   }
   else {
      for (i=0;i<n;i++,x++) {
         ptr[i] = (GLubyte) XDITHER( x, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


static void write_span_rgb_DITHER8_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   XDITHER_SETUP(y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            ptr[i] = (GLubyte) XDITHER( x, rgb[i][0], rgb[i][1], rgb[i][2] );
         }
      }
   }
   else {
      const GLubyte *data = (GLubyte *) rgb;
      for (i=0;i<n;i++,x++) {
         /*ptr[i] = XDITHER( x, rgb[i][0], rgb[i][1], rgb[i][2] );*/
         ptr[i] = (GLubyte) XDITHER( x, data[i+i+i], data[i+i+i+1], data[i+i+i+2] );
      }
   }
}



/*
 * Write a span of PF_1BIT pixels to an XImage.
 */
static void write_span_1BIT_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   SETUP_1BIT;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel(img, x, y, DITHER_1BIT(x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]));
         }
      }
   }
   else {
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, y, DITHER_1BIT(x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]) );
      }
   }
}


/*
 * Write a span of PF_1BIT pixels to an XImage (no alpha).
 */
static void write_span_rgb_1BIT_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   SETUP_1BIT;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel(img, x, y, DITHER_1BIT(x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]));
         }
      }
   }
   else {
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, y, DITHER_1BIT(x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP]) );
      }
   }
}


/*
 * Write a span of PF_HPCR pixels to an XImage.
 */
static void write_span_HPCR_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            ptr[i] = DITHER_HPCR( x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         ptr[i] = DITHER_HPCR( x, y, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write a span of PF_HPCR pixels to an XImage (no alpha).
 */
static void write_span_rgb_HPCR_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            ptr[i] = DITHER_HPCR( x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         ptr[i] = DITHER_HPCR( x, y, rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
   }
}


/*
 * Write a span of PF_LOOKUP pixels to an XImage.
 */
static void write_span_LOOKUP_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   LOOKUP_SETUP;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel( img, x, y, LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, y, LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
      }
   }
}


/*
 * Write a span of PF_LOOKUP pixels to an XImage (no alpha).
 */
static void write_span_rgb_LOOKUP_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   LOOKUP_SETUP;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel( img, x, y, LOOKUP( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, y, LOOKUP( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
      }
   }
}


/*
 * Write a span of 8-bit PF_LOOKUP pixels to an XImage.
 */
static void write_span_LOOKUP8_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   LOOKUP_SETUP;
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            ptr[i] = (GLubyte) LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         ptr[i] = (GLubyte) LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


static void write_rgb_LOOKUP8_ximage( const GLcontext *ctx,
                                      GLuint n, GLint x, GLint y,
                                      CONST GLubyte rgb[][3],
                                      const GLubyte mask[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   LOOKUP_SETUP;
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            ptr[i] = (GLubyte) LOOKUP( rgb[i][0], rgb[i][1], rgb[i][2] );
         }
      }
   }
   else {
      /* draw all pixels */
      const GLubyte *data = (GLubyte *) rgb;
      for (i=0;i<n;i++,x++) {
         /*ptr[i] = LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );*/
         ptr[i] = (GLubyte) LOOKUP( data[i+i+i], data[i+i+i+1], data[i+i+i+2] );
      }
   }
}


/*
 * Write a span of PF_GRAYSCALE pixels to an XImage.
 */
static void write_span_GRAYSCALE_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel( img, x, y, GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, y, GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
      }
   }
}


/*
 * Write a span of PF_GRAYSCALE pixels to an XImage (no alpha).
 */
static void write_span_rgb_GRAYSCALE_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel( img, x, y, GRAY_RGB( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, y, GRAY_RGB( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] ) );
      }
   }
}


/*
 * Write a span of 8-bit PF_GRAYSCALE pixels to an XImage.
 */
static void write_span_GRAYSCALE8_ximage( RGBA_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = (GLubyte) GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++) {
         ptr[i] = (GLubyte) GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write a span of 8-bit PF_GRAYSCALE pixels to an XImage (no alpha).
 */
static void write_span_rgb_GRAYSCALE8_ximage( RGB_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x, y );
   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            ptr[i] = (GLubyte) GRAY_RGB( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0;i<n;i++) {
         ptr[i] = (GLubyte) GRAY_RGB( rgb[i][RCOMP], rgb[i][GCOMP], rgb[i][BCOMP] );
      }
   }
}




/**********************************************************************/
/*** Write COLOR PIXEL functions                                    ***/
/**********************************************************************/


#define RGBA_PIXEL_ARGS   const GLcontext *ctx,				\
			  GLuint n, const GLint x[], const GLint y[],	\
			  CONST GLubyte rgba[][4], const GLubyte mask[]


/*
 * Write an array of PF_TRUECOLOR pixels to a pixmap.
 */
static void write_pixels_TRUECOLOR_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUECOLOR( p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
	 XMesaSetForeground( dpy, gc, p );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_TRUEDITHER pixels to a pixmap.
 */
static void write_pixels_TRUEDITHER_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUEDITHER(p, x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
         XMesaSetForeground( dpy, gc, p );
         XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_8A8B8G8R pixels to a pixmap.
 */
static void write_pixels_8A8B8G8R_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaSetForeground( dpy, gc,
                         PACK_8A8B8G8R( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP], rgba[i][ACOMP] ));
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_8R8G8B pixels to a pixmap.
 */
static void write_pixels_8R8G8B_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaSetForeground( dpy, gc, PACK_8R8G8B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_8R8G8B24 pixels to a pixmap.
 */
static void write_pixels_8R8G8B24_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaSetForeground( dpy, gc, PACK_8R8G8B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_5R6G5B pixels to a pixmap.
 */
static void write_pixels_5R6G5B_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaSetForeground( dpy, gc, PACK_5R6G5B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_DITHER_5R6G5B pixels to a pixmap.
 */
static void write_pixels_DITHER_5R6G5B_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUEDITHER(p, x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
	 XMesaSetForeground( dpy, gc, p );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_DITHER pixels to a pixmap.
 */
static void write_pixels_DITHER_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   DITHER_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaSetForeground( dpy, gc,
                         DITHER(x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]) );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_1BIT pixels to a pixmap.
 */
static void write_pixels_1BIT_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   SETUP_1BIT;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaSetForeground( dpy, gc,
                         DITHER_1BIT( x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ));
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_HPCR pixels to a pixmap.
 */
static void write_pixels_HPCR_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         XMesaSetForeground( dpy, gc,
                         DITHER_HPCR( x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ));
         XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_LOOKUP pixels to a pixmap.
 */
static void write_pixels_LOOKUP_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   LOOKUP_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         XMesaSetForeground( dpy, gc, LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
         XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_GRAYSCALE pixels to a pixmap.
 */
static void write_pixels_GRAYSCALE_pixmap( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         XMesaSetForeground( dpy, gc, GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
         XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_TRUECOLOR pixels to an ximage.
 */
static void write_pixels_TRUECOLOR_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUECOLOR( p, rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), p );
      }
   }
}


/*
 * Write an array of PF_TRUEDITHER pixels to an XImage.
 */
static void write_pixels_TRUEDITHER_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUEDITHER(p, x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), p );
      }
   }
}


/*
 * Write an array of PF_8A8B8G8R pixels to an ximage.
 */
static void write_pixels_8A8B8G8R_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x[i], y[i] );
         *ptr = PACK_8A8B8G8R( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP], rgba[i][ACOMP] );
      }
   }
}


/*
 * Write an array of PF_8R8G8B pixels to an ximage.
 */
static void write_pixels_8R8G8B_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x[i], y[i] );
         *ptr = PACK_8R8G8B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write an array of PF_8R8G8B24 pixels to an ximage.
 */
static void write_pixels_8R8G8B24_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 bgr_t *ptr = PIXELADDR3( xmesa->xm_buffer, x[i], y[i] );
         ptr->r = rgba[i][RCOMP];
         ptr->g = rgba[i][GCOMP];
         ptr->b = rgba[i][BCOMP];
      }
   }
}


/*
 * Write an array of PF_5R6G5B pixels to an ximage.
 */
static void write_pixels_5R6G5B_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x[i], y[i] );
         *ptr = PACK_5R6G5B( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write an array of PF_DITHER_5R6G5B pixels to an ximage.
 */
static void write_pixels_DITHER_5R6G5B_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x[i], y[i] );
         PACK_TRUEDITHER( *ptr, x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write an array of PF_DITHER pixels to an XImage.
 */
static void write_pixels_DITHER_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   DITHER_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]),
                    DITHER( x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
      }
   }
}


/*
 * Write an array of 8-bit PF_DITHER pixels to an XImage.
 */
static void write_pixels_DITHER8_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   DITHER_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x[i],y[i]);
	 *ptr = (GLubyte) DITHER( x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write an array of PF_1BIT pixels to an XImage.
 */
static void write_pixels_1BIT_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   SETUP_1BIT;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]),
                    DITHER_1BIT( x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ));
      }
   }
}


/*
 * Write an array of PF_HPCR pixels to an XImage.
 */
static void write_pixels_HPCR_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x[i],y[i]);
         *ptr = (GLubyte) DITHER_HPCR( x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write an array of PF_LOOKUP pixels to an XImage.
 */
static void write_pixels_LOOKUP_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   LOOKUP_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), LOOKUP(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]) );
      }
   }
}


/*
 * Write an array of 8-bit PF_LOOKUP pixels to an XImage.
 */
static void write_pixels_LOOKUP8_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   LOOKUP_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x[i],y[i]);
	 *ptr = (GLubyte) LOOKUP( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}


/*
 * Write an array of PF_GRAYSCALE pixels to an XImage.
 */
static void write_pixels_GRAYSCALE_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]),
                    GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] ) );
      }
   }
}


/*
 * Write an array of 8-bit PF_GRAYSCALE pixels to an XImage.
 */
static void write_pixels_GRAYSCALE8_ximage( RGBA_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer, x[i], y[i] );
	 *ptr = (GLubyte) GRAY_RGB( rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP] );
      }
   }
}




/**********************************************************************/
/*** Write MONO COLOR SPAN functions                                ***/
/**********************************************************************/

#define MONO_SPAN_ARGS	const GLcontext *ctx,	\
		 	GLuint n, GLint x, GLint y, const GLchan color[4], \
                        const GLubyte mask[]


/*
 * Write a span of identical pixels to a pixmap.
 */
static void write_span_mono_pixmap( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   const unsigned long pixel = xmesa_color_to_pixel(xmesa, color[RCOMP],
               color[GCOMP], color[BCOMP], color[ACOMP], xmesa->pixelformat);
   register GLuint i;
   XMesaSetForeground( xmesa->display, gc, pixel );
   y = FLIP(xmesa->xm_buffer, y);

   /* New code contributed by Jeff Epler and cleaned up by Keith
    * Whitwell.  
    */
   for (i = 0; i < n; ) {
      GLuint start = i;

      /* Identify and emit contiguous rendered pixels
       */
      while (i < n && (!mask || mask[i]))
	 i++;

      if (start < i) 
	 XMesaFillRectangle( dpy, buffer, gc,
			     (int)(x+start), (int) y,
			     (int)(i-start), 1);

      /* Eat up non-rendered pixels
       */
      while (i < n && !mask[i])
	 i++;
   }
}



static void write_span_mono_index_pixmap( const GLcontext *ctx, GLuint n,
                                          GLint x, GLint y, GLuint colorIndex,
                                          const GLubyte mask[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   XMesaSetForeground( xmesa->display, gc, colorIndex );
   y = FLIP(xmesa->xm_buffer, y);

   for (i = 0 ; i < n ;) {
      GLuint start = i;
      
      /* Identify and emit contiguous rendered pixels 
       */
      while (i < n && mask[i])
	 i++;

      if (start < i) 
	 XMesaFillRectangle( dpy, buffer, gc, 
			     (int)(x+start), (int) y, 
			     (int)(i-start), 1);

      /* Eat up non-rendered pixels
       */
      while (i < n && !mask[i])
	 i++;
   }
}



/*
 * Write a span of PF_TRUEDITHER pixels to a pixmap.
 */
static void write_span_mono_TRUEDITHER_pixmap( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLuint i;
   int yy = FLIP(xmesa->xm_buffer, y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUEDITHER(p, x, yy, r, g, b);
         XMesaSetForeground( dpy, gc, p );
         XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) yy );
      }
   }
}


/*
 * Write a span of PF_DITHER pixels to a pixmap.
 */
static void write_span_mono_DITHER_pixmap( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLuint i;
   int yy = FLIP(xmesa->xm_buffer, y);
   XDITHER_SETUP(yy);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         XMesaSetForeground( dpy, gc, XDITHER( x, r, g, b ) );
         XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) yy );
      }
   }
}


/*
 * Write a span of PF_1BIT pixels to a pixmap.
 */
static void write_span_mono_1BIT_pixmap( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLuint i;
   SETUP_1BIT;
   y = FLIP(xmesa->xm_buffer, y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         XMesaSetForeground( dpy, gc, DITHER_1BIT( x, y, r, g, b ) );
         XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
      }
   }
}


/*
 * Write a span of identical pixels to an XImage.
 */
static void write_span_mono_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   const unsigned long pixel = xmesa_color_to_pixel(xmesa, color[RCOMP],
               color[GCOMP], color[BCOMP], color[ACOMP], xmesa->pixelformat);
   y = FLIP(xmesa->xm_buffer, y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x, y, pixel );
      }
   }
}


static void write_span_mono_index_ximage( const GLcontext *ctx, GLuint n,
                                          GLint x, GLint y,
                                          GLuint colorIndex,
                                          const GLubyte mask[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x, y, colorIndex );
      }
   }
}


/*
 * Write a span of identical PF_TRUEDITHER pixels to an XImage.
 */
static void write_span_mono_TRUEDITHER_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   const GLint r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUEDITHER( p, x+i, y, r, g, b);
	 XMesaPutPixel( img, x+i, y, p );
      }
   }
}


/*
 * Write a span of identical 8A8B8G8R pixels to an XImage.
 */
static void write_span_mono_8A8B8G8R_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLuint i, *ptr;
   const unsigned long pixel = xmesa_color_to_pixel(xmesa, color[RCOMP],
               color[GCOMP], color[BCOMP], color[ACOMP], xmesa->pixelformat);
   ptr = PIXELADDR4( xmesa->xm_buffer, x, y );
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 ptr[i] = pixel;
      }
   }
}


/*
 * Write a span of identical 8R8G8B pixels to an XImage.
 */
static void write_span_mono_8R8G8B_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLuint pixel = PACK_8R8G8B(color[RCOMP], color[GCOMP], color[BCOMP]);
   GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x, y );
   GLuint i;
   for (i=0;i<n;i++) {
      if (!mask || mask[i]) {
	 ptr[i] = pixel;
      }
   }
}


/*
 * Write a span of identical 8R8G8B pixels to an XImage.
 */
static void write_span_mono_8R8G8B24_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP];
   const GLubyte g = color[GCOMP];
   const GLubyte b = color[BCOMP];
   GLuint i;
   bgr_t *ptr = PIXELADDR3( xmesa->xm_buffer, x, y );
   for (i=0;i<n;i++) {
      if (mask[i]) {
         ptr[i].r = r;
         ptr[i].g = g;
         ptr[i].b = b;
      }
   }
}


/*
 * Write a span of identical DITHER pixels to an XImage.
 */
static void write_span_mono_DITHER_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   XMesaImage *img = xmesa->xm_buffer->backimage;
   int yy = FLIP(xmesa->xm_buffer, y);
   register GLuint i;
   XDITHER_SETUP(yy);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x, yy, XDITHER( x, r, g, b ) );
      }
   }
}


/*
 * Write a span of identical 8-bit DITHER pixels to an XImage.
 */
static void write_span_mono_DITHER8_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x,y);
   register GLuint i;
   XDITHER_SETUP(y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
	 ptr[i] = (GLubyte) XDITHER( x, r, g, b );
      }
   }
}


/*
 * Write a span of identical 8-bit LOOKUP pixels to an XImage.
 */
static void write_span_mono_LOOKUP8_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x,y);
   GLubyte pixel;
   LOOKUP_SETUP;
   pixel = LOOKUP(color[RCOMP], color[GCOMP], color[BCOMP]);
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 ptr[i] = pixel;
      }
   }
}


/*
 * Write a span of identical PF_1BIT pixels to an XImage.
 */
static void write_span_mono_1BIT_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   SETUP_1BIT;
   y = FLIP(xmesa->xm_buffer, y);
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x, y, DITHER_1BIT( x, y, r, g, b ) );
      }
   }
}


/*
 * Write a span of identical HPCR pixels to an XImage.
 */
static void write_span_mono_HPCR_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x,y);
   register GLuint i;
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
         ptr[i] = DITHER_HPCR( x, y, r, g, b );
      }
   }
}


/*
 * Write a span of identical 8-bit GRAYSCALE pixels to an XImage.
 */
static void write_span_mono_GRAYSCALE8_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte p = GRAY_RGB(color[RCOMP], color[GCOMP], color[BCOMP]);
   GLubyte *ptr = (GLubyte *) PIXELADDR1( xmesa->xm_buffer,x,y);
   GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 ptr[i] = p;
      }
   }
}



/*
 * Write a span of identical PF_DITHER_5R6G5B pixels to an XImage.
 */
static void write_span_mono_DITHER_5R6G5B_ximage( MONO_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x, y );
   const GLint r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   for (i=0;i<n;i++) {
      if (mask[i]) {
         PACK_TRUEDITHER(ptr[i], x+i, y, r, g, b);
      }
   }
}



/**********************************************************************/
/*** Write MONO COLOR PIXELS functions                              ***/
/**********************************************************************/

#define MONO_PIXEL_ARGS	const GLcontext *ctx,				\
			GLuint n, const GLint x[], const GLint y[],	\
			const GLchan color[4], const GLubyte mask[]

/*
 * Write an array of identical pixels to a pixmap.
 */
static void write_pixels_mono_pixmap( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   const unsigned long pixel = xmesa_color_to_pixel(xmesa, color[RCOMP],
               color[GCOMP], color[BCOMP], color[ACOMP], xmesa->pixelformat);
   XMesaSetForeground( xmesa->display, gc, pixel );
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaDrawPoint( dpy, buffer, gc,
                         (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


static void write_pixels_mono_index_pixmap(const GLcontext *ctx,
                                           GLuint n,
                                           const GLint x[], const GLint y[],
                                           GLuint colorIndex,
                                           const GLubyte mask[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   XMesaSetForeground( xmesa->display, gc, colorIndex );
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaDrawPoint( dpy, buffer, gc,
                         (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_TRUEDITHER pixels to a pixmap.
 */
static void write_pixels_mono_TRUEDITHER_pixmap( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUEDITHER(p, x[i], y[i], r, g, b);
         XMesaSetForeground( dpy, gc, p );
	 XMesaDrawPoint( dpy, buffer, gc,
                         (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_DITHER pixels to a pixmap.
 */
static void write_pixels_mono_DITHER_pixmap( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   DITHER_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         XMesaSetForeground( dpy, gc, DITHER( x[i], y[i], r, g, b ) );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of PF_1BIT pixels to a pixmap.
 */
static void write_pixels_mono_1BIT_pixmap( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   SETUP_1BIT;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         XMesaSetForeground( dpy, gc, DITHER_1BIT( x[i], y[i], r, g, b ) );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of identical pixels to an XImage.
 */
static void write_pixels_mono_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   const unsigned long pixel = xmesa_color_to_pixel(xmesa, color[RCOMP],
               color[GCOMP], color[BCOMP], color[ACOMP], xmesa->pixelformat);
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), pixel );
      }
   }
}


static void write_pixels_mono_index_ximage( const GLcontext *ctx, GLuint n,
                                            const GLint x[], const GLint y[],
                                            GLuint colorIndex,
                                            const GLubyte mask[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), colorIndex );
      }
   }
}


/*
 * Write an array of identical TRUEDITHER pixels to an XImage.
 */
static void write_pixels_mono_TRUEDITHER_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   const int r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   for (i=0;i<n;i++) {
      if (mask[i]) {
         unsigned long p;
         PACK_TRUEDITHER(p, x[i], FLIP(xmesa->xm_buffer, y[i]), r, g, b);
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), p );
      }
   }
}



/*
 * Write an array of identical 8A8B8G8R pixels to an XImage
 */
static void write_pixels_mono_8A8B8G8R_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLuint p = PACK_8A8B8G8R(color[RCOMP], color[GCOMP],
                                  color[BCOMP], color[ACOMP]);
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x[i], y[i] );
	 *ptr = p;
      }
   }
}


/*
 * Write an array of identical 8R8G8B pixels to an XImage.
 */
static void write_pixels_mono_8R8G8B_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   const GLuint p = PACK_8R8G8B(color[RCOMP], color[GCOMP], color[BCOMP]);
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLuint *ptr = PIXELADDR4( xmesa->xm_buffer, x[i], y[i] );
	 *ptr = p;
      }
   }
}


/*
 * Write an array of identical 8R8G8B pixels to an XImage.
 */
static void write_pixels_mono_8R8G8B24_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 bgr_t *ptr = PIXELADDR3( xmesa->xm_buffer, x[i], y[i] );
         ptr->r = r;
         ptr->g = g;
         ptr->b = b;
      }
   }
}


/*
 * Write an array of identical PF_DITHER pixels to an XImage.
 */
static void write_pixels_mono_DITHER_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   DITHER_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), DITHER( x[i], y[i], r, g, b ) );
      }
   }
}


/*
 * Write an array of identical 8-bit PF_DITHER pixels to an XImage.
 */
static void write_pixels_mono_DITHER8_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLuint i;
   DITHER_SETUP;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x[i],y[i]);
	 *ptr = (GLubyte) DITHER( x[i], y[i], r, g, b );
      }
   }
}


/*
 * Write an array of identical 8-bit PF_LOOKUP pixels to an XImage.
 */
static void write_pixels_mono_LOOKUP8_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   GLubyte pixel;
   LOOKUP_SETUP;
   pixel = LOOKUP(color[RCOMP], color[GCOMP], color[BCOMP]);
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x[i],y[i]);
	 *ptr = pixel;
      }
   }
}



/*
 * Write an array of identical PF_1BIT pixels to an XImage.
 */
static void write_pixels_mono_1BIT_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   SETUP_1BIT;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]),
                        DITHER_1BIT( x[i], y[i], r, g, b ));
      }
   }
}


/*
 * Write an array of identical PF_HPCR pixels to an XImage.
 */
static void write_pixels_mono_HPCR_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x[i],y[i]);
         *ptr = DITHER_HPCR( x[i], y[i], r, g, b );
      }
   }
}


/*
 * Write an array of identical 8-bit PF_GRAYSCALE pixels to an XImage.
 */
static void write_pixels_mono_GRAYSCALE8_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   register GLubyte p = GRAY_RGB(color[RCOMP], color[GCOMP], color[BCOMP]);
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLubyte *ptr = PIXELADDR1( xmesa->xm_buffer,x[i],y[i]);
	 *ptr = p;
      }
   }
}


/*
 * Write an array of identical PF_DITHER_5R6G5B pixels to an XImage.
 */
static void write_pixels_mono_DITHER_5R6G5B_ximage( MONO_PIXEL_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const int r = color[RCOMP], g = color[GCOMP], b = color[BCOMP];
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 GLushort *ptr = PIXELADDR2( xmesa->xm_buffer, x[i], y[i] );
         PACK_TRUEDITHER(*ptr, x[i], y[i], r, g, b);
      }
   }
}



/**********************************************************************/
/*** Write INDEX SPAN functions                                     ***/
/**********************************************************************/

#define INDEX_SPAN_ARGS	const GLcontext *ctx,				\
			GLuint n, GLint x, GLint y, const GLuint index[], \
			const GLubyte mask[]

#define INDEX8_SPAN_ARGS const GLcontext *ctx,				\
			 GLuint n, GLint x, GLint y, const GLubyte index[], \
			 const GLubyte mask[]


/*
 * Write a span of CI pixels to a Pixmap.
 */
static void write_span_index_pixmap( INDEX_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, (unsigned long) index[i] );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      for (i=0;i<n;i++,x++) {
         XMesaSetForeground( dpy, gc, (unsigned long) index[i] );
         XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
      }
   }
}


/*
 * Write a span of 8-bit CI pixels to a Pixmap.
 */
static void write_span_index8_pixmap( INDEX8_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaSetForeground( dpy, gc, (unsigned long) index[i] );
            XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
         }
      }
   }
   else {
      for (i=0;i<n;i++,x++) {
         XMesaSetForeground( dpy, gc, (unsigned long) index[i] );
         XMesaDrawPoint( dpy, buffer, gc, (int) x, (int) y );
      }
   }
}


/*
 * Write a span of CI pixels to an XImage.
 */
static void write_span_index_ximage( INDEX_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   y = FLIP(xmesa->xm_buffer, y);
   if (mask) {
      for (i=0;i<n;i++,x++) {
         if (mask[i]) {
            XMesaPutPixel( img, x, y, (unsigned long) index[i] );
         }
      }
   }
   else {
      for (i=0;i<n;i++,x++) {
         XMesaPutPixel( img, x, y, (unsigned long) index[i] );
      }
   }
}


/*
 * Write a span of 8-bit CI pixels to a non 8-bit XImage.
 */
static void write_span_index8_ximage( INDEX8_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (mask) {
      GLuint i;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            XMesaPutPixel(xmesa->xm_buffer->backimage, x+i, y, index[i]);
         }
      }
   }
   else {
      GLuint i;
      for (i=0;i<n;i++) {
         XMesaPutPixel(xmesa->xm_buffer->backimage, x+i, y, index[i]);
      }
   }
}

/*
 * Write a span of 8-bit CI pixels to an 8-bit XImage.
 */
static void write_span_index8_ximage8( INDEX8_SPAN_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLubyte *dst = PIXELADDR1( xmesa->xm_buffer,x,y);
   if (mask) {
      GLuint i;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            dst[i] = index[i];
         }
      }
   }
   else {
      MEMCPY( dst, index, n );
   }
}



/**********************************************************************/
/*** Write INDEX PIXELS functions                                   ***/
/**********************************************************************/

#define INDEX_PIXELS_ARGS	const GLcontext *ctx,			\
				GLuint n, const GLint x[], const GLint y[], \
				const GLuint index[], const GLubyte mask[]


/*
 * Write an array of CI pixels to a Pixmap.
 */
static void write_pixels_index_pixmap( INDEX_PIXELS_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaDrawable buffer = xmesa->xm_buffer->buffer;
   XMesaGC gc = xmesa->xm_buffer->gc;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaSetForeground( dpy, gc, (unsigned long) index[i] );
	 XMesaDrawPoint( dpy, buffer, gc, (int) x[i], (int) FLIP(xmesa->xm_buffer, y[i]) );
      }
   }
}


/*
 * Write an array of CI pixels to an XImage.
 */
static void write_pixels_index_ximage( INDEX_PIXELS_ARGS )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_buffer->backimage;
   register GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
	 XMesaPutPixel( img, x[i], FLIP(xmesa->xm_buffer, y[i]), (unsigned long) index[i] );
      }
   }
}




/**********************************************************************/
/*****                      Pixel reading                         *****/
/**********************************************************************/



/*
 * Read a horizontal span of color-index pixels.
 */
static void read_index_span( const GLcontext *ctx,
			     GLuint n, GLint x, GLint y, GLuint index[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer source = xmesa->xm_buffer;
   GLuint i;

   y = FLIP(source, y);

   if (source->buffer) {
#ifndef XFree86Server
      XMesaImage *span = NULL;
      int error;
      catch_xgetimage_errors( xmesa->display );
      span = XGetImage( xmesa->display, source->buffer,
		        x, y, n, 1, AllPlanes, ZPixmap );
      error = check_xgetimage_errors();
      if (span && !error) {
	 for (i=0;i<n;i++) {
	    index[i] = (GLuint) XMesaGetPixel( span, i, 0 );
	 }
      }
      else {
	 /* return 0 pixels */
	 for (i=0;i<n;i++) {
	    index[i] = 0;
	 }
      }
      if (span) {
	 XMesaDestroyImage( span );
      }
#else
      (*xmesa->display->GetImage)(source->buffer,
				  x, y, n, 1, ZPixmap,
				  ~0L, (pointer)index);
#endif
   }
   else if (source->backimage) {
      XMesaImage *img = source->backimage;
      for (i=0;i<n;i++,x++) {
	 index[i] = (GLuint) XMesaGetPixel( img, x, y );
      }
   }
}



/*
 * Read a horizontal span of color pixels.
 */
static void read_color_span( const GLcontext *ctx,
			     GLuint n, GLint x, GLint y,
                             GLubyte rgba[][4] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer source = xmesa->xm_buffer;

   if (source->buffer) {
      /* Read from Pixmap or Window */
      XMesaImage *span = NULL;
      int error;
#ifdef XFree86Server
      span = XMesaCreateImage(xmesa->xm_visual->BitsPerPixel, n, 1, NULL);
      span->data = (char *)MALLOC(span->height * span->bytes_per_line);
      error = (!span->data);
      (*xmesa->display->GetImage)(source->buffer,
				  x, FLIP(source, y), n, 1, ZPixmap,
				  ~0L, (pointer)span->data);
#else
      catch_xgetimage_errors( xmesa->display );
      span = XGetImage( xmesa->display, source->buffer,
		        x, FLIP(source, y), n, 1, AllPlanes, ZPixmap );
      error = check_xgetimage_errors();
#endif
      if (span && !error) {
	 switch (xmesa->pixelformat) {
	    case PF_Truecolor:
	    case PF_Dither_True:
               {
                  const GLubyte *pixelToR = xmesa->xm_visual->PixelToR;
                  const GLubyte *pixelToG = xmesa->xm_visual->PixelToG;
                  const GLubyte *pixelToB = xmesa->xm_visual->PixelToB;
                  unsigned long rMask = GET_REDMASK(xmesa->xm_visual);
                  unsigned long gMask = GET_GREENMASK(xmesa->xm_visual);
                  unsigned long bMask = GET_BLUEMASK(xmesa->xm_visual);
                  GLint rShift = xmesa->xm_visual->rshift;
                  GLint gShift = xmesa->xm_visual->gshift;
                  GLint bShift = xmesa->xm_visual->bshift;
                  GLuint i;
                  for (i=0;i<n;i++) {
                     unsigned long p;
                     p = XMesaGetPixel( span, i, 0 );
                     rgba[i][RCOMP] = pixelToR[(p & rMask) >> rShift];
                     rgba[i][GCOMP] = pixelToG[(p & gMask) >> gShift];
                     rgba[i][BCOMP] = pixelToB[(p & bMask) >> bShift];
                     rgba[i][ACOMP] = 255;
                  }
               }
	       break;
            case PF_5R6G5B:
            case PF_Dither_5R6G5B:
               {
                  const GLubyte *pixelToR = xmesa->xm_visual->PixelToR;
                  const GLubyte *pixelToG = xmesa->xm_visual->PixelToG;
                  const GLubyte *pixelToB = xmesa->xm_visual->PixelToB;
                  GLuint i;
                  for (i=0;i<n;i++) {
                     unsigned long p = XMesaGetPixel( span, i, 0 );
                     /* fast, but not quite accurate
                     rgba[i][RCOMP] = ((p >> 8) & 0xf8);
                     rgba[i][GCOMP] = ((p >> 3) & 0xfc);
                     rgba[i][BCOMP] = ((p << 3) & 0xff);
                     */
                     rgba[i][RCOMP] = pixelToR[p >> 11];
                     rgba[i][GCOMP] = pixelToG[(p >> 5) & 0x3f];
                     rgba[i][BCOMP] = pixelToB[p & 0x1f];
                     rgba[i][ACOMP] = 255;
                  }
               }
	       break;
	    case PF_8A8B8G8R:
               {
                  const GLuint *ptr4 = (GLuint *) span->data;
                  GLuint i;
                  for (i=0;i<n;i++) {
                     GLuint p4 = *ptr4++;
                     rgba[i][RCOMP] = (GLubyte) ( p4        & 0xff);
                     rgba[i][GCOMP] = (GLubyte) ((p4 >> 8)  & 0xff);
                     rgba[i][BCOMP] = (GLubyte) ((p4 >> 16) & 0xff);
                     rgba[i][ACOMP] = (GLubyte) ((p4 >> 24) & 0xff);
                  }
	       }
	       break;
            case PF_8R8G8B:
               {
                  const GLuint *ptr4 = (GLuint *) span->data;
                  GLuint i;
                  for (i=0;i<n;i++) {
                     GLuint p4 = *ptr4++;
                     rgba[i][RCOMP] = (GLubyte) ((p4 >> 16) & 0xff);
                     rgba[i][GCOMP] = (GLubyte) ((p4 >> 8)  & 0xff);
                     rgba[i][BCOMP] = (GLubyte) ( p4        & 0xff);
                     rgba[i][ACOMP] = 255;
                  }
	       }
	       break;
            case PF_8R8G8B24:
               {
                  const bgr_t *ptr3 = (bgr_t *) span->data;
                  GLuint i;
                  for (i=0;i<n;i++) {
                     rgba[i][RCOMP] = ptr3[i].r;
                     rgba[i][GCOMP] = ptr3[i].g;
                     rgba[i][BCOMP] = ptr3[i].b;
                     rgba[i][ACOMP] = 255;
                  }
	       }
	       break;
            case PF_HPCR:
               {
                  GLubyte *ptr1 = (GLubyte *) span->data;
                  GLuint i;
                  for (i=0;i<n;i++) {
                     GLubyte p = *ptr1++;
                     rgba[i][RCOMP] =  p & 0xE0;
                     rgba[i][GCOMP] = (p & 0x1C) << 3;
                     rgba[i][BCOMP] = (p & 0x03) << 6;
                     rgba[i][ACOMP] = 255;
                  }
               }
               break;
	    case PF_Dither:
	    case PF_Lookup:
	    case PF_Grayscale:
               {
                  GLubyte *rTable = source->pixel_to_r;
                  GLubyte *gTable = source->pixel_to_g;
                  GLubyte *bTable = source->pixel_to_b;
                  if (GET_VISUAL_DEPTH(xmesa->xm_visual)==8) {
                     const GLubyte *ptr1 = (GLubyte *) span->data;
                     GLuint i;
                     for (i=0;i<n;i++) {
                        unsigned long p = *ptr1++;
                        rgba[i][RCOMP] = rTable[p];
                        rgba[i][GCOMP] = gTable[p];
                        rgba[i][BCOMP] = bTable[p];
                        rgba[i][ACOMP] = 255;
                     }
                  }
                  else {
                     GLuint i;
                     for (i=0;i<n;i++) {
                        unsigned long p = XMesaGetPixel( span, i, 0 );
                        rgba[i][RCOMP] = rTable[p];
                        rgba[i][GCOMP] = gTable[p];
                        rgba[i][BCOMP] = bTable[p];
                        rgba[i][ACOMP] = 255;
                     }
                  }
               }
	       break;
	    case PF_1Bit:
               {
                  int bitFlip = xmesa->xm_visual->bitFlip;
                  GLuint i;
                  for (i=0;i<n;i++) {
                     unsigned long p;
                     p = XMesaGetPixel( span, i, 0 ) ^ bitFlip;
                     rgba[i][RCOMP] = (GLubyte) (p * 255);
                     rgba[i][GCOMP] = (GLubyte) (p * 255);
                     rgba[i][BCOMP] = (GLubyte) (p * 255);
                     rgba[i][ACOMP] = 255;
                  }
               }
	       break;
	    default:
	       _mesa_problem(NULL,"Problem in DD.read_color_span (1)");
               return;
	 }
      }
      else {
	 /* return black pixels */
         GLuint i;
	 for (i=0;i<n;i++) {
	    rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = rgba[i][ACOMP] = 0;
	 }
      }
      if (span) {
	 XMesaDestroyImage( span );
      }
   }
   else if (source->backimage) {
      /* Read from XImage back buffer */
      switch (xmesa->pixelformat) {
         case PF_Truecolor:
         case PF_Dither_True:
            {
               const GLubyte *pixelToR = xmesa->xm_visual->PixelToR;
               const GLubyte *pixelToG = xmesa->xm_visual->PixelToG;
               const GLubyte *pixelToB = xmesa->xm_visual->PixelToB;
               unsigned long rMask = GET_REDMASK(xmesa->xm_visual);
               unsigned long gMask = GET_GREENMASK(xmesa->xm_visual);
               unsigned long bMask = GET_BLUEMASK(xmesa->xm_visual);
               GLint rShift = xmesa->xm_visual->rshift;
               GLint gShift = xmesa->xm_visual->gshift;
               GLint bShift = xmesa->xm_visual->bshift;
               XMesaImage *img = source->backimage;
               GLuint i;
               y = FLIP(source, y);
               for (i=0;i<n;i++) {
                  unsigned long p;
		  p = XMesaGetPixel( img, x+i, y );
                  rgba[i][RCOMP] = pixelToR[(p & rMask) >> rShift];
                  rgba[i][GCOMP] = pixelToG[(p & gMask) >> gShift];
                  rgba[i][BCOMP] = pixelToB[(p & bMask) >> bShift];
                  rgba[i][ACOMP] = 255;
               }
            }
            break;
         case PF_5R6G5B:
         case PF_Dither_5R6G5B:
            {
               const GLubyte *pixelToR = xmesa->xm_visual->PixelToR;
               const GLubyte *pixelToG = xmesa->xm_visual->PixelToG;
               const GLubyte *pixelToB = xmesa->xm_visual->PixelToB;
               const GLushort *ptr2 = PIXELADDR2( source, x, y );
               GLuint i;
#if defined(__i386__) /* word stores don't have to be on 4-byte boundaries */
               const GLuint *ptr4 = (const GLuint *) ptr2;
               GLuint extraPixel = (n & 1);
               n -= extraPixel;
               for (i = 0; i < n; i += 2) {
                  const GLuint p = *ptr4++;
                  const GLuint p0 = p & 0xffff;
                  const GLuint p1 = p >> 16;
                  /* fast, but not quite accurate
                  rgba[i][RCOMP] = ((p >> 8) & 0xf8);
                  rgba[i][GCOMP] = ((p >> 3) & 0xfc);
                  rgba[i][BCOMP] = ((p << 3) & 0xff);
                  */
                  rgba[i][RCOMP] = pixelToR[p0 >> 11];
                  rgba[i][GCOMP] = pixelToG[(p0 >> 5) & 0x3f];
                  rgba[i][BCOMP] = pixelToB[p0 & 0x1f];
                  rgba[i][ACOMP] = 255;
                  rgba[i+1][RCOMP] = pixelToR[p1 >> 11];
                  rgba[i+1][GCOMP] = pixelToG[(p1 >> 5) & 0x3f];
                  rgba[i+1][BCOMP] = pixelToB[p1 & 0x1f];
                  rgba[i+1][ACOMP] = 255;
               }
               if (extraPixel) {
                  GLushort p = ptr2[n];
                  rgba[n][RCOMP] = pixelToR[p >> 11];
                  rgba[n][GCOMP] = pixelToG[(p >> 5) & 0x3f];
                  rgba[n][BCOMP] = pixelToB[p & 0x1f];
                  rgba[n][ACOMP] = 255;
               }
#else
               for (i = 0; i < n; i++) {
                  const GLushort p = ptr2[i];
                  rgba[i][RCOMP] = pixelToR[p >> 11];
                  rgba[i][GCOMP] = pixelToG[(p >> 5) & 0x3f];
                  rgba[i][BCOMP] = pixelToB[p & 0x1f];
                  rgba[i][ACOMP] = 255;
               }
#endif
            }
            break;
	 case PF_8A8B8G8R:
            {
               const GLuint *ptr4 = PIXELADDR4( source, x, y );
               GLuint i;
               for (i=0;i<n;i++) {
                  GLuint p4 = *ptr4++;
                  rgba[i][RCOMP] = (GLubyte) ( p4        & 0xff);
                  rgba[i][GCOMP] = (GLubyte) ((p4 >> 8)  & 0xff);
                  rgba[i][BCOMP] = (GLubyte) ((p4 >> 16) & 0xff);
                  rgba[i][ACOMP] = (GLint)   ((p4 >> 24) & 0xff);
               }
            }
	    break;
	 case PF_8R8G8B:
            {
               const GLuint *ptr4 = PIXELADDR4( source, x, y );
               GLuint i;
               for (i=0;i<n;i++) {
                  GLuint p4 = *ptr4++;
                  rgba[i][RCOMP] = (GLubyte) ((p4 >> 16) & 0xff);
                  rgba[i][GCOMP] = (GLubyte) ((p4 >> 8)  & 0xff);
                  rgba[i][BCOMP] = (GLubyte) ( p4        & 0xff);
                  rgba[i][ACOMP] = 255;
               }
            }
	    break;
	 case PF_8R8G8B24:
            {
               const bgr_t *ptr3 = PIXELADDR3( source, x, y );
               GLuint i;
               for (i=0;i<n;i++) {
                  rgba[i][RCOMP] = ptr3[i].r;
                  rgba[i][GCOMP] = ptr3[i].g;
                  rgba[i][BCOMP] = ptr3[i].b;
                  rgba[i][ACOMP] = 255;
               }
            }
	    break;
         case PF_HPCR:
            {
               const GLubyte *ptr1 = PIXELADDR1( source, x, y );
               GLuint i;
               for (i=0;i<n;i++) {
                  GLubyte p = *ptr1++;
                  rgba[i][RCOMP] =  p & 0xE0;
                  rgba[i][GCOMP] = (p & 0x1C) << 3;
                  rgba[i][BCOMP] = (p & 0x03) << 6;
                  rgba[i][ACOMP] = 255;
               }
            }
            break;
	 case PF_Dither:
	 case PF_Lookup:
	 case PF_Grayscale:
            {
               const GLubyte *rTable = source->pixel_to_r;
               const GLubyte *gTable = source->pixel_to_g;
               const GLubyte *bTable = source->pixel_to_b;
               if (GET_VISUAL_DEPTH(xmesa->xm_visual)==8) {
                  GLubyte *ptr1 = PIXELADDR1( source, x, y );
                  GLuint i;
                  for (i=0;i<n;i++) {
                     unsigned long p = *ptr1++;
                     rgba[i][RCOMP] = rTable[p];
                     rgba[i][GCOMP] = gTable[p];
                     rgba[i][BCOMP] = bTable[p];
                     rgba[i][ACOMP] = 255;
                  }
               }
               else {
                  XMesaImage *img = source->backimage;
                  GLuint i;
                  y = FLIP(source, y);
                  for (i=0;i<n;i++,x++) {
                     unsigned long p = XMesaGetPixel( img, x, y );
                     rgba[i][RCOMP] = rTable[p];
                     rgba[i][GCOMP] = gTable[p];
                     rgba[i][BCOMP] = bTable[p];
                     rgba[i][ACOMP] = 255;
                  }
               }
            }
	    break;
	 case PF_1Bit:
            {
               XMesaImage *img = source->backimage;
               int bitFlip = xmesa->xm_visual->bitFlip;
               GLuint i;
               y = FLIP(source, y);
               for (i=0;i<n;i++,x++) {
                  unsigned long p;
		  p = XMesaGetPixel( img, x, y ) ^ bitFlip;
                  rgba[i][RCOMP] = (GLubyte) (p * 255);
                  rgba[i][GCOMP] = (GLubyte) (p * 255);
                  rgba[i][BCOMP] = (GLubyte) (p * 255);
                  rgba[i][ACOMP] = 255;
               }
	    }
	    break;
	 default:
	    _mesa_problem(NULL,"Problem in DD.read_color_span (2)");
            return;
      }
   }
}



/*
 * Read an array of color index pixels.
 */
static void read_index_pixels( const GLcontext *ctx,
			       GLuint n, const GLint x[], const GLint y[],
                               GLuint indx[], const GLubyte mask[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint i;
   XMesaBuffer source = xmesa->xm_buffer;

   if (source->buffer) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            indx[i] = (GLuint) read_pixel( xmesa->display,
                                           source->buffer,
                                           x[i], FLIP(source, y[i]) );
         }
      }
   }
   else if (source->backimage) {
      XMesaImage *img = source->backimage;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            indx[i] = (GLuint) XMesaGetPixel( img, x[i], FLIP(source, y[i]) );
         }
      }
   }
}



static void read_color_pixels( const GLcontext *ctx,
			       GLuint n, const GLint x[], const GLint y[],
                               GLubyte rgba[][4], const GLubyte mask[] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   register GLuint i;
   XMesaBuffer source = xmesa->xm_buffer;
   XMesaDrawable buffer = source->buffer;  /* the X drawable */

   if (source->buffer) {
      switch (xmesa->pixelformat) {
	 case PF_Truecolor:
         case PF_Dither_True:
         case PF_5R6G5B:
         case PF_Dither_5R6G5B:
            {
               unsigned long rMask = GET_REDMASK(xmesa->xm_visual);
               unsigned long gMask = GET_GREENMASK(xmesa->xm_visual);
               unsigned long bMask = GET_BLUEMASK(xmesa->xm_visual);
               GLubyte *pixelToR = xmesa->xm_visual->PixelToR;
               GLubyte *pixelToG = xmesa->xm_visual->PixelToG;
               GLubyte *pixelToB = xmesa->xm_visual->PixelToB;
               GLint rShift = xmesa->xm_visual->rshift;
               GLint gShift = xmesa->xm_visual->gshift;
               GLint bShift = xmesa->xm_visual->bshift;
               for (i=0;i<n;i++) {
                  if (mask[i]) {
                     unsigned long p = read_pixel( dpy, buffer,
                                                   x[i], FLIP(source, y[i]) );
                     rgba[i][RCOMP] = pixelToR[(p & rMask) >> rShift];
                     rgba[i][GCOMP] = pixelToG[(p & gMask) >> gShift];
                     rgba[i][BCOMP] = pixelToB[(p & bMask) >> bShift];
                     rgba[i][ACOMP] = 255;
                  }
               }
            }
            break;
	 case PF_8A8B8G8R:
	    for (i=0;i<n;i++) {
               if (mask[i]) {
                  unsigned long p = read_pixel( dpy, buffer,
                                                x[i], FLIP(source, y[i]) );
                  rgba[i][RCOMP] = (GLubyte) ( p        & 0xff);
                  rgba[i][GCOMP] = (GLubyte) ((p >> 8)  & 0xff);
                  rgba[i][BCOMP] = (GLubyte) ((p >> 16) & 0xff);
                  rgba[i][ACOMP] = (GLubyte) ((p >> 24) & 0xff);
               }
	    }
	    break;
	 case PF_8R8G8B:
	    for (i=0;i<n;i++) {
               if (mask[i]) {
                  unsigned long p = read_pixel( dpy, buffer,
                                                x[i], FLIP(source, y[i]) );
                  rgba[i][RCOMP] = (GLubyte) ((p >> 16) & 0xff);
                  rgba[i][GCOMP] = (GLubyte) ((p >> 8)  & 0xff);
                  rgba[i][BCOMP] = (GLubyte) ( p        & 0xff);
                  rgba[i][ACOMP] = 255;
               }
	    }
	    break;
	 case PF_8R8G8B24:
	    for (i=0;i<n;i++) {
               if (mask[i]) {
                  unsigned long p = read_pixel( dpy, buffer,
                                                x[i], FLIP(source, y[i]) );
                  rgba[i][RCOMP] = (GLubyte) ((p >> 16) & 0xff);
                  rgba[i][GCOMP] = (GLubyte) ((p >> 8)  & 0xff);
                  rgba[i][BCOMP] = (GLubyte) ( p        & 0xff);
                  rgba[i][ACOMP] = 255;
               }
	    }
	    break;
         case PF_HPCR:
            {
               for (i=0;i<n;i++) {
                  if (mask[i]) {
                     unsigned long p = read_pixel( dpy, buffer,
                                                   x[i], FLIP(source, y[i]) );
                     rgba[i][RCOMP] = (GLubyte) ( p & 0xE0      );
                     rgba[i][GCOMP] = (GLubyte) ((p & 0x1C) << 3);
                     rgba[i][BCOMP] = (GLubyte) ((p & 0x03) << 6);
                     rgba[i][ACOMP] = (GLubyte) 255;
                  }
               }
            }
            break;
	 case PF_Dither:
	 case PF_Lookup:
	 case PF_Grayscale:
            {
               GLubyte *rTable = source->pixel_to_r;
               GLubyte *gTable = source->pixel_to_g;
               GLubyte *bTable = source->pixel_to_b;
               for (i=0;i<n;i++) {
                  if (mask[i]) {
                     unsigned long p = read_pixel( dpy, buffer,
                                                   x[i], FLIP(source, y[i]) );
                     rgba[i][RCOMP] = rTable[p];
                     rgba[i][GCOMP] = gTable[p];
                     rgba[i][BCOMP] = bTable[p];
                     rgba[i][ACOMP] = 255;
                  }
               }
	    }
	    break;
	 case PF_1Bit:
            {
               int bitFlip = xmesa->xm_visual->bitFlip;
               for (i=0;i<n;i++) {
                  if (mask[i]) {
                     unsigned long p = read_pixel( dpy, buffer,
                                     x[i], FLIP(source, y[i])) ^ bitFlip;
                     rgba[i][RCOMP] = (GLubyte) (p * 255);
                     rgba[i][GCOMP] = (GLubyte) (p * 255);
                     rgba[i][BCOMP] = (GLubyte) (p * 255);
                     rgba[i][ACOMP] = 255;
                  }
               }
	    }
	    break;
	 default:
	    _mesa_problem(NULL,"Problem in DD.read_color_pixels (1)");
            return;
      }
   }
   else if (source->backimage) {
      switch (xmesa->pixelformat) {
	 case PF_Truecolor:
         case PF_Dither_True:
         case PF_5R6G5B:
         case PF_Dither_5R6G5B:
            {
               unsigned long rMask = GET_REDMASK(xmesa->xm_visual);
               unsigned long gMask = GET_GREENMASK(xmesa->xm_visual);
               unsigned long bMask = GET_BLUEMASK(xmesa->xm_visual);
               GLubyte *pixelToR = xmesa->xm_visual->PixelToR;
               GLubyte *pixelToG = xmesa->xm_visual->PixelToG;
               GLubyte *pixelToB = xmesa->xm_visual->PixelToB;
               GLint rShift = xmesa->xm_visual->rshift;
               GLint gShift = xmesa->xm_visual->gshift;
               GLint bShift = xmesa->xm_visual->bshift;
               XMesaImage *img = source->backimage;
               for (i=0;i<n;i++) {
                  if (mask[i]) {
                     unsigned long p;
                     p = XMesaGetPixel( img, x[i], FLIP(source, y[i]) );
                     rgba[i][RCOMP] = pixelToR[(p & rMask) >> rShift];
                     rgba[i][GCOMP] = pixelToG[(p & gMask) >> gShift];
                     rgba[i][BCOMP] = pixelToB[(p & bMask) >> bShift];
                     rgba[i][ACOMP] = 255;
                  }
               }
            }
            break;
	 case PF_8A8B8G8R:
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLuint *ptr4 = PIXELADDR4( source, x[i], y[i] );
                  GLuint p4 = *ptr4;
                  rgba[i][RCOMP] = (GLubyte) ( p4        & 0xff);
                  rgba[i][GCOMP] = (GLubyte) ((p4 >> 8)  & 0xff);
                  rgba[i][BCOMP] = (GLubyte) ((p4 >> 16) & 0xff);
                  rgba[i][ACOMP] = (GLubyte) ((p4 >> 24) & 0xff);
               }
	    }
	    break;
	 case PF_8R8G8B:
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  GLuint *ptr4 = PIXELADDR4( source, x[i], y[i] );
                  GLuint p4 = *ptr4;
                  rgba[i][RCOMP] = (GLubyte) ((p4 >> 16) & 0xff);
                  rgba[i][GCOMP] = (GLubyte) ((p4 >> 8)  & 0xff);
                  rgba[i][BCOMP] = (GLubyte) ( p4        & 0xff);
                  rgba[i][ACOMP] = 255;
               }
	    }
	    break;
	 case PF_8R8G8B24:
	    for (i=0;i<n;i++) {
	       if (mask[i]) {
                  bgr_t *ptr3 = PIXELADDR3( source, x[i], y[i] );
                  rgba[i][RCOMP] = ptr3->r;
                  rgba[i][GCOMP] = ptr3->g;
                  rgba[i][BCOMP] = ptr3->b;
                  rgba[i][ACOMP] = 255;
               }
	    }
	    break;
         case PF_HPCR:
            for (i=0;i<n;i++) {
               if (mask[i]) {
                  GLubyte *ptr1 = PIXELADDR1( source, x[i], y[i] );
                  GLubyte p = *ptr1;
                  rgba[i][RCOMP] =  p & 0xE0;
                  rgba[i][GCOMP] = (p & 0x1C) << 3;
                  rgba[i][BCOMP] = (p & 0x03) << 6;
                  rgba[i][ACOMP] = 255;
               }
            }
            break;
	 case PF_Dither:
	 case PF_Lookup:
	 case PF_Grayscale:
            {
               GLubyte *rTable = source->pixel_to_r;
               GLubyte *gTable = source->pixel_to_g;
               GLubyte *bTable = source->pixel_to_b;
               XMesaImage *img = source->backimage;
               for (i=0;i<n;i++) {
                  if (mask[i]) {
                     unsigned long p;
                     p = XMesaGetPixel( img, x[i], FLIP(source, y[i]) );
                     rgba[i][RCOMP] = rTable[p];
                     rgba[i][GCOMP] = gTable[p];
                     rgba[i][BCOMP] = bTable[p];
                     rgba[i][ACOMP] = 255;
                  }
               }
	    }
	    break;
	 case PF_1Bit:
            {
               XMesaImage *img = source->backimage;
               int bitFlip = xmesa->xm_visual->bitFlip;
               for (i=0;i<n;i++) {
                  if (mask[i]) {
                     unsigned long p;
                     p = XMesaGetPixel( img, x[i], FLIP(source, y[i]) ) ^ bitFlip;
                     rgba[i][RCOMP] = (GLubyte) (p * 255);
                     rgba[i][GCOMP] = (GLubyte) (p * 255);
                     rgba[i][BCOMP] = (GLubyte) (p * 255);
                     rgba[i][ACOMP] = 255;
                  }
               }
	    }
	    break;
	 default:
	    _mesa_problem(NULL,"Problem in DD.read_color_pixels (1)");
            return;
      }
   }
}


static void
clear_color_HPCR_ximage( GLcontext *ctx, const GLfloat color[4] )
{
   int i;
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[3], color[3]);

   if (color[0] == 0.0 && color[1] == 0.0 && color[2] == 0.0) {
      /* black is black */
      MEMSET( xmesa->xm_visual->hpcr_clear_ximage_pattern, 0x0 ,
              sizeof(xmesa->xm_visual->hpcr_clear_ximage_pattern));
   }
   else {
      /* build clear pattern */
      for (i=0; i<16; i++) {
         xmesa->xm_visual->hpcr_clear_ximage_pattern[0][i] =
            DITHER_HPCR(i, 0,
                        xmesa->clearcolor[0],
                        xmesa->clearcolor[1],
                        xmesa->clearcolor[2]);
         xmesa->xm_visual->hpcr_clear_ximage_pattern[1][i]    =
            DITHER_HPCR(i, 1,
                        xmesa->clearcolor[0],
                        xmesa->clearcolor[1],
                        xmesa->clearcolor[2]);
      }
   }
}


static void
clear_color_HPCR_pixmap( GLcontext *ctx, const GLfloat color[4] )
{
   int i;
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[3], color[3]);

   if (color[0] == 0.0 && color[1] == 0.0 && color[2] == 0.0) {
      /* black is black */
      for (i=0; i<16; i++) {
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 0, 0);
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 1, 0);
      }
   }
   else {
      for (i=0; i<16; i++) {
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 0,
                       DITHER_HPCR(i, 0,
                                   xmesa->clearcolor[0],
                                   xmesa->clearcolor[1],
                                   xmesa->clearcolor[2]));
         XMesaPutPixel(xmesa->xm_visual->hpcr_clear_ximage, i, 1,
                       DITHER_HPCR(i, 1,
                                   xmesa->clearcolor[0],
                                   xmesa->clearcolor[1],
                                   xmesa->clearcolor[2]));
      }
   }
   /* change tile pixmap content */
   XMesaPutImage(xmesa->display,
		 (XMesaDrawable)xmesa->xm_visual->hpcr_clear_pixmap,
		 xmesa->xm_buffer->cleargc,
		 xmesa->xm_visual->hpcr_clear_ximage, 0, 0, 0, 0, 16, 2);
}



void xmesa_update_span_funcs( GLcontext *ctx )
{
   XMesaContext xmesa = XMESA_CONTEXT(ctx);
   int depth=GET_VISUAL_DEPTH(xmesa->xm_visual);
   struct swrast_device_driver *dd = _swrast_GetDeviceDriverReference( ctx );

   /*
    * These drawing functions depend on color buffer config:
    */
   if (xmesa->xm_buffer->buffer!=XIMAGE) {
      /* Writing to window or back pixmap */
      switch (xmesa->pixelformat) {
	 case PF_Index:
	    dd->WriteCI32Span     = write_span_index_pixmap;
	    dd->WriteCI8Span      = write_span_index8_pixmap;
	    dd->WriteMonoCISpan   = write_span_mono_index_pixmap;
	    dd->WriteCI32Pixels   = write_pixels_index_pixmap;
	    dd->WriteMonoCIPixels = write_pixels_mono_index_pixmap;
	    break;
	 case PF_Truecolor:
	    dd->WriteRGBASpan       = write_span_TRUECOLOR_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_TRUECOLOR_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_TRUECOLOR_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
	    break;
	 case PF_Dither_True:
	    dd->WriteRGBASpan       = write_span_TRUEDITHER_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_TRUEDITHER_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_TRUEDITHER_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_TRUEDITHER_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_TRUEDITHER_pixmap;
	    break;
	 case PF_8A8B8G8R:
	    dd->WriteRGBASpan       = write_span_8A8B8G8R_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_8A8B8G8R_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_8A8B8G8R_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
	    break;
	 case PF_8R8G8B:
	    dd->WriteRGBASpan       = write_span_8R8G8B_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_8R8G8B_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_8R8G8B_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
	    break;
	 case PF_8R8G8B24:
	    dd->WriteRGBASpan       = write_span_8R8G8B24_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_8R8G8B24_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_8R8G8B24_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
	    break;
	 case PF_5R6G5B:
	    dd->WriteRGBASpan       = write_span_5R6G5B_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_5R6G5B_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_5R6G5B_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
	    break;
	 case PF_Dither_5R6G5B:
	    dd->WriteRGBASpan       = write_span_DITHER_5R6G5B_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_DITHER_5R6G5B_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_TRUEDITHER_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_DITHER_5R6G5B_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_TRUEDITHER_pixmap;
	    break;
	 case PF_Dither:
	    dd->WriteRGBASpan       = write_span_DITHER_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_DITHER_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_DITHER_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_DITHER_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_DITHER_pixmap;
	    break;
	 case PF_1Bit:
	    dd->WriteRGBASpan       = write_span_1BIT_pixmap;
	    dd->WriteRGBSpan        = write_span_rgb_1BIT_pixmap;
	    dd->WriteMonoRGBASpan   = write_span_mono_1BIT_pixmap;
	    dd->WriteRGBAPixels     = write_pixels_1BIT_pixmap;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_1BIT_pixmap;
	    break;
         case PF_HPCR:
            dd->WriteRGBASpan       = write_span_HPCR_pixmap;
            dd->WriteRGBSpan        = write_span_rgb_HPCR_pixmap;
            dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
            dd->WriteRGBAPixels     = write_pixels_HPCR_pixmap;
            dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
	    if (xmesa->xm_visual->hpcr_clear_flag) {
	       ctx->Driver.ClearColor = clear_color_HPCR_pixmap;
	    }
            break;
         case PF_Lookup:
            dd->WriteRGBASpan       = write_span_LOOKUP_pixmap;
            dd->WriteRGBSpan        = write_span_rgb_LOOKUP_pixmap;
            dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
            dd->WriteRGBAPixels     = write_pixels_LOOKUP_pixmap;
            dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
            break;
         case PF_Grayscale:
            dd->WriteRGBASpan       = write_span_GRAYSCALE_pixmap;
            dd->WriteRGBSpan        = write_span_rgb_GRAYSCALE_pixmap;
            dd->WriteMonoRGBASpan   = write_span_mono_pixmap;
            dd->WriteRGBAPixels     = write_pixels_GRAYSCALE_pixmap;
            dd->WriteMonoRGBAPixels = write_pixels_mono_pixmap;
            break;
	 default:
	    _mesa_problem(NULL,"Bad pixel format in xmesa_update_state (1)");
            return;
      }
   }
   else if (xmesa->xm_buffer->buffer==XIMAGE) {
      /* Writing to back XImage */
      switch (xmesa->pixelformat) {
	 case PF_Index:
	    dd->WriteCI32Span     = write_span_index_ximage;
	    if (depth==8)
               dd->WriteCI8Span   = write_span_index8_ximage8;
            else
               dd->WriteCI8Span   = write_span_index8_ximage;
	    dd->WriteMonoCISpan   = write_span_mono_index_ximage;
	    dd->WriteCI32Pixels   = write_pixels_index_ximage;
	    dd->WriteMonoCIPixels = write_pixels_mono_index_ximage;
	    break;
	 case PF_Truecolor:
	    /* Generic RGB */
	    dd->WriteRGBASpan       = write_span_TRUECOLOR_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_TRUECOLOR_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_ximage;
	    dd->WriteRGBAPixels     = write_pixels_TRUECOLOR_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_ximage;
	    break;
	 case PF_Dither_True:
	    dd->WriteRGBASpan       = write_span_TRUEDITHER_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_TRUEDITHER_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_TRUEDITHER_ximage;
	    dd->WriteRGBAPixels     = write_pixels_TRUEDITHER_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_TRUEDITHER_ximage;
	    break;
	 case PF_8A8B8G8R:
	    dd->WriteRGBASpan       = write_span_8A8B8G8R_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_8A8B8G8R_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_8A8B8G8R_ximage;
	    dd->WriteRGBAPixels     = write_pixels_8A8B8G8R_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_8A8B8G8R_ximage;
	    break;
	 case PF_8R8G8B:
	    dd->WriteRGBASpan       = write_span_8R8G8B_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_8R8G8B_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_8R8G8B_ximage;
	    dd->WriteRGBAPixels     = write_pixels_8R8G8B_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_8R8G8B_ximage;
	    break;
	 case PF_8R8G8B24:
	    dd->WriteRGBASpan       = write_span_8R8G8B24_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_8R8G8B24_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_8R8G8B24_ximage;
	    dd->WriteRGBAPixels     = write_pixels_8R8G8B24_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_8R8G8B24_ximage;
	    break;
	 case PF_5R6G5B:
	    dd->WriteRGBASpan       = write_span_5R6G5B_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_5R6G5B_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_ximage;
	    dd->WriteRGBAPixels     = write_pixels_5R6G5B_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_ximage;
	    break;
	 case PF_Dither_5R6G5B:
	    dd->WriteRGBASpan       = write_span_DITHER_5R6G5B_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_DITHER_5R6G5B_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_DITHER_5R6G5B_ximage;
	    dd->WriteRGBAPixels     = write_pixels_DITHER_5R6G5B_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_DITHER_5R6G5B_ximage;
	    break;
	 case PF_Dither:
	    if (depth==8) {
	       dd->WriteRGBASpan      = write_span_DITHER8_ximage;
	       dd->WriteRGBSpan       = write_span_rgb_DITHER8_ximage;
	       dd->WriteMonoRGBASpan  = write_span_mono_DITHER8_ximage;
	       dd->WriteRGBAPixels    = write_pixels_DITHER8_ximage;
	       dd->WriteMonoRGBAPixels = write_pixels_mono_DITHER8_ximage;
	    }
	    else {
	       dd->WriteRGBASpan       = write_span_DITHER_ximage;
	       dd->WriteRGBSpan        = write_span_rgb_DITHER_ximage;
	       dd->WriteMonoRGBASpan   = write_span_mono_DITHER_ximage;
	       dd->WriteRGBAPixels     = write_pixels_DITHER_ximage;
	       dd->WriteMonoRGBAPixels = write_pixels_mono_DITHER_ximage;
	    }
	    break;
	 case PF_1Bit:
	    dd->WriteRGBASpan       = write_span_1BIT_ximage;
	    dd->WriteRGBSpan        = write_span_rgb_1BIT_ximage;
	    dd->WriteMonoRGBASpan   = write_span_mono_1BIT_ximage;
	    dd->WriteRGBAPixels     = write_pixels_1BIT_ximage;
	    dd->WriteMonoRGBAPixels = write_pixels_mono_1BIT_ximage;
	    break;
         case PF_HPCR:
            dd->WriteRGBASpan       = write_span_HPCR_ximage;
            dd->WriteRGBSpan        = write_span_rgb_HPCR_ximage;
            dd->WriteMonoRGBASpan   = write_span_mono_HPCR_ximage;
            dd->WriteRGBAPixels     = write_pixels_HPCR_ximage;
            dd->WriteMonoRGBAPixels = write_pixels_mono_HPCR_ximage;
	    if (xmesa->xm_visual->hpcr_clear_flag) {
               ctx->Driver.ClearColor = clear_color_HPCR_ximage;
	    }
            break;
         case PF_Lookup:
	    if (depth==8) {
               dd->WriteRGBASpan       = write_span_LOOKUP8_ximage;
               dd->WriteRGBSpan        = write_rgb_LOOKUP8_ximage;
               dd->WriteMonoRGBASpan   = write_span_mono_LOOKUP8_ximage;
               dd->WriteRGBAPixels     = write_pixels_LOOKUP8_ximage;
               dd->WriteMonoRGBAPixels = write_pixels_mono_LOOKUP8_ximage;
            }
            else {
               dd->WriteRGBASpan       = write_span_LOOKUP_ximage;
               dd->WriteRGBSpan        = write_span_rgb_LOOKUP_ximage;
               dd->WriteMonoRGBASpan   = write_span_mono_ximage;
               dd->WriteRGBAPixels     = write_pixels_LOOKUP_ximage;
               dd->WriteMonoRGBAPixels = write_pixels_mono_ximage;
            }
            break;
         case PF_Grayscale:
	    if (depth==8) {
	       dd->WriteRGBASpan       = write_span_GRAYSCALE8_ximage;
	       dd->WriteRGBSpan        = write_span_rgb_GRAYSCALE8_ximage;
	       dd->WriteMonoRGBASpan   = write_span_mono_GRAYSCALE8_ximage;
	       dd->WriteRGBAPixels     = write_pixels_GRAYSCALE8_ximage;
	       dd->WriteMonoRGBAPixels = write_pixels_mono_GRAYSCALE8_ximage;
	    }
	    else {
	       dd->WriteRGBASpan       = write_span_GRAYSCALE_ximage;
	       dd->WriteRGBSpan        = write_span_rgb_GRAYSCALE_ximage;
	       dd->WriteMonoRGBASpan   = write_span_mono_ximage;
	       dd->WriteRGBAPixels     = write_pixels_GRAYSCALE_ximage;
	       dd->WriteMonoRGBAPixels = write_pixels_mono_ximage;
	    }
	    break;
	 default:
	    _mesa_problem(NULL,"Bad pixel format in xmesa_update_state (2)");
            return;
      }
   }

   /* Pixel/span reading functions: */
   dd->ReadCI32Span = read_index_span;
   dd->ReadRGBASpan = read_color_span;
   dd->ReadCI32Pixels = read_index_pixels;
   dd->ReadRGBAPixels = read_color_pixels;
}
