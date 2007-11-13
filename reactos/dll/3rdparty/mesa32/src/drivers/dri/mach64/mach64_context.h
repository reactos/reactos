/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	Jos�Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#ifndef __MACH64_CONTEXT_H__
#define __MACH64_CONTEXT_H__

#include "dri_util.h"
#include "drm.h"
#include "mach64_drm.h"

#include "mtypes.h"

#include "mach64_reg.h"

#include "texmem.h"

struct mach64_context;
typedef struct mach64_context mach64ContextRec;
typedef struct mach64_context *mach64ContextPtr;

#include "mach64_lock.h"
#include "mach64_screen.h"

/* Experimental driver options */
#define MACH64_CLIENT_STATE_EMITS       0

/* Performace monitoring */
#define ENABLE_PERF_BOXES               1

/* Native vertex format */
#define MACH64_NATIVE_VTXFMT		1

/* Flags for what context state needs to be updated:
 */
#define MACH64_NEW_ALPHA		0x0001
#define MACH64_NEW_DEPTH		0x0002
#define MACH64_NEW_FOG			0x0004
#define MACH64_NEW_CLIP			0x0008
#define MACH64_NEW_CULL			0x0010
#define MACH64_NEW_MASKS		0x0020
#define MACH64_NEW_RENDER_UNUSED	0x0040
#define MACH64_NEW_WINDOW		0x0080
#define MACH64_NEW_TEXTURE		0x0100
#define MACH64_NEW_CONTEXT		0x0200
#define MACH64_NEW_ALL			0x03ff

/* Flags for software fallback cases:
 */
#define MACH64_FALLBACK_TEXTURE		0x0001
#define MACH64_FALLBACK_DRAW_BUFFER	0x0002
#define MACH64_FALLBACK_READ_BUFFER	0x0004
#define MACH64_FALLBACK_STENCIL		0x0008
#define MACH64_FALLBACK_RENDER_MODE	0x0010
#define MACH64_FALLBACK_LOGICOP		0x0020
#define MACH64_FALLBACK_SEP_SPECULAR	0x0040
#define MACH64_FALLBACK_BLEND_EQ	0x0080
#define MACH64_FALLBACK_BLEND_FUNC	0x0100
#define MACH64_FALLBACK_DISABLE		0x0200

#define CARD32 GLuint		/* KW: For building in mesa tree */

#if MACH64_NATIVE_VTXFMT

/* The vertex structures.
 */

/* The size of this union is not of relevence:
 */
union mach64_vertex_t {
   GLfloat f[16];
   GLuint ui[16];
   GLushort us2[16][2];
   GLubyte ub4[16][4];
};

typedef union mach64_vertex_t mach64Vertex, *mach64VertexPtr;

#else

/* Use the templated vertex format:
 */
#define TAG(x) mach64##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

#endif /* MACH64_NATIVE_VTXFMT */

/* Subpixel offsets for window coordinates:
 * These are enough to fix most glean tests except polygonOffset.
 * There are also still some gaps that show in e.g. the tunnel Mesa demo
 * or the lament xscreensaver hack.
 */
#define SUBPIXEL_X	(0.0125F)
#define SUBPIXEL_Y	(0.15F)


typedef void (*mach64_tri_func)( mach64ContextPtr,
				   mach64Vertex *,
				   mach64Vertex *,
				   mach64Vertex * );

typedef void (*mach64_line_func)( mach64ContextPtr,
				    mach64Vertex *,
				    mach64Vertex * );

typedef void (*mach64_point_func)( mach64ContextPtr,
				     mach64Vertex * );

struct mach64_texture_object {
   driTextureObject   base;

   GLuint bufAddr;

   GLint heap; /* same as base.heap->heapId */

   /* For communicating values from mach64AllocTexObj(), mach64SetTexImages()
    * to mach64UpdateTextureUnit(). Alternately, we can use the tObj values or
    * set the context registers directly.
    */
   GLint widthLog2;
   GLint heightLog2;
   GLint maxLog2;

   GLint hasAlpha;
   GLint textureFormat;

   GLboolean BilinearMin;
   GLboolean BilinearMag;
   GLboolean ClampS;
   GLboolean ClampT;
};

typedef struct mach64_texture_object mach64TexObj, *mach64TexObjPtr;

struct mach64_context {
   GLcontext *glCtx;

   /* Driver and hardware state management
    */
   GLuint new_state;
   GLuint dirty;			/* Hardware state to be updated */
   drm_mach64_context_regs_t setup;

   GLuint NewGLState;
   GLuint Fallback;
   GLuint SetupIndex;
   GLuint SetupNewInputs;
   GLuint RenderIndex;
   GLfloat hw_viewport[16];
   GLfloat depth_scale;
   GLuint vertex_size;
   GLuint vertex_stride_shift;
   GLuint vertex_format;
   GLuint num_verts;
   GLubyte *verts;		

   CARD32 Color;			/* Current draw color */
   CARD32 ClearColor;			/* Color used to clear color buffer */
   CARD32 ClearDepth;			/* Value used to clear depth buffer */

   /* Map GL texture units onto hardware
    */
   GLint multitex;
   GLint tmu_source[2];
   GLint tex_dest[2];

   /* Texture object bookkeeping
    */
   mach64TexObjPtr CurrentTexObj[2];

   GLint firstTexHeap, lastTexHeap;
   driTexHeap *texture_heaps[MACH64_NR_TEX_HEAPS];
   driTextureObject swapped;

   /* Fallback rasterization functions
    */
   mach64_point_func draw_point;
   mach64_line_func draw_line;
   mach64_tri_func draw_tri;

   /* Culling */
   GLfloat backface_sign;

   /* DMA buffers
    */
   void *vert_buf;
   size_t vert_total;
   unsigned vert_used;

   GLuint hw_primitive;
   GLenum render_primitive;

   /* Visual, drawable, cliprect and scissor information
    */
   GLint drawOffset, drawPitch;
   GLint drawX, drawY;                  /* origin of drawable in draw buffer */
   GLint readOffset, readPitch;

   GLuint numClipRects;			/* Cliprects for the draw buffer */
   drm_clip_rect_t *pClipRects;

   GLint scissor;
   drm_clip_rect_t ScissorRect;	/* Current software scissor */

   /* Mirrors of some DRI state
    */
   __DRIcontextPrivate	*driContext;	/* DRI context */
   __DRIscreenPrivate	*driScreen;	/* DRI screen */
   __DRIdrawablePrivate	*driDrawable;	/* DRI drawable bound to this ctx */

   unsigned int lastStamp;		/* mirror driDrawable->lastStamp */

   drm_context_t hHWContext;
   drm_hw_lock_t *driHwLock;
   int driFd;

   mach64ScreenPtr mach64Screen;	/* Screen private DRI data */
   drm_mach64_sarea_t *sarea;		/* Private SAREA data */

   GLuint hardwareWentIdle;

#if ENABLE_PERF_BOXES
   /* Performance counters
    */
   GLuint boxes;			/* Draw performance boxes */
   GLuint c_clears;
   GLuint c_drawWaits;
   GLuint c_textureSwaps;
   GLuint c_textureBytes;
   GLuint c_agpTextureBytes;
   GLuint c_texsrc_agp;
   GLuint c_texsrc_card;
   GLuint c_vertexBuffers;
#endif

   /* VBI
    */
   GLuint vbl_seq;
   GLuint vblank_flags;
   GLuint do_irqs;

   /* Configuration cache
    */
   driOptionCache optionCache;
};

#define MACH64_CONTEXT(ctx)		((mach64ContextPtr)(ctx->DriverCtx))


extern GLboolean mach64CreateContext( const __GLcontextModes *glVisual,
				      __DRIcontextPrivate *driContextPriv,
                                      void *sharedContextPrivate );

extern void mach64DestroyContext( __DRIcontextPrivate * );

extern GLboolean mach64MakeCurrent( __DRIcontextPrivate *driContextPriv,
                                    __DRIdrawablePrivate *driDrawPriv,
                                    __DRIdrawablePrivate *driReadPriv );

extern GLboolean mach64UnbindContext( __DRIcontextPrivate *driContextPriv );

/* ================================================================
 * Byte ordering
 */
#if MESA_LITTLE_ENDIAN == 1
#define LE32_IN( x )		( *(GLuint *)(x) )
#define LE32_IN_FLOAT( x )	( *(GLfloat *)(x) )
#define LE32_OUT( x, y )	do { *(GLuint *)(x) = (y); } while (0)
#define LE32_OUT_FLOAT( x, y )	do { *(GLfloat *)(x) = (y); } while (0)
#else
#include <byteswap.h>
#define LE32_IN( x )		bswap_32( *(GLuint *)(x) )
#define LE32_IN_FLOAT( x )						\
({									\
   GLuint __tmp = bswap_32( *(GLuint *)(x) );				\
   *(GLfloat *)&__tmp;							\
})
#define LE32_OUT( x, y )	do { *(GLuint *)(x) = bswap_32( y ); } while (0)
#define LE32_OUT_FLOAT( x, y )						\
do {									\
   GLuint __tmp;							\
   *(GLfloat *)&__tmp = (y);						\
   *(GLuint *)(x) = bswap_32( __tmp );					\
} while (0)
#endif

/* ================================================================
 * DMA buffers
 */

#define DMALOCALS       CARD32 *buf=NULL; int requested=0; int outcount=0

/* called while locked for interleaved client-side state emits */
#define DMAGETPTR( dwords )					\
do {								\
   requested = (dwords);					\
   buf = (CARD32 *)mach64AllocDmaLocked( mmesa, ((dwords)*4) );	\
   outcount = 0;						\
} while(0)

#define DMAOUTREG( reg, val )				\
do {							\
   LE32_OUT( &buf[outcount++], ADRINDEX( reg ) );	\
   LE32_OUT( &buf[outcount++], ( val ) );		\
} while(0)

#define DMAADVANCE()						\
do {								\
   if (outcount < requested) {					\
      mmesa->vert_used -= (requested - outcount) * 4;	\
   }								\
} while(0)

/* ================================================================
 * Debugging:
 */

#define DO_DEBUG		1

#if DO_DEBUG
extern int MACH64_DEBUG;
#else
#define MACH64_DEBUG		0
#endif

#define DEBUG_ALWAYS_SYNC	0x001
#define DEBUG_VERBOSE_API	0x002
#define DEBUG_VERBOSE_MSG	0x004
#define DEBUG_VERBOSE_LRU	0x008
#define DEBUG_VERBOSE_DRI	0x010
#define DEBUG_VERBOSE_IOCTL	0x020
#define DEBUG_VERBOSE_PRIMS	0x040
#define DEBUG_VERBOSE_COUNT	0x080
#define DEBUG_NOWAIT		0x100
#define DEBUG_VERBOSE_FALLBACK	0x200
#endif /* __MACH64_CONTEXT_H__ */
