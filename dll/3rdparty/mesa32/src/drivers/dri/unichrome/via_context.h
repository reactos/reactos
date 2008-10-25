/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef _VIACONTEXT_H
#define _VIACONTEXT_H

#include "dri_util.h"

#include "mtypes.h"
#include "drm.h"
#include "mm.h"
#include "tnl/t_vertex.h"

#include "via_screen.h"
#include "via_tex.h"
#include "via_drm.h"

struct via_context;

/* Chip tags.  These are used to group the adapters into
 * related families.
 */
enum VIACHIPTAGS {
    VIA_UNKNOWN = 0,
    VIA_CLE266,
    VIA_KM400,
    VIA_K8M800,
    VIA_PM800,
    VIA_LAST
};

#define VIA_FALLBACK_TEXTURE           	0x1
#define VIA_FALLBACK_DRAW_BUFFER       	0x2
#define VIA_FALLBACK_READ_BUFFER       	0x4
#define VIA_FALLBACK_COLORMASK         	0x8
#define VIA_FALLBACK_SPECULAR          	0x20
#define VIA_FALLBACK_LOGICOP           	0x40
#define VIA_FALLBACK_RENDERMODE        	0x80
#define VIA_FALLBACK_STENCIL           	0x100
#define VIA_FALLBACK_BLEND_EQ          	0x200
#define VIA_FALLBACK_BLEND_FUNC        	0x400
#define VIA_FALLBACK_USER_DISABLE      	0x800
#define VIA_FALLBACK_PROJ_TEXTURE      	0x1000
#define VIA_FALLBACK_POLY_STIPPLE	0x2000

#define VIA_DMA_BUFSIZ                  4096
#define VIA_DMA_HIGHWATER               (VIA_DMA_BUFSIZ - 128)

#define VIA_NO_CLIPRECTS 0x1


/* Use the templated vertex formats:
 */
#define TAG(x) via##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*via_tri_func)(struct via_context *, viaVertex *, viaVertex *,
                             viaVertex *);
typedef void (*via_line_func)(struct via_context *, viaVertex *, viaVertex *);
typedef void (*via_point_func)(struct via_context *, viaVertex *);

/**
 * Derived from gl_renderbuffer.
 */
struct via_renderbuffer {
   struct gl_renderbuffer Base;  /* must be first! */
   drm_handle_t handle;
   drmSize size;
   unsigned long offset;
   unsigned long index;
   GLuint pitch;
   GLuint bpp;
   char *map;
   GLuint orig;		/* The drawing origin, 
			 * at (drawX,drawY) in screen space.
			 */
   char *origMap;

   int drawX;                   /* origin of drawable in draw buffer */
   int drawY;    
   int drawW;                  
   int drawH;    

   __DRIdrawablePrivate *dPriv;
};


#define VIA_MAX_TEXLEVELS	10

struct via_tex_buffer {
   struct via_tex_buffer *next, *prev;
   struct via_texture_image *image;
   unsigned long index;
   unsigned long offset;
   GLuint size;
   GLuint memType;    
   unsigned char *bufAddr;
   GLuint texBase;
   GLuint lastUsed;
};



struct via_texture_image {
   struct gl_texture_image image;
   struct via_tex_buffer *texMem;
   GLint pitchLog2;
};

struct via_texture_object {
   struct gl_texture_object obj; /* The "parent" object */

   GLuint texelBytes;
   GLuint memType;

   GLuint regTexFM;
   GLuint regTexWidthLog2[2];
   GLuint regTexHeightLog2[2];
   GLuint regTexBaseH[4];
   struct {
      GLuint baseL;
      GLuint pitchLog2;
   } regTexBaseAndPitch[12];

   GLint firstLevel, lastLevel;  /* upload tObj->Image[first .. lastLevel] */
};              



struct via_context {
   GLint refcount;   
   GLcontext *glCtx;
   GLcontext *shareCtx;

   /* XXX These don't belong here.  They should be per-drawable state. */
   struct via_renderbuffer front;
   struct via_renderbuffer back;
   struct via_renderbuffer depth;
   struct via_renderbuffer stencil; /* mirrors depth */
   struct via_renderbuffer breadcrumb;

   GLboolean hasBack;
   GLboolean hasDepth;
   GLboolean hasStencil;
   GLboolean hasAccum;
   GLuint    depthBits;
   GLuint    stencilBits;

   GLboolean have_hw_stencil;
   GLuint ClearDepth;
   GLuint depth_clear_mask;
   GLuint stencil_clear_mask;
   GLfloat depth_max;
   GLfloat polygon_offset_scale;

   GLubyte    *dma;
   viaRegion tex;
    
   /* Bit flag to keep 0track of fallbacks.
    */
   GLuint Fallback;

   /* State for via_tris.c.
    */
   GLuint newState;            /* _NEW_* flags */
   GLuint newEmitState;            /* _NEW_* flags */
   GLuint newRenderState;            /* _NEW_* flags */

   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
   GLuint vertex_attr_count;

   GLuint setupIndex;
   GLuint renderIndex;
   GLmatrix ViewportMatrix;
   GLenum renderPrimitive;
   GLenum hwPrimitive;
   GLenum hwShadeModel;
   unsigned char *verts;

   /* drmBufPtr dma_buffer;
    */
   GLuint dmaLow;
   GLuint dmaCliprectAddr;
   GLuint dmaLastPrim;
   GLboolean useAgp;
   

   /* Fallback rasterization functions 
    */
   via_point_func drawPoint;
   via_line_func drawLine;
   via_tri_func drawTri;

   /* Hardware register
    */
   GLuint regCmdA_End;
   GLuint regCmdB;

   GLuint regEnable;
   GLuint regHFBBMSKL;
   GLuint regHROP;

   GLuint regHZWTMD;
   GLuint regHSTREF;
   GLuint regHSTMD;

   GLuint regHATMD;
   GLuint regHABLCsat;
   GLuint regHABLCop;
   GLuint regHABLAsat;
   GLuint regHABLAop;
   GLuint regHABLRCa;
   GLuint regHABLRFCa;
   GLuint regHABLRCbias;
   GLuint regHABLRCb;
   GLuint regHABLRFCb;
   GLuint regHABLRAa;
   GLuint regHABLRAb;
   GLuint regHFogLF;
   GLuint regHFogCL;
   GLuint regHFogCH;

   GLuint regHLP;
   GLuint regHLPRF;
   
   GLuint regHTXnCLOD[2];
   GLuint regHTXnTB[2];
   GLuint regHTXnMPMD[2];
   GLuint regHTXnTBLCsat[2];
   GLuint regHTXnTBLCop[2];
   GLuint regHTXnTBLMPfog[2];
   GLuint regHTXnTBLAsat[2];
   GLuint regHTXnTBLRCb[2];
   GLuint regHTXnTBLRAa[2];
   GLuint regHTXnTBLRFog[2];
   GLuint regHTXnTBLRCa[2];
   GLuint regHTXnTBLRCc[2];
   GLuint regHTXnTBLRCbias[2];
   GLuint regHTXnTBC[2];
   GLuint regHTXnTRAH[2];

   int vertexSize;
   int hwVertexSize;
   GLboolean ptexHack;
   int coloroffset;
   int specoffset;

   GLint lastStamp;

   GLuint ClearColor;
   GLuint ClearMask;

   /* DRI stuff
    */
   GLboolean doPageFlip;

   struct via_renderbuffer *drawBuffer;

   GLuint numClipRects;         /* cliprects for that buffer */
   drm_clip_rect_t *pClipRects;

   GLboolean scissor;
   drm_clip_rect_t drawRect;
   drm_clip_rect_t scissorRect;

   drm_context_t hHWContext;
   drm_hw_lock_t *driHwLock;
   int driFd;
   __DRInativeDisplay *display;

   /**
    * DRI drawable bound to this context for drawing.
    */
   __DRIdrawablePrivate	*driDrawable;

   /**
    * DRI drawable bound to this context for reading.
    */
   __DRIdrawablePrivate	*driReadable;

   __DRIscreenPrivate *driScreen;
   viaScreenPrivate *viaScreen;
   drm_via_sarea_t *sarea;
   volatile GLuint* regMMIOBase;
   volatile GLuint* pnGEMode;
   volatile GLuint* regEngineStatus;
   volatile GLuint* regTranSet;
   volatile GLuint* regTranSpace;
   GLuint agpBase;
   GLuint drawType;

   GLuint nDoneFirstFlip;
   GLuint agpFullCount;

   GLboolean clearTexCache;
   GLboolean thrashing;

   /* Configuration cache
    */
   driOptionCache optionCache;

   GLuint vblank_flags;
   GLuint vbl_seq;

   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;


   GLuint pfCurrentOffset;
   GLboolean allowPageFlip;

   GLuint lastBreadcrumbRead;
   GLuint lastBreadcrumbWrite;
   GLuint lastSwap[2];
   GLuint lastDma;
   
   GLuint total_alloc[VIA_MEM_SYSTEM+1];

   struct via_tex_buffer tex_image_list[VIA_MEM_SYSTEM+1];
   struct via_tex_buffer freed_tex_buffers;
   
};



#define VIA_CONTEXT(ctx)   ((struct via_context *)(ctx->DriverCtx))



/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE(vmesa)                                	\
	do {                                                    \
    	    char __ret = 0;                                     \
    	    DRM_CAS(vmesa->driHwLock, vmesa->hHWContext,        \
        	(DRM_LOCK_HELD|vmesa->hHWContext), __ret);      \
    	    if (__ret)                                          \
        	viaGetLock(vmesa, 0);                           \
	} while (0)


/* Release the kernel lock.
 */
#define UNLOCK_HARDWARE(vmesa)                                  	\
	DRM_UNLOCK(vmesa->driFd, vmesa->driHwLock, vmesa->hHWContext);	

	

extern GLuint VIA_DEBUG;

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
#define DEBUG_PIXEL     0x1000
#define DEBUG_2D        0x2000


extern void viaGetLock(struct via_context *vmesa, GLuint flags);
extern void viaLock(struct via_context *vmesa, GLuint flags);
extern void viaUnLock(struct via_context *vmesa, GLuint flags);
extern void viaEmitHwStateLocked(struct via_context *vmesa);
extern void viaEmitScissorValues(struct via_context *vmesa, int box_nr, int emit);
extern void viaXMesaSetBackClipRects(struct via_context *vmesa);
extern void viaXMesaSetFrontClipRects(struct via_context *vmesa);
extern void viaReAllocateBuffers(GLcontext *ctx, GLframebuffer *drawbuffer, GLuint width, GLuint height);
extern void viaXMesaWindowMoved(struct via_context *vmesa);

extern GLboolean viaTexCombineState(struct via_context *vmesa,
				    const struct gl_tex_env_combine_state * combine, 
				    unsigned unit );

/* Via hw already adjusted for GL pixel centers:
 */
#define SUBPIXEL_X 0
#define SUBPIXEL_Y 0

/* TODO XXX _SOLO temp defines to make code compilable */
#ifndef GLX_PBUFFER_BIT
#define GLX_PBUFFER_BIT        0x00000004
#endif
#ifndef GLX_WINDOW_BIT
#define GLX_WINDOW_BIT 0x00000001
#endif
#ifndef VERT_BIT_CLIP
#define VERT_BIT_CLIP       0x1000000
#endif

#endif
