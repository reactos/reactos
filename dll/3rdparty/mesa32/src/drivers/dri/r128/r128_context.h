/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_context.h,v 1.12 2002/12/16 16:18:52 dawes Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __R128_CONTEXT_H__
#define __R128_CONTEXT_H__

#include "dri_util.h"
#include "drm.h"
#include "r128_drm.h"

#include "mtypes.h"
#include "tnl/t_vertex.h"

#include "r128_reg.h"

#include "texmem.h"

struct r128_context;
typedef struct r128_context r128ContextRec;
typedef struct r128_context *r128ContextPtr;

#include "r128_lock.h"
#include "r128_texobj.h"
#include "r128_screen.h"

/* Flags for what context state needs to be updated:
 */
#define R128_NEW_ALPHA		0x0001
#define R128_NEW_DEPTH		0x0002
#define R128_NEW_FOG		0x0004
#define R128_NEW_CLIP		0x0008
#define R128_NEW_CULL		0x0010
#define R128_NEW_MASKS		0x0020
#define R128_NEW_RENDER_NOT	0x0040
#define R128_NEW_WINDOW		0x0080
#define R128_NEW_CONTEXT	0x0100
#define R128_NEW_ALL		0x01ff

/* Flags for software fallback cases:
 */
#define R128_FALLBACK_TEXTURE		0x0001
#define R128_FALLBACK_DRAW_BUFFER	0x0002
#define R128_FALLBACK_READ_BUFFER	0x0004
#define R128_FALLBACK_STENCIL		0x0008
#define R128_FALLBACK_RENDER_MODE	0x0010
#define R128_FALLBACK_LOGICOP		0x0020
#define R128_FALLBACK_SEP_SPECULAR	0x0040
#define R128_FALLBACK_BLEND_EQ		0x0080
#define R128_FALLBACK_BLEND_FUNC	0x0100
#define R128_FALLBACK_PROJTEX		0x0200
#define R128_FALLBACK_DISABLE		0x0400


/* Use the templated vertex format:
 */
#define TAG(x) r128##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

/* Reasons why the GL_BLEND fallback mightn't work:
 */
#define R128_BLEND_ENV_COLOR	0x1
#define R128_BLEND_MULTITEX	0x2

/* Subpixel offsets for window coordinates (triangles):
 */
#define SUBPIXEL_X  (0.0F)
#define SUBPIXEL_Y  (0.125F)


typedef void (*r128_tri_func)( r128ContextPtr, 
				 r128Vertex *,
				 r128Vertex *,
				 r128Vertex * );

typedef void (*r128_line_func)( r128ContextPtr, 
				  r128Vertex *,
				  r128Vertex * );

typedef void (*r128_point_func)( r128ContextPtr,
				   r128Vertex * );


struct r128_context {
   GLcontext *glCtx;			/* Mesa context */

   /* Driver and hardware state management
    */
   GLuint new_state;
   GLuint dirty;			/* Hardware state to be updated */
   drm_r128_context_regs_t setup;

   /* Vertex state */
   GLuint vertex_size;
   GLuint vertex_format;
   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
   GLuint vertex_attr_count;
   char *verts;			/* points to tnl->clipspace.vertex_buf */
   GLuint num_verts;
   int coloroffset, specoffset;
   DECLARE_RENDERINPUTS(tnl_state_bitset);	/* tnl->render_inputs for this _tnl_install_attrs */

   GLuint NewGLState;
   GLuint Fallback;
   GLuint RenderIndex;
   GLfloat hw_viewport[16];
   GLfloat depth_scale;

   u_int32_t ClearColor;			/* Color used to clear color buffer */
   u_int32_t ClearDepth;			/* Value used to clear depth buffer */
   u_int32_t ClearStencil;		/* Value used to clear stencil */

   /* Map GL texture units onto hardware
    */
   GLint multitex;
   GLint tmu_source[2];
   GLuint tex_combine[2];
   GLuint blend_flags;
   GLuint env_color;

   /* Texture object bookkeeping
    */
   unsigned              nr_heaps;
   driTexHeap          * texture_heaps[ R128_NR_TEX_HEAPS ];
   driTextureObject      swapped;

   r128TexObjPtr CurrentTexObj[2];

   int texture_depth;
 
   /* Fallback rasterization functions 
    */
   r128_point_func draw_point;
   r128_line_func draw_line;
   r128_tri_func draw_tri;

   /* Vertex buffers
    */
   drmBufPtr vert_buf;

   GLuint hw_primitive;
   GLenum render_primitive;

   /* Page flipping
    */
   GLuint doPageFlip;

   /* Cliprect and scissor information
    */
   GLuint numClipRects;			/* Cliprects for the draw buffer */
   drm_clip_rect_t *pClipRects;

   GLuint scissor;
   drm_clip_rect_t ScissorRect;	/* Current software scissor */

   /* Mirrors of some DRI state
    */
   __DRIcontextPrivate	*driContext;	/* DRI context */
   __DRIscreenPrivate	*driScreen;	/* DRI screen */
   __DRIdrawablePrivate	*driDrawable;	/* DRI drawable bound to this ctx */

   unsigned int lastStamp;	        /* mirror driDrawable->lastStamp */

   drm_context_t hHWContext;
   drm_hw_lock_t *driHwLock;
   int driFd;

   r128ScreenPtr r128Screen;		/* Screen private DRI data */
   drm_r128_sarea_t *sarea;		/* Private SAREA data */

   /* Performance counters
    */
   GLuint boxes;			/* Draw performance boxes */
   GLuint hardwareWentIdle;
   GLuint c_clears;
   GLuint c_drawWaits;
   GLuint c_textureSwaps;
   GLuint c_textureBytes;
   GLuint c_vertexBuffers;

   /* VBI
    */
   GLuint vbl_seq;
   GLuint vblank_flags;

   /* Configuration cache
    */
   driOptionCache optionCache;
};

#define R128_CONTEXT(ctx)		((r128ContextPtr)(ctx->DriverCtx))

#define R128_IS_PLAIN( rmesa ) \
		(rmesa->r128Screen->chipset == R128_CARD_TYPE_R128)
#define R128_IS_PRO( rmesa ) \
		(rmesa->r128Screen->chipset == R128_CARD_TYPE_R128_PRO)
#define R128_IS_MOBILITY( rmesa ) \
		(rmesa->r128Screen->chipset == R128_CARD_TYPE_R128_MOBILITY)


extern GLboolean r128CreateContext( const __GLcontextModes *glVisual,
				    __DRIcontextPrivate *driContextPriv,
                                    void *sharedContextPrivate );

extern void r128DestroyContext( __DRIcontextPrivate * );

extern GLboolean r128MakeCurrent( __DRIcontextPrivate *driContextPriv,
                                  __DRIdrawablePrivate *driDrawPriv,
                                  __DRIdrawablePrivate *driReadPriv );

extern GLboolean r128UnbindContext( __DRIcontextPrivate *driContextPriv );

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1
#define ENABLE_PERF_BOXES	0

#if DO_DEBUG
extern int R128_DEBUG;
#else
#define R128_DEBUG		0
#endif

#define DEBUG_ALWAYS_SYNC	0x01
#define DEBUG_VERBOSE_API	0x02
#define DEBUG_VERBOSE_MSG	0x04
#define DEBUG_VERBOSE_LRU	0x08
#define DEBUG_VERBOSE_DRI	0x10
#define DEBUG_VERBOSE_IOCTL	0x20
#define DEBUG_VERBOSE_2D	0x40
#define DEBUG_VERBOSE_FALL	0x80

#endif /* __R128_CONTEXT_H__ */
