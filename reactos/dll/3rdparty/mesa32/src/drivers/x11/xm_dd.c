/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file xm_dd.h
 * General device driver functions for Xlib driver.
 */

#include "glxheader.h"
#include "bufferobj.h"
#include "buffers.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "drawpix.h"
#include "extensions.h"
#include "framebuffer.h"
#include "macros.h"
#include "image.h"
#include "imports.h"
#include "mtypes.h"
#include "state.h"
#include "texobj.h"
#include "teximage.h"
#include "texstore.h"
#include "texformat.h"
#include "xmesaP.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"



/*
 * Dithering kernels and lookup tables.
 */

const int xmesa_kernel8[DITH_DY * DITH_DX] = {
    0 * MAXC,  8 * MAXC,  2 * MAXC, 10 * MAXC,
   12 * MAXC,  4 * MAXC, 14 * MAXC,  6 * MAXC,
    3 * MAXC, 11 * MAXC,  1 * MAXC,  9 * MAXC,
   15 * MAXC,  7 * MAXC, 13 * MAXC,  5 * MAXC,
};

const short xmesa_HPCR_DRGB[3][2][16] = {
   {
      { 16, -4,  1,-11, 14, -6,  3, -9, 15, -5,  2,-10, 13, -7,  4, -8},
      {-15,  5,  0, 12,-13,  7, -2, 10,-14,  6, -1, 11,-12,  8, -3,  9}
   },
   {
      {-11, 15, -7,  3, -8, 14, -4,  2,-10, 16, -6,  4, -9, 13, -5,  1},
      { 12,-14,  8, -2,  9,-13,  5, -1, 11,-15,  7, -3, 10,-12,  6,  0}
   },
   {
      {  6,-18, 26,-14,  2,-22, 30,-10,  8,-16, 28,-12,  4,-20, 32, -8},
      { -4, 20,-24, 16,  0, 24,-28, 12, -6, 18,-26, 14, -2, 22,-30, 10}
   }
};

const int xmesa_kernel1[16] = {
   0*47,  9*47,  4*47, 12*47,     /* 47 = (255*3)/16 */
   6*47,  2*47, 14*47,  8*47,
  10*47,  1*47,  5*47, 11*47,
   7*47, 13*47,  3*47, 15*47
};


static void
finish_or_flush( GLcontext *ctx )
{
#ifdef XFree86Server
      /* NOT_NEEDED */
#else
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (xmesa) {
      _glthread_LOCK_MUTEX(_xmesa_lock);
      XSync( xmesa->display, False );
      _glthread_UNLOCK_MUTEX(_xmesa_lock);
   }
#endif
}


static void
clear_index( GLcontext *ctx, GLuint index )
{
   if (ctx->DrawBuffer->Name == 0) {
      const XMesaContext xmesa = XMESA_CONTEXT(ctx);
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      xmesa->clearpixel = (unsigned long) index;
      XMesaSetForeground( xmesa->display, xmbuf->cleargc, (unsigned long) index );
   }
}


static void
clear_color( GLcontext *ctx, const GLfloat color[4] )
{
   if (ctx->DrawBuffer->Name == 0) {
      const XMesaContext xmesa = XMESA_CONTEXT(ctx);
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);

      CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[0], color[0]);
      CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[1], color[1]);
      CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[2], color[2]);
      CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[3], color[3]);
      xmesa->clearpixel = xmesa_color_to_pixel( ctx,
                                                xmesa->clearcolor[0],
                                                xmesa->clearcolor[1],
                                                xmesa->clearcolor[2],
                                                xmesa->clearcolor[3],
                                                xmesa->xm_visual->undithered_pf );
      _glthread_LOCK_MUTEX(_xmesa_lock);
      XMesaSetForeground( xmesa->display, xmbuf->cleargc,
                          xmesa->clearpixel );
      _glthread_UNLOCK_MUTEX(_xmesa_lock);
   }
}



/* Set index mask ala glIndexMask */
static void
index_mask( GLcontext *ctx, GLuint mask )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
   /* not sure this conditional is really needed */
   if (xmbuf->backxrb && xmbuf->backxrb->pixmap) {
      unsigned long m;
      if (mask==0xffffffff) {
	 m = ((unsigned long)~0L);
      }
      else {
         m = (unsigned long) mask;
      }
      XMesaSetPlaneMask( xmesa->display, xmbuf->cleargc, m );
      XMesaSetPlaneMask( xmesa->display, xmbuf->gc, m );
   }
}


/* Implements glColorMask() */
static void
color_mask(GLcontext *ctx,
           GLboolean rmask, GLboolean gmask, GLboolean bmask, GLboolean amask)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer xmbuf;
   const int xclass = xmesa->xm_visual->mesa_visual.visualType;
   (void) amask;

   if (ctx->DrawBuffer->Name != 0)
      return;

   xmbuf = XMESA_BUFFER(ctx->DrawBuffer);

   if (xclass == GLX_TRUE_COLOR || xclass == GLX_DIRECT_COLOR) {
      unsigned long m;
      if (rmask && gmask && bmask) {
         m = ((unsigned long)~0L);
      }
      else {
         m = 0;
         if (rmask)   m |= GET_REDMASK(xmesa->xm_visual);
         if (gmask)   m |= GET_GREENMASK(xmesa->xm_visual);
         if (bmask)   m |= GET_BLUEMASK(xmesa->xm_visual);
      }
      XMesaSetPlaneMask( xmesa->display, xmbuf->cleargc, m );
      XMesaSetPlaneMask( xmesa->display, xmbuf->gc, m );
   }
}



/**********************************************************************/
/*** glClear implementations                                        ***/
/**********************************************************************/


/**
 * Clear the front or back color buffer, if it's implemented with a pixmap.
 */
static void
clear_pixmap(GLcontext *ctx, struct xmesa_renderbuffer *xrb,
             GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);

   assert(xmbuf);
   assert(xrb->pixmap);
   assert(xmesa);
   assert(xmesa->display);
   assert(xrb->pixmap);
   assert(xmbuf->cleargc);

   XMesaFillRectangle( xmesa->display, xrb->pixmap, xmbuf->cleargc,
                       x, xrb->Base.Height - y - height,
                       width, height );
}


static void
clear_8bit_ximage( GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLint i;
   for (i = 0; i < height; i++) {
      GLubyte *ptr = PIXEL_ADDR1(xrb, x, y + i);
      MEMSET( ptr, xmesa->clearpixel, width );
   }
}


static void
clear_HPCR_ximage( GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLint i;
   for (i = y; i < y + height; i++) {
      GLubyte *ptr = PIXEL_ADDR1( xrb, x, i );
      int j;
      const GLubyte *sptr = xmesa->xm_visual->hpcr_clear_ximage_pattern[0];
      if (i & 1) {
         sptr += 16;
      }
      for (j = x; j < x + width; j++) {
         *ptr = sptr[j&15];
         ptr++;
      }
   }
}


static void
clear_16bit_ximage( GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                    GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLuint pixel = (GLuint) xmesa->clearpixel;
   GLint i, j;

   if (xmesa->swapbytes) {
      pixel = ((pixel >> 8) & 0x00ff) | ((pixel << 8) & 0xff00);
   }

   for (j = 0; j < height; j++) {
      GLushort *ptr2 = PIXEL_ADDR2(xrb, x, y + j);
      for (i = 0; i < width; i++) {
         ptr2[i] = pixel;
      }
   }
}


/* Optimized code provided by Nozomi Ytow <noz@xfree86.org> */
static void
clear_24bit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                   GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = xmesa->clearcolor[0];
   const GLubyte g = xmesa->clearcolor[1];
   const GLubyte b = xmesa->clearcolor[2];

   if (r == g && g == b) {
      /* same value for all three components (gray) */
      GLint j;
      for (j = 0; j < height; j++) {
         bgr_t *ptr3 = PIXEL_ADDR3(xrb, x, y + j);
         MEMSET(ptr3, r, 3 * width);
      }
   }
   else {
      /* non-gray clear color */
      GLint i, j;
      for (j = 0; j < height; j++) {
         bgr_t *ptr3 = PIXEL_ADDR3(xrb, x, y + j);
         for (i = 0; i < width; i++) {
            ptr3->r = r;
            ptr3->g = g;
            ptr3->b = b;
            ptr3++;
         }
      }
   }
}


static void
clear_32bit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                   GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint pixel = (GLuint) xmesa->clearpixel;

   if (!xrb->ximage)
      return;

   if (xmesa->swapbytes) {
      pixel = ((pixel >> 24) & 0x000000ff)
            | ((pixel >> 8)  & 0x0000ff00)
            | ((pixel << 8)  & 0x00ff0000)
            | ((pixel << 24) & 0xff000000);
   }

   if (width == xrb->Base.Width && height == xrb->Base.Height) {
      /* clearing whole buffer */
      const GLuint n = xrb->Base.Width * xrb->Base.Height;
      GLuint *ptr4 = (GLuint *) xrb->ximage->data;
      if (pixel == 0) {
         /* common case */
         _mesa_memset(ptr4, pixel, 4 * n);
      }
      else {
         GLuint i;
         for (i = 0; i < n; i++)
            ptr4[i] = pixel;
      }
   }
   else {
      /* clearing scissored region */
      GLint i, j;
      for (j = 0; j < height; j++) {
         GLuint *ptr4 = PIXEL_ADDR4(xrb, x, y + j);
         for (i = 0; i < width; i++) {
            ptr4[i] = pixel;
         }
      }
   }
}


static void
clear_nbit_ximage(GLcontext *ctx, struct xmesa_renderbuffer *xrb,
                  GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xrb->ximage;
   GLint i, j;

   /* TODO: optimize this */
   y = YFLIP(xrb, y);
   for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) {
         XMesaPutPixel(img, x+i, y-j, xmesa->clearpixel);
      }
   }
}



static void
clear_buffers(GLcontext *ctx, GLbitfield buffers)
{
   if (ctx->DrawBuffer->Name == 0) {
      /* this is a window system framebuffer */
      const GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask;
      XMesaBuffer b = XMESA_BUFFER(ctx->DrawBuffer);
      const GLint x = ctx->DrawBuffer->_Xmin;
      const GLint y = ctx->DrawBuffer->_Ymin;
      const GLint width = ctx->DrawBuffer->_Xmax - x;
      const GLint height = ctx->DrawBuffer->_Ymax - y;

      /* we can't handle color or index masking */
      if (*colorMask == 0xffffffff && ctx->Color.IndexMask == 0xffffffff) {
         if (buffers & BUFFER_BIT_FRONT_LEFT) {
            /* clear front color buffer */
            struct gl_renderbuffer *frontRb
               = ctx->DrawBuffer->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
            if (b->frontxrb == xmesa_renderbuffer(frontRb)) {
               /* renderbuffer is not wrapped - great! */
               b->frontxrb->clearFunc(ctx, b->frontxrb, x, y, width, height);
               buffers &= ~BUFFER_BIT_FRONT_LEFT;
            }
            else {
               /* we can't directly clear an alpha-wrapped color buffer */
            }
         }
         if (buffers & BUFFER_BIT_BACK_LEFT) {
            /* clear back color buffer */
            struct gl_renderbuffer *backRb
               = ctx->DrawBuffer->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
            if (b->backxrb == xmesa_renderbuffer(backRb)) {
               /* renderbuffer is not wrapped - great! */
               b->backxrb->clearFunc(ctx, b->backxrb, x, y, width, height);
               buffers &= ~BUFFER_BIT_BACK_LEFT;
            }
         }
      }
   }
   if (buffers)
      _swrast_Clear(ctx, buffers);
}


#ifndef XFree86Server
/* XXX this was never tested in the Xserver environment */

/**
 * This function implements glDrawPixels() with an XPutImage call when
 * drawing to the front buffer (X Window drawable).
 * The image format must be GL_BGRA to match the PF_8R8G8B pixel format.
 */
static void
xmesa_DrawPixels_8R8G8B( GLcontext *ctx,
                         GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const struct gl_pixelstore_attrib *unpack,
                         const GLvoid *pixels )
{
   const SWcontext *swrast = SWRAST_CONTEXT( ctx );
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb->Wrapped);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (ctx->DrawBuffer->Name == 0 &&
       format == GL_BGRA &&
       type == GL_UNSIGNED_BYTE &&
       (swrast->_RasterMask & ~CLIP_BIT) == 0 && /* no blend, z-test, etc */
       ctx->_ImageTransferState == 0 &&  /* no color tables, scale/bias, etc */
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0 &&
       xrb->pixmap &&
       xrb->Base.AlphaBits == 0)
   {
      const XMesaContext xmesa = XMESA_CONTEXT(ctx);
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      XMesaDisplay *dpy = xmesa->xm_visual->display;
      const XMesaGC gc = xmbuf->gc;
      int dstX = x;
      int dstY = y;
      int w = width;
      int h = height;
      struct gl_pixelstore_attrib clippedUnpack = *unpack;

      ASSERT(xmesa->xm_visual->dithered_pf == PF_8R8G8B);
      ASSERT(xmesa->xm_visual->undithered_pf == PF_8R8G8B);
      ASSERT(dpy);
      ASSERT(gc);

      if (unpack->BufferObj->Name) {
         /* unpack from PBO */
         GLubyte *buf;
         if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
                                        format, type, pixels)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(invalid PBO access)");
            return;
         }
         buf = (GLubyte *) ctx->Driver.MapBuffer(ctx,
                                                 GL_PIXEL_UNPACK_BUFFER_EXT,
                                                 GL_READ_ONLY_ARB,
                                                 unpack->BufferObj);
         if (!buf) {
            /* buffer is already mapped - that's an error */
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(PBO is mapped)");
            return;
         }
         pixels = ADD_POINTERS(buf, pixels);
      }

      if (_mesa_clip_drawpixels(ctx, &dstX, &dstY, &w, &h, &clippedUnpack)) {
         /* This is a little tricky since all coordinates up to now have
          * been in the OpenGL bottom-to-top orientation.  X is top-to-bottom
          * so we have to carefully compute the Y coordinates/addresses here.
          */
         int srcX = clippedUnpack.SkipPixels;
         int srcY = clippedUnpack.SkipRows;
         int rowLength = clippedUnpack.RowLength;
         XMesaImage ximage;
         MEMSET(&ximage, 0, sizeof(XMesaImage));
         ximage.width = width;
         ximage.height = height;
         ximage.format = ZPixmap;
         ximage.data = (char *) pixels
            + ((srcY + h - 1) * rowLength + srcX) * 4;
         ximage.byte_order = LSBFirst;
         ximage.bitmap_unit = 32;
         ximage.bitmap_bit_order = LSBFirst;
         ximage.bitmap_pad = 32;
         ximage.depth = 24;
         ximage.bytes_per_line = -rowLength * 4; /* negative to flip image */
         ximage.bits_per_pixel = 32;
         /* it seems we don't need to set the ximage.red/green/blue_mask fields */
         /* flip Y axis for dest position */
         dstY = YFLIP(xrb, dstY) - h + 1;
         XPutImage(dpy, xrb->pixmap, gc, &ximage, 0, 0, dstX, dstY, w, h);
      }

      if (unpack->BufferObj->Name) {
         ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                 unpack->BufferObj);
      }
   }
   else {
      /* software fallback */
      _swrast_DrawPixels(ctx, x, y, width, height,
                         format, type, unpack, pixels);
   }
}



/**
 * This function implements glDrawPixels() with an XPutImage call when
 * drawing to the front buffer (X Window drawable).  The image format
 * must be GL_RGB and image type must be GL_UNSIGNED_SHORT_5_6_5 to
 * match the PF_5R6G5B pixel format.
 */
static void
xmesa_DrawPixels_5R6G5B( GLcontext *ctx,
                         GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const struct gl_pixelstore_attrib *unpack,
                         const GLvoid *pixels )
{
   struct xmesa_renderbuffer *xrb
      = xmesa_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0][0]->Wrapped);
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT( ctx );
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
   const XMesaGC gc = xmbuf->gc;

   ASSERT(dpy);
   ASSERT(gc);
   ASSERT(xmesa->xm_visual->undithered_pf == PF_5R6G5B);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (xrb->pixmap &&
       format == GL_RGB &&
       type == GL_UNSIGNED_SHORT_5_6_5 &&
       !ctx->Color.DitherFlag &&  /* no dithering */
       (swrast->_RasterMask & ~CLIP_BIT) == 0 && /* no blend, z-test, etc */
       ctx->_ImageTransferState == 0 &&  /* no color tables, scale/bias, etc */
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0) {
      int dstX = x;
      int dstY = y;
      int w = width;
      int h = height;
      struct gl_pixelstore_attrib clippedUnpack = *unpack;

      if (unpack->BufferObj->Name) {
         /* unpack from PBO */
         GLubyte *buf;
         if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
                                        format, type, pixels)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(invalid PBO access)");
            return;
         }
         buf = (GLubyte *) ctx->Driver.MapBuffer(ctx,
                                                 GL_PIXEL_UNPACK_BUFFER_EXT,
                                                 GL_READ_ONLY_ARB,
                                                 unpack->BufferObj);
         if (!buf) {
            /* buffer is already mapped - that's an error */
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(PBO is mapped)");
            return;
         }
         pixels = ADD_POINTERS(buf, pixels);
      }

      if (_mesa_clip_drawpixels(ctx, &dstX, &dstY, &w, &h, &clippedUnpack)) {
         /* This is a little tricky since all coordinates up to now have
          * been in the OpenGL bottom-to-top orientation.  X is top-to-bottom
          * so we have to carefully compute the Y coordinates/addresses here.
          */
         int srcX = clippedUnpack.SkipPixels;
         int srcY = clippedUnpack.SkipRows;
         int rowLength = clippedUnpack.RowLength;
         XMesaImage ximage;
         MEMSET(&ximage, 0, sizeof(XMesaImage));
         ximage.width = width;
         ximage.height = height;
         ximage.format = ZPixmap;
         ximage.data = (char *) pixels
            + ((srcY + h - 1) * rowLength + srcX) * 2;
         ximage.byte_order = LSBFirst;
         ximage.bitmap_unit = 16;
         ximage.bitmap_bit_order = LSBFirst;
         ximage.bitmap_pad = 16;
         ximage.depth = 16;
         ximage.bytes_per_line = -rowLength * 2; /* negative to flip image */
         ximage.bits_per_pixel = 16;
         /* it seems we don't need to set the ximage.red/green/blue_mask fields */
         /* flip Y axis for dest position */
         dstY = YFLIP(xrb, dstY) - h + 1;
         XPutImage(dpy, xrb->pixmap, gc, &ximage, 0, 0, dstX, dstY, w, h);
      }

      if (unpack->BufferObj->Name) {
         ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                 unpack->BufferObj);
      }
   }
   else {
      /* software fallback */
      _swrast_DrawPixels(ctx, x, y, width, height,
                         format, type, unpack, pixels);
   }
}



/**
 * Implement glCopyPixels for the front color buffer (or back buffer Pixmap)
 * for the color buffer.  Don't support zooming, pixel transfer, etc.
 * We do support copying from one window to another, ala glXMakeCurrentRead.
 */
static void
xmesa_CopyPixels( GLcontext *ctx,
                  GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                  GLint destx, GLint desty, GLenum type )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT( ctx );
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   const XMesaGC gc = ((XMesaBuffer) ctx->DrawBuffer)->gc;
   struct xmesa_renderbuffer *srcXrb
      = xmesa_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer->Wrapped);
   struct xmesa_renderbuffer *dstXrb
      = xmesa_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0][0]->Wrapped);

   ASSERT(dpy);
   ASSERT(gc);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (ctx->Color.DrawBuffer[0] == GL_FRONT &&
       ctx->Pixel.ReadBuffer == GL_FRONT &&
       srcXrb->pixmap &&
       dstXrb->pixmap &&
       type == GL_COLOR &&
       (swrast->_RasterMask & ~CLIP_BIT) == 0 && /* no blend, z-test, etc */
       ctx->_ImageTransferState == 0 &&  /* no color tables, scale/bias, etc */
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0) {
      /* Note: we don't do any special clipping work here.  We could,
       * but X will do it for us.
       */
      srcy = YFLIP(srcXrb, srcy) - height + 1;
      desty = YFLIP(dstXrb, desty) - height + 1;
      XCopyArea(dpy, srcXrb->pixmap, dstXrb->pixmap, gc,
                srcx, srcy, width, height, destx, desty);
   }
   else {
      _swrast_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type );
   }
}
#endif /* XFree86Server */



/*
 * Every driver should implement a GetString function in order to
 * return a meaningful GL_RENDERER string.
 */
static const GLubyte *
get_string( GLcontext *ctx, GLenum name )
{
   (void) ctx;
   switch (name) {
      case GL_RENDERER:
#ifdef XFree86Server
         return (const GLubyte *) "Mesa GLX Indirect";
#else
         return (const GLubyte *) "Mesa X11";
#endif
      case GL_VENDOR:
#ifdef XFree86Server
         return (const GLubyte *) "Mesa project: www.mesa3d.org";
#else
         return NULL;
#endif
      default:
         return NULL;
   }
}


/*
 * We implement the glEnable function only because we care about
 * dither enable/disable.
 */
static void
enable( GLcontext *ctx, GLenum pname, GLboolean state )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   switch (pname) {
      case GL_DITHER:
         if (state)
            xmesa->pixelformat = xmesa->xm_visual->dithered_pf;
         else
            xmesa->pixelformat = xmesa->xm_visual->undithered_pf;
         break;
      default:
         ;  /* silence compiler warning */
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
		 XMESA_BUFFER(ctx->DrawBuffer)->cleargc,
		 xmesa->xm_visual->hpcr_clear_ximage, 0, 0, 0, 0, 16, 2);
}


/**
 * Called when the driver should update its state, based on the new_state
 * flags.
 */
void
xmesa_update_state( GLcontext *ctx, GLbitfield new_state )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   /* Propagate statechange information to swrast and swrast_setup
    * modules.  The X11 driver has no internal GL-dependent state.
    */
   _swrast_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );

   if (ctx->DrawBuffer->Name != 0)
      return;

   /*
    * GL_DITHER, GL_READ/DRAW_BUFFER, buffer binding state, etc. effect
    * renderbuffer span/clear funcs.
    */
   if (new_state & (_NEW_COLOR | _NEW_PIXEL | _NEW_BUFFERS)) {
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      struct xmesa_renderbuffer *front_xrb, *back_xrb;

      front_xrb = xmbuf->frontxrb;
      if (front_xrb) {
         xmesa_set_renderbuffer_funcs(front_xrb, xmesa->pixelformat,
                                      xmesa->xm_visual->BitsPerPixel);
         front_xrb->clearFunc = clear_pixmap;
      }

      back_xrb = xmbuf->backxrb;
      if (back_xrb) {
         xmesa_set_renderbuffer_funcs(back_xrb, xmesa->pixelformat,
                                      xmesa->xm_visual->BitsPerPixel);
         if (xmbuf->backxrb->pixmap) {
            back_xrb->clearFunc = clear_pixmap;
         }
         else {
            switch (xmesa->xm_visual->BitsPerPixel) {
            case 8:
               if (xmesa->xm_visual->hpcr_clear_flag) {
                  back_xrb->clearFunc = clear_HPCR_ximage;
               }
               else {
                  back_xrb->clearFunc = clear_8bit_ximage;
               }
               break;
            case 16:
               back_xrb->clearFunc = clear_16bit_ximage;
               break;
            case 24:
               back_xrb->clearFunc = clear_24bit_ximage;
               break;
            case 32:
               back_xrb->clearFunc = clear_32bit_ximage;
               break;
            default:
               back_xrb->clearFunc = clear_nbit_ximage;
               break;
            }
         }
      }
   }

   if (xmesa->xm_visual->hpcr_clear_flag) {
      /* this depends on whether we're drawing to the front or back buffer */
      /* XXX FIX THIS! */
#if 0
      if (pixmap) {
         ctx->Driver.ClearColor = clear_color_HPCR_pixmap;
      }
      else {
         ctx->Driver.ClearColor = clear_color_HPCR_ximage;
      }
#else
      (void) clear_color_HPCR_pixmap;
      (void) clear_color_HPCR_ximage;
#endif
   }
}



/**
 * Called via ctx->Driver.TestProxyTeximage().  Normally, we'd just use
 * the _mesa_test_proxy_teximage() fallback function, but we're going to
 * special-case the 3D texture case to allow textures up to 512x512x32
 * texels.
 */
static GLboolean
test_proxy_teximage(GLcontext *ctx, GLenum target, GLint level,
                    GLint internalFormat, GLenum format, GLenum type,
                    GLint width, GLint height, GLint depth, GLint border)
{
   if (target == GL_PROXY_TEXTURE_3D) {
      /* special case for 3D textures */
      if (width * height * depth > 512 * 512 * 64 ||
          width  < 2 * border ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           _mesa_bitcount(width  - 2 * border) != 1) ||
          height < 2 * border ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           _mesa_bitcount(height - 2 * border) != 1) ||
          depth  < 2 * border ||
          (!ctx->Extensions.ARB_texture_non_power_of_two &&
           _mesa_bitcount(depth  - 2 * border) != 1)) {
         /* Bad size, or too many texels */
         return GL_FALSE;
      }
      return GL_TRUE;
   }
   else {
      /* use the fallback routine for 1D, 2D, cube and rect targets */
      return _mesa_test_proxy_teximage(ctx, target, level, internalFormat,
                                       format, type, width, height, depth,
                                       border);
   }
}


/**
 * In SW, we don't really compress GL_COMPRESSED_RGB[A] textures!
 */
static const struct gl_texture_format *
choose_tex_format( GLcontext *ctx, GLint internalFormat,
                   GLenum format, GLenum type )
{
   switch (internalFormat) {
      case GL_COMPRESSED_RGB_ARB:
         return &_mesa_texformat_rgb;
      case GL_COMPRESSED_RGBA_ARB:
         return &_mesa_texformat_rgba;
      default:
         return _mesa_choose_tex_format(ctx, internalFormat, format, type);
   }
}


/**
 * Called by glViewport.
 * This is a good time for us to poll the current X window size and adjust
 * our renderbuffers to match the current window size.
 * Remember, we have no opportunity to respond to conventional
 * X Resize/StructureNotify events since the X driver has no event loop.
 * Thus, we poll.
 * Note that this trick isn't fool-proof.  If the application never calls
 * glViewport, our notion of the current window size may be incorrect.
 * That problem led to the GLX_MESA_resize_buffers extension.
 */
static void
xmesa_viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   XMesaContext xmctx = XMESA_CONTEXT(ctx);
   XMesaBuffer xmdrawbuf = XMESA_BUFFER(ctx->WinSysDrawBuffer);
   XMesaBuffer xmreadbuf = XMESA_BUFFER(ctx->WinSysReadBuffer);
   xmesa_check_and_update_buffer_size(xmctx, xmdrawbuf);
   xmesa_check_and_update_buffer_size(xmctx, xmreadbuf);
   (void) x;
   (void) y;
   (void) w;
   (void) h;
}


#if ENABLE_EXT_timer_query

/*
 * The GL_EXT_timer_query extension is not enabled for the XServer
 * indirect renderer.  Not sure about how/if wrapping of gettimeofday()
 * is done, etc.
 */

struct xmesa_query_object
{
   struct gl_query_object Base;
   struct timeval StartTime;
};


static struct gl_query_object *
xmesa_new_query_object(GLcontext *ctx, GLuint id)
{
   struct xmesa_query_object *q = CALLOC_STRUCT(xmesa_query_object);
   if (q) {
      q->Base.Id = id;
      q->Base.Ready = GL_TRUE;
   }
   return &q->Base;
}


static void
xmesa_begin_query(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
   if (target == GL_TIME_ELAPSED_EXT) {
      struct xmesa_query_object *xq = (struct xmesa_query_object *) q;
      (void) gettimeofday(&xq->StartTime, NULL);
   }
}


/**
 * Return the difference between the two given times in microseconds.
 */
#ifdef __VMS
#define suseconds_t unsigned int
#endif
static GLuint64EXT
time_diff(const struct timeval *t0, const struct timeval *t1)
{
   GLuint64EXT seconds0 = t0->tv_sec & 0xff;  /* 0 .. 255 seconds */
   GLuint64EXT seconds1 = t1->tv_sec & 0xff;  /* 0 .. 255 seconds */
   GLuint64EXT nanosec0 = (seconds0 * 1000000 + t0->tv_usec) * 1000;
   GLuint64EXT nanosec1 = (seconds1 * 1000000 + t1->tv_usec) * 1000;
   return nanosec1 - nanosec0;
}


static void
xmesa_end_query(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
   if (target == GL_TIME_ELAPSED_EXT) {
      struct xmesa_query_object *xq = (struct xmesa_query_object *) q;
      struct timeval endTime;
      (void) gettimeofday(&endTime, NULL);
      /* result is in nanoseconds! */
      q->Result = time_diff(&xq->StartTime, &endTime);
   }
   q->Ready = GL_TRUE;
}

#endif /* ENABLE_timer_query */


/**
 * Initialize the device driver function table with the functions
 * we implement in this driver.
 */
void
xmesa_init_driver_functions( XMesaVisual xmvisual,
                             struct dd_function_table *driver )
{
   driver->GetString = get_string;
   driver->UpdateState = xmesa_update_state;
   driver->GetBufferSize = NULL; /* OBSOLETE */
   driver->Flush = finish_or_flush;
   driver->Finish = finish_or_flush;
   driver->ClearIndex = clear_index;
   driver->ClearColor = clear_color;
   driver->IndexMask = index_mask;
   driver->ColorMask = color_mask;
   driver->Enable = enable;
   driver->Clear = clear_buffers;
   driver->Viewport = xmesa_viewport;
#ifndef XFree86Server
   driver->CopyPixels = xmesa_CopyPixels;
   if (xmvisual->undithered_pf == PF_8R8G8B &&
       xmvisual->dithered_pf == PF_8R8G8B) {
      driver->DrawPixels = xmesa_DrawPixels_8R8G8B;
   }
   else if (xmvisual->undithered_pf == PF_5R6G5B) {
      driver->DrawPixels = xmesa_DrawPixels_5R6G5B;
   }
#endif
   driver->TestProxyTexImage = test_proxy_teximage;
#if ENABLE_EXT_texure_compression_s3tc
   driver->ChooseTextureFormat = choose_tex_format;
#else
   (void) choose_tex_format;
#endif

#if ENABLE_EXT_timer_query
   driver->NewQueryObject = xmesa_new_query_object;
   driver->BeginQuery = xmesa_begin_query;
   driver->EndQuery = xmesa_end_query;
#endif
}


#define XMESA_NEW_POINT  (_NEW_POINT | \
                          _NEW_RENDERMODE | \
                          _SWRAST_NEW_RASTERMASK)

#define XMESA_NEW_LINE   (_NEW_LINE | \
                          _NEW_TEXTURE | \
                          _NEW_LIGHT | \
                          _NEW_DEPTH | \
                          _NEW_RENDERMODE | \
                          _SWRAST_NEW_RASTERMASK)

#define XMESA_NEW_TRIANGLE (_NEW_POLYGON | \
                            _NEW_TEXTURE | \
                            _NEW_LIGHT | \
                            _NEW_DEPTH | \
                            _NEW_RENDERMODE | \
                            _SWRAST_NEW_RASTERMASK)


/**
 * Extend the software rasterizer with our line/point/triangle
 * functions.
 * Called during context creation only.
 */
void xmesa_register_swrast_functions( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );

   swrast->choose_point = xmesa_choose_point;
   swrast->choose_line = xmesa_choose_line;
   swrast->choose_triangle = xmesa_choose_triangle;

   /* XXX these lines have no net effect.  Remove??? */
   swrast->InvalidatePointMask |= XMESA_NEW_POINT;
   swrast->InvalidateLineMask |= XMESA_NEW_LINE;
   swrast->InvalidateTriangleMask |= XMESA_NEW_TRIANGLE;
}
