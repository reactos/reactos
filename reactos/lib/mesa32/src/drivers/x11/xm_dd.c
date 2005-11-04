/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


#include "glxheader.h"
#include "bufferobj.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "drawpix.h"
#include "extensions.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"
#include "state.h"
#include "texobj.h"
#include "teximage.h"
#include "texstore.h"
#include "texformat.h"
#include "xmesaP.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast/s_auxbuffer.h"
#include "swrast/s_context.h"
#include "swrast/s_drawpix.h"
#include "swrast/s_alphabuf.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#ifdef XFree86Server
#include <GL/glxtokens.h>
#endif



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


/*
 * Return the size (width, height) of the X window for the given GLframebuffer.
 * Output:  width - width of buffer in pixels.
 *          height - height of buffer in pixels.
 */
static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   /* We can do this cast because the first field in the XMesaBuffer
    * struct is a GLframebuffer struct.  If this weren't true, we'd
    * need a pointer from the GLframebuffer to the XMesaBuffer.
    */
   const XMesaBuffer xmBuffer = (XMesaBuffer) buffer;
   unsigned int winwidth, winheight;
#ifdef XFree86Server
   /* XFree86 GLX renderer */
   winwidth = MIN2(xmBuffer->frontbuffer->width, MAX_WIDTH);
   winheight = MIN2(xmBuffer->frontbuffer->height, MAX_HEIGHT);
#else
   Window root;
   int winx, winy;
   unsigned int bw, d;

   _glthread_LOCK_MUTEX(_xmesa_lock);
   XSync(xmBuffer->xm_visual->display, 0); /* added for Chromium */
   XGetGeometry( xmBuffer->xm_visual->display, xmBuffer->frontbuffer, &root,
		 &winx, &winy, &winwidth, &winheight, &bw, &d );
   _glthread_UNLOCK_MUTEX(_xmesa_lock);
#endif

   *width = winwidth;
   *height = winheight;
}


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



/*
 * This chooses the color buffer for reading and writing spans, points,
 * lines, and triangles.
 */
void
xmesa_set_buffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit )
{
   /* We can make this cast since the XMesaBuffer wraps GLframebuffer.
    * GLframebuffer is the first member in a XMesaBuffer struct.
    */
   XMesaBuffer target = (XMesaBuffer) buffer;
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   /* This assignment tells the span/point/line/triangle functions
    * which XMesaBuffer to use.
    */
   xmesa->xm_buffer = target;

   /*
    * Now determine front vs back color buffer.
    */
   if (bufferBit == DD_FRONT_LEFT_BIT) {
      target->buffer = target->frontbuffer;
      xmesa_update_span_funcs(ctx);
   }
   else if (bufferBit == DD_BACK_LEFT_BIT) {
      ASSERT(target->db_state);
      if (target->backpixmap) {
         /* back buffer is a pixmap */
         target->buffer = (XMesaDrawable) target->backpixmap;
      }
      else if (target->backimage) {
         /* back buffer is an XImage */
         target->buffer = None;
      }
      else {
         /* No back buffer!!!!  Must be out of memory, use front buffer */
         target->buffer = target->frontbuffer;
      }
      xmesa_update_span_funcs(ctx);
   }
   else if (bufferBit & (DD_AUX0_BIT | DD_AUX1_BIT | DD_AUX2_BIT | DD_AUX3_BIT)) {
      _swrast_use_aux_buffer(ctx, buffer, bufferBit);
   } 
   else {
      _mesa_problem(ctx, "invalid buffer 0x%x in set_buffer() in xm_dd.c");
      return;
   }
}



static void
clear_index( GLcontext *ctx, GLuint index )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   xmesa->clearpixel = (unsigned long) index;
   XMesaSetForeground( xmesa->display, xmesa->xm_draw_buffer->cleargc,
                       (unsigned long) index );
}


static void
clear_color( GLcontext *ctx, const GLfloat color[4] )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(xmesa->clearcolor[3], color[3]);
   xmesa->clearpixel = xmesa_color_to_pixel( xmesa,
                                             xmesa->clearcolor[0],
                                             xmesa->clearcolor[1],
                                             xmesa->clearcolor[2],
                                             xmesa->clearcolor[3],
                                             xmesa->xm_visual->undithered_pf );
   _glthread_LOCK_MUTEX(_xmesa_lock);
   XMesaSetForeground( xmesa->display, xmesa->xm_draw_buffer->cleargc,
                       xmesa->clearpixel );
   _glthread_UNLOCK_MUTEX(_xmesa_lock);
}



/* Set index mask ala glIndexMask */
static void
index_mask( GLcontext *ctx, GLuint mask )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (xmesa->xm_draw_buffer->buffer != XIMAGE) {
      unsigned long m;
      if (mask==0xffffffff) {
	 m = ((unsigned long)~0L);
      }
      else {
         m = (unsigned long) mask;
      }
      XMesaSetPlaneMask( xmesa->display, xmesa->xm_draw_buffer->cleargc, m );
      XMesaSetPlaneMask( xmesa->display, xmesa->xm_draw_buffer->gc, m );
   }
}


/* Implements glColorMask() */
static void
color_mask(GLcontext *ctx,
           GLboolean rmask, GLboolean gmask, GLboolean bmask, GLboolean amask)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const int xclass = xmesa->xm_visual->mesa_visual.visualType;
   (void) amask;

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
      XMesaSetPlaneMask( xmesa->display, xmesa->xm_draw_buffer->cleargc, m );
      XMesaSetPlaneMask( xmesa->display, xmesa->xm_draw_buffer->gc, m );
   }
}



/**********************************************************************/
/*** glClear implementations                                        ***/
/**********************************************************************/


static void
clear_front_pixmap( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (all) {
      XMesaFillRectangle( xmesa->display, xmesa->xm_draw_buffer->frontbuffer,
                          xmesa->xm_draw_buffer->cleargc,
                          0, 0,
                          xmesa->xm_draw_buffer->width+1,
                          xmesa->xm_draw_buffer->height+1 );
   }
   else {
      XMesaFillRectangle( xmesa->display, xmesa->xm_draw_buffer->frontbuffer,
                          xmesa->xm_draw_buffer->cleargc,
                          x, xmesa->xm_draw_buffer->height - y - height,
                          width, height );
   }
}


static void
clear_back_pixmap( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (all) {
      XMesaFillRectangle( xmesa->display, xmesa->xm_draw_buffer->backpixmap,
                          xmesa->xm_draw_buffer->cleargc,
                          0, 0,
                          xmesa->xm_draw_buffer->width+1,
                          xmesa->xm_draw_buffer->height+1 );
   }
   else {
      XMesaFillRectangle( xmesa->display, xmesa->xm_draw_buffer->backpixmap,
                          xmesa->xm_draw_buffer->cleargc,
                          x, xmesa->xm_draw_buffer->height - y - height,
                          width, height );
   }
}


static void
clear_8bit_ximage( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (all) {
      size_t n = xmesa->xm_draw_buffer->backimage->bytes_per_line
         * xmesa->xm_draw_buffer->backimage->height;
      MEMSET( xmesa->xm_draw_buffer->backimage->data, xmesa->clearpixel, n );
   }
   else {
      GLint i;
      for (i=0;i<height;i++) {
         GLubyte *ptr = PIXELADDR1( xmesa->xm_draw_buffer, x, y+i );
         MEMSET( ptr, xmesa->clearpixel, width );
      }
   }
}


static void
clear_HPCR_ximage( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (all) {
      GLint i, c16 = (xmesa->xm_draw_buffer->backimage->bytes_per_line>>4)<<4;
      GLubyte *ptr  = (GLubyte *)xmesa->xm_draw_buffer->backimage->data;
      for (i=0; i<xmesa->xm_draw_buffer->backimage->height; i++) {
         GLint j;
         GLubyte *sptr = xmesa->xm_visual->hpcr_clear_ximage_pattern[0];
         if (i&1) {
            sptr += 16;
         }
         for (j=0; j<c16; j+=16) {
            ptr[0] = sptr[0];
            ptr[1] = sptr[1];
            ptr[2] = sptr[2];
            ptr[3] = sptr[3];
            ptr[4] = sptr[4];
            ptr[5] = sptr[5];
            ptr[6] = sptr[6];
            ptr[7] = sptr[7];
            ptr[8] = sptr[8];
            ptr[9] = sptr[9];
            ptr[10] = sptr[10];
            ptr[11] = sptr[11];
            ptr[12] = sptr[12];
            ptr[13] = sptr[13];
            ptr[14] = sptr[14];
            ptr[15] = sptr[15];
            ptr += 16;
         }
         for (; j<xmesa->xm_draw_buffer->backimage->bytes_per_line; j++) {
            *ptr = sptr[j&15];
            ptr++;
         }
      }
   }
   else {
      GLint i;
      for (i=y; i<y+height; i++) {
         GLubyte *ptr = PIXELADDR1( xmesa->xm_draw_buffer, x, i );
         int j;
         GLubyte *sptr = xmesa->xm_visual->hpcr_clear_ximage_pattern[0];
         if (i&1) {
            sptr += 16;
         }
         for (j=x; j<x+width; j++) {
            *ptr = sptr[j&15];
            ptr++;
         }
      }
   }
}


static void
clear_16bit_ximage( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint pixel = (GLuint) xmesa->clearpixel;
   if (xmesa->swapbytes) {
      pixel = ((pixel >> 8) & 0x00ff) | ((pixel << 8) & 0xff00);
   }
   if (all) {
      register GLuint n;
      register GLuint *ptr4 = (GLuint *) xmesa->xm_draw_buffer->backimage->data;
      if ((pixel & 0xff) == ((pixel >> 8) & 0xff)) {
         /* low and high bytes are equal so use memset() */
         n = xmesa->xm_draw_buffer->backimage->bytes_per_line
            * xmesa->xm_draw_buffer->height;
         MEMSET( ptr4, pixel & 0xff, n );
      }
      else {
         pixel = pixel | (pixel<<16);
         n = xmesa->xm_draw_buffer->backimage->bytes_per_line
            * xmesa->xm_draw_buffer->height / 4;
         do {
            *ptr4++ = pixel;
               n--;
         } while (n!=0);

         if ((xmesa->xm_draw_buffer->backimage->bytes_per_line *
              xmesa->xm_draw_buffer->height) & 0x2)
            *(GLushort *)ptr4 = pixel & 0xffff;
      }
   }
   else {
      register int i, j;
      for (j=0;j<height;j++) {
         register GLushort *ptr2 = PIXELADDR2( xmesa->xm_draw_buffer, x, y+j );
         for (i=0;i<width;i++) {
            *ptr2++ = pixel;
         }
      }
   }
}


/* Optimized code provided by Nozomi Ytow <noz@xfree86.org> */
static void
clear_24bit_ximage( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = xmesa->clearcolor[0];
   const GLubyte g = xmesa->clearcolor[1];
   const GLubyte b = xmesa->clearcolor[2];
#if 0	/* See below */
   register GLuint clearPixel;
   if (xmesa->swapbytes) {
      clearPixel = (b << 16) | (g << 8) | r;
   }
   else {
      clearPixel = (r << 16) | (g << 8) | b;
   }
#endif

   if (all) {
      if (r==g && g==b) {
         /* same value for all three components (gray) */
         const GLint w3 = xmesa->xm_draw_buffer->width * 3;
         const GLint h = xmesa->xm_draw_buffer->height;
         GLint i;
         for (i = 0; i < h; i++) {
            bgr_t *ptr3 = PIXELADDR3(xmesa->xm_draw_buffer, 0, i);
            MEMSET(ptr3, r, w3);
         }
      }
      else {
         /* the usual case */
         const GLint w = xmesa->xm_draw_buffer->width;
         const GLint h = xmesa->xm_draw_buffer->height;
         GLint i, j;
         for (i = 0; i < h; i++) {
            bgr_t *ptr3 = PIXELADDR3(xmesa->xm_draw_buffer, 0, i);
            for (j = 0; j < w; j++) {
               ptr3->r = r;
               ptr3->g = g;
               ptr3->b = b;
               ptr3++;
            }
         }
#if 0 /* this code doesn't work for all window widths */
         register GLuint *ptr4 = (GLuint *) ptr3;
         register GLuint px;
         GLuint pixel4[3];
         register GLuint *p = pixel4;
         pixel4[0] = clearPixel | (clearPixel << 24);
         pixel4[1] = (clearPixel << 16) | (clearPixel >> 8);
         pixel4[2] = (clearPixel << 8) | (clearPixel >>  16);
         switch (3 & (int)(ptr3 - (bgr_t*) ptr4)){
            case 0:
               break;
            case 1:
               px = *ptr4 & 0x00ffffff;
               px |= pixel4[0] & 0xff000000;
               *ptr4++ = px;
               px = *ptr4 & 0xffff0000;
               px |= pixel4[2] & 0x0000ffff;
               *ptr4 = px;
               if (0 == --n)
                  break;
            case 2:
               px = *ptr4 & 0x0000fffff;
               px |= pixel4[1] & 0xffff0000;
               *ptr4++ = px;
               px = *ptr4 & 0xffffff00;
               px |= pixel4[2] & 0x000000ff;
               *ptr4 = px;
               if (0 == --n)
                  break;
            case 3:
               px = *ptr4 & 0x000000ff;
               px |= pixel4[2] & 0xffffff00;
               *ptr4++ = px;
               --n;
               break;
         }
         while (n > 3) {
            p = pixel4;
            *ptr4++ = *p++;
            *ptr4++ = *p++;
            *ptr4++ = *p++;
            n -= 4;
         }
         switch (n) {
            case 3:
               p = pixel4;
               *ptr4++ = *p++;
               *ptr4++ = *p++;
               px = *ptr4 & 0xffffff00;
               px |= clearPixel & 0xff;
               *ptr4 = px;
               break;
            case 2:
               p = pixel4;
               *ptr4++ = *p++;
               px = *ptr4 & 0xffff0000;
               px |= *p & 0xffff;
               *ptr4 = px;
               break;
            case 1:
               px = *ptr4 & 0xff000000;
               px |= *p & 0xffffff;
               *ptr4 = px;
               break;
            case 0:
               break;
         }
#endif
      }
   }
   else {
      /* only clear subrect of color buffer */
      if (r==g && g==b) {
         /* same value for all three components (gray) */
         GLint j;
         for (j=0;j<height;j++) {
            bgr_t *ptr3 = PIXELADDR3( xmesa->xm_draw_buffer, x, y+j );
            MEMSET(ptr3, r, 3 * width);
         }
      }
      else {
         /* non-gray clear color */
         GLint i, j;
         for (j = 0; j < height; j++) {
            bgr_t *ptr3 = PIXELADDR3( xmesa->xm_draw_buffer, x, y+j );
            for (i = 0; i < width; i++) {
               ptr3->r = r;
               ptr3->g = g;
               ptr3->b = b;
               ptr3++;
            }
         }
#if 0 /* this code might not always (seems ptr3 always == ptr4) */
         GLint j;
         GLuint pixel4[3];
         pixel4[0] = clearPixel | (clearPixel << 24);
         pixel4[1] = (clearPixel << 16) | (clearPixel >> 8);
         pixel4[2] = (clearPixel << 8) | (clearPixel >>  16);
         for (j=0;j<height;j++) {
            bgr_t *ptr3 = PIXELADDR3( xmesa->xm_draw_buffer, x, y+j );
            register GLuint *ptr4 = (GLuint *)ptr3;
            register GLuint *p, px;
            GLuint w = width;
            switch (3 & (int)(ptr3 - (bgr_t*) ptr4)){
               case 0:
                  break;
               case 1:
                  px = *ptr4 & 0x00ffffff;
                  px |= pixel4[0] & 0xff000000;
                  *ptr4++ = px;
                  px = *ptr4 & 0xffff0000;
                  px |= pixel4[2] & 0x0000ffff;
                  *ptr4 = px;
                  if (0 == --w)
                     break;
               case 2:
                  px = *ptr4 & 0x0000fffff;
                  px |= pixel4[1] & 0xffff0000;
                  *ptr4++ = px;
                  px = *ptr4 & 0xffffff00;
                  px |= pixel4[2] & 0x000000ff;
                  *ptr4 = px;
                  if (0 == --w)
                     break;
               case 3:
                  px = *ptr4 & 0x000000ff;
                  px |= pixel4[2] & 0xffffff00;
                  *ptr4++ = px;
                  --w;
                  break;
            }
            while (w > 3){
               p = pixel4;
               *ptr4++ = *p++;
               *ptr4++ = *p++;
               *ptr4++ = *p++;
               w -= 4;
            }
            switch (w) {
               case 3:
                  p = pixel4;
                  *ptr4++ = *p++;
                  *ptr4++ = *p++;
                  px = *ptr4 & 0xffffff00;
                  px |= *p & 0xff;
                  *ptr4 = px;
                  break;
               case 2:
                  p = pixel4;
                  *ptr4++ = *p++;
                  px = *ptr4 & 0xffff0000;
                  px |= *p & 0xffff;
                  *ptr4 = px;
                  break;
               case 1:
                  px = *ptr4 & 0xff000000;
                  px |= pixel4[0] & 0xffffff;
                  *ptr4 = px;
                  break;
               case 0:
                  break;
            }
         }
#endif
      }
   }
}


static void
clear_32bit_ximage( GLcontext *ctx, GLboolean all,
                    GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint pixel = (GLuint) xmesa->clearpixel;
   if (xmesa->swapbytes) {
      pixel = ((pixel >> 24) & 0x000000ff)
            | ((pixel >> 8)  & 0x0000ff00)
            | ((pixel << 8)  & 0x00ff0000)
            | ((pixel << 24) & 0xff000000);
   }
   if (all) {
      register GLint n = xmesa->xm_draw_buffer->width * xmesa->xm_draw_buffer->height;
      register GLuint *ptr4 = (GLuint *) xmesa->xm_draw_buffer->backimage->data;
      if (pixel==0) {
         MEMSET( ptr4, pixel, 4*n );
      }
      else {
         do {
            *ptr4++ = pixel;
            n--;
         } while (n!=0);
      }
   }
   else {
      register int i, j;
      for (j=0;j<height;j++) {
         register GLuint *ptr4 = PIXELADDR4( xmesa->xm_draw_buffer, x, y+j );
         for (i=0;i<width;i++) {
            *ptr4++ = pixel;
         }
      }
   }
}


static void
clear_nbit_ximage( GLcontext *ctx, GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xmesa->xm_draw_buffer->backimage;
   if (all) {
      register int i, j;
      width = xmesa->xm_draw_buffer->width;
      height = xmesa->xm_draw_buffer->height;
      for (j=0;j<height;j++) {
         for (i=0;i<width;i++) {
            XMesaPutPixel( img, i, j, xmesa->clearpixel );
         }
      }
   }
   else {
      /* TODO: optimize this */
      register int i, j;
      y = FLIP(xmesa->xm_draw_buffer, y);
      for (j=0;j<height;j++) {
         for (i=0;i<width;i++) {
            XMesaPutPixel( img, x+i, y-j, xmesa->clearpixel );
         }
      }
   }
}



static void
clear_buffers( GLcontext *ctx, GLbitfield mask,
               GLboolean all, GLint x, GLint y, GLint width, GLint height )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask;

   if ((mask & (DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT)) &&
       xmesa->xm_draw_buffer->mesa_buffer.UseSoftwareAlphaBuffers &&
       ctx->Color.ColorMask[ACOMP]) {
      _swrast_clear_alpha_buffers(ctx);
   }

   /* we can't handle color or index masking */
   if (*colorMask == 0xffffffff && ctx->Color.IndexMask == 0xffffffff) {
      if (mask & DD_FRONT_LEFT_BIT) {
	 ASSERT(xmesa->xm_draw_buffer->front_clear_func);
	 (*xmesa->xm_draw_buffer->front_clear_func)( ctx, all, x, y, width, height );
	 mask &= ~DD_FRONT_LEFT_BIT;
      }
      if (mask & DD_BACK_LEFT_BIT) {
	 ASSERT(xmesa->xm_draw_buffer->back_clear_func);
	 (*xmesa->xm_draw_buffer->back_clear_func)( ctx, all, x, y, width, height );
	 mask &= ~DD_BACK_LEFT_BIT;
      }
   }

   if (mask)
      _swrast_Clear( ctx, mask, all, x, y, width, height );
}


/*
 * When we detect that the user has resized the window this function will
 * get called.  Here we'll reallocate the back buffer, depth buffer,
 * stencil buffer etc. to match the new window size.
 */
void
xmesa_resize_buffers( GLframebuffer *buffer )
{
   int height = (int) buffer->Height;
   /* We can do this cast because the first field in the XMesaBuffer
    * struct is a GLframebuffer struct.  If this weren't true, we'd
    * need a pointer from the GLframebuffer to the XMesaBuffer.
    */
   XMesaBuffer xmBuffer = (XMesaBuffer) buffer;

   xmBuffer->width = buffer->Width;
   xmBuffer->height = buffer->Height;
   xmesa_alloc_back_buffer( xmBuffer );

   /* Needed by FLIP macro */
   xmBuffer->bottom = height - 1;

   if (xmBuffer->backimage) {
      /* Needed by PIXELADDR1 macro */
      xmBuffer->ximage_width1 = xmBuffer->backimage->bytes_per_line;
      xmBuffer->ximage_origin1 = (GLubyte *) xmBuffer->backimage->data
         + xmBuffer->ximage_width1 * (height-1);

      /* Needed by PIXELADDR2 macro */
      xmBuffer->ximage_width2 = xmBuffer->backimage->bytes_per_line / 2;
      xmBuffer->ximage_origin2 = (GLushort *) xmBuffer->backimage->data
         + xmBuffer->ximage_width2 * (height-1);

      /* Needed by PIXELADDR3 macro */
      xmBuffer->ximage_width3 = xmBuffer->backimage->bytes_per_line;
      xmBuffer->ximage_origin3 = (GLubyte *) xmBuffer->backimage->data
         + xmBuffer->ximage_width3 * (height-1);

      /* Needed by PIXELADDR4 macro */
      xmBuffer->ximage_width4 = xmBuffer->backimage->width;
      xmBuffer->ximage_origin4 = (GLuint *) xmBuffer->backimage->data
         + xmBuffer->ximage_width4 * (height-1);
   }

   _swrast_alloc_buffers( buffer );
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
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT( ctx );
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   const XMesaDrawable buffer = xmesa->xm_draw_buffer->buffer;
   const XMesaGC gc = xmesa->xm_draw_buffer->gc;

   ASSERT(dpy);
   ASSERT(gc);
   ASSERT(xmesa->xm_visual->dithered_pf == PF_8R8G8B);
   ASSERT(xmesa->xm_visual->undithered_pf == PF_8R8G8B);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (buffer &&   /* buffer != 0 means it's a Window or Pixmap */
       format == GL_BGRA &&
       type == GL_UNSIGNED_BYTE &&
       (swrast->_RasterMask & ~CLIP_BIT) == 0 && /* no blend, z-test, etc */
       ctx->_ImageTransferState == 0 &&  /* no color tables, scale/bias, etc */
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0) {
      int dstX = x;
      int dstY = y;
      int w = width;
      int h = height;
      int srcX = unpack->SkipPixels;
      int srcY = unpack->SkipRows;
      int rowLength = unpack->RowLength ? unpack->RowLength : width;

      pixels = _swrast_validate_pbo_access(unpack, width, height, 1,
                                           format, type, (GLvoid *) pixels);
      if (!pixels)
         return;

      if (_swrast_clip_pixelrect(ctx, &dstX, &dstY, &w, &h, &srcX, &srcY)) {
         /* This is a little tricky since all coordinates up to now have
          * been in the OpenGL bottom-to-top orientation.  X is top-to-bottom
          * so we have to carefully compute the Y coordinates/addresses here.
          */
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
         dstY = FLIP(xmesa->xm_draw_buffer, dstY) - h + 1;
         XPutImage(dpy, buffer, gc, &ximage, 0, 0, dstX, dstY, w, h);
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
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT( ctx );
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   const XMesaDrawable buffer = xmesa->xm_draw_buffer->buffer;
   const XMesaGC gc = xmesa->xm_draw_buffer->gc;

   ASSERT(dpy);
   ASSERT(gc);
   ASSERT(xmesa->xm_visual->undithered_pf == PF_5R6G5B);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (buffer &&   /* buffer != 0 means it's a Window or Pixmap */
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
      int srcX = unpack->SkipPixels;
      int srcY = unpack->SkipRows;
      int rowLength = unpack->RowLength ? unpack->RowLength : width;

      pixels = _swrast_validate_pbo_access(unpack, width, height, 1,
                                           format, type, (GLvoid *) pixels);
      if (!pixels)
         return;

      if (_swrast_clip_pixelrect(ctx, &dstX, &dstY, &w, &h, &srcX, &srcY)) {
         /* This is a little tricky since all coordinates up to now have
          * been in the OpenGL bottom-to-top orientation.  X is top-to-bottom
          * so we have to carefully compute the Y coordinates/addresses here.
          */
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
         dstY = FLIP(xmesa->xm_draw_buffer, dstY) - h + 1;
         XPutImage(dpy, buffer, gc, &ximage, 0, 0, dstX, dstY, w, h);
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
   const XMesaDrawable drawBuffer = xmesa->xm_draw_buffer->buffer;
   const XMesaDrawable readBuffer = xmesa->xm_read_buffer->buffer;
   const XMesaGC gc = xmesa->xm_draw_buffer->gc;

   ASSERT(dpy);
   ASSERT(gc);

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (ctx->Color.DrawBuffer == GL_FRONT &&
       ctx->Pixel.ReadBuffer == GL_FRONT &&
       drawBuffer &&  /* buffer != 0 means it's a Window or Pixmap */
       readBuffer &&
       type == GL_COLOR &&
       (swrast->_RasterMask & ~CLIP_BIT) == 0 && /* no blend, z-test, etc */
       ctx->_ImageTransferState == 0 &&  /* no color tables, scale/bias, etc */
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0) {
      /* Note: we don't do any special clipping work here.  We could,
       * but X will do it for us.
       */
      srcy = FLIP(xmesa->xm_read_buffer, srcy) - height + 1;
      desty = FLIP(xmesa->xm_draw_buffer, desty) - height + 1;
      XCopyArea(dpy, readBuffer, drawBuffer, gc,
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


void xmesa_update_state( GLcontext *ctx, GLuint new_state )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   /* Propogate statechange information to swrast and swrast_setup
    * modules.  The X11 driver has no internal GL-dependent state.
    */
   _swrast_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );


   /* setup pointers to front and back buffer clear functions */
   xmesa->xm_draw_buffer->front_clear_func = clear_front_pixmap;
   if (xmesa->xm_draw_buffer->backpixmap != XIMAGE) {
      xmesa->xm_draw_buffer->back_clear_func = clear_back_pixmap;
   }
   else if (sizeof(GLushort)!=2 || sizeof(GLuint)!=4) {
      xmesa->xm_draw_buffer->back_clear_func = clear_nbit_ximage;
   }
   else switch (xmesa->xm_visual->BitsPerPixel) {
   case 8:
      if (xmesa->xm_visual->hpcr_clear_flag) {
	 xmesa->xm_draw_buffer->back_clear_func = clear_HPCR_ximage;
      }
      else {
	 xmesa->xm_draw_buffer->back_clear_func = clear_8bit_ximage;
      }
      break;
   case 16:
      xmesa->xm_draw_buffer->back_clear_func = clear_16bit_ximage;
      break;
   case 24:
      xmesa->xm_draw_buffer->back_clear_func = clear_24bit_ximage;
      break;
   case 32:
      xmesa->xm_draw_buffer->back_clear_func = clear_32bit_ximage;
      break;
   default:
      xmesa->xm_draw_buffer->back_clear_func = clear_nbit_ximage;
      break;
   }

   if (ctx->Color._DrawDestMask & (DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT)) {
      xmesa_update_span_funcs(ctx);
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
 * Initialize the device driver function table with the functions
 * we implement in this driver.
 */
void xmesa_init_driver_functions( XMesaVisual xmvisual,
                                  struct dd_function_table *driver )
{
   driver->GetString = get_string;
   driver->UpdateState = xmesa_update_state;
   driver->GetBufferSize = get_buffer_size;
   driver->Flush = finish_or_flush;
   driver->Finish = finish_or_flush;
   driver->ClearIndex = clear_index;
   driver->ClearColor = clear_color;
   driver->IndexMask = index_mask;
   driver->ColorMask = color_mask;
   driver->Enable = enable;
   driver->Clear = clear_buffers;
   driver->ResizeBuffers = xmesa_resize_buffers;
#ifndef XFree86Server
   driver->CopyPixels = /*_swrast_CopyPixels;*/xmesa_CopyPixels;
   if (xmvisual->undithered_pf == PF_8R8G8B &&
       xmvisual->dithered_pf == PF_8R8G8B) {
      driver->DrawPixels = xmesa_DrawPixels_8R8G8B;
   }
   else if (xmvisual->undithered_pf == PF_5R6G5B) {
      driver->DrawPixels = xmesa_DrawPixels_5R6G5B;
   }
#endif
   driver->TestProxyTexImage = test_proxy_teximage;
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


/* Extend the software rasterizer with our line/point/triangle
 * functions.
 */
void xmesa_register_swrast_functions( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );
   struct swrast_device_driver *dd = _swrast_GetDeviceDriverReference(ctx);

   dd->SetBuffer = xmesa_set_buffer;

   swrast->choose_point = xmesa_choose_point;
   swrast->choose_line = xmesa_choose_line;
   swrast->choose_triangle = xmesa_choose_triangle;

   swrast->invalidate_point |= XMESA_NEW_POINT;
   swrast->invalidate_line |= XMESA_NEW_LINE;
   swrast->invalidate_triangle |= XMESA_NEW_TRIANGLE;
}
