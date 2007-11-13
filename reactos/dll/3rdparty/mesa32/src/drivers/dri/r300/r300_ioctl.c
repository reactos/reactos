/*
Copyright (C) The Weather Channel, Inc.  2002.
Copyright (C) 2004 Nicolai Haehnle.
All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#include <sched.h>
#include <errno.h>

#include "glheader.h"
#include "imports.h"
#include "macros.h"
#include "context.h"
#include "swrast/swrast.h"

#include "r300_context.h"
#include "radeon_ioctl.h"
#include "r300_ioctl.h"
#include "r300_cmdbuf.h"
#include "r300_state.h"
#include "r300_program.h"
#include "radeon_reg.h"
#include "r300_emit.h"

#include "vblank.h"

#define CLEARBUFFER_COLOR	0x1
#define CLEARBUFFER_DEPTH	0x2
#define CLEARBUFFER_STENCIL	0x4

static void r300ClearBuffer(r300ContextPtr r300, int flags, int buffer)
{
	GLcontext *ctx = r300->radeon.glCtx;
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	GLuint cboffset, cbpitch;
	drm_r300_cmd_header_t *cmd2;
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;
	r300ContextPtr rmesa = r300;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s: %s buffer (%i,%i %ix%i)\n",
			__FUNCTION__, buffer ? "back" : "front",
			dPriv->x, dPriv->y, dPriv->w, dPriv->h);

	if (buffer) {
		cboffset = r300->radeon.radeonScreen->backOffset;
		cbpitch = r300->radeon.radeonScreen->backPitch;
	} else {
		cboffset = r300->radeon.radeonScreen->frontOffset;
		cbpitch = r300->radeon.radeonScreen->frontPitch;
	}

	cboffset += r300->radeon.radeonScreen->fbLocation;

	cp_wait(r300, R300_WAIT_3D | R300_WAIT_3D_CLEAN);
	end_3d(rmesa);

	R300_STATECHANGE(r300, cb);
	reg_start(R300_RB3D_COLOROFFSET0, 0);
	e32(cboffset);

	if (r300->radeon.radeonScreen->cpp == 4)
		cbpitch |= R300_COLOR_FORMAT_ARGB8888;
	else
		cbpitch |= R300_COLOR_FORMAT_RGB565;

	if (r300->radeon.sarea->tiling_enabled)
		cbpitch |= R300_COLOR_TILE_ENABLE;

	reg_start(R300_RB3D_COLORPITCH0, 0);
	e32(cbpitch);

	R300_STATECHANGE(r300, cmk);
	reg_start(R300_RB3D_COLORMASK, 0);

	if (flags & CLEARBUFFER_COLOR) {
		e32((ctx->Color.ColorMask[BCOMP] ? R300_COLORMASK0_B : 0) |
		    (ctx->Color.ColorMask[GCOMP] ? R300_COLORMASK0_G : 0) |
		    (ctx->Color.ColorMask[RCOMP] ? R300_COLORMASK0_R : 0) |
		    (ctx->Color.ColorMask[ACOMP] ? R300_COLORMASK0_A : 0));
	} else {
		e32(0x0);
	}

	R300_STATECHANGE(r300, zs);
	reg_start(R300_RB3D_ZSTENCIL_CNTL_0, 2);

	{
		uint32_t t1, t2;

		t1 = 0x0;
		t2 = 0x0;

		if (flags & CLEARBUFFER_DEPTH) {
			t1 |= R300_RB3D_Z_WRITE_ONLY;
			t2 |=
			    (R300_ZS_ALWAYS << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);
		} else {
			t1 |= R300_RB3D_Z_DISABLED_1;	// disable
		}

		if (flags & CLEARBUFFER_STENCIL) {
			t1 |= R300_RB3D_STENCIL_ENABLE;
			t2 |=
			    (R300_ZS_ALWAYS <<
			     R300_RB3D_ZS1_FRONT_FUNC_SHIFT) |
			    (R300_ZS_REPLACE <<
			     R300_RB3D_ZS1_FRONT_FAIL_OP_SHIFT) |
			    (R300_ZS_REPLACE <<
			     R300_RB3D_ZS1_FRONT_ZPASS_OP_SHIFT) |
			    (R300_ZS_REPLACE <<
			     R300_RB3D_ZS1_FRONT_ZFAIL_OP_SHIFT) |
			    (R300_ZS_ALWAYS <<
			     R300_RB3D_ZS1_BACK_FUNC_SHIFT) |
			    (R300_ZS_REPLACE <<
			     R300_RB3D_ZS1_BACK_FAIL_OP_SHIFT) |
			    (R300_ZS_REPLACE <<
			     R300_RB3D_ZS1_BACK_ZPASS_OP_SHIFT) |
			    (R300_ZS_REPLACE <<
			     R300_RB3D_ZS1_BACK_ZFAIL_OP_SHIFT);
		}

		e32(t1);
		e32(t2);
		e32(r300->state.stencil.clear);
	}

	cmd2 = (drm_r300_cmd_header_t *) r300AllocCmdBuf(r300, 9, __FUNCTION__);
	cmd2[0].packet3.cmd_type = R300_CMD_PACKET3;
	cmd2[0].packet3.packet = R300_CMD_PACKET3_CLEAR;
	cmd2[1].u = r300PackFloat32(dPriv->w / 2.0);
	cmd2[2].u = r300PackFloat32(dPriv->h / 2.0);
	cmd2[3].u = r300PackFloat32(ctx->Depth.Clear);
	cmd2[4].u = r300PackFloat32(1.0);
	cmd2[5].u = r300PackFloat32(ctx->Color.ClearColor[0]);
	cmd2[6].u = r300PackFloat32(ctx->Color.ClearColor[1]);
	cmd2[7].u = r300PackFloat32(ctx->Color.ClearColor[2]);
	cmd2[8].u = r300PackFloat32(ctx->Color.ClearColor[3]);

	reg_start(R300_RB3D_DSTCACHE_CTLSTAT, 0);
	e32(R300_RB3D_DSTCACHE_UNKNOWN_0A);

	reg_start(R300_RB3D_ZCACHE_CTLSTAT, 0);
	e32(R300_RB3D_ZCACHE_UNKNOWN_03);
	cp_wait(rmesa, R300_WAIT_3D | R300_WAIT_3D_CLEAN);
}

static void r300EmitClearState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	r300ContextPtr rmesa = r300;
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	int i;
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;
	int has_tcl = 1;

	if (!(r300->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL))
		has_tcl = 0;

	/* FIXME: the values written to R300_VAP_INPUT_ROUTE_0_0 and
	 * R300_VAP_INPUT_ROUTE_0_1 are in fact known, however, the values are
	 * quite complex; see the functions in r300_emit.c.
	 *
	 * I believe it would be a good idea to extend the functions in
	 * r300_emit.c so that they can be used to setup the default values for
	 * these registers, as well as the actual values used for rendering.
	 */
	R300_STATECHANGE(r300, vir[0]);
	reg_start(R300_VAP_INPUT_ROUTE_0_0, 0);
	if (!has_tcl)
		e32(0x22030003);
	else
		e32(0x21030003);

	/* disable fog */
	R300_STATECHANGE(r300, fogs);
	reg_start(R300_RE_FOG_STATE, 0);
	e32(0x0);

	R300_STATECHANGE(r300, vir[1]);
	reg_start(R300_VAP_INPUT_ROUTE_1_0, 0);
	e32(0xF688F688);

	/* R300_VAP_INPUT_CNTL_0, R300_VAP_INPUT_CNTL_1 */
	R300_STATECHANGE(r300, vic);
	reg_start(R300_VAP_INPUT_CNTL_0, 1);
	e32(R300_INPUT_CNTL_0_COLOR);
	e32(R300_INPUT_CNTL_POS | R300_INPUT_CNTL_COLOR | R300_INPUT_CNTL_TC0);

	if (!has_tcl) {
		R300_STATECHANGE(r300, vte);
		/* comes from fglrx startup of clear */
		reg_start(R300_SE_VTE_CNTL, 1);
		e32(R300_VTX_W0_FMT | R300_VPORT_X_SCALE_ENA |
		    R300_VPORT_X_OFFSET_ENA | R300_VPORT_Y_SCALE_ENA |
		    R300_VPORT_Y_OFFSET_ENA | R300_VPORT_Z_SCALE_ENA |
		    R300_VPORT_Z_OFFSET_ENA);
		e32(0x8);

		reg_start(0x21dc, 0);
		e32(0xaaaaaaaa);
	}

	R300_STATECHANGE(r300, vof);
	reg_start(R300_VAP_OUTPUT_VTX_FMT_0, 1);
	e32(R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT |
	    R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT);
	e32(0x0);			/* no textures */

	R300_STATECHANGE(r300, txe);
	reg_start(R300_TX_ENABLE, 0);
	e32(0x0);

	R300_STATECHANGE(r300, vpt);
	reg_start(R300_SE_VPORT_XSCALE, 5);
	efloat(1.0);
	efloat(dPriv->x);
	efloat(1.0);
	efloat(dPriv->y);
	efloat(1.0);
	efloat(0.0);

	R300_STATECHANGE(r300, at);
	reg_start(R300_PP_ALPHA_TEST, 0);
	e32(0x0);

	R300_STATECHANGE(r300, bld);
	reg_start(R300_RB3D_CBLEND, 1);
	e32(0x0);
	e32(0x0);

	R300_STATECHANGE(r300, unk221C);
	reg_start(R300_VAP_UNKNOWN_221C, 0);
	e32(R300_221C_CLEAR);

	R300_STATECHANGE(r300, ps);
	reg_start(R300_RE_POINTSIZE, 0);
	e32(((dPriv->w * 6) << R300_POINTSIZE_X_SHIFT) |
	    ((dPriv->h * 6) << R300_POINTSIZE_Y_SHIFT));

	R300_STATECHANGE(r300, ri);
	reg_start(R300_RS_INTERP_0, 8);
	for (i = 0; i < 8; ++i) {
		e32(R300_RS_INTERP_USED);
	}

	R300_STATECHANGE(r300, rc);
	/* The second constant is needed to get glxgears display anything .. */
	reg_start(R300_RS_CNTL_0, 1);
	e32((1 << R300_RS_CNTL_CI_CNT_SHIFT) | R300_RS_CNTL_0_UNKNOWN_18);
	e32(0x0);

	R300_STATECHANGE(r300, rr);
	reg_start(R300_RS_ROUTE_0, 0);
	e32(R300_RS_ROUTE_0_COLOR);

	R300_STATECHANGE(r300, fp);
	reg_start(R300_PFS_CNTL_0, 2);
	e32(0x0);
	e32(0x0);
	e32(0x0);
	reg_start(R300_PFS_NODE_0, 3);
	e32(0x0);
	e32(0x0);
	e32(0x0);
	e32(R300_PFS_NODE_OUTPUT_COLOR);

	R300_STATECHANGE(r300, fpi[0]);
	R300_STATECHANGE(r300, fpi[1]);
	R300_STATECHANGE(r300, fpi[2]);
	R300_STATECHANGE(r300, fpi[3]);

	reg_start(R300_PFS_INSTR0_0, 0);
	e32(FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO)));

	reg_start(R300_PFS_INSTR1_0, 0);
	e32(FP_SELC(0, NO, XYZ, FP_TMP(0), 0, 0));

	reg_start(R300_PFS_INSTR2_0, 0);
	e32(FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO)));

	reg_start(R300_PFS_INSTR3_0, 0);
	e32(FP_SELA(0, NO, W, FP_TMP(0), 0, 0));

	if (has_tcl) {
		R300_STATECHANGE(r300, pvs);
		reg_start(R300_VAP_PVS_CNTL_1, 2);
		e32((0 << R300_PVS_CNTL_1_PROGRAM_START_SHIFT) |
		    (0 << R300_PVS_CNTL_1_POS_END_SHIFT) |
		    (1 << R300_PVS_CNTL_1_PROGRAM_END_SHIFT));
		e32(0x0);
		e32(1 << R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT);

		R300_STATECHANGE(r300, vpi);
		vsf_start_fragment(0x0, 8);
		e32(VP_OUT(ADD, OUT, 0, XYZW));
		e32(VP_IN(IN, 0));
		e32(VP_ZERO());
		e32(0x0);

		e32(VP_OUT(ADD, OUT, 1, XYZW));
		e32(VP_IN(IN, 1));
		e32(VP_ZERO());
		e32(0x0);
	}
}

/**
 * Buffer clear
 */
static void r300Clear(GLcontext * ctx, GLbitfield mask)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	int flags = 0;
	int bits = 0;
	int swapped;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "r300Clear\n");

	{
		LOCK_HARDWARE(&r300->radeon);
		UNLOCK_HARDWARE(&r300->radeon);
		if (dPriv->numClipRects == 0)
			return;
	}

	if (mask & BUFFER_BIT_FRONT_LEFT) {
		flags |= BUFFER_BIT_FRONT_LEFT;
		mask &= ~BUFFER_BIT_FRONT_LEFT;
	}

	if (mask & BUFFER_BIT_BACK_LEFT) {
		flags |= BUFFER_BIT_BACK_LEFT;
		mask &= ~BUFFER_BIT_BACK_LEFT;
	}

	if (mask & BUFFER_BIT_DEPTH) {
		bits |= CLEARBUFFER_DEPTH;
		mask &= ~BUFFER_BIT_DEPTH;
	}

	if ((mask & BUFFER_BIT_STENCIL) && r300->state.stencil.hw_stencil) {
		bits |= CLEARBUFFER_STENCIL;
		mask &= ~BUFFER_BIT_STENCIL;
	}

	if (mask) {
		if (RADEON_DEBUG & DEBUG_FALLBACKS)
			fprintf(stderr, "%s: swrast clear, mask: %x\n",
				__FUNCTION__, mask);
		_swrast_Clear(ctx, mask);
	}

	swapped = r300->radeon.sarea->pfCurrentPage == 1;

	/* Make sure it fits there. */
	r300EnsureCmdBufSpace(r300, 421 * 3, __FUNCTION__);
	if (flags || bits)
		r300EmitClearState(ctx);

	if (flags & BUFFER_BIT_FRONT_LEFT) {
		r300ClearBuffer(r300, bits | CLEARBUFFER_COLOR, swapped);
		bits = 0;
	}

	if (flags & BUFFER_BIT_BACK_LEFT) {
		r300ClearBuffer(r300, bits | CLEARBUFFER_COLOR, swapped ^ 1);
		bits = 0;
	}

	if (bits)
		r300ClearBuffer(r300, bits, 0);

}

void r300Flush(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300->cmdbuf.count_used > r300->cmdbuf.count_reemit)
		r300FlushCmdBuf(r300, __FUNCTION__);
}

#ifdef USER_BUFFERS
#include "r300_mem.h"

static void r300RefillCurrentDmaRegion(r300ContextPtr rmesa, int size)
{
	struct r300_dma_buffer *dmabuf;
	size = MAX2(size, RADEON_BUFFER_SIZE * 16);

	if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA))
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (rmesa->dma.flush) {
		rmesa->dma.flush(rmesa);
	}

	if (rmesa->dma.current.buf)
		r300ReleaseDmaRegion(rmesa, &rmesa->dma.current, __FUNCTION__);

	if (rmesa->dma.nr_released_bufs > 4)
		r300FlushCmdBuf(rmesa, __FUNCTION__);

	dmabuf = CALLOC_STRUCT(r300_dma_buffer);
	dmabuf->buf = (void *)1;	/* hack */
	dmabuf->refcount = 1;

	dmabuf->id = r300_mem_alloc(rmesa, 4, size);
	if (dmabuf->id == 0) {
		LOCK_HARDWARE(&rmesa->radeon);	/* no need to validate */

		r300FlushCmdBufLocked(rmesa, __FUNCTION__);
		radeonWaitForIdleLocked(&rmesa->radeon);

		dmabuf->id = r300_mem_alloc(rmesa, 4, size);

		UNLOCK_HARDWARE(&rmesa->radeon);

		if (dmabuf->id == 0) {
			fprintf(stderr,
				"Error: Could not get dma buffer... exiting\n");
			_mesa_exit(-1);
		}
	}

	rmesa->dma.current.buf = dmabuf;
	rmesa->dma.current.address = r300_mem_ptr(rmesa, dmabuf->id);
	rmesa->dma.current.end = size;
	rmesa->dma.current.start = 0;
	rmesa->dma.current.ptr = 0;
}

void r300ReleaseDmaRegion(r300ContextPtr rmesa,
			  struct r300_dma_region *region, const char *caller)
{
	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s from %s\n", __FUNCTION__, caller);

	if (!region->buf)
		return;

	if (rmesa->dma.flush)
		rmesa->dma.flush(rmesa);

	if (--region->buf->refcount == 0) {
		r300_mem_free(rmesa, region->buf->id);
		FREE(region->buf);
		rmesa->dma.nr_released_bufs++;
	}

	region->buf = 0;
	region->start = 0;
}

/* Allocates a region from rmesa->dma.current.  If there isn't enough
 * space in current, grab a new buffer (and discard what was left of current)
 */
void r300AllocDmaRegion(r300ContextPtr rmesa,
			struct r300_dma_region *region,
			int bytes, int alignment)
{
	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, bytes);

	if (rmesa->dma.flush)
		rmesa->dma.flush(rmesa);

	if (region->buf)
		r300ReleaseDmaRegion(rmesa, region, __FUNCTION__);

	alignment--;
	rmesa->dma.current.start = rmesa->dma.current.ptr =
	    (rmesa->dma.current.ptr + alignment) & ~alignment;

	if (rmesa->dma.current.ptr + bytes > rmesa->dma.current.end)
		r300RefillCurrentDmaRegion(rmesa, (bytes + 0x7) & ~0x7);

	region->start = rmesa->dma.current.start;
	region->ptr = rmesa->dma.current.start;
	region->end = rmesa->dma.current.start + bytes;
	region->address = rmesa->dma.current.address;
	region->buf = rmesa->dma.current.buf;
	region->buf->refcount++;

	rmesa->dma.current.ptr += bytes;	/* bug - if alignment > 7 */
	rmesa->dma.current.start =
	    rmesa->dma.current.ptr = (rmesa->dma.current.ptr + 0x7) & ~0x7;

	assert(rmesa->dma.current.ptr <= rmesa->dma.current.end);
}

#else
static void r300RefillCurrentDmaRegion(r300ContextPtr rmesa)
{
	struct r300_dma_buffer *dmabuf;
	int fd = rmesa->radeon.dri.fd;
	int index = 0;
	int size = 0;
	drmDMAReq dma;
	int ret;

	if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA))
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (rmesa->dma.flush) {
		rmesa->dma.flush(rmesa);
	}

	if (rmesa->dma.current.buf)
		r300ReleaseDmaRegion(rmesa, &rmesa->dma.current, __FUNCTION__);

	if (rmesa->dma.nr_released_bufs > 4)
		r300FlushCmdBuf(rmesa, __FUNCTION__);

	dma.context = rmesa->radeon.dri.hwContext;
	dma.send_count = 0;
	dma.send_list = NULL;
	dma.send_sizes = NULL;
	dma.flags = 0;
	dma.request_count = 1;
	dma.request_size = RADEON_BUFFER_SIZE;
	dma.request_list = &index;
	dma.request_sizes = &size;
	dma.granted_count = 0;

	LOCK_HARDWARE(&rmesa->radeon);	/* no need to validate */

	ret = drmDMA(fd, &dma);

	if (ret != 0) {
		/* Try to release some buffers and wait until we can't get any more */
		if (rmesa->dma.nr_released_bufs) {
			r300FlushCmdBufLocked(rmesa, __FUNCTION__);
		}

		if (RADEON_DEBUG & DEBUG_DMA)
			fprintf(stderr, "Waiting for buffers\n");

		radeonWaitForIdleLocked(&rmesa->radeon);
		ret = drmDMA(fd, &dma);

		if (ret != 0) {
			UNLOCK_HARDWARE(&rmesa->radeon);
			fprintf(stderr,
				"Error: Could not get dma buffer... exiting\n");
			_mesa_exit(-1);
		}
	}

	UNLOCK_HARDWARE(&rmesa->radeon);

	if (RADEON_DEBUG & DEBUG_DMA)
		fprintf(stderr, "Allocated buffer %d\n", index);

	dmabuf = CALLOC_STRUCT(r300_dma_buffer);
	dmabuf->buf = &rmesa->radeon.radeonScreen->buffers->list[index];
	dmabuf->refcount = 1;

	rmesa->dma.current.buf = dmabuf;
	rmesa->dma.current.address = dmabuf->buf->address;
	rmesa->dma.current.end = dmabuf->buf->total;
	rmesa->dma.current.start = 0;
	rmesa->dma.current.ptr = 0;
}

void r300ReleaseDmaRegion(r300ContextPtr rmesa,
			  struct r300_dma_region *region, const char *caller)
{
	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s from %s\n", __FUNCTION__, caller);

	if (!region->buf)
		return;

	if (rmesa->dma.flush)
		rmesa->dma.flush(rmesa);

	if (--region->buf->refcount == 0) {
		drm_radeon_cmd_header_t *cmd;

		if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA))
			fprintf(stderr, "%s -- DISCARD BUF %d\n",
				__FUNCTION__, region->buf->buf->idx);
		cmd =
		    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa,
								sizeof
								(*cmd) / 4,
								__FUNCTION__);
		cmd->dma.cmd_type = R300_CMD_DMA_DISCARD;
		cmd->dma.buf_idx = region->buf->buf->idx;

		FREE(region->buf);
		rmesa->dma.nr_released_bufs++;
	}

	region->buf = 0;
	region->start = 0;
}

/* Allocates a region from rmesa->dma.current.  If there isn't enough
 * space in current, grab a new buffer (and discard what was left of current)
 */
void r300AllocDmaRegion(r300ContextPtr rmesa,
			struct r300_dma_region *region,
			int bytes, int alignment)
{
	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s %d\n", __FUNCTION__, bytes);

	if (rmesa->dma.flush)
		rmesa->dma.flush(rmesa);

	if (region->buf)
		r300ReleaseDmaRegion(rmesa, region, __FUNCTION__);

	alignment--;
	rmesa->dma.current.start = rmesa->dma.current.ptr =
	    (rmesa->dma.current.ptr + alignment) & ~alignment;

	if (rmesa->dma.current.ptr + bytes > rmesa->dma.current.end)
		r300RefillCurrentDmaRegion(rmesa);

	region->start = rmesa->dma.current.start;
	region->ptr = rmesa->dma.current.start;
	region->end = rmesa->dma.current.start + bytes;
	region->address = rmesa->dma.current.address;
	region->buf = rmesa->dma.current.buf;
	region->buf->refcount++;

	rmesa->dma.current.ptr += bytes;	/* bug - if alignment > 7 */
	rmesa->dma.current.start =
	    rmesa->dma.current.ptr = (rmesa->dma.current.ptr + 0x7) & ~0x7;

	assert(rmesa->dma.current.ptr <= rmesa->dma.current.end);
}

#endif

GLboolean r300IsGartMemory(r300ContextPtr rmesa, const GLvoid * pointer,
			   GLint size)
{
	int offset =
	    (char *)pointer -
	    (char *)rmesa->radeon.radeonScreen->gartTextures.map;
	int valid = (size >= 0 && offset >= 0
		     && offset + size <
		     rmesa->radeon.radeonScreen->gartTextures.size);

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "r300IsGartMemory( %p ) : %d\n", pointer,
			valid);

	return valid;
}

GLuint r300GartOffsetFromVirtual(r300ContextPtr rmesa, const GLvoid * pointer)
{
	int offset =
	    (char *)pointer -
	    (char *)rmesa->radeon.radeonScreen->gartTextures.map;

	//fprintf(stderr, "offset=%08x\n", offset);

	if (offset < 0
	    || offset > rmesa->radeon.radeonScreen->gartTextures.size)
		return ~0;
	else
		return rmesa->radeon.radeonScreen->gart_texture_offset + offset;
}

void r300InitIoctlFuncs(struct dd_function_table *functions)
{
	functions->Clear = r300Clear;
	functions->Finish = radeonFinish;
	functions->Flush = r300Flush;
}
