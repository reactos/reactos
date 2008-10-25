/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_context.h,v 1.2 2002/12/16 16:18:54 dawes Exp $ */
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
 */

#ifndef __R200_CONTEXT_H__
#define __R200_CONTEXT_H__

#include "tnl/t_vertex.h"
#include "drm.h"
#include "radeon_drm.h"
#include "dri_util.h"
#include "texmem.h"

#include "macros.h"
#include "mtypes.h"
#include "colormac.h"
#include "r200_reg.h"
#include "r200_vertprog.h"

#define ENABLE_HW_3D_TEXTURE 1  /* XXX this is temporary! */

#ifndef R200_EMIT_VAP_PVS_CNTL
#error This driver requires a newer libdrm to compile
#endif

struct r200_context;
typedef struct r200_context r200ContextRec;
typedef struct r200_context *r200ContextPtr;

/* This union is used to avoid warnings/miscompilation
   with float to uint32_t casts due to strict-aliasing */
typedef union { GLfloat f; uint32_t ui32; } float_ui32_type;

#include "r200_lock.h"
#include "radeon_screen.h"
#include "mm.h"

/* Flags for software fallback cases */
/* See correponding strings in r200_swtcl.c */
#define R200_FALLBACK_TEXTURE           0x01
#define R200_FALLBACK_DRAW_BUFFER       0x02
#define R200_FALLBACK_STENCIL           0x04
#define R200_FALLBACK_RENDER_MODE       0x08
#define R200_FALLBACK_DISABLE           0x10
#define R200_FALLBACK_BORDER_MODE       0x20

/* The blit width for texture uploads
 */
#define BLIT_WIDTH_BYTES 1024

/* Use the templated vertex format:
 */
#define COLOR_IS_RGBA
#define TAG(x) r200##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*r200_tri_func)( r200ContextPtr,
				 r200Vertex *,
				 r200Vertex *,
				 r200Vertex * );

typedef void (*r200_line_func)( r200ContextPtr,
				  r200Vertex *,
				  r200Vertex * );

typedef void (*r200_point_func)( r200ContextPtr,
				   r200Vertex * );


struct r200_vertex_program {
        struct gl_vertex_program mesa_program; /* Must be first */
        int translated;
        /* need excess instr: 1 for late loop checking, 2 for 
           additional instr due to instr/attr, 3 for fog */
        VERTEX_SHADER_INSTRUCTION instr[R200_VSF_MAX_INST + 6];
        int pos_end;
        int inputs[VERT_ATTRIB_MAX];
        GLubyte inputmap_rev[16];
        int native;
        int fogpidx;
        int fogmode;
};

struct r200_colorbuffer_state {
   GLuint clear;
#if 000
   GLint drawOffset, drawPitch;
#endif
   int roundEnable;
};


struct r200_depthbuffer_state {
   GLuint clear;
   GLfloat scale;
};

#if 000
struct r200_pixel_state {
   GLint readOffset, readPitch;
};
#endif

struct r200_scissor_state {
   drm_clip_rect_t rect;
   GLboolean enabled;

   GLuint numClipRects;			/* Cliprects active */
   GLuint numAllocedClipRects;		/* Cliprects available */
   drm_clip_rect_t *pClipRects;
};

struct r200_stencilbuffer_state {
   GLboolean hwBuffer;
   GLuint clear;			/* rb3d_stencilrefmask value */
};

struct r200_stipple_state {
   GLuint mask[32];
};



#define TEX_0   0x1
#define TEX_1   0x2
#define TEX_2	0x4
#define TEX_3	0x8
#define TEX_4	0x10
#define TEX_5	0x20
#define TEX_ALL 0x3f

typedef struct r200_tex_obj r200TexObj, *r200TexObjPtr;

/* Texture object in locally shared texture space.
 */
struct r200_tex_obj {
   driTextureObject   base;

   GLuint bufAddr;			/* Offset to start of locally
					   shared texture block */

   GLuint dirty_state;		        /* Flags (1 per texunit) for
					   whether or not this texobj
					   has dirty hardware state
					   (pp_*) that needs to be
					   brought into the
					   texunit. */

   drm_radeon_tex_image_t image[6][RADEON_MAX_TEXTURE_LEVELS];
					/* Six, for the cube faces */
   GLboolean image_override;		/* Image overridden by GLX_EXT_tfp */

   GLuint pp_txfilter;		        /* hardware register values */
   GLuint pp_txformat;
   GLuint pp_txformat_x;
   GLuint pp_txoffset;		        /* Image location in texmem.
					   All cube faces follow. */
   GLuint pp_txsize;		        /* npot only */
   GLuint pp_txpitch;		        /* npot only */
   GLuint pp_border_color;
   GLuint pp_cubic_faces;	        /* cube face 1,2,3,4 log2 sizes */

   GLboolean  border_fallback;

   GLuint tile_bits;			/* hw texture tile bits used on this texture */
};


struct r200_texture_env_state {
   r200TexObjPtr texobj;
   GLuint outputreg;
   GLuint unitneeded;
};

#define R200_MAX_TEXTURE_UNITS 6

struct r200_texture_state {
   struct r200_texture_env_state unit[R200_MAX_TEXTURE_UNITS];
};


struct r200_state_atom {
   struct r200_state_atom *next, *prev;
   const char *name;		         /* for debug */
   int cmd_size;		         /* size in bytes */
   GLuint idx;
   int *cmd;			         /* one or more cmd's */
   int *lastcmd;			 /* one or more cmd's */
   GLboolean dirty;
   GLboolean (*check)( GLcontext *, int );    /* is this state active? */
};
   


/* Trying to keep these relatively short as the variables are becoming
 * extravagently long.  Drop the driver name prefix off the front of
 * everything - I think we know which driver we're in by now, and keep the
 * prefix to 3 letters unless absolutely impossible.  
 */

#define CTX_CMD_0             0
#define CTX_PP_MISC           1
#define CTX_PP_FOG_COLOR      2
#define CTX_RE_SOLID_COLOR    3
#define CTX_RB3D_BLENDCNTL    4
#define CTX_RB3D_DEPTHOFFSET  5
#define CTX_RB3D_DEPTHPITCH   6
#define CTX_RB3D_ZSTENCILCNTL 7
#define CTX_CMD_1             8
#define CTX_PP_CNTL           9
#define CTX_RB3D_CNTL         10
#define CTX_RB3D_COLOROFFSET  11
#define CTX_CMD_2             12 /* why */
#define CTX_RB3D_COLORPITCH   13 /* why */
#define CTX_STATE_SIZE_OLDDRM 14
#define CTX_CMD_3             14
#define CTX_RB3D_BLENDCOLOR   15
#define CTX_RB3D_ABLENDCNTL   16
#define CTX_RB3D_CBLENDCNTL   17
#define CTX_STATE_SIZE_NEWDRM 18

#define SET_CMD_0               0
#define SET_SE_CNTL             1
#define SET_RE_CNTL             2 /* replace se_coord_fmt */
#define SET_STATE_SIZE          3

#define VTE_CMD_0               0
#define VTE_SE_VTE_CNTL         1
#define VTE_STATE_SIZE          2

#define LIN_CMD_0               0
#define LIN_RE_LINE_PATTERN     1
#define LIN_RE_LINE_STATE       2
#define LIN_CMD_1               3
#define LIN_SE_LINE_WIDTH       4
#define LIN_STATE_SIZE          5

#define MSK_CMD_0               0
#define MSK_RB3D_STENCILREFMASK 1
#define MSK_RB3D_ROPCNTL        2
#define MSK_RB3D_PLANEMASK      3
#define MSK_STATE_SIZE          4

#define VPT_CMD_0           0
#define VPT_SE_VPORT_XSCALE          1
#define VPT_SE_VPORT_XOFFSET         2
#define VPT_SE_VPORT_YSCALE          3
#define VPT_SE_VPORT_YOFFSET         4
#define VPT_SE_VPORT_ZSCALE          5
#define VPT_SE_VPORT_ZOFFSET         6
#define VPT_STATE_SIZE      7

#define ZBS_CMD_0               0
#define ZBS_SE_ZBIAS_FACTOR     1
#define ZBS_SE_ZBIAS_CONSTANT   2
#define ZBS_STATE_SIZE          3

#define MSC_CMD_0               0
#define MSC_RE_MISC             1
#define MSC_STATE_SIZE          2

#define TAM_CMD_0               0
#define TAM_DEBUG3              1
#define TAM_STATE_SIZE          2

#define TEX_CMD_0                   0
#define TEX_PP_TXFILTER             1  /*2c00*/
#define TEX_PP_TXFORMAT             2  /*2c04*/
#define TEX_PP_TXFORMAT_X           3  /*2c08*/
#define TEX_PP_TXSIZE               4  /*2c0c*/
#define TEX_PP_TXPITCH              5  /*2c10*/
#define TEX_PP_BORDER_COLOR         6  /*2c14*/
#define TEX_CMD_1_OLDDRM            7
#define TEX_PP_TXOFFSET_OLDDRM      8  /*2d00 */
#define TEX_STATE_SIZE_OLDDRM       9
#define TEX_PP_CUBIC_FACES          7
#define TEX_PP_TXMULTI_CTL          8
#define TEX_CMD_1_NEWDRM            9
#define TEX_PP_TXOFFSET_NEWDRM     10
#define TEX_STATE_SIZE_NEWDRM      11

#define CUBE_CMD_0                  0  /* 1 register follows */ /* this command unnecessary */
#define CUBE_PP_CUBIC_FACES         1  /* 0x2c18 */             /* with new enough drm */
#define CUBE_CMD_1                  2  /* 5 registers follow */
#define CUBE_PP_CUBIC_OFFSET_F1     3  /* 0x2d04 */
#define CUBE_PP_CUBIC_OFFSET_F2     4  /* 0x2d08 */
#define CUBE_PP_CUBIC_OFFSET_F3     5  /* 0x2d0c */
#define CUBE_PP_CUBIC_OFFSET_F4     6  /* 0x2d10 */
#define CUBE_PP_CUBIC_OFFSET_F5     7  /* 0x2d14 */
#define CUBE_STATE_SIZE             8

#define PIX_CMD_0                   0
#define PIX_PP_TXCBLEND             1
#define PIX_PP_TXCBLEND2            2
#define PIX_PP_TXABLEND             3
#define PIX_PP_TXABLEND2            4
#define PIX_STATE_SIZE              5

#define TF_CMD_0                    0
#define TF_TFACTOR_0                1
#define TF_TFACTOR_1                2
#define TF_TFACTOR_2                3
#define TF_TFACTOR_3                4
#define TF_TFACTOR_4                5
#define TF_TFACTOR_5                6
#define TF_STATE_SIZE               7

#define ATF_CMD_0                   0
#define ATF_TFACTOR_0               1
#define ATF_TFACTOR_1               2
#define ATF_TFACTOR_2               3
#define ATF_TFACTOR_3               4
#define ATF_TFACTOR_4               5
#define ATF_TFACTOR_5               6
#define ATF_TFACTOR_6               7
#define ATF_TFACTOR_7               8
#define ATF_STATE_SIZE              9

/* ATI_FRAGMENT_SHADER */
#define AFS_CMD_0                 0
#define AFS_IC0                   1 /* 2f00 */
#define AFS_IC1                   2 /* 2f04 */
#define AFS_IA0                   3 /* 2f08 */
#define AFS_IA1                   4 /* 2f0c */
#define AFS_STATE_SIZE           33

#define PVS_CMD_0                 0
#define PVS_CNTL_1                1
#define PVS_CNTL_2                2
#define PVS_STATE_SIZE            3

/* those are quite big... */
#define VPI_CMD_0                 0
#define VPI_OPDST_0               1
#define VPI_SRC0_0                2
#define VPI_SRC1_0                3
#define VPI_SRC2_0                4
#define VPI_OPDST_63              253
#define VPI_SRC0_63               254
#define VPI_SRC1_63               255
#define VPI_SRC2_63               256
#define VPI_STATE_SIZE            257

#define VPP_CMD_0                0
#define VPP_PARAM0_0             1
#define VPP_PARAM1_0             2
#define VPP_PARAM2_0             3
#define VPP_PARAM3_0             4
#define VPP_PARAM0_95            381
#define VPP_PARAM1_95            382
#define VPP_PARAM2_95            383
#define VPP_PARAM3_95            384
#define VPP_STATE_SIZE           385

#define TCL_CMD_0                 0
#define TCL_LIGHT_MODEL_CTL_0     1
#define TCL_LIGHT_MODEL_CTL_1     2
#define TCL_PER_LIGHT_CTL_0       3
#define TCL_PER_LIGHT_CTL_1       4
#define TCL_PER_LIGHT_CTL_2       5
#define TCL_PER_LIGHT_CTL_3       6
#define TCL_CMD_1                 7
#define TCL_UCP_VERT_BLEND_CTL    8
#define TCL_STATE_SIZE            9

#define MSL_CMD_0                     0
#define MSL_MATRIX_SELECT_0           1
#define MSL_MATRIX_SELECT_1           2
#define MSL_MATRIX_SELECT_2           3
#define MSL_MATRIX_SELECT_3           4
#define MSL_MATRIX_SELECT_4           5
#define MSL_STATE_SIZE                6

#define TCG_CMD_0                 0
#define TCG_TEX_PROC_CTL_2            1
#define TCG_TEX_PROC_CTL_3            2
#define TCG_TEX_PROC_CTL_0            3
#define TCG_TEX_PROC_CTL_1            4
#define TCG_TEX_CYL_WRAP_CTL      5
#define TCG_STATE_SIZE            6

#define MTL_CMD_0            0	
#define MTL_EMMISSIVE_RED    1	
#define MTL_EMMISSIVE_GREEN  2	
#define MTL_EMMISSIVE_BLUE   3	
#define MTL_EMMISSIVE_ALPHA  4	
#define MTL_AMBIENT_RED      5
#define MTL_AMBIENT_GREEN    6
#define MTL_AMBIENT_BLUE     7
#define MTL_AMBIENT_ALPHA    8
#define MTL_DIFFUSE_RED      9
#define MTL_DIFFUSE_GREEN    10
#define MTL_DIFFUSE_BLUE     11
#define MTL_DIFFUSE_ALPHA    12
#define MTL_SPECULAR_RED     13
#define MTL_SPECULAR_GREEN   14
#define MTL_SPECULAR_BLUE    15
#define MTL_SPECULAR_ALPHA   16
#define MTL_CMD_1            17
#define MTL_SHININESS        18
#define MTL_STATE_SIZE       19

#define VAP_CMD_0                   0
#define VAP_SE_VAP_CNTL             1
#define VAP_STATE_SIZE              2

/* Replaces a lot of packet info from radeon
 */
#define VTX_CMD_0                   0
#define VTX_VTXFMT_0            1
#define VTX_VTXFMT_1            2
#define VTX_TCL_OUTPUT_VTXFMT_0 3
#define VTX_TCL_OUTPUT_VTXFMT_1 4
#define VTX_CMD_1               5
#define VTX_TCL_OUTPUT_COMPSEL  6
#define VTX_CMD_2               7
#define VTX_STATE_CNTL          8
#define VTX_STATE_SIZE          9

/* SPR - point sprite state
 */
#define SPR_CMD_0              0
#define SPR_POINT_SPRITE_CNTL  1
#define SPR_STATE_SIZE         2

#define PTP_CMD_0              0
#define PTP_VPORT_SCALE_0      1
#define PTP_VPORT_SCALE_1      2
#define PTP_VPORT_SCALE_PTSIZE 3
#define PTP_VPORT_SCALE_3      4
#define PTP_CMD_1              5
#define PTP_ATT_CONST_QUAD     6
#define PTP_ATT_CONST_LIN      7
#define PTP_ATT_CONST_CON      8
#define PTP_ATT_CONST_3        9
#define PTP_EYE_X             10
#define PTP_EYE_Y             11
#define PTP_EYE_Z             12
#define PTP_EYE_3             13
#define PTP_CLAMP_MIN         14
#define PTP_CLAMP_MAX         15
#define PTP_CLAMP_2           16
#define PTP_CLAMP_3           17
#define PTP_STATE_SIZE        18

#define VTX_COLOR(v,n)   (((v)>>(R200_VTX_COLOR_0_SHIFT+(n)*2))&\
                         R200_VTX_COLOR_MASK)

/**
 * Given the \c R200_SE_VTX_FMT_1 for the current vertex state, determine
 * how many components are in texture coordinate \c n.
 */
#define VTX_TEXn_COUNT(v,n)   (((v) >> (3 * n)) & 0x07)

#define MAT_CMD_0              0
#define MAT_ELT_0              1
#define MAT_STATE_SIZE         17

#define GRD_CMD_0                  0
#define GRD_VERT_GUARD_CLIP_ADJ    1
#define GRD_VERT_GUARD_DISCARD_ADJ 2
#define GRD_HORZ_GUARD_CLIP_ADJ    3
#define GRD_HORZ_GUARD_DISCARD_ADJ 4
#define GRD_STATE_SIZE             5

/* position changes frequently when lighting in modelpos - separate
 * out to new state item?  
 */
#define LIT_CMD_0                  0
#define LIT_AMBIENT_RED            1
#define LIT_AMBIENT_GREEN          2
#define LIT_AMBIENT_BLUE           3
#define LIT_AMBIENT_ALPHA          4
#define LIT_DIFFUSE_RED            5
#define LIT_DIFFUSE_GREEN          6
#define LIT_DIFFUSE_BLUE           7
#define LIT_DIFFUSE_ALPHA          8
#define LIT_SPECULAR_RED           9
#define LIT_SPECULAR_GREEN         10
#define LIT_SPECULAR_BLUE          11
#define LIT_SPECULAR_ALPHA         12
#define LIT_POSITION_X             13
#define LIT_POSITION_Y             14
#define LIT_POSITION_Z             15
#define LIT_POSITION_W             16
#define LIT_DIRECTION_X            17
#define LIT_DIRECTION_Y            18
#define LIT_DIRECTION_Z            19
#define LIT_DIRECTION_W            20
#define LIT_ATTEN_QUADRATIC        21
#define LIT_ATTEN_LINEAR           22
#define LIT_ATTEN_CONST            23
#define LIT_ATTEN_XXX              24
#define LIT_CMD_1                  25
#define LIT_SPOT_DCD               26
#define LIT_SPOT_DCM               27
#define LIT_SPOT_EXPONENT          28
#define LIT_SPOT_CUTOFF            29
#define LIT_SPECULAR_THRESH        30
#define LIT_RANGE_CUTOFF           31 /* ? */
#define LIT_ATTEN_CONST_INV        32
#define LIT_STATE_SIZE             33

/* Fog
 */
#define FOG_CMD_0      0
#define FOG_R          1
#define FOG_C          2
#define FOG_D          3
#define FOG_PAD        4
#define FOG_STATE_SIZE 5

/* UCP
 */
#define UCP_CMD_0      0
#define UCP_X          1
#define UCP_Y          2
#define UCP_Z          3
#define UCP_W          4
#define UCP_STATE_SIZE 5

/* GLT - Global ambient
 */
#define GLT_CMD_0      0
#define GLT_RED        1
#define GLT_GREEN      2
#define GLT_BLUE       3
#define GLT_ALPHA      4
#define GLT_STATE_SIZE 5

/* EYE
 */
#define EYE_CMD_0          0
#define EYE_X              1
#define EYE_Y              2
#define EYE_Z              3
#define EYE_RESCALE_FACTOR 4
#define EYE_STATE_SIZE     5

/* CST - constant state
 */
#define CST_CMD_0                             0
#define CST_PP_CNTL_X                         1
#define CST_CMD_1                             2
#define CST_RB3D_DEPTHXY_OFFSET               3
#define CST_CMD_2                             4
#define CST_RE_AUX_SCISSOR_CNTL               5
#define CST_CMD_3                             6
#define CST_RE_SCISSOR_TL_0                   7
#define CST_RE_SCISSOR_BR_0                   8
#define CST_CMD_4                             9
#define CST_SE_VAP_CNTL_STATUS                10
#define CST_CMD_5                             11
#define CST_RE_POINTSIZE                      12
#define CST_CMD_6                             13
#define CST_SE_TCL_INPUT_VTX_0                14
#define CST_SE_TCL_INPUT_VTX_1                15
#define CST_SE_TCL_INPUT_VTX_2                16
#define CST_SE_TCL_INPUT_VTX_3                17
#define CST_STATE_SIZE                        18

#define PRF_CMD_0         0
#define PRF_PP_TRI_PERF   1
#define PRF_PP_PERF_CNTL  2
#define PRF_STATE_SIZE    3


struct r200_hw_state {
   /* Head of the linked list of state atoms. */
   struct r200_state_atom atomlist;

   /* Hardware state, stored as cmdbuf commands:  
    *   -- Need to doublebuffer for
    *           - reviving state after loss of context
    *           - eliding noop statechange loops? (except line stipple count)
    */
   struct r200_state_atom ctx;
   struct r200_state_atom set;
   struct r200_state_atom vte;
   struct r200_state_atom lin;
   struct r200_state_atom msk;
   struct r200_state_atom vpt;
   struct r200_state_atom vap;
   struct r200_state_atom vtx;
   struct r200_state_atom tcl;
   struct r200_state_atom msl;
   struct r200_state_atom tcg;
   struct r200_state_atom msc;
   struct r200_state_atom cst;
   struct r200_state_atom tam;
   struct r200_state_atom tf;
   struct r200_state_atom tex[6];
   struct r200_state_atom cube[6];
   struct r200_state_atom zbs;
   struct r200_state_atom mtl[2];
   struct r200_state_atom mat[9];
   struct r200_state_atom lit[8]; /* includes vec, scl commands */
   struct r200_state_atom ucp[6];
   struct r200_state_atom pix[6]; /* pixshader stages */
   struct r200_state_atom eye; /* eye pos */
   struct r200_state_atom grd; /* guard band clipping */
   struct r200_state_atom fog;
   struct r200_state_atom glt;
   struct r200_state_atom prf;
   struct r200_state_atom afs[2];
   struct r200_state_atom pvs;
   struct r200_state_atom vpi[2];
   struct r200_state_atom vpp[2];
   struct r200_state_atom atf;
   struct r200_state_atom spr;
   struct r200_state_atom ptp;

   int max_state_size;	/* Number of bytes necessary for a full state emit. */
   GLboolean is_dirty, all_dirty;
};

struct r200_state {
   /* Derived state for internal purposes:
    */
   struct r200_colorbuffer_state color;
   struct r200_depthbuffer_state depth;
#if 00
   struct r200_pixel_state pixel;
#endif
   struct r200_scissor_state scissor;
   struct r200_stencilbuffer_state stencil;
   struct r200_stipple_state stipple;
   struct r200_texture_state texture;
   GLuint envneeded;
};

/* Need refcounting on dma buffers:
 */
struct r200_dma_buffer {
   int refcount;		/* the number of retained regions in buf */
   drmBufPtr buf;
};

#define GET_START(rvb) (rmesa->r200Screen->gart_buffer_offset +		\
			(rvb)->address - rmesa->dma.buf0_address +	\
			(rvb)->start)

/* A retained region, eg vertices for indexed vertices.
 */
struct r200_dma_region {
   struct r200_dma_buffer *buf;
   char *address;		/* == buf->address */
   int start, end, ptr;		/* offsets from start of buf */
   int aos_start;
   int aos_stride;
   int aos_size;
};


struct r200_dma {
   /* Active dma region.  Allocations for vertices and retained
    * regions come from here.  Also used for emitting random vertices,
    * these may be flushed by calling flush_current();
    */
   struct r200_dma_region current;
   
   void (*flush)( r200ContextPtr );

   char *buf0_address;		/* start of buf[0], for index calcs */
   GLuint nr_released_bufs;	/* flush after so many buffers released */
};

struct r200_dri_mirror {
   __DRIcontextPrivate	*context;	/* DRI context */
   __DRIscreenPrivate	*screen;	/* DRI screen */
   __DRIdrawablePrivate	*drawable;	/* DRI drawable bound to this ctx */
   __DRIdrawablePrivate	*readable;	/* DRI readable bound to this ctx */

   drm_context_t hwContext;
   drm_hw_lock_t *hwLock;
   int fd;
   int drmMinor;
};


#define R200_CMD_BUF_SZ  (16*1024) 

struct r200_store {
   GLuint statenr;
   GLuint primnr;
   char cmd_buf[R200_CMD_BUF_SZ];
   int cmd_used;   
   int elts_start;
};


/* r200_tcl.c
 */
struct r200_tcl_info {
   GLuint hw_primitive;

/* hw can handle 12 components max */
   struct r200_dma_region *aos_components[12];
   GLuint nr_aos_components;

   GLuint *Elts;

   struct r200_dma_region indexed_verts;
   struct r200_dma_region vertex_data[15];
};


/* r200_swtcl.c
 */
struct r200_swtcl_info {
   GLuint RenderIndex;
   
   /**
    * Size of a hardware vertex.  This is calculated when \c ::vertex_attrs is
    * installed in the Mesa state vector.
    */
   GLuint vertex_size;

   /**
    * Attributes instructing the Mesa TCL pipeline where / how to put vertex
    * data in the hardware buffer.
    */
   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];

   /**
    * Number of elements of \c ::vertex_attrs that are actually used.
    */
   GLuint vertex_attr_count;

   /**
    * Cached pointer to the buffer where Mesa will store vertex data.
    */
   GLubyte *verts;

   /* Fallback rasterization functions
    */
   r200_point_func draw_point;
   r200_line_func draw_line;
   r200_tri_func draw_tri;

   GLuint hw_primitive;
   GLenum render_primitive;
   GLuint numverts;

   /**
    * Offset of the 4UB color data within a hardware (swtcl) vertex.
    */
   GLuint coloroffset;

   /**
    * Offset of the 3UB specular color data within a hardware (swtcl) vertex.
    */
   GLuint specoffset;

   /**
    * Should Mesa project vertex data or will the hardware do it?
    */
   GLboolean needproj;

   struct r200_dma_region indexed_verts;
};


struct r200_ioctl {
   GLuint vertex_offset;
   GLuint vertex_size;
};



#define R200_MAX_PRIMS 64



struct r200_prim {
   GLuint start;
   GLuint end;
   GLuint prim;
};

   /* A maximum total of 29 elements per vertex:  3 floats for position, 3
    * floats for normal, 4 floats for color, 4 bytes for secondary color,
    * 3 floats for each texture unit (18 floats total).
    * 
    * we maybe need add. 4 to prevent segfault if someone specifies
    * GL_TEXTURE6/GL_TEXTURE7 (esp. for the codegen-path) (FIXME: )
    * 
    * The position data is never actually stored here, so 3 elements could be
    * trimmed out of the buffer.
    */

#define R200_MAX_VERTEX_SIZE ((3*6)+11)


struct r200_context {
   GLcontext *glCtx;			/* Mesa context */

   /* Driver and hardware state management
    */
   struct r200_hw_state hw;
   struct r200_state state;
   struct r200_vertex_program *curr_vp_hw;

   /* Texture object bookkeeping
    */
   unsigned              nr_heaps;
   driTexHeap          * texture_heaps[ RADEON_NR_TEX_HEAPS ];
   driTextureObject      swapped;
   int                   texture_depth;
   float                 initialMaxAnisotropy;

   /* Rasterization and vertex state:
    */
   GLuint TclFallback;
   GLuint Fallback;
   GLuint NewGLState;
   DECLARE_RENDERINPUTS(tnl_index_bitset);	/* index of bits for last tnl_install_attrs */

   /* Vertex buffers
    */
   struct r200_ioctl ioctl;
   struct r200_dma dma;
   struct r200_store store;
   /* A full state emit as of the first state emit in the main store, in case
    * the context is lost.
    */
   struct r200_store backup_store;

   /* Page flipping
    */
   GLuint doPageFlip;

   /* Busy waiting
    */
   GLuint do_usleeps;
   GLuint do_irqs;
   GLuint irqsEmitted;
   drm_radeon_irq_wait_t iw;

   /* Clientdata textures;
    */
   GLuint prefer_gart_client_texturing;

   /* Drawable, cliprect and scissor information
    */
   GLuint numClipRects;			/* Cliprects for the draw buffer */
   drm_clip_rect_t *pClipRects;
   unsigned int lastStamp;
   GLboolean lost_context;
   GLboolean save_on_next_emit;
   radeonScreenPtr r200Screen;	/* Screen private DRI data */
   drm_radeon_sarea_t *sarea;		/* Private SAREA data */

   /* TCL stuff
    */
   GLmatrix TexGenMatrix[R200_MAX_TEXTURE_UNITS];
   GLboolean recheck_texgen[R200_MAX_TEXTURE_UNITS];
   GLboolean TexGenNeedNormals[R200_MAX_TEXTURE_UNITS];
   GLuint TexMatEnabled;
   GLuint TexMatCompSel;
   GLuint TexGenEnabled;
   GLuint TexGenCompSel;
   GLmatrix tmpmat;

   /* VBI / buffer swap
    */
   GLuint vbl_seq;
   GLuint vblank_flags;

   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;


   /* r200_tcl.c
    */
   struct r200_tcl_info tcl;

   /* r200_swtcl.c
    */
   struct r200_swtcl_info swtcl;

   /* Mirrors of some DRI state
    */
   struct r200_dri_mirror dri;

   /* Configuration cache
    */
   driOptionCache optionCache;

   GLboolean using_hyperz;
   GLboolean texmicrotile;

  struct ati_fragment_shader *afs_loaded;
};

#define R200_CONTEXT(ctx)		((r200ContextPtr)(ctx->DriverCtx))


static __inline GLuint r200PackColor( GLuint cpp,
					GLubyte r, GLubyte g,
					GLubyte b, GLubyte a )
{
   switch ( cpp ) {
   case 2:
      return PACK_COLOR_565( r, g, b );
   case 4:
      return PACK_COLOR_8888( a, r, g, b );
   default:
      return 0;
   }
}


extern void r200DestroyContext( __DRIcontextPrivate *driContextPriv );
extern GLboolean r200CreateContext( const __GLcontextModes *glVisual,
				    __DRIcontextPrivate *driContextPriv,
				    void *sharedContextPrivate);
extern void r200SwapBuffers( __DRIdrawablePrivate *dPriv );
extern void r200CopySubBuffer( __DRIdrawablePrivate * dPriv,
			       int x, int y, int w, int h );
extern GLboolean r200MakeCurrent( __DRIcontextPrivate *driContextPriv,
				  __DRIdrawablePrivate *driDrawPriv,
				  __DRIdrawablePrivate *driReadPriv );
extern GLboolean r200UnbindContext( __DRIcontextPrivate *driContextPriv );

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1

#if DO_DEBUG
extern int R200_DEBUG;
#else
#define R200_DEBUG		0
#endif

#define DEBUG_TEXTURE	0x001
#define DEBUG_STATE	0x002
#define DEBUG_IOCTL	0x004
#define DEBUG_PRIMS	0x008
#define DEBUG_VERTS	0x010
#define DEBUG_FALLBACKS	0x020
#define DEBUG_VFMT	0x040
#define DEBUG_CODEGEN	0x080
#define DEBUG_VERBOSE	0x100
#define DEBUG_DRI       0x200
#define DEBUG_DMA       0x400
#define DEBUG_SANITY    0x800
#define DEBUG_SYNC      0x1000
#define DEBUG_PIXEL     0x2000
#define DEBUG_MEMORY    0x4000

#endif /* __R200_CONTEXT_H__ */
