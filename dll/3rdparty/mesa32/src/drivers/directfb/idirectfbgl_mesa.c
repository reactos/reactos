/*
 * Copyright (C) 2004-2007 Claudio Ciccani <klan@directfb.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * 
 * Based on glfbdev.c, written by Brian Paul.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#include <directfb.h>
#include <directfb_version.h>

#include <directfbgl.h>

#include <direct/mem.h>
#include <direct/messages.h>
#include <direct/interface.h>

#undef CLAMP
#include "glheader.h"
#include "buffers.h"
#include "context.h"
#include "extensions.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "imports.h"
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


#define VERSION_CODE( M, m, r )  (((M) * 1000) + ((m) * 100) + ((r)))
#define DIRECTFB_VERSION_CODE    VERSION_CODE( DIRECTFB_MAJOR_VERSION, \
                                               DIRECTFB_MINOR_VERSION, \
                                               DIRECTFB_MICRO_VERSION )


static DFBResult
Probe( void *data );

static DFBResult
Construct( IDirectFBGL      *thiz,
           IDirectFBSurface *surface );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBGL, Mesa )

/*
 * private data struct of IDirectFBGL
 */
typedef struct {
     int                     ref;       /* reference counter */
     
     int                     locked;
     
     IDirectFBSurface       *surface;
     DFBSurfacePixelFormat   format;
     int                     width;
     int                     height;
     
     struct {
          GLubyte           *start;
          GLubyte           *end;
          int                pitch;
     } video;

     GLvisual                visual;
     GLframebuffer           framebuffer;
     GLcontext               context;
     struct gl_renderbuffer  render;
} IDirectFBGL_data;

/******************************************************************************/

static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int    global_ref  = 0;

static inline int directfbgl_init( void )
{
     pthread_mutexattr_t attr;
     int                 ret;
     
     if (global_ref++)
          return 0;

     pthread_mutexattr_init( &attr );
     pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
     ret = pthread_mutex_init( &global_lock, &attr );
     pthread_mutexattr_destroy( &attr );

     return ret;
}

static inline void directfbgl_finish( void )
{
     if (--global_ref == 0)
          pthread_mutex_destroy( &global_lock );
}

#define directfbgl_lock()    pthread_mutex_lock( &global_lock )
#define directfbgl_unlock()  pthread_mutex_unlock( &global_lock )

/******************************************************************************/

static bool  directfbgl_init_visual    ( GLvisual              *visual,
                                         DFBSurfacePixelFormat  format );
static bool  directfbgl_create_context ( GLcontext             *context,
                                         GLframebuffer         *framebuffer,
                                         GLvisual              *visual,
                                         DFBSurfacePixelFormat  format,
                                         IDirectFBGL_data      *data );
static void  directfbgl_destroy_context( GLcontext             *context,
                                         GLframebuffer         *framebuffer );

/******************************************************************************/


static void
IDirectFBGL_Mesa_Destruct( IDirectFBGL *thiz )
{
     IDirectFBGL_data *data = (IDirectFBGL_data*) thiz->priv;

     directfbgl_destroy_context( &data->context, &data->framebuffer );
     
     if (data->surface)
          data->surface->Release( data->surface );

     DIRECT_DEALLOCATE_INTERFACE( thiz );

     directfbgl_finish();
}

static DFBResult
IDirectFBGL_Mesa_AddRef( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBGL_Mesa_Release( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL )

     if (--data->ref == 0) 
          IDirectFBGL_Mesa_Destruct( thiz );

     return DFB_OK;
}

static DFBResult
IDirectFBGL_Mesa_Lock( IDirectFBGL *thiz )
{
     IDirectFBSurface *surface;
     int               width   = 0;
     int               height  = 0;
     DFBResult         ret;
     
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     if (data->locked) {
          data->locked++;
          return DFB_OK;
     }

     if (directfbgl_lock())
          return DFB_LOCKED;

     surface = data->surface;
     surface->GetSize( surface, &width, &height );
     
     ret = surface->Lock( surface, DSLF_READ | DSLF_WRITE, 
                          (void*)&data->video.start, &data->video.pitch );
     if (ret) {
          D_ERROR( "DirectFBGL/Mesa: couldn't lock surface.\n" );
          directfbgl_unlock();
          return ret;
     }
     data->video.end = data->video.start + (height-1) * data->video.pitch;

     data->render.Data = data->video.start;

     _mesa_make_current( &data->context, 
                         &data->framebuffer, &data->framebuffer );
     
     if (data->width != width || data->height != height) {
          _mesa_resize_framebuffer( &data->context, 
                                    &data->framebuffer, width, height );
          data->width  = width;
          data->height = height;                        
     }

     data->locked++;
     
     return DFB_OK;
}

static DFBResult
IDirectFBGL_Mesa_Unlock( IDirectFBGL *thiz )
{     
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     if (!data->locked)
          return DFB_OK;
          
     if (--data->locked == 0) {
          _mesa_make_current( NULL, NULL, NULL );
     
          data->surface->Unlock( data->surface );

          directfbgl_unlock();
     }
     
     return DFB_OK;
}

static DFBResult
IDirectFBGL_Mesa_GetAttributes( IDirectFBGL     *thiz,
                                DFBGLAttributes *attributes )
{
     DFBSurfaceCapabilities   caps;
     GLvisual                *visual;
     
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     if (!attributes)
          return DFB_INVARG;

     data->surface->GetCapabilities( data->surface, &caps );

     visual = &data->visual;
     
     attributes->buffer_size      = visual->rgbBits ? : visual->indexBits;
     attributes->depth_size       = visual->depthBits;
     attributes->stencil_size     = visual->stencilBits;
     attributes->aux_buffers      = visual->numAuxBuffers;
     attributes->red_size         = visual->redBits;
     attributes->green_size       = visual->greenBits;
     attributes->blue_size        = visual->blueBits;
     attributes->alpha_size       = visual->alphaBits;
     attributes->accum_red_size   = visual->accumRedBits;
     attributes->accum_green_size = visual->accumGreenBits;
     attributes->accum_blue_size  = visual->accumBlueBits;
     attributes->accum_alpha_size = visual->accumAlphaBits;
     attributes->double_buffer    = ((caps & DSCAPS_FLIPPING) != 0);
     attributes->stereo           = (visual->stereoMode != 0);

     return DFB_OK;
}

#if DIRECTFBGL_INTERFACE_VERSION >= 1
static DFBResult
IDirectFBGL_Mesa_GetProcAddress( IDirectFBGL  *thiz,
                                 const char   *name,
                                 void        **ret_address )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     if (!name)
          return DFB_INVARG;
          
     if (!ret_address)
          return DFB_INVARG;
          
     *ret_address = _glapi_get_proc_address( name );
          
     return (*ret_address) ? DFB_OK : DFB_UNSUPPORTED;
}
#endif


/* exported symbols */

static DFBResult
Probe( void *data )
{
     return DFB_OK;
}

static DFBResult
Construct( IDirectFBGL *thiz, IDirectFBSurface *surface )
{
     DFBResult ret;
     
     /* Initialize global resources. */
     if (directfbgl_init())
          return DFB_INIT;
     
     /* Allocate interface data. */
     DIRECT_ALLOCATE_INTERFACE_DATA( thiz, IDirectFBGL );
 
     /* Initialize interface data. */
     data->ref = 1;

     /* Duplicate destination surface. */
     ret = surface->GetSubSurface( surface, NULL, &data->surface );
     if (ret) {
          IDirectFBGL_Mesa_Destruct( thiz );
          return ret;
     }

     data->surface->GetPixelFormat( data->surface, &data->format );
     data->surface->GetSize( data->surface, &data->width, &data->height );

     /* Configure visual. */
     if (!directfbgl_init_visual( &data->visual, data->format )) {
          D_ERROR( "DirectFBGL/Mesa: failed to initialize visual.\n" );
          IDirectFBGL_Mesa_Destruct( thiz );
          return DFB_UNSUPPORTED;
     }
     
     /* Create context. */
     if (!directfbgl_create_context( &data->context, &data->framebuffer,
                                     &data->visual, data->format, data )) {
          D_ERROR( "DirectFBGL/Mesa: failed to create context.\n" );
          IDirectFBGL_Mesa_Destruct( thiz );
          return DFB_UNSUPPORTED;
     }

     /* Assign interface pointers. */
     thiz->AddRef         = IDirectFBGL_Mesa_AddRef;
     thiz->Release        = IDirectFBGL_Mesa_Release;
     thiz->Lock           = IDirectFBGL_Mesa_Lock;
     thiz->Unlock         = IDirectFBGL_Mesa_Unlock;
     thiz->GetAttributes  = IDirectFBGL_Mesa_GetAttributes;
#if DIRECTFBGL_INTERFACE_VERSION >= 1
     thiz->GetProcAddress = IDirectFBGL_Mesa_GetProcAddress;
#endif 

     return DFB_OK;
}


/***************************** Driver functions ******************************/

static const GLubyte*
dfbGetString( GLcontext *ctx, GLenum pname )
{
     return NULL;
}

static void
dfbUpdateState( GLcontext *ctx, GLuint new_state )
{
     _swrast_InvalidateState( ctx, new_state );
     _swsetup_InvalidateState( ctx, new_state );
     _vbo_InvalidateState( ctx, new_state );
     _tnl_InvalidateState( ctx, new_state );
}

static void
dfbGetBufferSize( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
     GLcontext        *ctx  = _mesa_get_current_context();
     IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;

     *width  = (GLuint) data->width;
     *height = (GLuint) data->height;
}

/**
 * We only implement this function as a mechanism to check if the
 * framebuffer size has changed (and update corresponding state).
 */
static void
dfbSetViewport( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h )
{
     /* Nothing to do (the surface can't be resized while it's locked). */
     return;
}

static void
dfbClear( GLcontext *ctx, GLbitfield mask )
{
     IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
 
#define BUFFER_BIT_MASK (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_FRONT_RIGHT | \
                         BUFFER_BIT_BACK_LEFT  | BUFFER_BIT_BACK_RIGHT  )
     if (mask & BUFFER_BIT_MASK  &&
         ctx->Color.ColorMask[0] &&
         ctx->Color.ColorMask[1] &&
         ctx->Color.ColorMask[2] &&
         ctx->Color.ColorMask[3])
     {
          DFBRegion clip;
          GLubyte   a, r, g, b;
          
          UNCLAMPED_FLOAT_TO_UBYTE( a, ctx->Color.ClearColor[ACOMP] );
          UNCLAMPED_FLOAT_TO_UBYTE( r, ctx->Color.ClearColor[RCOMP] );
          UNCLAMPED_FLOAT_TO_UBYTE( g, ctx->Color.ClearColor[GCOMP] );
          UNCLAMPED_FLOAT_TO_UBYTE( b, ctx->Color.ClearColor[BCOMP] );

          clip.x1 = ctx->DrawBuffer->_Xmin;
          clip.y1 = ctx->DrawBuffer->_Ymin;
          clip.x2 = ctx->DrawBuffer->_Xmax - 1;
          clip.y2 = ctx->DrawBuffer->_Ymax - 1;
          data->surface->SetClip( data->surface, &clip );
          
          data->surface->Unlock( data->surface );
          
          data->surface->Clear( data->surface, r, g, b, a );
          
          data->surface->Lock( data->surface, DSLF_READ | DSLF_WRITE,
                               (void*)&data->video.start, &data->video.pitch );
          data->video.end = data->video.start + (data->height-1) * data->video.pitch;
          data->render.Data = data->video.start;
          
          mask &= ~BUFFER_BIT_MASK;
     }
#undef BUFFER_BIT_MASK
     
     if (mask)
          _swrast_Clear( ctx, mask );
}


/************************ RenderBuffer functions *****************************/

static void
dfbDeleteRenderbuffer( struct gl_renderbuffer *render )
{
     return;
}

static GLboolean
dfbRenderbufferStorage( GLcontext *ctx, struct gl_renderbuffer *render,
                        GLenum internalFormat, GLuint width, GLuint height )
{
     return GL_TRUE;
}


/***************************** Span functions ********************************/

/* RGB332 */
#define NAME(PREFIX) PREFIX##_RGB332
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = data->video.end - (Y) * data->video.pitch + (X);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[RCOMP]) & 0xe0)     ) | \
          (((S[GCOMP]) & 0xe0) >> 3) | \
          (((S[BCOMP])       ) >> 6) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0xe0)     ); \
   D[GCOMP] = ((*P & 0x1c) << 3); \
   D[BCOMP] = ((*P & 0x03) << 6); \
   D[ACOMP] = 0xff

#include "swrast/s_spantemp.h"

/* ARGB4444 */
#define NAME(PREFIX) PREFIX##_ARGB4444
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (data->video.end - (Y) * data->video.pitch + (X) * 2);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL_RGB(P, X, Y, S) \
   *P = ( 0xf000                     | \
          (((S[RCOMP]) & 0xf0) << 4) | \
          (((S[GCOMP]) & 0xf0)     ) | \
          (((S[BCOMP]) & 0xf0) >> 4) )
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[ACOMP]) & 0xf0) << 8) | \
          (((S[RCOMP]) & 0xf0) << 4) | \
          (((S[GCOMP]) & 0xf0)     ) | \
          (((S[BCOMP]) & 0xf0) >> 4) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x0f00) >> 4); \
   D[GCOMP] = ((*P & 0x00f0)     ); \
   D[BCOMP] = ((*P & 0x000f) << 4); \
   D[ACOMP] = ((*P & 0xf000) >> 8)

#include "swrast/s_spantemp.h"

/* ARGB2554 */
#define NAME(PREFIX) PREFIX##_ARGB2554
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (data->video.end - (Y) * data->video.pitch + (X) * 2);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL_RGB(P, X, Y, S) \
   *P = ( 0xc000                     | \
          (((S[RCOMP]) & 0xf8) << 6) | \
          (((S[GCOMP]) & 0xf8) << 1) | \
          (((S[BCOMP]) & 0xf0) >> 4) )
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[ACOMP]) & 0xc0) << 8) | \
          (((S[RCOMP]) & 0xf8) << 6) | \
          (((S[GCOMP]) & 0xf8) << 1) | \
          (((S[BCOMP]) & 0xf0) >> 4) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x3e00) >>  9); \
   D[GCOMP] = ((*P & 0x01f0) >>  4); \
   D[BCOMP] = ((*P & 0x000f) <<  4); \
   D[ACOMP] = ((*P & 0xc000) >> 14)

#include "swrast/s_spantemp.h"
   
/* ARGB1555 */
#define NAME(PREFIX) PREFIX##_ARGB1555
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (data->video.end - (Y) * data->video.pitch + (X) * 2);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL_RGB(P, X, Y, S) \
   *P = ( 0x8000                      | \
          (((S[RCOMP]) & 0xf8) <<  7) | \
          (((S[GCOMP]) & 0xf8) <<  2) | \
          (((S[BCOMP])       ) >>  3) )
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[ACOMP]) & 0x80) << 16) | \
          (((S[RCOMP]) & 0xf8) <<  7) | \
          (((S[GCOMP]) & 0xf8) <<  2) | \
          (((S[BCOMP])       ) >>  3) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x7c00) >> 7); \
   D[GCOMP] = ((*P & 0x03e0) >> 2); \
   D[BCOMP] = ((*P & 0x001f) << 3); \
   D[ACOMP] = ((*P & 0x8000) ? 0xff : 0)

#include "swrast/s_spantemp.h"

/* RGB16 */
#define NAME(PREFIX) PREFIX##_RGB16
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (data->video.end - (Y) * data->video.pitch + (X) * 2);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[RCOMP]) & 0xf8) << 8) | \
          (((S[GCOMP]) & 0xfc) << 3) | \
          (((S[BCOMP])       ) >> 3) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0xf800) >> 8); \
   D[GCOMP] = ((*P & 0x07e0) >> 3); \
   D[BCOMP] = ((*P & 0x001f) << 3); \
   D[ACOMP] = 0xff

#include "swrast/s_spantemp.h"

/* RGB24 */
#define NAME(PREFIX) PREFIX##_RGB24
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = data->video.end - (Y) * data->video.pitch + (X) * 3;
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(P, X, Y, S) \
   P[0] = S[BCOMP];  P[1] = S[GCOMP];  P[2] = S[BCOMP]
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = P[2];  D[GCOMP] = P[1];  D[BCOMP] = P[0]; D[ACOMP] = 0xff

#include "swrast/s_spantemp.h"

/* RGB32 */
#define NAME(PREFIX) PREFIX##_RGB32
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLuint *P = (GLuint*) (data->video.end - (Y) * data->video.pitch + (X) * 4);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( ((S[RCOMP]) << 16) | \
          ((S[GCOMP]) <<  8) | \
          ((S[BCOMP])      ) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x00ff0000) >> 16); \
   D[GCOMP] = ((*P & 0x0000ff00) >>  8); \
   D[BCOMP] = ((*P & 0x000000ff)      ); \
   D[ACOMP] = 0xff

#include "swrast/s_spantemp.h"
   
/* ARGB */
#define NAME(PREFIX) PREFIX##_ARGB
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLuint *P = (GLuint*) (data->video.end - (Y) * data->video.pitch + (X) * 4);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL_RGB(P, X, Y, S) \
   *P = ( 0xff000000         | \
          ((S[RCOMP]) << 16) | \
          ((S[GCOMP]) <<  8) | \
          ((S[BCOMP])      ) )
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( ((S[ACOMP]) << 24) | \
          ((S[RCOMP]) << 16) | \
          ((S[GCOMP]) <<  8) | \
          ((S[BCOMP])      ) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x00ff0000) >> 16); \
   D[GCOMP] = ((*P & 0x0000ff00) >>  8); \
   D[BCOMP] = ((*P & 0x000000ff)      ); \
   D[ACOMP] = ((*P & 0xff000000) >> 24)

#include "swrast/s_spantemp.h"

/* AiRGB */
#define NAME(PREFIX) PREFIX##_AiRGB
#define FORMAT GL_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLuint *P = (GLuint*) (data->video.end - (Y) * data->video.pitch + (X) * 4);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL_RGB(P, X, Y, S) \
   *P = ( ((S[RCOMP]) << 16) | \
          ((S[GCOMP]) <<  8) | \
          ((S[BCOMP])      ) )
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[ACOMP]) ^ 0xff) << 24) | \
          (((S[RCOMP])       ) << 16) | \
          (((S[GCOMP])       ) <<  8) | \
          (((S[BCOMP])       )      ) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] =  ((*P & 0x00ff0000) >> 16); \
   D[GCOMP] =  ((*P & 0x0000ff00) >>  8); \
   D[BCOMP] =  ((*P & 0x000000ff)      ); \
   D[ACOMP] = (((*P & 0xff000000) >> 24) ^ 0xff)

#include "swrast/s_spantemp.h"


/*****************************************************************************/

static bool
directfbgl_init_visual( GLvisual              *visual,
                        DFBSurfacePixelFormat  format )
{
     GLboolean  rgbFlag        = GL_TRUE;
     GLboolean  dbFlag         = GL_FALSE;
     GLboolean  stereoFlag     = GL_FALSE;
     GLint      redBits        = 0;
     GLint      blueBits       = 0;
     GLint      greenBits      = 0;
     GLint      alphaBits      = 0;
     GLint      indexBits      = 0;
     GLint      depthBits      = 0;
     GLint      stencilBits    = 0;
     GLint      accumRedBits   = 0;
     GLint      accumGreenBits = 0;
     GLint      accumBlueBits  = 0;
     GLint      accumAlphaBits = 0;
     GLint      numSamples     = 0;

     /* FIXME: LUT8 support. */
     switch (format) {
          case DSPF_RGB332:
               redBits   = 3;
               greenBits = 3;
               blueBits  = 2;
               break;
          case DSPF_ARGB4444:
               redBits   = 4;
               greenBits = 4;
               blueBits  = 4;
               alphaBits = 4;
               break;
          case DSPF_ARGB2554:
               redBits   = 5;
               greenBits = 5;
               blueBits  = 4;
               alphaBits = 2;
               break;
          case DSPF_ARGB1555:
               redBits   = 5;
               greenBits = 5;
               blueBits  = 5;
               alphaBits = 1;
               break;
          case DSPF_RGB16:
               redBits   = 5;
               greenBits = 6;
               blueBits  = 5;
               break;
          case DSPF_ARGB:
          case DSPF_AiRGB:
               alphaBits = 8;
          case DSPF_RGB24:
          case DSPF_RGB32:
               redBits   = 8;
               greenBits = 8;
               blueBits  = 8;
               break;
          default:
               D_WARN( "unsupported pixelformat" );
               return false;
     }

     if (rgbFlag) {
          accumRedBits   = redBits;
          accumGreenBits = greenBits;
          accumBlueBits  = blueBits;
          accumAlphaBits = alphaBits;
          depthBits      = redBits + greenBits + blueBits;
          stencilBits    = alphaBits;
     } else
          depthBits      = 8;

     return _mesa_initialize_visual( visual,
                                     rgbFlag, dbFlag, stereoFlag,
                                     redBits, greenBits, blueBits, alphaBits,
                                     indexBits, depthBits, stencilBits,
                                     accumRedBits, accumGreenBits,
                                     accumBlueBits, accumAlphaBits,
                                     numSamples );
}

static bool
directfbgl_create_context( GLcontext             *context,
                           GLframebuffer         *framebuffer,
                           GLvisual              *visual,
                           DFBSurfacePixelFormat  format,
                           IDirectFBGL_data      *data )
{
     struct dd_function_table functions;
     
     _mesa_initialize_framebuffer( framebuffer, visual ); 
     
     _mesa_init_driver_functions( &functions );
     functions.GetString     = dfbGetString;
     functions.UpdateState   = dfbUpdateState;
     functions.GetBufferSize = dfbGetBufferSize;
     functions.Viewport      = dfbSetViewport;
     functions.Clear         = dfbClear;
     
     if (!_mesa_initialize_context( context, visual, NULL,
                                    &functions, (void*) data )) {
          D_DEBUG( "DirectFBGL/Mesa: _mesa_initialize_context() failed.\n" );
          _mesa_free_framebuffer_data( framebuffer );
          return false;
     }

     _swrast_CreateContext( context );
     _vbo_CreateContext( context );
     _tnl_CreateContext( context );
     _swsetup_CreateContext( context );
     _swsetup_Wakeup( context );

     _mesa_init_renderbuffer( &data->render, 0 );
     data->render.InternalFormat = GL_RGBA;
     data->render._BaseFormat    = GL_RGBA;
     data->render.DataType       = GL_UNSIGNED_BYTE;
     data->render.Data           = data->video.start;
     data->render.Delete         = dfbDeleteRenderbuffer;
     data->render.AllocStorage   = dfbRenderbufferStorage;
     
     switch (format) {
          case DSPF_RGB332:
               data->render.GetRow        = get_row_RGB332;
               data->render.GetValues     = get_values_RGB332;
               data->render.PutRow        = put_row_RGB332;
               data->render.PutRowRGB     = put_row_rgb_RGB332;
               data->render.PutMonoRow    = put_mono_row_RGB332;
               data->render.PutValues     = put_values_RGB332;
               data->render.PutMonoValues = put_mono_values_RGB332;
               break;
          case DSPF_ARGB4444: 
               data->render.GetRow        = get_row_ARGB4444;
               data->render.GetValues     = get_values_ARGB4444;
               data->render.PutRow        = put_row_ARGB4444;
               data->render.PutRowRGB     = put_row_rgb_ARGB4444;
               data->render.PutMonoRow    = put_mono_row_ARGB4444;
               data->render.PutValues     = put_values_ARGB4444;
               data->render.PutMonoValues = put_mono_values_ARGB4444;
               break;
          case DSPF_ARGB2554: 
               data->render.GetRow        = get_row_ARGB2554;
               data->render.GetValues     = get_values_ARGB2554;
               data->render.PutRow        = put_row_ARGB2554;
               data->render.PutRowRGB     = put_row_rgb_ARGB2554;
               data->render.PutMonoRow    = put_mono_row_ARGB2554;
               data->render.PutValues     = put_values_ARGB2554;
               data->render.PutMonoValues = put_mono_values_ARGB2554;
               break;
          case DSPF_ARGB1555:
               data->render.GetRow        = get_row_ARGB1555;
               data->render.GetValues     = get_values_ARGB1555;
               data->render.PutRow        = put_row_ARGB1555;
               data->render.PutRowRGB     = put_row_rgb_ARGB1555;
               data->render.PutMonoRow    = put_mono_row_ARGB1555;
               data->render.PutValues     = put_values_ARGB1555;
               data->render.PutMonoValues = put_mono_values_ARGB1555;
               break;
          case DSPF_RGB16:
               data->render.GetRow        = get_row_RGB16;
               data->render.GetValues     = get_values_RGB16;
               data->render.PutRow        = put_row_RGB16;
               data->render.PutRowRGB     = put_row_rgb_RGB16;
               data->render.PutMonoRow    = put_mono_row_RGB16;
               data->render.PutValues     = put_values_RGB16;
               data->render.PutMonoValues = put_mono_values_RGB16;
               break;
          case DSPF_RGB24:
               data->render.GetRow        = get_row_RGB24;
               data->render.GetValues     = get_values_RGB24;
               data->render.PutRow        = put_row_RGB24;
               data->render.PutRowRGB     = put_row_rgb_RGB24;
               data->render.PutMonoRow    = put_mono_row_RGB24;
               data->render.PutValues     = put_values_RGB24;
               data->render.PutMonoValues = put_mono_values_RGB24;
               break;
          case DSPF_RGB32:
               data->render.GetRow        = get_row_RGB32;
               data->render.GetValues     = get_values_RGB32;
               data->render.PutRow        = put_row_RGB32;
               data->render.PutRowRGB     = put_row_rgb_RGB32;
               data->render.PutMonoRow    = put_mono_row_RGB32;
               data->render.PutValues     = put_values_RGB32;
               data->render.PutMonoValues = put_mono_values_RGB32;
               break;
          case DSPF_ARGB:
               data->render.GetRow        = get_row_ARGB;
               data->render.GetValues     = get_values_ARGB;
               data->render.PutRow        = put_row_ARGB;
               data->render.PutRowRGB     = put_row_rgb_ARGB;
               data->render.PutMonoRow    = put_mono_row_ARGB;
               data->render.PutValues     = put_values_ARGB;
               data->render.PutMonoValues = put_mono_values_ARGB;
               break;
          case DSPF_AiRGB:
               data->render.GetRow        = get_row_AiRGB;
               data->render.GetValues     = get_values_AiRGB;
               data->render.PutRow        = put_row_AiRGB;
               data->render.PutRowRGB     = put_row_rgb_AiRGB;
               data->render.PutMonoRow    = put_mono_row_AiRGB;
               data->render.PutValues     = put_values_AiRGB;
               data->render.PutMonoValues = put_mono_values_AiRGB;
               break;
          default:
               D_BUG( "unexpected pixelformat" );
               return false;
     }

     _mesa_add_renderbuffer( framebuffer, BUFFER_FRONT_LEFT, &data->render );
     
     _mesa_add_soft_renderbuffers( framebuffer,
                                   GL_FALSE,
                                   visual->haveDepthBuffer,
                                   visual->haveStencilBuffer,
                                   visual->haveAccumBuffer,
                                   GL_FALSE,
                                   GL_FALSE );

     TNL_CONTEXT( context )->Driver.RunPipeline = _tnl_run_pipeline;

     _mesa_enable_sw_extensions( context );
     
     return true;
}

static void
directfbgl_destroy_context( GLcontext     *context,
                            GLframebuffer *framebuffer )
{
     _mesa_free_framebuffer_data( framebuffer );
     _mesa_notifyDestroy( context );
     _mesa_free_context_data( context );
}    
 
