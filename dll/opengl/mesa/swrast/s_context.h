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


/**
 * \file swrast/s_context.h
 * \brief Software rasterization context and private types.
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

/**
 * \mainpage swrast module
 *
 * This module, software rasterization, contains the software fallback
 * routines for drawing points, lines, triangles, bitmaps and images.
 * All rendering boils down to writing spans (arrays) of pixels with
 * particular colors.  The span-writing routines must be implemented
 * by the device driver.
 */


#ifndef S_CONTEXT_H
#define S_CONTEXT_H

#include "main/compiler.h"
#include "main/mtypes.h"
#include "swrast.h"
#include "s_span.h"


typedef void (*texture_sample_func)(struct gl_context *ctx,
                                    const struct gl_texture_object *tObj,
                                    GLuint n, const GLfloat texcoords[][4],
                                    const GLfloat lambda[], GLfloat rgba[][4]);

typedef void (_ASMAPIP blend_func)( struct gl_context *ctx, GLuint n,
                                    const GLubyte mask[],
                                    GLvoid *src, const GLvoid *dst,
                                    GLenum chanType);

typedef void (*swrast_point_func)( struct gl_context *ctx, const SWvertex *);

typedef void (*swrast_line_func)( struct gl_context *ctx,
                                  const SWvertex *, const SWvertex *);

typedef void (*swrast_tri_func)( struct gl_context *ctx, const SWvertex *,
                                 const SWvertex *, const SWvertex *);


typedef void (*validate_texture_image_func)(struct gl_context *ctx,
                                            struct gl_texture_object *texObj,
                                            GLuint face, GLuint level);


/**
 * \defgroup Bitmasks
 * Bitmasks to indicate which rasterization options are enabled
 * (RasterMask)
 */
/*@{*/
#define ALPHATEST_BIT		0x001	/**< Alpha-test pixels */
#define BLEND_BIT		0x002	/**< Blend pixels */
#define DEPTH_BIT		0x004	/**< Depth-test pixels */
#define FOG_BIT			0x008	/**< Fog pixels */
#define LOGIC_OP_BIT		0x010	/**< Apply logic op in software */
#define CLIP_BIT		0x020	/**< Scissor or window clip pixels */
#define STENCIL_BIT		0x040	/**< Stencil pixels */
#define MASKING_BIT		0x080	/**< Do glColorMask or glIndexMask */
#define MULTI_DRAW_BIT		0x400	/**< Write to more than one color- */
                                        /**< buffer or no buffers. */
#define TEXTURE_BIT		0x1000	/**< Texturing really enabled */
#define CLAMPING_BIT            0x8000  /**< Clamp colors to [0,1] */
/*@}*/

#define _SWRAST_NEW_RASTERMASK (_NEW_BUFFERS|	\
			        _NEW_SCISSOR|	\
			        _NEW_COLOR|	\
			        _NEW_DEPTH|	\
			        _NEW_FOG|	\
			        _NEW_STENCIL|	\
			        _NEW_TEXTURE|	\
			        _NEW_VIEWPORT|	\
			        _NEW_DEPTH)


struct swrast_texture_image;


/**
 * Fetch a texel from texture image at given position.
 */
typedef void (*FetchTexelFunc)(const struct swrast_texture_image *texImage,
                               GLint col, GLint row, GLint img,
                               GLfloat *texelOut);


/**
 * Subclass of gl_texture_image.
 * We need extra fields/info to keep tracking of mapped texture buffers,
 * strides and Fetch functions.
 */
struct swrast_texture_image
{
   struct gl_texture_image Base;

   GLboolean _IsPowerOfTwo;  /**< Are all dimensions powers of two? */

   /** used for mipmap LOD computation */
   GLfloat WidthScale, HeightScale, DepthScale;

   /** These fields only valid when texture memory is mapped */
   GLint RowStride;		/**< Padded width in units of texels */
   GLuint *ImageOffsets;        /**< if 3D texture: array [Depth] of offsets to
                                     each 2D slice in 'Data', in texels */
   GLubyte *Map;		/**< Pointer to mapped image memory */

   /** Malloc'd texture memory */
   GLubyte *Buffer;

   FetchTexelFunc FetchTexel;
};


/** cast wrapper */
static inline struct swrast_texture_image *
swrast_texture_image(struct gl_texture_image *img)
{
   return (struct swrast_texture_image *) img;
}

/** cast wrapper */
static inline const struct swrast_texture_image *
swrast_texture_image_const(const struct gl_texture_image *img)
{
   return (const struct swrast_texture_image *) img;
}


/**
 * Subclass of gl_renderbuffer with extra fields needed for software
 * rendering.
 */
struct swrast_renderbuffer
{
   struct gl_renderbuffer Base;

   GLubyte *Buffer;     /**< The malloc'd memory for buffer */

   /** These fields are only valid while buffer is mapped for rendering */
   GLubyte *Map;
   GLint RowStride;    /**< in bytes */

   /** For span rendering */
   GLenum ColorType;
};


/** cast wrapper */
static inline struct swrast_renderbuffer *
swrast_renderbuffer(struct gl_renderbuffer *img)
{
   return (struct swrast_renderbuffer *) img;
}



/**
 * \struct SWcontext
 * \brief  Per-context state that's private to the software rasterizer module.
 */
typedef struct
{
   /** Driver interface:
    */
   struct swrast_device_driver Driver;

   /** Configuration mechanisms to make software rasterizer match
    * characteristics of the hardware rasterizer (if present):
    */
   GLboolean AllowVertexFog;
   GLboolean AllowPixelFog;

   /** Derived values, invalidated on statechanges, updated from
    * _swrast_validate_derived():
    */
   GLbitfield _RasterMask;
   GLfloat _BackfaceSign;      /** +1 or -1 */
   GLfloat _BackfaceCullSign;  /** +1, 0, or -1 */
   GLboolean _PreferPixelFog;    /* Compute fog blend factor per fragment? */
   GLboolean _TextureCombinePrimary;
   GLboolean _FogEnabled;
   GLboolean _DeferredTexture;

   /** List/array of the fragment attributes to interpolate */
   GLuint _ActiveAttribs[FRAG_ATTRIB_MAX];
   /** Same info, but as a bitmask of FRAG_BIT_x bits */
   GLbitfield64 _ActiveAttribMask;
   /** Number of fragment attributes to interpolate */
   GLuint _NumActiveAttribs;
   /** Indicates how each attrib is to be interpolated (lines/tris) */
   GLenum _InterpMode[FRAG_ATTRIB_MAX]; /* GL_FLAT or GL_SMOOTH (for now) */

   /* Working values:
    */
   GLuint StippleCounter;    /**< Line stipple counter */
   GLbitfield NewState;
   GLuint StateChanges;
   GLenum Primitive;    /* current primitive being drawn (ala glBegin) */

   void (*InvalidateState)( struct gl_context *ctx, GLbitfield new_state );

   /**
    * When the NewState mask intersects these masks, we invalidate the
    * Point/Line/Triangle function pointers below.
    */
   /*@{*/
   GLbitfield InvalidatePointMask;
   GLbitfield InvalidateLineMask;
   GLbitfield InvalidateTriangleMask;
   /*@}*/

   /**
    * Device drivers plug in functions for these callbacks.
    * Will be called when the GL state change mask intersects the above masks.
    */
   /*@{*/
   void (*choose_point)( struct gl_context * );
   void (*choose_line)( struct gl_context * );
   void (*choose_triangle)( struct gl_context * );
   /*@}*/

   /**
    * Current point, line and triangle drawing functions.
    */
   /*@{*/
   swrast_point_func Point;
   swrast_line_func Line;
   swrast_tri_func Triangle;
   /*@}*/

   /**
    * Placeholders for when separate specular (or secondary color) is
    * enabled but texturing is not.
    */
   /*@{*/
   swrast_point_func SpecPoint;
   swrast_line_func SpecLine;
   swrast_tri_func SpecTriangle;
   /*@}*/

   /**
    * Typically, we'll allocate a sw_span structure as a local variable
    * and set its 'array' pointer to point to this object.  The reason is
    * this object is big and causes problems when allocated on the stack
    * on some systems.
    */
   SWspanarrays *SpanArrays;
   SWspanarrays *ZoomedArrays;  /**< For pixel zooming */

   /**
    * Used to buffer N GL_POINTS, instead of rendering one by one.
    */
   SWspan PointSpan;

   /** Internal hooks, kept up to date by the same mechanism as above.
    */
   blend_func BlendFunc;
   texture_sample_func TextureSample;

   /** Buffer for saving the sampled texture colors.
    * Needed for GL_ARB_texture_env_crossbar implementation.
    */
   GLfloat *TexelBuffer;

   validate_texture_image_func ValidateTextureImage;

} SWcontext;


extern void
_swrast_validate_derived( struct gl_context *ctx );

extern void
_swrast_update_texture_samplers(struct gl_context *ctx);


/** Return SWcontext for the given struct gl_context */
static inline SWcontext *
SWRAST_CONTEXT(struct gl_context *ctx)
{
   return (SWcontext *) ctx->swrast_context;
}

/** const version of above */
static inline const SWcontext *
CONST_SWRAST_CONTEXT(const struct gl_context *ctx)
{
   return (const SWcontext *) ctx->swrast_context;
}


/**
 * Called prior to framebuffer reading/writing.
 * For drivers that rely on swrast for fallback rendering, this is the
 * driver's opportunity to map renderbuffers and textures.
 */
static inline void
swrast_render_start(struct gl_context *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (swrast->Driver.SpanRenderStart)
      swrast->Driver.SpanRenderStart(ctx);
}


/** Called after framebuffer reading/writing */
static inline void
swrast_render_finish(struct gl_context *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (swrast->Driver.SpanRenderFinish)
      swrast->Driver.SpanRenderFinish(ctx);
}


extern void
_swrast_span_render_start(struct gl_context *ctx);

extern void
_swrast_span_render_finish(struct gl_context *ctx);

extern void
_swrast_map_textures(struct gl_context *ctx);

extern void
_swrast_unmap_textures(struct gl_context *ctx);

extern void
_swrast_map_texture(struct gl_context *ctx, struct gl_texture_object *texObj);

extern void
_swrast_unmap_texture(struct gl_context *ctx, struct gl_texture_object *texObj);


extern void
_swrast_map_renderbuffers(struct gl_context *ctx);

extern void
_swrast_unmap_renderbuffers(struct gl_context *ctx);


/**
 * Size of an RGBA pixel, in bytes, for given datatype.
 */
#define RGBA_PIXEL_SIZE(TYPE)                                     \
         ((TYPE == GL_UNSIGNED_BYTE) ? 4 * sizeof(GLubyte) :      \
          ((TYPE == GL_UNSIGNED_SHORT) ? 4 * sizeof(GLushort)     \
           : 4 * sizeof(GLfloat)))



/*
 * Fixed point arithmetic macros
 */
#ifndef FIXED_FRAC_BITS
#define FIXED_FRAC_BITS 11
#endif

#define FIXED_SHIFT     FIXED_FRAC_BITS
#define FIXED_ONE       (1 << FIXED_SHIFT)
#define FIXED_HALF      (1 << (FIXED_SHIFT-1))
#define FIXED_FRAC_MASK (FIXED_ONE - 1)
#define FIXED_INT_MASK  (~FIXED_FRAC_MASK)
#define FIXED_EPSILON   1
#define FIXED_SCALE     ((float) FIXED_ONE)
#define FIXED_DBL_SCALE ((double) FIXED_ONE)
#define FloatToFixed(X) (IROUND((X) * FIXED_SCALE))
#define FixedToDouble(X) ((X) * (1.0 / FIXED_DBL_SCALE))
#define IntToFixed(I)   ((I) << FIXED_SHIFT)
#define FixedToInt(X)   ((X) >> FIXED_SHIFT)
#define FixedToUns(X)   (((unsigned int)(X)) >> FIXED_SHIFT)
#define FixedCeil(X)    (((X) + FIXED_ONE - FIXED_EPSILON) & FIXED_INT_MASK)
#define FixedFloor(X)   ((X) & FIXED_INT_MASK)
#define FixedToFloat(X) ((X) * (1.0F / FIXED_SCALE))
#define PosFloatToFixed(X)      FloatToFixed(X)
#define SignedFloatToFixed(X)   FloatToFixed(X)



/*
 * XXX these macros are just bandages for now in order to make
 * CHAN_BITS==32 compile cleanly.
 * These should probably go elsewhere at some point.
 */
#if CHAN_TYPE == GL_FLOAT
#define ChanToFixed(X)  (X)
#define FixedToChan(X)  (X)
#else
#define ChanToFixed(X)  IntToFixed(X)
#define FixedToChan(X)  FixedToInt(X)
#endif


/**
 * For looping over fragment attributes in the pointe, line
 * triangle rasterizers.
 */
#define ATTRIB_LOOP_BEGIN                                \
   {                                                     \
      GLuint a;                                          \
      for (a = 0; a < swrast->_NumActiveAttribs; a++) {  \
         const GLuint attr = swrast->_ActiveAttribs[a];

#define ATTRIB_LOOP_END } }


/**
 * Return the address of a pixel value in a mapped renderbuffer.
 */
static inline GLubyte *
_swrast_pixel_address(struct gl_renderbuffer *rb, GLint x, GLint y)
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
   const GLint bpp = _mesa_get_format_bytes(rb->Format);
   const GLint rowStride = srb->RowStride;
   assert(x >= 0);
   assert(y >= 0);
   /* NOTE: using <= only because of s_tritemp.h which gets a pixel
    * address but doesn't necessarily access it.
    */
   assert(x <= (GLint) rb->Width);
   assert(y <= (GLint) rb->Height);
   assert(srb->Map);
   return (GLubyte *) srb->Map + y * rowStride + x * bpp;
}



#endif
