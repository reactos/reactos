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

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
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

//#define CB_DPATH

#define CLEARBUFFER_COLOR	0x1
#define CLEARBUFFER_DEPTH	0x2
#define CLEARBUFFER_STENCIL	0x4

static void r300ClearBuffer(r300ContextPtr r300, int flags, int buffer)
{
	GLcontext* ctx = r300->radeon.glCtx;
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	GLuint cboffset, cbpitch;
	drm_r300_cmd_header_t* cmd2;
#ifdef CB_DPATH
	r300ContextPtr rmesa=r300;
	LOCAL_VARS;
#else
	int i;
#endif
	
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

#ifndef CB_DPATH
	R300_STATECHANGE(r300, vir[0]);
	((drm_r300_cmd_header_t*)r300->hw.vir[0].cmd)->packet0.count = 1;
	r300->hw.vir[0].cmd[1] = 0x21030003;

	R300_STATECHANGE(r300, vir[1]);
	((drm_r300_cmd_header_t*)r300->hw.vir[1].cmd)->packet0.count = 1;
	r300->hw.vir[1].cmd[1] = 0xF688F688;

	R300_STATECHANGE(r300, vic);
	r300->hw.vic.cmd[R300_VIC_CNTL_0] = 0x00000001;
	r300->hw.vic.cmd[R300_VIC_CNTL_1] = 0x00000405;
	
	R300_STATECHANGE(r300, vof);
	r300->hw.vof.cmd[R300_VOF_CNTL_0] = R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT
				| R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT;
	r300->hw.vof.cmd[R300_VOF_CNTL_1] = 0; /* no textures */
	
	R300_STATECHANGE(r300, txe);
	r300->hw.txe.cmd[R300_TXE_ENABLE] = 0;
	
	R300_STATECHANGE(r300, vpt);
	r300->hw.vpt.cmd[R300_VPT_XSCALE] = r300PackFloat32(1.0);
	r300->hw.vpt.cmd[R300_VPT_XOFFSET] = r300PackFloat32(dPriv->x);
	r300->hw.vpt.cmd[R300_VPT_YSCALE] = r300PackFloat32(1.0);
	r300->hw.vpt.cmd[R300_VPT_YOFFSET] = r300PackFloat32(dPriv->y);
	r300->hw.vpt.cmd[R300_VPT_ZSCALE] = r300PackFloat32(1.0);
	r300->hw.vpt.cmd[R300_VPT_ZOFFSET] = r300PackFloat32(0.0);

	R300_STATECHANGE(r300, at);
	r300->hw.at.cmd[R300_AT_ALPHA_TEST] = 0;
	
	R300_STATECHANGE(r300, bld);
	r300->hw.bld.cmd[R300_BLD_CBLEND] = 0;
	r300->hw.bld.cmd[R300_BLD_ABLEND] = 0;
	
	if (r300->radeon.radeonScreen->cpp == 4)
		cbpitch |= R300_COLOR_FORMAT_ARGB8888;
	else
		cbpitch |= R300_COLOR_FORMAT_RGB565;
	
	if (r300->radeon.sarea->tiling_enabled)
		cbpitch |= R300_COLOR_TILE_ENABLE;
	
	R300_STATECHANGE(r300, cb);
	r300->hw.cb.cmd[R300_CB_OFFSET] = cboffset;
	r300->hw.cb.cmd[R300_CB_PITCH] = cbpitch;

	R300_STATECHANGE(r300, unk221C);
	r300->hw.unk221C.cmd[1] = R300_221C_CLEAR;

	R300_STATECHANGE(r300, ps);
	r300->hw.ps.cmd[R300_PS_POINTSIZE] =
		((dPriv->w * 6) << R300_POINTSIZE_X_SHIFT) |
		((dPriv->h * 6) << R300_POINTSIZE_Y_SHIFT);

	R300_STATECHANGE(r300, ri);
	for(i = 1; i <= 8; ++i)
		r300->hw.ri.cmd[i] = R300_RS_INTERP_USED;

	R300_STATECHANGE(r300, rc);
	/* The second constant is needed to get glxgears display anything .. */
	r300->hw.rc.cmd[1] = (1 << R300_RS_CNTL_CI_CNT_SHIFT) | R300_RS_CNTL_0_UNKNOWN_18;
	r300->hw.rc.cmd[2] = 0;
	
	R300_STATECHANGE(r300, rr);
	((drm_r300_cmd_header_t*)r300->hw.rr.cmd)->packet0.count = 1;
	r300->hw.rr.cmd[1] = 0x00004000;

	R300_STATECHANGE(r300, cmk);
	if (flags & CLEARBUFFER_COLOR) {
		r300->hw.cmk.cmd[R300_CMK_COLORMASK] =
			(ctx->Color.ColorMask[BCOMP] ? R300_COLORMASK0_B : 0) |
			(ctx->Color.ColorMask[GCOMP] ? R300_COLORMASK0_G : 0) |
			(ctx->Color.ColorMask[RCOMP] ? R300_COLORMASK0_R : 0) |
			(ctx->Color.ColorMask[ACOMP] ? R300_COLORMASK0_A : 0);
	} else {
		r300->hw.cmk.cmd[R300_CMK_COLORMASK] = 0;
	}

	R300_STATECHANGE(r300, fp);
	r300->hw.fp.cmd[R300_FP_CNTL0] = 0; /* 1 pass, no textures */
	r300->hw.fp.cmd[R300_FP_CNTL1] = 0; /* no temporaries */
	r300->hw.fp.cmd[R300_FP_CNTL2] = 0; /* no offset, one ALU instr */
	r300->hw.fp.cmd[R300_FP_NODE0] = 0;
	r300->hw.fp.cmd[R300_FP_NODE1] = 0;
	r300->hw.fp.cmd[R300_FP_NODE2] = 0;
	r300->hw.fp.cmd[R300_FP_NODE3] = R300_PFS_NODE_LAST_NODE;

	R300_STATECHANGE(r300, fpi[0]);
	R300_STATECHANGE(r300, fpi[1]);
	R300_STATECHANGE(r300, fpi[2]);
	R300_STATECHANGE(r300, fpi[3]);
	((drm_r300_cmd_header_t*)r300->hw.fpi[0].cmd)->packet0.count = 1;
	((drm_r300_cmd_header_t*)r300->hw.fpi[1].cmd)->packet0.count = 1;
	((drm_r300_cmd_header_t*)r300->hw.fpi[2].cmd)->packet0.count = 1;
	((drm_r300_cmd_header_t*)r300->hw.fpi[3].cmd)->packet0.count = 1;

	/* MOV o0, t0 */
	r300->hw.fpi[0].cmd[1] = FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO));
	r300->hw.fpi[1].cmd[1] = FP_SELC(0,NO,XYZ,FP_TMP(0),0,0);
	r300->hw.fpi[2].cmd[1] = FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO));
	r300->hw.fpi[3].cmd[1] = FP_SELA(0,NO,W,FP_TMP(0),0,0);

	R300_STATECHANGE(r300, pvs);
	r300->hw.pvs.cmd[R300_PVS_CNTL_1] =
		(0 << R300_PVS_CNTL_1_PROGRAM_START_SHIFT) |
		(0 << R300_PVS_CNTL_1_POS_END_SHIFT) |
		(1 << R300_PVS_CNTL_1_PROGRAM_END_SHIFT);
	r300->hw.pvs.cmd[R300_PVS_CNTL_2] = 0; /* no parameters */
	r300->hw.pvs.cmd[R300_PVS_CNTL_3] =
		(1 << R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT);

	R300_STATECHANGE(r300, vpi);
	((drm_r300_cmd_header_t*)r300->hw.vpi.cmd)->packet0.count = 8;

	/* MOV o0, i0; */
	r300->hw.vpi.cmd[1] = VP_OUT(ADD,OUT,0,XYZW);
	r300->hw.vpi.cmd[2] = VP_IN(IN,0);
	r300->hw.vpi.cmd[3] = VP_ZERO();
	r300->hw.vpi.cmd[4] = 0;

	/* MOV o1, i1; */
	r300->hw.vpi.cmd[5] = VP_OUT(ADD,OUT,1,XYZW);
	r300->hw.vpi.cmd[6] = VP_IN(IN,1);
	r300->hw.vpi.cmd[7] = VP_ZERO();
	r300->hw.vpi.cmd[8] = 0;

	R300_STATECHANGE(r300, zs);
	if (flags & CLEARBUFFER_DEPTH) {
		r300->hw.zs.cmd[R300_ZS_CNTL_0] &= R300_RB3D_STENCIL_ENABLE;
		r300->hw.zs.cmd[R300_ZS_CNTL_0] |= 0x6; // test and write
		r300->hw.zs.cmd[R300_ZS_CNTL_1] &= ~(R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= (R300_ZS_ALWAYS<<R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);
/*
		R300_STATECHANGE(r300, zb);
		r300->hw.zb.cmd[R300_ZB_OFFSET] =
			1024*4*300 +
			r300->radeon.radeonScreen->frontOffset +
			r300->radeon.radeonScreen->fbLocation;
		r300->hw.zb.cmd[R300_ZB_PITCH] =
			r300->radeon.radeonScreen->depthPitch;
*/
	} else {
		r300->hw.zs.cmd[R300_ZS_CNTL_0] &= R300_RB3D_STENCIL_ENABLE;
		r300->hw.zs.cmd[R300_ZS_CNTL_0] |= R300_RB3D_Z_DISABLED_1; // disable
		r300->hw.zs.cmd[R300_ZS_CNTL_1] &= ~(R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);
	}
	
	R300_STATECHANGE(r300, zs);
	if (flags & CLEARBUFFER_STENCIL) {
		r300->hw.zs.cmd[R300_ZS_CNTL_0] &= ~R300_RB3D_STENCIL_ENABLE;
		r300->hw.zs.cmd[R300_ZS_CNTL_0] |= R300_RB3D_STENCIL_ENABLE;
		r300->hw.zs.cmd[R300_ZS_CNTL_1] &= 
		    ~((R300_ZS_MASK << R300_RB3D_ZS1_FRONT_FUNC_SHIFT) | (R300_ZS_MASK << R300_RB3D_ZS1_BACK_FUNC_SHIFT));
		r300->hw.zs.cmd[R300_ZS_CNTL_1] |= 
		    (R300_ZS_ALWAYS<<R300_RB3D_ZS1_FRONT_FUNC_SHIFT) | 
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_FRONT_FAIL_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_FRONT_ZPASS_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_FRONT_ZFAIL_OP_SHIFT) |
		    (R300_ZS_ALWAYS<<R300_RB3D_ZS1_BACK_FUNC_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_BACK_FAIL_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_BACK_ZPASS_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_BACK_ZFAIL_OP_SHIFT) ;
		r300->hw.zs.cmd[R300_ZS_CNTL_2] = r300->state.stencil.clear;
	}
			
	/* Make sure we have enough space */
	r300EnsureCmdBufSpace(r300, r300->hw.max_state_size + 9+8, __FUNCTION__);

	r300EmitState(r300);
#else
	R300_STATECHANGE(r300, cb);
	reg_start(R300_RB3D_COLOROFFSET0, 0);
	e32(cboffset);
	
	if (r300->radeon.radeonScreen->cpp == 4)
		cbpitch |= R300_COLOR_FORMAT_ARGB8888;
	else
		cbpitch |= R300_COLOR_FORMAT_RGB565;
	
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
		e32(0);
	}
	
	R300_STATECHANGE(r300, zs);
	reg_start(R300_RB3D_ZSTENCIL_CNTL_0, 2);
	
	{
	uint32_t t1, t2;
	
	t1 = r300->hw.zs.cmd[R300_ZS_CNTL_0];
	t2 = r300->hw.zs.cmd[R300_ZS_CNTL_1];
	
	if (flags & CLEARBUFFER_DEPTH) {
		t1 &= R300_RB3D_STENCIL_ENABLE;
		t1 |= 0x6; // test and write
		
		t2 &= ~(R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);
		t2 |= (R300_ZS_ALWAYS<<R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);
/*
		R300_STATECHANGE(r300, zb);
		r300->hw.zb.cmd[R300_ZB_OFFSET] =
			1024*4*300 +
			r300->radeon.radeonScreen->frontOffset +
			r300->radeon.radeonScreen->fbLocation;
		r300->hw.zb.cmd[R300_ZB_PITCH] =
			r300->radeon.radeonScreen->depthPitch;
*/
	} else {
		t1 &= R300_RB3D_STENCIL_ENABLE;
		t1 |= R300_RB3D_Z_DISABLED_1; // disable
		
		t2 &= ~(R300_ZS_MASK << R300_RB3D_ZS1_DEPTH_FUNC_SHIFT);
	}
	
	if (flags & CLEARBUFFER_STENCIL) {
		t1 &= ~R300_RB3D_STENCIL_ENABLE;
		t1 |= R300_RB3D_STENCIL_ENABLE;
		
		t2 &= 
		    ~((R300_ZS_MASK << R300_RB3D_ZS1_FRONT_FUNC_SHIFT) | (R300_ZS_MASK << R300_RB3D_ZS1_BACK_FUNC_SHIFT));
		t2 |= 
		    (R300_ZS_ALWAYS<<R300_RB3D_ZS1_FRONT_FUNC_SHIFT) | 
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_FRONT_FAIL_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_FRONT_ZPASS_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_FRONT_ZFAIL_OP_SHIFT) |
		    (R300_ZS_ALWAYS<<R300_RB3D_ZS1_BACK_FUNC_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_BACK_FAIL_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_BACK_ZPASS_OP_SHIFT) |
		    (R300_ZS_REPLACE<<R300_RB3D_ZS1_BACK_ZFAIL_OP_SHIFT) ;
	}
	
	e32(t1);
	e32(t2);
	e32(r300->state.stencil.clear);
	}
	
#endif

	cmd2 = (drm_r300_cmd_header_t*)r300AllocCmdBuf(r300, 9, __FUNCTION__);
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

}

#ifdef CB_DPATH
static void r300EmitClearState(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	r300ContextPtr rmesa=r300;
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	int i;
	LOCAL_VARS;
	
	R300_STATECHANGE(r300, vir[0]);
	reg_start(R300_VAP_INPUT_ROUTE_0_0, 0);
	e32(0x21030003);
	
	R300_STATECHANGE(r300, vir[1]);
	reg_start(R300_VAP_INPUT_ROUTE_1_0, 0);
	e32(0xF688F688);

	R300_STATECHANGE(r300, vic);
	reg_start(R300_VAP_INPUT_CNTL_0, 1);
	e32(0x00000001);
	e32(0x00000405);
	
	R300_STATECHANGE(r300, vof);
	reg_start(R300_VAP_OUTPUT_VTX_FMT_0, 1);
	e32(R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT | R300_VAP_OUTPUT_VTX_FMT_0__COLOR_PRESENT);
	e32(0); /* no textures */
		
	
	R300_STATECHANGE(r300, txe);
	reg_start(R300_TX_ENABLE, 0);
	e32(0);
	
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
	e32(0);
	
	R300_STATECHANGE(r300, bld);
	reg_start(R300_RB3D_CBLEND, 1);
	e32(0);
	e32(0);
	
	R300_STATECHANGE(r300, unk221C);
	reg_start(0x221C, 0);
	e32(R300_221C_CLEAR);
	
	R300_STATECHANGE(r300, ps);
	reg_start(R300_RE_POINTSIZE, 0);
	e32(((dPriv->w * 6) << R300_POINTSIZE_X_SHIFT) |
		((dPriv->h * 6) << R300_POINTSIZE_Y_SHIFT));
	
	R300_STATECHANGE(r300, ri);
	reg_start(R300_RS_INTERP_0, 8);
	for(i = 0; i < 8; ++i){
		e32(R300_RS_INTERP_USED);
	}

	R300_STATECHANGE(r300, rc);
	/* The second constant is needed to get glxgears display anything .. */
	reg_start(R300_RS_CNTL_0, 1);
	e32(R300_RS_CNTL_0_UNKNOWN_7 | R300_RS_CNTL_0_UNKNOWN_18);
	e32(0);
	
	R300_STATECHANGE(r300, rr);
	reg_start(R300_RS_ROUTE_0, 0);
	e32(0x00004000);
	
	R300_STATECHANGE(r300, fp);
	reg_start(R300_PFS_CNTL_0, 2);
	e32(0);
	e32(0);
	e32(0);
	reg_start(R300_PFS_NODE_0, 3);
	e32(0);
	e32(0);
	e32(0);
	e32(R300_PFS_NODE_LAST_NODE);
	
	R300_STATECHANGE(r300, fpi[0]);
	R300_STATECHANGE(r300, fpi[1]);
	R300_STATECHANGE(r300, fpi[2]);
	R300_STATECHANGE(r300, fpi[3]);
	
	reg_start(R300_PFS_INSTR0_0, 0);
	e32(FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO)));
	
	reg_start(R300_PFS_INSTR1_0, 0);
	e32(FP_SELC(0,NO,XYZ,FP_TMP(0),0,0));
	
	reg_start(R300_PFS_INSTR2_0, 0);
	e32(FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO)));
	
	reg_start(R300_PFS_INSTR3_0, 0);
	e32(FP_SELA(0,NO,W,FP_TMP(0),0,0));
	
	R300_STATECHANGE(r300, pvs);
	reg_start(R300_VAP_PVS_CNTL_1, 2);
	e32((0 << R300_PVS_CNTL_1_PROGRAM_START_SHIFT) |
		(0 << R300_PVS_CNTL_1_POS_END_SHIFT) |
		(1 << R300_PVS_CNTL_1_PROGRAM_END_SHIFT));
	e32(0);
	e32(1 << R300_PVS_CNTL_3_PROGRAM_UNKNOWN_SHIFT);
	
	R300_STATECHANGE(r300, vpi);
	vsf_start_fragment(0x0, 8);
	e32(VP_OUT(ADD,OUT,0,XYZW));
	e32(VP_IN(IN,0));
	e32(VP_ZERO());
	e32(0);
	
	e32(VP_OUT(ADD,OUT,1,XYZW));
	e32(VP_IN(IN,1));
	e32(VP_ZERO());
	e32(0);
	
}
#endif

/**
 * Buffer clear
 */
static void r300Clear(GLcontext * ctx, GLbitfield mask, GLboolean all,
		      GLint cx, GLint cy, GLint cw, GLint ch)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = r300->radeon.dri.drawable;
	int flags = 0;
	int bits = 0;
	int swapped;

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s:  all=%d cx=%d cy=%d cw=%d ch=%d\n",
			__FUNCTION__, all, cx, cy, cw, ch);

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
	
	if ( (mask & BUFFER_BIT_STENCIL) && r300->state.stencil.hw_stencil) {
		bits |= CLEARBUFFER_STENCIL;
		mask &= ~BUFFER_BIT_STENCIL;
	}

	if (mask) {
		if (RADEON_DEBUG & DEBUG_FALLBACKS)
			fprintf(stderr, "%s: swrast clear, mask: %x\n",
				__FUNCTION__, mask);
		_swrast_Clear(ctx, mask, all, cx, cy, cw, ch);
	}

	swapped = r300->radeon.doPageFlip && (r300->radeon.sarea->pfCurrentPage == 1);

#ifdef CB_DPATH
	if(flags || bits)
		r300EmitClearState(ctx);
#endif

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

#ifndef CB_DPATH
	/* Recalculate the hardware state. This could be done more efficiently,
	 * but do keep it like this for now.
	 */
	r300ResetHwState(r300);
	
	/* r300ClearBuffer has trampled all over the hardware state.. */
	r300->hw.all_dirty=GL_TRUE;
#endif
}

void r300Flush(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "%s\n", __FUNCTION__);

	if (r300->cmdbuf.count_used > r300->cmdbuf.count_reemit)
		r300FlushCmdBuf(r300, __FUNCTION__);
}

void r300RefillCurrentDmaRegion(r300ContextPtr rmesa)
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
			fprintf(stderr, "Error: Could not get dma buffer... exiting\n");
			exit(-1);
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
			fprintf(stderr, "%s -- DISCARD BUF %d\n", __FUNCTION__,
				region->buf->buf->idx);
		cmd =
		    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa,
								sizeof(*cmd) / 4,
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

/* Called via glXGetMemoryOffsetMESA() */
GLuint r300GetMemoryOffsetMESA(__DRInativeDisplay * dpy, int scrn,
			       const GLvoid * pointer)
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr rmesa;
	GLuint card_offset;

	if (!ctx || !(rmesa = R300_CONTEXT(ctx))) {
		fprintf(stderr, "%s: no context\n", __FUNCTION__);
		return ~0;
	}

	if (!r300IsGartMemory(rmesa, pointer, 0))
		return ~0;

	if (rmesa->radeon.dri.drmMinor < 6)
		return ~0;

	card_offset = r300GartOffsetFromVirtual(rmesa, pointer);

	return card_offset - rmesa->radeon.radeonScreen->gart_base;
}

GLboolean r300IsGartMemory(r300ContextPtr rmesa, const GLvoid * pointer,
			   GLint size)
{
	int offset =
	    (char *)pointer - (char *)rmesa->radeon.radeonScreen->gartTextures.map;
	int valid = (size >= 0 && offset >= 0
		     && offset + size < rmesa->radeon.radeonScreen->gartTextures.size);

	if (RADEON_DEBUG & DEBUG_IOCTL)
		fprintf(stderr, "r300IsGartMemory( %p ) : %d\n", pointer,
			valid);

	return valid;
}

GLuint r300GartOffsetFromVirtual(r300ContextPtr rmesa, const GLvoid * pointer)
{
	int offset =
	    (char *)pointer - (char *)rmesa->radeon.radeonScreen->gartTextures.map;

	//fprintf(stderr, "offset=%08x\n", offset);

	if (offset < 0 || offset > rmesa->radeon.radeonScreen->gartTextures.size)
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
