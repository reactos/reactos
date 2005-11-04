/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

/*
 * This file contains the implementations of all the XMesa* functions.
 *
 *
 * NOTES:
 *
 * The window coordinate system origin (0,0) is in the lower-left corner
 * of the window.  X11's window coordinate origin is in the upper-left
 * corner of the window.  Therefore, most drawing functions in this
 * file have to flip Y coordinates.
 *
 * Define USE_XSHM in the Makefile with -DUSE_XSHM if you want to compile
 * in support for the MIT Shared Memory extension.  If enabled, when you
 * use an Ximage for the back buffer in double buffered mode, the "swap"
 * operation will be faster.  You must also link with -lXext.
 *
 * Byte swapping:  If the Mesa host and the X display use a different
 * byte order then there's some trickiness to be aware of when using
 * XImages.  The byte ordering used for the XImage is that of the X
 * display, not the Mesa host.
 * The color-to-pixel encoding for True/DirectColor must be done
 * according to the display's visual red_mask, green_mask, and blue_mask.
 * If XPutPixel is used to put a pixel into an XImage then XPutPixel will
 * do byte swapping if needed.  If one wants to directly "poke" the pixel
 * into the XImage's buffer then the pixel must be byte swapped first.  In
 * Mesa, when byte swapping is needed we use the PF_TRUECOLOR pixel format
 * and use XPutPixel everywhere except in the implementation of
 * glClear(GL_COLOR_BUFFER_BIT).  We want this function to be fast so
 * instead of using XPutPixel we "poke" our values after byte-swapping
 * the clear pixel value if needed.
 *
 */

#ifdef __CYGWIN__
#undef WIN32
#undef __WIN32__
#endif

#include "glxheader.h"
#include "GL/xmesa.h"
#include "xmesaP.h"
#include "context.h"
#include "extensions.h"
#include "framebuffer.h"
#include "glthread.h"
#include "imports.h"
#include "matrix.h"
#include "mtypes.h"
#include "macros.h"
#include "renderbuffer.h"
#include "texformat.h"
#include "texobj.h"
#include "texstore.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"

#ifdef XFree86Server
#include <GL/glxtokens.h>
#endif

/*
 * Global X driver lock
 */
_glthread_Mutex _xmesa_lock;



/*
 * Lookup tables for HPCR pixel format:
 */
static short hpcr_rgbTbl[3][256] = {
{
 16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  23,
 24,  24,  25,  25,  26,  26,  27,  27,  28,  28,  29,  29,  30,  30,  31,  31,
 32,  32,  33,  33,  34,  34,  35,  35,  36,  36,  37,  37,  38,  38,  39,  39,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239
},
{
 16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  23,
 24,  24,  25,  25,  26,  26,  27,  27,  28,  28,  29,  29,  30,  30,  31,  31,
 32,  32,  33,  33,  34,  34,  35,  35,  36,  36,  37,  37,  38,  38,  39,  39,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239
},
{
 32,  32,  33,  33,  34,  34,  35,  35,  36,  36,  37,  37,  38,  38,  39,  39,
 40,  40,  41,  41,  42,  42,  43,  43,  44,  44,  45,  45,  46,  46,  47,  47,
 48,  48,  49,  49,  50,  50,  51,  51,  52,  52,  53,  53,  54,  54,  55,  55,
 56,  56,  57,  57,  58,  58,  59,  59,  60,  60,  61,  61,  62,  62,  63,  63,
 64,  64,  65,  65,  66,  66,  67,  67,  68,  68,  69,  69,  70,  70,  71,  71,
 72,  72,  73,  73,  74,  74,  75,  75,  76,  76,  77,  77,  78,  78,  79,  79,
 80,  80,  81,  81,  82,  82,  83,  83,  84,  84,  85,  85,  86,  86,  87,  87,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223
}
};



/**********************************************************************/
/*****                     X Utility Functions                    *****/
/**********************************************************************/


/*
 * Return the host's byte order as LSBFirst or MSBFirst ala X.
 */
#ifndef XFree86Server
static int host_byte_order( void )
{
   int i = 1;
   char *cptr = (char *) &i;
   return (*cptr==1) ? LSBFirst : MSBFirst;
}
#endif


/*
 * Error handling.
 */
#ifndef XFree86Server
static volatile int mesaXErrorFlag = 0;

static int mesaHandleXError( XMesaDisplay *dpy, XErrorEvent *event )
{
   (void) dpy;
   (void) event;
   mesaXErrorFlag = 1;
   return 0;
}
#endif


/*
 * Check if the X Shared Memory extension is available.
 * Return:  0 = not available
 *          1 = shared XImage support available
 *          2 = shared Pixmap support available also
 */
#ifndef XFree86Server
static int check_for_xshm( XMesaDisplay *display )
{
#ifdef USE_XSHM
   int major, minor, ignore;
   Bool pixmaps;

   if (XQueryExtension( display, "MIT-SHM", &ignore, &ignore, &ignore )) {
      if (XShmQueryVersion( display, &major, &minor, &pixmaps )==True) {
	 return (pixmaps==True) ? 2 : 1;
      }
      else {
	 return 0;
      }
   }
   else {
      return 0;
   }
#else
   /* Can't compile XSHM support */
   return 0;
#endif
}
#endif


/*
 * Apply gamma correction to an intensity value in [0..max].  Return the
 * new intensity value.
 */
static GLint gamma_adjust( GLfloat gamma, GLint value, GLint max )
{
   if (gamma == 1.0) {
      return value;
   }
   else {
      double x = (double) value / (double) max;
      return IROUND_POS((GLfloat) max * _mesa_pow(x, 1.0F/gamma));
   }
}



/*
 * Return the true number of bits per pixel for XImages.
 * For example, if we request a 24-bit deep visual we may actually need/get
 * 32bpp XImages.  This function returns the appropriate bpp.
 * Input:  dpy - the X display
 *         visinfo - desribes the visual to be used for XImages
 * Return:  true number of bits per pixel for XImages
 */
#ifdef XFree86Server

static int bits_per_pixel( XMesaVisual xmv )
{
   const int depth = xmv->nplanes;
   int i;
   for (i = 0; i < screenInfo.numPixmapFormats; i++) {
      if (screenInfo.formats[i].depth == depth)
         return screenInfo.formats[i].bitsPerPixel;
   }
   return depth;  /* should never get here, but this should be safe */
}

#else

static int bits_per_pixel( XMesaVisual xmv )
{
   XMesaDisplay *dpy = xmv->display;
   XMesaVisualInfo visinfo = xmv->visinfo;
   XMesaImage *img;
   int bitsPerPixel;
   /* Create a temporary XImage */
   img = XCreateImage( dpy, visinfo->visual, visinfo->depth,
		       ZPixmap, 0,           /*format, offset*/
		       (char*) MALLOC(8),    /*data*/
		       1, 1,                 /*width, height*/
		       32,                   /*bitmap_pad*/
		       0                     /*bytes_per_line*/
                     );
   assert(img);
   /* grab the bits/pixel value */
   bitsPerPixel = img->bits_per_pixel;
   /* free the XImage */
   _mesa_free( img->data );
   img->data = NULL;
   XMesaDestroyImage( img );
   return bitsPerPixel;
}
#endif



/*
 * Determine if a given X window ID is valid (window exists).
 * Do this by calling XGetWindowAttributes() for the window and
 * checking if we catch an X error.
 * Input:  dpy - the display
 *         win - the window to check for existance
 * Return:  GL_TRUE - window exists
 *          GL_FALSE - window doesn't exist
 */
#ifndef XFree86Server
static GLboolean WindowExistsFlag;

static int window_exists_err_handler( XMesaDisplay* dpy, XErrorEvent* xerr )
{
   (void) dpy;
   if (xerr->error_code == BadWindow) {
      WindowExistsFlag = GL_FALSE;
   }
   return 0;
}

static GLboolean window_exists( XMesaDisplay *dpy, Window win )
{
   XWindowAttributes wa;
   int (*old_handler)( XMesaDisplay*, XErrorEvent* );
   WindowExistsFlag = GL_TRUE;
   old_handler = XSetErrorHandler(window_exists_err_handler);
   XGetWindowAttributes( dpy, win, &wa ); /* dummy request */
   XSetErrorHandler(old_handler);
   return WindowExistsFlag;
}
#endif



/**********************************************************************/
/*****                Linked list of XMesaBuffers                 *****/
/**********************************************************************/

static XMesaBuffer XMesaBufferList = NULL;


/**
 * Allocate a new XMesaBuffer, initialize basic fields and add to
 * the list of all buffers.
 */
static XMesaBuffer
alloc_xmesa_buffer(XMesaVisual vis, BufferType type, XMesaColormap cmap)
{
   XMesaBuffer b = (XMesaBuffer) CALLOC_STRUCT(xmesa_buffer);
   if (b) {
      GLboolean swAlpha;

      b->display = vis->display;
      b->xm_visual = vis;
      b->type = type;
      b->cmap = cmap;

      _mesa_initialize_framebuffer(&b->mesa_buffer, &vis->mesa_visual);

      /* determine back buffer implementation */
      if (vis->mesa_visual.doubleBufferMode) {
         if (vis->ximage_flag) {
            b->db_state = BACK_XIMAGE;
         }
         else {
            b->db_state = BACK_PIXMAP;
         }
      }
      else {
         b->db_state = 0;
      }

      /* Allocate the framebuffer's renderbuffers */
      assert(!b->mesa_buffer.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
      assert(!b->mesa_buffer.Attachment[BUFFER_BACK_LEFT].Renderbuffer);

      /* front renderbuffer */
      b->frontxrb = xmesa_new_renderbuffer(NULL, 0, vis->mesa_visual.rgbMode);
      _mesa_add_renderbuffer(&b->mesa_buffer, BUFFER_FRONT_LEFT,
                             &b->frontxrb->Base);

      /* back renderbuffer */
      if (vis->mesa_visual.doubleBufferMode) {
         b->backxrb =xmesa_new_renderbuffer(NULL, 0, vis->mesa_visual.rgbMode);
         _mesa_add_renderbuffer(&b->mesa_buffer, BUFFER_BACK_LEFT,
                                &b->backxrb->Base);
      }

      /* determine if we need software alpha planes */
      if (vis->mesa_visual.alphaBits > 0
          && vis->undithered_pf != PF_8A8B8G8R
          && vis->undithered_pf != PF_8A8R8G8B) {
         /* Visual has alpha, but pixel format doesn't support it.
          * We'll use an alpha renderbuffer wrapper.
          */
         swAlpha = GL_TRUE;
      }
      else {
         swAlpha = GL_FALSE;
      }

      _mesa_add_soft_renderbuffers(&b->mesa_buffer,
                                   GL_FALSE,  /* color */
                                   vis->mesa_visual.haveDepthBuffer,
                                   vis->mesa_visual.haveStencilBuffer,
                                   vis->mesa_visual.haveAccumBuffer,
                                   swAlpha,
                                   vis->mesa_visual.numAuxBuffers > 0 );

      /* insert into linked list */
      b->Next = XMesaBufferList;
      XMesaBufferList = b;
   }
   return b;
}


/*
 * Find an XMesaBuffer by matching X display and colormap but NOT matching
 * the notThis buffer.
 */
static XMesaBuffer find_xmesa_buffer(XMesaDisplay *dpy,
                                     XMesaColormap cmap,
                                     XMesaBuffer notThis)
{
   XMesaBuffer b;
   for (b=XMesaBufferList; b; b=b->Next) {
      if (b->display==dpy && b->cmap==cmap && b!=notThis) {
         return b;
      }
   }
   return NULL;
}


/*
 * Free an XMesaBuffer, remove from linked list, perhaps free X colormap
 * entries.
 */
static void free_xmesa_buffer(int client, XMesaBuffer buffer)
{
   XMesaBuffer prev = NULL, b;
   (void) client;
   for (b=XMesaBufferList; b; b=b->Next) {
      if (b==buffer) {
         /* unlink bufer from list */
         if (prev)
            prev->Next = buffer->Next;
         else
            XMesaBufferList = buffer->Next;
         /* Check to free X colors */
         if (buffer->num_alloced>0) {
            /* If no other buffer uses this X colormap then free the colors. */
            if (!find_xmesa_buffer(buffer->display, buffer->cmap, buffer)) {
#ifdef XFree86Server
               (void)FreeColors(buffer->cmap, client,
				buffer->num_alloced, buffer->alloced_colors,
				0);
#else
               XFreeColors(buffer->display, buffer->cmap,
                           buffer->alloced_colors, buffer->num_alloced, 0);
#endif
            }
         }

         _mesa_free_framebuffer_data(&buffer->mesa_buffer);
         _mesa_free(buffer);

         return;
      }
      /* continue search */
      prev = b;
   }
   /* buffer not found in XMesaBufferList */
   _mesa_problem(NULL,"free_xmesa_buffer() - buffer not found\n");
}


/* Copy X color table stuff from one XMesaBuffer to another. */
static void copy_colortable_info(XMesaBuffer dst, const XMesaBuffer src)
{
   MEMCPY(dst->color_table, src->color_table, sizeof(src->color_table));
   MEMCPY(dst->pixel_to_r, src->pixel_to_r, sizeof(src->pixel_to_r));
   MEMCPY(dst->pixel_to_g, src->pixel_to_g, sizeof(src->pixel_to_g));
   MEMCPY(dst->pixel_to_b, src->pixel_to_b, sizeof(src->pixel_to_b));
   dst->num_alloced = src->num_alloced;
   MEMCPY(dst->alloced_colors, src->alloced_colors,
          sizeof(src->alloced_colors));
}



/**********************************************************************/
/*****                   Misc Private Functions                   *****/
/**********************************************************************/


/*
 * Return number of bits set in n.
 */
static int bitcount( unsigned long n )
{
   int bits;
   for (bits=0; n>0; n=n>>1) {
      if (n&1) {
         bits++;
      }
   }
   return bits;
}



/**
 * Allocate a shared memory XImage back buffer for the given XMesaBuffer.
 * Return:  GL_TRUE if success, GL_FALSE if error
 */
#ifndef XFree86Server
static GLboolean
alloc_shm_back_buffer(XMesaBuffer b, GLuint width, GLuint height)
{
#ifdef USE_XSHM
   /*
    * We have to do a _lot_ of error checking here to be sure we can
    * really use the XSHM extension.  It seems different servers trigger
    * errors at different points if the extension won't work.  Therefore
    * we have to be very careful...
    */
   GC gc;
   int (*old_handler)( XMesaDisplay *, XErrorEvent * );

   if (width == 0 || height == 0) {
      /* this will be true the first time we're called on 'b' */
      return GL_FALSE;
   }

   b->backxrb->ximage = XShmCreateImage(b->xm_visual->display,
                                        b->xm_visual->visinfo->visual,
                                        b->xm_visual->visinfo->depth,
                                        ZPixmap, NULL, &b->shminfo,
                                        width, height);
   if (b->backxrb->ximage == NULL) {
      _mesa_warning(NULL, "alloc_back_buffer: Shared memory error (XShmCreateImage), disabling.");
      b->shm = 0;
      return GL_FALSE;
   }

   b->shminfo.shmid = shmget( IPC_PRIVATE, b->backxrb->ximage->bytes_per_line
			     * b->backxrb->ximage->height, IPC_CREAT|0777 );
   if (b->shminfo.shmid < 0) {
      _mesa_warning(NULL, "shmget failed while allocating back buffer");
      XDestroyImage( b->backxrb->ximage );
      b->backxrb->ximage = NULL;
      _mesa_warning(NULL, "alloc_back_buffer: Shared memory error (shmget), disabling.");
      b->shm = 0;
      return GL_FALSE;
   }

   b->shminfo.shmaddr = b->backxrb->ximage->data
                      = (char*)shmat( b->shminfo.shmid, 0, 0 );
   if (b->shminfo.shmaddr == (char *) -1) {
      _mesa_warning(NULL, "shmat() failed while allocating back buffer");
      XDestroyImage( b->backxrb->ximage );
      shmctl( b->shminfo.shmid, IPC_RMID, 0 );
      b->backxrb->ximage = NULL;
      _mesa_warning(NULL, "alloc_back_buffer: Shared memory error (shmat), disabling.");
      b->shm = 0;
      return GL_FALSE;
   }

   b->shminfo.readOnly = False;
   mesaXErrorFlag = 0;
   old_handler = XSetErrorHandler( mesaHandleXError );
   /* This may trigger the X protocol error we're ready to catch: */
   XShmAttach( b->xm_visual->display, &b->shminfo );
   XSync( b->xm_visual->display, False );

   if (mesaXErrorFlag) {
      /* we are on a remote display, this error is normal, don't print it */
      XFlush( b->xm_visual->display );
      mesaXErrorFlag = 0;
      XDestroyImage( b->backxrb->ximage );
      shmdt( b->shminfo.shmaddr );
      shmctl( b->shminfo.shmid, IPC_RMID, 0 );
      b->backxrb->ximage = NULL;
      b->shm = 0;
      (void) XSetErrorHandler( old_handler );
      return GL_FALSE;
   }

   shmctl( b->shminfo.shmid, IPC_RMID, 0 ); /* nobody else needs it */

   /* Finally, try an XShmPutImage to be really sure the extension works */
   gc = XCreateGC( b->xm_visual->display, b->frontxrb->drawable, 0, NULL );
   XShmPutImage( b->xm_visual->display, b->frontxrb->drawable, gc,
		 b->backxrb->ximage, 0, 0, 0, 0, 1, 1 /*one pixel*/, False );
   XSync( b->xm_visual->display, False );
   XFreeGC( b->xm_visual->display, gc );
   (void) XSetErrorHandler( old_handler );
   if (mesaXErrorFlag) {
      XFlush( b->xm_visual->display );
      mesaXErrorFlag = 0;
      XDestroyImage( b->backxrb->ximage );
      shmdt( b->shminfo.shmaddr );
      shmctl( b->shminfo.shmid, IPC_RMID, 0 );
      b->backxrb->ximage = NULL;
      b->shm = 0;
      return GL_FALSE;
   }

   return GL_TRUE;
#else
   /* Can't compile XSHM support */
   return GL_FALSE;
#endif
}
#endif




/*
 * Setup an off-screen pixmap or Ximage to use as the back buffer.
 * Input:  b - the X/Mesa buffer
 */
void
xmesa_alloc_back_buffer( XMesaBuffer b, GLuint width, GLuint height )
{
   if (width == 0 || height == 0)
      return;

   if (b->db_state == BACK_XIMAGE) {
      /* Deallocate the old backxrb->ximage, if any */
      if (b->backxrb->ximage) {
#if defined(USE_XSHM) && !defined(XFree86Server)
	 if (b->shm) {
	    XShmDetach( b->xm_visual->display, &b->shminfo );
	    XDestroyImage( b->backxrb->ximage );
	    shmdt( b->shminfo.shmaddr );
	 }
	 else
#endif
	   XMesaDestroyImage( b->backxrb->ximage );
	 b->backxrb->ximage = NULL;
      }

      /* Allocate new back buffer */
#ifdef XFree86Server
      {
	 /* Allocate a regular XImage for the back buffer. */
	 b->backxrb->ximage = XMesaCreateImage(b->xm_visual->BitsPerPixel,
					 width, height, NULL);
#else
      if (b->shm == 0 || !alloc_shm_back_buffer(b, width, height)) {
	 /* Allocate a regular XImage for the back buffer. */
	 b->backxrb->ximage = XCreateImage( b->xm_visual->display,
                                      b->xm_visual->visinfo->visual,
                                      GET_VISUAL_DEPTH(b->xm_visual),
				      ZPixmap, 0,   /* format, offset */
				      NULL,
                                      width, height,
				      8, 0 );  /* pad, bytes_per_line */
#endif
	 if (!b->backxrb->ximage) {
	    _mesa_warning(NULL, "alloc_back_buffer: XCreateImage failed.");
	 }
         b->backxrb->ximage->data = (char *) MALLOC( b->backxrb->ximage->height
                                             * b->backxrb->ximage->bytes_per_line );
         if (!b->backxrb->ximage->data) {
            _mesa_warning(NULL, "alloc_back_buffer: MALLOC failed.");
            XMesaDestroyImage( b->backxrb->ximage );
            b->backxrb->ximage = NULL;
         }
         /* this call just updates the width/origin fields in the xrb */
         b->backxrb->Base.AllocStorage(NULL, &b->backxrb->Base, 
                                       b->backxrb->Base.InternalFormat,
                                       b->backxrb->ximage->width,
                                       b->backxrb->ximage->height);
      }
      b->backxrb->pixmap = None;
   }
   else if (b->db_state==BACK_PIXMAP) {
      if (!width)
         width = 1;
      if (!height)
         height = 1;

      /* Free the old back pixmap */
      if (b->backxrb->pixmap) {
	 XMesaFreePixmap( b->xm_visual->display, b->backxrb->pixmap );
      }
      /* Allocate new back pixmap */
      b->backxrb->pixmap = XMesaCreatePixmap( b->xm_visual->display,
                                              b->frontxrb->drawable,
                                              width, height,
                                              GET_VISUAL_DEPTH(b->xm_visual) );
      b->backxrb->ximage = NULL;
   }
}



/*
 * A replacement for XAllocColor.  This function should never
 * fail to allocate a color.  When XAllocColor fails, we return
 * the nearest matching color.  If we have to allocate many colors
 * this function isn't too efficient; the XQueryColors() could be
 * done just once.
 * Written by Michael Pichler, Brian Paul, Mark Kilgard
 * Input:  dpy - X display
 *         cmap - X colormap
 *         cmapSize - size of colormap
 * In/Out: color - the XColor struct
 * Output:  exact - 1=exact color match, 0=closest match
 *          alloced - 1=XAlloc worked, 0=XAlloc failed
 */
static void
noFaultXAllocColor( int client,
                    XMesaDisplay *dpy,
                    XMesaColormap cmap,
                    int cmapSize,
                    XMesaColor *color,
                    int *exact, int *alloced )
{
#ifdef XFree86Server
   Pixel *ppixIn;
   xrgb *ctable;
#else
   /* we'll try to cache ctable for better remote display performance */
   static Display *prevDisplay = NULL;
   static XMesaColormap prevCmap = 0;
   static int prevCmapSize = 0;
   static XMesaColor *ctable = NULL;
#endif
   XMesaColor subColor;
   int i, bestmatch;
   double mindist;       /* 3*2^16^2 exceeds long int precision. */

   (void) client;

   /* First try just using XAllocColor. */
#ifdef XFree86Server
   if (AllocColor(cmap,
		  &color->red, &color->green, &color->blue,
		  &color->pixel,
		  client) == Success) {
#else
   if (XAllocColor(dpy, cmap, color)) {
#endif
      *exact = 1;
      *alloced = 1;
      return;
   }

   /* Alloc failed, search for closest match */

   /* Retrieve color table entries. */
   /* XXX alloca candidate. */
#ifdef XFree86Server
   ppixIn = (Pixel *) MALLOC(cmapSize * sizeof(Pixel));
   ctable = (xrgb *) MALLOC(cmapSize * sizeof(xrgb));
   for (i = 0; i < cmapSize; i++) {
      ppixIn[i] = i;
   }
   QueryColors(cmap, cmapSize, ppixIn, ctable);
#else
   if (prevDisplay != dpy || prevCmap != cmap
       || prevCmapSize != cmapSize || !ctable) {
      /* free previously cached color table */
      if (ctable)
         _mesa_free(ctable);
      /* Get the color table from X */
      ctable = (XMesaColor *) MALLOC(cmapSize * sizeof(XMesaColor));
      assert(ctable);
      for (i = 0; i < cmapSize; i++) {
         ctable[i].pixel = i;
      }
      XQueryColors(dpy, cmap, ctable, cmapSize);
      prevDisplay = dpy;
      prevCmap = cmap;
      prevCmapSize = cmapSize;
   }
#endif

   /* Find best match. */
   bestmatch = -1;
   mindist = 0.0;
   for (i = 0; i < cmapSize; i++) {
      double dr = 0.30 * ((double) color->red - (double) ctable[i].red);
      double dg = 0.59 * ((double) color->green - (double) ctable[i].green);
      double db = 0.11 * ((double) color->blue - (double) ctable[i].blue);
      double dist = dr * dr + dg * dg + db * db;
      if (bestmatch < 0 || dist < mindist) {
         bestmatch = i;
         mindist = dist;
      }
   }

   /* Return result. */
   subColor.red   = ctable[bestmatch].red;
   subColor.green = ctable[bestmatch].green;
   subColor.blue  = ctable[bestmatch].blue;
   /* Try to allocate the closest match color.  This should only
    * fail if the cell is read/write.  Otherwise, we're incrementing
    * the cell's reference count.
    */
#ifdef XFree86Server
   if (AllocColor(cmap,
		  &subColor.red, &subColor.green, &subColor.blue,
		  &subColor.pixel,
		  client) == Success) {
#else
   if (XAllocColor(dpy, cmap, &subColor)) {
#endif
      *alloced = 1;
   }
   else {
      /* do this to work around a problem reported by Frank Ortega */
      subColor.pixel = (unsigned long) bestmatch;
      subColor.red   = ctable[bestmatch].red;
      subColor.green = ctable[bestmatch].green;
      subColor.blue  = ctable[bestmatch].blue;
      subColor.flags = DoRed | DoGreen | DoBlue;
      *alloced = 0;
   }
#ifdef XFree86Server
   _mesa_free(ppixIn);
   _mesa_free(ctable);
#else
   /* don't free table, save it for next time */
#endif

   *color = subColor;
   *exact = 0;
}




/*
 * Do setup for PF_GRAYSCALE pixel format.
 * Note that buffer may be NULL.
 */
static GLboolean setup_grayscale( int client, XMesaVisual v,
                                  XMesaBuffer buffer, XMesaColormap cmap )
{
   if (GET_VISUAL_DEPTH(v)<4 || GET_VISUAL_DEPTH(v)>16) {
      return GL_FALSE;
   }

   if (buffer) {
      XMesaBuffer prevBuffer;

      if (!cmap) {
         return GL_FALSE;
      }

      prevBuffer = find_xmesa_buffer(v->display, cmap, buffer);
      if (prevBuffer &&
          (buffer->xm_visual->mesa_visual.rgbMode ==
           prevBuffer->xm_visual->mesa_visual.rgbMode)) {
         /* Copy colormap stuff from previous XMesaBuffer which uses same
          * X colormap.  Do this to avoid time spent in noFaultXAllocColor.
          */
         copy_colortable_info(buffer, prevBuffer);
      }
      else {
         /* Allocate 256 shades of gray */
         int gray;
         int colorsfailed = 0;
         for (gray=0;gray<256;gray++) {
            GLint r = gamma_adjust( v->RedGamma,   gray, 255 );
            GLint g = gamma_adjust( v->GreenGamma, gray, 255 );
            GLint b = gamma_adjust( v->BlueGamma,  gray, 255 );
            int exact, alloced;
            XMesaColor xcol;
            xcol.red   = (r << 8) | r;
            xcol.green = (g << 8) | g;
            xcol.blue  = (b << 8) | b;
            noFaultXAllocColor( client, v->display,
                                cmap, GET_COLORMAP_SIZE(v),
                                &xcol, &exact, &alloced );
            if (!exact) {
               colorsfailed++;
            }
            if (alloced) {
               assert(buffer->num_alloced<256);
               buffer->alloced_colors[buffer->num_alloced] = xcol.pixel;
               buffer->num_alloced++;
            }

            /*OLD
            assert(gray < 576);
            buffer->color_table[gray*3+0] = xcol.pixel;
            buffer->color_table[gray*3+1] = xcol.pixel;
            buffer->color_table[gray*3+2] = xcol.pixel;
            assert(xcol.pixel < 65536);
            buffer->pixel_to_r[xcol.pixel] = gray * 30 / 100;
            buffer->pixel_to_g[xcol.pixel] = gray * 59 / 100;
            buffer->pixel_to_b[xcol.pixel] = gray * 11 / 100;
            */
            buffer->color_table[gray] = xcol.pixel;
            assert(xcol.pixel < 65536);
            buffer->pixel_to_r[xcol.pixel] = gray;
            buffer->pixel_to_g[xcol.pixel] = gray;
            buffer->pixel_to_b[xcol.pixel] = gray;
         }

         if (colorsfailed && _mesa_getenv("MESA_DEBUG")) {
            _mesa_warning(NULL,
                  "Note: %d out of 256 needed colors do not match exactly.\n",
                  colorsfailed );
         }
      }
   }

   v->dithered_pf = PF_Grayscale;
   v->undithered_pf = PF_Grayscale;
   return GL_TRUE;
}



/*
 * Setup RGB rendering for a window with a PseudoColor, StaticColor,
 * or 8-bit TrueColor visual visual.  We try to allocate a palette of 225
 * colors (5 red, 9 green, 5 blue) and dither to approximate a 24-bit RGB
 * color.  While this function was originally designed just for 8-bit
 * visuals, it has also proven to work from 4-bit up to 16-bit visuals.
 * Dithering code contributed by Bob Mercier.
 */
static GLboolean setup_dithered_color( int client, XMesaVisual v,
                                       XMesaBuffer buffer, XMesaColormap cmap )
{
   if (GET_VISUAL_DEPTH(v)<4 || GET_VISUAL_DEPTH(v)>16) {
      return GL_FALSE;
   }

   if (buffer) {
      XMesaBuffer prevBuffer;

      if (!cmap) {
         return GL_FALSE;
      }

      prevBuffer = find_xmesa_buffer(v->display, cmap, buffer);
      if (prevBuffer &&
          (buffer->xm_visual->mesa_visual.rgbMode ==
           prevBuffer->xm_visual->mesa_visual.rgbMode)) {
         /* Copy colormap stuff from previous, matching XMesaBuffer.
          * Do this to avoid time spent in noFaultXAllocColor.
          */
         copy_colortable_info(buffer, prevBuffer);
      }
      else {
         /* Allocate X colors and initialize color_table[], red_table[], etc */
         int r, g, b, i;
         int colorsfailed = 0;
         for (r = 0; r < DITH_R; r++) {
            for (g = 0; g < DITH_G; g++) {
               for (b = 0; b < DITH_B; b++) {
                  XMesaColor xcol;
                  int exact, alloced;
                  xcol.red  =gamma_adjust(v->RedGamma,   r*65535/(DITH_R-1),65535);
                  xcol.green=gamma_adjust(v->GreenGamma, g*65535/(DITH_G-1),65535);
                  xcol.blue =gamma_adjust(v->BlueGamma,  b*65535/(DITH_B-1),65535);
                  noFaultXAllocColor( client, v->display,
                                      cmap, GET_COLORMAP_SIZE(v),
                                      &xcol, &exact, &alloced );
                  if (!exact) {
                     colorsfailed++;
                  }
                  if (alloced) {
                     assert(buffer->num_alloced<256);
                     buffer->alloced_colors[buffer->num_alloced] = xcol.pixel;
                     buffer->num_alloced++;
                  }
                  i = DITH_MIX( r, g, b );
                  assert(i < 576);
                  buffer->color_table[i] = xcol.pixel;
                  assert(xcol.pixel < 65536);
                  buffer->pixel_to_r[xcol.pixel] = r * 255 / (DITH_R-1);
                  buffer->pixel_to_g[xcol.pixel] = g * 255 / (DITH_G-1);
                  buffer->pixel_to_b[xcol.pixel] = b * 255 / (DITH_B-1);
               }
            }
         }

         if (colorsfailed && _mesa_getenv("MESA_DEBUG")) {
            _mesa_warning(NULL,
                  "Note: %d out of %d needed colors do not match exactly.\n",
                  colorsfailed, DITH_R * DITH_G * DITH_B );
         }
      }
   }

   v->dithered_pf = PF_Dither;
   v->undithered_pf = PF_Lookup;
   return GL_TRUE;
}


/*
 * Setup for Hewlett Packard Color Recovery 8-bit TrueColor mode.
 * HPCR simulates 24-bit color fidelity with an 8-bit frame buffer.
 * Special dithering tables have to be initialized.
 */
static void setup_8bit_hpcr( XMesaVisual v )
{
   /* HP Color Recovery contributed by:  Alex De Bruyn (ad@lms.be)
    * To work properly, the atom _HP_RGB_SMOOTH_MAP_LIST must be defined
    * on the root window AND the colormap obtainable by XGetRGBColormaps
    * for that atom must be set on the window.  (see also tkInitWindow)
    * If that colormap is not set, the output will look stripy.
    */

   /* Setup color tables with gamma correction */
   int i;
   double g;

   g = 1.0 / v->RedGamma;
   for (i=0; i<256; i++) {
      GLint red = IROUND_POS(255.0 * _mesa_pow( hpcr_rgbTbl[0][i]/255.0, g ));
      v->hpcr_rgbTbl[0][i] = CLAMP( red, 16, 239 );
   }

   g = 1.0 / v->GreenGamma;
   for (i=0; i<256; i++) {
      GLint green = IROUND_POS(255.0 * _mesa_pow( hpcr_rgbTbl[1][i]/255.0, g ));
      v->hpcr_rgbTbl[1][i] = CLAMP( green, 16, 239 );
   }

   g = 1.0 / v->BlueGamma;
   for (i=0; i<256; i++) {
      GLint blue = IROUND_POS(255.0 * _mesa_pow( hpcr_rgbTbl[2][i]/255.0, g ));
      v->hpcr_rgbTbl[2][i] = CLAMP( blue, 32, 223 );
   }
   v->undithered_pf = PF_HPCR;  /* can't really disable dithering for now */
   v->dithered_pf = PF_HPCR;

   /* which method should I use to clear */
   /* GL_FALSE: keep the ordinary method  */
   /* GL_TRUE : clear with dither pattern */
   v->hpcr_clear_flag = _mesa_getenv("MESA_HPCR_CLEAR") ? GL_TRUE : GL_FALSE;

   if (v->hpcr_clear_flag) {
      v->hpcr_clear_pixmap = XMesaCreatePixmap(v->display,
                                               DefaultRootWindow(v->display),
                                               16, 2, 8);
#ifndef XFree86Server
      v->hpcr_clear_ximage = XGetImage(v->display, v->hpcr_clear_pixmap,
                                       0, 0, 16, 2, AllPlanes, ZPixmap);
#endif
   }
}


/*
 * Setup RGB rendering for a window with a True/DirectColor visual.
 */
static void setup_truecolor( XMesaVisual v, XMesaBuffer buffer,
                             XMesaColormap cmap )
{
   unsigned long rmask, gmask, bmask;
   (void) buffer;
   (void) cmap;

   /* Compute red multiplier (mask) and bit shift */
   v->rshift = 0;
   rmask = GET_REDMASK(v);
   while ((rmask & 1)==0) {
      v->rshift++;
      rmask = rmask >> 1;
   }

   /* Compute green multiplier (mask) and bit shift */
   v->gshift = 0;
   gmask = GET_GREENMASK(v);
   while ((gmask & 1)==0) {
      v->gshift++;
      gmask = gmask >> 1;
   }

   /* Compute blue multiplier (mask) and bit shift */
   v->bshift = 0;
   bmask = GET_BLUEMASK(v);
   while ((bmask & 1)==0) {
      v->bshift++;
      bmask = bmask >> 1;
   }

   /*
    * Compute component-to-pixel lookup tables and dithering kernel
    */
   {
      static GLubyte kernel[16] = {
          0*16,  8*16,  2*16, 10*16,
         12*16,  4*16, 14*16,  6*16,
          3*16, 11*16,  1*16,  9*16,
         15*16,  7*16, 13*16,  5*16,
      };
      GLint rBits = bitcount(rmask);
      GLint gBits = bitcount(gmask);
      GLint bBits = bitcount(bmask);
      GLint maxBits;
      GLuint i;

      /* convert pixel components in [0,_mask] to RGB values in [0,255] */
      for (i=0; i<=rmask; i++)
         v->PixelToR[i] = (unsigned char) ((i * 255) / rmask);
      for (i=0; i<=gmask; i++)
         v->PixelToG[i] = (unsigned char) ((i * 255) / gmask);
      for (i=0; i<=bmask; i++)
         v->PixelToB[i] = (unsigned char) ((i * 255) / bmask);

      /* convert RGB values from [0,255] to pixel components */

      for (i=0;i<256;i++) {
         GLint r = gamma_adjust(v->RedGamma,   i, 255);
         GLint g = gamma_adjust(v->GreenGamma, i, 255);
         GLint b = gamma_adjust(v->BlueGamma,  i, 255);
         v->RtoPixel[i] = (r >> (8-rBits)) << v->rshift;
         v->GtoPixel[i] = (g >> (8-gBits)) << v->gshift;
         v->BtoPixel[i] = (b >> (8-bBits)) << v->bshift;
      }
      /* overflow protection */
      for (i=256;i<512;i++) {
         v->RtoPixel[i] = v->RtoPixel[255];
         v->GtoPixel[i] = v->GtoPixel[255];
         v->BtoPixel[i] = v->BtoPixel[255];
      }

      /* setup dithering kernel */
      maxBits = rBits;
      if (gBits > maxBits)  maxBits = gBits;
      if (bBits > maxBits)  maxBits = bBits;
      for (i=0;i<16;i++) {
         v->Kernel[i] = kernel[i] >> maxBits;
      }

      v->undithered_pf = PF_Truecolor;
      v->dithered_pf = (GET_VISUAL_DEPTH(v)<24) ? PF_Dither_True : PF_Truecolor;
   }

   /*
    * Now check for TrueColor visuals which we can optimize.
    */
   if (   GET_REDMASK(v)  ==0x0000ff
       && GET_GREENMASK(v)==0x00ff00
       && GET_BLUEMASK(v) ==0xff0000
       && CHECK_BYTE_ORDER(v)
       && v->BitsPerPixel==32
       && sizeof(GLuint)==4
       && v->RedGamma==1.0 && v->GreenGamma==1.0 && v->BlueGamma==1.0) {
      /* common 32 bpp config used on SGI, Sun */
      v->undithered_pf = v->dithered_pf = PF_8A8B8G8R;
   }
   else if (GET_REDMASK(v)  ==0xff0000
       &&   GET_GREENMASK(v)==0x00ff00
       &&   GET_BLUEMASK(v) ==0x0000ff
       && CHECK_BYTE_ORDER(v)
       && v->BitsPerPixel==32
       && sizeof(GLuint)==4
       && v->RedGamma==1.0 && v->GreenGamma==1.0 && v->BlueGamma==1.0) {
      /* common 32 bpp config used on Linux, HP, IBM */
      if (GET_VISUAL_DEPTH(v)==32)
	  v->undithered_pf = v->dithered_pf = PF_8A8R8G8B;
      else
	  v->undithered_pf = v->dithered_pf = PF_8R8G8B;
   }
   else if (GET_REDMASK(v)  ==0xff0000
       &&   GET_GREENMASK(v)==0x00ff00
       &&   GET_BLUEMASK(v) ==0x0000ff
       && CHECK_BYTE_ORDER(v)
       && v->BitsPerPixel==24
       && sizeof(GLuint)==4
       && v->RedGamma==1.0 && v->GreenGamma==1.0 && v->BlueGamma==1.0) {
      /* common packed 24 bpp config used on Linux */
      v->undithered_pf = v->dithered_pf = PF_8R8G8B24;
   }
   else if (GET_REDMASK(v)  ==0xf800
       &&   GET_GREENMASK(v)==0x07e0
       &&   GET_BLUEMASK(v) ==0x001f
       && CHECK_BYTE_ORDER(v)
       && v->BitsPerPixel==16
       && sizeof(GLushort)==2
       && v->RedGamma==1.0 && v->GreenGamma==1.0 && v->BlueGamma==1.0) {
      /* 5-6-5 color weight on common PC VGA boards */
      v->undithered_pf = PF_5R6G5B;
      v->dithered_pf = PF_Dither_5R6G5B;
   }
   else if (GET_REDMASK(v)  ==0xe0
       &&   GET_GREENMASK(v)==0x1c
       &&   GET_BLUEMASK(v) ==0x03
       && CHECK_FOR_HPCR(v)) {
      setup_8bit_hpcr( v );
   }
}



/*
 * Setup RGB rendering for a window with a monochrome visual.
 */
static void setup_monochrome( XMesaVisual v, XMesaBuffer b )
{
   (void) b;
   v->dithered_pf = v->undithered_pf = PF_1Bit;
   /* if black=1 then we must flip pixel values */
   v->bitFlip = (GET_BLACK_PIXEL(v) != 0);
}



/*
 * When a context is "made current" for the first time, we can finally
 * finish initializing the context's visual and buffer information.
 * Input:  v - the XMesaVisual to initialize
 *         b - the XMesaBuffer to initialize (may be NULL)
 *         rgb_flag - TRUE = RGBA mode, FALSE = color index mode
 *         window - the window/pixmap we're rendering into
 *         cmap - the colormap associated with the window/pixmap
 * Return:  GL_TRUE=success, GL_FALSE=failure
 */
static GLboolean initialize_visual_and_buffer( int client,
                                               XMesaVisual v,
                                               XMesaBuffer b,
                                               GLboolean rgb_flag,
                                               XMesaDrawable window,
                                               XMesaColormap cmap )
{
   struct xmesa_renderbuffer *front_xrb, *back_xrb;
#ifndef XFree86Server
   XGCValues gcvalues;
#endif

   if (b) {
      assert(b->xm_visual == v);
   }

   if (b) {
      front_xrb = b->frontxrb;
      back_xrb = b->backxrb;
   }
   else {
      front_xrb = back_xrb = NULL;
   }

   /* Save true bits/pixel */
   v->BitsPerPixel = bits_per_pixel(v);
   assert(v->BitsPerPixel > 0);


   if (rgb_flag==GL_FALSE) {
      /* COLOR-INDEXED WINDOW:
       * Even if the visual is TrueColor or DirectColor we treat it as
       * being color indexed.  This is weird but might be useful to someone.
       */
      v->dithered_pf = v->undithered_pf = PF_Index;
      v->mesa_visual.indexBits = GET_VISUAL_DEPTH(v);
   }
   else {
      /* RGB WINDOW:
       * We support RGB rendering into almost any kind of visual.
       */
      const int xclass = v->mesa_visual.visualType;
      if (xclass==GLX_TRUE_COLOR || xclass==GLX_DIRECT_COLOR) {
	 setup_truecolor( v, b, cmap );
      }
      else if (xclass==GLX_STATIC_GRAY && GET_VISUAL_DEPTH(v)==1) {
	 setup_monochrome( v, b );
      }
      else if (xclass==GLX_GRAY_SCALE || xclass==GLX_STATIC_GRAY) {
         if (!setup_grayscale( client, v, b, cmap )) {
            return GL_FALSE;
         }
      }
      else if ((xclass==GLX_PSEUDO_COLOR || xclass==GLX_STATIC_COLOR)
               && GET_VISUAL_DEPTH(v)>=4 && GET_VISUAL_DEPTH(v)<=16) {
	 if (!setup_dithered_color( client, v, b, cmap )) {
            return GL_FALSE;
         }
      }
      else {
	 _mesa_warning(NULL, "XMesa: RGB mode rendering not supported in given visual.");
	 return GL_FALSE;
      }
      v->mesa_visual.indexBits = 0;

      if (_mesa_getenv("MESA_NO_DITHER")) {
	 v->dithered_pf = v->undithered_pf;
      }
   }


   /*
    * If MESA_INFO env var is set print out some debugging info
    * which can help Brian figure out what's going on when a user
    * reports bugs.
    */
   if (_mesa_getenv("MESA_INFO")) {
      _mesa_printf("X/Mesa visual = %p\n", (void *) v);
      _mesa_printf("X/Mesa dithered pf = %u\n", v->dithered_pf);
      _mesa_printf("X/Mesa undithered pf = %u\n", v->undithered_pf);
      _mesa_printf("X/Mesa level = %d\n", v->mesa_visual.level);
      _mesa_printf("X/Mesa depth = %d\n", GET_VISUAL_DEPTH(v));
      _mesa_printf("X/Mesa bits per pixel = %d\n", v->BitsPerPixel);
   }

   if (b && window) {
      /* Do window-specific initializations */

      b->frontxrb->drawable = window;
      b->frontxrb->pixmap = (XMesaPixmap) window;

      /* Setup for single/double buffering */
      if (v->mesa_visual.doubleBufferMode) {
         /* Double buffered */
#ifndef XFree86Server
         b->shm = check_for_xshm( v->display );
#endif
         xmesa_alloc_back_buffer(b, b->mesa_buffer.Width, b->mesa_buffer.Height);
      }

      /* X11 graphics contexts */
#ifdef XFree86Server
      b->gc = CreateScratchGC(v->display, window->depth);
#else
      b->gc = XCreateGC( v->display, window, 0, NULL );
#endif
      XMesaSetFunction( v->display, b->gc, GXcopy );

      /* cleargc - for glClear() */
#ifdef XFree86Server
      b->cleargc = CreateScratchGC(v->display, window->depth);
#else
      b->cleargc = XCreateGC( v->display, window, 0, NULL );
#endif
      XMesaSetFunction( v->display, b->cleargc, GXcopy );

      /*
       * Don't generate Graphics Expose/NoExpose events in swapbuffers().
       * Patch contributed by Michael Pichler May 15, 1995.
       */
#ifdef XFree86Server
      b->swapgc = CreateScratchGC(v->display, window->depth);
      {
	  CARD32 v[1];
	  v[0] = FALSE;
	  dixChangeGC(NullClient, b->swapgc, GCGraphicsExposures, v, NULL);
      }
#else
      gcvalues.graphics_exposures = False;
      b->swapgc = XCreateGC( v->display, window,
                              GCGraphicsExposures, &gcvalues);
#endif
      XMesaSetFunction( v->display, b->swapgc, GXcopy );
      /*
       * Set fill style and tile pixmap once for all for HPCR stuff
       * (instead of doing it each time in clear_color_HPCR_pixmap())
       * Initialize whole stuff
       * Patch contributed by Jacques Leroy March 8, 1998.
       */
      if (v->hpcr_clear_flag && back_xrb->pixmap) {
	int i;
	for (i=0; i<16; i++)
        {
	   XMesaPutPixel(v->hpcr_clear_ximage, i, 0, 0);
	   XMesaPutPixel(v->hpcr_clear_ximage, i, 1, 0);
        }
        XMesaPutImage(b->display, (XMesaDrawable)v->hpcr_clear_pixmap,
		      b->cleargc, v->hpcr_clear_ximage, 0, 0, 0, 0, 16, 2);
	XMesaSetFillStyle( v->display, b->cleargc, FillTiled);
	XMesaSetTile( v->display, b->cleargc, v->hpcr_clear_pixmap );
      }

      /* Initialize the row buffer XImage for use in write_color_span() */
#ifdef XFree86Server
      b->rowimage = XMesaCreateImage(GET_VISUAL_DEPTH(v), MAX_WIDTH, 1,
				     (char *)MALLOC(MAX_WIDTH*4));
#else
      b->rowimage = XCreateImage( v->display,
                                  v->visinfo->visual,
                                  v->visinfo->depth,
                                  ZPixmap, 0,           /*format, offset*/
                                  (char*) MALLOC(MAX_WIDTH*4),  /*data*/
                                  MAX_WIDTH, 1,         /*width, height*/
                                  32,                   /*bitmap_pad*/
                                  0                     /*bytes_per_line*/ );
#endif
      if (!b->rowimage)
         return GL_FALSE;
   }

   return GL_TRUE;
}



/*
 * Convert an RGBA color to a pixel value.
 */
unsigned long
xmesa_color_to_pixel(GLcontext *ctx,
                     GLubyte r, GLubyte g, GLubyte b, GLubyte a,
                     GLuint pixelFormat)
{
   XMesaContext xmesa = XMESA_CONTEXT(ctx);
   switch (pixelFormat) {
      case PF_Index:
         return 0;
      case PF_Truecolor:
         {
            unsigned long p;
            PACK_TRUECOLOR( p, r, g, b );
            return p;
         }
      case PF_8A8B8G8R:
         return PACK_8A8B8G8R( r, g, b, a );
      case PF_8A8R8G8B:
         return PACK_8A8R8G8B( r, g, b, a );
      case PF_8R8G8B:
         /* fall through */
      case PF_8R8G8B24:
         return PACK_8R8G8B( r, g, b );
      case PF_5R6G5B:
         return PACK_5R6G5B( r, g, b );
      case PF_Dither:
         {
            DITHER_SETUP;
            return DITHER( 1, 0, r, g, b );
         }
      case PF_1Bit:
         /* 382 = (3*255)/2 */
         return ((r+g+b) > 382) ^ xmesa->xm_visual->bitFlip;
      case PF_HPCR:
         return DITHER_HPCR(1, 1, r, g, b);
      case PF_Lookup:
         {
            LOOKUP_SETUP;
            return LOOKUP( r, g, b );
         }
      case PF_Grayscale:
         return GRAY_RGB( r, g, b );
      case PF_Dither_True:
         /* fall through */
      case PF_Dither_5R6G5B:
         {
            unsigned long p;
            PACK_TRUEDITHER(p, 1, 0, r, g, b);
            return p;
         }
      default:
         _mesa_problem(ctx, "Bad pixel format in xmesa_color_to_pixel");
   }
   return 0;
}


#define NUM_VISUAL_TYPES   6

/**
 * Convert an X visual type to a GLX visual type.
 * 
 * \param visualType X visual type (i.e., \c TrueColor, \c StaticGray, etc.)
 *        to be converted.
 * \return If \c visualType is a valid X visual type, a GLX visual type will
 *         be returned.  Otherwise \c GLX_NONE will be returned.
 * 
 * \note
 * This code was lifted directly from lib/GL/glx/glcontextmodes.c in the
 * DRI CVS tree.
 */
static GLint
xmesa_convert_from_x_visual_type( int visualType )
{
    static const int glx_visual_types[ NUM_VISUAL_TYPES ] = {
	GLX_STATIC_GRAY,  GLX_GRAY_SCALE,
	GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
	GLX_TRUE_COLOR,   GLX_DIRECT_COLOR
    };

    return ( (unsigned) visualType < NUM_VISUAL_TYPES )
	? glx_visual_types[ visualType ] : GLX_NONE;
}


/**********************************************************************/
/*****                       Public Functions                     *****/
/**********************************************************************/


/*
 * Create a new X/Mesa visual.
 * Input:  display - X11 display
 *         visinfo - an XVisualInfo pointer
 *         rgb_flag - GL_TRUE = RGB mode,
 *                    GL_FALSE = color index mode
 *         alpha_flag - alpha buffer requested?
 *         db_flag - GL_TRUE = double-buffered,
 *                   GL_FALSE = single buffered
 *         stereo_flag - stereo visual?
 *         ximage_flag - GL_TRUE = use an XImage for back buffer,
 *                       GL_FALSE = use an off-screen pixmap for back buffer
 *         depth_size - requested bits/depth values, or zero
 *         stencil_size - requested bits/stencil values, or zero
 *         accum_red_size - requested bits/red accum values, or zero
 *         accum_green_size - requested bits/green accum values, or zero
 *         accum_blue_size - requested bits/blue accum values, or zero
 *         accum_alpha_size - requested bits/alpha accum values, or zero
 *         num_samples - number of samples/pixel if multisampling, or zero
 *         level - visual level, usually 0
 *         visualCaveat - ala the GLX extension, usually GLX_NONE
 * Return;  a new XMesaVisual or 0 if error.
 */
XMesaVisual XMesaCreateVisual( XMesaDisplay *display,
                               XMesaVisualInfo visinfo,
                               GLboolean rgb_flag,
                               GLboolean alpha_flag,
                               GLboolean db_flag,
                               GLboolean stereo_flag,
                               GLboolean ximage_flag,
                               GLint depth_size,
                               GLint stencil_size,
                               GLint accum_red_size,
                               GLint accum_green_size,
                               GLint accum_blue_size,
                               GLint accum_alpha_size,
                               GLint num_samples,
                               GLint level,
                               GLint visualCaveat )
{
   char *gamma;
   XMesaVisual v;
   GLint red_bits, green_bits, blue_bits, alpha_bits;

   /* For debugging only */
   if (_mesa_getenv("MESA_XSYNC")) {
      /* This makes debugging X easier.
       * In your debugger, set a breakpoint on _XError to stop when an
       * X protocol error is generated.
       */
#ifdef XFree86Server
      /* NOT_NEEDED */
#else
      XSynchronize( display, 1 );
#endif
   }

   v = (XMesaVisual) CALLOC_STRUCT(xmesa_visual);
   if (!v) {
      return NULL;
   }

   /*
    * In the X server, NULL is passed in for the display.  It will have
    * to be set before using this visual.  See XMesaSetVisualDisplay()
    * below.
    */
   v->display = display;

   /* Save a copy of the XVisualInfo struct because the user may X_mesa_free()
    * the struct but we may need some of the information contained in it
    * at a later time.
    */
#ifndef XFree86Server
   v->visinfo = (XVisualInfo *) MALLOC(sizeof(*visinfo));
   if(!v->visinfo) {
      _mesa_free(v);
      return NULL;
   }
   MEMCPY(v->visinfo, visinfo, sizeof(*visinfo));
#endif

   /* check for MESA_GAMMA environment variable */
   gamma = _mesa_getenv("MESA_GAMMA");
   if (gamma) {
      v->RedGamma = v->GreenGamma = v->BlueGamma = 0.0;
      sscanf( gamma, "%f %f %f", &v->RedGamma, &v->GreenGamma, &v->BlueGamma );
      if (v->RedGamma<=0.0)    v->RedGamma = 1.0;
      if (v->GreenGamma<=0.0)  v->GreenGamma = v->RedGamma;
      if (v->BlueGamma<=0.0)   v->BlueGamma = v->RedGamma;
   }
   else {
      v->RedGamma = v->GreenGamma = v->BlueGamma = 1.0;
   }

   v->ximage_flag = ximage_flag;

#ifdef XFree86Server
   /* We could calculate these values by ourselves.  nplanes is either the sum
    * of the red, green, and blue bits or the number index bits.
    * ColormapEntries is either (1U << index_bits) or
    * (1U << max(redBits, greenBits, blueBits)).
    */
   v->nplanes = visinfo->nplanes;
   v->ColormapEntries = visinfo->ColormapEntries;

   v->mesa_visual.redMask = visinfo->redMask;
   v->mesa_visual.greenMask = visinfo->greenMask;
   v->mesa_visual.blueMask = visinfo->blueMask;
   v->mesa_visual.visualID = visinfo->vid;
   v->mesa_visual.screen = 0; /* FIXME: What should be done here? */
#else
   v->mesa_visual.redMask = visinfo->red_mask;
   v->mesa_visual.greenMask = visinfo->green_mask;
   v->mesa_visual.blueMask = visinfo->blue_mask;
   v->mesa_visual.visualID = visinfo->visualid;
   v->mesa_visual.screen = visinfo->screen;
#endif

#if defined(XFree86Server) || !(defined(__cplusplus) || defined(c_plusplus))
   v->mesa_visual.visualType = xmesa_convert_from_x_visual_type(visinfo->class);
#else
   v->mesa_visual.visualType = xmesa_convert_from_x_visual_type(visinfo->c_class);
#endif

   v->mesa_visual.visualRating = visualCaveat;

   (void) initialize_visual_and_buffer( 0, v, NULL, rgb_flag, 0, 0 );

   {
      const int xclass = v->mesa_visual.visualType;
      if (xclass==GLX_TRUE_COLOR || xclass==GLX_DIRECT_COLOR) {
         red_bits   = bitcount(GET_REDMASK(v));
         green_bits = bitcount(GET_GREENMASK(v));
         blue_bits  = bitcount(GET_BLUEMASK(v));
         alpha_bits = 0;
      }
      else {
         /* this is an approximation */
         int depth;
         depth = GET_VISUAL_DEPTH(v);
         red_bits = depth / 3;
         depth -= red_bits;
         green_bits = depth / 2;
         depth -= green_bits;
         blue_bits = depth;
         alpha_bits = 0;
         assert( red_bits + green_bits + blue_bits == GET_VISUAL_DEPTH(v) );
      }
   }

   if (alpha_flag && alpha_bits == 0)
      alpha_bits = 8;

   _mesa_initialize_visual( &v->mesa_visual,
                            rgb_flag, db_flag, stereo_flag,
                            red_bits, green_bits,
                            blue_bits, alpha_bits,
                            v->mesa_visual.indexBits,
                            depth_size,
                            stencil_size,
                            accum_red_size, accum_green_size,
                            accum_blue_size, accum_alpha_size,
                            0 );

   /* XXX minor hack */
   v->mesa_visual.level = level;
   return v;
}


void XMesaSetVisualDisplay( XMesaDisplay *dpy, XMesaVisual v )
{
    v->display = dpy;
}


void XMesaDestroyVisual( XMesaVisual v )
{
#ifndef XFree86Server
   _mesa_free(v->visinfo);
#endif
   _mesa_free(v);
}



/**
 * Create a new XMesaContext.
 * \param v  the XMesaVisual
 * \param share_list  another XMesaContext with which to share display
 *                    lists or NULL if no sharing is wanted.
 * \return an XMesaContext or NULL if error.
 */
XMesaContext XMesaCreateContext( XMesaVisual v, XMesaContext share_list )
{
   static GLboolean firstTime = GL_TRUE;
   XMesaContext c;
   GLcontext *mesaCtx;
   struct dd_function_table functions;
   TNLcontext *tnl;

   if (firstTime) {
      _glthread_INIT_MUTEX(_xmesa_lock);
      firstTime = GL_FALSE;
   }

   /* Note: the XMesaContext contains a Mesa GLcontext struct (inheritance) */
   c = (XMesaContext) CALLOC_STRUCT(xmesa_context);
   if (!c)
      return NULL;

   mesaCtx = &(c->mesa);

   /* initialize with default driver functions, then plug in XMesa funcs */
   _mesa_init_driver_functions(&functions);
   xmesa_init_driver_functions(v, &functions);
   if (!_mesa_initialize_context(mesaCtx, &v->mesa_visual,
                      share_list ? &(share_list->mesa) : (GLcontext *) NULL,
                      &functions, (void *) c)) {
      _mesa_free(c);
      return NULL;
   }

   _mesa_enable_sw_extensions(mesaCtx);
   _mesa_enable_1_3_extensions(mesaCtx);
   _mesa_enable_1_4_extensions(mesaCtx);
   _mesa_enable_1_5_extensions(mesaCtx);
   _mesa_enable_2_0_extensions(mesaCtx);
#if SWTC
    if (c->Mesa_DXTn) {
       _mesa_enable_extension(c, "GL_EXT_texture_compression_s3tc");
       _mesa_enable_extension(c, "GL_S3_s3tc");
    }
    _mesa_enable_extension(c, "GL_3DFX_texture_compression_FXT1");
#endif

   /* finish up xmesa context initializations */
   c->swapbytes = CHECK_BYTE_ORDER(v) ? GL_FALSE : GL_TRUE;
   c->xm_visual = v;
   c->xm_buffer = NULL;   /* set later by XMesaMakeCurrent */
   c->display = v->display;
   c->pixelformat = v->dithered_pf;      /* Dithering is enabled by default */

   /* Initialize the software rasterizer and helper modules.
    */
   if (!_swrast_CreateContext( mesaCtx ) ||
       !_ac_CreateContext( mesaCtx ) ||
       !_tnl_CreateContext( mesaCtx ) ||
       !_swsetup_CreateContext( mesaCtx )) {
      _mesa_free_context_data(&c->mesa);
      _mesa_free(c);
      return NULL;
   }

   /* tnl setup */
   tnl = TNL_CONTEXT(mesaCtx);
   tnl->Driver.RunPipeline = _tnl_run_pipeline;
   /* swrast setup */
   xmesa_register_swrast_functions( mesaCtx );
   _swsetup_Wakeup(mesaCtx);

   return c;
}



void XMesaDestroyContext( XMesaContext c )
{
   GLcontext *mesaCtx = &c->mesa;
#ifdef FX
   XMesaBuffer xmbuf = XMESA_BUFFER(mesaCtx->DrawBuffer);

   if (xmbuf && xmbuf->FXctx)
      fxMesaDestroyContext(xmbuf->FXctx);
#endif
   _swsetup_DestroyContext( mesaCtx );
   _swrast_DestroyContext( mesaCtx );
   _tnl_DestroyContext( mesaCtx );
   _ac_DestroyContext( mesaCtx );
   _mesa_free_context_data( mesaCtx );
   _mesa_free( c );
}



/*
 * XXX this isn't a public function!  It's a hack for the 3Dfx driver.
 * Create a new XMesaBuffer from an X window.
 * Input:  v - the XMesaVisual
 *         w - the window
 *         c - the context
 * Return:  new XMesaBuffer or NULL if error
 */
XMesaBuffer
XMesaCreateWindowBuffer2(XMesaVisual v, XMesaWindow w, XMesaContext c)
{
#ifndef XFree86Server
   XWindowAttributes attr;
#endif
#ifdef FX
   char *fxEnvVar;
#endif
   int client = 0;
   XMesaBuffer b;
   XMesaColormap cmap;

   assert(v);
   (void) c;

   /* Check that window depth matches visual depth */
#ifdef XFree86Server
   client = CLIENT_ID(((XMesaDrawable)w)->id);

   if (GET_VISUAL_DEPTH(v) != ((XMesaDrawable)w)->depth) {
      _mesa_warning(NULL, "XMesaCreateWindowBuffer: depth mismatch between visual (%d) and window (%d)!\n",
                    GET_VISUAL_DEPTH(v), ((XMesaDrawable) w)->depth);
      return NULL;
   }
#else
   XGetWindowAttributes( v->display, w, &attr );

   if (GET_VISUAL_DEPTH(v) != attr.depth) {
      _mesa_warning(NULL, "XMesaCreateWindowBuffer: depth mismatch between visual (%d) and window (%d)!\n",
                    GET_VISUAL_DEPTH(v), attr.depth);
      return NULL;
   }
#endif

   /* Find colormap */
#ifdef XFree86Server
   cmap = (ColormapPtr)LookupIDByType(wColormap(w), RT_COLORMAP);
#else
   if (attr.colormap) {
      cmap = attr.colormap;
   }
   else {
      _mesa_warning(NULL, "Window %u has no colormap!\n", (unsigned int) w);
      /* this is weird, a window w/out a colormap!? */
      /* OK, let's just allocate a new one and hope for the best */
      cmap = XCreateColormap(v->display, w, attr.visual, AllocNone);
   }
#endif

   b = alloc_xmesa_buffer(v, WINDOW, cmap);
   if (!b) {
      return NULL;
   }

   if (!initialize_visual_and_buffer( client, v, b, v->mesa_visual.rgbMode,
                                      (XMesaDrawable)w, cmap )) {
      free_xmesa_buffer(client, b);
      return NULL;
   }

#ifdef FX
   fxEnvVar = _mesa_getenv("MESA_GLX_FX");
   if (fxEnvVar) {
     if (fxEnvVar[0]!='d') {
       int attribs[100];
       int numAttribs = 0;
       int hw;
       if (v->mesa_visual.depthBits > 0) {
	 attribs[numAttribs++] = FXMESA_DEPTH_SIZE;
	 attribs[numAttribs++] = v->mesa_visual.depthBits;
       }
       if (v->mesa_visual.doubleBufferMode) {
	 attribs[numAttribs++] = FXMESA_DOUBLEBUFFER;
       }
       if (v->mesa_visual.accumRedBits > 0) {
	 attribs[numAttribs++] = FXMESA_ACCUM_SIZE;
	 attribs[numAttribs++] = v->mesa_visual.accumRedBits;
       }
       if (v->mesa_visual.stencilBits > 0) {
         attribs[numAttribs++] = FXMESA_STENCIL_SIZE;
         attribs[numAttribs++] = v->mesa_visual.stencilBits;
       }
       if (v->mesa_visual.alphaBits > 0) {
         attribs[numAttribs++] = FXMESA_ALPHA_SIZE;
         attribs[numAttribs++] = v->mesa_visual.alphaBits;
       }
       if (1) {
         attribs[numAttribs++] = FXMESA_SHARE_CONTEXT;
         attribs[numAttribs++] = (int) &(c->mesa);
       }
       attribs[numAttribs++] = FXMESA_NONE;

       /* [dBorca] we should take an envvar for `fxMesaSelectCurrentBoard'!!! */
       hw = fxMesaSelectCurrentBoard(0);

       /* if these fail, there's a new bug somewhere */
       ASSERT(b->mesa_buffer.Width > 0);
       ASSERT(b->mesa_buffer.Height > 0);

       if ((hw == GR_SSTTYPE_VOODOO) || (hw == GR_SSTTYPE_Voodoo2)) {
         b->FXctx = fxMesaCreateBestContext(0, b->mesa_buffer.Width,
                                            b->mesa_buffer.Height, attribs);
         if ((v->undithered_pf!=PF_Index) && (b->backxrb->ximage)) {
	   b->FXisHackUsable = b->FXctx ? GL_TRUE : GL_FALSE;
	   if (b->FXctx && (fxEnvVar[0]=='w' || fxEnvVar[0]=='W')) {
	     b->FXwindowHack = GL_TRUE;
	     FX_grSstControl(GR_CONTROL_DEACTIVATE);
	   }
           else {
	     b->FXwindowHack = GL_FALSE;
	   }
         }
       }
       else {
         if (fxEnvVar[0]=='w' || fxEnvVar[0]=='W')
	   b->FXctx = fxMesaCreateContext(w, GR_RESOLUTION_NONE,
					  GR_REFRESH_75Hz, attribs);
         else
	   b->FXctx = fxMesaCreateBestContext(0, b->mesa_buffer.Width,
                                              b->mesa_buffer.Height, attribs);
         b->FXisHackUsable = GL_FALSE;
         b->FXwindowHack = GL_FALSE;
       }
       /*
       fprintf(stderr,
               "voodoo %d, wid %d height %d hack: usable %d active %d\n",
               hw, b->mesa_buffer.Width, b->mesa_buffer.Height,
	       b->FXisHackUsable, b->FXwindowHack);
       */
     }
   }
   else {
      _mesa_warning(NULL, "WARNING: This Mesa Library includes the Glide driver but\n");
      _mesa_warning(NULL, "         you have not defined the MESA_GLX_FX env. var.\n");
      _mesa_warning(NULL, "         (check the README.3DFX file for more information).\n\n");
      _mesa_warning(NULL, "         you can disable this message with a 'export MESA_GLX_FX=disable'.\n");
   }
#endif

   return b;
}


XMesaBuffer
XMesaCreateWindowBuffer(XMesaVisual v, XMesaWindow w)
{
   return XMesaCreateWindowBuffer2( v, w, NULL );
}


/**
 * Create a new XMesaBuffer from an X pixmap.
 *
 * \param v    the XMesaVisual
 * \param p    the pixmap
 * \param cmap the colormap, may be 0 if using a \c GLX_TRUE_COLOR or
 *             \c GLX_DIRECT_COLOR visual for the pixmap
 * \returns new XMesaBuffer or NULL if error
 */
XMesaBuffer
XMesaCreatePixmapBuffer(XMesaVisual v, XMesaPixmap p, XMesaColormap cmap)
{
   int client = 0;
   XMesaBuffer b;

   assert(v);

   b = alloc_xmesa_buffer(v, PIXMAP, cmap);
   if (!b) {
      return NULL;
   }

#ifdef XFree86Server
   client = CLIENT_ID(((XMesaDrawable)p)->id);
#endif

   if (!initialize_visual_and_buffer(client, v, b, v->mesa_visual.rgbMode,
				     (XMesaDrawable)p, cmap)) {
      free_xmesa_buffer(client, b);
      return NULL;
   }

   return b;
}



XMesaBuffer
XMesaCreatePBuffer(XMesaVisual v, XMesaColormap cmap,
                   unsigned int width, unsigned int height)
{
#ifdef XFree86Server
   return 0;
#else
   int client = 0;
   XMesaWindow root;
   XMesaDrawable drawable;  /* X Pixmap Drawable */
   XMesaBuffer b;

   b = alloc_xmesa_buffer(v, PBUFFER, cmap);
   if (!b) {
      return NULL;
   }

   /* allocate pixmap for front buffer */
   root = RootWindow( v->display, v->visinfo->screen );
   drawable = XCreatePixmap( v->display, root, width, height, v->visinfo->depth );

   if (!initialize_visual_and_buffer(client, v, b, v->mesa_visual.rgbMode,
				     drawable, cmap)) {
      free_xmesa_buffer(client, b);
      return NULL;
   }

   return b;
#endif
}



/*
 * Deallocate an XMesaBuffer structure and all related info.
 */
void XMesaDestroyBuffer( XMesaBuffer b )
{
   int client = 0;

#ifdef XFree86Server
   if (b->frontxrb->drawable)
       client = CLIENT_ID(b->frontxrb->drawable->id);
#endif

   if (b->gc)  XMesaFreeGC( b->xm_visual->display, b->gc );
   if (b->cleargc)  XMesaFreeGC( b->xm_visual->display, b->cleargc );
   if (b->swapgc)  XMesaFreeGC( b->xm_visual->display, b->swapgc );

   if (b->xm_visual->mesa_visual.doubleBufferMode)
   {
      if (b->backxrb->ximage) {
#if defined(USE_XSHM) && !defined(XFree86Server)
         if (b->shm) {
            XShmDetach( b->xm_visual->display, &b->shminfo );
            XDestroyImage( b->backxrb->ximage );
            shmdt( b->shminfo.shmaddr );
         }
         else
#endif
            XMesaDestroyImage( b->backxrb->ximage );
       }
      if (b->backxrb->pixmap) {
         XMesaFreePixmap( b->xm_visual->display,
                          (XMesaPixmap) b->backxrb->pixmap );
         if (b->xm_visual->hpcr_clear_flag) {
            XMesaFreePixmap( b->xm_visual->display,
                             b->xm_visual->hpcr_clear_pixmap );
            XMesaDestroyImage( b->xm_visual->hpcr_clear_ximage );
         }
      }
   }
   if (b->rowimage) {
      _mesa_free( b->rowimage->data );
      b->rowimage->data = NULL;
      XMesaDestroyImage( b->rowimage );
   }

   free_xmesa_buffer(client, b);
}



/*
 * Bind buffer b to context c and make c the current rendering context.
 */
GLboolean XMesaMakeCurrent( XMesaContext c, XMesaBuffer b )
{
   return XMesaMakeCurrent2( c, b, b );
}


/*
 * Bind buffer b to context c and make c the current rendering context.
 */
GLboolean XMesaMakeCurrent2( XMesaContext c, XMesaBuffer drawBuffer,
                             XMesaBuffer readBuffer )
{
   if (c) {
      if (!drawBuffer || !readBuffer)
         return GL_FALSE;  /* must specify buffers! */

#ifdef FX
      if (drawBuffer->FXctx) {
         fxMesaMakeCurrent(drawBuffer->FXctx);

         c->xm_buffer = drawBuffer;

         return GL_TRUE;
      }
#endif
      if (&(c->mesa) == _mesa_get_current_context()
          && c->mesa.DrawBuffer == &drawBuffer->mesa_buffer
          && c->mesa.ReadBuffer == &readBuffer->mesa_buffer
          && ((XMesaBuffer) c->mesa.DrawBuffer)->wasCurrent) {
         /* same context and buffer, do nothing */
         return GL_TRUE;
      }

      c->xm_buffer = drawBuffer;

      _mesa_make_current(&(c->mesa),
                         &drawBuffer->mesa_buffer,
                         &readBuffer->mesa_buffer);

      if (c->xm_visual->mesa_visual.rgbMode) {
         /*
          * Must recompute and set these pixel values because colormap
          * can be different for different windows.
          */
         c->clearpixel = xmesa_color_to_pixel( &c->mesa,
                                               c->clearcolor[0],
                                               c->clearcolor[1],
                                               c->clearcolor[2],
                                               c->clearcolor[3],
                                               c->xm_visual->undithered_pf);
         XMesaSetForeground(c->display, drawBuffer->cleargc, c->clearpixel);
      }

      /* Solution to Stephane Rehel's problem with glXReleaseBuffersMESA(): */
      drawBuffer->wasCurrent = GL_TRUE;
   }
   else {
      /* Detach */
      _mesa_make_current( NULL, NULL, NULL );
   }
   return GL_TRUE;
}


/*
 * Unbind the context c from its buffer.
 */
GLboolean XMesaUnbindContext( XMesaContext c )
{
   /* A no-op for XFree86 integration purposes */
   return GL_TRUE;
}


XMesaContext XMesaGetCurrentContext( void )
{
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      XMesaContext xmesa = XMESA_CONTEXT(ctx);
      return xmesa;
   }
   else {
      return 0;
   }
}


XMesaBuffer XMesaGetCurrentBuffer( void )
{
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      return xmbuf;
   }
   else {
      return 0;
   }
}


/* New in Mesa 3.1 */
XMesaBuffer XMesaGetCurrentReadBuffer( void )
{
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      return (XMesaBuffer) (ctx->ReadBuffer);
   }
   else {
      return 0;
   }
}


GLboolean XMesaForceCurrent(XMesaContext c)
{
   if (c) {
      if (&(c->mesa) != _mesa_get_current_context()) {
	 _mesa_make_current(&c->mesa, c->mesa.DrawBuffer, c->mesa.ReadBuffer);
      }
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }
   return GL_TRUE;
}


GLboolean XMesaLoseCurrent(XMesaContext c)
{
   (void) c;
   _mesa_make_current(NULL, NULL, NULL);
   return GL_TRUE;
}


/*
 * Switch 3Dfx support hack between window and full-screen mode.
 */
GLboolean XMesaSetFXmode( GLint mode )
{
#ifdef FX
   const char *fx = _mesa_getenv("MESA_GLX_FX");
   if (fx && fx[0] != 'd') {
      GET_CURRENT_CONTEXT(ctx);
      GrHwConfiguration hw;
      if (!FX_grSstQueryHardware(&hw)) {
         /*fprintf(stderr, "!grSstQueryHardware\n");*/
         return GL_FALSE;
      }
      if (hw.num_sst < 1) {
         /*fprintf(stderr, "hw.num_sst < 1\n");*/
         return GL_FALSE;
      }
      if (ctx) {
         /* [dBorca] Hack alert: 
	  * oh, this is sooo wrong: ctx above is
	  * really an fxMesaContext, not an XMesaContext
	  */
         XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
         if (mode == XMESA_FX_WINDOW) {
	    if (xmbuf->FXisHackUsable) {
	       FX_grSstControl(GR_CONTROL_DEACTIVATE);
	       xmbuf->FXwindowHack = GL_TRUE;
	       return GL_TRUE;
	    }
	 }
	 else if (mode == XMESA_FX_FULLSCREEN) {
	    FX_grSstControl(GR_CONTROL_ACTIVATE);
	    xmbuf->FXwindowHack = GL_FALSE;
	    return GL_TRUE;
	 }
	 else {
	    /* Error: Bad mode value */
	 }
      }
   }
   /*fprintf(stderr, "fallthrough\n");*/
#else
   (void) mode;
#endif
   return GL_FALSE;
}



#ifdef FX
/*
 * Read image from VooDoo frame buffer into X/Mesa's back XImage.
 */
static void FXgetImage( XMesaBuffer b )
{
   GET_CURRENT_CONTEXT(ctx);
   static unsigned short pixbuf[MAX_WIDTH];
   GLuint x, y;
   int xpos, ypos;
   XMesaWindow root;
   unsigned int bw, depth, width, height;
   XMesaContext xmesa = XMESA_CONTEXT(ctx);

#ifdef XFree86Server
   x = b->frontxrb->pixmap->x;
   y = b->frontxrb->pixmap->y;
   width = b->frontxrb->pixmap->width;
   height = b->frontxrb->pixmap->height;
   depth = b->frontxrb->pixmap->depth;
#else
   XGetGeometry( b->xm_visual->display, b->frontxrb->pixmap,
                 &root, &xpos, &ypos, &width, &height, &bw, &depth);
#endif
   if (b->mesa_buffer.Width != width || b->mesa_buffer.Height != height) {
      b->mesa_buffer.Width = MIN2((int)width, b->FXctx->width);
      b->mesa_buffer.Height = MIN2((int)height, b->FXctx->height);
      if (b->mesa_buffer.Width & 1)
         b->mesa_buffer.Width--;  /* prevent odd width */
      xmesa_alloc_back_buffer(b, b->mesa_buffer.Width, b->mesa_buffer.Height);
   }

   /* [dBorca] we're always in the right GR_COLORFORMAT... aren't we? */
   /* grLfbWriteColorFormat(GR_COLORFORMAT_ARGB); */
   if (b->xm_visual->undithered_pf==PF_5R6G5B) {
      /* Special case: 16bpp RGB */
      grLfbReadRegion( GR_BUFFER_FRONTBUFFER,       /* src buffer */
                       0, b->FXctx->height - b->mesa_buffer.Height,  /*pos*/
                       b->mesa_buffer.Width, b->mesa_buffer.Height,  /* size */
                       b->mesa_buffer.Width * sizeof(GLushort), /* stride */
                       b->backxrb->ximage->data);         /* dest buffer */
   }
   else if (b->xm_visual->dithered_pf==PF_Dither
	    && GET_VISUAL_DEPTH(b->xm_visual)==8) {
      /* Special case: 8bpp RGB */
      for (y=0;y<b->mesa_buffer.Height;y++) {
         GLubyte *ptr = (GLubyte*) b->backxrb->ximage->data
                        + b->backxrb->ximage->bytes_per_line * y;
         XDITHER_SETUP(y);

         /* read row from 3Dfx frame buffer */
         grLfbReadRegion( GR_BUFFER_FRONTBUFFER,
                          0, b->FXctx->height-(b->mesa_buffer.Height-y),
                          b->mesa_buffer.Width, 1,
                          0,
                          pixbuf );

         /* write to XImage back buffer */
         for (x=0;x<b->mesa_buffer.Width;x++) {
            GLubyte r = (pixbuf[x] & 0xf800) >> 8;
            GLubyte g = (pixbuf[x] & 0x07e0) >> 3;
            GLubyte b = (pixbuf[x] & 0x001f) << 3;
            *ptr++ = XDITHER( x, r, g, b);
         }
      }
   }
   else {
      /* General case: slow! */
      for (y=0;y<b->mesa_buffer.Height;y++) {
         /* read row from 3Dfx frame buffer */
         grLfbReadRegion( GR_BUFFER_FRONTBUFFER,
                          0, b->FXctx->height-(b->mesa_buffer.Height-y),
                          b->mesa_buffer.Width, 1,
                          0,
                          pixbuf );

         /* write to XImage back buffer */
         for (x=0;x<b->mesa_buffer.Width;x++) {
            XMesaPutPixel(b->backxrb->ximage,x,y,
			  xmesa_color_to_pixel(ctx,
					       (pixbuf[x] & 0xf800) >> 8,
					       (pixbuf[x] & 0x07e0) >> 3,
					       (pixbuf[x] & 0x001f) << 3,
					       0xff,
                                               b->xm_visual->undithered_pf));
         }
      }
   }
   /* grLfbWriteColorFormat(GR_COLORFORMAT_ABGR); */
}
#endif


/*
 * Copy the back buffer to the front buffer.  If there's no back buffer
 * this is a no-op.
 */
void XMesaSwapBuffers( XMesaBuffer b )
{
   GET_CURRENT_CONTEXT(ctx);

   /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
   if (ctx && ctx->DrawBuffer == &(b->mesa_buffer))
      _mesa_notifySwapBuffers(ctx);

   if (b->db_state) {
#ifdef FX
      if (b->FXctx) {
         fxMesaSwapBuffers();

         if (b->FXwindowHack)
            FXgetImage(b);
         else
            return;
      }
#endif
     if (b->backxrb->ximage) {
	 /* Copy Ximage from host's memory to server's window */
#if defined(USE_XSHM) && !defined(XFree86Server)
	 if (b->shm) {
            /*_glthread_LOCK_MUTEX(_xmesa_lock);*/
	    XShmPutImage( b->xm_visual->display, b->frontxrb->drawable,
			  b->swapgc,
			  b->backxrb->ximage, 0, 0,
			  0, 0, b->mesa_buffer.Width, b->mesa_buffer.Height,
                          False );
            /*_glthread_UNLOCK_MUTEX(_xmesa_lock);*/
	 }
	 else
#endif
         {
            /*_glthread_LOCK_MUTEX(_xmesa_lock);*/
            XMesaPutImage( b->xm_visual->display, b->frontxrb->drawable,
			   b->swapgc,
			   b->backxrb->ximage, 0, 0,
			   0, 0, b->mesa_buffer.Width, b->mesa_buffer.Height );
            /*_glthread_UNLOCK_MUTEX(_xmesa_lock);*/
         }
      }
      else {
	 /* Copy pixmap to window on server */
         /*_glthread_LOCK_MUTEX(_xmesa_lock);*/
	 XMesaCopyArea( b->xm_visual->display,
			b->backxrb->pixmap,   /* source drawable */
			b->frontxrb->drawable,  /* dest. drawable */
			b->swapgc,
			0, 0, b->mesa_buffer.Width, b->mesa_buffer.Height,
			0, 0                 /* dest region */
		      );
         /*_glthread_UNLOCK_MUTEX(_xmesa_lock);*/
      }
   }
#if !defined(XFree86Server)
   XSync( b->xm_visual->display, False );
#endif
}



/*
 * Copy sub-region of back buffer to front buffer
 */
void XMesaCopySubBuffer( XMesaBuffer b, int x, int y, int width, int height )
{
   GET_CURRENT_CONTEXT(ctx);

   /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
   if (ctx && ctx->DrawBuffer == &(b->mesa_buffer))
      _mesa_notifySwapBuffers(ctx);

   if (b->db_state) {
      int yTop = b->mesa_buffer.Height - y - height;
#ifdef FX
      if (b->FXctx) {
         fxMesaSwapBuffers();
         if (b->FXwindowHack)
            FXgetImage(b);
         else
            return;
      }
#endif
      if (b->backxrb->ximage) {
         /* Copy Ximage from host's memory to server's window */
#if defined(USE_XSHM) && !defined(XFree86Server)
         if (b->shm) {
            /* XXX assuming width and height aren't too large! */
            XShmPutImage( b->xm_visual->display, b->frontxrb->drawable,
                          b->swapgc,
                          b->backxrb->ximage, x, yTop,
                          x, yTop, width, height, False );
            /* wait for finished event??? */
         }
         else
#endif
         {
            /* XXX assuming width and height aren't too large! */
            XMesaPutImage( b->xm_visual->display, b->frontxrb->drawable,
			   b->swapgc,
			   b->backxrb->ximage, x, yTop,
			   x, yTop, width, height );
         }
      }
      else {
         /* Copy pixmap to window on server */
         XMesaCopyArea( b->xm_visual->display,
			b->backxrb->pixmap,           /* source drawable */
			b->frontxrb->drawable,        /* dest. drawable */
			b->swapgc,
			x, yTop, width, height,  /* source region */
			x, yTop                  /* dest region */
                      );
      }
   }
}


/*
 * Return a pointer to the XMesa backbuffer Pixmap or XImage.  This function
 * is a way to get "under the hood" of X/Mesa so one can manipulate the
 * back buffer directly.
 * Output:  pixmap - pointer to back buffer's Pixmap, or 0
 *          ximage - pointer to back buffer's XImage, or NULL
 * Return:  GL_TRUE = context is double buffered
 *          GL_FALSE = context is single buffered
 */
#ifndef XFree86Server
GLboolean XMesaGetBackBuffer( XMesaBuffer b,
                              XMesaPixmap *pixmap,
                              XMesaImage **ximage )
{
   if (b->db_state) {
      if (pixmap)  *pixmap = b->backxrb->pixmap;
      if (ximage)  *ximage = b->backxrb->ximage;
      return GL_TRUE;
   }
   else {
      *pixmap = 0;
      *ximage = NULL;
      return GL_FALSE;
   }
}
#endif /* XFree86Server */


/*
 * Return the depth buffer associated with an XMesaBuffer.
 * Input:  b - the XMesa buffer handle
 * Output:  width, height - size of buffer in pixels
 *          bytesPerValue - bytes per depth value (2 or 4)
 *          buffer - pointer to depth buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLboolean XMesaGetDepthBuffer( XMesaBuffer b, GLint *width, GLint *height,
                               GLint *bytesPerValue, void **buffer )
{
   struct gl_renderbuffer *rb
      = b->mesa_buffer.Attachment[BUFFER_DEPTH].Renderbuffer;
   if (!rb || !rb->Data) {
      *width = 0;
      *height = 0;
      *bytesPerValue = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = b->mesa_buffer.Width;
      *height = b->mesa_buffer.Height;
      *bytesPerValue = b->mesa_buffer.Visual.depthBits <= 16
         ? sizeof(GLushort) : sizeof(GLuint);
      *buffer = rb->Data;
      return GL_TRUE;
   }
}


void XMesaFlush( XMesaContext c )
{
   if (c && c->xm_visual) {
#ifdef XFree86Server
      /* NOT_NEEDED */
#else
      XSync( c->xm_visual->display, False );
#endif
   }
}



const char *XMesaGetString( XMesaContext c, int name )
{
   (void) c;
   if (name==XMESA_VERSION) {
      return "5.0";
   }
   else if (name==XMESA_EXTENSIONS) {
      return "";
   }
   else {
      return NULL;
   }
}



XMesaBuffer XMesaFindBuffer( XMesaDisplay *dpy, XMesaDrawable d )
{
   XMesaBuffer b;
   for (b=XMesaBufferList; b; b=b->Next) {
      if (b->frontxrb->drawable == d && b->display == dpy) {
         return b;
      }
   }
   return NULL;
}



/*
 * Look for XMesaBuffers whose X window has been destroyed.
 * Deallocate any such XMesaBuffers.
 */
void XMesaGarbageCollect( void )
{
   XMesaBuffer b, next;
   for (b=XMesaBufferList; b; b=next) {
      next = b->Next;
      if (b->display && b->frontxrb->drawable && b->type == WINDOW) {
#ifdef XFree86Server
	 /* NOT_NEEDED */
#else
         XSync(b->display, False);
         if (!window_exists( b->display, b->frontxrb->drawable )) {
            /* found a dead window, free the ancillary info */
            XMesaDestroyBuffer( b );
         }
#endif
      }
   }
}


void XMesaReset( void )
{
    while (XMesaBufferList)
	XMesaDestroyBuffer(XMesaBufferList);

    XMesaBufferList = NULL;
}


unsigned long XMesaDitherColor( XMesaContext xmesa, GLint x, GLint y,
                                GLfloat red, GLfloat green,
                                GLfloat blue, GLfloat alpha )
{
   GLcontext *ctx = &xmesa->mesa;
   GLint r = (GLint) (red   * 255.0F);
   GLint g = (GLint) (green * 255.0F);
   GLint b = (GLint) (blue  * 255.0F);
   GLint a = (GLint) (alpha * 255.0F);

   switch (xmesa->pixelformat) {
      case PF_Index:
         return 0;
      case PF_Truecolor:
         {
            unsigned long p;
            PACK_TRUECOLOR( p, r, g, b );
            return p;
         }
      case PF_8A8B8G8R:
         return PACK_8A8B8G8R( r, g, b, a );
      case PF_8A8R8G8B:
         return PACK_8A8R8G8B( r, g, b, a );
      case PF_8R8G8B:
         return PACK_8R8G8B( r, g, b );
      case PF_5R6G5B:
         return PACK_5R6G5B( r, g, b );
      case PF_Dither:
         {
            DITHER_SETUP;
            return DITHER( x, y, r, g, b );
         }
      case PF_1Bit:
         /* 382 = (3*255)/2 */
         return ((r+g+b) > 382) ^ xmesa->xm_visual->bitFlip;
      case PF_HPCR:
         return DITHER_HPCR(x, y, r, g, b);
      case PF_Lookup:
         {
            LOOKUP_SETUP;
            return LOOKUP( r, g, b );
         }
      case PF_Grayscale:
         return GRAY_RGB( r, g, b );
      case PF_Dither_5R6G5B:
         /* fall through */
      case PF_Dither_True:
         {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, r, g, b);
            return p;
         }
      default:
         _mesa_problem(NULL, "Bad pixel format in XMesaDitherColor");
   }
   return 0;
}


/*
 * This is typically called when the window size changes and we need
 * to reallocate the buffer's back/depth/stencil/accum buffers.
 */
void XMesaResizeBuffers( XMesaBuffer b )
{
#ifdef XFree86Server
   GLuint winwidth, winheight;
   GET_CURRENT_CONTEXT(ctx);

   winwidth = MIN2(b->frontxrb->drawable->width, MAX_WIDTH);
   winheight = MIN2(b->frontxrb->drawable->height, MAX_HEIGHT);

   xmesa_resize_buffers(ctx, &(b->mesa_buffer), winwidth, winheight);
#else
   Window root;
   int xpos, ypos;
   unsigned int width, height, bw, depth;
   GET_CURRENT_CONTEXT(ctx);
   XGetGeometry( b->xm_visual->display, b->frontxrb->pixmap,
                 &root, &xpos, &ypos, &width, &height, &bw, &depth);
   xmesa_resize_buffers(ctx, &(b->mesa_buffer), width, height);
#endif
}

