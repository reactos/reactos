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
#include "fbobject.h"
#include "imports.h"
#include "mtypes.h"
#include "renderbuffer.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"



/*
 * This is the OS/Mesa context struct.
 * Notice how it includes a GLcontext.  By doing this we're mimicking
 * C++ inheritance/derivation.
 * Later, we can cast a GLcontext pointer into an OSMesaContext pointer
 * or vice versa.
 */
struct osmesa_context {
   GLcontext mesa;		/* The core GL/Mesa context */
   GLvisual *gl_visual;		/* Describes the buffers */
   GLframebuffer *gl_buffer;	/* Depth, stencil, accum, etc buffers */
   GLenum format;		/* either GL_RGBA or GL_COLOR_INDEX */
   void *buffer;		/* the image buffer */
   GLint width, height;		/* size of image buffer */
   GLint rowlength;		/* number of pixels per row */
   GLint userRowLength;		/* user-specified number of pixels per row */
   GLint rshift, gshift;	/* bit shifts for RGBA formats */
   GLint bshift, ashift;
   GLint rInd, gInd, bInd, aInd;/* index offsets for RGBA formats */
   GLchan *rowaddr[MAX_HEIGHT];	/* address of first pixel in each image row */
   GLboolean yup;		/* TRUE  -> Y increases upward */
				/* FALSE -> Y increases downward */
};


/* Just cast, since we're using structure containment */
#define OSMESA_CONTEXT(ctx)  ((OSMesaContext) (ctx->DriverCtx))



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
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}


static void
set_buffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit )
{
   /* separate read buffer not supported */
   ASSERT(buffer == ctx->DrawBuffer);
   ASSERT(bufferBit == BUFFER_BIT_FRONT_LEFT);
}


/*
 * Just return the current buffer size.
 * There's no window to track the size of.
 */
static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   /* don't use GET_CURRENT_CONTEXT(ctx) here - it's a problem on Windows */
   GLcontext *ctx = (GLcontext *) _glapi_get_context();
   (void) buffer;
   if (ctx) {
      OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
      *width = osmesa->width;
      *height = osmesa->height;
   }
}


/**********************************************************************/
/*****        Read/write spans/arrays of pixels                   *****/
/**********************************************************************/

/* RGBA */
#define NAME(PREFIX) PREFIX##_RGBA
#define FORMAT GL_RGBA
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#if CHAN_TYPE == GL_FLOAT
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = MAX2((VALUE[RCOMP]), 0.0F); \
   DST[1] = MAX2((VALUE[GCOMP]), 0.0F); \
   DST[2] = MAX2((VALUE[BCOMP]), 0.0F); \
   DST[3] = CLAMP((VALUE[ACOMP]), 0.0F, CHAN_MAXF)
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = MAX2((VALUE[RCOMP]), 0.0F); \
   DST[1] = MAX2((VALUE[GCOMP]), 0.0F); \
   DST[2] = MAX2((VALUE[BCOMP]), 0.0F); \
   DST[3] = CHAN_MAXF
#else
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP];  \
   DST[3] = CHAN_MAX
#endif
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* BGRA */
#define NAME(PREFIX) PREFIX##_BGRA
#define FORMAT GL_RGBA
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
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
   DST[3] = CHAN_MAX
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = SRC[3]
#include "swrast/s_spantemp.h"

/* ARGB */
#define NAME(PREFIX) PREFIX##_ARGB
#define FORMAT GL_RGBA
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
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
   DST[0] = CHAN_MAX
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[1];  \
   DST[GCOMP] = SRC[2];  \
   DST[BCOMP] = SRC[3];  \
   DST[ACOMP] = SRC[0]
#include "swrast/s_spantemp.h"

/* RGB */
#define NAME(PREFIX) PREFIX##_RGB
#define FORMAT GL_RGBA
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[2] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[0];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[2];  \
   DST[ACOMP] = CHAN_MAX
#include "swrast/s_spantemp.h"

/* BGR */
#define NAME(PREFIX) PREFIX##_BGR
#define FORMAT GL_RGBA
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[2] = VALUE[RCOMP];  \
   DST[1] = VALUE[GCOMP];  \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2];  \
   DST[GCOMP] = SRC[1];  \
   DST[BCOMP] = SRC[0];  \
   DST[ACOMP] = CHAN_MAX
#include "swrast/s_spantemp.h"

/* 16-bit BGR */
#if CHAN_TYPE == GL_UNSIGNED_BYTE
#define NAME(PREFIX) PREFIX##_RGB_565
#define FORMAT GL_RGBA
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
#endif /* CHAN_TYPE == GL_UNSIGNED_BYTE */

/* color index */
#define NAME(PREFIX) PREFIX##_CI
#define FORMAT GL_COLOR_INDEX8_EXT
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = osmesa->rowaddr[Y] + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]
#include "swrast/s_spantemp.h"




/**********************************************************************/
/*****                   Optimized line rendering                 *****/
/**********************************************************************/


#if CHAN_TYPE == GL_FLOAT
#define PACK_RGBA(DST, R, G, B, A)	\
do {					\
   (DST)[0] = MAX2( R, 0.0F );		\
   (DST)[1] = MAX2( G, 0.0F );		\
   (DST)[2] = MAX2( B, 0.0F );		\
   (DST)[3] = CLAMP(A, 0.0F, CHAN_MAXF);\
} while (0)
#else
#define PACK_RGBA(DST, R, G, B, A)	\
do {					\
   (DST)[osmesa->rInd] = R;		\
   (DST)[osmesa->gInd] = G;		\
   (DST)[osmesa->bInd] = B;		\
   (DST)[osmesa->aInd] = A;		\
} while (0)
#endif

#define PACK_RGB(DST, R, G, B)  \
do {				\
   (DST)[0] = R;		\
   (DST)[1] = G;		\
   (DST)[2] = B;		\
} while (0)

#define PACK_BGR(DST, R, G, B)  \
do {				\
   (DST)[0] = B;		\
   (DST)[1] = G;		\
   (DST)[2] = R;		\
} while (0)

#define PACK_RGB_565(DST, R, G, B)					\
do {									\
   (DST) = (((int) (R) << 8) & 0xf800) | (((int) (G) << 3) & 0x7e0) | ((int) (B) >> 3);\
} while (0)

#define UNPACK_RED(P)      ( (P)[osmesa->rInd] )
#define UNPACK_GREEN(P)    ( (P)[osmesa->gInd] )
#define UNPACK_BLUE(P)     ( (P)[osmesa->bInd] )
#define UNPACK_ALPHA(P)    ( (P)[osmesa->aInd] )

#define PIXELADDR1(X,Y)  (osmesa->rowaddr[Y] + (X))
#define PIXELADDR2(X,Y)  (osmesa->rowaddr[Y] + 2 * (X))
#define PIXELADDR3(X,Y)  (osmesa->rowaddr[Y] + 3 * (X))
#define PIXELADDR4(X,Y)  (osmesa->rowaddr[Y] + 4 * (X))


/*
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



/*
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



/*
 * Analyze context state to see if we can provide a fast line drawing
 * function, like those in lines.c.  Otherwise, return NULL.
 */
static swrast_line_func
osmesa_choose_line_function( GLcontext *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (CHAN_BITS != 8)                    return NULL;
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
#define RENDER_SPAN( span )					\
   GLuint i;							\
   GLchan *img = PIXELADDR4(span.x, span.y); 			\
   for (i = 0; i < span.end; i++, img += 4) {			\
      const GLdepth z = FixedToDepth(span.z);			\
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

#define RENDER_SPAN( span )				\
   GLuint i;						\
   GLuint *img = (GLuint *) PIXELADDR4(span.x, span.y);	\
   for (i = 0; i < span.end; i++) {			\
      const GLdepth z = FixedToDepth(span.z);		\
      if (z < zRow[i]) {				\
         img[i] = pixel;				\
         zRow[i] = z;					\
      }							\
      span.z += span.zStep;				\
   }
#ifdef WIN32
#include "..\swrast\s_tritemp.h"
#else
#include "swrast/s_tritemp.h"
#endif



/*
 * Return pointer to an accelerated triangle function if possible.
 */
static swrast_tri_func
osmesa_choose_triangle_function( GLcontext *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (CHAN_BITS != 8)                  return (swrast_tri_func) NULL;
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


#define OSMESA_NEW_LINE   (_NEW_LINE | \
                           _NEW_TEXTURE | \
                           _NEW_LIGHT | \
                           _NEW_DEPTH | \
                           _NEW_RENDERMODE | \
                           _SWRAST_NEW_RASTERMASK)

#define OSMESA_NEW_TRIANGLE (_NEW_POLYGON | \
                             _NEW_TEXTURE | \
                             _NEW_LIGHT | \
                             _NEW_DEPTH | \
                             _NEW_RENDERMODE | \
                             _SWRAST_NEW_RASTERMASK)


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

   if (osmesa->format == OSMESA_RGBA) {
      rb->GetRow = get_row_RGBA;
      rb->GetValues = get_values_RGBA;
      rb->PutRow = put_row_RGBA;
      rb->PutRowRGB = put_row_rgb_RGBA;
      rb->PutMonoRow = put_mono_row_RGBA;
      rb->PutValues = put_values_RGBA;
      rb->PutMonoValues = put_mono_values_RGBA;
   }
   else if (osmesa->format == OSMESA_BGRA) {
      rb->GetRow = get_row_BGRA;
      rb->GetValues = get_values_BGRA;
      rb->PutRow = put_row_BGRA;
      rb->PutRowRGB = put_row_rgb_BGRA;
      rb->PutMonoRow = put_mono_row_BGRA;
      rb->PutValues = put_values_BGRA;
      rb->PutMonoValues = put_mono_values_BGRA;
   }
   else if (osmesa->format == OSMESA_ARGB) {
      rb->GetRow = get_row_ARGB;
      rb->GetValues = get_values_ARGB;
      rb->PutRow = put_row_ARGB;
      rb->PutRowRGB = put_row_rgb_ARGB;
      rb->PutMonoRow = put_mono_row_ARGB;
      rb->PutValues = put_values_ARGB;
      rb->PutMonoValues = put_mono_values_ARGB;
   }
   else if (osmesa->format == OSMESA_RGB) {
      rb->GetRow = get_row_RGB;
      rb->GetValues = get_values_RGB;
      rb->PutRow = put_row_RGB;
      rb->PutRowRGB = put_row_rgb_RGB;
      rb->PutMonoRow = put_mono_row_RGB;
      rb->PutValues = put_values_RGB;
      rb->PutMonoValues = put_mono_values_RGB;
   }
   else if (osmesa->format == OSMESA_BGR) {
      rb->GetRow = get_row_BGR;
      rb->GetValues = get_values_BGR;
      rb->PutRow = put_row_BGR;
      rb->PutRowRGB = put_row_rgb_BGR;
      rb->PutMonoRow = put_mono_row_BGR;
      rb->PutValues = put_values_BGR;
      rb->PutMonoValues = put_mono_values_BGR;
   }
#if CHAN_TYPE == GL_UNSIGNED_BYTE
   else if (osmesa->format == OSMESA_RGB_565) {
      rb->GetRow = get_row_RGB_565;
      rb->GetValues = get_values_RGB_565;
      rb->PutRow = put_row_RGB_565;
      rb->PutRow = put_row_rgb_RGB_565;
      rb->PutMonoRow = put_mono_row_RGB_565;
      rb->PutValues = put_values_RGB_565;
      rb->PutMonoValues = put_mono_values_RGB_565;
   }
#endif
   else if (osmesa->format == OSMESA_COLOR_INDEX) {
      rb->GetRow = get_row_CI;
      rb->GetValues = get_values_CI;
      rb->PutRow = put_row_CI;
      rb->PutMonoRow = put_mono_row_CI;
      rb->PutValues = put_values_CI;
      rb->PutMonoValues = put_mono_values_CI;
   }
   else {
      _mesa_problem(ctx, "bad pixel format in osmesa renderbuffer_storage");
   }

   return GL_TRUE;
}


/**
 * Allocate a new renderbuffer tpo describe the user-provided color buffer.
 */
static struct gl_renderbuffer *
new_osmesa_renderbuffer(GLenum format)
{
   struct gl_renderbuffer *rb = CALLOC_STRUCT(gl_renderbuffer);
   if (rb) {
      const GLuint name = 0;
      _mesa_init_renderbuffer(rb, name);

      rb->Delete = osmesa_delete_renderbuffer;
      rb->AllocStorage = osmesa_renderbuffer_storage;

      if (format == OSMESA_COLOR_INDEX) {
         rb->_BaseFormat = GL_COLOR_INDEX;
         rb->InternalFormat = GL_COLOR_INDEX;
         rb->DataType = GL_UNSIGNED_BYTE;
      }
      else {
         rb->_BaseFormat = GL_RGBA;
         rb->InternalFormat = GL_RGBA;
         rb->DataType = CHAN_TYPE;
      }
   }
   return rb;
}



/**********************************************************************/
/*****                    Public Functions                        *****/
/**********************************************************************/


/*
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



/*
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
   GLint rshift, gshift, bshift, ashift;
   GLint rind, gind, bind, aind;
   GLint indexBits = 0, redBits = 0, greenBits = 0, blueBits = 0, alphaBits =0;
   GLboolean rgbmode;
   const GLuint i4 = 1;
   const GLubyte *i1 = (GLubyte *) &i4;
   const GLint little_endian = *i1;

   rind = gind = bind = aind = 0;
   if (format==OSMESA_COLOR_INDEX) {
      indexBits = 8;
      rshift = gshift = bshift = ashift = 0;
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
      if (little_endian) {
         rshift = 0;
         gshift = 8;
         bshift = 16;
         ashift = 24;
      }
      else {
         rshift = 24;
         gshift = 16;
         bshift = 8;
         ashift = 0;
      }
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
      if (little_endian) {
         bshift = 0;
         gshift = 8;
         rshift = 16;
         ashift = 24;
      }
      else {
         bshift = 24;
         gshift = 16;
         rshift = 8;
         ashift = 0;
      }
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
      if (little_endian) {
         ashift = 0;
         rshift = 8;
         gshift = 16;
         bshift = 24;
      }
      else {
         ashift = 24;
         rshift = 16;
         gshift = 8;
         bshift = 0;
      }
      rgbmode = GL_TRUE;
   }
   else if (format==OSMESA_RGB) {
      indexBits = 0;
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = 0;
      bshift = 0;
      gshift = 8;
      rshift = 16;
      ashift = 24;
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
      bshift = 0;
      gshift = 8;
      rshift = 16;
      ashift = 24;
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
      rshift = 11;
      gshift = 5;
      bshift = 0;
      ashift = 0;
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
         FREE(osmesa);
         return NULL;
      }

      /* Initialize device driver function table */
      _mesa_init_driver_functions(&functions);
      /* override with our functions */
      functions.GetString = get_string;
      functions.UpdateState = osmesa_update_state;
      functions.GetBufferSize = get_buffer_size;

      if (!_mesa_initialize_context(&osmesa->mesa,
                                    osmesa->gl_visual,
                                    sharelist ? &sharelist->mesa
                                              : (GLcontext *) NULL,
                                    &functions, (void *) osmesa)) {
         _mesa_destroy_visual( osmesa->gl_visual );
         FREE(osmesa);
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
         FREE(osmesa);
         return NULL;
      }

      /* create front color buffer in user-provided memory (no back buffer) */
      _mesa_add_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT,
                             new_osmesa_renderbuffer(format));
      _mesa_add_soft_renderbuffers(osmesa->gl_buffer,
                                   GL_FALSE, /* color */
                                   osmesa->gl_visual->haveDepthBuffer,
                                   osmesa->gl_visual->haveStencilBuffer,
                                   osmesa->gl_visual->haveAccumBuffer,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */ );

      osmesa->format = format;
      osmesa->buffer = NULL;
      osmesa->width = 0;
      osmesa->height = 0;
      osmesa->userRowLength = 0;
      osmesa->rowlength = 0;
      osmesa->yup = GL_TRUE;
      osmesa->rshift = rshift;
      osmesa->gshift = gshift;
      osmesa->bshift = bshift;
      osmesa->ashift = ashift;
      osmesa->rInd = rind;
      osmesa->gInd = gind;
      osmesa->bInd = bind;
      osmesa->aInd = aind;

      /* Initialize the software rasterizer and helper modules. */
      {
	 GLcontext *ctx = &osmesa->mesa;
         SWcontext *swrast;
         struct swrast_device_driver *swdd;
         TNLcontext *tnl;

	 if (!_swrast_CreateContext( ctx ) ||
             !_ac_CreateContext( ctx ) ||
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

         swdd = _swrast_GetDeviceDriverReference( ctx );
         swdd->SetBuffer = set_buffer;

         /* Extend the software rasterizer with our optimized line and triangle
          * drawing functions.
          */
         swrast = SWRAST_CONTEXT( ctx );
         swrast->choose_line = osmesa_choose_line;
         swrast->choose_triangle = osmesa_choose_triangle;
         swrast->invalidate_line |= OSMESA_NEW_LINE;
         swrast->invalidate_triangle |= OSMESA_NEW_TRIANGLE;
      }
   }
   return osmesa;
}


/*
 * Destroy an Off-Screen Mesa rendering context.
 *
 * Input:  ctx - the context to destroy
 */
GLAPI void GLAPIENTRY
OSMesaDestroyContext( OSMesaContext ctx )
{
   if (ctx) {
      _swsetup_DestroyContext( &ctx->mesa );
      _tnl_DestroyContext( &ctx->mesa );
      _ac_DestroyContext( &ctx->mesa );
      _swrast_DestroyContext( &ctx->mesa );

      _mesa_destroy_visual( ctx->gl_visual );
      _mesa_destroy_framebuffer( ctx->gl_buffer );
      _mesa_free_context_data( &ctx->mesa );
      FREE( ctx );
   }
}


/*
 * Recompute the values of the context's rowaddr array.
 */
static void
compute_row_addresses( OSMesaContext ctx )
{
   GLint bytesPerPixel, bytesPerRow, i;
   GLubyte *origin = (GLubyte *) ctx->buffer;

   if (ctx->format == OSMESA_COLOR_INDEX) {
      /* CI mode */
      bytesPerPixel = 1 * sizeof(GLchan);
   }
   else if ((ctx->format == OSMESA_RGB) || (ctx->format == OSMESA_BGR)) {
      /* RGB mode */
      bytesPerPixel = 3 * sizeof(GLchan);
   }
   else if (ctx->format == OSMESA_RGB_565) {
      /* 5/6/5 RGB pixel in 16 bits */
      bytesPerPixel = 2;
   }
   else {
      /* RGBA mode */
      bytesPerPixel = 4 * sizeof(GLchan);
   }

   bytesPerRow = ctx->rowlength * bytesPerPixel;

   if (ctx->yup) {
      /* Y=0 is bottom line of window */
      for (i = 0; i < MAX_HEIGHT; i++) {
         ctx->rowaddr[i] = (GLchan *) ((GLubyte *) origin + i * bytesPerRow);
      }
   }
   else {
      /* Y=0 is top line of window */
      for (i = 0; i < MAX_HEIGHT; i++) {
         GLint j = ctx->height - i - 1;
         ctx->rowaddr[i] = (GLchan *) ((GLubyte *) origin + j * bytesPerRow);
      }
   }
}


/*
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
 * Input:  ctx - the rendering context
 *         buffer - the image buffer memory
 *         type - data type for pixel components
 *            Normally, only GL_UNSIGNED_BYTE and GL_UNSIGNED_SHORT_5_6_5
 *            are supported.  But if Mesa's been compiled with CHAN_BITS==16
 *            then type must be GL_UNSIGNED_SHORT.  And if Mesa's been build
 *            with CHAN_BITS==32 then type must be GL_FLOAT.
 *         width, height - size of image buffer in pixels, at least 1
 * Return:  GL_TRUE if success, GL_FALSE if error because of invalid ctx,
 *          invalid buffer address, invalid type, width<1, height<1,
 *          width>internal limit or height>internal limit.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type,
                   GLsizei width, GLsizei height )
{
   if (!ctx || !buffer ||
       width < 1 || height < 1 ||
       width > MAX_WIDTH || height > MAX_HEIGHT) {
      return GL_FALSE;
   }

   if (ctx->format == OSMESA_RGB_565) {
      if (type != GL_UNSIGNED_SHORT_5_6_5)
         return GL_FALSE;
   }
   else if (type != CHAN_TYPE) {
      return GL_FALSE;
   }

   /* Need to set these before calling _mesa_make_current() since the first
    * time the context is bound, _mesa_make_current() will call our
    * get_buffer_size() function to initialize the viewport.  These are the
    * values returned by get_buffer_size():
    */
   ctx->buffer = buffer;
   ctx->width = width;
   ctx->height = height;

   osmesa_update_state( &ctx->mesa, 0 );
   _mesa_make_current( &ctx->mesa, ctx->gl_buffer, ctx->gl_buffer );

   if (ctx->userRowLength)
      ctx->rowlength = ctx->userRowLength;
   else
      ctx->rowlength = width;

   compute_row_addresses( ctx );

   /* this will make ensure we recognize the new buffer size */
   _mesa_resize_framebuffer(&ctx->mesa, ctx->gl_buffer, width, height);

   /* Added by Gerk Huisma: */
   _tnl_MakeCurrent( &ctx->mesa, ctx->mesa.DrawBuffer,
                     ctx->mesa.ReadBuffer );

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
         osmesa->rowlength = value ? value : osmesa->width;
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
         *value = osmesa->width;
         return;
      case OSMESA_HEIGHT:
         *value = osmesa->height;
         return;
      case OSMESA_FORMAT:
         *value = osmesa->format;
         return;
      case OSMESA_TYPE:
         *value = CHAN_TYPE;
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

/*
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
      /*if ((!c->gl_buffer) || (!c->gl_buffer->DepthBuffer)) {*/
      *width = 0;
      *height = 0;
      *bytesPerValue = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = c->gl_buffer->Width;
      *height = c->gl_buffer->Height;
      if (c->gl_visual->depthBits <= 16)
         *bytesPerValue = sizeof(GLushort);
      else
         *bytesPerValue = sizeof(GLuint);
      *buffer = rb->Data;
      return GL_TRUE;
   }
}

/*
 * Return the color buffer associated with an OSMesa context.
 * Input:  c - the OSMesa context
 * Output:  width, height - size of buffer in pixels
 *          format - the pixel format (OSMESA_FORMAT)
 *          buffer - pointer to color buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLAPI GLboolean GLAPIENTRY
OSMesaGetColorBuffer( OSMesaContext c, GLint *width,
                      GLint *height, GLint *format, void **buffer )
{
   if (!c->buffer) {
      *width = 0;
      *height = 0;
      *format = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = c->width;
      *height = c->height;
      *format = c->format;
      *buffer = c->buffer;
      return GL_TRUE;
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
