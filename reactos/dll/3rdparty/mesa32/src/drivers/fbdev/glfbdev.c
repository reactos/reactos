/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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
#include "buffers.h"
#include "context.h"
#include "extensions.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "imports.h"
#include "renderbuffer.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "vbo/vbo.h"
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
   size_t size;                    /* color buffer size in bytes */
   GLuint bytesPerPixel;
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

/*
 * Derived from Mesa's gl_renderbuffer class.
 */
struct GLFBDevRenderbufferRec {
   struct gl_renderbuffer Base;
   GLubyte *bottom;                /* pointer to last row */
   GLuint rowStride;               /* in bytes */
   GLboolean mallocedBuffer;
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
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}


static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   const GLFBDevBufferPtr fbdevbuffer = GLFBDEV_BUFFER(buffer);
   *width = fbdevbuffer->var.xres;
   *height = fbdevbuffer->var.yres;
}


/**
 * We only implement this function as a mechanism to check if the
 * framebuffer size has changed (and update corresponding state).
 */
static void
viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   GLuint newWidth, newHeight;
   GLframebuffer *buffer;

   buffer = ctx->WinSysDrawBuffer;
   get_buffer_size( buffer, &newWidth, &newHeight );
   if (buffer->Width != newWidth || buffer->Height != newHeight) {
      _mesa_resize_framebuffer(ctx, buffer, newWidth, newHeight );
   }

   buffer = ctx->WinSysReadBuffer;
   get_buffer_size( buffer, &newWidth, &newHeight );
   if (buffer->Width != newWidth || buffer->Height != newHeight) {
      _mesa_resize_framebuffer(ctx, buffer, newWidth, newHeight );
   }
}


/*
 * Generate code for span functions.
 */

/* 24-bit BGR */
#define NAME(PREFIX) PREFIX##_B8G8R8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct GLFBDevRenderbufferRec *frb = (struct GLFBDevRenderbufferRec *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = frb->bottom - (Y) * frb->rowStride + (X) * 3
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[RCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 32-bit BGRA */
#define NAME(PREFIX) PREFIX##_B8G8R8A8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct GLFBDevRenderbufferRec *frb = (struct GLFBDevRenderbufferRec *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = frb->bottom - (Y) * frb->rowStride + (X) * 4
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[RCOMP];  \
   DST[3] = VALUE[ACOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]

#include "swrast/s_spantemp.h"


/* 16-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G6R5
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct GLFBDevRenderbufferRec *frb = (struct GLFBDevRenderbufferRec *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (frb->bottom - (Y) * frb->rowStride + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = ( (((VALUE[RCOMP]) & 0xf8) << 8) | (((VALUE[GCOMP]) & 0xfc) << 3) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((SRC[0]) >> 8) & 0xf8) | (((SRC[0]) >> 11) & 0x7) ); \
   DST[GCOMP] = ( (((SRC[0]) >> 3) & 0xfc) | (((SRC[0]) >>  5) & 0x3) ); \
   DST[BCOMP] = ( (((SRC[0]) << 3) & 0xf8) | (((SRC[0])      ) & 0x7) ); \
   DST[ACOMP] = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 15-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G5R5
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct GLFBDevRenderbufferRec *frb = (struct GLFBDevRenderbufferRec *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (frb->bottom - (Y) * frb->rowStride + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = ( (((VALUE[RCOMP]) & 0xf8) << 7) | (((VALUE[GCOMP]) & 0xf8) << 2) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((SRC[0]) >> 7) & 0xf8) | (((SRC[0]) >> 10) & 0x7) ); \
   DST[GCOMP] = ( (((SRC[0]) >> 2) & 0xf8) | (((SRC[0]) >>  5) & 0x7) ); \
   DST[BCOMP] = ( (((SRC[0]) << 3) & 0xf8) | (((SRC[0])      ) & 0x7) ); \
   DST[ACOMP] = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 8-bit color index */
#define NAME(PREFIX) PREFIX##_CI8
#define CI_MODE
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct GLFBDevRenderbufferRec *frb = (struct GLFBDevRenderbufferRec *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = frb->bottom - (Y) * frb->rowStride + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]

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
      return "1.0.1";
   default:
      return NULL;
   }
}


GLFBDevProc
glFBDevGetProcAddress( const char *procName )
{
   struct name_address {
      const char *name;
      const GLFBDevProc func;
   };
   static const struct name_address functions[] = {
      { "glFBDevGetString", (GLFBDevProc) glFBDevGetString },
      { "glFBDevGetProcAddress", (GLFBDevProc) glFBDevGetProcAddress },
      { "glFBDevCreateVisual", (GLFBDevProc) glFBDevCreateVisual },
      { "glFBDevDestroyVisual", (GLFBDevProc) glFBDevDestroyVisual },
      { "glFBDevGetVisualAttrib", (GLFBDevProc) glFBDevGetVisualAttrib },
      { "glFBDevCreateBuffer", (GLFBDevProc) glFBDevCreateBuffer },
      { "glFBDevDestroyBuffer", (GLFBDevProc) glFBDevDestroyBuffer },
      { "glFBDevGetBufferAttrib", (GLFBDevProc) glFBDevGetBufferAttrib },
      { "glFBDevGetCurrentDrawBuffer", (GLFBDevProc) glFBDevGetCurrentDrawBuffer },
      { "glFBDevGetCurrentReadBuffer", (GLFBDevProc) glFBDevGetCurrentReadBuffer },
      { "glFBDevSwapBuffers", (GLFBDevProc) glFBDevSwapBuffers },
      { "glFBDevCreateContext", (GLFBDevProc) glFBDevCreateContext },
      { "glFBDevDestroyContext", (GLFBDevProc) glFBDevDestroyContext },
      { "glFBDevGetContextAttrib", (GLFBDevProc) glFBDevGetContextAttrib },
      { "glFBDevGetCurrentContext", (GLFBDevProc) glFBDevGetCurrentContext },
      { "glFBDevMakeCurrent", (GLFBDevProc) glFBDevMakeCurrent },
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
      case GLFBDEV_MULTISAMPLE:
	 numSamples = attrib[1];
	 attrib++;
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

      if (fixInfo->visual == FB_VISUAL_TRUECOLOR ||
	  fixInfo->visual == FB_VISUAL_DIRECTCOLOR) {
	 if(varInfo->bits_per_pixel == 24
	    && varInfo->red.offset == 16
	    && varInfo->green.offset == 8
	    && varInfo->blue.offset == 0)
	    vis->pixelFormat = PF_B8G8R8;

	 else if(varInfo->bits_per_pixel == 32
		 && varInfo->red.offset == 16
		 && varInfo->green.offset == 8
		 && varInfo->blue.offset == 0)
	    vis->pixelFormat = PF_B8G8R8A8;

	 else if(varInfo->bits_per_pixel == 16
		 && varInfo->red.offset == 11
		 && varInfo->green.offset == 5
		 && varInfo->blue.offset == 0)
	    vis->pixelFormat = PF_B5G6R5;

	 else if(varInfo->bits_per_pixel == 16
		 && varInfo->red.offset == 10
		 && varInfo->green.offset == 5
		 && varInfo->blue.offset == 0)
	    vis->pixelFormat = PF_B5G5R5;

	 else {
	    _mesa_problem(NULL, "Unsupported fbdev RGB visual/bitdepth!\n");
	    _mesa_free(vis);
	    return NULL;
	 }
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
   /* XXX unfinished */
   (void) visual;
   (void) attrib;
   return -1;
}


static void
delete_renderbuffer(struct gl_renderbuffer *rb)
{
   struct GLFBDevRenderbufferRec *frb = (struct GLFBDevRenderbufferRec *) rb;
   if (frb->mallocedBuffer) {
      _mesa_free(frb->Base.Data);
   }
   _mesa_free(frb);
}


static GLboolean
renderbuffer_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
                     GLenum internalFormat, GLuint width, GLuint height)
{
   /* no-op: the renderbuffer storage is allocated just once when it's
    * created.  Never resized or reallocated.
    */
   return GL_TRUE;
}


static struct GLFBDevRenderbufferRec *
new_glfbdev_renderbuffer(void *bufferStart, const GLFBDevVisualPtr visual)
{
   struct GLFBDevRenderbufferRec *rb = CALLOC_STRUCT(GLFBDevRenderbufferRec);
   if (rb) {
      GLuint name = 0;
      int pixelFormat = visual->pixelFormat;

      _mesa_init_renderbuffer(&rb->Base, name);

      rb->Base.Delete = delete_renderbuffer;
      rb->Base.AllocStorage = renderbuffer_storage;

      if (pixelFormat == PF_B8G8R8) {
         rb->Base.GetRow = get_row_B8G8R8;
         rb->Base.GetValues = get_values_B8G8R8;
         rb->Base.PutRow = put_row_B8G8R8;
         rb->Base.PutRowRGB = put_row_rgb_B8G8R8;
         rb->Base.PutMonoRow = put_mono_row_B8G8R8;
         rb->Base.PutValues = put_values_B8G8R8;
         rb->Base.PutMonoValues = put_mono_values_B8G8R8;
      }
      else if (pixelFormat == PF_B8G8R8A8) {
         rb->Base.GetRow = get_row_B8G8R8A8;
         rb->Base.GetValues = get_values_B8G8R8A8;
         rb->Base.PutRow = put_row_B8G8R8A8;
         rb->Base.PutRowRGB = put_row_rgb_B8G8R8A8;
         rb->Base.PutMonoRow = put_mono_row_B8G8R8A8;
         rb->Base.PutValues = put_values_B8G8R8A8;
         rb->Base.PutMonoValues = put_mono_values_B8G8R8A8;
      }
      else if (pixelFormat == PF_B5G6R5) {
         rb->Base.GetRow = get_row_B5G6R5;
         rb->Base.GetValues = get_values_B5G6R5;
         rb->Base.PutRow = put_row_B5G6R5;
         rb->Base.PutRowRGB = put_row_rgb_B5G6R5;
         rb->Base.PutMonoRow = put_mono_row_B5G6R5;
         rb->Base.PutValues = put_values_B5G6R5;
         rb->Base.PutMonoValues = put_mono_values_B5G6R5;
      }
      else if (pixelFormat == PF_B5G5R5) {
         rb->Base.GetRow = get_row_B5G5R5;
         rb->Base.GetValues = get_values_B5G5R5;
         rb->Base.PutRow = put_row_B5G5R5;
         rb->Base.PutRowRGB = put_row_rgb_B5G5R5;
         rb->Base.PutMonoRow = put_mono_row_B5G5R5;
         rb->Base.PutValues = put_values_B5G5R5;
         rb->Base.PutMonoValues = put_mono_values_B5G5R5;
      }
      else if (pixelFormat == PF_CI8) {
         rb->Base.GetRow = get_row_CI8;
         rb->Base.GetValues = get_values_CI8;
         rb->Base.PutRow = put_row_CI8;
         rb->Base.PutMonoRow = put_mono_row_CI8;
         rb->Base.PutValues = put_values_CI8;
         rb->Base.PutMonoValues = put_mono_values_CI8;
      }

      if (pixelFormat == PF_CI8) {
         rb->Base.InternalFormat = GL_COLOR_INDEX8_EXT;
         rb->Base._BaseFormat = GL_COLOR_INDEX;
      }
      else {
         rb->Base.InternalFormat = GL_RGBA;
         rb->Base._BaseFormat = GL_RGBA;
      }
      rb->Base.DataType = GL_UNSIGNED_BYTE;
      rb->Base.Data = bufferStart;

      rb->rowStride = visual->var.xres_virtual * visual->var.bits_per_pixel / 8;
      rb->bottom = (GLubyte *) bufferStart
	  + (visual->var.yres - 1) * rb->rowStride;

      rb->Base.Width = visual->var.xres;
      rb->Base.Height = visual->var.yres;

      rb->Base.RedBits = visual->var.red.length;
      rb->Base.GreenBits = visual->var.green.length;
      rb->Base.BlueBits = visual->var.blue.length;
      rb->Base.AlphaBits = visual->var.transp.length;

      rb->Base.InternalFormat = pixelFormat;
   }
   return rb;
}

GLFBDevBufferPtr
glFBDevCreateBuffer( const struct fb_fix_screeninfo *fixInfo,
                     const struct fb_var_screeninfo *varInfo,
                     const GLFBDevVisualPtr visual,
                     void *frontBuffer, void *backBuffer, size_t size )
{
   struct GLFBDevRenderbufferRec *frontrb, *backrb;
   GLFBDevBufferPtr buf;

   ASSERT(visual);
   ASSERT(frontBuffer);
   ASSERT(size > 0);

   /* this is to update the visual if there was a resize and the
      buffer is created again */
   visual->var = *varInfo;
   visual->fix = *fixInfo;

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

   /* basic framebuffer setup */
   _mesa_initialize_framebuffer(&buf->glframebuffer, &visual->glvisual);
   /* add front renderbuffer */
   frontrb = new_glfbdev_renderbuffer(frontBuffer, visual);
   _mesa_add_renderbuffer(&buf->glframebuffer, BUFFER_FRONT_LEFT,
                          &frontrb->Base);
   /* add back renderbuffer */
   if (visual->glvisual.doubleBufferMode) {
      int malloced = !backBuffer;
      if (malloced) {
         /* malloc a back buffer */
         backBuffer = _mesa_malloc(size);
         if (!backBuffer) {
            _mesa_free_framebuffer_data(&buf->glframebuffer);
            _mesa_free(buf);
            return NULL;
         }
      }

      backrb = new_glfbdev_renderbuffer(backBuffer, visual);
      if(malloced)
	 backrb->mallocedBuffer = GL_TRUE;

      _mesa_add_renderbuffer(&buf->glframebuffer, BUFFER_BACK_LEFT,
                             &backrb->Base);
   }
   /* add software renderbuffers */
   _mesa_add_soft_renderbuffers(&buf->glframebuffer,
                                GL_FALSE, /* color */
                                visual->glvisual.haveDepthBuffer,
                                visual->glvisual.haveStencilBuffer,
                                visual->glvisual.haveAccumBuffer,
                                GL_FALSE, /* alpha */
                                GL_FALSE /* aux bufs */);

   buf->fix = *fixInfo;   /* struct assignment */
   buf->var = *varInfo;   /* struct assignment */
   buf->visual = visual;  /* ptr assignment */
   buf->size = size;
   buf->bytesPerPixel = visual->var.bits_per_pixel / 8;

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
#if 0
      /* free the software depth, stencil, accum buffers */
      _mesa_free_framebuffer_data(&buffer->glframebuffer);
      _mesa_free(buffer);
#else
      {
         struct gl_framebuffer *fb = &buffer->glframebuffer;
         _mesa_unreference_framebuffer(&fb);
      }
#endif
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
   struct GLFBDevRenderbufferRec *frontrb = (struct GLFBDevRenderbufferRec *)
      buffer->glframebuffer.Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
   struct GLFBDevRenderbufferRec *backrb = (struct GLFBDevRenderbufferRec *)
      buffer->glframebuffer.Attachment[BUFFER_BACK_LEFT].Renderbuffer;

   if (!buffer || !buffer->visual->glvisual.doubleBufferMode)
      return;

   /* check if swapping currently bound buffer */
   if (fbdevctx->drawBuffer == buffer) {
      /* flush pending rendering */
      _mesa_notifySwapBuffers(&fbdevctx->glcontext);
   }

   ASSERT(frontrb->Base.Data);
   ASSERT(backrb->Base.Data);
   _mesa_memcpy(frontrb->Base.Data, backrb->Base.Data, buffer->size);
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
   functions.Viewport = viewport;

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
   _vbo_CreateContext( glctx );
   _tnl_CreateContext( glctx );
   _swsetup_CreateContext( glctx );
   _swsetup_Wakeup( glctx );

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
      GLcontext *mesaCtx = &context->glcontext;

      _swsetup_DestroyContext( mesaCtx );
      _swrast_DestroyContext( mesaCtx );
      _tnl_DestroyContext( mesaCtx );
      _vbo_DestroyContext( mesaCtx );

      if (fbdevctx == context) {
         /* destroying current context */
         _mesa_make_current(NULL, NULL, NULL);
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
      _mesa_make_current( &context->glcontext,
                          &drawBuffer->glframebuffer,
                          &readBuffer->glframebuffer );
      context->drawBuffer = drawBuffer;
      context->readBuffer = readBuffer;
      context->curBuffer = drawBuffer;
   }
   else {
      /* unbind */
      _mesa_make_current( NULL, NULL, NULL );
   }

   return 1;
}

#endif /* USE_GLFBDEV_DRIVER */
