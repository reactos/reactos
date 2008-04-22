/*
 * Mesa 3-D graphics library
 * Version:  6.5
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


#include "glxheader.h"
#include "xmesaP.h"

#ifdef FX
#include "../glide/fxdrv.h"

void
FXcreateContext(XMesaVisual v, XMesaWindow w, XMesaContext c, XMesaBuffer b)
{
   char *fxEnvVar = _mesa_getenv("MESA_GLX_FX");
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
}


void FXdestroyContext( XMesaBuffer b )
{
   if (b && b->FXctx)
      fxMesaDestroyContext(b->FXctx);
}


GLboolean FXmakeCurrent( XMesaBuffer b )
{
   if (b->FXctx) {
      fxMesaMakeCurrent(b->FXctx);

      return GL_TRUE;
   }
   return GL_FALSE;
}


/*
 * Read image from VooDoo frame buffer into X/Mesa's back XImage.
 */
static void FXgetImage( XMesaBuffer b )
{
   GET_CURRENT_CONTEXT(ctx);
   static unsigned short pixbuf[MAX_WIDTH];
   GLuint x, y;
   GLuint width, height;

#ifdef XFree86Server
   x = b->frontxrb->pixmap->x;
   y = b->frontxrb->pixmap->y;
   width = b->frontxrb->pixmap->width;
   height = b->frontxrb->pixmap->height;
   depth = b->frontxrb->pixmap->depth;
#else
   xmesa_get_window_size(b->display, b, &width, &height);
   x = y = 0;
#endif
   if (b->mesa_buffer.Width != width || b->mesa_buffer.Height != height) {
      b->mesa_buffer.Width = MIN2((int)width, b->FXctx->width);
      b->mesa_buffer.Height = MIN2((int)height, b->FXctx->height);
      if (b->mesa_buffer.Width & 1)
         b->mesa_buffer.Width--;  /* prevent odd width */
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


GLboolean FXswapBuffers( XMesaBuffer b )
{
   if (b->FXctx) {
      fxMesaSwapBuffers();

      if (!b->FXwindowHack)
         return GL_TRUE;

      FXgetImage(b);
   }
   return GL_FALSE;
}


/*
 * Switch 3Dfx support hack between window and full-screen mode.
 */
GLboolean XMesaSetFXmode( GLint mode )
{
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
   return GL_FALSE;
}
#endif
