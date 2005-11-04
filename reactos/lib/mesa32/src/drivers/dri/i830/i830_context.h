/*
 * GLX Hardware Device Driver for Intel i830
 * Copyright (C) 1999 Keith Whitwell
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
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* Adapted for use in the I830M driver: 
 *   Jeff Hartmann <jhartmann@2d3d.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_context.h,v 1.7 2003/02/06 04:18:01 dawes Exp $ */

#ifndef I830CONTEXT_INC
#define I830CONTEXT_INC

typedef struct i830_context_t i830Context;
typedef struct i830_context_t *i830ContextPtr;
typedef struct i830_texture_object_t *i830TextureObjectPtr;


#include "mtypes.h"
#include "drm.h"
#include "mm.h"
#include "tnl/t_vertex.h"

#include "i830_screen.h"
#include "i830_tex.h"

#define TAG(x) i830##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

#define DV_PF_555  (1<<8)
#define DV_PF_565  (2<<8)
#define DV_PF_8888 (3<<8)

#define I830_TEX_MAXLEVELS 10

#define I830_CONTEXT(ctx)	((i830ContextPtr)(ctx->DriverCtx))
#define GET_DISPATCH_AGE(imesa) imesa->sarea->last_dispatch
#define GET_ENQUEUE_AGE(imesa)	imesa->sarea->last_enqueue


typedef void (*i830_tri_func)(i830ContextPtr, i830Vertex *, i830Vertex *,
							  i830Vertex *);
typedef void (*i830_line_func)(i830ContextPtr, i830Vertex *, i830Vertex *);
typedef void (*i830_point_func)(i830ContextPtr, i830Vertex *);

#define I830_MAX_TEXTURE_UNITS    4

#define I830_FALLBACK_TEXTURE		 0x1
#define I830_FALLBACK_DRAW_BUFFER	 0x2
#define I830_FALLBACK_READ_BUFFER	 0x4
#define I830_FALLBACK_COLORMASK		 0x8
#define I830_FALLBACK_RENDERMODE	 0x10
#define I830_FALLBACK_STENCIL		 0x20
#define I830_FALLBACK_STIPPLE		 0x40
#define I830_FALLBACK_USER		 0x80

struct i830_context_t 
{
   GLint refcount;   
   GLcontext *glCtx;

   /*From I830 stuff*/
   int TextureMode;
   GLuint renderindex;
   GLuint TexBlendWordsUsed[I830_MAX_TEXTURE_UNITS];
   GLuint TexBlend[I830_MAX_TEXTURE_UNITS][I830_TEXBLEND_SIZE];
   GLuint Init_TexBlend[I830_MAX_TEXTURE_UNITS][I830_TEXBLEND_SIZE];
   GLuint Init_TexBlendWordsUsed[I830_MAX_TEXTURE_UNITS];
   GLuint Init_BufferSetup[I830_DEST_SETUP_SIZE];
   GLuint LodBias[I830_MAX_TEXTURE_UNITS];
   
   GLenum palette_format;
   GLuint palette[256];
   
   
   GLuint Init_Setup[I830_CTX_SETUP_SIZE];
   GLuint vertex_prim;
   drmBufPtr vertex_dma_buffer;
   
   GLboolean mask_red;
   GLboolean mask_green;
   GLboolean mask_blue;
   GLboolean mask_alpha;

   GLubyte clear_red;
   GLubyte clear_green;
   GLubyte clear_blue;
   GLubyte clear_alpha;

   GLfloat depth_scale;
   int depth_clear_mask;
   int stencil_clear_mask;
   int ClearDepth;
   int hw_stencil;
   
   GLuint MonoColor;
   
   GLuint LastTexEnabled;
   GLuint TexEnabledMask;
   
   /* Texture object bookkeeping
    */
   unsigned              nr_heaps;
   driTexHeap          * texture_heaps[1];
   driTextureObject      swapped;

   struct i830_texture_object_t *CurrentTexObj[I830_MAX_TEXTURE_UNITS];

   /* Rasterization and vertex state:
    */
   GLuint Fallback;
   GLuint NewGLState;

   /* Vertex state 
    */
   GLuint vertex_size;
   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
   GLuint vertex_attr_count;
   char *verts;			/* points to tnl->clipspace.vertex_buf */

   
   /* State for i830tris.c.
    */
   GLuint RenderIndex;
   GLmatrix ViewportMatrix;
   GLenum render_primitive;
   GLenum reduced_primitive;
   GLuint hw_primitive;

   drmBufPtr  vertex_buffer;
   char *vertex_addr;
   GLuint vertex_low;
   GLuint vertex_high;
   GLuint vertex_last_prim;
   
   GLboolean upload_cliprects;


   /* Fallback rasterization functions 
    */
   i830_point_func draw_point;
   i830_line_func draw_line;
   i830_tri_func draw_tri;

   /* Hardware state 
    */
   GLuint dirty;		/* I810_UPLOAD_* */
   GLuint Setup[I830_CTX_SETUP_SIZE];
   GLuint BufferSetup[I830_DEST_SETUP_SIZE];
   GLuint StippleSetup[I830_STP_SETUP_SIZE];
   unsigned int lastStamp;
   GLboolean hw_stipple;

   GLenum TexEnvImageFmt[2];

   /* State which can't be computed completely on the fly:
    */
   GLuint LcsCullMode;
   GLuint LcsLineWidth;
   GLuint LcsPointSize;

   /* Funny mesa mirrors
    */
   GLuint ClearColor;

   /* DRI stuff
    */
   GLuint needClip;
   GLframebuffer *glBuffer;

   /* These refer to the current draw (front vs. back) buffer:
    */
   char *drawMap;		/* draw buffer address in virtual mem */
   char *readMap;	
   int drawX;			/* origin of drawable in draw buffer */
   int drawY;
   GLuint numClipRects;		/* cliprects for that buffer */
   drm_clip_rect_t *pClipRects;

   int lastSwap;
   int texAge;
   int ctxAge;
   int dirtyAge;
   int perf_boxes;

   int do_irqs;
   
   GLboolean scissor;
   drm_clip_rect_t draw_rect;
   drm_clip_rect_t scissor_rect;

   drm_context_t hHWContext;
   drmLock *driHwLock;
   int driFd;

   __DRIdrawablePrivate *driDrawable;    /**< DRI drawable bound to this
					  * context for drawing.
					  */
   __DRIdrawablePrivate *driReadable;    /**< DRI drawable bound to this
					  * context for reading.
					  */

   /**
    * Drawable used by Mesa for software fallbacks for reading and
    * writing.  It is set by Mesa's \c SetBuffer callback, and will always be
    * either \c i830_context_t::driDrawable or \c i830_context_t::driReadable.
    */
   
   __DRIdrawablePrivate * mesa_drawable;

   __DRIscreenPrivate *driScreen;
   i830ScreenPrivate *i830Screen;
   I830SAREAPtr sarea;

   /**
    * Configuration cache
    */
   driOptionCache optionCache;
};


#define I830_TEX_UNIT_ENABLED(unit)     (1<<unit)
#define VALID_I830_TEXTURE_OBJECT(tobj) (tobj)

#define I830_CONTEXT(ctx)   ((i830ContextPtr)(ctx->DriverCtx))
#define I830_DRIVER_DATA(vb) ((i830VertexBufferPtr)((vb)->driver_data))
#define GET_DISPATCH_AGE(imesa) imesa->sarea->last_dispatch
#define GET_ENQUEUE_AGE(imesa)  imesa->sarea->last_enqueue


/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE( imesa )          	    	\
do {              					\
    char __ret=0;                   			\
    DRM_CAS(imesa->driHwLock, imesa->hHWContext,    	\
        (DRM_LOCK_HELD|imesa->hHWContext), __ret);  	\
    if (__ret)                      			\
        i830GetLock( imesa, 0 );            	 	\
}while (0)
 
  
  /* Unlock the hardware using the global current context 
   */
#define UNLOCK_HARDWARE(imesa)						\
do {									\
   imesa->perf_boxes |= imesa->sarea->perf_boxes;			\
   DRM_UNLOCK(imesa->driFd, imesa->driHwLock, imesa->hHWContext);	\
} while (0)

  /* This is the wrong way to do it, I'm sure.  Otherwise the drm
   * bitches that I've already got the heavyweight lock.  At worst,
   * this is 3 ioctls.  The best solution probably only gets me down 
   * to 2 ioctls in the worst case.
   */
#define LOCK_HARDWARE_QUIESCENT( imesa ) do {		 \
   LOCK_HARDWARE( imesa );        	   		 \
   i830RegetLockQuiescent( imesa );     		 \
} while(0)



extern void i830GetLock(i830ContextPtr imesa, GLuint flags);
extern void i830EmitHwStateLocked(i830ContextPtr imesa);
extern void i830EmitDrawingRectangle(i830ContextPtr imesa);
extern void i830XMesaSetBackClipRects(i830ContextPtr imesa);
extern void i830XMesaSetFrontClipRects(i830ContextPtr imesa);
extern void i830DDExtensionsInit(GLcontext *ctx);
extern void i830DDInitDriverFuncs(GLcontext *ctx);
extern void i830DDUpdateHwState(GLcontext *ctx);

#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125


/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1
#if DO_DEBUG
extern int I830_DEBUG;
#else
#define I830_DEBUG		0
#endif

#define DEBUG_TEXTURE	0x1
#define DEBUG_STATE	0x2
#define DEBUG_IOCTL	0x4
#define DEBUG_PRIMS	0x8
#define DEBUG_VERTS	0x10
#define DEBUG_FALLBACKS	0x20
#define DEBUG_VERBOSE	0x40
#define DEBUG_DRI       0x80
#define DEBUG_DMA       0x100
#define DEBUG_SANITY    0x200
#define DEBUG_SYNC      0x400
#define DEBUG_SLEEP     0x800


#define PCI_CHIP_845_G			0x2562
#define PCI_CHIP_I830_M			0x3577
#define PCI_CHIP_I855_GM		0x3582
#define PCI_CHIP_I865_G			0x2572

	
#endif

