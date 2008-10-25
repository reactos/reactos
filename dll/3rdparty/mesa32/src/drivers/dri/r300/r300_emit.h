/*
 * Copyright (C) 2005 Vladimir Dergachev.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Vladimir Dergachev <volodya@mindspring.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 *   Aapo Tahkola <aet@rasterburn.org>
 *   Ben Skeggs <darktama@iinet.net.au>
 *   Jerome Glisse <j.glisse@gmail.com>
 */

/* This files defines functions for accessing R300 hardware.
 */
#ifndef __R300_EMIT_H__
#define __R300_EMIT_H__

#include "glheader.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "radeon_reg.h"

/*
 * CP type-3 packets
 */
#define RADEON_CP_PACKET3_UNK1B                     0xC0001B00
#define RADEON_CP_PACKET3_INDX_BUFFER               0xC0003300
#define RADEON_CP_PACKET3_3D_DRAW_VBUF_2            0xC0003400
#define RADEON_CP_PACKET3_3D_DRAW_IMMD_2            0xC0003500
#define RADEON_CP_PACKET3_3D_DRAW_INDX_2            0xC0003600
#define RADEON_CP_PACKET3_3D_LOAD_VBPNTR            0xC0002F00
#define RADEON_CP_PACKET3_3D_CLEAR_ZMASK            0xC0003202
#define RADEON_CP_PACKET3_3D_CLEAR_CMASK            0xC0003802
#define RADEON_CP_PACKET3_3D_CLEAR_HIZ              0xC0003702

#define CP_PACKET0(reg, n)	(RADEON_CP_PACKET0 | ((n)<<16) | ((reg)>>2))

static __inline__ uint32_t cmdpacket0(int reg, int count)
{
	drm_r300_cmd_header_t cmd;

	cmd.packet0.cmd_type = R300_CMD_PACKET0;
	cmd.packet0.count = count;
	cmd.packet0.reghi = ((unsigned int)reg & 0xFF00) >> 8;
	cmd.packet0.reglo = ((unsigned int)reg & 0x00FF);

	return cmd.u;
}

static __inline__ uint32_t cmdvpu(int addr, int count)
{
	drm_r300_cmd_header_t cmd;

	cmd.vpu.cmd_type = R300_CMD_VPU;
	cmd.vpu.count = count;
	cmd.vpu.adrhi = ((unsigned int)addr & 0xFF00) >> 8;
	cmd.vpu.adrlo = ((unsigned int)addr & 0x00FF);

	return cmd.u;
}

static __inline__ uint32_t cmdpacket3(int packet)
{
	drm_r300_cmd_header_t cmd;

	cmd.packet3.cmd_type = R300_CMD_PACKET3;
	cmd.packet3.packet = packet;

	return cmd.u;
}

static __inline__ uint32_t cmdcpdelay(unsigned short count)
{
	drm_r300_cmd_header_t cmd;

	cmd.delay.cmd_type = R300_CMD_CP_DELAY;
	cmd.delay.count = count;

	return cmd.u;
}

static __inline__ uint32_t cmdwait(unsigned char flags)
{
	drm_r300_cmd_header_t cmd;

	cmd.wait.cmd_type = R300_CMD_WAIT;
	cmd.wait.flags = flags;

	return cmd.u;
}

static __inline__ uint32_t cmdpacify(void)
{
	drm_r300_cmd_header_t cmd;

	cmd.header.cmd_type = R300_CMD_END3D;

	return cmd.u;
}

/**
 * Prepare to write a register value to register at address reg.
 * If num_extra > 0 then the following extra values are written
 * to registers with address +4, +8 and so on..
 */
#define reg_start(reg, num_extra)					\
	do {								\
		int _n;							\
		_n=(num_extra);						\
		cmd = (drm_radeon_cmd_header_t*)			\
			r300AllocCmdBuf(rmesa,				\
					(_n+2),				\
					__FUNCTION__);			\
		cmd_reserved=_n+2;					\
		cmd_written=1;						\
		cmd[0].i=cmdpacket0((reg), _n+1);			\
	} while (0);

/**
 * Emit GLuint freestyle
 */
#define e32(dword)							\
	do {								\
		if(cmd_written<cmd_reserved) {				\
			cmd[cmd_written].i=(dword);			\
			cmd_written++;					\
		} else {						\
			fprintf(stderr,					\
				"e32 but no previous packet "		\
				"declaration.\n"			\
				"Aborting! in %s::%s at line %d, "	\
				"cmd_written=%d cmd_reserved=%d\n",	\
				__FILE__, __FUNCTION__, __LINE__,	\
				cmd_written, cmd_reserved);		\
			_mesa_exit(-1);					\
		}							\
	} while(0)

#define	efloat(f) e32(r300PackFloat32(f))

#define vsf_start_fragment(dest, length)				\
	do {								\
		int _n;							\
		_n = (length);						\
		cmd = (drm_radeon_cmd_header_t*)			\
			r300AllocCmdBuf(rmesa,				\
					(_n+1),				\
					__FUNCTION__);			\
		cmd_reserved = _n+2;					\
		cmd_written =1;						\
		cmd[0].i = cmdvpu((dest), _n/4);			\
	} while (0);

#define start_packet3(packet, count)					\
	{								\
		int _n;							\
		GLuint _p;						\
		_n = (count);						\
		_p = (packet);						\
		cmd = (drm_radeon_cmd_header_t*)			\
			r300AllocCmdBuf(rmesa,				\
					(_n+3),				\
					__FUNCTION__);			\
		cmd_reserved = _n+3;					\
		cmd_written = 2;					\
		if(_n > 0x3fff) {					\
			fprintf(stderr,"Too big packet3 %08x: cannot "	\
				"store %d dwords\n",			\
				_p, _n);				\
			_mesa_exit(-1);					\
		}							\
		cmd[0].i = cmdpacket3(R300_CMD_PACKET3_RAW);		\
		cmd[1].i = _p | ((_n & 0x3fff)<<16);			\
	}

/**
 * Must be sent to switch to 2d commands
 */
void static inline end_3d(r300ContextPtr rmesa)
{
	drm_radeon_cmd_header_t *cmd = NULL;

	cmd =
	    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, 1, __FUNCTION__);
	cmd[0].header.cmd_type = R300_CMD_END3D;
}

void static inline cp_delay(r300ContextPtr rmesa, unsigned short count)
{
	drm_radeon_cmd_header_t *cmd = NULL;

	cmd =
	    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, 1, __FUNCTION__);
	cmd[0].i = cmdcpdelay(count);
}

void static inline cp_wait(r300ContextPtr rmesa, unsigned char flags)
{
	drm_radeon_cmd_header_t *cmd = NULL;

	cmd =
	    (drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, 1, __FUNCTION__);
	cmd[0].i = cmdwait(flags);
}

extern int r300EmitArrays(GLcontext * ctx);

#ifdef USER_BUFFERS
void r300UseArrays(GLcontext * ctx);
#endif

extern void r300ReleaseArrays(GLcontext * ctx);

#endif
