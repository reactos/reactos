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
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
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
#include "radeon_context.h"

#define USE_ARB_F_P 1

struct r300_context;
typedef struct r300_context r300ContextRec;
typedef struct r300_context *r300ContextPtr;

#include "radeon_lock.h"
#include "mm.h"

/* Checkpoint.. for convenience */
#define CPT	{ fprintf(stderr, "%s:%s line %d\n", __FILE__, __FUNCTION__, __LINE__); }
/* From http://gcc.gnu.org/onlinedocs/gcc-3.2.3/gcc/Variadic-Macros.html .
   I suppose we could inline this and use macro to fetch out __LINE__ and stuff in case we run into trouble 
   with other compilers ... GLUE!
*/
#if 1
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
#else
#define WARN_ONCE(a, ...) {}
#endif

typedef GLuint uint32_t;
typedef GLubyte uint8_t;
struct r300_fragment_program;

  /* We should probably change types within vertex_shader
      and pixel_shader structure later on */
#define CARD32 GLuint
#include "vertex_shader.h"
#if USE_ARB_F_P == 1
#include "r300_fragprog.h"
#else
#include "pixel_shader.h"
#endif
#undef CARD32

static __inline__ uint32_t r300PackFloat32(float fl)
{
	union { float fl; uint32_t u; } u;

	u.fl = fl;
	return u.u;
}


/************ DMA BUFFERS **************/

/* Need refcounting on dma buffers:
 */
struct r300_dma_buffer {
	int refcount;		/* the number of retained regions in buf */
	drmBufPtr buf;
};

#define GET_START(rvb) (rmesa->radeon.radeonScreen->gart_buffer_offset +		\
			(rvb)->address - rmesa->dma.buf0_address +	\
			(rvb)->start)

/* A retained region, eg vertices for indexed vertices.
 */
struct r300_dma_region {
	struct r300_dma_buffer *buf;
	char *address;		/* == buf->address */
	int start, end, ptr;	/* offsets from start of buf */

    int aos_offset;     /* address in GART memory */
    int aos_stride;     /* distance between elements, in dwords */
    int aos_size;       /* number of components (1-4) */
    int aos_format;     /* format of components */
    int aos_reg;        /* VAP register assignment */
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


	/* hardware register values */
	/* Note that R200 has 8 registers per texture and R300 only 7 */
	GLuint filter;
	GLuint pitch; /* one of the unknown registers.. unknown 1 ?*/
	GLuint size;	/* npot only */
	GLuint format;
	GLuint offset;	/* Image location in the card's address space.
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
};

struct r300_texture_env_state {
	r300TexObjPtr texobj;
	GLenum format;
	GLenum envMode;
};

#define R300_MAX_TEXTURE_UNITS 8

struct r300_texture_state {
	struct r300_texture_env_state unit[R300_MAX_TEXTURE_UNITS];
	int tc_count; /* number of incoming texture coordinates from VAP */
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
	const char* name;	/* for debug */
	int cmd_size;		/* maximum size in dwords */
	GLuint idx;		/* index in an array (e.g. textures) */
	uint32_t* cmd;
	GLboolean dirty;

	int (*check)(r300ContextPtr, struct r300_state_atom* atom);
};


#define R300_VPT_CMD_0		0
#define R300_VPT_XSCALE		1
#define R300_VPT_XOFFSET	2
#define R300_VPT_YSCALE		3
#define R300_VPT_YOFFSET	4
#define R300_VPT_ZSCALE		5
#define R300_VPT_ZOFFSET	6
#define R300_VPT_CMDSIZE	7

#define R300_VIR_CMD_0		0 /* vir is variable size (at least 1) */
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

#define R300_RR_CMD_0		0 /* rr is variable size (at least 1) */
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
#define R300_VPI_CMDSIZE	1025 /* 256 16 byte instructions */

#define R300_VPP_CMD_0		0
#define R300_VPP_PARAM_0	1
#define R300_VPP_CMDSIZE	1025 /* 256 4-component parameters */

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

	GLboolean	is_dirty;
	GLboolean	all_dirty;
	int		max_state_size;	/* in dwords */

	struct r300_state_atom vpt;	/* viewport (1D98) */
	struct r300_state_atom unk2080;	/* (2080) */
	struct r300_state_atom vof;     /* VAP output format register 0x2090 */
	struct r300_state_atom vte;	/* (20B0) */
	struct r300_state_atom unk2134;	/* (2134) */
	struct r300_state_atom unk2140;	/* (2140) */
	struct r300_state_atom vir[2];	/* vap input route (2150/21E0) */
	struct r300_state_atom vic;	/* vap input control (2180) */
	struct r300_state_atom unk21DC; /* (21DC) */
	struct r300_state_atom unk221C; /* (221C) */
	struct r300_state_atom unk2220; /* (2220) */
	struct r300_state_atom unk2288; /* (2288) */
	struct r300_state_atom pvs;	/* pvs_cntl (22D0) */
	struct r300_state_atom gb_enable; /* (4008) */
	struct r300_state_atom gb_misc; /* Multisampling position shifts ? (4010) */
	struct r300_state_atom unk4200; /* (4200) */
	struct r300_state_atom unk4214; /* (4214) */
	struct r300_state_atom ps;	/* pointsize (421C) */
	struct r300_state_atom unk4230; /* (4230) */
	struct r300_state_atom lcntl;	/* line control */
	struct r300_state_atom unk4260; /* (4260) */
	struct r300_state_atom unk4274; /* (4274) */
	struct r300_state_atom unk4288; /* (4288) */
	struct r300_state_atom unk42A0;	/* (42A0) */
	struct r300_state_atom zbs;	/* zbias (42A4) */
	struct r300_state_atom unk42B4; /* (42B4) */
	struct r300_state_atom cul;	/* cull cntl (42B8) */
	struct r300_state_atom unk42C0; /* (42C0) */
	struct r300_state_atom rc;	/* rs control (4300) */
	struct r300_state_atom ri;	/* rs interpolators (4310) */
	struct r300_state_atom rr;	/* rs route (4330) */
	struct r300_state_atom unk43A4;	/* (43A4) */
	struct r300_state_atom unk43E8;	/* (43E8) */
	struct r300_state_atom fp;	/* fragment program cntl + nodes (4600) */
	struct r300_state_atom fpt;     /* texi - (4620) */
	struct r300_state_atom unk46A4;	/* (46A4) */
	struct r300_state_atom fpi[4];	/* fp instructions (46C0/47C0/48C0/49C0) */
	struct r300_state_atom unk4BC0;	/* (4BC0) */
	struct r300_state_atom unk4BC8;	/* (4BC8) */
	struct r300_state_atom at;	/* alpha test (4BD4) */
	struct r300_state_atom unk4BD8;	/* (4BD8) */
	struct r300_state_atom fpp;     /* 0x4C00 and following */
	struct r300_state_atom unk4E00;	/* (4E00) */
	struct r300_state_atom bld;	/* blending (4E04) */
	struct r300_state_atom cmk;	/* colormask (4E0C) */
	struct r300_state_atom unk4E10;	/* (4E10) */
	struct r300_state_atom cb;	/* colorbuffer (4E28) */
	struct r300_state_atom unk4E50;	/* (4E50) */
	struct r300_state_atom unk4E88;	/* (4E88) */
	struct r300_state_atom unk4EA0;	/* (4E88) I saw it only written on RV350 hardware..  */
	struct r300_state_atom zs;	/* zstencil control (4F00) */
	struct r300_state_atom unk4F10;	/* (4F10) */
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
		struct r300_state_atom unknown1;
		struct r300_state_atom size;
		struct r300_state_atom format;
		struct r300_state_atom offset;
		struct r300_state_atom unknown4;
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
	int		size;		/* DWORDs allocated for buffer */
	uint32_t*	cmd_buf;
	int		count_used;	/* DWORDs filled so far */
	int		count_reemit;	/* size of re-emission batch */
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

struct r300_vap_reg_state {
	   /* input register assigments */
	   int i_coords;
	   int i_normal;
	   int i_color[2];
	   int i_fog;
	   int i_tex[R300_MAX_TEXTURE_UNITS];
	   int i_index;
	   int i_pointsize;
	};

/* Vertex shader state */

/* Tested with rv350 and verified from misc web pages. */
#define VSF_MAX_FRAGMENT_LENGTH (256*4)
	
/* Tested with rv350 and verified from misc web pages. */
#define VSF_MAX_FRAGMENT_TEMPS (32)


struct r300_vertex_shader_fragment {
	int length;
	union {
		GLuint d[VSF_MAX_FRAGMENT_LENGTH];
		float f[VSF_MAX_FRAGMENT_LENGTH];
		VERTEX_SHADER_INSTRUCTION i[VSF_MAX_FRAGMENT_LENGTH/4];
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

	/* a bit of a waste - each uses only a subset of allocated space..
	    but easier to program */
	struct r300_vertex_shader_fragment matrix[3];
	struct r300_vertex_shader_fragment vector[2];

	struct r300_vertex_shader_fragment unknown1;
	struct r300_vertex_shader_fragment unknown2;

	int program_start;
	int unknown_ptr1;  /* pointer within program space */
	int program_end;

	int param_offset;
	int param_count;

	int unknown_ptr2;  /* pointer within program space */
	int unknown_ptr3;  /* pointer within program space */
	};
	
extern int hw_tcl_on;

#define CURRENT_VERTEX_SHADER(ctx) (ctx->VertexProgram._Enabled ? ctx->VertexProgram.Current : ctx->_TnlProgram)

//#define TMU_ENABLED(ctx, unit) (hw_tcl_on ? ctx->Texture.Unit[unit]._ReallyEnabled && (OutputsWritten & (1<<(VERT_RESULT_TEX0+(unit)))) : 
//	(r300->state.render_inputs & (_TNL_BIT_TEX0<<(unit))))
#define TMU_ENABLED(ctx, unit) (hw_tcl_on ? ctx->Texture.Unit[unit]._ReallyEnabled && OutputsWritten & (1<<(VERT_RESULT_TEX0+(unit))) : \
	ctx->Texture.Unit[unit]._ReallyEnabled && r300->state.render_inputs & (_TNL_BIT_TEX0<<(unit)))

/* r300_vertex_shader_state and r300_vertex_program should probably be merged together someday.
 * Keeping them them seperate for now should ensure fixed pipeline keeps functioning properly.
 */	
struct r300_vertex_program {
	struct vertex_program mesa_program; /* Must be first */
	int translated;
	
	struct r300_vertex_shader_fragment program;
	struct r300_vertex_shader_fragment params;
	
	int pos_end;
	unsigned long num_temporaries; /* Number of temp vars used by program */
	int inputs[VERT_ATTRIB_MAX];
	int outputs[VERT_RESULT_MAX];
};

#if USE_ARB_F_P == 1
#define PFS_MAX_ALU_INST	64
#define PFS_MAX_TEX_INST	64
#define PFS_MAX_TEX_INDIRECT 4
#define PFS_NUM_TEMP_REGS	32
#define PFS_NUM_CONST_REGS	32
struct r300_fragment_program {
	struct fragment_program mesa_program;

	GLcontext *ctx;
	GLboolean translated;
	GLboolean error;

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
	int v_pos;
	int s_pos;

	struct {
		int tex_offset;
		int tex_end;
		int alu_offset;
		int alu_end;
	} node[4];
	int cur_node;
	int first_node_has_tex;

	int alu_offset;
	int alu_end;
	int tex_offset;
	int tex_end;

	/* Hardware constants */
	GLfloat constant[PFS_NUM_CONST_REGS][4];
	int const_nr;

	/* Tracked parameters */
	struct {
		int idx;			/* hardware index */
		GLfloat *values;	/* pointer to values */
	} param[PFS_NUM_CONST_REGS];
	int param_nr;
	GLboolean params_uptodate;
	
	GLuint temps[PFS_NUM_TEMP_REGS];
	int temp_in_use;
	GLuint used_in_node;
	GLuint dest_in_node;
	GLuint inputs[32]; /* don't actually need 32... */

	int hwreg_in_use;
	int max_temp_idx;
};

#else
/* 64 appears to be the maximum */
#define PSF_MAX_PROGRAM_LENGTH 64

struct r300_pixel_shader_program {
	struct {
		int length;
		GLuint inst[PSF_MAX_PROGRAM_LENGTH];
		} tex;

	/* ALU intructions (logic and integer) */
	struct {
		int length;
		struct {
			GLuint inst0;
			GLuint inst1;
			GLuint inst2;
			GLuint inst3;
			} inst[PSF_MAX_PROGRAM_LENGTH];
		} alu;

	/* node information */
	/* nodes are used to synchronize ALU and TEX streams */
	/* There could be up to 4 nodes each consisting of
	   a number of TEX instructions followed by some ALU
	   instructions */
	/* the last node of a program should always be node3 */
	struct {
		int tex_offset;
		int tex_end;
		int alu_offset;
		int alu_end;
		} node[4];

	int active_nodes;	/* must be between 1 and 4, inclusive */
	int first_node_has_tex;  /* other nodes always have it */

	int temp_register_count;  /* magic value goes into PFS_CNTL_1 */

	/* entire program */
	int tex_offset;
	int tex_end;
	int alu_offset;
	int alu_end;

	};

#define MAX_PIXEL_SHADER_PARAMS 32
struct r300_pixel_shader_state {
	struct r300_pixel_shader_program program;
	
	int translated;
	int have_sample;
	GLuint color_reg;
	GLuint src_previous;
	
	/* parameters */
	int param_length;  /* to limit the number of unnecessary writes */
	struct {
		float x;
		float y;
		float z;
		float w;
		} param[MAX_PIXEL_SHADER_PARAMS];
	};
#endif // USE_ARB_F_P

/* 8 is somewhat bogus... it is probably something like 24 */
#define R300_MAX_AOS_ARRAYS		16

#define AOS_FORMAT_FLOAT	1
#define AOS_FORMAT_UBYTE	2
#define AOS_FORMAT_FLOAT_COLOR	3

#define REG_COORDS	0
#define REG_COLOR0	1
#define REG_TEX0	2

struct r300_aos_rec {
	GLuint offset;
	int element_size; /* in dwords */
	int stride;       /* distance between elements, in dwords */

	int format;

	int ncomponents; /* number of components - between 1 and 4, inclusive */

	int reg; /* which register they are assigned to. */

	};

struct r300_state {
	struct r300_depthbuffer_state depth;
	struct r300_texture_state texture;
	struct r300_vap_reg_state vap_reg;
	struct r300_vertex_shader_state vertex_shader;
#if USE_ARB_F_P == 0
	struct r300_pixel_shader_state pixel_shader;
#endif
	struct r300_dma_region aos[R300_MAX_AOS_ARRAYS];
	int aos_count;

	GLuint *Elts;
	struct r300_dma_region elt_dma;
	
	GLuint render_inputs; /* actual render inputs that R300 was configured for. 
				 They are the same as tnl->render_inputs for fixed pipeline */	
	
	struct {
		int transform_offset;  /* Transform matrix offset, -1 if none */
		} vap_param;  /* vertex processor parameter allocation - tells where to write parameters */
	
	struct r300_stencilbuffer_state stencil;
	
};


/**
 * R300 context structure.
 */
struct r300_context {
	struct radeon_context radeon; /* parent class, must be first */

	struct r300_hw_state hw;
	struct r300_cmdbuf cmdbuf;
	struct r300_state state;

	/* Vertex buffers
	 */
	struct r300_dma dma;
	GLboolean save_on_next_unlock;

	/* Texture object bookkeeping
	 */
	unsigned nr_heaps;
	driTexHeap *texture_heaps[R200_NR_TEX_HEAPS];
	driTextureObject swapped;
	int texture_depth;
	float initialMaxAnisotropy;

	/* Clientdata textures;
	 */
	GLuint prefer_gart_client_texturing;

	/* TCL stuff
	 */
	GLmatrix TexGenMatrix[R300_MAX_TEXTURE_UNITS];
	GLboolean recheck_texgen[R300_MAX_TEXTURE_UNITS];
	GLboolean TexGenNeedNormals[R300_MAX_TEXTURE_UNITS];
	GLuint TexMatEnabled;
	GLuint TexMatCompSel;
	GLuint TexGenEnabled;
	GLuint TexGenInputs;
	GLuint TexGenCompSel;
	GLmatrix tmpmat;
};

#define R300_CONTEXT(ctx)		((r300ContextPtr)(ctx->DriverCtx))

static __inline GLuint r300PackColor( GLuint cpp,
					GLubyte r, GLubyte g,
					GLubyte b, GLubyte a )
{
   switch ( cpp ) {
   case 2:
      return PACK_COLOR_565( r, g, b );
   case 4:
      return PACK_COLOR_8888( r, g, b, a );
   default:
      return 0;
   }
}
extern void r300DestroyContext(__DRIcontextPrivate * driContextPriv);
extern GLboolean r300CreateContext(const __GLcontextModes * glVisual,
				   __DRIcontextPrivate * driContextPriv,
				   void *sharedContextPrivate);

void translate_vertex_shader(struct r300_vertex_program *vp);
extern void r300InitShaderFuncs(struct dd_function_table *functions);
extern void r300VertexProgUpdateParams(GLcontext *ctx, struct r300_vertex_program *vp);

#endif				/* __R300_CONTEXT_H__ */
