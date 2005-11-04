#ifndef __EMIT_H__
#define __EMIT_H__
#include "glheader.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"

/* convenience macros */
#define RADEON_CP_PACKET0                           0x00000000
#define RADEON_CP_PACKET1                           0x40000000
#define RADEON_CP_PACKET2                           0x80000000
#define RADEON_CP_PACKET3                           0xC0000000

#define RADEON_CP_PACKET3_NOP                       0xC0001000
#define RADEON_CP_PACKET3_NEXT_CHAR                 0xC0001900
#define RADEON_CP_PACKET3_UNK1B                     0xC0001B00
#define RADEON_CP_PACKET3_PLY_NEXTSCAN              0xC0001D00
#define RADEON_CP_PACKET3_SET_SCISSORS              0xC0001E00
#define RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM     0xC0002300
#define RADEON_CP_PACKET3_LOAD_MICROCODE            0xC0002400
#define RADEON_CP_PACKET3_WAIT_FOR_IDLE             0xC0002600
#define RADEON_CP_PACKET3_3D_DRAW_VBUF              0xC0002800
#define RADEON_CP_PACKET3_3D_DRAW_IMMD              0xC0002900
#define RADEON_CP_PACKET3_3D_DRAW_INDX              0xC0002A00
#define RADEON_CP_PACKET3_LOAD_PALETTE              0xC0002C00
#define RADEON_CP_PACKET3_INDX_BUFFER               0xC0003300
#define RADEON_CP_PACKET3_3D_DRAW_VBUF_2            0xC0003400
#define RADEON_CP_PACKET3_3D_DRAW_IMMD_2            0xC0003500
#define RADEON_CP_PACKET3_3D_DRAW_INDX_2            0xC0003600
#define RADEON_CP_PACKET3_3D_LOAD_VBPNTR            0xC0002F00
#define RADEON_CP_PACKET3_CNTL_PAINT                0xC0009100
#define RADEON_CP_PACKET3_CNTL_BITBLT               0xC0009200
#define RADEON_CP_PACKET3_CNTL_SMALLTEXT            0xC0009300
#define RADEON_CP_PACKET3_CNTL_HOSTDATA_BLT         0xC0009400
#define RADEON_CP_PACKET3_CNTL_POLYLINE             0xC0009500
#define RADEON_CP_PACKET3_CNTL_POLYSCANLINES        0xC0009800
#define RADEON_CP_PACKET3_CNTL_PAINT_MULTI          0xC0009A00
#define RADEON_CP_PACKET3_CNTL_BITBLT_MULTI         0xC0009B00
#define RADEON_CP_PACKET3_CNTL_TRANS_BITBLT         0xC0009C00
#define RADEON_CP_PACKET3_3D_CLEAR_ZMASK            0xC0003202
#define RADEON_CP_PACKET3_3D_CLEAR_CMASK            0xC0003802
#define RADEON_CP_PACKET3_3D_CLEAR_HIZ              0xC0003702

#define CP_PACKET0(reg, n)	(RADEON_CP_PACKET0 | ((n)<<16) | ((reg)>>2))

/* Glue to R300 Mesa driver */
#define LOCAL_VARS int cmd_reserved=0;\
	int cmd_written=0; \
	drm_radeon_cmd_header_t *cmd=NULL;

#define PREFIX_VOID r300ContextPtr rmesa

#define PREFIX PREFIX_VOID ,

#define PASS_PREFIX_VOID rmesa
#define PASS_PREFIX rmesa ,

typedef GLuint CARD32;

/* This files defines functions for accessing R300 hardware.
   It needs to be customized to whatever code r300_lib.c is used
   in */

void static inline check_space(int dwords)
{
}

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

/* Prepare to write a register value to register at address reg.
   If num_extra > 0 then the following extra values are written
   to registers with address +4, +8 and so on.. */
#define reg_start(reg, num_extra) \
	{ \
	int _n; \
	_n=(num_extra); \
	cmd=(drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, \
					(_n+2), \
					__FUNCTION__); \
	cmd_reserved=_n+2; \
	cmd_written=1; \
	cmd[0].i=cmdpacket0((reg), _n+1); \
	}

/* Prepare to write a register value to register at address reg.
   If num_extra > 0 then the following extra values are written
   into the same register. */
/* It is here to permit r300_lib to compile and link anyway, but
   complain if actually called */
#define reg_start_pump(reg, num_extra) \
	{ \
	fprintf(stderr, "I am not defined.. Error ! in %s::%s at line %d\n", \
			__FILE__, __FUNCTION__, __LINE__); \
	exit(-1); \
	}

/* Emit CARD32 freestyle*/
#define e32(dword)   { \
	if(cmd_written<cmd_reserved){\
		cmd[cmd_written].i=(dword); \
		cmd_written++; \
		} else { \
		fprintf(stderr, "e32 but no previous packet declaration.. Aborting! in %s::%s at line %d, cmd_written=%d cmd_reserved=%d\n", \
			__FILE__, __FUNCTION__, __LINE__, cmd_written, cmd_reserved); \
		exit(-1); \
		} \
	}

#define	efloat(f) e32(r300PackFloat32(f))

#define vsf_start_fragment(dest, length)  \
	{ \
	int _n; \
	_n=(length); \
	cmd=(drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, \
					(_n+1), \
					__FUNCTION__); \
	cmd_reserved=_n+2; \
	cmd_written=1; \
	cmd[0].i=cmdvpu((dest), _n/4); \
	}

#define start_packet3(packet, count)	\
	{ \
	int _n; \
	CARD32 _p; \
	_n=(count); \
	_p=(packet); \
	cmd=(drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, \
					(_n+3), \
					__FUNCTION__); \
	cmd_reserved=_n+3; \
	cmd_written=2; \
	if(_n>0x3fff) {\
		fprintf(stderr,"Too big packet3 %08x: cannot store %d dwords\n", \
			_p, _n); \
		exit(-1); \
		} \
	cmd[0].i=cmdpacket3(R300_CMD_PACKET3_RAW); \
	cmd[1].i=_p | ((_n & 0x3fff)<<16); \
	}

	/* must be sent to switch to 2d commands */

void static inline end_3d(PREFIX_VOID)
{
LOCAL_VARS
(void)cmd_reserved; (void)cmd_written;

cmd=(drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, \
					1, \
					__FUNCTION__); \

cmd[0].header.cmd_type=R300_CMD_END3D;
}

void static inline cp_delay(PREFIX unsigned short count)
{
LOCAL_VARS
(void)cmd_reserved; (void)cmd_written;

cmd=(drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, \
					1, \
					__FUNCTION__); \

cmd[0].i=cmdcpdelay(count);
}

void static inline cp_wait(PREFIX unsigned char flags)
{
LOCAL_VARS
(void)cmd_reserved; (void)cmd_written;

cmd=(drm_radeon_cmd_header_t *) r300AllocCmdBuf(rmesa, \
					1, \
					__FUNCTION__); \

cmd[0].i=cmdwait(flags);
}

/* fire vertex buffer */
static void inline fire_AOS(PREFIX int vertex_count, int type)
{
LOCAL_VARS
check_space(9);

start_packet3(RADEON_CP_PACKET3_3D_DRAW_VBUF_2, 0);
/*	e32(0x840c0024);  */
	e32(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (vertex_count<<16) | type);
}

/* these are followed by the corresponding data */
#define start_index32_packet(vertex_count, type) \
	{\
	int _vc;\
	_vc=(vertex_count); \
	start_packet3(RADEON_CP_PACKET3_3D_DRAW_INDX_2, _vc); \
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (_vc<<16) | type \
		    | R300_VAP_VF_CNTL__INDEX_SIZE_32bit); \
	}

#define start_index16_packet(vertex_count, type) \
	{\
	int _vc, _n;\
	_vc=(vertex_count); \
	_n=(vertex_count+1)>>1; \
	start_packet3(RADEON_CP_PACKET3_3D_DRAW_INDX_2, _n); \
		e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (_vc<<16) | type); \
	}

/* Interestingly enough this ones needs the call to setup_AOS, even thought
   some of the data so setup is not needed and some is not as arbitrary
   as when used by DRAW_VBUF_2 or DRAW_INDX_2 */
#define start_immediate_packet(vertex_count, type, vertex_size) \
	{\
	int _vc; \
	_vc=(vertex_count); \
	start_packet3(RADEON_CP_PACKET3_3D_DRAW_IMMD_2, _vc*(vertex_size)); \
	e32(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_EMBEDDED | (_vc<<16) | type); \
	}

#endif
