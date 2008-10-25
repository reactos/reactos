/* $Id: svgamesa.c,v 1.27 2006/10/15 18:51:22 brianp Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include "GL/svgamesa.h"
#include "buffers.h"
#include "context.h"
#include "extensions.h"
#include "imports.h"
#include "matrix.h"
#include "mtypes.h"
#include "swrast/swrast.h"
#include "svgapix.h"
#include "svgamesa8.h"
#include "svgamesa15.h"
#include "svgamesa16.h"
#include "svgamesa24.h"
#include "svgamesa32.h"

struct svga_buffer SVGABuffer;
vga_modeinfo * SVGAInfo;
SVGAMesaContext SVGAMesa;    /* the current context */

#ifdef SVGA_DEBUG

#include <sys/types.h>
#include <signal.h>

FILE * logfile;
char cbuf[1024]={0};

void SVGAlog(char * what)
{
 logfile=fopen("svgamesa.log","a");
 if (!logfile) return;
 fprintf(logfile,"%s\n",what);
 fclose(logfile);
}
#endif

/**********************************************************************/
/*****                       Init stuff...                        *****/
/**********************************************************************/

int SVGAMesaInit( int GraphMode )
{
   vga_init();
   if (!vga_hasmode(GraphMode))
   {
    fprintf(stderr,"GraphMode %d unavailable...",GraphMode);
#ifdef SVGA_DEBUG
    SVGAlog("SVGAMesaInit: invalid GraphMode (doesn't exist)");
#endif
    return(1);
   }
   SVGAInfo=vga_getmodeinfo(GraphMode);
   if (SVGAInfo->flags & IS_MODEX)
   {
    fprintf(stderr,"ModeX not implemented...");
#ifdef SVGA_DEBUG
    SVGAlog("SVGAMesaInit: invalid GraphMode (ModeX)");
#endif
    return(2);
   }
   if (!SVGAInfo->bytesperpixel)
   {
    fprintf(stderr,"1 / 4 bit color not implemented...");
#ifdef SVGA_DEBUG
    SVGAlog("SVGAMesaInit: invalid GraphMode (1 or 4 bit)");
#endif
    return(3);
   }
   switch (SVGAInfo->colors) {
    case   256: SVGABuffer.Depth = 8;  break;
    case 32768: SVGABuffer.Depth = 15; break;
    case 65536: SVGABuffer.Depth = 16; break;
    default: SVGABuffer.Depth = SVGAInfo->bytesperpixel<<3; break;
   }
   SVGABuffer.BufferSize=SVGAInfo->linewidth*SVGAInfo->height;
#ifdef SVGA_DEBUG
   sprintf(cbuf,"SVGAMesaInit: double buffer info.\n" \
                 "              depth  : %d\n" \
                 "              mode   : %d\n" \
		 "              width  : %d\n" \
		 "              height : %d\n" \
		 "              bufsize: %d\n", \
		 SVGABuffer.Depth,GraphMode,SVGAInfo->linewidth, \
		 SVGAInfo->height,SVGABuffer.BufferSize);
   SVGAlog(cbuf);
#endif
   SVGABuffer.FrontBuffer=(void*)malloc(SVGABuffer.BufferSize + 4);
   if (!SVGABuffer.FrontBuffer) {
    {
     fprintf(stderr,"Not enough RAM for FRONT_LEFT_BUFFER...");
#ifdef SVGA_DEBUG
     SVGAlog("SVGAMesaInit: Not enough RAM (front buffer)");
#endif
     return(4);
    }
   }
#ifdef SVGA_DEBUG
   sprintf(cbuf,"SVGAMesaInit: FrontBuffer - %p",SVGABuffer.FrontBuffer);
   SVGAlog(cbuf);
#endif
   SVGABuffer.BackBuffer=(void*)malloc(SVGABuffer.BufferSize + 4);
   if (!SVGABuffer.BackBuffer) {
    {
     free(SVGABuffer.FrontBuffer);
     fprintf(stderr,"Not enough RAM for BACK_LEFT_BUFFER...");
#ifdef SVGA_DEBUG
     SVGAlog("SVGAMesaInit: Not enough RAM (back buffer)");
#endif
     return(5);
    }
   }
#ifdef SVGA_DEBUG
   sprintf(cbuf,"SVGAMesaInit: BackBuffer - %p",SVGABuffer.BackBuffer);
   SVGAlog(cbuf);
#endif

   vga_setmode(GraphMode);
   SVGABuffer.VideoRam=vga_getgraphmem();
#ifdef SVGA_DEBUG
   sprintf(cbuf,"SVGAMesaInit: VRAM - %p",SVGABuffer.VideoRam);
   SVGAlog(cbuf);
   sprintf(cbuf,"SVGAMesaInit: done. (Mode %d)",GraphMode);
   SVGAlog(cbuf);
#endif

   SVGABuffer.DrawBuffer = SVGABuffer.BackBuffer;
   SVGABuffer.ReadBuffer = SVGABuffer.BackBuffer;

   return 0;
}

int SVGAMesaClose( void )
{
   vga_setmode(TEXT);
   free(SVGABuffer.FrontBuffer);
   free(SVGABuffer.BackBuffer);
   return 0;
}

void SVGAMesaSetCI(int ndx, GLubyte red, GLubyte green, GLubyte blue)
{
   if (ndx<256)
      vga_setpalette(ndx, red>>2, green>>2, blue>>2);
}

/**********************************************************************/
/*****                 Miscellaneous functions                    *****/
/**********************************************************************/

static void copy_buffer( const GLubyte * buffer) {
 int size = SVGABuffer.BufferSize, page = 0;

#ifdef SVGA_DEBUG
   sprintf(cbuf,"copy_buffer: copy %p to %p",buffer,SVGABuffer.VideoRam);
   SVGAlog(cbuf);
#endif

 while(size>0) {
 vga_setpage(page++);
  if (size>>16) {
   memcpy(SVGABuffer.VideoRam,buffer,0x10000);
   buffer+=0x10000;
  }else{
   memcpy(SVGABuffer.VideoRam,buffer,size & 0xffff);
  }
  size-=0xffff;
 }
}

static void get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   *width = SVGAMesa->width = vga_getxdim();
   *height = SVGAMesa->height = vga_getydim();
}

/**
 * We only implement this function as a mechanism to check if the
 * framebuffer size has changed (and update corresponding state).
 */
static void viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   GLuint newWidth, newHeight;
   GLframebuffer *buffer = ctx->WinSysDrawBuffer;
   get_buffer_size( buffer, &newWidth, &newHeight );
   if (buffer->Width != newWidth || buffer->Height != newHeight) {
      _mesa_resize_framebuffer(ctx, buffer, newWidth, newHeight );
   }
}

static void set_buffer( GLcontext *ctx, GLframebuffer *colorBuffer,
                        GLenum buffer )
{
   /* We can ignore colorBuffer since we don't support a MakeCurrentRead()
    * function.
    */
   (void) colorBuffer;

   if (buffer == GL_FRONT_LEFT) {
      SVGABuffer.ReadBuffer = SVGABuffer.FrontBuffer;
      SVGABuffer.DrawBuffer = SVGABuffer.FrontBuffer;
#if 0
      void * tmpptr;
      /*    vga_waitretrace(); */
      copy_buffer(SVGABuffer.FrontBuffer);
      tmpptr=SVGABuffer.BackBuffer;
      SVGABuffer.BackBuffer=SVGABuffer.FrontBuffer;
      SVGABuffer.FrontBuffer=tmpptr;
#endif
   }
   else if (buffer == GL_BACK_LEFT) {
      SVGABuffer.ReadBuffer = SVGABuffer.BackBuffer;
      SVGABuffer.DrawBuffer = SVGABuffer.BackBuffer;
#if 0
      /*    vga_waitretrace(); */
      copy_buffer(SVGABuffer.BackBuffer);
#endif
   }
}

/**********************************************************************/
/*****                                                            *****/
/**********************************************************************/

static void svgamesa_update_state( GLcontext *ctx, GLuint new_state )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );

   /* Initialize all the pointers in the DD struct.  Do this whenever */
   /* a new context is made current or we change buffers via set_buffer! */

   ctx->Driver.UpdateState = svgamesa_update_state;

   ctx->Driver.GetBufferSize = get_buffer_size;
   ctx->Driver.Viewport = viewport;

   /* Fill in the swrast driver interface:
    */
   swdd->SetBuffer = set_buffer;

   switch (SVGABuffer.Depth) {
    case  8: ctx->Driver.ClearIndex = __clear_index8;
             ctx->Driver.Clear 	    = __clear8;

             swdd->ReadCI32Span         = __read_ci32_span8;
             swdd->ReadCI32Pixels       = __read_ci32_pixels8;
             swdd->WriteCI8Span         = __write_ci8_span8;
             swdd->WriteCI32Span        = __write_ci32_span8;
             swdd->WriteCI32Pixels      = __write_ci32_pixels8;
             swdd->WriteMonoCISpan      = __write_mono_ci_span8;
             swdd->WriteMonoCIPixels    = __write_mono_ci_pixels8;
#ifdef SVGA_DEBUG
    SVGAlog("SVGAUpdateState: 8 bit mode.");
#endif

	     break;
    case 15: ctx->Driver.ClearColor = __clear_color15;
             ctx->Driver.Clear 	    = __clear15;

             swdd->ReadRGBASpan         = __read_rgba_span15;
             swdd->ReadRGBAPixels       = __read_rgba_pixels15;
             swdd->WriteRGBASpan        = __write_rgba_span15;
             swdd->WriteRGBAPixels      = __write_rgba_pixels15;
             swdd->WriteMonoRGBASpan    = __write_mono_rgba_span15;
             swdd->WriteMonoRGBAPixels  = __write_mono_rgba_pixels15;
#ifdef SVGA_DEBUG
    SVGAlog("SVGAUpdateState: 15 bit mode.");
#endif
	     break;
    case 16: ctx->Driver.ClearColor = __clear_color16;
             ctx->Driver.Clear 	    = __clear16;

             swdd->ReadRGBASpan         = __read_rgba_span16;
             swdd->ReadRGBAPixels       = __read_rgba_pixels16;
             swdd->WriteRGBASpan        = __write_rgba_span16;
             swdd->WriteRGBAPixels      = __write_rgba_pixels16;
             swdd->WriteMonoRGBASpan    = __write_mono_rgba_span16;
             swdd->WriteMonoRGBAPixels  = __write_mono_rgba_pixels16;
	     break;
#ifdef SVGA_DEBUG
    SVGAlog("SVGAUpdateState: 16 bit mode.");
#endif
    case 24: ctx->Driver.ClearColor = __clear_color24;
             ctx->Driver.Clear 	    = __clear24;

             swdd->ReadRGBASpan         = __read_rgba_span24;
             swdd->ReadRGBAPixels       = __read_rgba_pixels24;
             swdd->WriteRGBASpan        = __write_rgba_span24;
             swdd->WriteRGBAPixels      = __write_rgba_pixels24;
             swdd->WriteMonoRGBASpan    = __write_mono_rgba_span24;
             swdd->WriteMonoRGBAPixels  = __write_mono_rgba_pixels24;
	     break;
#ifdef SVGA_DEBUG
    SVGAlog("SVGAUpdateState: 32 bit mode.");
#endif
    case 32: ctx->Driver.ClearColor = __clear_color32;
             ctx->Driver.Clear 	    = __clear32;

             swdd->ReadRGBASpan         = __read_rgba_span32;
             swdd->ReadRGBAPixels       = __read_rgba_pixels32;
             swdd->WriteRGBASpan        = __write_rgba_span32;
             swdd->WriteRGBAPixels      = __write_rgba_pixels32;
             swdd->WriteMonoRGBASpan    = __write_mono_rgba_span32;
             swdd->WriteMonoRGBAPixels  = __write_mono_rgba_pixels32;
   }	
}

/*
 * Create a new VGA/Mesa context and return a handle to it.
 */
SVGAMesaContext SVGAMesaCreateContext( GLboolean doubleBuffer )
{
   SVGAMesaContext ctx;
#ifndef DEV
   GLboolean rgb_flag;
   GLfloat redscale, greenscale, bluescale, alphascale;
   GLint index_bits;
   GLint redbits, greenbits, bluebits, alphabits;

   /* determine if we're in RGB or color index mode */
   if ((SVGABuffer.Depth==32) || (SVGABuffer.Depth==24)) {
      rgb_flag = GL_TRUE;
      redscale = greenscale = bluescale = alphascale = 255.0;
      redbits = greenbits = bluebits = 8;
      alphabits = 0;
      index_bits = 0;
   }
   else if (SVGABuffer.Depth==8) {
      rgb_flag = GL_FALSE;
      redscale = greenscale = bluescale = alphascale = 0.0;
      redbits = greenbits = bluebits = alphabits = 0;
      index_bits = 8;
   }
   else if (SVGABuffer.Depth==15) {
      rgb_flag = GL_TRUE;
      redscale = greenscale = bluescale = alphascale = 31.0;
      redbits = greenbits = bluebits = 5;
      alphabits = 0;
      index_bits = 0;
   }
   else if (SVGABuffer.Depth==16) {
      rgb_flag = GL_TRUE;
      redscale = bluescale = alphascale = 31.0;
      greenscale = 63.0;
      redbits = bluebits = 5;
      greenbits = 6;
      alphabits = 0;
      index_bits = 0;
   }

   ctx = (SVGAMesaContext) calloc( 1, sizeof(struct svgamesa_context) );
   if (!ctx) {
      return NULL;
   }

   ctx->gl_vis = _mesa_create_visual( rgb_flag,
                                      doubleBuffer,
                                      GL_FALSE,  /* stereo */
                                      redbits, greenbits,
                                      bluebits, alphabits,
                                      index_bits,
                                      16,   /* depth_size */
                                      8,    /* stencil_size */
                                      16, 16, 16, 16,   /* accum_size */
                                      1     /* samples */
                                      );

   ctx->gl_ctx = _mesa_create_context( ctx->gl_vis,
                                       NULL,  /* share list context */
                                       (void *) ctx, GL_FALSE );

   _mesa_enable_sw_extensions(ctx->gl_ctx);
   _mesa_enable_1_3_extensions(ctx->gl_ctx);

   _mesa_init_driver_functions(&ctx->Driver);

   ctx->gl_buffer = _mesa_create_framebuffer( ctx->gl_vis,
                                              ctx->gl_vis->depthBits > 0,
                                              ctx->gl_vis->stencilBits > 0,
                                              ctx->gl_vis->accumRedBits > 0,
                                              ctx->gl_vis->alphaBits > 0 );

   ctx->width = ctx->height = 0;  /* temporary until first "make-current" */
#endif
   return ctx;
}

/*
 * Destroy the given VGA/Mesa context.
 */
void SVGAMesaDestroyContext( SVGAMesaContext ctx )
{
#ifndef DEV
   if (ctx) {
      _mesa_destroy_visual( ctx->gl_vis );
      _mesa_destroy_context( ctx->gl_ctx );
      free( ctx );
      if (ctx==SVGAMesa) {
         SVGAMesa = NULL;
      }
   }
#endif
}

/*
 * Make the specified VGA/Mesa context the current one.
 */
void SVGAMesaMakeCurrent( SVGAMesaContext ctx )
{
#ifndef DEV
   SVGAMesa = ctx;
   svgamesa_update_state( ctx->gl_ctx, ~0 );
   _mesa_make_current( ctx->gl_ctx, ctx->gl_buffer );

   if (ctx->width==0 || ctx->height==0) {
      ctx->width = vga_getxdim();
      ctx->height = vga_getydim();
   }
#endif
}

/*
 * Return a handle to the current VGA/Mesa context.
 */
SVGAMesaContext SVGAMesaGetCurrentContext( void )
{
   return SVGAMesa;
}

/*
 * Swap front/back buffers for current context if double buffered.
 */
void SVGAMesaSwapBuffers( void )
{
#if 000
   void * tmpptr;
#endif

   /* vga_waitretrace(); */
   copy_buffer(SVGABuffer.BackBuffer);

#ifndef DEV
   _mesa_notifySwapBuffers( SVGAMesa->gl_ctx );
   if (SVGAMesa->gl_vis->doubleBufferMode)
#endif /* DEV */
   {
#ifdef SVGA_DEBUG
      sprintf(cbuf,"SVGAMesaSwapBuffers : Swapping...");
      SVGAlog(cbuf);
#endif /* SVGA_DEBUG */
#if 000
      tmpptr=SVGABuffer.BackBuffer;
      SVGABuffer.BackBuffer=SVGABuffer.FrontBuffer;
      SVGABuffer.FrontBuffer=tmpptr;
#endif
#ifdef SVGA_DEBUG
      sprintf(cbuf,"SVGAMesaSwapBuffers : WriteBuffer : %p\n"
              "                      Readbuffer  : %p", \
              SVGABuffer.BackBuffer, SVGABuffer.FrontBuffer );
      SVGAlog(cbuf);
#endif /* SVGA_DEBUG */
   }
}

#else /*SVGA*/

/*
 * Need this to provide at least one external definition when SVGA is
 * not defined on the compiler command line.
 */
extern int gl_svga_dummy_function(void);
int gl_svga_dummy_function(void)
{
   return 0;
}

#endif  /*SVGA*/

