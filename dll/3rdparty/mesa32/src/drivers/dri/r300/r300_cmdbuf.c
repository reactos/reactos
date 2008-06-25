/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

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
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#include "glheader.h"
#include "state.h"
#include "imports.h"
#include "macros.h"
#include "context.h"
#include "swrast/swrast.h"
#include "simple_list.h"

#include "drm.h"
#include "radeon_drm.h"

#include "radeon_ioctl.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "radeon_reg.h"
#include "r300_reg.h"
#include "r300_cmdbuf.h"
#include "r300_emit.h"
#include "r300_state.h"

// Set this to 1 for extremely verbose debugging of command buffers
#define DEBUG_CMDBUF		0

/**
 * Send the current command buffer via ioctl to the hardware.
 */
int r300FlushCmdBufLocked(r300ContextPtr r300, const char *caller)
{
	int ret;
	int i;
	drm_radeon_cmd_buffer_t cmd;
	int start;

	if (r300->radeon.lost_context) {
		start = 0;
		r300->radeon.lost_context = GL_FALSE;
	} else
		start = r300->cmdbuf.count_reemit;

	if (RADEON_DEBUG & DEBUG_IOCTL) {
		fprintf(stderr, "%s from %s - %i cliprects\n",
			__FUNCTION__, caller, r300->radeon.numClipRects);

		if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_VERBOSE)
			for (i = start; i < r300->cmdbuf.count_used; ++i)
				fprintf(stderr, "%d: %08x\n", i,
					r300->cmdbuf.cmd_buf[i]);
	}

	cmd.buf = (char *)(r300->cmdbuf.cmd_buf + start);
	cmd.bufsz = (r300->cmdbuf.count_used - start) * 4;

	if (r300->radeon.state.scissor.enabled) {
		cmd.nbox = r300->radeon.state.scissor.numClipRects;
		cmd.boxes =
		    (drm_clip_rect_t *) r300->radeon.state.scissor.pClipRects;
	} else {
		cmd.nbox = r300->radeon.numClipRects;
		cmd.boxes = (drm_clip_rect_t *) r300->radeon.pClipRects;
	}

	ret = drmCommandWrite(r300->radeon.dri.fd,
			      DRM_RADEON_CMDBUF, &cmd, sizeof(cmd));

	if (RADEON_DEBUG & DEBUG_SYNC) {
		fprintf(stderr, "Syncing in %s (from %s)\n\n",
			__FUNCTION__, caller);
		radeonWaitForIdleLocked(&r300->radeon);
	}

	r300->dma.nr_released_bufs = 0;
	r300->cmdbuf.count_used = 0;
	r300->cmdbuf.count_reemit = 0;

	return ret;
}

int r300FlushCmdBuf(r300ContextPtr r300, const char *caller)
{
	int ret;

	LOCK_HARDWARE(&r300->radeon);

	ret = r300FlushCmdBufLocked(r300, caller);

	UNLOCK_HARDWARE(&r300->radeon);

	if (ret) {
		fprintf(stderr, "drmRadeonCmdBuffer: %d\n", ret);
		_mesa_exit(ret);
	}

	return ret;
}

static void r300PrintStateAtom(r300ContextPtr r300, struct r300_state_atom *state)
{
	int i;
	int dwords = (*state->check) (r300, state);

	fprintf(stderr, "  emit %s/%d/%d\n", state->name, dwords,
		state->cmd_size);

	if (RADEON_DEBUG & DEBUG_VERBOSE)
		for (i = 0; i < dwords; i++)
			fprintf(stderr, "      %s[%d]: %08X\n",
				state->name, i, state->cmd[i]);
}

/**
 * Emit all atoms with a dirty field equal to dirty.
 *
 * The caller must have ensured that there is enough space in the command
 * buffer.
 */
static __inline__ void r300EmitAtoms(r300ContextPtr r300, GLboolean dirty)
{
	struct r300_state_atom *atom;
	uint32_t *dest;

	dest = r300->cmdbuf.cmd_buf + r300->cmdbuf.count_used;

	if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
		foreach(atom, &r300->hw.atomlist) {
			if ((atom->dirty || r300->hw.all_dirty) == dirty) {
				int dwords = (*atom->check) (r300, atom);

				if (dwords)
					r300PrintStateAtom(r300, atom);
				else
					fprintf(stderr,
						"  skip state %s\n",
						atom->name);
			}
		}
	}

	/* Emit WAIT */
	*dest = cmdwait(R300_WAIT_3D | R300_WAIT_3D_CLEAN);
	dest++;
	r300->cmdbuf.count_used++;

	/* Emit cache flush */
	*dest = cmdpacket0(R300_TX_CNTL, 1);
	dest++;
	r300->cmdbuf.count_used++;

	*dest = R300_TX_FLUSH;
	dest++;
	r300->cmdbuf.count_used++;

	/* Emit END3D */
	*dest = cmdpacify();
	dest++;
	r300->cmdbuf.count_used++;

	/* Emit actual atoms */

	foreach(atom, &r300->hw.atomlist) {
		if ((atom->dirty || r300->hw.all_dirty) == dirty) {
			int dwords = (*atom->check) (r300, atom);

			if (dwords) {
				memcpy(dest, atom->cmd, dwords * 4);
				dest += dwords;
				r300->cmdbuf.count_used += dwords;
				atom->dirty = GL_FALSE;
			}
		}
	}
}

/**
 * Copy dirty hardware state atoms into the command buffer.
 *
 * We also copy out clean state if we're at the start of a buffer. That makes
 * it easy to recover from lost contexts.
 */
void r300EmitState(r300ContextPtr r300)
{
	if (RADEON_DEBUG & (DEBUG_STATE | DEBUG_PRIMS))
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300->cmdbuf.count_used && !r300->hw.is_dirty
	    && !r300->hw.all_dirty)
		return;

	/* To avoid going across the entire set of states multiple times, just check
	 * for enough space for the case of emitting all state, and inline the
	 * r300AllocCmdBuf code here without all the checks.
	 */
	r300EnsureCmdBufSpace(r300, r300->hw.max_state_size, __FUNCTION__);

	if (!r300->cmdbuf.count_used) {
		if (RADEON_DEBUG & DEBUG_STATE)
			fprintf(stderr, "Begin reemit state\n");

		r300EmitAtoms(r300, GL_FALSE);
		r300->cmdbuf.count_reemit = r300->cmdbuf.count_used;
	}

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "Begin dirty state\n");

	r300EmitAtoms(r300, GL_TRUE);

	assert(r300->cmdbuf.count_used < r300->cmdbuf.size);

	r300->hw.is_dirty = GL_FALSE;
	r300->hw.all_dirty = GL_FALSE;
}

#define CHECK( NM, COUNT )				\
static int check_##NM( r300ContextPtr r300, 		\
			struct r300_state_atom* atom )	\
{							\
   (void) atom;	(void) r300;				\
   return (COUNT);					\
}

#define packet0_count(ptr) (((drm_r300_cmd_header_t*)(ptr))->packet0.count)
#define vpu_count(ptr) (((drm_r300_cmd_header_t*)(ptr))->vpu.count)

CHECK(always, atom->cmd_size)
    CHECK(variable, packet0_count(atom->cmd) ? (1 + packet0_count(atom->cmd)) : 0)
    CHECK(vpu, vpu_count(atom->cmd) ? (1 + vpu_count(atom->cmd) * 4) : 0)
#undef packet0_count
#undef vpu_count
#define ALLOC_STATE( ATOM, CHK, SZ, IDX )				\
   do {									\
      r300->hw.ATOM.cmd_size = (SZ);					\
      r300->hw.ATOM.cmd = (uint32_t*)CALLOC((SZ) * sizeof(uint32_t));	\
      r300->hw.ATOM.name = #ATOM;					\
      r300->hw.ATOM.idx = (IDX);					\
      r300->hw.ATOM.check = check_##CHK;				\
      r300->hw.ATOM.dirty = GL_FALSE;					\
      r300->hw.max_state_size += (SZ);					\
      insert_at_tail(&r300->hw.atomlist, &r300->hw.ATOM);		\
   } while (0)
/**
 * Allocate memory for the command buffer and initialize the state atom
 * list. Note that the initial hardware state is set by r300InitState().
 */
void r300InitCmdBuf(r300ContextPtr r300)
{
	int size, mtu;
	int has_tcl = 1;

	if (!(r300->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL))
		has_tcl = 0;

	r300->hw.max_state_size = 2 + 2;	/* reserve extra space for WAIT_IDLE and tex cache flush */

	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & DEBUG_TEXTURE) {
		fprintf(stderr, "Using %d maximum texture units..\n", mtu);
	}

	/* Setup the atom linked list */
	make_empty_list(&r300->hw.atomlist);
	r300->hw.atomlist.name = "atom-list";

	/* Initialize state atoms */
	ALLOC_STATE(vpt, always, R300_VPT_CMDSIZE, 0);
	r300->hw.vpt.cmd[R300_VPT_CMD_0] = cmdpacket0(R300_SE_VPORT_XSCALE, 6);
	ALLOC_STATE(vap_cntl, always, 2, 0);
	r300->hw.vap_cntl.cmd[0] = cmdpacket0(R300_VAP_CNTL, 1);
	ALLOC_STATE(vte, always, 3, 0);
	r300->hw.vte.cmd[0] = cmdpacket0(R300_SE_VTE_CNTL, 2);
	ALLOC_STATE(unk2134, always, 3, 0);
	r300->hw.unk2134.cmd[0] = cmdpacket0(0x2134, 2);
	ALLOC_STATE(vap_cntl_status, always, 2, 0);
	r300->hw.vap_cntl_status.cmd[0] = cmdpacket0(R300_VAP_CNTL_STATUS, 1);
	ALLOC_STATE(vir[0], variable, R300_VIR_CMDSIZE, 0);
	r300->hw.vir[0].cmd[R300_VIR_CMD_0] =
	    cmdpacket0(R300_VAP_INPUT_ROUTE_0_0, 1);
	ALLOC_STATE(vir[1], variable, R300_VIR_CMDSIZE, 1);
	r300->hw.vir[1].cmd[R300_VIR_CMD_0] =
	    cmdpacket0(R300_VAP_INPUT_ROUTE_1_0, 1);
	ALLOC_STATE(vic, always, R300_VIC_CMDSIZE, 0);
	r300->hw.vic.cmd[R300_VIC_CMD_0] = cmdpacket0(R300_VAP_INPUT_CNTL_0, 2);
	ALLOC_STATE(unk21DC, always, 2, 0);
	r300->hw.unk21DC.cmd[0] = cmdpacket0(0x21DC, 1);
	ALLOC_STATE(unk221C, always, 2, 0);
	r300->hw.unk221C.cmd[0] = cmdpacket0(R300_VAP_UNKNOWN_221C, 1);
	ALLOC_STATE(unk2220, always, 5, 0);
	r300->hw.unk2220.cmd[0] = cmdpacket0(0x2220, 4);
	ALLOC_STATE(unk2288, always, 2, 0);
	r300->hw.unk2288.cmd[0] = cmdpacket0(R300_VAP_UNKNOWN_2288, 1);
	ALLOC_STATE(vof, always, R300_VOF_CMDSIZE, 0);
	r300->hw.vof.cmd[R300_VOF_CMD_0] =
	    cmdpacket0(R300_VAP_OUTPUT_VTX_FMT_0, 2);

	if (has_tcl) {
		ALLOC_STATE(pvs, always, R300_PVS_CMDSIZE, 0);
		r300->hw.pvs.cmd[R300_PVS_CMD_0] =
		    cmdpacket0(R300_VAP_PVS_CNTL_1, 3);
	}

	ALLOC_STATE(gb_enable, always, 2, 0);
	r300->hw.gb_enable.cmd[0] = cmdpacket0(R300_GB_ENABLE, 1);
	ALLOC_STATE(gb_misc, always, R300_GB_MISC_CMDSIZE, 0);
	r300->hw.gb_misc.cmd[0] = cmdpacket0(R300_GB_MSPOS0, 5);
	ALLOC_STATE(txe, always, R300_TXE_CMDSIZE, 0);
	r300->hw.txe.cmd[R300_TXE_CMD_0] = cmdpacket0(R300_TX_ENABLE, 1);
	ALLOC_STATE(unk4200, always, 5, 0);
	r300->hw.unk4200.cmd[0] = cmdpacket0(0x4200, 4);
	ALLOC_STATE(unk4214, always, 2, 0);
	r300->hw.unk4214.cmd[0] = cmdpacket0(0x4214, 1);
	ALLOC_STATE(ps, always, R300_PS_CMDSIZE, 0);
	r300->hw.ps.cmd[0] = cmdpacket0(R300_RE_POINTSIZE, 1);
	ALLOC_STATE(unk4230, always, 4, 0);
	r300->hw.unk4230.cmd[0] = cmdpacket0(0x4230, 3);
	ALLOC_STATE(lcntl, always, 2, 0);
	r300->hw.lcntl.cmd[0] = cmdpacket0(R300_RE_LINE_CNT, 1);
	ALLOC_STATE(unk4260, always, 4, 0);
	r300->hw.unk4260.cmd[0] = cmdpacket0(0x4260, 3);
	ALLOC_STATE(shade, always, 5, 0);
	r300->hw.shade.cmd[0] = cmdpacket0(R300_RE_SHADE, 4);
	ALLOC_STATE(polygon_mode, always, 4, 0);
	r300->hw.polygon_mode.cmd[0] = cmdpacket0(R300_RE_POLYGON_MODE, 3);
	ALLOC_STATE(fogp, always, 3, 0);
	r300->hw.fogp.cmd[0] = cmdpacket0(R300_RE_FOG_SCALE, 2);
	ALLOC_STATE(zbias_cntl, always, 2, 0);
	r300->hw.zbias_cntl.cmd[0] = cmdpacket0(R300_RE_ZBIAS_CNTL, 1);
	ALLOC_STATE(zbs, always, R300_ZBS_CMDSIZE, 0);
	r300->hw.zbs.cmd[R300_ZBS_CMD_0] =
	    cmdpacket0(R300_RE_ZBIAS_T_FACTOR, 4);
	ALLOC_STATE(occlusion_cntl, always, 2, 0);
	r300->hw.occlusion_cntl.cmd[0] = cmdpacket0(R300_RE_OCCLUSION_CNTL, 1);
	ALLOC_STATE(cul, always, R300_CUL_CMDSIZE, 0);
	r300->hw.cul.cmd[R300_CUL_CMD_0] = cmdpacket0(R300_RE_CULL_CNTL, 1);
	ALLOC_STATE(unk42C0, always, 3, 0);
	r300->hw.unk42C0.cmd[0] = cmdpacket0(0x42C0, 2);
	ALLOC_STATE(rc, always, R300_RC_CMDSIZE, 0);
	r300->hw.rc.cmd[R300_RC_CMD_0] = cmdpacket0(R300_RS_CNTL_0, 2);
	ALLOC_STATE(ri, always, R300_RI_CMDSIZE, 0);
	r300->hw.ri.cmd[R300_RI_CMD_0] = cmdpacket0(R300_RS_INTERP_0, 8);
	ALLOC_STATE(rr, variable, R300_RR_CMDSIZE, 0);
	r300->hw.rr.cmd[R300_RR_CMD_0] = cmdpacket0(R300_RS_ROUTE_0, 1);
	ALLOC_STATE(unk43A4, always, 3, 0);
	r300->hw.unk43A4.cmd[0] = cmdpacket0(0x43A4, 2);
	ALLOC_STATE(unk43E8, always, 2, 0);
	r300->hw.unk43E8.cmd[0] = cmdpacket0(0x43E8, 1);
	ALLOC_STATE(fp, always, R300_FP_CMDSIZE, 0);
	r300->hw.fp.cmd[R300_FP_CMD_0] = cmdpacket0(R300_PFS_CNTL_0, 3);
	r300->hw.fp.cmd[R300_FP_CMD_1] = cmdpacket0(R300_PFS_NODE_0, 4);
	ALLOC_STATE(fpt, variable, R300_FPT_CMDSIZE, 0);
	r300->hw.fpt.cmd[R300_FPT_CMD_0] = cmdpacket0(R300_PFS_TEXI_0, 0);
	ALLOC_STATE(unk46A4, always, 6, 0);
	r300->hw.unk46A4.cmd[0] = cmdpacket0(0x46A4, 5);
	ALLOC_STATE(fpi[0], variable, R300_FPI_CMDSIZE, 0);
	r300->hw.fpi[0].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR0_0, 1);
	ALLOC_STATE(fpi[1], variable, R300_FPI_CMDSIZE, 1);
	r300->hw.fpi[1].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR1_0, 1);
	ALLOC_STATE(fpi[2], variable, R300_FPI_CMDSIZE, 2);
	r300->hw.fpi[2].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR2_0, 1);
	ALLOC_STATE(fpi[3], variable, R300_FPI_CMDSIZE, 3);
	r300->hw.fpi[3].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR3_0, 1);
	ALLOC_STATE(fogs, always, R300_FOGS_CMDSIZE, 0);
	r300->hw.fogs.cmd[R300_FOGS_CMD_0] = cmdpacket0(R300_RE_FOG_STATE, 1);
	ALLOC_STATE(fogc, always, R300_FOGC_CMDSIZE, 0);
	r300->hw.fogc.cmd[R300_FOGC_CMD_0] = cmdpacket0(R300_FOG_COLOR_R, 3);
	ALLOC_STATE(at, always, R300_AT_CMDSIZE, 0);
	r300->hw.at.cmd[R300_AT_CMD_0] = cmdpacket0(R300_PP_ALPHA_TEST, 2);
	ALLOC_STATE(unk4BD8, always, 2, 0);
	r300->hw.unk4BD8.cmd[0] = cmdpacket0(0x4BD8, 1);
	ALLOC_STATE(fpp, variable, R300_FPP_CMDSIZE, 0);
	r300->hw.fpp.cmd[R300_FPP_CMD_0] = cmdpacket0(R300_PFS_PARAM_0_X, 0);
	ALLOC_STATE(unk4E00, always, 2, 0);
	r300->hw.unk4E00.cmd[0] = cmdpacket0(0x4E00, 1);
	ALLOC_STATE(bld, always, R300_BLD_CMDSIZE, 0);
	r300->hw.bld.cmd[R300_BLD_CMD_0] = cmdpacket0(R300_RB3D_CBLEND, 2);
	ALLOC_STATE(cmk, always, R300_CMK_CMDSIZE, 0);
	r300->hw.cmk.cmd[R300_CMK_CMD_0] = cmdpacket0(R300_RB3D_COLORMASK, 1);
	ALLOC_STATE(blend_color, always, 4, 0);
	r300->hw.blend_color.cmd[0] = cmdpacket0(R300_RB3D_BLEND_COLOR, 3);
	ALLOC_STATE(cb, always, R300_CB_CMDSIZE, 0);
	r300->hw.cb.cmd[R300_CB_CMD_0] = cmdpacket0(R300_RB3D_COLOROFFSET0, 1);
	r300->hw.cb.cmd[R300_CB_CMD_1] = cmdpacket0(R300_RB3D_COLORPITCH0, 1);
	ALLOC_STATE(unk4E50, always, 10, 0);
	r300->hw.unk4E50.cmd[0] = cmdpacket0(0x4E50, 9);
	ALLOC_STATE(unk4E88, always, 2, 0);
	r300->hw.unk4E88.cmd[0] = cmdpacket0(0x4E88, 1);
	ALLOC_STATE(unk4EA0, always, 3, 0);
	r300->hw.unk4EA0.cmd[0] = cmdpacket0(0x4EA0, 2);
	ALLOC_STATE(zs, always, R300_ZS_CMDSIZE, 0);
	r300->hw.zs.cmd[R300_ZS_CMD_0] =
	    cmdpacket0(R300_RB3D_ZSTENCIL_CNTL_0, 3);
	ALLOC_STATE(zstencil_format, always, 5, 0);
	r300->hw.zstencil_format.cmd[0] =
	    cmdpacket0(R300_RB3D_ZSTENCIL_FORMAT, 4);
	ALLOC_STATE(zb, always, R300_ZB_CMDSIZE, 0);
	r300->hw.zb.cmd[R300_ZB_CMD_0] = cmdpacket0(R300_RB3D_DEPTHOFFSET, 2);
	ALLOC_STATE(unk4F28, always, 2, 0);
	r300->hw.unk4F28.cmd[0] = cmdpacket0(0x4F28, 1);
	ALLOC_STATE(unk4F30, always, 3, 0);
	r300->hw.unk4F30.cmd[0] = cmdpacket0(0x4F30, 2);
	ALLOC_STATE(unk4F44, always, 2, 0);
	r300->hw.unk4F44.cmd[0] = cmdpacket0(0x4F44, 1);
	ALLOC_STATE(unk4F54, always, 2, 0);
	r300->hw.unk4F54.cmd[0] = cmdpacket0(0x4F54, 1);

	/* VPU only on TCL */
	if (has_tcl) {
		ALLOC_STATE(vpi, vpu, R300_VPI_CMDSIZE, 0);
		r300->hw.vpi.cmd[R300_VPI_CMD_0] =
		    cmdvpu(R300_PVS_UPLOAD_PROGRAM, 0);
		ALLOC_STATE(vpp, vpu, R300_VPP_CMDSIZE, 0);
		r300->hw.vpp.cmd[R300_VPP_CMD_0] =
		    cmdvpu(R300_PVS_UPLOAD_PARAMETERS, 0);
		ALLOC_STATE(vps, vpu, R300_VPS_CMDSIZE, 0);
		r300->hw.vps.cmd[R300_VPS_CMD_0] =
		    cmdvpu(R300_PVS_UPLOAD_POINTSIZE, 1);
	}

	/* Textures */
	ALLOC_STATE(tex.filter, variable, mtu + 1, 0);
	r300->hw.tex.filter.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_FILTER_0, 0);

	ALLOC_STATE(tex.filter_1, variable, mtu + 1, 0);
	r300->hw.tex.filter_1.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_FILTER1_0, 0);

	ALLOC_STATE(tex.size, variable, mtu + 1, 0);
	r300->hw.tex.size.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_SIZE_0, 0);

	ALLOC_STATE(tex.format, variable, mtu + 1, 0);
	r300->hw.tex.format.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_FORMAT_0, 0);

	ALLOC_STATE(tex.pitch, variable, mtu + 1, 0);
	r300->hw.tex.pitch.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_PITCH_0, 0);

	ALLOC_STATE(tex.offset, variable, mtu + 1, 0);
	r300->hw.tex.offset.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_OFFSET_0, 0);

	ALLOC_STATE(tex.chroma_key, variable, mtu + 1, 0);
	r300->hw.tex.chroma_key.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_CHROMA_KEY_0, 0);

	ALLOC_STATE(tex.border_color, variable, mtu + 1, 0);
	r300->hw.tex.border_color.cmd[R300_TEX_CMD_0] =
	    cmdpacket0(R300_TX_BORDER_COLOR_0, 0);

	r300->hw.is_dirty = GL_TRUE;
	r300->hw.all_dirty = GL_TRUE;

	/* Initialize command buffer */
	size =
	    256 * driQueryOptioni(&r300->radeon.optionCache,
				  "command_buffer_size");
	if (size < 2 * r300->hw.max_state_size) {
		size = 2 * r300->hw.max_state_size + 65535;
	}
	if (size > 64 * 256)
		size = 64 * 256;

	if (RADEON_DEBUG & (DEBUG_IOCTL | DEBUG_DMA)) {
		fprintf(stderr, "sizeof(drm_r300_cmd_header_t)=%zd\n",
			sizeof(drm_r300_cmd_header_t));
		fprintf(stderr, "sizeof(drm_radeon_cmd_buffer_t)=%zd\n",
			sizeof(drm_radeon_cmd_buffer_t));
		fprintf(stderr,
			"Allocating %d bytes command buffer (max state is %d bytes)\n",
			size * 4, r300->hw.max_state_size * 4);
	}

	r300->cmdbuf.size = size;
	r300->cmdbuf.cmd_buf = (uint32_t *) CALLOC(size * 4);
	r300->cmdbuf.count_used = 0;
	r300->cmdbuf.count_reemit = 0;
}

/**
 * Destroy the command buffer and state atoms.
 */
void r300DestroyCmdBuf(r300ContextPtr r300)
{
	struct r300_state_atom *atom;

	FREE(r300->cmdbuf.cmd_buf);

	foreach(atom, &r300->hw.atomlist) {
		FREE(atom->cmd);
	}
}

void r300EmitBlit(r300ContextPtr rmesa,
		  GLuint color_fmt,
		  GLuint src_pitch,
		  GLuint src_offset,
		  GLuint dst_pitch,
		  GLuint dst_offset,
		  GLint srcx, GLint srcy,
		  GLint dstx, GLint dsty, GLuint w, GLuint h)
{
	drm_r300_cmd_header_t *cmd;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr,
			"%s src %x/%x %d,%d dst: %x/%x %d,%d sz: %dx%d\n",
			__FUNCTION__, src_pitch, src_offset, srcx, srcy,
			dst_pitch, dst_offset, dstx, dsty, w, h);

	assert((src_pitch & 63) == 0);
	assert((dst_pitch & 63) == 0);
	assert((src_offset & 1023) == 0);
	assert((dst_offset & 1023) == 0);
	assert(w < (1 << 16));
	assert(h < (1 << 16));

	cmd = (drm_r300_cmd_header_t *) r300AllocCmdBuf(rmesa, 8, __FUNCTION__);

	cmd[0].header.cmd_type = R300_CMD_PACKET3;
	cmd[0].header.pad0 = R300_CMD_PACKET3_RAW;
	cmd[1].u = R300_CP_CMD_BITBLT_MULTI | (5 << 16);
	cmd[2].u = (RADEON_GMC_SRC_PITCH_OFFSET_CNTL |
		    RADEON_GMC_DST_PITCH_OFFSET_CNTL |
		    RADEON_GMC_BRUSH_NONE |
		    (color_fmt << 8) |
		    RADEON_GMC_SRC_DATATYPE_COLOR |
		    RADEON_ROP3_S |
		    RADEON_DP_SRC_SOURCE_MEMORY |
		    RADEON_GMC_CLR_CMP_CNTL_DIS | RADEON_GMC_WR_MSK_DIS);

	cmd[3].u = ((src_pitch / 64) << 22) | (src_offset >> 10);
	cmd[4].u = ((dst_pitch / 64) << 22) | (dst_offset >> 10);
	cmd[5].u = (srcx << 16) | srcy;
	cmd[6].u = (dstx << 16) | dsty;	/* dst */
	cmd[7].u = (w << 16) | h;
}

void r300EmitWait(r300ContextPtr rmesa, GLuint flags)
{
	drm_r300_cmd_header_t *cmd;

	assert(!(flags & ~(R300_WAIT_2D | R300_WAIT_3D)));

	cmd = (drm_r300_cmd_header_t *) r300AllocCmdBuf(rmesa, 1, __FUNCTION__);
	cmd[0].u = 0;
	cmd[0].wait.cmd_type = R300_CMD_WAIT;
	cmd[0].wait.flags = flags;
}
