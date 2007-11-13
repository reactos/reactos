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
 * \author Keith Whitwell <keith@tungstengraphics.com>
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_CONTEXT_H__
#define __R300_CONTEXT_H__

#include "tnl/t_vertex.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "texmem.h"

#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#define USER_BUFFERS

//#define OPTIMIZE_ELTS

struct r300_context;
typedef struct r300_context r300ContextRec;
typedef struct r300_context *r300ContextPtr;

#include "radeon_lock.h"
#include "mm.h"

/* From http://gcc.gnu.org/onlinedocs/gcc-3.2.3/gcc/Variadic-Macros.html .
   I suppose we could inline this and use macro to fetch out __LINE__ and stuff in case we run into trouble
   with other compilers ... GLUE!
*/
#define WARN_ONCE(a, ...)	{ \
	static int warn##__LINE__=1; \
	if(warn##__LINE__){ \
		fprintf(stderr, "*********************************WARN_ONCE*********************************\n"); \
		fprintf(stderr, "File %s function %s line %d\n", \
			__FILE__, __FUNCTION__, __LINE__); \
		fprintf(stderr,  a, ## __VA_ARGS__);\
		fprintf(stderr, "***************************************************************************\n"); \
		warn##__LINE__=0;\
		} \
	}

#include "r300_vertprog.h"
#include "r300_fragprog.h"

/**
 * This function takes a float and packs it into a uint32_t
 */
static __inline__ uint32_t r300PackFloat32(float fl)
{
	union {
		float fl;
		uint32_t u;
	} u;

	u.fl = fl;
	return u.u;
}

/* This is probably wrong for some values, I need to test this
 * some more.  Range checking would be a good idea also..
 *
 * But it works for most things.  I'll fix it later if someone
 * else with a better clue doesn't
 */
static __inline__ uint32_t r300PackFloat24(float f)
{
	float mantissa;
	int exponent;
	uint32_t float24 = 0;

	if (f == 0.0)
		return 0;

	mantissa = frexpf(f, &exponent);

	/* Handle -ve */
	if (mantissa < 0) {
		float24 |= (1 << 23);
		mantissa = mantissa * -1.0;
	}
	/* Handle exponent, bias of 63 */
	exponent += 62;
	float24 |= (exponent << 16);
	/* Kill 7 LSB of mantissa */
	float24 |= (r300PackFloat32(mantissa) & 0x7FFFFF) >> 7;

	return float24;
}

/************ DMA BUFFERS **************/

/* Need refcounting on dma buffers:
 */
struct r300_dma_buffer {
	int refcount;		/**< the number of retained regions in buf */
	drmBufPtr buf;
	int id;
};
#undef GET_START
#ifdef USER_BUFFERS
#define GET_START(rvb) (r300GartOffsetFromVirtual(rmesa, (rvb)->address+(rvb)->start))
#else
#define GET_START(rvb) (rmesa->radeon.radeonScreen->gart_buffer_offset +		\
			(rvb)->address - rmesa->dma.buf0_address +	\
			(rvb)->start)
#endif
/* A retained region, eg vertices for indexed vertices.
 */
struct r300_dma_region {
	struct r300_dma_buffer *buf;
	char *address;		/* == buf->address */
	int start, end, ptr;	/* offsets from start of buf */

	int aos_offset;		/* address in GART memory */
	int aos_stride;		/* distance between elements, in dwords */
	int aos_size;		/* number of components (1-4) */
	int aos_reg;		/* VAP register assignment */
};

struct r300_dma {
	/* Active dma region.  Allocations for vertices and retained
	 * regions come from here.  Also used for emitting random vertices,
	 * these may be flushed by calling flush_current();
	 */
	struct r300_dma_region current;

	void (*flush) (r300ContextPtr);

	char *buf0_address;	/* start of buf[0], for index calcs */

	/* Number of "in-flight" DMA buffers, i.e. the number of buffers
	 * for which a DISCARD command is currently queued in the command buffer.
	 */
	GLuint nr_released_bufs;
};

       /* Texture related */

typedef struct r300_tex_obj r300TexObj, *r300TexObjPtr;

/* Texture object in locally shared texture space.
 */
struct r300_tex_obj {
	driTextureObject base;

	GLuint bufAddr;		/* Offset to start of locally
				   shared texture block */

	GLuint dirty_state;	/* Flags (1 per texunit) for
				   whether or not this texobj
				   has dirty hardware state
				   (pp_*) that needs to be
				   brought into the
				   texunit. */

	drm_radeon_tex_image_t image[6][RADEON_MAX_TEXTURE_LEVELS];
	/* Six, for the cube faces */

	GLboolean image_override;	/* Image overridden by GLX_EXT_tfp */

	GLuint pitch;		/* this isn't sent to hardware just used in calculations */
	/* hardware register values */
	/* Note that R200 has 8 registers per texture and R300 only 7 */
	GLuint filter;
	GLuint filter_1;
	GLuint pitch_reg;
	GLuint size;		/* npot only */
	GLuint format;
	GLuint offset;		/* Image location in the card's address space.
				   All cube faces follow. */
	GLuint unknown4;
	GLuint unknown5;
	/* end hardware registers */

	/* registers computed by r200 code - keep them here to
	   compare against what is actually written.

	   to be removed later.. */
	GLuint pp_border_color;
	GLuint pp_cubic_faces;	/* cube face 1,2,3,4 log2 sizes */
	GLuint format_x;

	GLboolean border_fallback;

	GLuint tile_bits;	/* hw texture tile bits used on this texture */
};

struct r300_texture_env_state {
	r300TexObjPtr texobj;
	GLenum format;
	GLenum envMode;
};

/* The blit width for texture uploads
 */
#define R300_BLIT_WIDTH_BYTES 1024
#define R300_MAX_TEXTURE_UNITS 8

struct r300_texture_state {
	struct r300_texture_env_state unit[R300_MAX_TEXTURE_UNITS];
	int tc_count;		/* number of incoming texture coordinates from VAP */
};

/**
 * A block of hardware state.
 *
 * When check returns non-zero, the returned number of dwords must be
 * copied verbatim into the command buffer in order to update a state atom
 * when it is dirty.
 */
struct r300_state_atom {
	struct r300_state_atom *next, *prev;
	const char *name;	/* for debug */
	int cmd_size;		/* maximum size in dwords */
	GLuint idx;		/* index in an array (e.g. textures) */
	uint32_t *cmd;
	GLboolean dirty;

	int (*check) (r300ContextPtr, struct r300_state_atom * atom);
};

#define R300_VPT_CMD_0		0
#define R300_VPT_XSCALE		1
#define R300_VPT_XOFFSET	2
#define R300_VPT_YSCALE		3
#define R300_VPT_YOFFSET	4
#define R300_VPT_ZSCALE		5
#define R300_VPT_ZOFFSET	6
#define R300_VPT_CMDSIZE	7

#define R300_VIR_CMD_0		0	/* vir is variable size (at least 1) */
#define R300_VIR_CNTL_0		1
#define R300_VIR_CNTL_1		2
#define R300_VIR_CNTL_2		3
#define R300_VIR_CNTL_3		4
#define R300_VIR_CNTL_4		5
#define R300_VIR_CNTL_5		6
#define R300_VIR_CNTL_6		7
#define R300_VIR_CNTL_7		8
#define R300_VIR_CMDSIZE	9

#define R300_VIC_CMD_0		0
#define R300_VIC_CNTL_0		1
#define R300_VIC_CNTL_1		2
#define R300_VIC_CMDSIZE	3

#define R300_VOF_CMD_0		0
#define R300_VOF_CNTL_0		1
#define R300_VOF_CNTL_1		2
#define R300_VOF_CMDSIZE	3

#define R300_PVS_CMD_0		0
#define R300_PVS_CNTL_1		1
#define R300_PVS_CNTL_2		2
#define R300_PVS_CNTL_3		3
#define R300_PVS_CMDSIZE	4

#define R300_GB_MISC_CMD_0		0
#define R300_GB_MISC_MSPOS_0		1
#define R300_GB_MISC_MSPOS_1		2
#define R300_GB_MISC_TILE_CONFIG	3
#define R300_GB_MISC_SELECT		4
#define R300_GB_MISC_AA_CONFIG		5
#define R300_GB_MISC_CMDSIZE		6

#define R300_TXE_CMD_0		0
#define R300_TXE_ENABLE		1
#define R300_TXE_CMDSIZE	2

#define R300_PS_CMD_0		0
#define R300_PS_POINTSIZE	1
#define R300_PS_CMDSIZE		2

#define R300_ZBS_CMD_0		0
#define R300_ZBS_T_FACTOR	1
#define R300_ZBS_T_CONSTANT	2
#define R300_ZBS_W_FACTOR	3
#define R300_ZBS_W_CONSTANT	4
#define R300_ZBS_CMDSIZE	5

#define R300_CUL_CMD_0		0
#define R300_CUL_CULL		1
#define R300_CUL_CMDSIZE	2

#define R300_RC_CMD_0		0
#define R300_RC_CNTL_0		1
#define R300_RC_CNTL_1		2
#define R300_RC_CMDSIZE		3

#define R300_RI_CMD_0		0
#define R300_RI_INTERP_0	1
#define R300_RI_INTERP_1	2
#define R300_RI_INTERP_2	3
#define R300_RI_INTERP_3	4
#define R300_RI_INTERP_4	5
#define R300_RI_INTERP_5	6
#define R300_RI_INTERP_6	7
#define R300_RI_INTERP_7	8
#define R300_RI_CMDSIZE		9

#define R300_RR_CMD_0		0	/* rr is variable size (at least 1) */
#define R300_RR_ROUTE_0		1
#define R300_RR_ROUTE_1		2
#define R300_RR_ROUTE_2		3
#define R300_RR_ROUTE_3		4
#define R300_RR_ROUTE_4		5
#define R300_RR_ROUTE_5		6
#define R300_RR_ROUTE_6		7
#define R300_RR_ROUTE_7		8
#define R300_RR_CMDSIZE		9

#define R300_FP_CMD_0		0
#define R300_FP_CNTL0		1
#define R300_FP_CNTL1		2
#define R300_FP_CNTL2		3
#define R300_FP_CMD_1		4
#define R300_FP_NODE0		5
#define R300_FP_NODE1		6
#define R300_FP_NODE2		7
#define R300_FP_NODE3		8
#define R300_FP_CMDSIZE		9

#define R300_FPT_CMD_0		0
#define R300_FPT_INSTR_0	1
#define R300_FPT_CMDSIZE	65

#define R300_FPI_CMD_0		0
#define R300_FPI_INSTR_0	1
#define R300_FPI_CMDSIZE	65

#define R300_FPP_CMD_0		0
#define R300_FPP_PARAM_0	1
#define R300_FPP_CMDSIZE	(32*4+1)

#define R300_FOGS_CMD_0		0
#define R300_FOGS_STATE		1
#define R300_FOGS_CMDSIZE	2

#define R300_FOGC_CMD_0		0
#define R300_FOGC_R		1
#define R300_FOGC_G		2
#define R300_FOGC_B		3
#define R300_FOGC_CMDSIZE	4

#define R300_FOGP_CMD_0		0
#define R300_FOGP_SCALE		1
#define R300_FOGP_START		2
#define R300_FOGP_CMDSIZE	3

#define R300_AT_CMD_0		0
#define R300_AT_ALPHA_TEST	1
#define R300_AT_UNKNOWN		2
#define R300_AT_CMDSIZE		3

#define R300_BLD_CMD_0		0
#define R300_BLD_CBLEND		1
#define R300_BLD_ABLEND		2
#define R300_BLD_CMDSIZE	3

#define R300_CMK_CMD_0		0
#define R300_CMK_COLORMASK	1
#define R300_CMK_CMDSIZE	2

#define R300_CB_CMD_0		0
#define R300_CB_OFFSET		1
#define R300_CB_CMD_1		2
#define R300_CB_PITCH		3
#define R300_CB_CMDSIZE		4

#define R300_ZS_CMD_0		0
#define R300_ZS_CNTL_0		1
#define R300_ZS_CNTL_1		2
#define R300_ZS_CNTL_2		3
#define R300_ZS_CMDSIZE		4

#define R300_ZB_CMD_0		0
#define R300_ZB_OFFSET		1
#define R300_ZB_PITCH		2
#define R300_ZB_CMDSIZE		3

#define R300_VPI_CMD_0		0
#define R300_VPI_INSTR_0	1
#define R300_VPI_CMDSIZE	1025	/* 256 16 byte instructions */

#define R300_VPP_CMD_0		0
#define R300_VPP_PARAM_0	1
#define R300_VPP_CMDSIZE	1025	/* 256 4-component parameters */

#define R300_VPS_CMD_0		0
#define R300_VPS_ZERO_0		1
#define R300_VPS_ZERO_1		2
#define R300_VPS_POINTSIZE	3
#define R300_VPS_ZERO_3		4
#define R300_VPS_CMDSIZE	5

	/* the layout is common for all fields inside tex */
#define R300_TEX_CMD_0		0
#define R300_TEX_VALUE_0	1
/* We don't really use this, instead specify mtu+1 dynamically
#define R300_TEX_CMDSIZE	(MAX_TEXTURE_UNITS+1)
*/

/**
 * Cache for hardware register state.
 */
struct r300_hw_state {
	struct r300_state_atom atomlist;

	GLboolean is_dirty;
	GLboolean all_dirty;
	int max_state_size;	/* in dwords */

	struct r300_state_atom vpt;	/* viewport (1D98) */
	struct r300_state_atom vap_cntl;
	struct r300_state_atom vof;	/* VAP output format register 0x2090 */
	struct r300_state_atom vte;	/* (20B0) */
	struct r300_state_atom unk2134;	/* (2134) */
	struct r300_state_atom vap_cntl_status;
	struct r300_state_atom vir[2];	/* vap input route (2150/21E0) */
	struct r300_state_atom vic;	/* vap input control (2180) */
	struct r300_state_atom unk21DC;	/* (21DC) */
	struct r300_state_atom unk221C;	/* (221C) */
	struct r300_state_atom unk2220;	/* (2220) */
	struct r300_state_atom unk2288;	/* (2288) */
	struct r300_state_atom pvs;	/* pvs_cntl (22D0) */
	struct r300_state_atom gb_enable;	/* (4008) */
	struct r300_state_atom gb_misc;	/* Multisampling position shifts ? (4010) */
	struct r300_state_atom unk4200;	/* (4200) */
	struct r300_state_atom unk4214;	/* (4214) */
	struct r300_state_atom ps;	/* pointsize (421C) */
	struct r300_state_atom unk4230;	/* (4230) */
	struct r300_state_atom lcntl;	/* line control */
	struct r300_state_atom unk4260;	/* (4260) */
	struct r300_state_atom shade;
	struct r300_state_atom polygon_mode;
	struct r300_state_atom fogp;	/* fog parameters (4294) */
	struct r300_state_atom unk429C;	/* (429C) */
	struct r300_state_atom zbias_cntl;
	struct r300_state_atom zbs;	/* zbias (42A4) */
	struct r300_state_atom occlusion_cntl;
	struct r300_state_atom cul;	/* cull cntl (42B8) */
	struct r300_state_atom unk42C0;	/* (42C0) */
	struct r300_state_atom rc;	/* rs control (4300) */
	struct r300_state_atom ri;	/* rs interpolators (4310) */
	struct r300_state_atom rr;	/* rs route (4330) */
	struct r300_state_atom unk43A4;	/* (43A4) */
	struct r300_state_atom unk43E8;	/* (43E8) */
	struct r300_state_atom fp;	/* fragment program cntl + nodes (4600) */
	struct r300_state_atom fpt;	/* texi - (4620) */
	struct r300_state_atom unk46A4;	/* (46A4) */
	struct r300_state_atom fpi[4];	/* fp instructions (46C0/47C0/48C0/49C0) */
	struct r300_state_atom fogs;	/* fog state (4BC0) */
	struct r300_state_atom fogc;	/* fog color (4BC8) */
	struct r300_state_atom at;	/* alpha test (4BD4) */
	struct r300_state_atom unk4BD8;	/* (4BD8) */
	struct r300_state_atom fpp;	/* 0x4C00 and following */
	struct r300_state_atom unk4E00;	/* (4E00) */
	struct r300_state_atom bld;	/* blending (4E04) */
	struct r300_state_atom cmk;	/* colormask (4E0C) */
	struct r300_state_atom blend_color;	/* constant blend color */
	struct r300_state_atom cb;	/* colorbuffer (4E28) */
	struct r300_state_atom unk4E50;	/* (4E50) */
	struct r300_state_atom unk4E88;	/* (4E88) */
	struct r300_state_atom unk4EA0;	/* (4E88) I saw it only written on RV350 hardware..  */
	struct r300_state_atom zs;	/* zstencil control (4F00) */
	struct r300_state_atom zstencil_format;
	struct r300_state_atom zb;	/* z buffer (4F20) */
	struct r300_state_atom unk4F28;	/* (4F28) */
	struct r300_state_atom unk4F30;	/* (4F30) */
	struct r300_state_atom unk4F44;	/* (4F44) */
	struct r300_state_atom unk4F54;	/* (4F54) */

	struct r300_state_atom vpi;	/* vp instructions */
	struct r300_state_atom vpp;	/* vp parameters */
	struct r300_state_atom vps;	/* vertex point size (?) */
	/* 8 texture units */
	/* the state is grouped by function and not by
	   texture unit. This makes single unit updates
	   really awkward - we are much better off
	   updating the whole thing at once */
	struct {
		struct r300_state_atom filter;
		struct r300_state_atom filter_1;
		struct r300_state_atom size;
		struct r300_state_atom format;
		struct r300_state_atom pitch;
		struct r300_state_atom offset;
		struct r300_state_atom chroma_key;
		struct r300_state_atom border_color;
	} tex;
	struct r300_state_atom txe;	/* tex enable (4104) */
};

/**
 * This structure holds the command buffer while it is being constructed.
 *
 * The first batch of commands in the buffer is always the state that needs
 * to be re-emitted when the context is lost. This batch can be skipped
 * otherwise.
 */
struct r300_cmdbuf {
	int size;		/* DWORDs allocated for buffer */
	uint32_t *cmd_buf;
	int count_used;		/* DWORDs filled so far */
	int count_reemit;	/* size of re-emission batch */
};

/**
 * State cache
 */

struct r300_depthbuffer_state {
	GLfloat scale;
};

struct r300_stencilbuffer_state {
	GLuint clear;
	GLboolean hw_stencil;

};

/* Vertex shader state */

/* Perhaps more if we store programs in vmem? */
/* drm_r300_cmd_header_t->vpu->count is unsigned char */
#define VSF_MAX_FRAGMENT_LENGTH (255*4)

/* Can be tested with colormat currently. */
#define VSF_MAX_FRAGMENT_TEMPS (14)

#define STATE_R300_WINDOW_DIMENSION (STATE_INTERNAL_DRIVER+0)
#define STATE_R300_TEXRECT_FACTOR (STATE_INTERNAL_DRIVER+1)

struct r300_vertex_shader_fragment {
	int length;
	union {
		GLuint d[VSF_MAX_FRAGMENT_LENGTH];
		float f[VSF_MAX_FRAGMENT_LENGTH];
		VERTEX_SHADER_INSTRUCTION i[VSF_MAX_FRAGMENT_LENGTH / 4];
	} body;
};

#define VSF_DEST_PROGRAM	0x0
#define VSF_DEST_MATRIX0	0x200
#define VSF_DEST_MATRIX1	0x204
#define VSF_DEST_MATRIX2	0x208
#define VSF_DEST_VECTOR0	0x20c
#define VSF_DEST_VECTOR1	0x20d
#define VSF_DEST_UNKNOWN1	0x400
#define VSF_DEST_UNKNOWN2	0x406

struct r300_vertex_shader_state {
	struct r300_vertex_shader_fragment program;

	struct r300_vertex_shader_fragment unknown1;
	struct r300_vertex_shader_fragment unknown2;

	int program_start;
	int unknown_ptr1;	/* pointer within program space */
	int program_end;

	int param_offset;
	int param_count;

	int unknown_ptr2;	/* pointer within program space */
	int unknown_ptr3;	/* pointer within program space */
};

extern int hw_tcl_on;

//#define CURRENT_VERTEX_SHADER(ctx) (ctx->VertexProgram._Current)
#define CURRENT_VERTEX_SHADER(ctx) (R300_CONTEXT(ctx)->selected_vp)

/* Should but doesnt work */
//#define CURRENT_VERTEX_SHADER(ctx) (R300_CONTEXT(ctx)->curr_vp)

/* r300_vertex_shader_state and r300_vertex_program should probably be merged together someday.
 * Keeping them them seperate for now should ensure fixed pipeline keeps functioning properly.
 */

struct r300_vertex_program_key {
	GLuint InputsRead;
	GLuint OutputsWritten;
};

struct r300_vertex_program {
	struct r300_vertex_program *next;
	struct r300_vertex_program_key key;
	int translated;

	struct r300_vertex_shader_fragment program;

	int pos_end;
	int num_temporaries;	/* Number of temp vars used by program */
	int wpos_idx;
	int inputs[VERT_ATTRIB_MAX];
	int outputs[VERT_RESULT_MAX];
	int native;
	int ref_count;
	int use_ref_count;
};

struct r300_vertex_program_cont {
	struct gl_vertex_program mesa_program;	/* Must be first */
	struct r300_vertex_shader_fragment params;
	struct r300_vertex_program *progs;
};

#define PFS_MAX_ALU_INST	64
#define PFS_MAX_TEX_INST	64
#define PFS_MAX_TEX_INDIRECT 4
#define PFS_NUM_TEMP_REGS	32
#define PFS_NUM_CONST_REGS	16

/* Mapping Mesa registers to R300 temporaries */
struct reg_acc {
	int reg;		/* Assigned hw temp */
	unsigned int refcount;	/* Number of uses by mesa program */
};

/**
 * Describe the current lifetime information for an R300 temporary
 */
struct reg_lifetime {
	/* Index of the first slot where this register is free in the sense
	   that it can be used as a new destination register.
	   This is -1 if the register has been assigned to a Mesa register
	   and the last access to the register has not yet been emitted */
	int free;

	/* Index of the first slot where this register is currently reserved.
	   This is used to stop e.g. a scalar operation from being moved
	   before the allocation time of a register that was first allocated
	   for a vector operation. */
	int reserved;

	/* Index of the first slot in which the register can be used as a
	   source without losing the value that is written by the last
	   emitted instruction that writes to the register */
	int vector_valid;
	int scalar_valid;

	/* Index to the slot where the register was last read.
	   This is also the first slot in which the register may be written again */
	int vector_lastread;
	int scalar_lastread;
};

/**
 * Store usage information about an ALU instruction slot during the
 * compilation of a fragment program.
 */
#define SLOT_SRC_VECTOR  (1<<0)
#define SLOT_SRC_SCALAR  (1<<3)
#define SLOT_SRC_BOTH    (SLOT_SRC_VECTOR | SLOT_SRC_SCALAR)
#define SLOT_OP_VECTOR   (1<<16)
#define SLOT_OP_SCALAR   (1<<17)
#define SLOT_OP_BOTH     (SLOT_OP_VECTOR | SLOT_OP_SCALAR)

struct r300_pfs_compile_slot {
	/* Bitmask indicating which parts of the slot are used, using SLOT_ constants
	   defined above */
	unsigned int used;

	/* Selected sources */
	int vsrc[3];
	int ssrc[3];
};

/**
 * Store information during compilation of fragment programs.
 */
struct r300_pfs_compile_state {
	int nrslots;		/* number of ALU slots used so far */

	/* Track which (parts of) slots are already filled with instructions */
	struct r300_pfs_compile_slot slot[PFS_MAX_ALU_INST];

	/* Track the validity of R300 temporaries */
	struct reg_lifetime hwtemps[PFS_NUM_TEMP_REGS];

	/* Used to map Mesa's inputs/temps onto hardware temps */
	int temp_in_use;
	struct reg_acc temps[PFS_NUM_TEMP_REGS];
	struct reg_acc inputs[32];	/* don't actually need 32... */

	/* Track usage of hardware temps, for register allocation,
	 * indirection detection, etc. */
	GLuint used_in_node;
	GLuint dest_in_node;
};

/**
 * Store everything about a fragment program that is needed
 * to render with that program.
 */
struct r300_fragment_program {
	struct gl_fragment_program mesa_program;

	GLcontext *ctx;
	GLboolean translated;
	GLboolean error;
	struct r300_pfs_compile_state *cs;

	struct {
		int length;
		GLuint inst[PFS_MAX_TEX_INST];
	} tex;

	struct {
		struct {
			GLuint inst0;
			GLuint inst1;
			GLuint inst2;
			GLuint inst3;
		} inst[PFS_MAX_ALU_INST];
	} alu;

	struct {
		int tex_offset;
		int tex_end;
		int alu_offset;
		int alu_end;
		int flags;
	} node[4];
	int cur_node;
	int first_node_has_tex;

	int alu_offset;
	int alu_end;
	int tex_offset;
	int tex_end;

	/* Hardware constants.
	 * Contains a pointer to the value. The destination of the pointer
	 * is supposed to be updated when GL state changes.
	 * Typically, this is either a pointer into
	 * gl_program_parameter_list::ParameterValues, or a pointer to a
	 * global constant (e.g. for sin/cos-approximation)
	 */
	const GLfloat *constant[PFS_NUM_CONST_REGS];
	int const_nr;

	int max_temp_idx;

	GLuint optimization;
};

#define R300_MAX_AOS_ARRAYS		16

#define AOS_FORMAT_USHORT	0
#define AOS_FORMAT_FLOAT	1
#define AOS_FORMAT_UBYTE	2
#define AOS_FORMAT_FLOAT_COLOR	3

#define REG_COORDS	0
#define REG_COLOR0	1
#define REG_TEX0	2

struct dt {
	GLint size;
	GLenum type;
	GLsizei stride;
	void *data;
};

struct radeon_vertex_buffer {
	int Count;
	void *Elts;
	int elt_size;
	int elt_min, elt_max;	/* debug */

	struct dt AttribPtr[VERT_ATTRIB_MAX];

	const struct _mesa_prim *Primitive;
	GLuint PrimitiveCount;
	GLint LockFirst;
	GLsizei LockCount;
	int lock_uptodate;
};

struct r300_state {
	struct r300_depthbuffer_state depth;
	struct r300_texture_state texture;
	int sw_tcl_inputs[VERT_ATTRIB_MAX];
	struct r300_vertex_shader_state vertex_shader;
	struct r300_pfs_compile_state pfs_compile;
	struct r300_dma_region aos[R300_MAX_AOS_ARRAYS];
	int aos_count;
	struct radeon_vertex_buffer VB;

	GLuint *Elts;
	struct r300_dma_region elt_dma;

	 DECLARE_RENDERINPUTS(render_inputs_bitset);	/* actual render inputs that R300 was configured for.
							   They are the same as tnl->render_inputs for fixed pipeline */

	struct {
		int transform_offset;	/* Transform matrix offset, -1 if none */
	} vap_param;		/* vertex processor parameter allocation - tells where to write parameters */

	struct r300_stencilbuffer_state stencil;

};

#define R300_FALLBACK_NONE 0
#define R300_FALLBACK_TCL 1
#define R300_FALLBACK_RAST 2

/**
 * \brief R300 context structure.
 */
struct r300_context {
	struct radeon_context radeon;	/* parent class, must be first */

	struct r300_hw_state hw;
	struct r300_cmdbuf cmdbuf;
	struct r300_state state;
	struct gl_vertex_program *curr_vp;
	struct r300_vertex_program *selected_vp;

	/* Vertex buffers
	 */
	struct r300_dma dma;
	GLboolean save_on_next_unlock;
	GLuint NewGLState;

	/* Texture object bookkeeping
	 */
	unsigned nr_heaps;
	driTexHeap *texture_heaps[RADEON_NR_TEX_HEAPS];
	driTextureObject swapped;
	int texture_depth;
	float initialMaxAnisotropy;

	/* Clientdata textures;
	 */
	GLuint prefer_gart_client_texturing;

#ifdef USER_BUFFERS
	struct r300_memory_manager *rmm;
#endif

	GLvector4f dummy_attrib[_TNL_ATTRIB_MAX];
	GLvector4f *temp_attrib[_TNL_ATTRIB_MAX];

	GLboolean disable_lowimpact_fallback;
};

struct r300_buffer_object {
	struct gl_buffer_object mesa_obj;
	int id;
};

#define R300_CONTEXT(ctx)		((r300ContextPtr)(ctx->DriverCtx))

extern void r300DestroyContext(__DRIcontextPrivate * driContextPriv);
extern GLboolean r300CreateContext(const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);

extern void r300SelectVertexShader(r300ContextPtr r300);
extern void r300InitShaderFuncs(struct dd_function_table *functions);
extern int r300VertexProgUpdateParams(GLcontext * ctx,
				      struct r300_vertex_program_cont *vp,
				      float *dst);

#define RADEON_D_CAPTURE 0
#define RADEON_D_PLAYBACK 1
#define RADEON_D_PLAYBACK_RAW 2
#define RADEON_D_T 3

#endif				/* __R300_CONTEXT_H__ */
