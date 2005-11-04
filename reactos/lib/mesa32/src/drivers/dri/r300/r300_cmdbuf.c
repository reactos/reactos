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

/*
 * Authors:
 *   Nicolai Haehnle <prefect_@gmx.net>
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


// Set this to 1 for extremely verbose debugging of command buffers
#define DEBUG_CMDBUF		0


/**
 * Send the current command buffer via ioctl to the hardware.
 */
int r300FlushCmdBufLocked(r300ContextPtr r300, const char* caller)
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

	cmd.buf = (char*)(r300->cmdbuf.cmd_buf + start);
	cmd.bufsz = (r300->cmdbuf.count_used - start) * 4;

	if (r300->radeon.state.scissor.enabled) {
		cmd.nbox = r300->radeon.state.scissor.numClipRects;
		cmd.boxes = (drm_clip_rect_t *)r300->radeon.state.scissor.pClipRects;
	} else {
		cmd.nbox = r300->radeon.numClipRects;
		cmd.boxes = (drm_clip_rect_t *)r300->radeon.pClipRects;
	}

	ret = drmCommandWrite(r300->radeon.dri.fd,
			DRM_RADEON_CMDBUF, &cmd, sizeof(cmd));

	if (RADEON_DEBUG & DEBUG_SYNC) {
		fprintf(stderr, "Syncing in %s (from %s)\n\n", __FUNCTION__, caller);
		radeonWaitForIdleLocked(&r300->radeon);
	}

	r300->dma.nr_released_bufs = 0;
	r300->cmdbuf.count_used = 0;
	r300->cmdbuf.count_reemit = 0;

	return ret;
}


int r300FlushCmdBuf(r300ContextPtr r300, const char* caller)
{
	int ret;
	int i;
	drm_radeon_cmd_buffer_t cmd;
	int start;

	LOCK_HARDWARE(&r300->radeon);

	ret=r300FlushCmdBufLocked(r300, caller);

	UNLOCK_HARDWARE(&r300->radeon);

	if (ret) {
		fprintf(stderr, "drmRadeonCmdBuffer: %d (exiting)\n", ret);
		exit(ret);
	}

	return ret;
}


static void print_state_atom(struct r300_state_atom *state, int dwords)
{
	int i;

	fprintf(stderr, "  emit %s/%d/%d\n", state->name, dwords, state->cmd_size);

	if (RADEON_DEBUG & DEBUG_VERBOSE)
		for (i = 0; i < dwords; i++)
			fprintf(stderr, "      %s[%d]: %08X\n", state->name, i,
				state->cmd[i]);
}

/**
 * Emit all atoms with a dirty field equal to dirty.
 *
 * The caller must have ensured that there is enough space in the command
 * buffer.
 */
static __inline__ void r300DoEmitState(r300ContextPtr r300, GLboolean dirty)
{
	struct r300_state_atom* atom;
	uint32_t* dest;

	dest = r300->cmdbuf.cmd_buf + r300->cmdbuf.count_used;

	if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
		foreach(atom, &r300->hw.atomlist) {
			if ((atom->dirty || r300->hw.all_dirty) == dirty) {
				int dwords = (*atom->check)(r300, atom);

				if (dwords)
					print_state_atom(atom, dwords);
				else
					fprintf(stderr, "  skip state %s\n",
						atom->name);
			}
		}
	}


	/* Emit WAIT */
	*dest = cmdwait(R300_WAIT_3D | R300_WAIT_3D_CLEAN);
	dest ++;
	r300->cmdbuf.count_used ++;

	/* Emit END3D */
	*dest = cmdpacify();
	dest ++;
	r300->cmdbuf.count_used ++;

	/* Emit actual atoms */

	foreach(atom, &r300->hw.atomlist) {
		if ((atom->dirty || r300->hw.all_dirty) == dirty) {
			int dwords = (*atom->check)(r300, atom);

			if (dwords) {
				memcpy(dest, atom->cmd, dwords*4);
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

	if (r300->cmdbuf.count_used && !r300->hw.is_dirty && !r300->hw.all_dirty)
		return;

	/* To avoid going across the entire set of states multiple times, just check
	 * for enough space for the case of emitting all state, and inline the
	 * r300AllocCmdBuf code here without all the checks.
	 */
	r300EnsureCmdBufSpace(r300, r300->hw.max_state_size, __FUNCTION__);

	if (!r300->cmdbuf.count_used) {
		if (RADEON_DEBUG & DEBUG_STATE)
			fprintf(stderr, "Begin reemit state\n");

		r300DoEmitState(r300, GL_FALSE);
		r300->cmdbuf.count_reemit = r300->cmdbuf.count_used;
	}

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "Begin dirty state\n");

	r300DoEmitState(r300, GL_TRUE);

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

CHECK( always, atom->cmd_size )
CHECK( never, 0 )
CHECK( variable, packet0_count(atom->cmd) ? (1 + packet0_count(atom->cmd)) : 0 )
CHECK( vpu, vpu_count(atom->cmd) ? (1 + vpu_count(atom->cmd)*4) : 0 )

#undef packet0_count
#undef vpu_count

#define ALLOC_STATE( ATOM, CHK, SZ, NM, IDX )				\
   do {									\
      r300->hw.ATOM.cmd_size = (SZ);					\
      r300->hw.ATOM.cmd = (uint32_t*)CALLOC((SZ) * sizeof(uint32_t));	\
      r300->hw.ATOM.name = (NM);					\
      r300->hw.ATOM.idx = (IDX);					\
      r300->hw.ATOM.check = check_##CHK;				\
      r300->hw.ATOM.dirty = GL_FALSE;					\
      r300->hw.max_state_size += (SZ);					\
   } while (0)


/**
 * Allocate memory for the command buffer and initialize the state atom
 * list. Note that the initial hardware state is set by r300InitState().
 */
void r300InitCmdBuf(r300ContextPtr r300)
{
	int size, i, mtu;
	
	r300->hw.max_state_size = 2; /* reserve extra space for WAIT_IDLE */

	mtu = r300->radeon.glCtx->Const.MaxTextureUnits;
	if (RADEON_DEBUG & DEBUG_TEXTURE) {
		fprintf(stderr, "Using %d maximum texture units..\n", mtu);
	}

	/* Initialize state atoms */
	ALLOC_STATE( vpt, always, R300_VPT_CMDSIZE, "vpt", 0 );
		r300->hw.vpt.cmd[R300_VPT_CMD_0] = cmdpacket0(R300_SE_VPORT_XSCALE, 6);
	ALLOC_STATE( unk2080, always, 2, "unk2080", 0 );
		r300->hw.unk2080.cmd[0] = cmdpacket0(0x2080, 1);
	ALLOC_STATE( vte, always, 3, "vte", 0 );
		r300->hw.vte.cmd[0] = cmdpacket0(R300_SE_VTE_CNTL, 2);
	ALLOC_STATE( unk2134, always, 3, "unk2134", 0 );
		r300->hw.unk2134.cmd[0] = cmdpacket0(0x2134, 2);
	ALLOC_STATE( unk2140, always, 2, "unk2140", 0 );
		r300->hw.unk2140.cmd[0] = cmdpacket0(0x2140, 1);
	ALLOC_STATE( vir[0], variable, R300_VIR_CMDSIZE, "vir/0", 0 );
		r300->hw.vir[0].cmd[R300_VIR_CMD_0] = cmdpacket0(R300_VAP_INPUT_ROUTE_0_0, 1);
	ALLOC_STATE( vir[1], variable, R300_VIR_CMDSIZE, "vir/1", 1 );
		r300->hw.vir[1].cmd[R300_VIR_CMD_0] = cmdpacket0(R300_VAP_INPUT_ROUTE_1_0, 1);
	ALLOC_STATE( vic, always, R300_VIC_CMDSIZE, "vic", 0 );
		r300->hw.vic.cmd[R300_VIC_CMD_0] = cmdpacket0(R300_VAP_INPUT_CNTL_0, 2);
	ALLOC_STATE( unk21DC, always, 2, "unk21DC", 0 );
		r300->hw.unk21DC.cmd[0] = cmdpacket0(0x21DC, 1);
	ALLOC_STATE( unk221C, always, 2, "unk221C", 0 );
		r300->hw.unk221C.cmd[0] = cmdpacket0(0x221C, 1);
	ALLOC_STATE( unk2220, always, 5, "unk2220", 0 );
		r300->hw.unk2220.cmd[0] = cmdpacket0(0x2220, 4);
	ALLOC_STATE( unk2288, always, 2, "unk2288", 0 );
		r300->hw.unk2288.cmd[0] = cmdpacket0(0x2288, 1);
	ALLOC_STATE( vof, always, R300_VOF_CMDSIZE, "vof", 0 );
		r300->hw.vof.cmd[R300_VOF_CMD_0] = cmdpacket0(R300_VAP_OUTPUT_VTX_FMT_0, 2);
	ALLOC_STATE( pvs, always, R300_PVS_CMDSIZE, "pvs", 0 );
		r300->hw.pvs.cmd[R300_PVS_CMD_0] = cmdpacket0(R300_VAP_PVS_CNTL_1, 3);
	ALLOC_STATE( gb_enable, always, 2, "gb_enable", 0 );
		r300->hw.gb_enable.cmd[0] = cmdpacket0(R300_GB_ENABLE, 1);
	ALLOC_STATE( gb_misc, always, R300_GB_MISC_CMDSIZE, "gb_misc", 0 );
		r300->hw.gb_misc.cmd[0] = cmdpacket0(R300_GB_MSPOS0, 5);
	ALLOC_STATE( txe, always, R300_TXE_CMDSIZE, "txe", 0 );
		r300->hw.txe.cmd[R300_TXE_CMD_0] = cmdpacket0(R300_TX_ENABLE, 1);
	ALLOC_STATE( unk4200, always, 5, "unk4200", 0 );
		r300->hw.unk4200.cmd[0] = cmdpacket0(0x4200, 4);
	ALLOC_STATE( unk4214, always, 2, "unk4214", 0 );
		r300->hw.unk4214.cmd[0] = cmdpacket0(0x4214, 1);
	ALLOC_STATE( ps, always, R300_PS_CMDSIZE, "ps", 0 );
		r300->hw.ps.cmd[0] = cmdpacket0(R300_RE_POINTSIZE, 1);
	ALLOC_STATE( unk4230, always, 4, "unk4230", 0 );
		r300->hw.unk4230.cmd[0] = cmdpacket0(0x4230, 3);
	ALLOC_STATE( lcntl, always, 2, "lcntl", 0 );
		r300->hw.lcntl.cmd[0] = cmdpacket0(R300_RE_LINE_CNT, 1);
	ALLOC_STATE( unk4260, always, 4, "unk4260", 0 );
		r300->hw.unk4260.cmd[0] = cmdpacket0(0x4260, 3);
	ALLOC_STATE( unk4274, always, 5, "unk4274", 0 );
		r300->hw.unk4274.cmd[0] = cmdpacket0(0x4274, 4);
	ALLOC_STATE( unk4288, always, 6, "unk4288", 0 );
		r300->hw.unk4288.cmd[0] = cmdpacket0(0x4288, 5);
	ALLOC_STATE( unk42A0, always, 2, "unk42A0", 0 );
		r300->hw.unk42A0.cmd[0] = cmdpacket0(0x42A0, 1);
	ALLOC_STATE( zbs, always, R300_ZBS_CMDSIZE, "zbs", 0 );
		r300->hw.zbs.cmd[R300_ZBS_CMD_0] = cmdpacket0(R300_RE_ZBIAS_T_FACTOR, 4);
	ALLOC_STATE( unk42B4, always, 2, "unk42B4", 0 );
		r300->hw.unk42B4.cmd[0] = cmdpacket0(0x42B4, 1);
	ALLOC_STATE( cul, always, R300_CUL_CMDSIZE, "cul", 0 );
		r300->hw.cul.cmd[R300_CUL_CMD_0] = cmdpacket0(R300_RE_CULL_CNTL, 1);
	ALLOC_STATE( unk42C0, always, 3, "unk42C0", 0 );
		r300->hw.unk42C0.cmd[0] = cmdpacket0(0x42C0, 2);
	ALLOC_STATE( rc, always, R300_RC_CMDSIZE, "rc", 0 );
		r300->hw.rc.cmd[R300_RC_CMD_0] = cmdpacket0(R300_RS_CNTL_0, 2);
	ALLOC_STATE( ri, always, R300_RI_CMDSIZE, "ri", 0 );
		r300->hw.ri.cmd[R300_RI_CMD_0] = cmdpacket0(R300_RS_INTERP_0, 8);
	ALLOC_STATE( rr, variable, R300_RR_CMDSIZE, "rr", 0 );
		r300->hw.rr.cmd[R300_RR_CMD_0] = cmdpacket0(R300_RS_ROUTE_0, 1);
	ALLOC_STATE( unk43A4, always, 3, "unk43A4", 0 );
		r300->hw.unk43A4.cmd[0] = cmdpacket0(0x43A4, 2);
	ALLOC_STATE( unk43E8, always, 2, "unk43E8", 0 );
		r300->hw.unk43E8.cmd[0] = cmdpacket0(0x43E8, 1);
	ALLOC_STATE( fp, always, R300_FP_CMDSIZE, "fp", 0 );
		r300->hw.fp.cmd[R300_FP_CMD_0] = cmdpacket0(R300_PFS_CNTL_0, 3);
		r300->hw.fp.cmd[R300_FP_CMD_1] = cmdpacket0(R300_PFS_NODE_0, 4);
	ALLOC_STATE( fpt, variable, R300_FPT_CMDSIZE, "fpt", 0 );
		r300->hw.fpt.cmd[R300_FPT_CMD_0] = cmdpacket0(R300_PFS_TEXI_0, 0);
	ALLOC_STATE( unk46A4, always, 6, "unk46A4", 0 );
		r300->hw.unk46A4.cmd[0] = cmdpacket0(0x46A4, 5);
	ALLOC_STATE( fpi[0], variable, R300_FPI_CMDSIZE, "fpi/0", 0 );
		r300->hw.fpi[0].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR0_0, 1);
	ALLOC_STATE( fpi[1], variable, R300_FPI_CMDSIZE, "fpi/1", 1 );
		r300->hw.fpi[1].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR1_0, 1);
	ALLOC_STATE( fpi[2], variable, R300_FPI_CMDSIZE, "fpi/2", 2 );
		r300->hw.fpi[2].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR2_0, 1);
	ALLOC_STATE( fpi[3], variable, R300_FPI_CMDSIZE, "fpi/3", 3 );
		r300->hw.fpi[3].cmd[R300_FPI_CMD_0] = cmdpacket0(R300_PFS_INSTR3_0, 1);
	ALLOC_STATE( unk4BC0, always, 2, "unk4BC0", 0 );
		r300->hw.unk4BC0.cmd[0] = cmdpacket0(0x4BC0, 1);
	ALLOC_STATE( unk4BC8, always, 4, "unk4BC8", 0 );
		r300->hw.unk4BC8.cmd[0] = cmdpacket0(0x4BC8, 3);
	ALLOC_STATE( at, always, R300_AT_CMDSIZE, "at", 0 );
		r300->hw.at.cmd[R300_AT_CMD_0] = cmdpacket0(R300_PP_ALPHA_TEST, 2);
	ALLOC_STATE( unk4BD8, always, 2, "unk4BD8", 0 );
		r300->hw.unk4BD8.cmd[0] = cmdpacket0(0x4BD8, 1);
	ALLOC_STATE( fpp, variable, R300_FPP_CMDSIZE, "fpp", 0 );
		r300->hw.fpp.cmd[R300_FPP_CMD_0] = cmdpacket0(R300_PFS_PARAM_0_X, 0);
	ALLOC_STATE( unk4E00, always, 2, "unk4E00", 0 );
		r300->hw.unk4E00.cmd[0] = cmdpacket0(0x4E00, 1);
	ALLOC_STATE( bld, always, R300_BLD_CMDSIZE, "bld", 0 );
		r300->hw.bld.cmd[R300_BLD_CMD_0] = cmdpacket0(R300_RB3D_CBLEND, 2);
	ALLOC_STATE( cmk, always, R300_CMK_CMDSIZE, "cmk", 0 );
		r300->hw.cmk.cmd[R300_CMK_CMD_0] = cmdpacket0(R300_RB3D_COLORMASK, 1);
	ALLOC_STATE( unk4E10, always, 4, "unk4E10", 0 );
		r300->hw.unk4E10.cmd[0] = cmdpacket0(0x4E10, 3);
	ALLOC_STATE( cb, always, R300_CB_CMDSIZE, "cb", 0 );
		r300->hw.cb.cmd[R300_CB_CMD_0] = cmdpacket0(R300_RB3D_COLOROFFSET0, 1);
		r300->hw.cb.cmd[R300_CB_CMD_1] = cmdpacket0(R300_RB3D_COLORPITCH0, 1);
	ALLOC_STATE( unk4E50, always, 10, "unk4E50", 0 );
		r300->hw.unk4E50.cmd[0] = cmdpacket0(0x4E50, 9);
	ALLOC_STATE( unk4E88, always, 2, "unk4E88", 0 );
		r300->hw.unk4E88.cmd[0] = cmdpacket0(0x4E88, 1);
	ALLOC_STATE( unk4EA0, always, 3, "unk4EA0 R350 only", 0 );
		r300->hw.unk4EA0.cmd[0] = cmdpacket0(0x4EA0, 2);
	ALLOC_STATE( zs, always, R300_ZS_CMDSIZE, "zstencil", 0 );
		r300->hw.zs.cmd[R300_ZS_CMD_0] = cmdpacket0(R300_RB3D_ZSTENCIL_CNTL_0, 3);
	ALLOC_STATE( unk4F10, always, 5, "unk4F10", 0 );
		r300->hw.unk4F10.cmd[0] = cmdpacket0(0x4F10, 4);
	ALLOC_STATE( zb, always, R300_ZB_CMDSIZE, "zb", 0 );
		r300->hw.zb.cmd[R300_ZB_CMD_0] = cmdpacket0(R300_RB3D_DEPTHOFFSET, 2);
	ALLOC_STATE( unk4F28, always, 2, "unk4F28", 0 );
		r300->hw.unk4F28.cmd[0] = cmdpacket0(0x4F28, 1);
	ALLOC_STATE( unk4F30, always, 3, "unk4F30", 0 );
		r300->hw.unk4F30.cmd[0] = cmdpacket0(0x4F30, 2);
	ALLOC_STATE( unk4F44, always, 2, "unk4F44", 0 );
		r300->hw.unk4F44.cmd[0] = cmdpacket0(0x4F44, 1);
	ALLOC_STATE( unk4F54, always, 2, "unk4F54", 0 );
		r300->hw.unk4F54.cmd[0] = cmdpacket0(0x4F54, 1);

	ALLOC_STATE( vpi, vpu, R300_VPI_CMDSIZE, "vpi", 0 );
		r300->hw.vpi.cmd[R300_VPI_CMD_0] = cmdvpu(R300_PVS_UPLOAD_PROGRAM, 0);
	ALLOC_STATE( vpp, vpu, R300_VPP_CMDSIZE, "vpp", 0 );
		r300->hw.vpp.cmd[R300_VPP_CMD_0] = cmdvpu(R300_PVS_UPLOAD_PARAMETERS, 0);
	ALLOC_STATE( vps, vpu, R300_VPS_CMDSIZE, "vps", 0 );
		r300->hw.vps.cmd[R300_VPS_CMD_0] = cmdvpu(R300_PVS_UPLOAD_POINTSIZE, 1);

	/* Textures */
	ALLOC_STATE( tex.filter, variable, mtu+1, "tex_filter", 0 );
		r300->hw.tex.filter.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_FILTER_0, 0);

	ALLOC_STATE( tex.unknown1, variable, mtu+1, "tex_unknown1", 0 );
		r300->hw.tex.unknown1.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_UNK1_0, 0);

	ALLOC_STATE( tex.size, variable, mtu+1, "tex_size", 0 );
		r300->hw.tex.size.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_SIZE_0, 0);

	ALLOC_STATE( tex.format, variable, mtu+1, "tex_format", 0 );
		r300->hw.tex.format.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_FORMAT_0, 0);

	ALLOC_STATE( tex.offset, variable, mtu+1, "tex_offset", 0 );
		r300->hw.tex.offset.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_OFFSET_0, 0);

	ALLOC_STATE( tex.unknown4, variable, mtu+1, "tex_unknown4", 0 );
		r300->hw.tex.unknown4.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_UNK4_0, 0);

	ALLOC_STATE( tex.border_color, variable, mtu+1, "tex_border_color", 0 );
		r300->hw.tex.border_color.cmd[R300_TEX_CMD_0] = cmdpacket0(R300_TX_BORDER_COLOR_0, 0);


	/* Setup the atom linked list */
	make_empty_list(&r300->hw.atomlist);
	r300->hw.atomlist.name = "atom-list";

	insert_at_tail(&r300->hw.atomlist, &r300->hw.vpt);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk2080);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.vte);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk2134);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk2140);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.vir[0]);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.vir[1]);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.vic);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk21DC);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk221C);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk2220);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk2288);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.vof);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.pvs);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.gb_enable);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.gb_misc);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.txe);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4200);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4214);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.ps);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4230);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.lcntl);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4260);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4274);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4288);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk42A0);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.zbs);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk42B4);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.cul);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk42C0);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.rc);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.ri);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.rr);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk43A4);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk43E8);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.fp);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.fpt);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk46A4);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.fpi[0]);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.fpi[1]);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.fpi[2]);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.fpi[3]);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4BC0);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4BC8);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.at);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4BD8);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.fpp);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4E00);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.bld);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.cmk);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4E10);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.cb);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4E50);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4E88);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4EA0);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.zs);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4F10);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.zb);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4F28);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4F30);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4F44);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.unk4F54);

	insert_at_tail(&r300->hw.atomlist, &r300->hw.vpi);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.vpp);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.vps);

	insert_at_tail(&r300->hw.atomlist, &r300->hw.tex.filter);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.tex.unknown1);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.tex.size);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.tex.format);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.tex.offset);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.tex.unknown4);
	insert_at_tail(&r300->hw.atomlist, &r300->hw.tex.border_color);

	r300->hw.is_dirty = GL_TRUE;
	r300->hw.all_dirty = GL_TRUE;

	/* Initialize command buffer */
	size = 256 * driQueryOptioni(&r300->radeon.optionCache, "command_buffer_size");
	if (size < 2*r300->hw.max_state_size) {
		size = 2*r300->hw.max_state_size+65535;
	}
	if (size > 64*256)
		size = 64*256;

	if (RADEON_DEBUG & (DEBUG_IOCTL|DEBUG_DMA)) {
		fprintf(stderr, "sizeof(drm_r300_cmd_header_t)=%d\n",
			sizeof(drm_r300_cmd_header_t));
		fprintf(stderr, "sizeof(drm_radeon_cmd_buffer_t)=%d\n",
			sizeof(drm_radeon_cmd_buffer_t));
		fprintf(stderr,
			"Allocating %d bytes command buffer (max state is %d bytes)\n",
			size*4, r300->hw.max_state_size*4);
	}

	r300->cmdbuf.size = size;
	r300->cmdbuf.cmd_buf = (uint32_t*)CALLOC(size*4);
	r300->cmdbuf.count_used = 0;
	r300->cmdbuf.count_reemit = 0;
}


/**
 * Destroy the command buffer and state atoms.
 */
void r300DestroyCmdBuf(r300ContextPtr r300)
{
	struct r300_state_atom* atom;

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
	drm_radeon_cmd_header_t *cmd;

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

	cmd =
	    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, 8,
							__FUNCTION__);

	cmd[0].header.cmd_type = R300_CMD_PACKET3;
	cmd[1].i = R200_CP_CMD_BITBLT_MULTI | (5 << 16);
	cmd[2].i = (RADEON_GMC_SRC_PITCH_OFFSET_CNTL |
		    RADEON_GMC_DST_PITCH_OFFSET_CNTL |
		    RADEON_GMC_BRUSH_NONE |
		    (color_fmt << 8) |
		    RADEON_GMC_SRC_DATATYPE_COLOR |
		    RADEON_ROP3_S |
		    RADEON_DP_SRC_SOURCE_MEMORY |
		    RADEON_GMC_CLR_CMP_CNTL_DIS | RADEON_GMC_WR_MSK_DIS);

	cmd[3].i = ((src_pitch / 64) << 22) | (src_offset >> 10);
	cmd[4].i = ((dst_pitch / 64) << 22) | (dst_offset >> 10);
	cmd[5].i = (srcx << 16) | srcy;
	cmd[6].i = (dstx << 16) | dsty;	/* dst */
	cmd[7].i = (w << 16) | h;
}

void r300EmitWait(r300ContextPtr rmesa, GLuint flags)
{
	if (rmesa->radeon.dri.drmMinor >= 6) {
		drm_radeon_cmd_header_t *cmd;

		assert(!(flags & ~(R300_WAIT_2D | R300_WAIT_3D)));

		cmd =
		    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa,
								1,
								__FUNCTION__);
		cmd[0].i = 0;
		cmd[0].wait.cmd_type = R300_CMD_WAIT;
		cmd[0].wait.flags = flags;
	}
}

void r300EmitAOS(r300ContextPtr rmesa, GLuint nr, GLuint offset)
{
	if (RADEON_DEBUG & DEBUG_VERTS)
	    fprintf(stderr, "%s: nr=%d, ofs=0x%08x\n", __func__, nr, offset);
    int sz = 1 + (nr >> 1) * 3 + (nr & 1) * 2;
    int i;
    LOCAL_VARS

    start_packet3(RADEON_CP_PACKET3_3D_LOAD_VBPNTR, sz-1);
    e32(nr);
    for(i=0;i+1<nr;i+=2){
        e32(  (rmesa->state.aos[i].aos_size << 0)
             |(rmesa->state.aos[i].aos_stride << 8)
             |(rmesa->state.aos[i+1].aos_size << 16)
             |(rmesa->state.aos[i+1].aos_stride << 24)
        );
        e32(rmesa->state.aos[i].aos_offset+offset*4*rmesa->state.aos[i].aos_stride);
        e32(rmesa->state.aos[i+1].aos_offset+offset*4*rmesa->state.aos[i+1].aos_stride);
    }
    if(nr & 1){
        e32(  (rmesa->state.aos[nr-1].aos_size << 0)
             |(rmesa->state.aos[nr-1].aos_stride << 8)
        );
        e32(rmesa->state.aos[nr-1].aos_offset+offset*4*rmesa->state.aos[nr-1].aos_stride);
    }

}

