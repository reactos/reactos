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



#ifndef SAVAGECONTEXT_INC
#define SAVAGECONTEXT_INC

typedef struct savage_context_t savageContext;
typedef struct savage_context_t *savageContextPtr;
typedef struct savage_texture_object_t *savageTextureObjectPtr;

#include <X11/Xlibint.h>
#include "dri_util.h"
#include "mtypes.h"
#include "xf86drm.h"
#include "drm.h"
#include "savage_drm.h"
#include "savage_init.h"
#include "savage_3d_reg.h"
#include "mm.h"
#include "tnl/t_vertex.h"

#include "texmem.h"

#include "xmlconfig.h"

/* Reasons to fallback on all primitives.
 */
#define SAVAGE_FALLBACK_TEXTURE        0x1
#define SAVAGE_FALLBACK_DRAW_BUFFER    0x2
#define SAVAGE_FALLBACK_READ_BUFFER    0x4
#define SAVAGE_FALLBACK_COLORMASK      0x8  
#define SAVAGE_FALLBACK_SPECULAR       0x10 
#define SAVAGE_FALLBACK_LOGICOP        0x20
/*frank 2001/11/12 add the stencil fallbak*/
#define SAVAGE_FALLBACK_STENCIL        0x40
#define SAVAGE_FALLBACK_RENDERMODE     0x80
#define SAVAGE_FALLBACK_BLEND_EQ       0x100
#define SAVAGE_FALLBACK_NORAST         0x200
#define SAVAGE_FALLBACK_PROJ_TEXTURE   0x400


#define HW_CULL    1

/* for savagectx.new_state - manage GL->driver state changes
 */
#define SAVAGE_NEW_TEXTURE 0x1
#define SAVAGE_NEW_CULL    0x2

/* What needs to be changed for the current vertex dma buffer?
 * This will go away!
 */
#define SAVAGE_UPLOAD_LOCAL	0x1  /* DrawLocalCtrl (S4) or 
					DrawCtrl and ZBufCtrl (S3D) */
#define SAVAGE_UPLOAD_TEX0	0x2  /* texture unit 0 */
#define SAVAGE_UPLOAD_TEX1	0x4  /* texture unit 1 (S4 only) */
#define SAVAGE_UPLOAD_FOGTBL	0x8  /* fog table */
#define SAVAGE_UPLOAD_GLOBAL	0x10 /* most global regs */
#define SAVAGE_UPLOAD_TEXGLOBAL 0x20 /* TexBlendColor (S4 only) */

/*define the max numer of vertex in vertex buf*/
#define SAVAGE_MAX_VERTEXS 0x10000

/* Don't make it too big. We don't want to buffer up a whole frame
 * that would force the application to wait later. */
#define SAVAGE_CMDBUF_SIZE 1024

/* Use the templated vertex formats:
 */
#define TAG(x) savage##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*savage_tri_func)( savageContextPtr, savageVertex *,
				 savageVertex *, savageVertex * );
typedef void (*savage_line_func)( savageContextPtr,
				  savageVertex *, savageVertex * );
typedef void (*savage_point_func)( savageContextPtr, savageVertex * );


/**************************************************************
 ****************    enums for chip IDs ************************
 **************************************************************/

#define CHIP_S3GX3MS1NB             0x8A25
#define CHIP_S3GX3MS1NBK            0x8A26
#define CHIP_S3TWISTER              0x8D01
#define CHIP_S3TWISTERK             0x8D02
#define CHIP_S3TWISTER_P4M          0x8D04
#define CHIP_S3PARAMOUNT128         0x8C22              /*SuperSavage 128/MX*/
#define CHIP_S3TRISTAR128SDR        0x8C2A              /*SuperSavage 128/IX*/
#define CHIP_S3TRISTAR64SDRM7       0x8C2C              /*SuperSavage/IX M7 Package*/
#define CHIP_S3TRISTAR64SDR         0x8C2E              /*SuperSavage/IX*/
#define CHIP_S3TRISTAR64CDDR        0x8C2F              /*SuperSavage/IXC DDR*/

#define IS_SAVAGE(imesa) (imesa->savageScreen->deviceID == CHIP_S3GX3MS1NB ||	\
			imesa->savageScreen->deviceID == CHIP_S3GX3MS1NBK || \
                        imesa->savageScreen->deviceID == CHIP_S3TWISTER || \
                        imesa->savageScreen->deviceID == CHIP_S3TWISTERK || \
                        imesa->savageScreen->deviceID == CHIP_S3TWISTER_P4M || \
                        imesa->savageScreen->deviceID == CHIP_S3PARAMOUNT128 || \
                        imesa->savageScreen->deviceID == CHIP_S3TRISTAR128SDR || \
                        imesa->savageScreen->deviceID == CHIP_S3TRISTAR64SDRM7 || \
                        imesa->savageScreen->deviceID == CHIP_S3TRISTAR64SDR || \
			imesa->savageScreen->deviceID == CHIP_S3TRISTAR64CDDR )


struct savage_vtxbuf_t {
    GLuint total, used, flushed; /* in 32 bit units */
    GLuint idx;		/* for DMA buffers */
    u_int32_t *buf;
};

struct savage_cmdbuf_t {
    GLuint size; /* size in qwords */
    drm_savage_cmd_header_t *base;  /* initial state starts here */
    drm_savage_cmd_header_t *start; /* drawing/state commands start here */
    drm_savage_cmd_header_t *write; /* append stuff here */
};

struct savage_elt_t {
    GLuint n;				/* number of elts currently allocated */
    drm_savage_cmd_header_t *cmd;	/* the indexed drawing command */
};


struct savage_context_t {
    GLint refcount;

    GLcontext *glCtx;

    int lastTexHeap;
    driTexHeap *textureHeaps[SAVAGE_NR_TEX_HEAPS];
    driTextureObject swapped;

    driTextureObject *CurrentTexObj[2];

    /* Hardware state
     */

    savageRegisters regs, oldRegs, globalRegMask;

    /* Manage our own state */
    GLuint new_state; 
    GLuint new_gl_state;
    GLboolean ptexHack;

    /* Command buffer */
    struct savage_cmdbuf_t cmdBuf;

    /* Elt book-keeping */
    struct savage_elt_t elts;
    GLint firstElt;

    /* Vertex buffers */
    struct savage_vtxbuf_t dmaVtxBuf, clientVtxBuf;
    struct savage_vtxbuf_t *vtxBuf;

    /* aperture base */
    GLubyte *apertureBase[5];
    GLuint aperturePitch;
    /* Manage hardware state */
    GLuint dirty;
    GLboolean lostContext;
    GLuint bTexEn1;
    /* One of the few bits of hardware state that can't be calculated
     * completely on the fly:
     */
    GLuint LcsCullMode;
    GLuint texEnvColor;

   /* Vertex state 
    */
   GLuint vertex_size;
   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
   GLuint vertex_attr_count;
   char *verts;			/* points to tnl->clipspace.vertex_buf */

   /* Rasterization state 
    */
   GLuint SetupNewInputs;
   GLuint SetupIndex;
   GLuint RenderIndex;
   
   GLuint hw_primitive;
   GLenum raster_primitive;
   GLenum render_primitive;

   GLuint skip;
   GLubyte HwPrim;
   GLuint HwVertexSize;

   /* Fallback rasterization functions 
    */
   savage_point_func draw_point;
   savage_line_func draw_line;
   savage_tri_func draw_tri;

    /* Funny mesa mirrors
     */
    GLuint MonoColor;
    GLuint ClearColor;
    GLfloat depth_scale;
    GLfloat hw_viewport[16];
    /* DRI stuff */
    GLuint bufferSize;

    GLframebuffer *glBuffer;
   
    /* Two flags to keep track of fallbacks. */
    GLuint Fallback;

    GLuint needClip;

    /* These refer to the current draw (front vs. back) buffer:
     */
    int drawX;   		/* origin of drawable in draw buffer */
    int drawY;
    GLuint numClipRects;		/* cliprects for that buffer */
    GLint currentClip;
    drm_clip_rect_t *pClipRects;

    /*  use this bit to support single/double buffer */
    GLuint IsDouble;
    /*  use this to indicate Fullscreen mode */   
    GLuint IsFullScreen; /* FIXME - open/close fullscreen is gone, is this needed? */
    GLuint backup_frontOffset;
    GLuint backup_backOffset;
    GLuint backup_frontBitmapDesc;
    GLuint toggle;
    GLuint backup_streamFIFO;
    GLuint NotFirstFrame;
   
    GLboolean inSwap;
    GLuint lastSwap;
    GLuint ctxAge;
    GLuint dirtyAge;
    GLuint any_contend;		/* throttle me harder */

    /* Scissor state needs to be mirrored so buffered commands can be
     * emitted with the old scissor state when scissor state changes.
     */
    struct {
	GLboolean enabled;
	GLint x, y;
	GLsizei w, h;
    } scissor;

    drm_context_t hHWContext;
    drm_hw_lock_t *driHwLock;
    GLuint driFd;

    __DRIdrawablePrivate *driDrawable;
    __DRIdrawablePrivate *driReadable;

    __DRIscreenPrivate *driScreen;
    savageScreenPrivate *savageScreen; 
    drm_savage_sarea_t *sarea;

    GLboolean hw_stencil;

    /* Performance counters
     */
    GLuint c_textureSwaps;

    /* Configuration cache
     */
    driOptionCache optionCache;
    GLint texture_depth;
    GLboolean no_rast;
    GLboolean float_depth;
    GLboolean enable_fastpath;
    GLboolean enable_vdma;
    GLboolean sync_frames;
};

#define SAVAGE_CONTEXT(ctx) ((savageContextPtr)(ctx->DriverCtx))

/* To remove all debugging, make sure SAVAGE_DEBUG is defined as a
 * preprocessor symbol, and equal to zero.  
 */
#ifndef SAVAGE_DEBUG
extern int SAVAGE_DEBUG;
#endif

#define DEBUG_FALLBACKS      0x001
#define DEBUG_VERBOSE_API    0x002
#define DEBUG_VERBOSE_TEX    0x004
#define DEBUG_VERBOSE_MSG    0x008
#define DEBUG_DMA            0x010
#define DEBUG_STATE          0x020

#define TARGET_FRONT    0x0
#define TARGET_BACK     0x1
#define TARGET_DEPTH    0x2

#define SUBPIXEL_X -0.5
#define SUBPIXEL_Y -0.375

#endif
