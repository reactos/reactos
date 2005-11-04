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


/*
 * OpenGL (Mesa) interface for fbdev.
 * For info about fbdev:
 * http://www.tldp.org/HOWTO/Framebuffer-HOWTO.html
 *
 * known VGA modes
 * Colours   640x400 640x480 800x600 1024x768 1152x864 1280x1024 1600x1200
 * --------+--------------------------------------------------------------
 *  4 bits |    ?       ?     0x302      ?        ?        ?         ?
 *  8 bits |  0x300   0x301   0x303    0x305    0x161    0x307     0x31C
 * 15 bits |    ?     0x310   0x313    0x316    0x162    0x319     0x31D
 * 16 bits |    ?     0x311   0x314    0x317    0x163    0x31A     0x31E
 * 24 bits |    ?     0x312   0x315    0x318      ?      0x31B     0x31F
 * 32 bits |    ?       ?       ?        ?      0x164      ?
 */


#ifdef USE_GLFBDEV_DRIVER

#include "glheader.h"
#include <linux/fb.h>
#include "GL/glfbdev.h"
#include "context.h"
#include "extensions.h"
#include "imports.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"


#define PF_B8G8R8     1
#define PF_B8G8R8A8   2
#define PF_B5G6R5     3
#define PF_B5G5R5     4
#define PF_CI8        5


/*
 * Derived from Mesa's GLvisual class.
 */
struct GLFBDevVisualRec {
   GLvisual glvisual;              /* base class */
   struct fb_fix_screeninfo fix;
   struct fb_var_screeninfo var;
   int pixelFormat;
};

/*
 * Derived from Mesa's GLframebuffer class.
 */
struct GLFBDevBufferRec {
   GLframebuffer glframebuffer;    /* base class */
   GLFBDevVisualPtr visual;
   struct fb_fix_screeninfo fix;
   struct fb_var_screeninfo var;
   void *frontStart;
   void *backStart;
   size_t size;
   GLuint bytesPerPixel;
   GLuint rowStride;               /* in bytes */
   GLubyte *frontBottom;           /* pointer to last row */
   GLubyte *backBottom;            /* pointer to last row */
   GLubyte *curBottom;             /* = frontBottom or backBottom */
   GLboolean mallocBackBuffer;
};

/*
 * Derived from Mesa's GLcontext class.
 */
struct GLFBDevContextRec {
   GLcontext glcontext;            /* base class */
   GLFBDevVisualPtr visual;
   GLFBDevBufferPtr drawBuffer;
   GLFBDevBufferPtr readBuffer;
   GLFBDevBufferPtr curBuffer;
};



#define GLFBDEV_CONTEXT(CTX)  ((GLFBDevContextPtr) (CTX))
#define GLFBDEV_BUFFER(BUF)  ((GLFBDevBufferPtr) (BUF))


/**********************************************************************/
/* Internal device driver functions                                   */
/**********************************************************************/


static const GLubyte *
get_string(GLcontext *ctx, GLenum pname)
{
   (void) ctx;
   switch (pname) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa glfbdev";
      default:
         return NULL;
   }
}


static void
update_state( GLcontext *ctx, GLuint new_state )
{
   /* not much to do here - pass it on */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}


static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   const GLFBDevBufferPtr fbdevbuffer = (GLFBDevBufferPtr) buffer;
   *width = fbdevbuffer->var.xres_virtual;
   *height = fbdevbuffer->var.yres_virtual;
}


/* specifies the buffer for swrast span rendering/reading */
static void
set_buffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit )
{
   GLFBDevContextPtr fbdevctx = GLFBDEV_CONTEXT(ctx);
   GLFBDevBufferPtr fbdevbuf = GLFBDEV_BUFFER(buffer);
   fbdevctx->curBuffer = fbdevbuf;
   switch (bufferBit) {
   case DD_FRONT_LEFT_BIT:
      fbdevbuf->curBottom = fbdevbuf->frontBottom;
      break;
   case DD_BACK_LEFT_BIT:
      fbdevbuf->curBottom = fbdevbuf->backBottom;
      break;
   default:
      _mesa_problem(ctx, "bad bufferBit in set_buffer()");
   }
}


/*
 * Generate code for span functions.
 */

/* 24-bit BGR */
#define NAME(PREFIX) PREFIX##_B8G8R8
#define SPAN_VARS \
   const GLFBDevContextPtr fbdevctx = GLFBDEV_CONTEXT(ctx); \
   const GLFBDevBufferPtr fbdevbuf = fbdevctx->curBuffer;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = fbdevbuf->curBottom - (Y) * fbdevbuf->rowStride + (X) * 3
#define INC_PIXEL_PTR(P) P += 3
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = B;  P[1] = G;  P[2] = R
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = B;  P[1] = G;  P[2] = R
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[2];  G = P[1];  B = P[0];  A = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 32-bit BGRA */
#define NAME(PREFIX) PREFIX##_B8G8R8A8
#define SPAN_VARS \
   const GLFBDevContextPtr fbdevctx = GLFBDEV_CONTEXT(ctx); \
   const GLFBDevBufferPtr fbdevbuf = fbdevctx->curBuffer;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = fbdevbuf->curBottom - (Y) * fbdevbuf->rowStride + (X) * 4
#define INC_PIXEL_PTR(P) P += 4
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = B;  P[1] = G;  P[2] = R;  P[3] = 255
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = B;  P[1] = G;  P[2] = R;  P[3] = A
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[2];  G = P[1];  B = P[0];  A = P[3]

#include "swrast/s_spantemp.h"


/* 16-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G6R5
#define SPAN_VARS \
   const GLFBDevContextPtr fbdevctx = GLFBDEV_CONTEXT(ctx); \
   const GLFBDevBufferPtr fbdevbuf = fbdevctx->curBuffer;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (fbdevbuf->curBottom - (Y) * fbdevbuf->rowStride + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   *P = ( (((R) & 0xf8) << 8) | (((G) & 0xfc) << 3) | ((B) >> 3) )
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   *P = ( (((R) & 0xf8) << 8) | (((G) & 0xfc) << 3) | ((B) >> 3) )
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = ( (((*P) >> 8) & 0xf8) | (((*P) >> 11) & 0x7) ); \
   G = ( (((*P) >> 3) & 0xfc) | (((*P) >>  5) & 0x3) ); \
   B = ( (((*P) << 3) & 0xf8) | (((*P)      ) & 0x7) ); \
   A = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 15-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G5R5
#define SPAN_VARS \
   const GLFBDevContextPtr fbdevctx = GLFBDEV_CONTEXT(ctx); \
   const GLFBDevBufferPtr fbdevbuf = fbdevctx->curBuffer;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (fbdevbuf->curBottom - (Y) * fbdevbuf->rowStride + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   *P = ( (((R) & 0xf8) << 7) | (((G) & 0xf8) << 2) | ((B) >> 3) )
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   *P = ( (((R) & 0xf8) << 7) | (((G) & 0xf8) << 2) | ((B) >> 3) )
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = ( (((*P) >> 7) & 0xf8) | (((*P) >> 10) & 0x7) ); \
   G = ( (((*P) >> 2) & 0xf8) | (((*P) >>  5) & 0x7) ); \
   B = ( (((*P) << 3) & 0xf8) | (((*P)      ) & 0x7) ); \
   A = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 8-bit color index */
#define NAME(PREFIX) PREFIX##_CI8
#define SPAN_VARS \
   const GLFBDevContextPtr fbdevctx = GLFBDEV_CONTEXT(ctx); \
   const GLFBDevBufferPtr fbdevbuf = fbdevctx->curBuffer;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = fbdevbuf->curBottom - (Y) * fbdevbuf->rowStride + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_CI_PIXEL(P, CI) \
   P[0] = CI
#define FETCH_CI_PIXEL(CI, P) \
   CI = P[0]

#include "swrast/s_spantemp.h"

/**********************************************************************/
/* Public API functions                                               */
/**********************************************************************/


const char *
glFBDevGetString( int str )
{
   switch (str) {
   case GLFBDEV_VENDOR:
      return "Mesa Project";
   case GLFBDEV_VERSION:
      return "1.0.0";
   default:
      return NULL;
   }
}


const void *
glFBDevGetProcAddress( const char *procName )
{
   struct name_address {
      const char *name;
      const void *func;
   };
   static const struct name_address functions[] = {
      { "glFBDevGetString", (void *) glFBDevGetString },
      { "glFBDevGetProcAddress", (void *) glFBDevGetProcAddress },
      { "glFBDevCreateVisual", (void *) glFBDevCreateVisual },
      { "glFBDevDestroyVisual", (void *) glFBDevDestroyVisual },
      { "glFBDevGetVisualAttrib", (void *) glFBDevGetVisualAttrib },
      { "glFBDevCreateBuffer", (void *) glFBDevCreateBuffer },
      { "glFBDevDestroyBuffer", (void *) glFBDevDestroyBuffer },
      { "glFBDevGetBufferAttrib", (void *) glFBDevGetBufferAttrib },
      { "glFBDevGetCurrentDrawBuffer", (void *) glFBDevGetCurrentDrawBuffer },
      { "glFBDevGetCurrentReadBuffer", (void *) glFBDevGetCurrentReadBuffer },
      { "glFBDevSwapBuffers", (void *) glFBDevSwapBuffers },
      { "glFBDevCreateContext", (void *) glFBDevCreateContext },
      { "glFBDevDestroyContext", (void *) glFBDevDestroyContext },
      { "glFBDevGetContextAttrib", (void *) glFBDevGetContextAttrib },
      { "glFBDevGetCurrentContext", (void *) glFBDevGetCurrentContext },
      { "glFBDevMakeCurrent", (void *) glFBDevMakeCurrent },
      { NULL, NULL }
   };
   const struct name_address *entry;
   for (entry = functions; entry->name; entry++) {
      if (_mesa_strcmp(entry->name, procName) == 0) {
         return entry->func;
      }
   }
   return _glapi_get_proc_address(procName);
}


GLFBDevVisualPtr
glFBDevCreateVisual( const struct fb_fix_screeninfo *fixInfo,
                     const struct fb_var_screeninfo *varInfo,
                     const int *attribs )
{
   GLFBDevVisualPtr vis;
   const int *attrib;
   GLboolean rgbFlag = GL_TRUE, dbFlag = GL_FALSE, stereoFlag = GL_FALSE;
   GLint redBits = 0, greenBits = 0, blueBits = 0, alphaBits = 0;
   GLint indexBits = 0, depthBits = 0, stencilBits = 0;
   GLint accumRedBits = 0, accumGreenBits = 0;
   GLint accumBlueBits = 0, accumAlphaBits = 0;
   GLint numSamples = 0;

   ASSERT(fixInfo);
   ASSERT(varInfo);

   vis = CALLOC_STRUCT(GLFBDevVisualRec);
   if (!vis)
      return NULL;

   vis->fix = *fixInfo;  /* struct assignment */
   vis->var = *varInfo;  /* struct assignment */

   for (attrib = attribs; attrib && *attrib != GLFBDEV_NONE; attrib++) {
      switch (*attrib) {
      case GLFBDEV_DOUBLE_BUFFER:
         dbFlag = GL_TRUE;
         break;
      case GLFBDEV_COLOR_INDEX:
         rgbFlag = GL_FALSE;
         break;
      case GLFBDEV_DEPTH_SIZE:
         depthBits = attrib[1];
         attrib++;
         break;
      case GLFBDEV_STENCIL_SIZE:
         stencilBits = attrib[1];
         attrib++;
         break;
      case GLFBDEV_ACCUM_SIZE:
         accumRedBits = accumGreenBits = accumBlueBits = accumAlphaBits
            = attrib[1];
         attrib++;
         break;
      case GLFBDEV_LEVEL:
         /* ignored for now */
         break;
      default:
         /* unexpected token */
         _mesa_free(vis);
         return NULL;
      }
   }

   if (rgbFlag) {
      redBits   = varInfo->red.length;
      greenBits = varInfo->green.length;
      blueBits  = varInfo->blue.length;
      alphaBits = varInfo->transp.length;

      if ((fixInfo->visual == FB_VISUAL_TRUECOLOR ||
           fixInfo->visual == FB_VISUAL_DIRECTCOLOR)
          && varInfo->bits_per_pixel == 24
          && varInfo->red.offset == 16
          && varInfo->green.offset == 8
          && varInfo->blue.offset == 0) {
         vis->pixelFormat = PF_B8G8R8;
      }
      else if ((fixInfo->visual == FB_VISUAL_TRUECOLOR ||
                fixInfo->visual == FB_VISUAL_DIRECTCOLOR)
               && varInfo->bits_per_pixel == 32
               && varInfo->red.offset == 16
               && varInfo->green.offset == 8
               && varInfo->blue.offset == 0
               && varInfo->transp.offset == 24) {
         vis->pixelFormat = PF_B8G8R8A8;
      }
      else if ((fixInfo->visual == FB_VISUAL_TRUECOLOR ||
                fixInfo->visual == FB_VISUAL_DIRECTCOLOR)
               && varInfo->bits_per_pixel == 16
               && varInfo->red.offset == 11
               && varInfo->green.offset == 5
               && varInfo->blue.offset == 0) {
         vis->pixelFormat = PF_B5G6R5;
      }
      else if ((fixInfo->visual == FB_VISUAL_TRUECOLOR ||
                fixInfo->visual == FB_VISUAL_DIRECTCOLOR)
               && varInfo->bits_per_pixel == 16
               && varInfo->red.offset == 10
               && varInfo->green.offset == 5
               && varInfo->blue.offset == 0) {
         vis->pixelFormat = PF_B5G5R5;
      }
      else {
         _mesa_problem(NULL, "Unsupported fbdev RGB visual/bitdepth!\n");
         /*
         printf("fixInfo->visual = 0x%x\n", fixInfo->visual);
         printf("varInfo->bits_per_pixel = %d\n", varInfo->bits_per_pixel);
         printf("varInfo->red.offset = %d\n", varInfo->red.offset);
         printf("varInfo->green.offset = %d\n", varInfo->green.offset);
         printf("varInfo->blue.offset = %d\n", varInfo->blue.offset);
         */
         _mesa_free(vis);
         return NULL;
      }
   }
   else {
      indexBits = varInfo->bits_per_pixel;
      if ((fixInfo->visual == FB_VISUAL_PSEUDOCOLOR ||
           fixInfo->visual == FB_VISUAL_STATIC_PSEUDOCOLOR)
          && varInfo->bits_per_pixel == 8) {
         vis->pixelFormat = PF_CI8;
      }
      else {
         _mesa_problem(NULL, "Unsupported fbdev CI visual/bitdepth!\n");
         _mesa_free(vis);
         return NULL;
      }
   }

   if (!_mesa_initialize_visual(&vis->glvisual, rgbFlag, dbFlag, stereoFlag,
                                redBits, greenBits, blueBits, alphaBits,
                                indexBits, depthBits, stencilBits,
                                accumRedBits, accumGreenBits,
                                accumBlueBits, accumAlphaBits,
                                numSamples)) {
      /* something was invalid */
      _mesa_free(vis);
      return NULL;
   }

   return vis;
}


void
glFBDevDestroyVisual( GLFBDevVisualPtr visual )
{
   if (visual)
      _mesa_free(visual);
}


int
glFBDevGetVisualAttrib( const GLFBDevVisualPtr visual, int attrib)
{
   (void) visual;
   (void) attrib;
   return -1;
}



GLFBDevBufferPtr
glFBDevCreateBuffer( const struct fb_fix_screeninfo *fixInfo,
                     const struct fb_var_screeninfo *varInfo,
                     const GLFBDevVisualPtr visual,
                     void *frontBuffer, void *backBuffer, size_t size )
{
   GLFBDevBufferPtr buf;

   ASSERT(visual);
   ASSERT(frontBuffer);
   ASSERT(size > 0);

   if (visual->fix.visual != fixInfo->visual ||
       visual->fix.type != fixInfo->type ||
       visual->var.bits_per_pixel != varInfo->bits_per_pixel ||
       visual->var.grayscale != varInfo->grayscale ||
       visual->var.red.offset != varInfo->red.offset ||
       visual->var.green.offset != varInfo->green.offset ||
       visual->var.blue.offset != varInfo->blue.offset ||
       visual->var.transp.offset != varInfo->transp.offset) {
      /* visual mismatch! */
      return NULL;
   }

   buf = CALLOC_STRUCT(GLFBDevBufferRec);
   if (!buf)
      return NULL;

   _mesa_initialize_framebuffer(&buf->glframebuffer, &visual->glvisual,
                                visual->glvisual.haveDepthBuffer,
                                visual->glvisual.haveStencilBuffer,
                                visual->glvisual.haveAccumBuffer,
                                GL_FALSE);

   buf->fix = *fixInfo;   /* struct assignment */
   buf->var = *varInfo;   /* struct assignment */
   buf->visual = visual;  /* ptr assignment */
   buf->frontStart = frontBuffer;
   buf->size = size;
   buf->bytesPerPixel = visual->var.bits_per_pixel / 8;
   buf->rowStride = visual->var.xres_virtual * buf->bytesPerPixel;
   buf->frontBottom = (GLubyte *) buf->frontStart
                    + (visual->var.yres_virtual - 1) * buf->rowStride;

   if (visual->glvisual.doubleBufferMode) {
      if (backBuffer) {
         buf->backStart = backBuffer;
         buf->mallocBackBuffer = GL_FALSE;
      }
      else {
         buf->backStart = _mesa_malloc(size);
         if (!buf->backStart) {
            _mesa_free_framebuffer_data(&buf->glframebuffer);
            _mesa_free(buf);
            return NULL;
         }
         buf->mallocBackBuffer = GL_TRUE;
      }
      buf->backBottom = (GLubyte *) buf->backStart
                      + (visual->var.yres_virtual - 1) * buf->rowStride;
      buf->curBottom = buf->backBottom;
   }
   else {
      buf->backStart = NULL;
      buf->mallocBackBuffer = GL_FALSE;
      buf->backBottom = NULL;
      buf->curBottom = buf->frontBottom;
   }

   return buf;
}


void
glFBDevDestroyBuffer( GLFBDevBufferPtr buffer )
{
   if (buffer) {
      /* check if destroying the current buffer */
      GLFBDevBufferPtr curDraw = glFBDevGetCurrentDrawBuffer();
      GLFBDevBufferPtr curRead = glFBDevGetCurrentReadBuffer();
      if (buffer == curDraw || buffer == curRead) {
         glFBDevMakeCurrent( NULL, NULL, NULL);
      }
      if (buffer->mallocBackBuffer) {
         _mesa_free(buffer->backStart);
      }
      /* free the software depth, stencil, accum buffers */
      _mesa_free_framebuffer_data(&buffer->glframebuffer);
      _mesa_free(buffer);
   }
}


int
glFBDevGetBufferAttrib( const GLFBDevBufferPtr buffer, int attrib)
{
   (void) buffer;
   (void) attrib;
   return -1;
}


GLFBDevBufferPtr
glFBDevGetCurrentDrawBuffer( void )
{
   GLFBDevContextPtr fbdevctx = glFBDevGetCurrentContext();
   if (fbdevctx)
      return fbdevctx->drawBuffer;
   else
      return NULL;
}


GLFBDevBufferPtr
glFBDevGetCurrentReadBuffer( void )
{
   GLFBDevContextPtr fbdevctx = glFBDevGetCurrentContext();
   if (fbdevctx)
      return fbdevctx->readBuffer;
   else
      return NULL;
}


void
glFBDevSwapBuffers( GLFBDevBufferPtr buffer )
{
   GLFBDevContextPtr fbdevctx = glFBDevGetCurrentContext();

   if (!buffer || !buffer->visual->glvisual.doubleBufferMode)
      return;

   /* check if swapping currently bound buffer */
   if (fbdevctx->drawBuffer == buffer) {
      /* flush pending rendering */
      _mesa_notifySwapBuffers(&fbdevctx->glcontext);
   }

   ASSERT(buffer->frontStart);
   ASSERT(buffer->backStart);
   _mesa_memcpy(buffer->frontStart, buffer->backStart, buffer->size);
}


GLFBDevContextPtr
glFBDevCreateContext( const GLFBDevVisualPtr visual, GLFBDevContextPtr share )
{
   GLFBDevContextPtr ctx;
   GLcontext *glctx;
   struct dd_function_table functions;

   ASSERT(visual);

   ctx = CALLOC_STRUCT(GLFBDevContextRec);
   if (!ctx)
      return NULL;

   /* build table of device driver functions */
   _mesa_init_driver_functions(&functions);
   functions.GetString = get_string;
   functions.UpdateState = update_state;
   functions.GetBufferSize = get_buffer_size;

   if (!_mesa_initialize_context(&ctx->glcontext, &visual->glvisual,
                                 share ? &share->glcontext : NULL,
                                 &functions, (void *) ctx)) {
      _mesa_free(ctx);
      return NULL;
   }

   ctx->visual = visual;

   /* Create module contexts */
   glctx = (GLcontext *) &ctx->glcontext;
   _swrast_CreateContext( glctx );
   _ac_CreateContext( glctx );
   _tnl_CreateContext( glctx );
   _swsetup_CreateContext( glctx );
   _swsetup_Wakeup( glctx );

   /* swrast init */
   {
      struct swrast_device_driver *swdd;
      swdd = _swrast_GetDeviceDriverReference( glctx );
      swdd->SetBuffer = set_buffer;
      if (visual->pixelFormat == PF_B8G8R8) {
         swdd->WriteRGBASpan = write_rgba_span_B8G8R8;
         swdd->WriteRGBSpan = write_rgb_span_B8G8R8;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B8G8R8;
         swdd->WriteRGBAPixels = write_rgba_pixels_B8G8R8;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B8G8R8;
         swdd->ReadRGBASpan = read_rgba_span_B8G8R8;
         swdd->ReadRGBAPixels = read_rgba_pixels_B8G8R8;
      }
      else if (visual->pixelFormat == PF_B8G8R8A8) {
         swdd->WriteRGBASpan = write_rgba_span_B8G8R8A8;
         swdd->WriteRGBSpan = write_rgb_span_B8G8R8A8;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B8G8R8A8;
         swdd->WriteRGBAPixels = write_rgba_pixels_B8G8R8A8;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B8G8R8A8;
         swdd->ReadRGBASpan = read_rgba_span_B8G8R8A8;
         swdd->ReadRGBAPixels = read_rgba_pixels_B8G8R8A8;
      }
      else if (visual->pixelFormat == PF_B5G6R5) {
         swdd->WriteRGBASpan = write_rgba_span_B5G6R5;
         swdd->WriteRGBSpan = write_rgb_span_B5G6R5;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B5G6R5;
         swdd->WriteRGBAPixels = write_rgba_pixels_B5G6R5;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B5G6R5;
         swdd->ReadRGBASpan = read_rgba_span_B5G6R5;
         swdd->ReadRGBAPixels = read_rgba_pixels_B5G6R5;
      }
      else if (visual->pixelFormat == PF_B5G5R5) {
         swdd->WriteRGBASpan = write_rgba_span_B5G5R5;
         swdd->WriteRGBSpan = write_rgb_span_B5G5R5;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B5G5R5;
         swdd->WriteRGBAPixels = write_rgba_pixels_B5G5R5;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B5G5R5;
         swdd->ReadRGBASpan = read_rgba_span_B5G5R5;
         swdd->ReadRGBAPixels = read_rgba_pixels_B5G5R5;
      }
      else if (visual->pixelFormat == PF_CI8) {
         swdd->WriteCI32Span = write_index32_span_CI8;
         swdd->WriteCI8Span = write_index8_span_CI8;
         swdd->WriteMonoCISpan = write_monoindex_span_CI8;
         swdd->WriteCI32Pixels = write_index_pixels_CI8;
         swdd->WriteMonoCIPixels = write_monoindex_pixels_CI8;
         swdd->ReadCI32Span = read_index_span_CI8;
         swdd->ReadCI32Pixels = read_index_pixels_CI8;
      }
      else {
         _mesa_printf("bad pixelformat: %d\n", visual->pixelFormat);
      }
   }

   /* use default TCL pipeline */
   {
      TNLcontext *tnl = TNL_CONTEXT(glctx);
      tnl->Driver.RunPipeline = _tnl_run_pipeline;
   }

   _mesa_enable_sw_extensions(glctx);

   return ctx;
}


void
glFBDevDestroyContext( GLFBDevContextPtr context )
{
   GLFBDevContextPtr fbdevctx = glFBDevGetCurrentContext();

   if (context) {
      if (fbdevctx == context) {
         /* destroying current context */
         _mesa_make_current2(NULL, NULL, NULL);
         _mesa_notifyDestroy(&context->glcontext);
      }
      _mesa_free_context_data(&context->glcontext);
      _mesa_free(context);
   }
}


int
glFBDevGetContextAttrib( const GLFBDevContextPtr context, int attrib)
{
   (void) context;
   (void) attrib;
   return -1;
}


GLFBDevContextPtr
glFBDevGetCurrentContext( void )
{
   GET_CURRENT_CONTEXT(ctx);
   return (GLFBDevContextPtr) ctx;
}


int
glFBDevMakeCurrent( GLFBDevContextPtr context,
                    GLFBDevBufferPtr drawBuffer,
                    GLFBDevBufferPtr readBuffer )
{
   if (context && drawBuffer && readBuffer) {
      /* Make sure the context's visual and the buffers' visuals match.
       * XXX we might do this by comparing specific fields like bits_per_pixel,
       * visual, etc. in the future.
       */
      if (context->visual != drawBuffer->visual ||
          context->visual != readBuffer->visual) {
         return 0;
      }
      _mesa_make_current2( &context->glcontext,
                           &drawBuffer->glframebuffer,
                           &readBuffer->glframebuffer );
      context->drawBuffer = drawBuffer;
      context->readBuffer = readBuffer;
      context->curBuffer = drawBuffer;
   }
   else {
      /* unbind */
      _mesa_make_current2( NULL, NULL, NULL );
   }

   return 1;
}

#endif /* USE_GLFBDEV_DRIVER */

