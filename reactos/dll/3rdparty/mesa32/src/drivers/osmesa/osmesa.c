/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * Off-Screen Mesa rendering / Rendering into client memory space
 *
 * Note on thread safety:  this driver is thread safe.  All
 * functions are reentrant.  The notion of current context is
 * managed by the core _mesa_make_current() and _mesa_get_current_context()
 * functions.  Those functions are thread-safe.
 */


#include "glheader.h"
#include "GL/osmesa.h"
#include "context.h"
#include "extensions.h"
#include "framebuffer.h"
#include "imports.h"
#include "mtypes.h"
#include "renderbuffer.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"
#include "vbo/vbo.h"



/**
 * OSMesa rendering context, derived from core Mesa GLcontext.
 */
struct osmesa_context
{
   GLcontext mesa;		/*< Base class - this must be first */
   GLvisual *gl_visual;		/*< Describes the buffers */
   struct gl_renderbuffer *rb;  /*< The user's colorbuffer */
   GLframebuffer *gl_buffer;	/*< The framebuffer, containing user's rb */
   GLenum format;		/*< User-specified context format */
   GLint userRowLength;		/*< user-specified number of pixels per row */
   GLint rInd, gInd, bInd, aInd;/*< index offsets for RGBA formats */
   GLvoid *rowaddr[MAX_HEIGHT];	/*< address of first pixel in each image row */
   GLboolean yup;		/*< TRUE  -> Y increases upward */
				/*< FALSE -> Y increases downward */
};


static INLINE OSMesaContext
OSMESA_CONTEXT(GLcontext *ctx)
{
   /* Just cast, since we're using structure containment */
   return (OSMesaContext) ctx;
}


/**********************************************************************/
/*** Private Device Driver Functions                                ***/
/**********************************************************************/


static const GLubyte *
get_string( GLcontext *ctx, GLenum name )
{
   (void) ctx;
   switch (name) {
      case GL_RENDERER:
#if CHAN_BITS == 32
         return (const GLubyte *) "Mesa OffScreen32";
#elif CHAN_BITS == 16
         return (const GLubyte *) "Mesa OffScreen16";
#else
         return (const GLubyte *) "Mesa OffScreen";
#endif
      default:
         return NULL;
   }
}


static void
osmesa_update_state( GLcontext *ctx, GLuint new_state )
{
   /* easy - just propogate */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
}



/**********************************************************************/
/*****        Read/write spans/arrays of pixels                   *****/
/**********************************************************************/

/* 8-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = 255
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 16-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = 65535
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 32-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = MAX2((VALUE[RCOMP]), 0.0F); \
   DST[1] = MAX2((VALUE[GCOMP]), 0.0F); \
   DST[2] = MAX2((VALUE[BCOMP]), 0.0F); \
   DST[3] = CLAMP((VALUE[ACOMP]), 0.0F, 1.0F)
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = MAX2((VALUE[RCOMP]), 0.0F); \
   DST[1] = MAX2((VALUE[GCOMP]), 0.0F); \
   DST[2] = MAX2((VALUE[BCOMP]), 0.0F); \
   DST[3] = 1.0F
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"


/* 8-bit BGRA */
#define NAME(PREFIX) PREFIX##_BGRA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = 255
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 16-bit BGRA */
#define NAME(PREFIX) PREFIX##_BGRA16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = 65535
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* 32-bit BGRA */
#define NAME(PREFIX) PREFIX##_BGRA32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP];  \
   DST[3] = 1.0F
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"


/* 8-bit ARGB */
#define NAME(PREFIX) PREFIX##_ARGB8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = 255
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[1];  \
   DST[GCOMP] = SRC[2];  \
   DST[BCOMP] = SRC[3];  \
   DST[ACOMP] = SRC[0]
#include "swrast/s_spantemp.h"

/* 16-bit ARGB */
#define NAME(PREFIX) PREFIX##_ARGB16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = 65535
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[1];  \
   DST[GCOMP] = SRC[2];  \
   DST[BCOMP] = SRC[3];  \
   DST[ACOMP] = SRC[0]
#include "swrast/s_spantemp.h"

/* 32-bit ARGB */
#define NAME(PREFIX) PREFIX##_ARGB32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[1] = VALUE[RCOMP];  \
   DST[2] = VALUE[GCOMP];  \
   DST[3] = VALUE[BCOMP];  \
   DST[0] = 1.0F
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[1];  \
   DST[GCOMP] = SRC[2];  \
   DST[BCOMP] = SRC[3];  \
   DST[ACOMP] = SRC[0]
#include "swrast/s_spantemp.h"


/* 8-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = 255
#include "swrast/s_spantemp.h"

/* 16-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = 65535U
#include "swrast/s_spantemp.h"

/* 32-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = 1.0F
#include "swrast/s_spantemp.h"


/* 8-bit BGR */
#define NAME(PREFIX) PREFIX##_BGR8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = 255
#include "swrast/s_spantemp.h"

/* 16-bit BGR */
#define NAME(PREFIX) PREFIX##_BGR16
#define RB_TYPE GLushort
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = 65535
#include "swrast/s_spantemp.h"

/* 32-bit BGR */
#define NAME(PREFIX) PREFIX##_BGR32
#define RB_TYPE GLfloat
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLfloat *P = (GLfloat *) osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = 1.0F
#include "swrast/s_spantemp.h"


/* 16-bit 5/6/5 RGB */
#define NAME(PREFIX) PREFIX##_RGB_565
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = ( (((VALUE[RCOMP]) & 0xf8) << 8) | (((VALUE[GCOMP]) & 0xfc) << 3) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((*SRC) >> 8) & 0xf8) | (((*SRC) >> 11) & 0x7) ); \
   DST[GCOMP] = ( (((*SRC) >> 3) & 0xfc) | (((*SRC) >>  5) & 0x3) ); \
   DST[BCOMP] = ( (((*SRC) << 3) & 0xf8) | (((*SRC)      ) & 0x7) ); \
   DST[ACOMP] = CHAN_MAX
#include "swrast/s_spantemp.h"


/* color index */
#define NAME(PREFIX) PREFIX##_CI
#define CI_MODE
#define RB_TYPE GLubyte
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *) osmesa->rowaddr[Y] + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]
#include "swrast/s_spantemp.h"




/**
 * Macros for optimized line/triangle rendering.
 * Only for 8-bit channel, RGBA, BGRA, ARGB formats.
 */

#define PACK_RGBA(DST, R, G, B, A)	\
do {					\
   (DST)[osmesa->rInd] = R;		\
   (DST)[osmesa->gInd] = G;		\
   (DST)[osmesa->bInd] = B;		\
   (DST)[osmesa->aInd] = A;		\
} while (0)

#define PIXELADDR4(X,Y)  ((GLchan *) osmesa->rowaddr[Y] + 4 * (X))


/**
 * Draw a flat-shaded, RGB line into an osmesa buffer.
 */
#define NAME flat_rgba_line
#define CLIP_HACK 1
#define SETUP_CODE						\
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);		\
   const GLchan *color = vert1->color;

#define PLOT(X, Y)						\
do {								\
   GLchan *p = PIXELADDR4(X, Y);				\
   PACK_RGBA(p, color[0], color[1], color[2], color[3]);	\
} while (0)

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif



/**
 * Draw a flat-shaded, Z-less, RGB line into an osmesa buffer.
 */
#define NAME flat_rgba_z_line
#define CLIP_HACK 1
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE					\
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);	\
   const GLchan *color = vert1->color;

#define PLOT(X, Y)					\
do {							\
   if (Z < *zPtr) {					\
      GLchan *p = PIXELADDR4(X, Y);			\
      PACK_RGBA(p, color[RCOMP], color[GCOMP],		\
                   color[BCOMP], color[ACOMP]);		\
      *zPtr = Z;					\
   }							\
} while (0)

#ifdef WIN32
#include "..\swrast\s_linetemp.h"
#else
#include "swrast/s_linetemp.h"
#endif



/**
 * Analyze context state to see if we can provide a fast line drawing
 * function.  Otherwise, return NULL.
 */
static swrast_line_func
osmesa_choose_line_function( GLcontext *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (osmesa->rb->DataType != GL_UNSIGNED_BYTE)
      return NULL;

   if (ctx->RenderMode != GL_RENDER)      return NULL;
   if (ctx->Line.SmoothFlag)              return NULL;
   if (ctx->Texture._EnabledUnits)        return NULL;
   if (ctx->Light.ShadeModel != GL_FLAT)  return NULL;
   if (ctx->Line.Width != 1.0F)           return NULL;
   if (ctx->Line.StippleFlag)             return NULL;
   if (ctx->Line.SmoothFlag)              return NULL;
   if (osmesa->format != OSMESA_RGBA &&
       osmesa->format != OSMESA_BGRA &&
       osmesa->format != OSMESA_ARGB)     return NULL;

   if (swrast->_RasterMask==DEPTH_BIT
       && ctx->Depth.Func==GL_LESS
       && ctx->Depth.Mask==GL_TRUE
       && ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS) {
      return (swrast_line_func) flat_rgba_z_line;
   }

   if (swrast->_RasterMask == 0) {
      return (swrast_line_func) flat_rgba_line;
   }

   return (swrast_line_func) NULL;
}


/**********************************************************************/
/*****                 Optimized triangle rendering               *****/
/**********************************************************************/


/*
 * Smooth-shaded, z-less triangle, RGBA color.
 */
#define NAME smooth_rgba_z_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define SETUP_CODE \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define RENDER_SPAN( span ) {					\
   GLuint i;							\
   GLchan *img = PIXELADDR4(span.x, span.y); 			\
   for (i = 0; i < span.end; i++, img += 4) {			\
      const GLuint z = FixedToDepth(span.z);			\
      if (z < zRow[i]) {					\
         PACK_RGBA(img, FixedToChan(span.red),			\
            FixedToChan(span.green), FixedToChan(span.blue),	\
            FixedToChan(span.alpha));				\
         zRow[i] = z;						\
      }								\
      span.red += span.redStep;					\
      span.green += span.greenStep;				\
      span.blue += span.blueStep;				\
      span.alpha += span.alphaStep;				\
      span.z += span.zStep;					\
   }                                                            \
}
#ifdef WIN32
#include "..\swrast\s_tritemp.h"
#else
#include "swrast/s_tritemp.h"
#endif



/*
 * Flat-shaded, z-less triangle, RGBA color.
 */
#define NAME flat_rgba_z_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE						\
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);		\
   GLuint pixel;						\
   PACK_RGBA((GLchan *) &pixel, v2->color[0], v2->color[1],	\
                                v2->color[2], v2->color[3]);

#define RENDER_SPAN( span ) {				\
   GLuint i;						\
   GLuint *img = (GLuint *) PIXELADDR4(span.x, span.y);	\
   for (i = 0; i < span.end; i++) {			\
      const GLuint z = FixedToDepth(span.z);		\
      if (z < zRow[i]) {				\
         img[i] = pixel;				\
         zRow[i] = z;					\
      }							\
      span.z += span.zStep;				\
   }                                                    \
}
#ifdef WIN32
#include "..\swrast\s_tritemp.h"
#else
#include "swrast/s_tritemp.h"
#endif



/**
 * Return pointer to an optimized triangle function if possible.
 */
static swrast_tri_func
osmesa_choose_triangle_function( GLcontext *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (osmesa->rb->DataType != GL_UNSIGNED_BYTE)
      return (swrast_tri_func) NULL;

   if (ctx->RenderMode != GL_RENDER)    return (swrast_tri_func) NULL;
   if (ctx->Polygon.SmoothFlag)         return (swrast_tri_func) NULL;
   if (ctx->Polygon.StippleFlag)        return (swrast_tri_func) NULL;
   if (ctx->Texture._EnabledUnits)      return (swrast_tri_func) NULL;
   if (osmesa->format != OSMESA_RGBA &&
       osmesa->format != OSMESA_BGRA &&
       osmesa->format != OSMESA_ARGB)   return (swrast_tri_func) NULL;
   if (ctx->Polygon.CullFlag && 
       ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
                                        return (swrast_tri_func) NULL;

   if (swrast->_RasterMask == DEPTH_BIT &&
       ctx->Depth.Func == GL_LESS &&
       ctx->Depth.Mask == GL_TRUE &&
       ctx->Visual.depthBits == DEFAULT_SOFTWARE_DEPTH_BITS) {
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         return (swrast_tri_func) smooth_rgba_z_triangle;
      }
      else {
         return (swrast_tri_func) flat_rgba_z_triangle;
      }
   }
   return (swrast_tri_func) NULL;
}



/* Override for the swrast triangle-selection function.  Try to use one
 * of our internal triangle functions, otherwise fall back to the
 * standard swrast functions.
 */
static void
osmesa_choose_triangle( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   swrast->Triangle = osmesa_choose_triangle_function( ctx );
   if (!swrast->Triangle)
      _swrast_choose_triangle( ctx );
}

static void
osmesa_choose_line( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   swrast->Line = osmesa_choose_line_function( ctx );
   if (!swrast->Line)
      _swrast_choose_line( ctx );
}



/**
 * Recompute the values of the context's rowaddr array.
 */
static void
compute_row_addresses( OSMesaContext osmesa )
{
   GLint bytesPerPixel, bytesPerRow, i;
   GLubyte *origin = (GLubyte *) osmesa->rb->Data;
   GLint bpc; /* bytes per channel */
   GLint rowlength; /* in pixels */
   GLint height = osmesa->rb->Height;

   if (osmesa->userRowLength)
      rowlength = osmesa->userRowLength;
   else
      rowlength = osmesa->rb->Width;

   if (osmesa->rb->DataType == GL_UNSIGNED_BYTE)
      bpc = 1;
   else if (osmesa->rb->DataType == GL_UNSIGNED_SHORT)
      bpc = 2;
   else if (osmesa->rb->DataType == GL_FLOAT)
      bpc = 4;
   else {
      _mesa_problem(&osmesa->mesa,
                    "Unexpected datatype in osmesa::compute_row_addresses");
      return;
   }

   if (osmesa->format == OSMESA_COLOR_INDEX) {
      /* CI mode */
      bytesPerPixel = 1 * sizeof(GLubyte);
   }
   else if ((osmesa->format == OSMESA_RGB) || (osmesa->format == OSMESA_BGR)) {
      /* RGB mode */
      bytesPerPixel = 3 * bpc;
   }
   else if (osmesa->format == OSMESA_RGB_565) {
      /* 5/6/5 RGB pixel in 16 bits */
      bytesPerPixel = 2;
   }
   else {
      /* RGBA mode */
      bytesPerPixel = 4 * bpc;
   }

   bytesPerRow = rowlength * bytesPerPixel;

   if (osmesa->yup) {
      /* Y=0 is bottom line of window */
      for (i = 0; i < height; i++) {
         osmesa->rowaddr[i] = (GLvoid *) ((GLubyte *) origin + i * bytesPerRow);
      }
   }
   else {
      /* Y=0 is top line of window */
      for (i = 0; i < height; i++) {
         GLint j = height - i - 1;
         osmesa->rowaddr[i] = (GLvoid *) ((GLubyte *) origin + j * bytesPerRow);
      }
   }
}



/**
 * Don't use _mesa_delete_renderbuffer since we can't free rb->Data.
 */
static void
osmesa_delete_renderbuffer(struct gl_renderbuffer *rb)
{
   _mesa_free(rb);
}


/**
 * Allocate renderbuffer storage.  We don't actually allocate any storage
 * since we're using a user-provided buffer.
 * Just set up all the gl_renderbuffer methods.
 */
static GLboolean
osmesa_renderbuffer_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
                            GLenum internalFormat, GLuint width, GLuint height)
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   GLint bpc; /* bits per channel */

   if (rb->DataType == GL_UNSIGNED_BYTE)
      bpc = 8;
   else if (rb->DataType == GL_UNSIGNED_SHORT)
      bpc = 16;
   else
      bpc = 32;

   rb->RedBits =
   rb->GreenBits =
   rb->BlueBits =
   rb->AlphaBits = bpc;

   /* Note: we can ignoring internalFormat for "window-system" renderbuffers */
   (void) internalFormat;

   if (osmesa->format == OSMESA_RGBA) {
      if (rb->DataType == GL_UNSIGNED_BYTE) {
         rb->GetRow = get_row_RGBA8;
         rb->GetValues = get_values_RGBA8;
         rb->PutRow = put_row_RGBA8;
         rb->PutRowRGB = put_row_rgb_RGBA8;
         rb->PutMonoRow = put_mono_row_RGBA8;
         rb->PutValues = put_values_RGBA8;
         rb->PutMonoValues = put_mono_values_RGBA8;
      }
      else if (rb->DataType == GL_UNSIGNED_SHORT) {
         rb->GetRow = get_row_RGBA16;
         rb->GetValues = get_values_RGBA16;
         rb->PutRow = put_row_RGBA16;
         rb->PutRowRGB = put_row_rgb_RGBA16;
         rb->PutMonoRow = put_mono_row_RGBA16;
         rb->PutValues = put_values_RGBA16;
         rb->PutMonoValues = put_mono_values_RGBA16;
      }
      else {
         rb->GetRow = get_row_RGBA32;
         rb->GetValues = get_values_RGBA32;
         rb->PutRow = put_row_RGBA32;
         rb->PutRowRGB = put_row_rgb_RGBA32;
         rb->PutMonoRow = put_mono_row_RGBA32;
         rb->PutValues = put_values_RGBA32;
         rb->PutMonoValues = put_mono_values_RGBA32;
      }
      rb->RedBits = rb->GreenBits = rb->BlueBits = rb->AlphaBits = bpc;
   }
   else if (osmesa->format == OSMESA_BGRA) {
      if (rb->DataType == GL_UNSIGNED_BYTE) {
         rb->GetRow = get_row_BGRA8;
         rb->GetValues = get_values_BGRA8;
         rb->PutRow = put_row_BGRA8;
         rb->PutRowRGB = put_row_rgb_BGRA8;
         rb->PutMonoRow = put_mono_row_BGRA8;
         rb->PutValues = put_values_BGRA8;
         rb->PutMonoValues = put_mono_values_BGRA8;
      }
      else if (rb->DataType == GL_UNSIGNED_SHORT) {
         rb->GetRow = get_row_BGRA16;
         rb->GetValues = get_values_BGRA16;
         rb->PutRow = put_row_BGRA16;
         rb->PutRowRGB = put_row_rgb_BGRA16;
         rb->PutMonoRow = put_mono_row_BGRA16;
         rb->PutValues = put_values_BGRA16;
         rb->PutMonoValues = put_mono_values_BGRA16;
      }
      else {
         rb->GetRow = get_row_BGRA32;
         rb->GetValues = get_values_BGRA32;
         rb->PutRow = put_row_BGRA32;
         rb->PutRowRGB = put_row_rgb_BGRA32;
         rb->PutMonoRow = put_mono_row_BGRA32;
         rb->PutValues = put_values_BGRA32;
         rb->PutMonoValues = put_mono_values_BGRA32;
      }
      rb->RedBits = rb->GreenBits = rb->BlueBits = rb->AlphaBits = bpc;
   }
   else if (osmesa->format == OSMESA_ARGB) {
      if (rb->DataType == GL_UNSIGNED_BYTE) {
         rb->GetRow = get_row_ARGB8;
         rb->GetValues = get_values_ARGB8;
         rb->PutRow = put_row_ARGB8;
         rb->PutRowRGB = put_row_rgb_ARGB8;
         rb->PutMonoRow = put_mono_row_ARGB8;
         rb->PutValues = put_values_ARGB8;
         rb->PutMonoValues = put_mono_values_ARGB8;
      }
      else if (rb->DataType == GL_UNSIGNED_SHORT) {
         rb->GetRow = get_row_ARGB16;
         rb->GetValues = get_values_ARGB16;
         rb->PutRow = put_row_ARGB16;
         rb->PutRowRGB = put_row_rgb_ARGB16;
         rb->PutMonoRow = put_mono_row_ARGB16;
         rb->PutValues = put_values_ARGB16;
         rb->PutMonoValues = put_mono_values_ARGB16;
      }
      else {
         rb->GetRow = get_row_ARGB32;
         rb->GetValues = get_values_ARGB32;
         rb->PutRow = put_row_ARGB32;
         rb->PutRowRGB = put_row_rgb_ARGB32;
         rb->PutMonoRow = put_mono_row_ARGB32;
         rb->PutValues = put_values_ARGB32;
         rb->PutMonoValues = put_mono_values_ARGB32;
      }
      rb->RedBits = rb->GreenBits = rb->BlueBits = rb->AlphaBits = bpc;
   }
   else if (osmesa->format == OSMESA_RGB) {
      if (rb->DataType == GL_UNSIGNED_BYTE) {
         rb->GetRow = get_row_RGB8;
         rb->GetValues = get_values_RGB8;
         rb->PutRow = put_row_RGB8;
         rb->PutRowRGB = put_row_rgb_RGB8;
         rb->PutMonoRow = put_mono_row_RGB8;
         rb->PutValues = put_values_RGB8;
         rb->PutMonoValues = put_mono_values_RGB8;
      }
      else if (rb->DataType == GL_UNSIGNED_SHORT) {
         rb->GetRow = get_row_RGB16;
         rb->GetValues = get_values_RGB16;
         rb->PutRow = put_row_RGB16;
         rb->PutRowRGB = put_row_rgb_RGB16;
         rb->PutMonoRow = put_mono_row_RGB16;
         rb->PutValues = put_values_RGB16;
         rb->PutMonoValues = put_mono_values_RGB16;
      }
      else {
         rb->GetRow = get_row_RGB32;
         rb->GetValues = get_values_RGB32;
         rb->PutRow = put_row_RGB32;
         rb->PutRowRGB = put_row_rgb_RGB32;
         rb->PutMonoRow = put_mono_row_RGB32;
         rb->PutValues = put_values_RGB32;
         rb->PutMonoValues = put_mono_values_RGB32;
      }
      rb->RedBits = rb->GreenBits = rb->BlueBits = bpc;
   }
   else if (osmesa->format == OSMESA_BGR) {
      if (rb->DataType == GL_UNSIGNED_BYTE) {
         rb->GetRow = get_row_BGR8;
         rb->GetValues = get_values_BGR8;
         rb->PutRow = put_row_BGR8;
         rb->PutRowRGB = put_row_rgb_BGR8;
         rb->PutMonoRow = put_mono_row_BGR8;
         rb->PutValues = put_values_BGR8;
         rb->PutMonoValues = put_mono_values_BGR8;
      }
      else if (rb->DataType == GL_UNSIGNED_SHORT) {
         rb->GetRow = get_row_BGR16;
         rb->GetValues = get_values_BGR16;
         rb->PutRow = put_row_BGR16;
         rb->PutRowRGB = put_row_rgb_BGR16;
         rb->PutMonoRow = put_mono_row_BGR16;
         rb->PutValues = put_values_BGR16;
         rb->PutMonoValues = put_mono_values_BGR16;
      }
      else {
         rb->GetRow = get_row_BGR32;
         rb->GetValues = get_values_BGR32;
         rb->PutRow = put_row_BGR32;
         rb->PutRowRGB = put_row_rgb_BGR32;
         rb->PutMonoRow = put_mono_row_BGR32;
         rb->PutValues = put_values_BGR32;
         rb->PutMonoValues = put_mono_values_BGR32;
      }
      rb->RedBits = rb->GreenBits = rb->BlueBits = bpc;
   }
   else if (osmesa->format == OSMESA_RGB_565) {
      ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
      rb->GetRow = get_row_RGB_565;
      rb->GetValues = get_values_RGB_565;
      rb->PutRow = put_row_RGB_565;
      rb->PutRowRGB = put_row_rgb_RGB_565;
      rb->PutMonoRow = put_mono_row_RGB_565;
      rb->PutValues = put_values_RGB_565;
      rb->PutMonoValues = put_mono_values_RGB_565;
      rb->RedBits = 5;
      rb->GreenBits = 6;
      rb->BlueBits = 5;
   }
   else if (osmesa->format == OSMESA_COLOR_INDEX) {
      rb->GetRow = get_row_CI;
      rb->GetValues = get_values_CI;
      rb->PutRow = put_row_CI;
      rb->PutMonoRow = put_mono_row_CI;
      rb->PutValues = put_values_CI;
      rb->PutMonoValues = put_mono_values_CI;
      rb->IndexBits = 8;
   }
   else {
      _mesa_problem(ctx, "bad pixel format in osmesa renderbuffer_storage");
   }

   rb->Width = width;
   rb->Height = height;

   compute_row_addresses( osmesa );

   return GL_TRUE;
}


/**
 * Allocate a new renderbuffer to describe the user-provided color buffer.
 */
static struct gl_renderbuffer *
new_osmesa_renderbuffer(GLcontext *ctx, GLenum format, GLenum type)
{
   const GLuint name = 0;
   struct gl_renderbuffer *rb = _mesa_new_renderbuffer(ctx, name);
   if (rb) {
      rb->RefCount = 1;
      rb->Delete = osmesa_delete_renderbuffer;
      rb->AllocStorage = osmesa_renderbuffer_storage;

      if (format == OSMESA_COLOR_INDEX) {
         rb->InternalFormat = GL_COLOR_INDEX;
         rb->_ActualFormat = GL_COLOR_INDEX8_EXT;
         rb->_BaseFormat = GL_COLOR_INDEX;
         rb->DataType = GL_UNSIGNED_BYTE;
      }
      else {
         rb->InternalFormat = GL_RGBA;
         rb->_ActualFormat = GL_RGBA;
         rb->_BaseFormat = GL_RGBA;
         rb->DataType = type;
      }
   }
   return rb;
}


/**********************************************************************/
/*****                    Public Functions                        *****/
/**********************************************************************/


/**
 * Create an Off-Screen Mesa rendering context.  The only attribute needed is
 * an RGBA vs Color-Index mode flag.
 *
 * Input:  format - either GL_RGBA or GL_COLOR_INDEX
 *         sharelist - specifies another OSMesaContext with which to share
 *                     display lists.  NULL indicates no sharing.
 * Return:  an OSMesaContext or 0 if error
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContext( GLenum format, OSMesaContext sharelist )
{
   const GLint accumBits = (format == OSMESA_COLOR_INDEX) ? 0 : 16;
   return OSMesaCreateContextExt(format, DEFAULT_SOFTWARE_DEPTH_BITS,
                                 8, accumBits, sharelist);
}



/**
 * New in Mesa 3.5
 *
 * Create context and specify size of ancillary buffers.
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits,
                        GLint accumBits, OSMesaContext sharelist )
{
   OSMesaContext osmesa;
   struct dd_function_table functions;
   GLint rind, gind, bind, aind;
   GLint indexBits = 0, redBits = 0, greenBits = 0, blueBits = 0, alphaBits =0;
   GLboolean rgbmode;
   GLenum type = CHAN_TYPE;

   rind = gind = bind = aind = 0;
   if (format==OSMESA_COLOR_INDEX) {
      indexBits = 8;
      rgbmode = GL_FALSE;
   }
   else if (format==OSMESA_RGBA) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      rind = 0;
      gind = 1;
      bind = 2;
      aind = 3;
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_BGRA) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      bind = 0;
      gind = 1;
      rind = 2;
      aind = 3;
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_ARGB) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      aind = 0;
      rind = 1;
      gind = 2;
      bind = 3;
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_RGB) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = 0;
      rind = 0;
      gind = 1;
      bind = 2;
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_BGR) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = 0;
      rind = 2;
      gind = 1;
      bind = 0;
      rgbmode = GL_TRUE;
   }
#if CHAN_TYPE == GL_UNSIGNED_BYTE
   else if (format==OSMESA_RGB_565) {
      indexBits = 0;
      redBits = 5;
      greenBits = 6;
      blueBits = 5;
      alphaBits = 0;
      rind = 0; /* not used */
      gind = 0;
      bind = 0;
      rgbmode = GL_TRUE;
   }
#endif
   else {
      return NULL;
   }

   osmesa = (OSMesaContext) CALLOC_STRUCT(osmesa_context);
   if (osmesa) {
      osmesa->gl_visual = _mesa_create_visual( rgbmode,
                                               GL_FALSE,    /* double buffer */
                                               GL_FALSE,    /* stereo */
                                               redBits,
                                               greenBits,
                                               blueBits,
                                               alphaBits,
                                               indexBits,
                                               depthBits,
                                               stencilBits,
                                               accumBits,
                                               accumBits,
                                               accumBits,
                                               alphaBits ? accumBits : 0,
                                               1            /* num samples */
                                               );
      if (!osmesa->gl_visual) {
         _mesa_free(osmesa);
         return NULL;
      }

      /* Initialize device driver function table */
      _mesa_init_driver_functions(&functions);
      /* override with our functions */
      functions.GetString = get_string;
      functions.UpdateState = osmesa_update_state;
      functions.GetBufferSize = NULL;

      if (!_mesa_initialize_context(&osmesa->mesa,
                                    osmesa->gl_visual,
                                    sharelist ? &sharelist->mesa
                                              : (GLcontext *) NULL,
                                    &functions, (void *) osmesa)) {
         _mesa_destroy_visual( osmesa->gl_visual );
         _mesa_free(osmesa);
         return NULL;
      }

      _mesa_enable_sw_extensions(&(osmesa->mesa));
      _mesa_enable_1_3_extensions(&(osmesa->mesa));
      _mesa_enable_1_4_extensions(&(osmesa->mesa));
      _mesa_enable_1_5_extensions(&(osmesa->mesa));

      osmesa->gl_buffer = _mesa_create_framebuffer(osmesa->gl_visual);
      if (!osmesa->gl_buffer) {
         _mesa_destroy_visual( osmesa->gl_visual );
         _mesa_free_context_data( &osmesa->mesa );
         _mesa_free(osmesa);
         return NULL;
      }

      /* create front color buffer in user-provided memory (no back buffer) */
      osmesa->rb = new_osmesa_renderbuffer(&osmesa->mesa, format, type);
      _mesa_add_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT, osmesa->rb);
      assert(osmesa->rb->RefCount == 2);
                        
      _mesa_add_soft_renderbuffers(osmesa->gl_buffer,
                                   GL_FALSE, /* color */
                                   osmesa->gl_visual->haveDepthBuffer,
                                   osmesa->gl_visual->haveStencilBuffer,
                                   osmesa->gl_visual->haveAccumBuffer,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */ );

      osmesa->format = format;
      osmesa->userRowLength = 0;
      osmesa->yup = GL_TRUE;
      osmesa->rInd = rind;
      osmesa->gInd = gind;
      osmesa->bInd = bind;
      osmesa->aInd = aind;

      /* Initialize the software rasterizer and helper modules. */
      {
	 GLcontext *ctx = &osmesa->mesa;
         SWcontext *swrast;
         TNLcontext *tnl;

	 if (!_swrast_CreateContext( ctx ) ||
             !_vbo_CreateContext( ctx ) ||
             !_tnl_CreateContext( ctx ) ||
             !_swsetup_CreateContext( ctx )) {
            _mesa_destroy_visual(osmesa->gl_visual);
            _mesa_free_context_data(ctx);
            _mesa_free(osmesa);
            return NULL;
         }
	
	 _swsetup_Wakeup( ctx );

         /* use default TCL pipeline */
         tnl = TNL_CONTEXT(ctx);
         tnl->Driver.RunPipeline = _tnl_run_pipeline;

         /* Extend the software rasterizer with our optimized line and triangle
          * drawing functions.
          */
         swrast = SWRAST_CONTEXT( ctx );
         swrast->choose_line = osmesa_choose_line;
         swrast->choose_triangle = osmesa_choose_triangle;
      }
   }
   return osmesa;
}


/**
 * Destroy an Off-Screen Mesa rendering context.
 *
 * \param osmesa  the context to destroy
 */
GLAPI void GLAPIENTRY
OSMesaDestroyContext( OSMesaContext osmesa )
{
   if (osmesa) {
      if (osmesa->rb)
         _mesa_reference_renderbuffer(&osmesa->rb, NULL);

      _swsetup_DestroyContext( &osmesa->mesa );
      _tnl_DestroyContext( &osmesa->mesa );
      _vbo_DestroyContext( &osmesa->mesa );
      _swrast_DestroyContext( &osmesa->mesa );

      _mesa_destroy_visual( osmesa->gl_visual );
      _mesa_unreference_framebuffer( &osmesa->gl_buffer );

      _mesa_free_context_data( &osmesa->mesa );
      _mesa_free( osmesa );
   }
}


/**
 * Bind an OSMesaContext to an image buffer.  The image buffer is just a
 * block of memory which the client provides.  Its size must be at least
 * as large as width*height*sizeof(type).  Its address should be a multiple
 * of 4 if using RGBA mode.
 *
 * Image data is stored in the order of glDrawPixels:  row-major order
 * with the lower-left image pixel stored in the first array position
 * (ie. bottom-to-top).
 *
 * If the context's viewport hasn't been initialized yet, it will now be
 * initialized to (0,0,width,height).
 *
 * Input:  osmesa - the rendering context
 *         buffer - the image buffer memory
 *         type - data type for pixel components
 *            Normally, only GL_UNSIGNED_BYTE and GL_UNSIGNED_SHORT_5_6_5
 *            are supported.  But if Mesa's been compiled with CHAN_BITS==16
 *            then type may be GL_UNSIGNED_SHORT or GL_UNSIGNED_BYTE.  And if
 *            Mesa's been build with CHAN_BITS==32 then type may be GL_FLOAT,
 *            GL_UNSIGNED_SHORT or GL_UNSIGNED_BYTE.
 *         width, height - size of image buffer in pixels, at least 1
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid osmesa,
 *          invalid buffer address, invalid type, width<1, height<1,
 *          width>internal limit or height>internal limit.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaMakeCurrent( OSMesaContext osmesa, void *buffer, GLenum type,
                   GLsizei width, GLsizei height )
{
   if (!osmesa || !buffer ||
       width < 1 || height < 1 ||
       width > MAX_WIDTH || height > MAX_HEIGHT) {
      return GL_FALSE;
   }

   if (osmesa->format == OSMESA_RGB_565 && type != GL_UNSIGNED_SHORT_5_6_5) {
      return GL_FALSE;
   }

#if 0
   if (!(type == GL_UNSIGNED_BYTE ||
         (type == GL_UNSIGNED_SHORT && CHAN_BITS >= 16) ||
         (type == GL_FLOAT && CHAN_BITS == 32))) {
      /* i.e. is sizeof(type) * 8 > CHAN_BITS? */
      return GL_FALSE;
   }
#endif

   osmesa_update_state( &osmesa->mesa, 0 );

   /* Call this periodically to detect when the user has begun using
    * GL rendering from multiple threads.
    */
   _glapi_check_multithread();

   /* Set renderbuffer fields.  Set width/height = 0 to force 
    * osmesa_renderbuffer_storage() being called by _mesa_resize_framebuffer()
    */
   osmesa->rb->Data = buffer;
   osmesa->rb->DataType = type;
   osmesa->rb->Width = osmesa->rb->Height = 0;

   /* Set the framebuffer's size.  This causes the
    * osmesa_renderbuffer_storage() function to get called.
    */
   _mesa_resize_framebuffer(&osmesa->mesa, osmesa->gl_buffer, width, height);
   osmesa->gl_buffer->Initialized = GL_TRUE; /* XXX TEMPORARY? */

   _mesa_make_current( &osmesa->mesa, osmesa->gl_buffer, osmesa->gl_buffer );

   /* Remove renderbuffer attachment, then re-add.  This installs the
    * renderbuffer adaptor/wrapper if needed (for bpp conversion).
    */
   _mesa_remove_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT);
   _mesa_add_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT, osmesa->rb);


   /* this updates the visual's red/green/blue/alphaBits fields */
   _mesa_update_framebuffer_visual(osmesa->gl_buffer);

   /* update the framebuffer size */
   _mesa_resize_framebuffer(&osmesa->mesa, osmesa->gl_buffer, width, height);

   return GL_TRUE;
}



GLAPI OSMesaContext GLAPIENTRY
OSMesaGetCurrentContext( void )
{
   GLcontext *ctx = _mesa_get_current_context();
   if (ctx)
      return (OSMesaContext) ctx;
   else
      return NULL;
}



GLAPI void GLAPIENTRY
OSMesaPixelStore( GLint pname, GLint value )
{
   OSMesaContext osmesa = OSMesaGetCurrentContext();

   switch (pname) {
      case OSMESA_ROW_LENGTH:
         if (value<0) {
            _mesa_error( &osmesa->mesa, GL_INVALID_VALUE,
                      "OSMesaPixelStore(value)" );
            return;
         }
         osmesa->userRowLength = value;
         break;
      case OSMESA_Y_UP:
         osmesa->yup = value ? GL_TRUE : GL_FALSE;
         break;
      default:
         _mesa_error( &osmesa->mesa, GL_INVALID_ENUM, "OSMesaPixelStore(pname)" );
         return;
   }

   compute_row_addresses( osmesa );
}


GLAPI void GLAPIENTRY
OSMesaGetIntegerv( GLint pname, GLint *value )
{
   OSMesaContext osmesa = OSMesaGetCurrentContext();

   switch (pname) {
      case OSMESA_WIDTH:
         if (osmesa->gl_buffer)
            *value = osmesa->gl_buffer->Width;
         else
            *value = 0;
         return;
      case OSMESA_HEIGHT:
         if (osmesa->gl_buffer)
            *value = osmesa->gl_buffer->Height;
         else
            *value = 0;
         return;
      case OSMESA_FORMAT:
         *value = osmesa->format;
         return;
      case OSMESA_TYPE:
         /* current color buffer's data type */
         if (osmesa->rb) {
            *value = osmesa->rb->DataType;
         }
         else {
            *value = 0;
         }
         return;
      case OSMESA_ROW_LENGTH:
         *value = osmesa->userRowLength;
         return;
      case OSMESA_Y_UP:
         *value = osmesa->yup;
         return;
      case OSMESA_MAX_WIDTH:
         *value = MAX_WIDTH;
         return;
      case OSMESA_MAX_HEIGHT:
         *value = MAX_HEIGHT;
         return;
      default:
         _mesa_error(&osmesa->mesa, GL_INVALID_ENUM, "OSMesaGetIntergerv(pname)");
         return;
   }
}


/**
 * Return the depth buffer associated with an OSMesa context.
 * Input:  c - the OSMesa context
 * Output:  width, height - size of buffer in pixels
 *          bytesPerValue - bytes per depth value (2 or 4)
 *          buffer - pointer to depth buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height,
                      GLint *bytesPerValue, void **buffer )
{
   struct gl_renderbuffer *rb = NULL;

   if (c->gl_buffer)
      rb = c->gl_buffer->Attachment[BUFFER_DEPTH].Renderbuffer;

   if (!rb || !rb->Data) {
      *width = 0;
      *height = 0;
      *bytesPerValue = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = rb->Width;
      *height = rb->Height;
      if (c->gl_visual->depthBits <= 16)
         *bytesPerValue = sizeof(GLushort);
      else
         *bytesPerValue = sizeof(GLuint);
      *buffer = rb->Data;
      return GL_TRUE;
   }
}


/**
 * Return the color buffer associated with an OSMesa context.
 * Input:  c - the OSMesa context
 * Output:  width, height - size of buffer in pixels
 *          format - the pixel format (OSMESA_FORMAT)
 *          buffer - pointer to color buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaGetColorBuffer( OSMesaContext osmesa, GLint *width,
                      GLint *height, GLint *format, void **buffer )
{
   if (osmesa->rb && osmesa->rb->Data) {
      *width = osmesa->rb->Width;
      *height = osmesa->rb->Height;
      *format = osmesa->format;
      *buffer = osmesa->rb->Data;
      return GL_TRUE;
   }
   else {
      *width = 0;
      *height = 0;
      *format = 0;
      *buffer = 0;
      return GL_FALSE;
   }
}


struct name_function
{
   const char *Name;
   OSMESAproc Function;
};

static struct name_function functions[] = {
   { "OSMesaCreateContext", (OSMESAproc) OSMesaCreateContext },
   { "OSMesaCreateContextExt", (OSMESAproc) OSMesaCreateContextExt },
   { "OSMesaDestroyContext", (OSMESAproc) OSMesaDestroyContext },
   { "OSMesaMakeCurrent", (OSMESAproc) OSMesaMakeCurrent },
   { "OSMesaGetCurrentContext", (OSMESAproc) OSMesaGetCurrentContext },
   { "OSMesaPixelsStore", (OSMESAproc) OSMesaPixelStore },
   { "OSMesaGetIntegerv", (OSMESAproc) OSMesaGetIntegerv },
   { "OSMesaGetDepthBuffer", (OSMESAproc) OSMesaGetDepthBuffer },
   { "OSMesaGetColorBuffer", (OSMESAproc) OSMesaGetColorBuffer },
   { "OSMesaGetProcAddress", (OSMESAproc) OSMesaGetProcAddress },
   { "OSMesaColorClamp", (OSMESAproc) OSMesaColorClamp },
   { NULL, NULL }
};


GLAPI OSMESAproc GLAPIENTRY
OSMesaGetProcAddress( const char *funcName )
{
   int i;
   for (i = 0; functions[i].Name; i++) {
      if (_mesa_strcmp(functions[i].Name, funcName) == 0)
         return functions[i].Function;
   }
   return _glapi_get_proc_address(funcName);
}


GLAPI void GLAPIENTRY
OSMesaColorClamp(GLboolean enable)
{
   OSMesaContext osmesa = OSMesaGetCurrentContext();

   if (enable == GL_TRUE) {
      osmesa->mesa.Color.ClampFragmentColor = GL_TRUE;
   }
   else {
      osmesa->mesa.Color.ClampFragmentColor = GL_FIXED_ONLY_ARB;
   }
}


