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


#include "main/glheader.h"
#include "GL/osmesa.h"
#include "main/context.h"
#include "main/extensions.h"
#include "main/formats.h"
#include "main/framebuffer.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/renderbuffer.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_lines.h"
#include "swrast/s_renderbuffer.h"
#include "swrast/s_triangle.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"
#include "vbo/vbo.h"


#define OSMESA_RENDERBUFFER_CLASS 0x053


/**
 * OSMesa rendering context, derived from core Mesa struct gl_context.
 */
struct osmesa_context
{
   struct gl_context mesa;		/*< Base class - this must be first */
   struct gl_config *gl_visual;		/*< Describes the buffers */
   struct swrast_renderbuffer *srb;     /*< The user's colorbuffer */
   struct gl_framebuffer *gl_buffer;	/*< The framebuffer, containing user's rb */
   GLenum format;		/*< User-specified context format */
   GLint userRowLength;		/*< user-specified number of pixels per row */
   GLint rInd, gInd, bInd, aInd;/*< index offsets for RGBA formats */
   GLvoid *rowaddr[MAX_HEIGHT];	/*< address of first pixel in each image row */
   GLboolean yup;		/*< TRUE  -> Y increases upward */
				/*< FALSE -> Y increases downward */
   GLenum DataType;
};


static INLINE OSMesaContext
OSMESA_CONTEXT(struct gl_context *ctx)
{
   /* Just cast, since we're using structure containment */
   return (OSMesaContext) ctx;
}


/**********************************************************************/
/*** Private Device Driver Functions                                ***/
/**********************************************************************/


static const GLubyte *
get_string( struct gl_context *ctx, GLenum name )
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
osmesa_update_state( struct gl_context *ctx, GLuint new_state )
{
   /* easy - just propogate */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
}



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

#include "swrast/s_linetemp.h"



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

#include "swrast/s_linetemp.h"



/**
 * Analyze context state to see if we can provide a fast line drawing
 * function.  Otherwise, return NULL.
 */
static swrast_line_func
osmesa_choose_line_function( struct gl_context *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

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
#include "swrast/s_tritemp.h"



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

#include "swrast/s_tritemp.h"



/**
 * Return pointer to an optimized triangle function if possible.
 */
static swrast_tri_func
osmesa_choose_triangle_function( struct gl_context *ctx )
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

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
osmesa_choose_triangle( struct gl_context *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   swrast->Triangle = osmesa_choose_triangle_function( ctx );
   if (!swrast->Triangle)
      _swrast_choose_triangle( ctx );
}

static void
osmesa_choose_line( struct gl_context *ctx )
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
   GLint bytesPerRow, i;
   GLubyte *origin = (GLubyte *) osmesa->srb->Buffer;
   GLint rowlength; /* in pixels */
   GLint height = osmesa->srb->Base.Height;

   if (osmesa->userRowLength)
      rowlength = osmesa->userRowLength;
   else
      rowlength = osmesa->srb->Base.Width;

   bytesPerRow = rowlength * _mesa_get_format_bytes(osmesa->srb->Base.Format);

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
 * Don't use _mesa_delete_renderbuffer since we can't free rb->Buffer.
 */
static void
osmesa_delete_renderbuffer(struct gl_renderbuffer *rb)
{
   free(rb);
}


/**
 * Allocate renderbuffer storage.  We don't actually allocate any storage
 * since we're using a user-provided buffer.
 * Just set up all the gl_renderbuffer methods.
 */
static GLboolean
osmesa_renderbuffer_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                            GLenum internalFormat, GLuint width, GLuint height)
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);

   /* Note: we can ignoring internalFormat for "window-system" renderbuffers */
   (void) internalFormat;

   /* Given the user-provided format and type, figure out which MESA_FORMAT_x
    * to use.
    * XXX There aren't Mesa formats for all the possible combinations here!
    * XXX Specifically, there's only RGBA-order 16-bit/channel and float
    * XXX formats.
    * XXX The 8-bit/channel formats should all be OK.
    */
   if (osmesa->format == OSMESA_RGBA) {
      if (osmesa->DataType == GL_UNSIGNED_BYTE) {
         if (_mesa_little_endian())
            rb->Format = MESA_FORMAT_RGBA8888_REV;
         else
            rb->Format = MESA_FORMAT_RGBA8888;
      }
      else if (osmesa->DataType == GL_UNSIGNED_SHORT) {
         rb->Format = MESA_FORMAT_RGBA_16;
      }
      else {
         rb->Format = MESA_FORMAT_RGBA_FLOAT32;
      }
   }
   else if (osmesa->format == OSMESA_BGRA) {
      if (osmesa->DataType == GL_UNSIGNED_BYTE) {
         if (_mesa_little_endian())
            rb->Format = MESA_FORMAT_ARGB8888;
         else
            rb->Format = MESA_FORMAT_ARGB8888_REV;
      }
      else if (osmesa->DataType == GL_UNSIGNED_SHORT) {
         _mesa_warning(ctx, "Unsupported OSMesa format BGRA/GLushort");
         rb->Format = MESA_FORMAT_RGBA_16; /* not exactly right */
      }
      else {
         _mesa_warning(ctx, "Unsupported OSMesa format BGRA/GLfloat");
         rb->Format = MESA_FORMAT_RGBA_FLOAT32; /* not exactly right */
      }
   }
   else if (osmesa->format == OSMESA_ARGB) {
      if (osmesa->DataType == GL_UNSIGNED_BYTE) {
         if (_mesa_little_endian())
            rb->Format = MESA_FORMAT_ARGB8888_REV;
         else
            rb->Format = MESA_FORMAT_ARGB8888;
      }
      else if (osmesa->DataType == GL_UNSIGNED_SHORT) {
         _mesa_warning(ctx, "Unsupported OSMesa format ARGB/GLushort");
         rb->Format = MESA_FORMAT_RGBA_16; /* not exactly right */
      }
      else {
         _mesa_warning(ctx, "Unsupported OSMesa format ARGB/GLfloat");
         rb->Format = MESA_FORMAT_RGBA_FLOAT32; /* not exactly right */
      }
   }
   else if (osmesa->format == OSMESA_RGB) {
      if (osmesa->DataType == GL_UNSIGNED_BYTE) {
         rb->Format = MESA_FORMAT_RGB888;
      }
      else if (osmesa->DataType == GL_UNSIGNED_SHORT) {
         _mesa_warning(ctx, "Unsupported OSMesa format RGB/GLushort");
         rb->Format = MESA_FORMAT_RGBA_16; /* not exactly right */
      }
      else {
         _mesa_warning(ctx, "Unsupported OSMesa format RGB/GLfloat");
         rb->Format = MESA_FORMAT_RGBA_FLOAT32; /* not exactly right */
      }
   }
   else if (osmesa->format == OSMESA_BGR) {
      if (osmesa->DataType == GL_UNSIGNED_BYTE) {
         rb->Format = MESA_FORMAT_BGR888;
      }
      else if (osmesa->DataType == GL_UNSIGNED_SHORT) {
         _mesa_warning(ctx, "Unsupported OSMesa format BGR/GLushort");
         rb->Format = MESA_FORMAT_RGBA_16; /* not exactly right */
      }
      else {
         _mesa_warning(ctx, "Unsupported OSMesa format BGR/GLfloat");
         rb->Format = MESA_FORMAT_RGBA_FLOAT32; /* not exactly right */
      }
   }
   else if (osmesa->format == OSMESA_RGB_565) {
      ASSERT(osmesa->DataType == GL_UNSIGNED_BYTE);
      rb->Format = MESA_FORMAT_RGB565;
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
static struct swrast_renderbuffer *
new_osmesa_renderbuffer(struct gl_context *ctx, GLenum format, GLenum type)
{
   const GLuint name = 0;
   struct swrast_renderbuffer *srb = CALLOC_STRUCT(swrast_renderbuffer);

   if (srb) {
      _mesa_init_renderbuffer(&srb->Base, name);

      srb->Base.ClassID = OSMESA_RENDERBUFFER_CLASS;
      srb->Base.RefCount = 1;
      srb->Base.Delete = osmesa_delete_renderbuffer;
      srb->Base.AllocStorage = osmesa_renderbuffer_storage;

      srb->Base.InternalFormat = GL_RGBA;
      srb->Base._BaseFormat = GL_RGBA;

      return srb;
   }
   return NULL;
}



static void
osmesa_MapRenderbuffer(struct gl_context *ctx,
                       struct gl_renderbuffer *rb,
                       GLuint x, GLuint y, GLuint w, GLuint h,
                       GLbitfield mode,
                       GLubyte **mapOut, GLint *rowStrideOut)
{
   const OSMesaContext osmesa = OSMESA_CONTEXT(ctx);

   if (rb->ClassID == OSMESA_RENDERBUFFER_CLASS) {
      /* this is an OSMesa renderbuffer which wraps user memory */
      struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
      const GLuint bpp = _mesa_get_format_bytes(rb->Format);
      GLint rowStride; /* in bytes */

      if (osmesa->userRowLength)
         rowStride = osmesa->userRowLength * bpp;
      else
         rowStride = rb->Width * bpp;

      if (!osmesa->yup) {
         /* Y=0 is top line of window */
         y = rb->Height - y - 1;
         *rowStrideOut = -rowStride;
      }
      else {
         *rowStrideOut = rowStride;
      }

      *mapOut = (GLubyte *) srb->Buffer + y * rowStride + x * bpp;
   }
   else {
      _swrast_map_soft_renderbuffer(ctx, rb, x, y, w, h, mode,
                                    mapOut, rowStrideOut);
   }
}


static void
osmesa_UnmapRenderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   if (rb->ClassID == OSMESA_RENDERBUFFER_CLASS) {
      /* no-op */
   }
   else {
      _swrast_unmap_soft_renderbuffer(ctx, rb);
   }
}


/**********************************************************************/
/*****                    Public Functions                        *****/
/**********************************************************************/


/**
 * Create an Off-Screen Mesa rendering context.  The only attribute needed is
 * an RGBA vs Color-Index mode flag.
 *
 * Input:  format - Must be GL_RGBA
 *         sharelist - specifies another OSMesaContext with which to share
 *                     display lists.  NULL indicates no sharing.
 * Return:  an OSMesaContext or 0 if error
 */
GLAPI OSMesaContext GLAPIENTRY
OSMesaCreateContext( GLenum format, OSMesaContext sharelist )
{
   return OSMesaCreateContextExt(format, DEFAULT_SOFTWARE_DEPTH_BITS,
                                 8, 0, sharelist);
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
   GLint redBits = 0, greenBits = 0, blueBits = 0, alphaBits =0;

   rind = gind = bind = aind = 0;
   if (format==OSMESA_RGBA) {
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      rind = 0;
      gind = 1;
      bind = 2;
      aind = 3;
   }
   else if (format==OSMESA_BGRA) {
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      bind = 0;
      gind = 1;
      rind = 2;
      aind = 3;
   }
   else if (format==OSMESA_ARGB) {
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = CHAN_BITS;
      aind = 0;
      rind = 1;
      gind = 2;
      bind = 3;
   }
   else if (format==OSMESA_RGB) {
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = 0;
      rind = 0;
      gind = 1;
      bind = 2;
   }
   else if (format==OSMESA_BGR) {
      redBits = CHAN_BITS;
      greenBits = CHAN_BITS;
      blueBits = CHAN_BITS;
      alphaBits = 0;
      rind = 2;
      gind = 1;
      bind = 0;
   }
#if CHAN_TYPE == GL_UNSIGNED_BYTE
   else if (format==OSMESA_RGB_565) {
      redBits = 5;
      greenBits = 6;
      blueBits = 5;
      alphaBits = 0;
      rind = 0; /* not used */
      gind = 0;
      bind = 0;
   }
#endif
   else {
      return NULL;
   }

   osmesa = (OSMesaContext) CALLOC_STRUCT(osmesa_context);
   if (osmesa) {
      osmesa->gl_visual = _mesa_create_visual( GL_FALSE,    /* double buffer */
                                               GL_FALSE,    /* stereo */
                                               redBits,
                                               greenBits,
                                               blueBits,
                                               alphaBits,
                                               depthBits,
                                               stencilBits,
                                               accumBits,
                                               accumBits,
                                               accumBits,
                                               alphaBits ? accumBits : 0,
                                               1            /* num samples */
                                               );
      if (!osmesa->gl_visual) {
         free(osmesa);
         return NULL;
      }

      /* Initialize device driver function table */
      _mesa_init_driver_functions(&functions);
      /* override with our functions */
      functions.GetString = get_string;
      functions.UpdateState = osmesa_update_state;
      functions.GetBufferSize = NULL;

      if (!_mesa_initialize_context(&osmesa->mesa,
                                    API_OPENGL,
                                    osmesa->gl_visual,
                                    sharelist ? &sharelist->mesa
                                              : (struct gl_context *) NULL,
                                    &functions, (void *) osmesa)) {
         _mesa_destroy_visual( osmesa->gl_visual );
         free(osmesa);
         return NULL;
      }

      _mesa_enable_sw_extensions(&(osmesa->mesa));
      _mesa_enable_1_3_extensions(&(osmesa->mesa));
      _mesa_enable_1_4_extensions(&(osmesa->mesa));
      _mesa_enable_1_5_extensions(&(osmesa->mesa));
      _mesa_enable_2_0_extensions(&(osmesa->mesa));
      _mesa_enable_2_1_extensions(&(osmesa->mesa));

      osmesa->gl_buffer = _mesa_create_framebuffer(osmesa->gl_visual);
      if (!osmesa->gl_buffer) {
         _mesa_destroy_visual( osmesa->gl_visual );
         _mesa_free_context_data( &osmesa->mesa );
         free(osmesa);
         return NULL;
      }

      /* Create depth/stencil/accum buffers.  We'll create the color
       * buffer later in OSMesaMakeCurrent().
       */
      _swrast_add_soft_renderbuffers(osmesa->gl_buffer,
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

      _mesa_meta_init(&osmesa->mesa);

      /* Initialize the software rasterizer and helper modules. */
      {
	 struct gl_context *ctx = &osmesa->mesa;
         SWcontext *swrast;
         TNLcontext *tnl;

	 if (!_swrast_CreateContext( ctx ) ||
             !_vbo_CreateContext( ctx ) ||
             !_tnl_CreateContext( ctx ) ||
             !_swsetup_CreateContext( ctx )) {
            _mesa_destroy_visual(osmesa->gl_visual);
            _mesa_free_context_data(ctx);
            free(osmesa);
            return NULL;
         }
	
	 _swsetup_Wakeup( ctx );

         /* use default TCL pipeline */
         tnl = TNL_CONTEXT(ctx);
         tnl->Driver.RunPipeline = _tnl_run_pipeline;

         ctx->Driver.MapRenderbuffer = osmesa_MapRenderbuffer;
         ctx->Driver.UnmapRenderbuffer = osmesa_UnmapRenderbuffer;

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
      if (osmesa->srb)
         _mesa_reference_renderbuffer((struct gl_renderbuffer **) &osmesa->srb, NULL);

      _mesa_meta_free( &osmesa->mesa );

      _swsetup_DestroyContext( &osmesa->mesa );
      _tnl_DestroyContext( &osmesa->mesa );
      _vbo_DestroyContext( &osmesa->mesa );
      _swrast_DestroyContext( &osmesa->mesa );

      _mesa_destroy_visual( osmesa->gl_visual );
      _mesa_reference_framebuffer( &osmesa->gl_buffer, NULL );

      _mesa_free_context_data( &osmesa->mesa );
      free( osmesa );
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


   /* Create a front/left color buffer which wraps the user-provided buffer.
    * There is no back color buffer.
    * If the user tries to use a 8, 16 or 32-bit/channel buffer that
    * doesn't match what Mesa was compiled for (CHAN_BITS) the
    * _mesa_add_renderbuffer() function will create a "wrapper" renderbuffer
    * that converts rendering from CHAN_BITS to the user-requested channel
    * size.
    */
   if (!osmesa->srb) {
      osmesa->srb = new_osmesa_renderbuffer(&osmesa->mesa, osmesa->format, type);
      _mesa_remove_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT);
      _mesa_add_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT,
                             &osmesa->srb->Base);
      assert(osmesa->srb->Base.RefCount == 2);
   }

   osmesa->DataType = type;

   /* Set renderbuffer fields.  Set width/height = 0 to force 
    * osmesa_renderbuffer_storage() being called by _mesa_resize_framebuffer()
    */
   osmesa->srb->Buffer = buffer;
   osmesa->srb->Base.Width = osmesa->srb->Base.Height = 0;

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
   _mesa_add_renderbuffer(osmesa->gl_buffer, BUFFER_FRONT_LEFT,
                          &osmesa->srb->Base);


   /* this updates the visual's red/green/blue/alphaBits fields */
   _mesa_update_framebuffer_visual(&osmesa->mesa, osmesa->gl_buffer);

   /* update the framebuffer size */
   _mesa_resize_framebuffer(&osmesa->mesa, osmesa->gl_buffer, width, height);

   return GL_TRUE;
}



GLAPI OSMesaContext GLAPIENTRY
OSMesaGetCurrentContext( void )
{
   struct gl_context *ctx = _mesa_get_current_context();
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
         *value = osmesa->DataType;
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
   struct swrast_renderbuffer *srb = NULL;

   if (c->gl_buffer)
      srb = swrast_renderbuffer(c->gl_buffer->
                                Attachment[BUFFER_DEPTH].Renderbuffer);

   if (!srb || !srb->Buffer) {
      *width = 0;
      *height = 0;
      *bytesPerValue = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = srb->Base.Width;
      *height = srb->Base.Height;
      if (c->gl_visual->depthBits <= 16)
         *bytesPerValue = sizeof(GLushort);
      else
         *bytesPerValue = sizeof(GLuint);
      *buffer = (void *) srb->Buffer;
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
   if (osmesa->srb && osmesa->srb->Buffer) {
      *width = osmesa->srb->Base.Width;
      *height = osmesa->srb->Base.Height;
      *format = osmesa->format;
      *buffer = (void *) osmesa->srb->Buffer;
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
      if (strcmp(functions[i].Name, funcName) == 0)
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


/**
 * When GLX_INDIRECT_RENDERING is defined, some symbols are missing in
 * libglapi.a.  We need to define them here.
 */
#ifdef GLX_INDIRECT_RENDERING

#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "glapi/glapi.h"
#include "glapi/glapitable.h"

#if defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   GET_DISPATCH()->FUNC ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   return GET_DISPATCH()->FUNC ARGS

/* skip normal ones */
#define _GLAPI_SKIP_NORMAL_ENTRY_POINTS
#include "glapi/glapitemp.h"

#endif /* GLX_INDIRECT_RENDERING */
