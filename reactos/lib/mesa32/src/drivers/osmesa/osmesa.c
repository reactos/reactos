/*
 * Mesa 3-D graphics library
 * Version:  6.0.1
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
 * Off-Screen Mesa rendering / Rendering into client memory space
 *
 * Note on thread safety:  this driver is thread safe.  All
 * functions are reentrant.  The notion of current context is
 * managed by the core _mesa_make_current() and _mesa_get_current_context()
 * functions.  Those functions are thread-safe.
 */


#include "glheader.h"
#include "GL/osmesa.h"
#include "buffers.h"
#include "bufferobj.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "imports.h"
#include "macros.h"
#include "matrix.h"
#include "mtypes.h"
#include "texformat.h"
#include "texobj.h"
#include "teximage.h"
#include "texstore.h"
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
   ASSERT(bufferBit == FRONT_LEFT_BIT);
}


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


static void
clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
       GLint x, GLint y, GLint width, GLint height )
{
   OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask;

   /* sanity check - we only have a front-left buffer */
   ASSERT((mask & (DD_FRONT_RIGHT_BIT |
                   DD_BACK_LEFT_BIT |
                   DD_BACK_RIGHT_BIT)) == 0);

   /* use optimized clear for common cases (clear whole buffer to black) */
   if (mask & DD_FRONT_LEFT_BIT) {
      if (osmesa->format == OSMESA_COLOR_INDEX) {
         if (ctx->Color.ClearIndex == 0 &&
             ctx->Color.IndexMask == (GLuint) ~0 &&
             osmesa->rowlength == osmesa->width &&
             all) {
            /* clear whole buffer to zeros */
            _mesa_bzero(osmesa->buffer,
                        osmesa->width * osmesa->height * sizeof(GLchan));
            mask &= ~DD_FRONT_LEFT_BIT;
         }
      }
      else {
         /* RGB[A] format */
         if (*colorMask == 0xffffffff &&
             ctx->Color.ClearColor[0] == 0.0F &&
             ctx->Color.ClearColor[1] == 0.0F &&
             ctx->Color.ClearColor[2] == 0.0F &&
             ctx->Color.ClearColor[3] == 0.0F &&
             osmesa->rowlength == osmesa->width &&
             all) {
            GLint bytesPerPixel;
            /* clear whole buffer to black */
            if (osmesa->format == OSMESA_RGBA ||
                osmesa->format == OSMESA_BGRA ||
                osmesa->format == OSMESA_ARGB)
               bytesPerPixel = 4 * sizeof(GLchan);
            else if (osmesa->format == OSMESA_RGB ||
                     osmesa->format == OSMESA_BGR)
               bytesPerPixel = 3 * sizeof(GLchan);
            else if (osmesa->format == OSMESA_RGB_565)
               bytesPerPixel = sizeof(GLushort);
            else {
               _mesa_problem(ctx, "bad pixel format in osmesa_clear()");
               return;
            }
            _mesa_bzero(osmesa->buffer,
                        bytesPerPixel * osmesa->width * osmesa->height);
            mask &= ~DD_FRONT_LEFT_BIT;
         }
      }
   }

   if (mask) {
      /* software fallback (spans) for everything else. */
      _swrast_Clear(ctx, mask, all, x, y, width, height);
   }
}


/**********************************************************************/
/*****        Read/write spans/arrays of pixels                   *****/
/**********************************************************************/


/* RGBA */
#define NAME(PREFIX) PREFIX##_RGBA
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#if CHAN_TYPE == GL_FLOAT
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = MAX2((R), 0.0F); \
   P[1] = MAX2((G), 0.0F); \
   P[2] = MAX2((B), 0.0F); \
   P[3] = CHAN_MAXF
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = MAX2((R), 0.0F); \
   P[1] = MAX2((G), 0.0F); \
   P[2] = MAX2((B), 0.0F); \
   P[3] = CLAMP((A), 0.0F, CHAN_MAXF)
#else
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = R;  P[1] = G;  P[2] = B;  P[3] = CHAN_MAX
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = R;  P[1] = G;  P[2] = B;  P[3] = A
#endif
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[0];  G = P[1];  B = P[2];  A = P[3]
#include "swrast/s_spantemp.h"

/* BGRA */
#define NAME(PREFIX) PREFIX##_BGRA
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[2] = R;  P[1] = G;  P[0] = B;  P[3] = CHAN_MAX
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[2] = R;  P[1] = G;  P[0] = B;  P[3] = A
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[2];  G = P[1];  B = P[0];  A = P[3]
#include "swrast/s_spantemp.h"

/* ARGB */
#define NAME(PREFIX) PREFIX##_ARGB
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 4 * (X)
#define INC_PIXEL_PTR(P) P += 4
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[1] = R;  P[2] = G;  P[3] = B;  P[0] = CHAN_MAX
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[1] = R;  P[2] = G;  P[3] = B;  P[0] = A
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[1];  G = P[2];  B = P[3];  A = P[0]
#include "swrast/s_spantemp.h"

/* RGB */
#define NAME(PREFIX) PREFIX##_RGB
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = R;  P[1] = G;  P[2] = B
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = R;  P[1] = G;  P[2] = B
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[0];  G = P[1];  B = P[2];  A = CHAN_MAX
#include "swrast/s_spantemp.h"

/* BGR */
#define NAME(PREFIX) PREFIX##_BGR
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + 3 * (X)
#define INC_PIXEL_PTR(P) P += 3
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = B;  P[1] = G;  P[2] = R
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = B;  P[1] = G;  P[2] = R
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   B = P[0];  G = P[1];  R = P[2];  A = CHAN_MAX
#include "swrast/s_spantemp.h"

/* 16-bit BGR */
#if CHAN_TYPE == GL_UNSIGNED_BYTE
#define NAME(PREFIX) PREFIX##_RGB_565
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) osmesa->rowaddr[Y] + (X)
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
#endif

/* color index */
#define NAME(PREFIX) PREFIX##_CI
#define SPAN_VARS \
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = osmesa->rowaddr[Y] + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_CI_PIXEL(P, CI) \
   P[0] = CI
#define FETCH_CI_PIXEL(CI, P) \
   CI = P[0]
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

/* one-time, per-context initialization */
static void
hook_in_driver_functions( GLcontext *ctx )
{
   OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   SWcontext *swrast = SWRAST_CONTEXT( ctx );
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   ASSERT((void *) osmesa == (void *) ctx->DriverCtx);

   /* use default TCL pipeline */
   tnl->Driver.RunPipeline = _tnl_run_pipeline;

   ctx->Driver.GetString = get_string;
   ctx->Driver.UpdateState = osmesa_update_state;
   ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;
   ctx->Driver.GetBufferSize = get_buffer_size;

   ctx->Driver.Accum = _swrast_Accum;
   ctx->Driver.Bitmap = _swrast_Bitmap;
   ctx->Driver.Clear = clear;  /* uses _swrast_Clear */
   ctx->Driver.CopyPixels = _swrast_CopyPixels;
   ctx->Driver.DrawPixels = _swrast_DrawPixels;
   ctx->Driver.ReadPixels = _swrast_ReadPixels;
   ctx->Driver.DrawBuffer = _swrast_DrawBuffer;

   ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
   ctx->Driver.TexImage1D = _mesa_store_teximage1d;
   ctx->Driver.TexImage2D = _mesa_store_teximage2d;
   ctx->Driver.TexImage3D = _mesa_store_teximage3d;
   ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
   ctx->Driver.TexSubImage2D = _mesa_store_texsubimage2d;
   ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
   ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

   ctx->Driver.CompressedTexImage1D = _mesa_store_compressed_teximage1d;
   ctx->Driver.CompressedTexImage2D = _mesa_store_compressed_teximage2d;
   ctx->Driver.CompressedTexImage3D = _mesa_store_compressed_teximage3d;
   ctx->Driver.CompressedTexSubImage1D = _mesa_store_compressed_texsubimage1d;
   ctx->Driver.CompressedTexSubImage2D = _mesa_store_compressed_texsubimage2d;
   ctx->Driver.CompressedTexSubImage3D = _mesa_store_compressed_texsubimage3d;

   ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
   ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
   ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
   ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

   swdd->SetBuffer = set_buffer;

   /* RGB(A) span/pixel functions */
   if (osmesa->format == OSMESA_RGB) {
      swdd->WriteRGBASpan = write_rgba_span_RGB;
      swdd->WriteRGBSpan = write_rgb_span_RGB;
      swdd->WriteMonoRGBASpan = write_monorgba_span_RGB;
      swdd->WriteRGBAPixels = write_rgba_pixels_RGB;
      swdd->WriteMonoRGBAPixels = write_monorgba_pixels_RGB;
      swdd->ReadRGBASpan = read_rgba_span_RGB;
      swdd->ReadRGBAPixels = read_rgba_pixels_RGB;
   }
   else if (osmesa->format == OSMESA_BGR) {
      swdd->WriteRGBASpan = write_rgba_span_BGR;
      swdd->WriteRGBSpan = write_rgb_span_BGR;
      swdd->WriteMonoRGBASpan = write_monorgba_span_BGR;
      swdd->WriteRGBAPixels = write_rgba_pixels_BGR;
      swdd->WriteMonoRGBAPixels = write_monorgba_pixels_BGR;
      swdd->ReadRGBASpan = read_rgba_span_BGR;
      swdd->ReadRGBAPixels = read_rgba_pixels_BGR;
   }
#if CHAN_TYPE == GL_UNSIGNED_BYTE
   else if (osmesa->format == OSMESA_RGB_565) {
      swdd->WriteRGBASpan = write_rgba_span_RGB_565;
      swdd->WriteRGBSpan = write_rgb_span_RGB_565;
      swdd->WriteMonoRGBASpan = write_monorgba_span_RGB_565;
      swdd->WriteRGBAPixels = write_rgba_pixels_RGB_565;
      swdd->WriteMonoRGBAPixels = write_monorgba_pixels_RGB_565;
      swdd->ReadRGBASpan = read_rgba_span_RGB_565;
      swdd->ReadRGBAPixels = read_rgba_pixels_RGB_565;
   }
#endif
   else if (osmesa->format == OSMESA_RGBA) {
      swdd->WriteRGBASpan = write_rgba_span_RGBA;
      swdd->WriteRGBSpan = write_rgb_span_RGBA;
      swdd->WriteMonoRGBASpan = write_monorgba_span_RGBA;
      swdd->WriteRGBAPixels = write_rgba_pixels_RGBA;
      swdd->WriteMonoRGBAPixels = write_monorgba_pixels_RGBA;
      swdd->ReadRGBASpan = read_rgba_span_RGBA;
      swdd->ReadRGBAPixels = read_rgba_pixels_RGBA;
   }
   else if (osmesa->format == OSMESA_BGRA) {
      swdd->WriteRGBASpan = write_rgba_span_BGRA;
      swdd->WriteRGBSpan = write_rgb_span_BGRA;
      swdd->WriteMonoRGBASpan = write_monorgba_span_BGRA;
      swdd->WriteRGBAPixels = write_rgba_pixels_BGRA;
      swdd->WriteMonoRGBAPixels = write_monorgba_pixels_BGRA;
      swdd->ReadRGBASpan = read_rgba_span_BGRA;
      swdd->ReadRGBAPixels = read_rgba_pixels_BGRA;
   }
   else if (osmesa->format == OSMESA_ARGB) {
      swdd->WriteRGBASpan = write_rgba_span_ARGB;
      swdd->WriteRGBSpan = write_rgb_span_ARGB;
      swdd->WriteMonoRGBASpan = write_monorgba_span_ARGB;
      swdd->WriteRGBAPixels = write_rgba_pixels_ARGB;
      swdd->WriteMonoRGBAPixels = write_monorgba_pixels_ARGB;
      swdd->ReadRGBASpan = read_rgba_span_ARGB;
      swdd->ReadRGBAPixels = read_rgba_pixels_ARGB;
   }
   else if (osmesa->format == OSMESA_COLOR_INDEX) {
      swdd->WriteCI32Span = write_index32_span_CI;
      swdd->WriteCI8Span = write_index8_span_CI;
      swdd->WriteMonoCISpan = write_monoindex_span_CI;
      swdd->WriteCI32Pixels = write_index_pixels_CI;
      swdd->WriteMonoCIPixels = write_monoindex_pixels_CI;
      swdd->ReadCI32Span = read_index_span_CI;
      swdd->ReadCI32Pixels = read_index_pixels_CI;
   }
   else {
      _mesa_problem(ctx, "bad pixel format in osmesa_update_state!\n");
   }

   /* Extend the software rasterizer with our optimized line and triangle
    * drawin functions.
    */
   swrast->choose_line = osmesa_choose_line;
   swrast->choose_triangle = osmesa_choose_triangle;
   swrast->invalidate_line |= OSMESA_NEW_LINE;
   swrast->invalidate_triangle |= OSMESA_NEW_TRIANGLE;
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

      /* Setup these pointers here since they're using for making the default
       * and proxy texture objects.  Actually, we don't really need to do
       * this since we're using the default fallback functions which
       * _mesa_initialize_context() would plug in if needed.
       */
      osmesa->mesa.Driver.NewTextureObject = _mesa_new_texture_object;
      osmesa->mesa.Driver.DeleteTexture = _mesa_delete_texture_object;

      if (!_mesa_initialize_context(&osmesa->mesa,
                                    osmesa->gl_visual,
                                    sharelist ? &sharelist->mesa
                                              : (GLcontext *) NULL,
                                    (void *) osmesa,
                                    GL_FALSE)) {
         _mesa_destroy_visual( osmesa->gl_visual );
         FREE(osmesa);
         return NULL;
      }

      _mesa_enable_sw_extensions(&(osmesa->mesa));
      _mesa_enable_1_3_extensions(&(osmesa->mesa));
      _mesa_enable_1_4_extensions(&(osmesa->mesa));
      _mesa_enable_1_5_extensions(&(osmesa->mesa));

      osmesa->gl_buffer = _mesa_create_framebuffer( osmesa->gl_visual,
                           (GLboolean) ( osmesa->gl_visual->depthBits > 0 ),
                           (GLboolean) ( osmesa->gl_visual->stencilBits > 0 ),
                           (GLboolean) ( osmesa->gl_visual->accumRedBits > 0 ),
                           GL_FALSE /* s/w alpha */ );

      if (!osmesa->gl_buffer) {
         _mesa_destroy_visual( osmesa->gl_visual );
         _mesa_free_context_data( &osmesa->mesa );
         FREE(osmesa);
         return NULL;
      }
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

	 _swrast_CreateContext( ctx );
	 _ac_CreateContext( ctx );
	 _tnl_CreateContext( ctx );
	 _swsetup_CreateContext( ctx );
	
	 _swsetup_Wakeup( ctx );
         hook_in_driver_functions( ctx );
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

   osmesa_update_state( &ctx->mesa, 0 );
   _mesa_make_current( &ctx->mesa, ctx->gl_buffer );

   ctx->buffer = buffer;
   ctx->width = width;
   ctx->height = height;
   if (ctx->userRowLength)
      ctx->rowlength = ctx->userRowLength;
   else
      ctx->rowlength = width;

   compute_row_addresses( ctx );

   /* init viewport */
   if (ctx->mesa.Viewport.Width == 0) {
      /* initialize viewport and scissor box to buffer size */
      _mesa_Viewport( 0, 0, width, height );
      ctx->mesa.Scissor.Width = width;
      ctx->mesa.Scissor.Height = height;
   }
   else {
      /* this will make ensure we recognize the new buffer size */
      _mesa_ResizeBuffersMESA();
   }

   /* Added by Gerk Huisma: */
   _tnl_MakeCurrent( &ctx->mesa, ctx->mesa.DrawBuffer,
                     ctx->mesa.ReadBuffer );

   return GL_TRUE;
}



GLAPI OSMesaContext GLAPIENTRY OSMesaGetCurrentContext( void )
{
   GLcontext *ctx = _mesa_get_current_context();
   if (ctx)
      return (OSMesaContext) ctx;
   else
      return NULL;
}



GLAPI void GLAPIENTRY OSMesaPixelStore( GLint pname, GLint value )
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


GLAPI void GLAPIENTRY OSMesaGetIntegerv( GLint pname, GLint *value )
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
   if ((!c->gl_buffer) || (!c->gl_buffer->DepthBuffer)) {
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
      *buffer = c->gl_buffer->DepthBuffer;
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



struct name_address {
   const char *Name;
   GLvoid *Address;
};

static struct name_address functions[] = {
   { "OSMesaCreateContext", (void *) OSMesaCreateContext },
   { "OSMesaCreateContextExt", (void *) OSMesaCreateContextExt },
   { "OSMesaDestroyContext", (void *) OSMesaDestroyContext },
   { "OSMesaMakeCurrent", (void *) OSMesaMakeCurrent },
   { "OSMesaGetCurrentContext", (void *) OSMesaGetCurrentContext },
   { "OSMesaPixelsStore", (void *) OSMesaPixelStore },
   { "OSMesaGetIntegerv", (void *) OSMesaGetIntegerv },
   { "OSMesaGetDepthBuffer", (void *) OSMesaGetDepthBuffer },
   { "OSMesaGetColorBuffer", (void *) OSMesaGetColorBuffer },
   { "OSMesaGetProcAddress", (void *) OSMesaGetProcAddress },
   { NULL, NULL }
};

GLAPI void * GLAPIENTRY
OSMesaGetProcAddress( const char *funcName )
{
   int i;
   for (i = 0; functions[i].Name; i++) {
      if (_mesa_strcmp(functions[i].Name, funcName) == 0)
         return (void *) functions[i].Address;
   }
   return (void *) _glapi_get_proc_address(funcName);
}
