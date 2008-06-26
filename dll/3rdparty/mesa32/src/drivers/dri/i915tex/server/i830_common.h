/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.
Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i830_common.h,v 1.1 2002/09/11 00:29:32 dawes Exp $ */

#ifndef _I830_COMMON_H_
#define _I830_COMMON_H_


#define I830_NR_TEX_REGIONS 255	/* maximum due to use of chars for next/prev */
#define I830_LOG_MIN_TEX_REGION_SIZE 14


/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_I830_INIT                     0x00
#define DRM_I830_FLUSH                    0x01
#define DRM_I830_FLIP                     0x02
#define DRM_I830_BATCHBUFFER              0x03
#define DRM_I830_IRQ_EMIT                 0x04
#define DRM_I830_IRQ_WAIT                 0x05
#define DRM_I830_GETPARAM                 0x06
#define DRM_I830_SETPARAM                 0x07
#define DRM_I830_ALLOC                    0x08
#define DRM_I830_FREE                     0x09
#define DRM_I830_INIT_HEAP                0x0a
#define DRM_I830_CMDBUFFER                0x0b
#define DRM_I830_DESTROY_HEAP             0x0c
#define DRM_I830_SET_VBLANK_PIPE          0x0d
#define DRM_I830_GET_VBLANK_PIPE          0x0e

typedef struct {
   enum {
      I830_INIT_DMA = 0x01,
      I830_CLEANUP_DMA = 0x02,
      I830_RESUME_DMA = 0x03
   } func;
   unsigned int mmio_offset;
   int sarea_priv_offset;
   unsigned int ring_start;
   unsigned int ring_end;
   unsigned int ring_size;
   unsigned int front_offset;
   unsigned int back_offset;
   unsigned int depth_offset;
   unsigned int w;
   unsigned int h;
   unsigned int pitch;
   unsigned int pitch_bits;
   unsigned int back_pitch;
   unsigned int depth_pitch;
   unsigned int cpp;
   unsigned int chipset;
} drmI830Init;

typedef struct {
	drmTextureRegion texList[I830_NR_TEX_REGIONS+1];
        int last_upload;	/* last time texture was uploaded */
        int last_enqueue;	/* last time a buffer was enqueued */
	int last_dispatch;	/* age of the most recently dispatched buffer */
	int ctxOwner;		/* last context to upload state */
	int texAge;
        int pf_enabled;		/* is pageflipping allowed? */
        int pf_active;               
        int pf_current_page;	/* which buffer is being displayed? */
        int perf_boxes;	        /* performance boxes to be displayed */   
	int width, height;      /* screen size in pixels */

	drm_handle_t front_handle;
	int front_offset;
	int front_size;

	drm_handle_t back_handle;
	int back_offset;
	int back_size;

	drm_handle_t depth_handle;
	int depth_offset;
	int depth_size;

	drm_handle_t tex_handle;
	int tex_offset;
	int tex_size;
	int log_tex_granularity;
	int pitch;
	int rotation;           /* 0, 90, 180 or 270 */
	int rotated_offset;
	int rotated_size;
	int rotated_pitch;
	int virtualX, virtualY;

	unsigned int front_tiled;
	unsigned int back_tiled;
	unsigned int depth_tiled;
	unsigned int rotated_tiled;
	unsigned int rotated2_tiled;

	int pipeA_x;
	int pipeA_y;
	int pipeA_w;
	int pipeA_h;
	int pipeB_x;
	int pipeB_y;
	int pipeB_w;
	int pipeB_h;

	/* Triple buffering */
	drm_handle_t third_handle;
	int third_offset;
	int third_size;
	unsigned int third_tiled;
} drmI830Sarea;

/* Flags for perf_boxes
 */
#define I830_BOX_RING_EMPTY    0x1 /* populated by kernel */
#define I830_BOX_FLIP          0x2 /* populated by kernel */
#define I830_BOX_WAIT          0x4 /* populated by kernel & client */
#define I830_BOX_TEXTURE_LOAD  0x8 /* populated by kernel */
#define I830_BOX_LOST_CONTEXT  0x10 /* populated by client */


typedef struct {
   	int start;		/* agp offset */
	int used;		/* nr bytes in use */
	int DR1;		/* hw flags for GFX_OP_DRAWRECT_INFO */
        int DR4;		/* window origin for GFX_OP_DRAWRECT_INFO*/
	int num_cliprects;	/* mulitpass with multiple cliprects? */
        drm_clip_rect_t *cliprects; /* pointer to userspace cliprects */
} drmI830BatchBuffer;

typedef struct {
   	char *buf;		/* agp offset */
	int sz; 		/* nr bytes in use */
	int DR1;		/* hw flags for GFX_OP_DRAWRECT_INFO */
        int DR4;		/* window origin for GFX_OP_DRAWRECT_INFO*/
	int num_cliprects;	/* mulitpass with multiple cliprects? */
        drm_clip_rect_t *cliprects; /* pointer to userspace cliprects */
} drmI830CmdBuffer;
 
typedef struct {
	int *irq_seq;
} drmI830IrqEmit;

typedef struct {
	int irq_seq;
} drmI830IrqWait;

typedef struct {
	int param;
	int *value;
} drmI830GetParam;

#define I830_PARAM_IRQ_ACTIVE     1
#define I830_PARAM_ALLOW_BATCHBUFFER   2 

typedef struct {
	int param;
	int value;
} drmI830SetParam;

#define I830_SETPARAM_USE_MI_BATCHBUFFER_START  1
#define I830_SETPARAM_TEX_LRU_LOG_GRANULARITY   2
#define I830_SETPARAM_ALLOW_BATCHBUFFER         3


/* A memory manager for regions of shared memory:
 */
#define I830_MEM_REGION_AGP 1

typedef struct {
	int region;
	int alignment;
	int size;
	int *region_offset;	/* offset from start of fb or agp */
} drmI830MemAlloc;

typedef struct {
	int region;
	int region_offset;
} drmI830MemFree;

typedef struct {
	int region;
	int size;
	int start;	
} drmI830MemInitHeap;

typedef struct {
	int region;
} drmI830MemDestroyHeap;

#define DRM_I830_VBLANK_PIPE_A  1
#define DRM_I830_VBLANK_PIPE_B  2

typedef struct {
        int pipe;
} drmI830VBlankPipe;

#endif /* _I830_DRM_H_ */
