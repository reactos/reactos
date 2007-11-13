/*
 * GLX Hardware Device Driver for Intel i810
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
/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810context.h,v 1.9 2002/12/16 16:18:51 dawes Exp $ */

#ifndef I810CONTEXT_INC
#define I810CONTEXT_INC

typedef struct i810_context_t i810Context;
typedef struct i810_context_t *i810ContextPtr;
typedef struct i810_texture_object_t *i810TextureObjectPtr;

#include "drm.h"
#include "mtypes.h"
#include "mm.h"

#include "i810screen.h"
#include "i810tex.h"


/* Reasons to disable hardware rasterization. 
 */
#define I810_FALLBACK_TEXTURE        0x1
#define I810_FALLBACK_DRAW_BUFFER    0x2
#define I810_FALLBACK_READ_BUFFER    0x4
#define I810_FALLBACK_COLORMASK      0x8  
#define I810_FALLBACK_SPECULAR       0x20 
#define I810_FALLBACK_LOGICOP        0x40
#define I810_FALLBACK_RENDERMODE     0x80
#define I810_FALLBACK_STENCIL        0x100
#define I810_FALLBACK_BLEND_EQ       0x200
#define I810_FALLBACK_BLEND_FUNC     0x400


#ifndef PCI_CHIP_I810				 
#define PCI_CHIP_I810              0x7121
#define PCI_CHIP_I810_DC100        0x7123
#define PCI_CHIP_I810_E            0x7125 
#define PCI_CHIP_I815              0x1132 
#endif

#define IS_I810(imesa) (imesa->i810Screen->deviceID == PCI_CHIP_I810 ||	\
			imesa->i810Screen->deviceID == PCI_CHIP_I810_DC100 || \
			imesa->i810Screen->deviceID == PCI_CHIP_I810_E)
#define IS_I815(imesa) (imesa->i810Screen->deviceID == PCI_CHIP_I815)


#define I810_UPLOAD_TEX(i) (I810_UPLOAD_TEX0<<(i))

/* Use the templated vertex formats:
 */
#define TAG(x) i810##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*i810_tri_func)( i810ContextPtr, i810Vertex *, i810Vertex *,
			       i810Vertex * );
typedef void (*i810_line_func)( i810ContextPtr, i810Vertex *, i810Vertex * );
typedef void (*i810_point_func)( i810ContextPtr, i810Vertex * );

struct i810_context_t {
   GLint refcount;   
   GLcontext *glCtx;

   /* Texture object bookkeeping
    */
   unsigned              nr_heaps;
   driTexHeap          * texture_heaps[1];
   driTextureObject      swapped;

   struct i810_texture_object_t *CurrentTexObj[2];


   /* Bit flag to keep track of fallbacks.
    */
   GLuint Fallback;

   /* State for i810vb.c and i810tris.c.
    */
   GLuint new_state;		/* _NEW_* flags */
   GLuint SetupNewInputs;
   GLuint SetupIndex;
   GLuint RenderIndex;
   GLmatrix ViewportMatrix;
   GLenum render_primitive;
   GLenum reduced_primitive;
   GLuint hw_primitive;
   GLubyte *verts;

   drmBufPtr  vertex_buffer;
   char *vertex_addr;
   GLuint vertex_low;
   GLuint vertex_high;
   GLuint vertex_last_prim;
   
   GLboolean upload_cliprects;


   /* Fallback rasterization functions 
    */
   i810_point_func draw_point;
   i810_line_func draw_line;
   i810_tri_func draw_tri;

   /* Hardware state 
    */
   GLuint dirty;		/* I810_UPLOAD_* */
   GLuint Setup[I810_CTX_SETUP_SIZE];
   GLuint BufferSetup[I810_DEST_SETUP_SIZE];
   int vertex_size;
   int vertex_stride_shift;
   unsigned int lastStamp;
   GLboolean stipple_in_hw;

   GLenum TexEnvImageFmt[2];

   /* State which can't be computed completely on the fly:
    */
   GLuint LcsCullMode;
   GLuint LcsLineWidth;
   GLuint LcsPointSize;

   /* Funny mesa mirrors
    */
   GLushort ClearColor;

   /* DRI stuff
    */
   GLuint needClip;
   GLframebuffer *glBuffer;
   GLboolean doPageFlip;

   /* These refer to the current draw (front vs. back) buffer:
    */
   int drawX;			/* origin of drawable in draw buffer */
   int drawY;
   GLuint numClipRects;		/* cliprects for that buffer */
   drm_clip_rect_t *pClipRects;

   int lastSwap;
   int texAge;
   int ctxAge;
   int dirtyAge;
  
 
   GLboolean scissor;
   drm_clip_rect_t draw_rect;
   drm_clip_rect_t scissor_rect;

   drm_context_t hHWContext;
   drm_hw_lock_t *driHwLock;
   int driFd;

   __DRIdrawablePrivate *driDrawable;
   __DRIscreenPrivate *driScreen;
   i810ScreenPrivate *i810Screen; 
   I810SAREAPtr sarea;
};


#define I810_CONTEXT(ctx)    ((i810ContextPtr)(ctx->DriverCtx))

#define GET_DISPATCH_AGE( imesa ) imesa->sarea->last_dispatch
#define GET_ENQUEUE_AGE( imesa ) imesa->sarea->last_enqueue


/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE( imesa )				\
  do {							\
    char __ret=0;					\
    DRM_CAS(imesa->driHwLock, imesa->hHWContext,	\
	    (DRM_LOCK_HELD|imesa->hHWContext), __ret);	\
    if (__ret)						\
        i810GetLock( imesa, 0 );			\
  } while (0)



/* Release the kernel lock.
 */
#define UNLOCK_HARDWARE(imesa)					\
    DRM_UNLOCK(imesa->driFd, imesa->driHwLock, imesa->hHWContext);	


/* This is the wrong way to do it, I'm sure.  Otherwise the drm
 * bitches that I've already got the heavyweight lock.  At worst,
 * this is 3 ioctls.  The best solution probably only gets me down 
 * to 2 ioctls in the worst case.
 */
#define LOCK_HARDWARE_QUIESCENT( imesa ) do {	\
   LOCK_HARDWARE( imesa );			\
   i810RegetLockQuiescent( imesa );		\
} while(0)


extern void i810GetLock( i810ContextPtr imesa, GLuint flags );
extern void i810EmitHwStateLocked( i810ContextPtr imesa );
extern void i810EmitScissorValues( i810ContextPtr imesa, int box_nr, int emit );
extern void i810EmitDrawingRectangle( i810ContextPtr imesa );
extern void i810XMesaSetBackClipRects( i810ContextPtr imesa );
extern void i810XMesaSetFrontClipRects( i810ContextPtr imesa );

#define SUBPIXEL_X -.5
#define SUBPIXEL_Y -.5

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1
#if DO_DEBUG
extern int I810_DEBUG;
#else
#define I810_DEBUG		0
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

#endif
