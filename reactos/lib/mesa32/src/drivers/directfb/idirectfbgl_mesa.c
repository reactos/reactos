/*
 * Copyright (C) 2004-2005 Claudio Ciccani <klan@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * 
 * Based on glfbdev.c, written by Brian Paul.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <directfb.h>

#include <direct/messages.h>
#include <direct/interface.h>
#include <direct/mem.h>

#ifdef CLAMP
# undef CLAMP
#endif 

#include "GL/directfbgl.h"
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
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"


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
     
     bool                    locked;
     
     IDirectFBSurface       *surface;
     DFBSurfacePixelFormat   format;
     int                     width;
     int                     height;
     
     struct {
          __u8              *start;
          __u8              *end;
          int                pitch;
     } video;

     GLvisual                visual;
     GLframebuffer           framebuffer;
     GLcontext               context;
     struct gl_renderbuffer  render;
} IDirectFBGL_data;


static bool  dfb_mesa_setup_visual   ( GLvisual              *visual,
                                       DFBSurfacePixelFormat  format );
static bool  dfb_mesa_create_context ( GLcontext             *context,
                                       GLframebuffer         *framebuffer,
                                       GLvisual              *visual,
                                       DFBSurfacePixelFormat  format,
                                       IDirectFBGL_data      *data );
static void  dfb_mesa_destroy_context( GLcontext             *context,
                                       GLframebuffer         *framebuffer );


static void
IDirectFBGL_Destruct( IDirectFBGL *thiz )
{
     IDirectFBGL_data *data = (IDirectFBGL_data*) thiz->priv;

     dfb_mesa_destroy_context( &data->context, &data->framebuffer );
     
     data->surface->Release( data->surface );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DFBResult
IDirectFBGL_AddRef( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     data->ref++;

     return DFB_OK;
}

static DFBResult
IDirectFBGL_Release( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL )

     if (--data->ref == 0) {
          IDirectFBGL_Destruct( thiz );
     }

     return DFB_OK;
}

static DFBResult
IDirectFBGL_Lock( IDirectFBGL *thiz )
{
     IDirectFBSurface *surface;
     int               width   = 0;
     int               height  = 0;
     DFBResult         err;
     
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     if (data->locked)
          return DFB_LOCKED;

     surface = data->surface;
     surface->GetSize( surface, &width, &height );
     
     err = surface->Lock( surface, DSLF_READ | DSLF_WRITE, 
                          (void**) &data->video.start, &data->video.pitch );
     if (err != DFB_OK) {
          D_ERROR( "DirectFBGL/Mesa: couldn't lock surface.\n" );
          return err;
     }
     data->video.end = data->video.start + (height-1) * data->video.pitch;

     data->render.Data = data->video.start;
     
     if (data->width != width || data->height != height) {
          data->width  = width;
          data->height = height;
          _mesa_ResizeBuffersMESA();
     }
     
     data->locked = true;
     
     return DFB_OK;
}

static DFBResult
IDirectFBGL_Unlock( IDirectFBGL *thiz )
{     
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     if (!data->locked)
          return DFB_OK;

     data->surface->Unlock( data->surface );
     data->video.start = NULL;
     data->video.end   = NULL;
    
     data->locked = false;

     return DFB_OK;
}

static DFBResult
IDirectFBGL_GetAttributes( IDirectFBGL     *thiz,
                           DFBGLAttributes *attributes )
{
     GLvisual *visual;
     
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL );

     if (!attributes)
          return DFB_INVARG;

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
     attributes->double_buffer    = (visual->doubleBufferMode != 0);
     attributes->stereo           = (visual->stereoMode != 0);

     return DFB_OK;
}


/* exported symbols */

static DFBResult
Probe( void *data )
{
     return DFB_OK;
}

static DFBResult
Construct( IDirectFBGL      *thiz,
           IDirectFBSurface *surface )
{ 
     /* Allocate interface data. */
     DIRECT_ALLOCATE_INTERFACE_DATA( thiz, IDirectFBGL );

     /* Initialize interface data. */
     data->ref     = 1;
     data->surface = surface;

     surface->AddRef( surface );
     surface->GetPixelFormat( surface, &data->format );
     surface->GetSize( surface, &data->width, &data->height );

     /* Configure visual. */
     if (!dfb_mesa_setup_visual( &data->visual, data->format )) {
          D_ERROR( "DirectFBGL/Mesa: failed to initialize visual.\n" );
          surface->Release( surface );
          return DFB_UNSUPPORTED;
     }
     
     /* Create context. */
     if (!dfb_mesa_create_context( &data->context, &data->framebuffer,
                                   &data->visual, data->format, data )) {
          D_ERROR( "DirectFBGL/Mesa: failed to create context.\n" );
          surface->Release( surface );
          return DFB_UNSUPPORTED;
     }

     /* Assign interface pointers. */
     thiz->AddRef        = IDirectFBGL_AddRef;
     thiz->Release       = IDirectFBGL_Release;
     thiz->Lock          = IDirectFBGL_Lock;
     thiz->Unlock        = IDirectFBGL_Unlock;
     thiz->GetAttributes = IDirectFBGL_GetAttributes;

     return DFB_OK;
}


/* internal functions */

static const GLubyte*
get_string( GLcontext *ctx, GLenum pname )
{
     switch (pname) {
          case GL_VENDOR:
               return "Claudio Ciccani";
          case GL_VERSION:
               return "1.0";
          default:
               return NULL;
     }
}

static void
update_state( GLcontext *ctx, GLuint new_state )
{
     _swrast_InvalidateState( ctx, new_state );
     _swsetup_InvalidateState( ctx, new_state );
     _ac_InvalidateState( ctx, new_state );
     _tnl_InvalidateState( ctx, new_state );
}

static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
     GLcontext        *ctx  = _mesa_get_current_context();
     IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;

     *width  = (GLuint) data->width;
     *height = (GLuint) data->height;
}

static void
set_viewport( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h )
{
     _mesa_ResizeBuffersMESA();
}

/* required but not used */
static void
set_buffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit )
{
     return;
}

static void
delete_renderbuffer( struct gl_renderbuffer *render )
{
     return;
}

static GLboolean
renderbuffer_storage( GLcontext *ctx, struct gl_renderbuffer *render,
                      GLenum internalFormat, GLuint width, GLuint height )
{
     return GL_TRUE;
}


/* RGB332 */
#define NAME(PREFIX) PREFIX##_RGB332
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = data->video.end - (Y) * data->video.pitch + (X);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[RCOMP]) & 0xe0)) | (((S[GCOMP]) & 0xe0) >> 3) | ((S[BCOMP]) >> 6) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0xe0)     ); \
   D[GCOMP] = ((*P & 0x1c) << 3); \
   D[BCOMP] = ((*P & 0x03) << 6); \
   D[ACOMP] = 0xff

#include "swrast/s_spantemp.h"

/* ARGB1555 */
#define NAME(PREFIX) PREFIX##_ARGB1555
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (data->video.end - (Y) * data->video.pitch + (X) * 2);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[RCOMP]) & 0xf8) << 7) | (((S[GCOMP]) & 0xf8) << 2) | ((S[BCOMP]) >> 3) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x7c00) >> 7); \
   D[GCOMP] = ((*P & 0x03e0) >> 2); \
   D[BCOMP] = ((*P & 0x001f) << 3); \
   D[ACOMP] = ((*P & 0x8000) ? 0xff : 0)

#include "swrast/s_spantemp.h"

/* RGB16 */
#define NAME(PREFIX) PREFIX##_RGB16
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) (data->video.end - (Y) * data->video.pitch + (X) * 2);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( (((S[RCOMP]) & 0xf8) << 8) | (((S[GCOMP]) & 0xfc) << 3) | ((S[BCOMP]) >> 3) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0xf800) >> 8); \
   D[GCOMP] = ((*P & 0x07e0) >> 3); \
   D[BCOMP] = ((*P & 0x001f) << 3); \
   D[ACOMP] = 0xff

#include "swrast/s_spantemp.h"

/* RGB24 */
#define NAME(PREFIX) PREFIX##_RGB24
#define FORMAT GL_RGBA8
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
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLuint *P = (GLuint*) (data->video.end - (Y) * data->video.pitch + (X) * 4);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( ((S[RCOMP]) << 16) | ((S[GCOMP]) << 8) | (S[BCOMP]) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x00ff0000) >> 16); \
   D[GCOMP] = ((*P & 0x0000ff00) >>  8); \
   D[BCOMP] = ((*P & 0x000000ff)      ); \
   D[ACOMP] = 0xff

#include "swrast/s_spantemp.h"
   
/* ARGB */
#define NAME(PREFIX) PREFIX##_ARGB
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLuint *P = (GLuint*) (data->video.end - (Y) * data->video.pitch + (X) * 4);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL_RGB(P, X, Y, S) \
   *P = ( 0xff000000  | ((S[RCOMP]) << 16) | ((S[GCOMP]) << 8) | (S[BCOMP]) )
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( ((S[ACOMP]) << 24) | ((S[RCOMP]) << 16) | ((S[GCOMP]) << 8) | (S[BCOMP]) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] = ((*P & 0x00ff0000) >> 16); \
   D[GCOMP] = ((*P & 0x0000ff00) >>  8); \
   D[BCOMP] = ((*P & 0x000000ff)      ); \
   D[ACOMP] = ((*P & 0xff000000) >> 24)

#include "swrast/s_spantemp.h"

/* AiRGB */
#define NAME(PREFIX) PREFIX##_AiRGB
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   IDirectFBGL_data *data = (IDirectFBGL_data*) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLuint *P = (GLuint*) (data->video.end - (Y) * data->video.pitch + (X) * 4);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL_RGB(P, X, Y, S) \
   *P = ( ((S[RCOMP]) << 16) | ((S[GCOMP]) << 8) | (S[BCOMP]) )
#define STORE_PIXEL(P, X, Y, S) \
   *P = ( ((0xff - (S[ACOMP])) << 24) | ((S[RCOMP]) << 16) | ((S[GCOMP]) << 8) | (S[BCOMP]) )
#define FETCH_PIXEL(D, P) \
   D[RCOMP] =         ((*P & 0x00ff0000) >> 16); \
   D[GCOMP] =         ((*P & 0x0000ff00) >>  8); \
   D[BCOMP] =         ((*P & 0x000000ff)      ); \
   D[ACOMP] = (0xff - ((*P & 0xff000000) >> 24))

#include "swrast/s_spantemp.h"


static bool
dfb_mesa_setup_visual( GLvisual              *visual,
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
dfb_mesa_create_context( GLcontext             *context,
                         GLframebuffer         *framebuffer,
                         GLvisual              *visual,
                         DFBSurfacePixelFormat  format,
                         IDirectFBGL_data      *data )
{
     struct dd_function_table     functions;
     struct swrast_device_driver *swdd;
     
     _mesa_initialize_framebuffer( framebuffer, visual ); 
     
     _mesa_init_driver_functions( &functions );
     functions.GetString     = get_string;
     functions.UpdateState   = update_state;
     functions.GetBufferSize = get_buffer_size;
     functions.Viewport      = set_viewport;
     
     if (!_mesa_initialize_context( context, visual, NULL,
                                    &functions, (void*) data )) {
          D_DEBUG( "DirectFBGL/Mesa: _mesa_initialize_context() failed.\n" );
          _mesa_free_framebuffer_data( framebuffer );
          return false;
     }

     _swrast_CreateContext( context );
     _ac_CreateContext( context );
     _tnl_CreateContext( context );
     _swsetup_CreateContext( context );
     _swsetup_Wakeup( context );
     
     swdd = _swrast_GetDeviceDriverReference( context );
     swdd->SetBuffer = set_buffer;

     _mesa_init_renderbuffer( &data->render, 0 );
     data->render.InternalFormat = GL_RGBA;
     data->render._BaseFormat    = GL_RGBA;
     data->render.DataType       = GL_UNSIGNED_BYTE;
     data->render.Data           = data->video.start;
     data->render.Delete         = delete_renderbuffer;
     data->render.AllocStorage   = renderbuffer_storage;
     
     switch (format) {
          case DSPF_RGB332:
               data->render.GetRow        = get_row_RGB332;
               data->render.GetValues     = get_values_RGB332;
               data->render.PutRow        = put_row_RGB332;
               data->render.PutMonoRow    = put_mono_row_RGB332;
               data->render.PutValues     = put_values_RGB332;
               data->render.PutMonoValues = put_mono_values_RGB332;
               break;
          case DSPF_ARGB1555:
               data->render.GetRow        = get_row_ARGB1555;
               data->render.GetValues     = get_values_ARGB1555;
               data->render.PutRow        = put_row_ARGB1555;
               data->render.PutMonoRow    = put_mono_row_ARGB1555;
               data->render.PutValues     = put_values_ARGB1555;
               data->render.PutMonoValues = put_mono_values_ARGB1555;
               break;
          case DSPF_RGB16:
               data->render.GetRow        = get_row_RGB16;
               data->render.GetValues     = get_values_RGB16;
               data->render.PutRow        = put_row_RGB16;
               data->render.PutMonoRow    = put_mono_row_RGB16;
               data->render.PutValues     = put_values_RGB16;
               data->render.PutMonoValues = put_mono_values_RGB16;
               break;
          case DSPF_RGB24:
               data->render.GetRow        = get_row_RGB24;
               data->render.GetValues     = get_values_RGB24;
               data->render.PutRow        = put_row_RGB24;
               data->render.PutMonoRow    = put_mono_row_RGB24;
               data->render.PutValues     = put_values_RGB24;
               data->render.PutMonoValues = put_mono_values_RGB24;
               break;
          case DSPF_RGB32:
               data->render.GetRow        = get_row_RGB32;
               data->render.GetValues     = get_values_RGB32;
               data->render.PutRow        = put_row_RGB32;
               data->render.PutMonoRow    = put_mono_row_RGB32;
               data->render.PutValues     = put_values_RGB32;
               data->render.PutMonoValues = put_mono_values_RGB32;
               break;
          case DSPF_ARGB:
               data->render.GetRow        = get_row_ARGB;
               data->render.GetValues     = get_values_ARGB;
               data->render.PutRow        = put_row_ARGB;
               data->render.PutMonoRow    = put_mono_row_ARGB;
               data->render.PutValues     = put_values_ARGB;
               data->render.PutMonoValues = put_mono_values_ARGB;
               break;
          case DSPF_AiRGB:
               data->render.GetRow        = get_row_AiRGB;
               data->render.GetValues     = get_values_AiRGB;
               data->render.PutRow        = put_row_AiRGB;
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

     _mesa_make_current( context, framebuffer, framebuffer );
     
     return true;
}

static void
dfb_mesa_destroy_context( GLcontext     *context,
                          GLframebuffer *framebuffer )
{
     _mesa_make_current( NULL, NULL, NULL );
     _mesa_free_framebuffer_data( framebuffer );
     _mesa_notifyDestroy( context );
     _mesa_free_context_data( context );
}    
 
