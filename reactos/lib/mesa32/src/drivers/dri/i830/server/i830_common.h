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

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i830_common.h,v 1.2 2002/12/10 01:27:05 dawes Exp $ */

/* Author: Jeff Hartmann <jhartmann@valinux.com> 

   Converted to common header format:
     Jens Owen <jens@tungstengraphics.com>
 */

#ifndef _I830_COMMON_H_
#define _I830_COMMON_H_

/* WARNING: These defines must be the same as what the Xserver uses.
 * if you change them, you must change the defines in the Xserver.
 */

#ifndef _I830_DEFINES_
#define _I830_DEFINES_

#define I830_DMA_BUF_ORDER		12
#define I830_DMA_BUF_SZ 		(1<<I830_DMA_BUF_ORDER)
#define I830_DMA_BUF_NR 		256
#define I830_NR_SAREA_CLIPRECTS 	8

/* Each region is a minimum of 64k, and there are at most 64 of them.
 */
#define I830_NR_TEX_REGIONS 64
#define I830_LOG_MIN_TEX_REGION_SIZE 16

/* if defining I830_ENABLE_4_TEXTURES, do it in i830_3d_reg.h, too */
#if !defined(I830_ENABLE_4_TEXTURES)
#define I830_TEXTURE_COUNT	2
#define I830_TEXBLEND_COUNT	2	/* always same as TEXTURE_COUNT? */
#else /* defined(I830_ENABLE_4_TEXTURES) */
#define I830_TEXTURE_COUNT	4
#define I830_TEXBLEND_COUNT	4	/* always same as TEXTURE_COUNT? */
#endif /* I830_ENABLE_4_TEXTURES */

#define I830_TEXBLEND_SIZE	12	/* (4 args + op) * 2 + COLOR_FACTOR */

#define I830_UPLOAD_CTX			0x1
#define I830_UPLOAD_BUFFERS		0x2
#define I830_UPLOAD_CLIPRECTS		0x4
#define I830_UPLOAD_TEX0_IMAGE		0x100	/* handled clientside */
#define I830_UPLOAD_TEX0_CUBE		0x200	/* handled clientside */
#define I830_UPLOAD_TEX1_IMAGE		0x400	/* handled clientside */
#define I830_UPLOAD_TEX1_CUBE		0x800	/* handled clientside */
#define I830_UPLOAD_TEX2_IMAGE		0x1000	/* handled clientside */
#define I830_UPLOAD_TEX2_CUBE		0x2000	/* handled clientside */
#define I830_UPLOAD_TEX3_IMAGE		0x4000	/* handled clientside */
#define I830_UPLOAD_TEX3_CUBE		0x8000	/* handled clientside */
#define I830_UPLOAD_TEX_N_IMAGE(n)	(0x100 << (n * 2))
#define I830_UPLOAD_TEX_N_CUBE(n)	(0x200 << (n * 2))
#define I830_UPLOAD_TEXIMAGE_MASK	0xff00
#define I830_UPLOAD_TEX0			0x10000
#define I830_UPLOAD_TEX1			0x20000
#define I830_UPLOAD_TEX2			0x40000
#define I830_UPLOAD_TEX3			0x80000
#define I830_UPLOAD_TEX_N(n)		(0x10000 << (n))
#define I830_UPLOAD_TEX_MASK		0xf0000
#define I830_UPLOAD_TEXBLEND0		0x100000
#define I830_UPLOAD_TEXBLEND1		0x200000
#define I830_UPLOAD_TEXBLEND2		0x400000
#define I830_UPLOAD_TEXBLEND3		0x800000
#define I830_UPLOAD_TEXBLEND_N(n)	(0x100000 << (n))
#define I830_UPLOAD_TEXBLEND_MASK	0xf00000
#define I830_UPLOAD_TEX_PALETTE_N(n)    (0x1000000 << (n))
#define I830_UPLOAD_TEX_PALETTE_SHARED	0x4000000
#define I830_UPLOAD_STIPPLE         	0x8000000

/* Indices into buf.Setup where various bits of state are mirrored per
 * context and per buffer.  These can be fired at the card as a unit,
 * or in a piecewise fashion as required.
 */

/* Destbuffer state 
 *    - backbuffer linear offset and pitch -- invarient in the current dri
 *    - zbuffer linear offset and pitch -- also invarient
 *    - drawing origin in back and depth buffers.
 *
 * Keep the depth/back buffer state here to acommodate private buffers
 * in the future.
 */

#define I830_DESTREG_CBUFADDR 0
/* Invarient */
#define I830_DESTREG_DBUFADDR 1
#define I830_DESTREG_DV0 2
#define I830_DESTREG_DV1 3
#define I830_DESTREG_SENABLE 4
#define I830_DESTREG_SR0 5
#define I830_DESTREG_SR1 6
#define I830_DESTREG_SR2 7
#define I830_DESTREG_DR0 8
#define I830_DESTREG_DR1 9
#define I830_DESTREG_DR2 10
#define I830_DESTREG_DR3 11
#define I830_DESTREG_DR4 12
#define I830_DEST_SETUP_SIZE 13

/* Context state
 */
#define I830_CTXREG_STATE1		0
#define I830_CTXREG_STATE2		1
#define I830_CTXREG_STATE3		2
#define I830_CTXREG_STATE4		3
#define I830_CTXREG_STATE5		4
#define I830_CTXREG_IALPHAB		5
#define I830_CTXREG_STENCILTST		6
#define I830_CTXREG_ENABLES_1		7
#define I830_CTXREG_ENABLES_2		8
#define I830_CTXREG_AA			9
#define I830_CTXREG_FOGCOLOR		10
#define I830_CTXREG_BLENDCOLR0		11
#define I830_CTXREG_BLENDCOLR		12	/* Dword 1 of 2 dword command */
#define I830_CTXREG_VF			13
#define I830_CTXREG_VF2			14
#define I830_CTXREG_MCSB0		15
#define I830_CTXREG_MCSB1		16
#define I830_CTX_SETUP_SIZE		17

/* 1.3: Stipple state
 */ 
#define I830_STPREG_ST0 0
#define I830_STPREG_ST1 1
#define I830_STP_SETUP_SIZE 2

/* Texture state (per tex unit)
 */
#define I830_TEXREG_MI0	0		/* GFX_OP_MAP_INFO (6 dwords) */
#define I830_TEXREG_MI1	1
#define I830_TEXREG_MI2	2
#define I830_TEXREG_MI3	3
#define I830_TEXREG_MI4	4
#define I830_TEXREG_MI5	5
#define I830_TEXREG_MF	6		/* GFX_OP_MAP_FILTER */
#define I830_TEXREG_MLC	7		/* GFX_OP_MAP_LOD_CTL */
#define I830_TEXREG_MLL	8		/* GFX_OP_MAP_LOD_LIMITS */
#define I830_TEXREG_MCS	9		/* GFX_OP_MAP_COORD_SETS */
#define I830_TEX_SETUP_SIZE 10

/* New version.  Kernel auto-detects.
 */
#define I830_TEXREG_TM0LI 	0 /* load immediate 2 texture map n */
#define I830_TEXREG_TM0S0	1
#define I830_TEXREG_TM0S1	2
#define I830_TEXREG_TM0S2	3
#define I830_TEXREG_TM0S3	4
#define I830_TEXREG_TM0S4	5
#define I830_TEXREG_NOP0	6	/* noop */
#define I830_TEXREG_NOP1	7	/* noop */
#define I830_TEXREG_NOP2	8	/* noop */
#define __I830_TEXREG_MCS	9	/* GFX_OP_MAP_COORD_SETS -- shared */
#define __I830_TEX_SETUP_SIZE   10


#define I830_FRONT   0x1
#define I830_BACK    0x2
#define I830_DEPTH   0x4

/* Driver specific DRM command indices
 * NOTE: these are not OS specific, but they are driver specific
 */
#define DRM_I830_INIT                     0x00
#define DRM_I830_VERTEX                   0x01
#define DRM_I830_CLEAR                    0x02
#define DRM_I830_FLUSH                    0x03
#define DRM_I830_GETAGE                   0x04
#define DRM_I830_GETBUF                   0x05
#define DRM_I830_SWAP                     0x06
#define DRM_I830_COPY                     0x07
#define DRM_I830_DOCOPY                   0x08
#define DRM_I830_FLIP                     0x09
#define DRM_I830_IRQ_EMIT                 0x0a
#define DRM_I830_IRQ_WAIT                 0x0b
#define DRM_I830_GETPARAM                 0x0c
#define DRM_I830_SETPARAM                 0x0d

#endif /* _I830_DEFINES_ */

typedef struct {
   enum {
      I830_INIT_DMA = 0x01,
      I830_CLEANUP_DMA = 0x02
   } func;
   unsigned int mmio_offset;
   unsigned int buffers_offset;
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
} drmI830Init;

typedef struct {
   int clear_color;
   int clear_depth;
   int flags;
   unsigned int clear_colormask;
   unsigned int clear_depthmask;
} drmI830Clear;

/* These may be placeholders if we have more cliprects than
 * I830_NR_SAREA_CLIPRECTS.  In that case, the client sets discard to
 * false, indicating that the buffer will be dispatched again with a
 * new set of cliprects.
 */
typedef struct {
   int idx;				/* buffer index */
   int used;				/* nr bytes in use */
   int discard;				/* client is finished with the buffer? */
} drmI830Vertex;

typedef struct {
   int idx;				/* buffer index */
   int used;				/* nr bytes in use */
   void *address;			/* Address to copy from */
} drmI830Copy;

typedef struct {
   void *virtual;
   int request_idx;
   int request_size;
   int granted;
} drmI830DMA;

typedef struct drm_i830_irq_emit {
	int *irq_seq;
} drmI830IrqEmit;

typedef struct drm_i830_irq_wait {
	int irq_seq;
} drmI830IrqWait;

typedef struct drm_i830_getparam {
	int param;
	int *value;
} drmI830GetParam;

#define I830_PARAM_IRQ_ACTIVE  1


typedef struct drm_i830_setparam {
	int param;
	int value;
} drmI830SetParam;

#define I830_SETPARAM_USE_MI_BATCHBUFFER_START  1



#endif /* _I830_DRM_H_ */
