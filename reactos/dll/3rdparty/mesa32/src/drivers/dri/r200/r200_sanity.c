/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_sanity.c,v 1.1 2002/10/30 12:51:52 alanh Exp $ */
/**************************************************************************

Copyright 2002 ATI Technologies Inc., Ontario, Canada, and
                     Tungsten Graphics Inc, Cedar Park, TX.

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
ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */
 
#include <errno.h> 

#include "glheader.h"
#include "imports.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_sanity.h"
#include "radeon_reg.h"
#include "r200_reg.h"

/* Set this '1' to get more verbiage.
 */
#define MORE_VERBOSE 1

#if MORE_VERBOSE
#define VERBOSE (R200_DEBUG & DEBUG_VERBOSE)
#define NORMAL  (1)
#else
#define VERBOSE 0
#define NORMAL  (R200_DEBUG & DEBUG_VERBOSE)
#endif


/* New (1.3) state mechanism.  3 commands (packet, scalar, vector) in
 * 1.3 cmdbuffers allow all previous state to be updated as well as
 * the tcl scalar and vector areas.  
 */
static struct { 
   int start; 
   int len; 
   const char *name;
} packet[RADEON_MAX_STATE_PACKETS] = {
   { RADEON_PP_MISC,7,"RADEON_PP_MISC" },
   { RADEON_PP_CNTL,3,"RADEON_PP_CNTL" },
   { RADEON_RB3D_COLORPITCH,1,"RADEON_RB3D_COLORPITCH" },
   { RADEON_RE_LINE_PATTERN,2,"RADEON_RE_LINE_PATTERN" },
   { RADEON_SE_LINE_WIDTH,1,"RADEON_SE_LINE_WIDTH" },
   { RADEON_PP_LUM_MATRIX,1,"RADEON_PP_LUM_MATRIX" },
   { RADEON_PP_ROT_MATRIX_0,2,"RADEON_PP_ROT_MATRIX_0" },
   { RADEON_RB3D_STENCILREFMASK,3,"RADEON_RB3D_STENCILREFMASK" },
   { RADEON_SE_VPORT_XSCALE,6,"RADEON_SE_VPORT_XSCALE" },
   { RADEON_SE_CNTL,2,"RADEON_SE_CNTL" },
   { RADEON_SE_CNTL_STATUS,1,"RADEON_SE_CNTL_STATUS" },
   { RADEON_RE_MISC,1,"RADEON_RE_MISC" },
   { RADEON_PP_TXFILTER_0,6,"RADEON_PP_TXFILTER_0" },
   { RADEON_PP_BORDER_COLOR_0,1,"RADEON_PP_BORDER_COLOR_0" },
   { RADEON_PP_TXFILTER_1,6,"RADEON_PP_TXFILTER_1" },
   { RADEON_PP_BORDER_COLOR_1,1,"RADEON_PP_BORDER_COLOR_1" },
   { RADEON_PP_TXFILTER_2,6,"RADEON_PP_TXFILTER_2" },
   { RADEON_PP_BORDER_COLOR_2,1,"RADEON_PP_BORDER_COLOR_2" },
   { RADEON_SE_ZBIAS_FACTOR,2,"RADEON_SE_ZBIAS_FACTOR" },
   { RADEON_SE_TCL_OUTPUT_VTX_FMT,11,"RADEON_SE_TCL_OUTPUT_VTX_FMT" },
   { RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED,17,"RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED" },
   { R200_PP_TXCBLEND_0, 4, "R200_EMIT_PP_TXCBLEND_0" },
   { R200_PP_TXCBLEND_1, 4, "R200_PP_TXCBLEND_1" },
   { R200_PP_TXCBLEND_2, 4, "R200_PP_TXCBLEND_2" },
   { R200_PP_TXCBLEND_3, 4, "R200_PP_TXCBLEND_3" },
   { R200_PP_TXCBLEND_4, 4, "R200_PP_TXCBLEND_4" },
   { R200_PP_TXCBLEND_5, 4, "R200_PP_TXCBLEND_5" },
   { R200_PP_TXCBLEND_6, 4, "R200_PP_TXCBLEND_6" },
   { R200_PP_TXCBLEND_7, 4, "R200_PP_TXCBLEND_7" },
   { R200_SE_TCL_LIGHT_MODEL_CTL_0, 6, "R200_SE_TCL_LIGHT_MODEL_CTL_0" },
   { R200_PP_TFACTOR_0, 6, "R200_PP_TFACTOR_0" },
   { R200_SE_VTX_FMT_0, 4, "R200_SE_VTX_FMT_0" },
   { R200_SE_VAP_CNTL, 1, "R200_SE_VAP_CNTL" },
   { R200_SE_TCL_MATRIX_SEL_0, 5, "R200_SE_TCL_MATRIX_SEL_0" },
   { R200_SE_TCL_TEX_PROC_CTL_2, 5, "R200_SE_TCL_TEX_PROC_CTL_2" },
   { R200_SE_TCL_UCP_VERT_BLEND_CTL, 1, "R200_SE_TCL_UCP_VERT_BLEND_CTL" },
   { R200_PP_TXFILTER_0, 6, "R200_PP_TXFILTER_0" },
   { R200_PP_TXFILTER_1, 6, "R200_PP_TXFILTER_1" },
   { R200_PP_TXFILTER_2, 6, "R200_PP_TXFILTER_2" },
   { R200_PP_TXFILTER_3, 6, "R200_PP_TXFILTER_3" },
   { R200_PP_TXFILTER_4, 6, "R200_PP_TXFILTER_4" },
   { R200_PP_TXFILTER_5, 6, "R200_PP_TXFILTER_5" },
   { R200_PP_TXOFFSET_0, 1, "R200_PP_TXOFFSET_0" },
   { R200_PP_TXOFFSET_1, 1, "R200_PP_TXOFFSET_1" },
   { R200_PP_TXOFFSET_2, 1, "R200_PP_TXOFFSET_2" },
   { R200_PP_TXOFFSET_3, 1, "R200_PP_TXOFFSET_3" },
   { R200_PP_TXOFFSET_4, 1, "R200_PP_TXOFFSET_4" },
   { R200_PP_TXOFFSET_5, 1, "R200_PP_TXOFFSET_5" },
   { R200_SE_VTE_CNTL, 1, "R200_SE_VTE_CNTL" },
   { R200_SE_TCL_OUTPUT_VTX_COMP_SEL, 1, "R200_SE_TCL_OUTPUT_VTX_COMP_SEL" },
   { R200_PP_TAM_DEBUG3, 1, "R200_PP_TAM_DEBUG3" },
   { R200_PP_CNTL_X, 1, "R200_PP_CNTL_X" }, 
   { R200_RB3D_DEPTHXY_OFFSET, 1, "R200_RB3D_DEPTHXY_OFFSET" }, 
   { R200_RE_AUX_SCISSOR_CNTL, 1, "R200_RE_AUX_SCISSOR_CNTL" }, 
   { R200_RE_SCISSOR_TL_0, 2, "R200_RE_SCISSOR_TL_0" }, 
   { R200_RE_SCISSOR_TL_1, 2, "R200_RE_SCISSOR_TL_1" }, 
   { R200_RE_SCISSOR_TL_2, 2, "R200_RE_SCISSOR_TL_2" }, 
   { R200_SE_VAP_CNTL_STATUS, 1, "R200_SE_VAP_CNTL_STATUS" }, 
   { R200_SE_VTX_STATE_CNTL, 1, "R200_SE_VTX_STATE_CNTL" }, 
   { R200_RE_POINTSIZE, 1, "R200_RE_POINTSIZE" }, 
   { R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0, 4, "R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0" },
   { R200_PP_CUBIC_FACES_0, 1, "R200_PP_CUBIC_FACES_0" }, /* 61 */
   { R200_PP_CUBIC_OFFSET_F1_0, 5, "R200_PP_CUBIC_OFFSET_F1_0" }, /* 62 */
   { R200_PP_CUBIC_FACES_1, 1, "R200_PP_CUBIC_FACES_1" },
   { R200_PP_CUBIC_OFFSET_F1_1, 5, "R200_PP_CUBIC_OFFSET_F1_1" },
   { R200_PP_CUBIC_FACES_2, 1, "R200_PP_CUBIC_FACES_2" },
   { R200_PP_CUBIC_OFFSET_F1_2, 5, "R200_PP_CUBIC_OFFSET_F1_2" },
   { R200_PP_CUBIC_FACES_3, 1, "R200_PP_CUBIC_FACES_3" },
   { R200_PP_CUBIC_OFFSET_F1_3, 5, "R200_PP_CUBIC_OFFSET_F1_3" },
   { R200_PP_CUBIC_FACES_4, 1, "R200_PP_CUBIC_FACES_4" },
   { R200_PP_CUBIC_OFFSET_F1_4, 5, "R200_PP_CUBIC_OFFSET_F1_4" },
   { R200_PP_CUBIC_FACES_5, 1, "R200_PP_CUBIC_FACES_5" },
   { R200_PP_CUBIC_OFFSET_F1_5, 5, "R200_PP_CUBIC_OFFSET_F1_5" },
   { RADEON_PP_TEX_SIZE_0, 2, "RADEON_PP_TEX_SIZE_0" },
   { RADEON_PP_TEX_SIZE_1, 2, "RADEON_PP_TEX_SIZE_1" },
   { RADEON_PP_TEX_SIZE_2, 2, "RADEON_PP_TEX_SIZE_2" },
   { R200_RB3D_BLENDCOLOR, 3, "R200_RB3D_BLENDCOLOR" },
   { R200_SE_TCL_POINT_SPRITE_CNTL, 1, "R200_SE_TCL_POINT_SPRITE_CNTL" },
   { RADEON_PP_CUBIC_FACES_0, 1, "RADEON_PP_CUBIC_FACES_0" },
   { RADEON_PP_CUBIC_OFFSET_T0_0, 5, "RADEON_PP_CUBIC_OFFSET_T0_0" },
   { RADEON_PP_CUBIC_FACES_1, 1, "RADEON_PP_CUBIC_FACES_1" },
   { RADEON_PP_CUBIC_OFFSET_T1_0, 5, "RADEON_PP_CUBIC_OFFSET_T1_0" },
   { RADEON_PP_CUBIC_FACES_2, 1, "RADEON_PP_CUBIC_FACES_2" },
   { RADEON_PP_CUBIC_OFFSET_T2_0, 5, "RADEON_PP_CUBIC_OFFSET_T2_0" },
   { R200_PP_TRI_PERF, 2, "R200_PP_TRI_PERF" },
   { R200_PP_TXCBLEND_8, 32, "R200_PP_AFS_0"},   /* 85 */
   { R200_PP_TXCBLEND_0, 32, "R200_PP_AFS_1"},
   { R200_PP_TFACTOR_0, 8, "R200_ATF_TFACTOR"},
   { R200_PP_TXFILTER_0, 8, "R200_PP_TXCTLALL_0"},
   { R200_PP_TXFILTER_1, 8, "R200_PP_TXCTLALL_1"},
   { R200_PP_TXFILTER_2, 8, "R200_PP_TXCTLALL_2"},
   { R200_PP_TXFILTER_3, 8, "R200_PP_TXCTLALL_3"},
   { R200_PP_TXFILTER_4, 8, "R200_PP_TXCTLALL_4"},
   { R200_PP_TXFILTER_5, 8, "R200_PP_TXCTLALL_5"},
   { R200_VAP_PVS_CNTL_1, 2, "R200_VAP_PVS_CNTL"},
};

struct reg_names {
   int idx;
   const char *name;
};

static struct reg_names reg_names[] = {
   { R200_PP_MISC, "R200_PP_MISC" },
   { R200_PP_FOG_COLOR, "R200_PP_FOG_COLOR" },
   { R200_RE_SOLID_COLOR, "R200_RE_SOLID_COLOR" },
   { R200_RB3D_BLENDCNTL, "R200_RB3D_BLENDCNTL" },
   { R200_RB3D_DEPTHOFFSET, "R200_RB3D_DEPTHOFFSET" },
   { R200_RB3D_DEPTHPITCH, "R200_RB3D_DEPTHPITCH" },
   { R200_RB3D_ZSTENCILCNTL, "R200_RB3D_ZSTENCILCNTL" },
   { R200_PP_CNTL, "R200_PP_CNTL" },
   { R200_RB3D_CNTL, "R200_RB3D_CNTL" },
   { R200_RB3D_COLOROFFSET, "R200_RB3D_COLOROFFSET" },
   { R200_RE_WIDTH_HEIGHT, "R200_RE_WIDTH_HEIGHT" },
   { R200_RB3D_COLORPITCH, "R200_RB3D_COLORPITCH" },
   { R200_SE_CNTL, "R200_SE_CNTL" },
   { R200_RE_CNTL, "R200_RE_CNTL" },
   { R200_RE_MISC, "R200_RE_MISC" },
   { R200_RE_STIPPLE_ADDR, "R200_RE_STIPPLE_ADDR" },
   { R200_RE_STIPPLE_DATA, "R200_RE_STIPPLE_DATA" },
   { R200_RE_LINE_PATTERN, "R200_RE_LINE_PATTERN" },
   { R200_RE_LINE_STATE, "R200_RE_LINE_STATE" },
   { R200_RE_SCISSOR_TL_0, "R200_RE_SCISSOR_TL_0" },
   { R200_RE_SCISSOR_BR_0, "R200_RE_SCISSOR_BR_0" },
   { R200_RE_SCISSOR_TL_1, "R200_RE_SCISSOR_TL_1" },
   { R200_RE_SCISSOR_BR_1, "R200_RE_SCISSOR_BR_1" },
   { R200_RE_SCISSOR_TL_2, "R200_RE_SCISSOR_TL_2" },
   { R200_RE_SCISSOR_BR_2, "R200_RE_SCISSOR_BR_2" },
   { R200_RB3D_DEPTHXY_OFFSET, "R200_RB3D_DEPTHXY_OFFSET" },
   { R200_RB3D_STENCILREFMASK, "R200_RB3D_STENCILREFMASK" },
   { R200_RB3D_ROPCNTL, "R200_RB3D_ROPCNTL" },
   { R200_RB3D_PLANEMASK, "R200_RB3D_PLANEMASK" },
   { R200_SE_VPORT_XSCALE, "R200_SE_VPORT_XSCALE" },
   { R200_SE_VPORT_XOFFSET, "R200_SE_VPORT_XOFFSET" },
   { R200_SE_VPORT_YSCALE, "R200_SE_VPORT_YSCALE" },
   { R200_SE_VPORT_YOFFSET, "R200_SE_VPORT_YOFFSET" },
   { R200_SE_VPORT_ZSCALE, "R200_SE_VPORT_ZSCALE" },
   { R200_SE_VPORT_ZOFFSET, "R200_SE_VPORT_ZOFFSET" },
   { R200_SE_ZBIAS_FACTOR, "R200_SE_ZBIAS_FACTOR" },
   { R200_SE_ZBIAS_CONSTANT, "R200_SE_ZBIAS_CONSTANT" },
   { R200_SE_LINE_WIDTH, "R200_SE_LINE_WIDTH" },
   { R200_SE_VAP_CNTL, "R200_SE_VAP_CNTL" },
   { R200_SE_VF_CNTL, "R200_SE_VF_CNTL" },
   { R200_SE_VTX_FMT_0, "R200_SE_VTX_FMT_0" },
   { R200_SE_VTX_FMT_1, "R200_SE_VTX_FMT_1" },
   { R200_SE_TCL_OUTPUT_VTX_FMT_0, "R200_SE_TCL_OUTPUT_VTX_FMT_0" },
   { R200_SE_TCL_OUTPUT_VTX_FMT_1, "R200_SE_TCL_OUTPUT_VTX_FMT_1" },
   { R200_SE_VTE_CNTL, "R200_SE_VTE_CNTL" },
   { R200_SE_VTX_NUM_ARRAYS, "R200_SE_VTX_NUM_ARRAYS" },
   { R200_SE_VTX_AOS_ATTR01, "R200_SE_VTX_AOS_ATTR01" },
   { R200_SE_VTX_AOS_ADDR0, "R200_SE_VTX_AOS_ADDR0" },
   { R200_SE_VTX_AOS_ADDR1, "R200_SE_VTX_AOS_ADDR1" },
   { R200_SE_VTX_AOS_ATTR23, "R200_SE_VTX_AOS_ATTR23" },
   { R200_SE_VTX_AOS_ADDR2, "R200_SE_VTX_AOS_ADDR2" },
   { R200_SE_VTX_AOS_ADDR3, "R200_SE_VTX_AOS_ADDR3" },
   { R200_SE_VTX_AOS_ATTR45, "R200_SE_VTX_AOS_ATTR45" },
   { R200_SE_VTX_AOS_ADDR4, "R200_SE_VTX_AOS_ADDR4" },
   { R200_SE_VTX_AOS_ADDR5, "R200_SE_VTX_AOS_ADDR5" },
   { R200_SE_VTX_AOS_ATTR67, "R200_SE_VTX_AOS_ATTR67" },
   { R200_SE_VTX_AOS_ADDR6, "R200_SE_VTX_AOS_ADDR6" },
   { R200_SE_VTX_AOS_ADDR7, "R200_SE_VTX_AOS_ADDR7" },
   { R200_SE_VTX_AOS_ATTR89, "R200_SE_VTX_AOS_ATTR89" },
   { R200_SE_VTX_AOS_ADDR8, "R200_SE_VTX_AOS_ADDR8" },
   { R200_SE_VTX_AOS_ADDR9, "R200_SE_VTX_AOS_ADDR9" },
   { R200_SE_VTX_AOS_ATTR1011, "R200_SE_VTX_AOS_ATTR1011" },
   { R200_SE_VTX_AOS_ADDR10, "R200_SE_VTX_AOS_ADDR10" },
   { R200_SE_VTX_AOS_ADDR11, "R200_SE_VTX_AOS_ADDR11" },
   { R200_SE_VF_MAX_VTX_INDX, "R200_SE_VF_MAX_VTX_INDX" },
   { R200_SE_VF_MIN_VTX_INDX, "R200_SE_VF_MIN_VTX_INDX" },
   { R200_SE_VTX_STATE_CNTL, "R200_SE_VTX_STATE_CNTL" },
   { R200_SE_TCL_VECTOR_INDX_REG, "R200_SE_TCL_VECTOR_INDX_REG" },
   { R200_SE_TCL_VECTOR_DATA_REG, "R200_SE_TCL_VECTOR_DATA_REG" },
   { R200_SE_TCL_SCALAR_INDX_REG, "R200_SE_TCL_SCALAR_INDX_REG" },
   { R200_SE_TCL_SCALAR_DATA_REG, "R200_SE_TCL_SCALAR_DATA_REG" },
   { R200_SE_TCL_MATRIX_SEL_0, "R200_SE_TCL_MATRIX_SEL_0" },
   { R200_SE_TCL_MATRIX_SEL_1, "R200_SE_TCL_MATRIX_SEL_1" },
   { R200_SE_TCL_MATRIX_SEL_2, "R200_SE_TCL_MATRIX_SEL_2" },
   { R200_SE_TCL_MATRIX_SEL_3, "R200_SE_TCL_MATRIX_SEL_3" },
   { R200_SE_TCL_MATRIX_SEL_4, "R200_SE_TCL_MATRIX_SEL_4" },
   { R200_SE_TCL_LIGHT_MODEL_CTL_0, "R200_SE_TCL_LIGHT_MODEL_CTL_0" },
   { R200_SE_TCL_LIGHT_MODEL_CTL_1, "R200_SE_TCL_LIGHT_MODEL_CTL_1" },
   { R200_SE_TCL_PER_LIGHT_CTL_0, "R200_SE_TCL_PER_LIGHT_CTL_0" },
   { R200_SE_TCL_PER_LIGHT_CTL_1, "R200_SE_TCL_PER_LIGHT_CTL_1" },
   { R200_SE_TCL_PER_LIGHT_CTL_2, "R200_SE_TCL_PER_LIGHT_CTL_2" },
   { R200_SE_TCL_PER_LIGHT_CTL_3, "R200_SE_TCL_PER_LIGHT_CTL_3" },
   { R200_SE_TCL_TEX_PROC_CTL_2, "R200_SE_TCL_TEX_PROC_CTL_2" },
   { R200_SE_TCL_TEX_PROC_CTL_3, "R200_SE_TCL_TEX_PROC_CTL_3" },
   { R200_SE_TCL_TEX_PROC_CTL_0, "R200_SE_TCL_TEX_PROC_CTL_0" },
   { R200_SE_TCL_TEX_PROC_CTL_1, "R200_SE_TCL_TEX_PROC_CTL_1" },
   { R200_SE_TC_TEX_CYL_WRAP_CTL, "R200_SE_TC_TEX_CYL_WRAP_CTL" },
   { R200_SE_TCL_UCP_VERT_BLEND_CTL, "R200_SE_TCL_UCP_VERT_BLEND_CTL" },
   { R200_SE_TCL_POINT_SPRITE_CNTL, "R200_SE_TCL_POINT_SPRITE_CNTL" },
   { R200_SE_VTX_ST_POS_0_X_4, "R200_SE_VTX_ST_POS_0_X_4" },
   { R200_SE_VTX_ST_POS_0_Y_4, "R200_SE_VTX_ST_POS_0_Y_4" },
   { R200_SE_VTX_ST_POS_0_Z_4, "R200_SE_VTX_ST_POS_0_Z_4" },
   { R200_SE_VTX_ST_POS_0_W_4, "R200_SE_VTX_ST_POS_0_W_4" },
   { R200_SE_VTX_ST_NORM_0_X, "R200_SE_VTX_ST_NORM_0_X" },
   { R200_SE_VTX_ST_NORM_0_Y, "R200_SE_VTX_ST_NORM_0_Y" },
   { R200_SE_VTX_ST_NORM_0_Z, "R200_SE_VTX_ST_NORM_0_Z" },
   { R200_SE_VTX_ST_PVMS, "R200_SE_VTX_ST_PVMS" },
   { R200_SE_VTX_ST_CLR_0_R, "R200_SE_VTX_ST_CLR_0_R" },
   { R200_SE_VTX_ST_CLR_0_G, "R200_SE_VTX_ST_CLR_0_G" },
   { R200_SE_VTX_ST_CLR_0_B, "R200_SE_VTX_ST_CLR_0_B" },
   { R200_SE_VTX_ST_CLR_0_A, "R200_SE_VTX_ST_CLR_0_A" },
   { R200_SE_VTX_ST_CLR_1_R, "R200_SE_VTX_ST_CLR_1_R" },
   { R200_SE_VTX_ST_CLR_1_G, "R200_SE_VTX_ST_CLR_1_G" },
   { R200_SE_VTX_ST_CLR_1_B, "R200_SE_VTX_ST_CLR_1_B" },
   { R200_SE_VTX_ST_CLR_1_A, "R200_SE_VTX_ST_CLR_1_A" },
   { R200_SE_VTX_ST_CLR_2_R, "R200_SE_VTX_ST_CLR_2_R" },
   { R200_SE_VTX_ST_CLR_2_G, "R200_SE_VTX_ST_CLR_2_G" },
   { R200_SE_VTX_ST_CLR_2_B, "R200_SE_VTX_ST_CLR_2_B" },
   { R200_SE_VTX_ST_CLR_2_A, "R200_SE_VTX_ST_CLR_2_A" },
   { R200_SE_VTX_ST_CLR_3_R, "R200_SE_VTX_ST_CLR_3_R" },
   { R200_SE_VTX_ST_CLR_3_G, "R200_SE_VTX_ST_CLR_3_G" },
   { R200_SE_VTX_ST_CLR_3_B, "R200_SE_VTX_ST_CLR_3_B" },
   { R200_SE_VTX_ST_CLR_3_A, "R200_SE_VTX_ST_CLR_3_A" },
   { R200_SE_VTX_ST_CLR_4_R, "R200_SE_VTX_ST_CLR_4_R" },
   { R200_SE_VTX_ST_CLR_4_G, "R200_SE_VTX_ST_CLR_4_G" },
   { R200_SE_VTX_ST_CLR_4_B, "R200_SE_VTX_ST_CLR_4_B" },
   { R200_SE_VTX_ST_CLR_4_A, "R200_SE_VTX_ST_CLR_4_A" },
   { R200_SE_VTX_ST_CLR_5_R, "R200_SE_VTX_ST_CLR_5_R" },
   { R200_SE_VTX_ST_CLR_5_G, "R200_SE_VTX_ST_CLR_5_G" },
   { R200_SE_VTX_ST_CLR_5_B, "R200_SE_VTX_ST_CLR_5_B" },
   { R200_SE_VTX_ST_CLR_5_A, "R200_SE_VTX_ST_CLR_5_A" },
   { R200_SE_VTX_ST_CLR_6_R, "R200_SE_VTX_ST_CLR_6_R" },
   { R200_SE_VTX_ST_CLR_6_G, "R200_SE_VTX_ST_CLR_6_G" },
   { R200_SE_VTX_ST_CLR_6_B, "R200_SE_VTX_ST_CLR_6_B" },
   { R200_SE_VTX_ST_CLR_6_A, "R200_SE_VTX_ST_CLR_6_A" },
   { R200_SE_VTX_ST_CLR_7_R, "R200_SE_VTX_ST_CLR_7_R" },
   { R200_SE_VTX_ST_CLR_7_G, "R200_SE_VTX_ST_CLR_7_G" },
   { R200_SE_VTX_ST_CLR_7_B, "R200_SE_VTX_ST_CLR_7_B" },
   { R200_SE_VTX_ST_CLR_7_A, "R200_SE_VTX_ST_CLR_7_A" },
   { R200_SE_VTX_ST_TEX_0_S, "R200_SE_VTX_ST_TEX_0_S" },
   { R200_SE_VTX_ST_TEX_0_T, "R200_SE_VTX_ST_TEX_0_T" },
   { R200_SE_VTX_ST_TEX_0_R, "R200_SE_VTX_ST_TEX_0_R" },
   { R200_SE_VTX_ST_TEX_0_Q, "R200_SE_VTX_ST_TEX_0_Q" },
   { R200_SE_VTX_ST_TEX_1_S, "R200_SE_VTX_ST_TEX_1_S" },
   { R200_SE_VTX_ST_TEX_1_T, "R200_SE_VTX_ST_TEX_1_T" },
   { R200_SE_VTX_ST_TEX_1_R, "R200_SE_VTX_ST_TEX_1_R" },
   { R200_SE_VTX_ST_TEX_1_Q, "R200_SE_VTX_ST_TEX_1_Q" },
   { R200_SE_VTX_ST_TEX_2_S, "R200_SE_VTX_ST_TEX_2_S" },
   { R200_SE_VTX_ST_TEX_2_T, "R200_SE_VTX_ST_TEX_2_T" },
   { R200_SE_VTX_ST_TEX_2_R, "R200_SE_VTX_ST_TEX_2_R" },
   { R200_SE_VTX_ST_TEX_2_Q, "R200_SE_VTX_ST_TEX_2_Q" },
   { R200_SE_VTX_ST_TEX_3_S, "R200_SE_VTX_ST_TEX_3_S" },
   { R200_SE_VTX_ST_TEX_3_T, "R200_SE_VTX_ST_TEX_3_T" },
   { R200_SE_VTX_ST_TEX_3_R, "R200_SE_VTX_ST_TEX_3_R" },
   { R200_SE_VTX_ST_TEX_3_Q, "R200_SE_VTX_ST_TEX_3_Q" },
   { R200_SE_VTX_ST_TEX_4_S, "R200_SE_VTX_ST_TEX_4_S" },
   { R200_SE_VTX_ST_TEX_4_T, "R200_SE_VTX_ST_TEX_4_T" },
   { R200_SE_VTX_ST_TEX_4_R, "R200_SE_VTX_ST_TEX_4_R" },
   { R200_SE_VTX_ST_TEX_4_Q, "R200_SE_VTX_ST_TEX_4_Q" },
   { R200_SE_VTX_ST_TEX_5_S, "R200_SE_VTX_ST_TEX_5_S" },
   { R200_SE_VTX_ST_TEX_5_T, "R200_SE_VTX_ST_TEX_5_T" },
   { R200_SE_VTX_ST_TEX_5_R, "R200_SE_VTX_ST_TEX_5_R" },
   { R200_SE_VTX_ST_TEX_5_Q, "R200_SE_VTX_ST_TEX_5_Q" },
   { R200_SE_VTX_ST_PNT_SPRT_SZ, "R200_SE_VTX_ST_PNT_SPRT_SZ" },
   { R200_SE_VTX_ST_DISC_FOG, "R200_SE_VTX_ST_DISC_FOG" },
   { R200_SE_VTX_ST_SHININESS_0, "R200_SE_VTX_ST_SHININESS_0" },
   { R200_SE_VTX_ST_SHININESS_1, "R200_SE_VTX_ST_SHININESS_1" },
   { R200_SE_VTX_ST_BLND_WT_0, "R200_SE_VTX_ST_BLND_WT_0" },
   { R200_SE_VTX_ST_BLND_WT_1, "R200_SE_VTX_ST_BLND_WT_1" },
   { R200_SE_VTX_ST_BLND_WT_2, "R200_SE_VTX_ST_BLND_WT_2" },
   { R200_SE_VTX_ST_BLND_WT_3, "R200_SE_VTX_ST_BLND_WT_3" },
   { R200_SE_VTX_ST_POS_1_X, "R200_SE_VTX_ST_POS_1_X" },
   { R200_SE_VTX_ST_POS_1_Y, "R200_SE_VTX_ST_POS_1_Y" },
   { R200_SE_VTX_ST_POS_1_Z, "R200_SE_VTX_ST_POS_1_Z" },
   { R200_SE_VTX_ST_POS_1_W, "R200_SE_VTX_ST_POS_1_W" },
   { R200_SE_VTX_ST_NORM_1_X, "R200_SE_VTX_ST_NORM_1_X" },
   { R200_SE_VTX_ST_NORM_1_Y, "R200_SE_VTX_ST_NORM_1_Y" },
   { R200_SE_VTX_ST_NORM_1_Z, "R200_SE_VTX_ST_NORM_1_Z" },
   { R200_SE_VTX_ST_USR_CLR_0_R, "R200_SE_VTX_ST_USR_CLR_0_R" },
   { R200_SE_VTX_ST_USR_CLR_0_G, "R200_SE_VTX_ST_USR_CLR_0_G" },
   { R200_SE_VTX_ST_USR_CLR_0_B, "R200_SE_VTX_ST_USR_CLR_0_B" },
   { R200_SE_VTX_ST_USR_CLR_0_A, "R200_SE_VTX_ST_USR_CLR_0_A" },
   { R200_SE_VTX_ST_USR_CLR_1_R, "R200_SE_VTX_ST_USR_CLR_1_R" },
   { R200_SE_VTX_ST_USR_CLR_1_G, "R200_SE_VTX_ST_USR_CLR_1_G" },
   { R200_SE_VTX_ST_USR_CLR_1_B, "R200_SE_VTX_ST_USR_CLR_1_B" },
   { R200_SE_VTX_ST_USR_CLR_1_A, "R200_SE_VTX_ST_USR_CLR_1_A" },
   { R200_SE_VTX_ST_CLR_0_PKD, "R200_SE_VTX_ST_CLR_0_PKD" },
   { R200_SE_VTX_ST_CLR_1_PKD, "R200_SE_VTX_ST_CLR_1_PKD" },
   { R200_SE_VTX_ST_CLR_2_PKD, "R200_SE_VTX_ST_CLR_2_PKD" },
   { R200_SE_VTX_ST_CLR_3_PKD, "R200_SE_VTX_ST_CLR_3_PKD" },
   { R200_SE_VTX_ST_CLR_4_PKD, "R200_SE_VTX_ST_CLR_4_PKD" },
   { R200_SE_VTX_ST_CLR_5_PKD, "R200_SE_VTX_ST_CLR_5_PKD" },
   { R200_SE_VTX_ST_CLR_6_PKD, "R200_SE_VTX_ST_CLR_6_PKD" },
   { R200_SE_VTX_ST_CLR_7_PKD, "R200_SE_VTX_ST_CLR_7_PKD" },
   { R200_SE_VTX_ST_POS_0_X_2, "R200_SE_VTX_ST_POS_0_X_2" },
   { R200_SE_VTX_ST_POS_0_Y_2, "R200_SE_VTX_ST_POS_0_Y_2" },
   { R200_SE_VTX_ST_PAR_CLR_LD, "R200_SE_VTX_ST_PAR_CLR_LD" },
   { R200_SE_VTX_ST_USR_CLR_PKD, "R200_SE_VTX_ST_USR_CLR_PKD" },
   { R200_SE_VTX_ST_POS_0_X_3, "R200_SE_VTX_ST_POS_0_X_3" },
   { R200_SE_VTX_ST_POS_0_Y_3, "R200_SE_VTX_ST_POS_0_Y_3" },
   { R200_SE_VTX_ST_POS_0_Z_3, "R200_SE_VTX_ST_POS_0_Z_3" },
   { R200_SE_VTX_ST_END_OF_PKT, "R200_SE_VTX_ST_END_OF_PKT" },
   { R200_RE_POINTSIZE, "R200_RE_POINTSIZE" },
   { R200_RE_TOP_LEFT, "R200_RE_TOP_LEFT" },
   { R200_RE_AUX_SCISSOR_CNTL, "R200_RE_AUX_SCISSOR_CNTL" },
   { R200_PP_TXFILTER_0, "R200_PP_TXFILTER_0" },
   { R200_PP_TXFORMAT_0, "R200_PP_TXFORMAT_0" },
   { R200_PP_TXSIZE_0, "R200_PP_TXSIZE_0" },
   { R200_PP_TXFORMAT_X_0, "R200_PP_TXFORMAT_X_0" },
   { R200_PP_TXPITCH_0, "R200_PP_TXPITCH_0" },
   { R200_PP_BORDER_COLOR_0, "R200_PP_BORDER_COLOR_0" },
   { R200_PP_CUBIC_FACES_0, "R200_PP_CUBIC_FACES_0" },
   { R200_PP_TXMULTI_CTL_0, "R200_PP_TXMULTI_CTL_0" },
   { R200_PP_TXFILTER_1, "R200_PP_TXFILTER_1" },
   { R200_PP_TXFORMAT_1, "R200_PP_TXFORMAT_1" },
   { R200_PP_TXSIZE_1, "R200_PP_TXSIZE_1" },
   { R200_PP_TXFORMAT_X_1, "R200_PP_TXFORMAT_X_1" },
   { R200_PP_TXPITCH_1, "R200_PP_TXPITCH_1" },
   { R200_PP_BORDER_COLOR_1, "R200_PP_BORDER_COLOR_1" },
   { R200_PP_CUBIC_FACES_1, "R200_PP_CUBIC_FACES_1" },
   { R200_PP_TXMULTI_CTL_1, "R200_PP_TXMULTI_CTL_1" },
   { R200_PP_TXFILTER_2, "R200_PP_TXFILTER_2" },
   { R200_PP_TXFORMAT_2, "R200_PP_TXFORMAT_2" },
   { R200_PP_TXSIZE_2, "R200_PP_TXSIZE_2" },
   { R200_PP_TXFORMAT_X_2, "R200_PP_TXFORMAT_X_2" },
   { R200_PP_TXPITCH_2, "R200_PP_TXPITCH_2" },
   { R200_PP_BORDER_COLOR_2, "R200_PP_BORDER_COLOR_2" },
   { R200_PP_CUBIC_FACES_2, "R200_PP_CUBIC_FACES_2" },
   { R200_PP_TXMULTI_CTL_2, "R200_PP_TXMULTI_CTL_2" },
   { R200_PP_TXFILTER_3, "R200_PP_TXFILTER_3" },
   { R200_PP_TXFORMAT_3, "R200_PP_TXFORMAT_3" },
   { R200_PP_TXSIZE_3, "R200_PP_TXSIZE_3" },
   { R200_PP_TXFORMAT_X_3, "R200_PP_TXFORMAT_X_3" },
   { R200_PP_TXPITCH_3, "R200_PP_TXPITCH_3" },
   { R200_PP_BORDER_COLOR_3, "R200_PP_BORDER_COLOR_3" },
   { R200_PP_CUBIC_FACES_3, "R200_PP_CUBIC_FACES_3" },
   { R200_PP_TXMULTI_CTL_3, "R200_PP_TXMULTI_CTL_3" },
   { R200_PP_TXFILTER_4, "R200_PP_TXFILTER_4" },
   { R200_PP_TXFORMAT_4, "R200_PP_TXFORMAT_4" },
   { R200_PP_TXSIZE_4, "R200_PP_TXSIZE_4" },
   { R200_PP_TXFORMAT_X_4, "R200_PP_TXFORMAT_X_4" },
   { R200_PP_TXPITCH_4, "R200_PP_TXPITCH_4" },
   { R200_PP_BORDER_COLOR_4, "R200_PP_BORDER_COLOR_4" },
   { R200_PP_CUBIC_FACES_4, "R200_PP_CUBIC_FACES_4" },
   { R200_PP_TXMULTI_CTL_4, "R200_PP_TXMULTI_CTL_4" },
   { R200_PP_TXFILTER_5, "R200_PP_TXFILTER_5" },
   { R200_PP_TXFORMAT_5, "R200_PP_TXFORMAT_5" },
   { R200_PP_TXSIZE_5, "R200_PP_TXSIZE_5" },
   { R200_PP_TXFORMAT_X_5, "R200_PP_TXFORMAT_X_5" },
   { R200_PP_TXPITCH_5, "R200_PP_TXPITCH_5" },
   { R200_PP_BORDER_COLOR_5, "R200_PP_BORDER_COLOR_5" },
   { R200_PP_CUBIC_FACES_5, "R200_PP_CUBIC_FACES_5" },
   { R200_PP_TXMULTI_CTL_5, "R200_PP_TXMULTI_CTL_5" },
   { R200_PP_TXOFFSET_0, "R200_PP_TXOFFSET_0" },
   { R200_PP_CUBIC_OFFSET_F1_0, "R200_PP_CUBIC_OFFSET_F1_0" },
   { R200_PP_CUBIC_OFFSET_F2_0, "R200_PP_CUBIC_OFFSET_F2_0" },
   { R200_PP_CUBIC_OFFSET_F3_0, "R200_PP_CUBIC_OFFSET_F3_0" },
   { R200_PP_CUBIC_OFFSET_F4_0, "R200_PP_CUBIC_OFFSET_F4_0" },
   { R200_PP_CUBIC_OFFSET_F5_0, "R200_PP_CUBIC_OFFSET_F5_0" },
   { R200_PP_TXOFFSET_1, "R200_PP_TXOFFSET_1" },
   { R200_PP_CUBIC_OFFSET_F1_1, "R200_PP_CUBIC_OFFSET_F1_1" },
   { R200_PP_CUBIC_OFFSET_F2_1, "R200_PP_CUBIC_OFFSET_F2_1" },
   { R200_PP_CUBIC_OFFSET_F3_1, "R200_PP_CUBIC_OFFSET_F3_1" },
   { R200_PP_CUBIC_OFFSET_F4_1, "R200_PP_CUBIC_OFFSET_F4_1" },
   { R200_PP_CUBIC_OFFSET_F5_1, "R200_PP_CUBIC_OFFSET_F5_1" },
   { R200_PP_TXOFFSET_2, "R200_PP_TXOFFSET_2" },
   { R200_PP_CUBIC_OFFSET_F1_2, "R200_PP_CUBIC_OFFSET_F1_2" },
   { R200_PP_CUBIC_OFFSET_F2_2, "R200_PP_CUBIC_OFFSET_F2_2" },
   { R200_PP_CUBIC_OFFSET_F3_2, "R200_PP_CUBIC_OFFSET_F3_2" },
   { R200_PP_CUBIC_OFFSET_F4_2, "R200_PP_CUBIC_OFFSET_F4_2" },
   { R200_PP_CUBIC_OFFSET_F5_2, "R200_PP_CUBIC_OFFSET_F5_2" },
   { R200_PP_TXOFFSET_3, "R200_PP_TXOFFSET_3" },
   { R200_PP_CUBIC_OFFSET_F1_3, "R200_PP_CUBIC_OFFSET_F1_3" },
   { R200_PP_CUBIC_OFFSET_F2_3, "R200_PP_CUBIC_OFFSET_F2_3" },
   { R200_PP_CUBIC_OFFSET_F3_3, "R200_PP_CUBIC_OFFSET_F3_3" },
   { R200_PP_CUBIC_OFFSET_F4_3, "R200_PP_CUBIC_OFFSET_F4_3" },
   { R200_PP_CUBIC_OFFSET_F5_3, "R200_PP_CUBIC_OFFSET_F5_3" },
   { R200_PP_TXOFFSET_4, "R200_PP_TXOFFSET_4" },
   { R200_PP_CUBIC_OFFSET_F1_4, "R200_PP_CUBIC_OFFSET_F1_4" },
   { R200_PP_CUBIC_OFFSET_F2_4, "R200_PP_CUBIC_OFFSET_F2_4" },
   { R200_PP_CUBIC_OFFSET_F3_4, "R200_PP_CUBIC_OFFSET_F3_4" },
   { R200_PP_CUBIC_OFFSET_F4_4, "R200_PP_CUBIC_OFFSET_F4_4" },
   { R200_PP_CUBIC_OFFSET_F5_4, "R200_PP_CUBIC_OFFSET_F5_4" },
   { R200_PP_TXOFFSET_5, "R200_PP_TXOFFSET_5" },
   { R200_PP_CUBIC_OFFSET_F1_5, "R200_PP_CUBIC_OFFSET_F1_5" },
   { R200_PP_CUBIC_OFFSET_F2_5, "R200_PP_CUBIC_OFFSET_F2_5" },
   { R200_PP_CUBIC_OFFSET_F3_5, "R200_PP_CUBIC_OFFSET_F3_5" },
   { R200_PP_CUBIC_OFFSET_F4_5, "R200_PP_CUBIC_OFFSET_F4_5" },
   { R200_PP_CUBIC_OFFSET_F5_5, "R200_PP_CUBIC_OFFSET_F5_5" },
   { R200_PP_TAM_DEBUG3, "R200_PP_TAM_DEBUG3" },
   { R200_PP_TFACTOR_0, "R200_PP_TFACTOR_0" },
   { R200_PP_TFACTOR_1, "R200_PP_TFACTOR_1" },
   { R200_PP_TFACTOR_2, "R200_PP_TFACTOR_2" },
   { R200_PP_TFACTOR_3, "R200_PP_TFACTOR_3" },
   { R200_PP_TFACTOR_4, "R200_PP_TFACTOR_4" },
   { R200_PP_TFACTOR_5, "R200_PP_TFACTOR_5" },
   { R200_PP_TFACTOR_6, "R200_PP_TFACTOR_6" },
   { R200_PP_TFACTOR_7, "R200_PP_TFACTOR_7" },
   { R200_PP_TXCBLEND_0, "R200_PP_TXCBLEND_0" },
   { R200_PP_TXCBLEND2_0, "R200_PP_TXCBLEND2_0" },
   { R200_PP_TXABLEND_0, "R200_PP_TXABLEND_0" },
   { R200_PP_TXABLEND2_0, "R200_PP_TXABLEND2_0" },
   { R200_PP_TXCBLEND_1, "R200_PP_TXCBLEND_1" },
   { R200_PP_TXCBLEND2_1, "R200_PP_TXCBLEND2_1" },
   { R200_PP_TXABLEND_1, "R200_PP_TXABLEND_1" },
   { R200_PP_TXABLEND2_1, "R200_PP_TXABLEND2_1" },
   { R200_PP_TXCBLEND_2, "R200_PP_TXCBLEND_2" },
   { R200_PP_TXCBLEND2_2, "R200_PP_TXCBLEND2_2" },
   { R200_PP_TXABLEND_2, "R200_PP_TXABLEND_2" },
   { R200_PP_TXABLEND2_2, "R200_PP_TXABLEND2_2" },
   { R200_PP_TXCBLEND_3, "R200_PP_TXCBLEND_3" },
   { R200_PP_TXCBLEND2_3, "R200_PP_TXCBLEND2_3" },
   { R200_PP_TXABLEND_3, "R200_PP_TXABLEND_3" },
   { R200_PP_TXABLEND2_3, "R200_PP_TXABLEND2_3" },
   { R200_PP_TXCBLEND_4, "R200_PP_TXCBLEND_4" },
   { R200_PP_TXCBLEND2_4, "R200_PP_TXCBLEND2_4" },
   { R200_PP_TXABLEND_4, "R200_PP_TXABLEND_4" },
   { R200_PP_TXABLEND2_4, "R200_PP_TXABLEND2_4" },
   { R200_PP_TXCBLEND_5, "R200_PP_TXCBLEND_5" },
   { R200_PP_TXCBLEND2_5, "R200_PP_TXCBLEND2_5" },
   { R200_PP_TXABLEND_5, "R200_PP_TXABLEND_5" },
   { R200_PP_TXABLEND2_5, "R200_PP_TXABLEND2_5" },
   { R200_PP_TXCBLEND_6, "R200_PP_TXCBLEND_6" },
   { R200_PP_TXCBLEND2_6, "R200_PP_TXCBLEND2_6" },
   { R200_PP_TXABLEND_6, "R200_PP_TXABLEND_6" },
   { R200_PP_TXABLEND2_6, "R200_PP_TXABLEND2_6" },
   { R200_PP_TXCBLEND_7, "R200_PP_TXCBLEND_7" },
   { R200_PP_TXCBLEND2_7, "R200_PP_TXCBLEND2_7" },
   { R200_PP_TXABLEND_7, "R200_PP_TXABLEND_7" },
   { R200_PP_TXABLEND2_7, "R200_PP_TXABLEND2_7" },
   { R200_RB3D_BLENDCOLOR, "R200_RB3D_BLENDCOLOR" },
   { R200_RB3D_ABLENDCNTL, "R200_RB3D_ABLENDCNTL" },
   { R200_RB3D_CBLENDCNTL, "R200_RB3D_CBLENDCNTL" },
   { R200_SE_TCL_OUTPUT_VTX_COMP_SEL, "R200_SE_TCL_OUTPUT_VTX_COMP_SEL" },
   { R200_PP_CNTL_X, "R200_PP_CNTL_X" },
   { R200_SE_VAP_CNTL_STATUS, "R200_SE_VAP_CNTL_STATUS" },
   { R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0, "R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0" },
   { R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_1, "R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_1" },
   { R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_2, "R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_2" },
   { R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_3, "R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_3" },
   { R200_PP_TRI_PERF, "R200_PP_TRI_PERF" },
   { R200_PP_PERF_CNTL, "R200_PP_PERF_CNTL" },
   { R200_PP_TXCBLEND_8, "R200_PP_TXCBLEND_8" },
   { R200_PP_TXCBLEND2_8, "R200_PP_TXCBLEND2_8" },
   { R200_PP_TXABLEND_8, "R200_PP_TXABLEND_8" },
   { R200_PP_TXABLEND2_8, "R200_PP_TXABLEND2_8" },
   { R200_PP_TXCBLEND_9, "R200_PP_TXCBLEND_9" },
   { R200_PP_TXCBLEND2_9, "R200_PP_TXCBLEND2_9" },
   { R200_PP_TXABLEND_9, "R200_PP_TXABLEND_9" },
   { R200_PP_TXABLEND2_9, "R200_PP_TXABLEND2_9" },
   { R200_PP_TXCBLEND_10, "R200_PP_TXCBLEND_10" },
   { R200_PP_TXCBLEND2_10, "R200_PP_TXCBLEND2_10" },
   { R200_PP_TXABLEND_10, "R200_PP_TXABLEND_10" },
   { R200_PP_TXABLEND2_10, "R200_PP_TXABLEND2_10" },
   { R200_PP_TXCBLEND_11, "R200_PP_TXCBLEND_11" },
   { R200_PP_TXCBLEND2_11, "R200_PP_TXCBLEND2_11" },
   { R200_PP_TXABLEND_11, "R200_PP_TXABLEND_11" },
   { R200_PP_TXABLEND2_11, "R200_PP_TXABLEND2_11" },
   { R200_PP_TXCBLEND_12, "R200_PP_TXCBLEND_12" },
   { R200_PP_TXCBLEND2_12, "R200_PP_TXCBLEND2_12" },
   { R200_PP_TXABLEND_12, "R200_PP_TXABLEND_12" },
   { R200_PP_TXABLEND2_12, "R200_PP_TXABLEND2_12" },
   { R200_PP_TXCBLEND_13, "R200_PP_TXCBLEND_13" },
   { R200_PP_TXCBLEND2_13, "R200_PP_TXCBLEND2_13" },
   { R200_PP_TXABLEND_13, "R200_PP_TXABLEND_13" },
   { R200_PP_TXABLEND2_13, "R200_PP_TXABLEND2_13" },
   { R200_PP_TXCBLEND_14, "R200_PP_TXCBLEND_14" },
   { R200_PP_TXCBLEND2_14, "R200_PP_TXCBLEND2_14" },
   { R200_PP_TXABLEND_14, "R200_PP_TXABLEND_14" },
   { R200_PP_TXABLEND2_14, "R200_PP_TXABLEND2_14" },
   { R200_PP_TXCBLEND_15, "R200_PP_TXCBLEND_15" },
   { R200_PP_TXCBLEND2_15, "R200_PP_TXCBLEND2_15" },
   { R200_PP_TXABLEND_15, "R200_PP_TXABLEND_15" },
   { R200_PP_TXABLEND2_15, "R200_PP_TXABLEND2_15" },
   { R200_VAP_PVS_CNTL_1, "R200_VAP_PVS_CNTL_1" },
   { R200_VAP_PVS_CNTL_2, "R200_VAP_PVS_CNTL_2" },
};

static struct reg_names scalar_names[] = {
   { R200_SS_LIGHT_DCD_ADDR, "R200_SS_LIGHT_DCD_ADDR" },
   { R200_SS_LIGHT_DCM_ADDR, "R200_SS_LIGHT_DCM_ADDR" },
   { R200_SS_LIGHT_SPOT_EXPONENT_ADDR, "R200_SS_LIGHT_SPOT_EXPONENT_ADDR" },
   { R200_SS_LIGHT_SPOT_CUTOFF_ADDR, "R200_SS_LIGHT_SPOT_CUTOFF_ADDR" },
   { R200_SS_LIGHT_SPECULAR_THRESH_ADDR, "R200_SS_LIGHT_SPECULAR_THRESH_ADDR" },
   { R200_SS_LIGHT_RANGE_CUTOFF_SQRD, "R200_SS_LIGHT_RANGE_CUTOFF_SQRD" },
   { R200_SS_LIGHT_RANGE_ATT_CONST, "R200_SS_LIGHT_RANGE_ATT_CONST" },
   { R200_SS_VERT_GUARD_CLIP_ADJ_ADDR, "R200_SS_VERT_GUARD_CLIP_ADJ_ADDR" },
   { R200_SS_VERT_GUARD_DISCARD_ADJ_ADDR, "R200_SS_VERT_GUARD_DISCARD_ADJ_ADDR" },
   { R200_SS_HORZ_GUARD_CLIP_ADJ_ADDR, "R200_SS_HORZ_GUARD_CLIP_ADJ_ADDR" },
   { R200_SS_HORZ_GUARD_DISCARD_ADJ_ADDR, "R200_SS_HORZ_GUARD_DISCARD_ADJ_ADDR" },
   { R200_SS_MAT_0_SHININESS, "R200_SS_MAT_0_SHININESS" },
   { R200_SS_MAT_1_SHININESS, "R200_SS_MAT_1_SHININESS" },
   { 1000, "" },
};

/* Puff these out to make them look like normal (dword) registers.
 */
static struct reg_names vector_names[] = {
   { 0, "start" },
   { R200_VS_LIGHT_AMBIENT_ADDR, "R200_VS_LIGHT_AMBIENT_ADDR" },
   { R200_VS_LIGHT_DIFFUSE_ADDR, "R200_VS_LIGHT_DIFFUSE_ADDR" },
   { R200_VS_LIGHT_SPECULAR_ADDR, "R200_VS_LIGHT_SPECULAR_ADDR" },
   { R200_VS_LIGHT_DIRPOS_ADDR, "R200_VS_LIGHT_DIRPOS_ADDR" },
   { R200_VS_LIGHT_HWVSPOT_ADDR, "R200_VS_LIGHT_HWVSPOT_ADDR" },
   { R200_VS_LIGHT_ATTENUATION_ADDR, "R200_VS_LIGHT_ATTENUATION_ADDR" },
   { R200_VS_SPOT_DUAL_CONE, "R200_VS_SPOT_DUAL_CONE" },
   { R200_VS_GLOBAL_AMBIENT_ADDR, "R200_VS_GLOBAL_AMBIENT_ADDR" },
   { R200_VS_FOG_PARAM_ADDR, "R200_VS_FOG_PARAM_ADDR" },
   { R200_VS_EYE_VECTOR_ADDR, "R200_VS_EYE_VECTOR_ADDR" },
   { R200_VS_UCP_ADDR, "R200_VS_UCP_ADDR" },
   { R200_VS_PNT_SPRITE_VPORT_SCALE, "R200_VS_PNT_SPRITE_VPORT_SCALE" },
   { R200_VS_MATRIX_0_MV, "R200_VS_MATRIX_0_MV" },
   { R200_VS_MATRIX_1_INV_MV, "R200_VS_MATRIX_1_INV_MV" },
   { R200_VS_MATRIX_2_MVP, "R200_VS_MATRIX_2_MVP" },
   { R200_VS_MATRIX_3_TEX0, "R200_VS_MATRIX_3_TEX0" },
   { R200_VS_MATRIX_4_TEX1, "R200_VS_MATRIX_4_TEX1" },
   { R200_VS_MATRIX_5_TEX2, "R200_VS_MATRIX_5_TEX2" },
   { R200_VS_MATRIX_6_TEX3, "R200_VS_MATRIX_6_TEX3" },
   { R200_VS_MATRIX_7_TEX4, "R200_VS_MATRIX_7_TEX4" },
   { R200_VS_MATRIX_8_TEX5, "R200_VS_MATRIX_8_TEX5" },
   { R200_VS_MAT_0_EMISS, "R200_VS_MAT_0_EMISS" },
   { R200_VS_MAT_0_AMB, "R200_VS_MAT_0_AMB" },
   { R200_VS_MAT_0_DIF, "R200_VS_MAT_0_DIF" },
   { R200_VS_MAT_0_SPEC, "R200_VS_MAT_0_SPEC" },
   { R200_VS_MAT_1_EMISS, "R200_VS_MAT_1_EMISS" },
   { R200_VS_MAT_1_AMB, "R200_VS_MAT_1_AMB" },
   { R200_VS_MAT_1_DIF, "R200_VS_MAT_1_DIF" },
   { R200_VS_MAT_1_SPEC, "R200_VS_MAT_1_SPEC" },
   { R200_VS_EYE2CLIP_MTX, "R200_VS_EYE2CLIP_MTX" },
   { R200_VS_PNT_SPRITE_ATT_CONST, "R200_VS_PNT_SPRITE_ATT_CONST" },
   { R200_VS_PNT_SPRITE_EYE_IN_MODEL, "R200_VS_PNT_SPRITE_EYE_IN_MODEL" },
   { R200_VS_PNT_SPRITE_CLAMP, "R200_VS_PNT_SPRITE_CLAMP" },
   { R200_VS_MAX, "R200_VS_MAX" },
   { 1000, "" },
};

union fi { float f; int i; };

#define ISVEC   1
#define ISFLOAT 2
#define TOUCHED 4

struct reg {
   int idx; 
   struct reg_names *closest;
   int flags;
   union fi current;
   union fi *values;
   int nvalues;
   int nalloc;
   float vmin, vmax;
};


static struct reg regs[Elements(reg_names)+1];
static struct reg scalars[512+1];
static struct reg vectors[512*4+1];

static int total, total_changed, bufs;

static void init_regs( void )
{
   struct reg_names *tmp;
   int i;

   for (i = 0 ; i < Elements(regs) ; i++) {
      regs[i].idx = reg_names[i].idx;
      regs[i].closest = &reg_names[i];
      regs[i].flags = 0;
   }

   for (i = 0, tmp = scalar_names ; i < Elements(scalars) ; i++) {
      if (tmp[1].idx == i) tmp++;
      scalars[i].idx = i;
      scalars[i].closest = tmp;
      scalars[i].flags = ISFLOAT;
   }

   for (i = 0, tmp = vector_names ; i < Elements(vectors) ; i++) {
      if (tmp[1].idx*4 == i) tmp++;
      vectors[i].idx = i;
      vectors[i].closest = tmp;
      vectors[i].flags = ISFLOAT|ISVEC;
   }

   regs[Elements(regs)-1].idx = -1;
   scalars[Elements(scalars)-1].idx = -1;
   vectors[Elements(vectors)-1].idx = -1;
}

static int find_or_add_value( struct reg *reg, int val )
{
   int j;

   for ( j = 0 ; j < reg->nvalues ; j++)
      if ( val == reg->values[j].i )
	 return 1;

   if (j == reg->nalloc) {
      reg->nalloc += 5;
      reg->nalloc *= 2;
      reg->values = (union fi *) realloc( reg->values, 
					  reg->nalloc * sizeof(union fi) );
   }

   reg->values[reg->nvalues++].i = val;
   return 0;
}

static struct reg *lookup_reg( struct reg *tab, int reg )
{
   int i;

   for (i = 0 ; tab[i].idx != -1 ; i++) {
      if (tab[i].idx == reg)
	 return &tab[i];
   }

   fprintf(stderr, "*** unknown reg 0x%x\n", reg);
   return NULL;
}


static const char *get_reg_name( struct reg *reg )
{
   static char tmp[80];

   if (reg->idx == reg->closest->idx) 
      return reg->closest->name;

   
   if (reg->flags & ISVEC) {
      if (reg->idx/4 != reg->closest->idx)
	 sprintf(tmp, "%s+%d[%d]", 
		 reg->closest->name, 
		 (reg->idx/4) - reg->closest->idx,
		 reg->idx%4);
      else
	 sprintf(tmp, "%s[%d]", reg->closest->name, reg->idx%4);
   }
   else {
      if (reg->idx != reg->closest->idx)
	 sprintf(tmp, "%s+%d", reg->closest->name, reg->idx - reg->closest->idx);
      else
	 sprintf(tmp, "%s", reg->closest->name);
   }

   return tmp;
}

static int print_int_reg_assignment( struct reg *reg, int data )
{
   int changed = (reg->current.i != data);
   int ever_seen = find_or_add_value( reg, data );
   
   if (VERBOSE || (NORMAL && (changed || !ever_seen)))
       fprintf(stderr, "   %s <-- 0x%x", get_reg_name(reg), data);
       
   if (NORMAL) {
      if (!ever_seen) 
	 fprintf(stderr, " *** BRAND NEW VALUE");
      else if (changed) 
	 fprintf(stderr, " *** CHANGED"); 
   }
   
   reg->current.i = data;

   if (VERBOSE || (NORMAL && (changed || !ever_seen)))
      fprintf(stderr, "\n");

   return changed;
}


static int print_float_reg_assignment( struct reg *reg, float data )
{
   int changed = (reg->current.f != data);
   int newmin = (data < reg->vmin);
   int newmax = (data > reg->vmax);

   if (VERBOSE || (NORMAL && (newmin || newmax || changed)))
      fprintf(stderr, "   %s <-- %.3f", get_reg_name(reg), data);

   if (NORMAL) {
      if (newmin) {
	 fprintf(stderr, " *** NEW MIN (prev %.3f)", reg->vmin);
	 reg->vmin = data;
      }
      else if (newmax) {
	 fprintf(stderr, " *** NEW MAX (prev %.3f)", reg->vmax);
	 reg->vmax = data;
      }
      else if (changed) {
	 fprintf(stderr, " *** CHANGED");
      }
   }

   reg->current.f = data;

   if (VERBOSE || (NORMAL && (newmin || newmax || changed)))
      fprintf(stderr, "\n");

   return changed;
}

static int print_reg_assignment( struct reg *reg, int data )
{
   float_ui32_type datau;
   datau.ui32 = data;
   reg->flags |= TOUCHED;
   if (reg->flags & ISFLOAT)
      return print_float_reg_assignment( reg, datau.f );
   else
      return print_int_reg_assignment( reg, data );
}

static void print_reg( struct reg *reg )
{
   if (reg->flags & TOUCHED) {
      if (reg->flags & ISFLOAT) {
	 fprintf(stderr, "   %s == %f\n", get_reg_name(reg), reg->current.f);
      } else {
	 fprintf(stderr, "   %s == 0x%x\n", get_reg_name(reg), reg->current.i);
      }
   }
}


static void dump_state( void )
{
   int i;

   for (i = 0 ; i < Elements(regs) ; i++) 
      print_reg( &regs[i] );

   for (i = 0 ; i < Elements(scalars) ; i++) 
      print_reg( &scalars[i] );

   for (i = 0 ; i < Elements(vectors) ; i++) 
      print_reg( &vectors[i] );
}



static int radeon_emit_packets( 
   drm_radeon_cmd_header_t header,
   drm_radeon_cmd_buffer_t *cmdbuf )
{
   int id = (int)header.packet.packet_id;
   int sz = packet[id].len;
   int *data = (int *)cmdbuf->buf;
   int i;
   
   if (sz * sizeof(int) > cmdbuf->bufsz) {
      fprintf(stderr, "Packet overflows cmdbuf\n");      
      return -EINVAL;
   }

   if (!packet[id].name) {
      fprintf(stderr, "*** Unknown packet 0 nr %d\n", id );
      return -EINVAL;
   }

   
   if (VERBOSE) 
      fprintf(stderr, "Packet 0 reg %s nr %d\n", packet[id].name, sz );

   for ( i = 0 ; i < sz ; i++) {
      struct reg *reg = lookup_reg( regs, packet[id].start + i*4 );
      if (print_reg_assignment( reg, data[i] ))
	 total_changed++;
      total++;
   }

   cmdbuf->buf += sz * sizeof(int);
   cmdbuf->bufsz -= sz * sizeof(int);
   return 0;
}


static int radeon_emit_scalars( 
   drm_radeon_cmd_header_t header,
   drm_radeon_cmd_buffer_t *cmdbuf )
{
   int sz = header.scalars.count;
   int *data = (int *)cmdbuf->buf;
   int start = header.scalars.offset;
   int stride = header.scalars.stride;
   int i;

   if (VERBOSE)
      fprintf(stderr, "emit scalars, start %d stride %d nr %d (end %d)\n",
	      start, stride, sz, start + stride * sz);


   for (i = 0 ; i < sz ; i++, start += stride) {
      struct reg *reg = lookup_reg( scalars, start );
      if (print_reg_assignment( reg, data[i] ))
	 total_changed++;
      total++;
   }
	 
   cmdbuf->buf += sz * sizeof(int);
   cmdbuf->bufsz -= sz * sizeof(int);
   return 0;
}


static int radeon_emit_scalars2( 
   drm_radeon_cmd_header_t header,
   drm_radeon_cmd_buffer_t *cmdbuf )
{
   int sz = header.scalars.count;
   int *data = (int *)cmdbuf->buf;
   int start = header.scalars.offset + 0x100;
   int stride = header.scalars.stride;
   int i;

   if (VERBOSE)
      fprintf(stderr, "emit scalars2, start %d stride %d nr %d (end %d)\n",
	      start, stride, sz, start + stride * sz);

   if (start + stride * sz > 258) {
      fprintf(stderr, "emit scalars OVERFLOW %d/%d/%d\n", start, stride, sz);
      return -1;
   }

   for (i = 0 ; i < sz ; i++, start += stride) {
      struct reg *reg = lookup_reg( scalars, start );
      if (print_reg_assignment( reg, data[i] ))
	 total_changed++;
      total++;
   }
	 
   cmdbuf->buf += sz * sizeof(int);
   cmdbuf->bufsz -= sz * sizeof(int);
   return 0;
}

/* Check: inf/nan/extreme-size?
 * Check: table start, end, nr, etc.
 */
static int radeon_emit_vectors( 
   drm_radeon_cmd_header_t header,
   drm_radeon_cmd_buffer_t *cmdbuf )
{
   int sz = header.vectors.count;
   int *data = (int *)cmdbuf->buf;
   int start = header.vectors.offset;
   int stride = header.vectors.stride;
   int i,j;

   if (VERBOSE)
      fprintf(stderr, "emit vectors, start %d stride %d nr %d (end %d) (0x%x)\n",
	      start, stride, sz, start + stride * sz, header.i);

/*    if (start + stride * (sz/4) > 128) { */
/*       fprintf(stderr, "emit vectors OVERFLOW %d/%d/%d\n", start, stride, sz); */
/*       return -1; */
/*    } */

   for (i = 0 ; i < sz ;  start += stride) {
      int changed = 0;
      for (j = 0 ; j < 4 ; i++,j++) {
	 struct reg *reg = lookup_reg( vectors, start*4+j );
	 if (print_reg_assignment( reg, data[i] ))
	    changed = 1;
      }
      if (changed)
	 total_changed += 4;
      total += 4;
   }
	 

   cmdbuf->buf += sz * sizeof(int);
   cmdbuf->bufsz -= sz * sizeof(int);
   return 0;
}

static int radeon_emit_veclinear( 
   drm_radeon_cmd_header_t header,
   drm_radeon_cmd_buffer_t *cmdbuf )
{
   int sz = header.veclinear.count * 4;
   int *data = (int *)cmdbuf->buf;
   float *fdata =(float *)cmdbuf->buf;
   int start = header.veclinear.addr_lo | (header.veclinear.addr_hi << 8);
   int i;

   if (1||VERBOSE)
      fprintf(stderr, "emit vectors linear, start %d nr %d (end %d) (0x%x)\n",
	      start, sz >> 2, start + (sz >> 2), header.i);


   if (start < 0x60) {
      for (i = 0 ; i < sz ;  i += 4) {
	 fprintf(stderr, "R200_VS_PARAM %d 0 %f\n", (i >> 2) + start, fdata[i]);
	 fprintf(stderr, "R200_VS_PARAM %d 1 %f\n", (i >> 2) + start, fdata[i+1]);
	 fprintf(stderr, "R200_VS_PARAM %d 2 %f\n", (i >> 2) + start, fdata[i+2]);
	 fprintf(stderr, "R200_VS_PARAM %d 3 %f\n", (i >> 2) + start, fdata[i+3]);
      }
   }
   else if ((start >= 0x100) && (start < 0x160)) {
      for (i = 0 ; i < sz ;  i += 4) {
	 fprintf(stderr, "R200_VS_PARAM %d 0 %f\n", (i >> 2) + start - 0x100 + 0x60, fdata[i]);
	 fprintf(stderr, "R200_VS_PARAM %d 1 %f\n", (i >> 2) + start - 0x100 + 0x60, fdata[i+1]);
	 fprintf(stderr, "R200_VS_PARAM %d 2 %f\n", (i >> 2) + start - 0x100 + 0x60, fdata[i+2]);
	 fprintf(stderr, "R200_VS_PARAM %d 3 %f\n", (i >> 2) + start - 0x100 + 0x60, fdata[i+3]);
      }
   }
   else if ((start >= 0x80) && (start < 0xc0)) {
      for (i = 0 ; i < sz ;  i += 4) {
	 fprintf(stderr, "R200_VS_PROG %d OPDST %08x\n", (i >> 2) + start - 0x80, data[i]);
	 fprintf(stderr, "R200_VS_PROG %d SRC1  %08x\n", (i >> 2) + start - 0x80, data[i+1]);
	 fprintf(stderr, "R200_VS_PROG %d SRC2  %08x\n", (i >> 2) + start - 0x80, data[i+2]);
	 fprintf(stderr, "R200_VS_PROG %d SRC3  %08x\n", (i >> 2) + start - 0x80, data[i+3]);
      }
   }
   else if ((start >= 0x180) && (start < 0x1c0)) {
      for (i = 0 ; i < sz ;  i += 4) {
	 fprintf(stderr, "R200_VS_PROG %d OPDST %08x\n", (i >> 2) + start - 0x180 + 0x40, data[i]);
	 fprintf(stderr, "R200_VS_PROG %d SRC1  %08x\n", (i >> 2) + start - 0x180 + 0x40, data[i+1]);
	 fprintf(stderr, "R200_VS_PROG %d SRC2  %08x\n", (i >> 2) + start - 0x180 + 0x40, data[i+2]);
	 fprintf(stderr, "R200_VS_PROG %d SRC3  %08x\n", (i >> 2) + start - 0x180 + 0x40, data[i+3]);
      }
   }
   else {
      fprintf(stderr, "write to unknown vector area\n");
   }

   cmdbuf->buf += sz * sizeof(int);
   cmdbuf->bufsz -= sz * sizeof(int);
   return 0;
}

#if 0
static int print_vertex_format( int vfmt )
{
   if (NORMAL) {
      fprintf(stderr, "   %s(%x): %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
	      "vertex format",
	      vfmt,
	      "xy,",
	      (vfmt & R200_VTX_Z0) ? "z," : "",
	      (vfmt & R200_VTX_W0) ? "w0," : "",
	      (vfmt & R200_VTX_FPCOLOR) ? "fpcolor," : "",
	      (vfmt & R200_VTX_FPALPHA) ? "fpalpha," : "",
	      (vfmt & R200_VTX_PKCOLOR) ? "pkcolor," : "",
	      (vfmt & R200_VTX_FPSPEC) ? "fpspec," : "",
	      (vfmt & R200_VTX_FPFOG) ? "fpfog," : "",
	      (vfmt & R200_VTX_PKSPEC) ? "pkspec," : "",
	      (vfmt & R200_VTX_ST0) ? "st0," : "",
	      (vfmt & R200_VTX_ST1) ? "st1," : "",
	      (vfmt & R200_VTX_Q1) ? "q1," : "",
	      (vfmt & R200_VTX_ST2) ? "st2," : "",
	      (vfmt & R200_VTX_Q2) ? "q2," : "",
	      (vfmt & R200_VTX_ST3) ? "st3," : "",
	      (vfmt & R200_VTX_Q3) ? "q3," : "",
	      (vfmt & R200_VTX_Q0) ? "q0," : "",
	      (vfmt & R200_VTX_N0) ? "n0," : "",
	      (vfmt & R200_VTX_XY1) ? "xy1," : "",
	      (vfmt & R200_VTX_Z1) ? "z1," : "",
	      (vfmt & R200_VTX_W1) ? "w1," : "",
	      (vfmt & R200_VTX_N1) ? "n1," : "");

   
      if (!find_or_add_value( &others[V_VTXFMT], vfmt ))
	 fprintf(stderr, " *** NEW VALUE");

      fprintf(stderr, "\n");
   }

   return 0;
}
#endif

static char *primname[0x10] = {
   "NONE",
   "POINTS",
   "LINES",
   "LINE_STRIP",
   "TRIANGLES",
   "TRIANGLE_FAN",
   "TRIANGLE_STRIP",
   "RECT_LIST",
   NULL,
   "3VRT_POINTS",
   "3VRT_LINES",
   "POINT_SPRITES",
   "LINE_LOOP",
   "QUADS",
   "QUAD_STRIP",
   "POLYGON",
};

static int print_prim_and_flags( int prim )
{
   int numverts;
   
   if (NORMAL)
      fprintf(stderr, "   %s(%x): %s%s%s%s%s%s\n",
	      "prim flags",
	      prim,
	      ((prim & 0x30) == R200_VF_PRIM_WALK_IND) ? "IND," : "",
	      ((prim & 0x30) == R200_VF_PRIM_WALK_LIST) ? "LIST," : "",
	      ((prim & 0x30) == R200_VF_PRIM_WALK_RING) ? "RING," : "",
	      (prim & R200_VF_COLOR_ORDER_RGBA) ? "RGBA," : "BGRA, ",
	      (prim & R200_VF_INDEX_SZ_4) ? "INDX-32," : "",
	      (prim & R200_VF_TCL_OUTPUT_VTX_ENABLE) ? "TCL_OUT_VTX," : "");

   numverts = prim>>16;
   
   if (NORMAL)
      fprintf(stderr, "   prim: %s numverts %d\n", primname[prim&0xf], numverts);

   switch (prim & 0xf) {
   case R200_VF_PRIM_NONE:
   case R200_VF_PRIM_POINTS:
      if (numverts < 1) {
	 fprintf(stderr, "Bad nr verts for line %d\n", numverts);
	 return -1;
      }
      break;
   case R200_VF_PRIM_LINES:
   case R200_VF_PRIM_POINT_SPRITES:
      if ((numverts & 1) || numverts == 0) {
	 fprintf(stderr, "Bad nr verts for line %d\n", numverts);
	 return -1;
      }
      break;
   case R200_VF_PRIM_LINE_STRIP:
   case R200_VF_PRIM_LINE_LOOP:
      if (numverts < 2) {
	 fprintf(stderr, "Bad nr verts for line_strip %d\n", numverts);
	 return -1;
      }
      break;
   case R200_VF_PRIM_TRIANGLES:
   case R200_VF_PRIM_3VRT_POINTS:
   case R200_VF_PRIM_3VRT_LINES:
   case R200_VF_PRIM_RECT_LIST:
      if (numverts % 3 || numverts == 0) {
	 fprintf(stderr, "Bad nr verts for tri %d\n", numverts);
	 return -1;
      }
      break;
   case R200_VF_PRIM_TRIANGLE_FAN:
   case R200_VF_PRIM_TRIANGLE_STRIP:
   case R200_VF_PRIM_POLYGON:
      if (numverts < 3) {
	 fprintf(stderr, "Bad nr verts for strip/fan %d\n", numverts);
	 return -1;
      }
      break;
   case R200_VF_PRIM_QUADS:
      if (numverts % 4 || numverts == 0) {
	 fprintf(stderr, "Bad nr verts for quad %d\n", numverts);
	 return -1;
      }
      break;
   case R200_VF_PRIM_QUAD_STRIP:
      if (numverts % 2 || numverts < 4) {
	 fprintf(stderr, "Bad nr verts for quadstrip %d\n", numverts);
	 return -1;
      }
      break;
   default:
      fprintf(stderr, "Bad primitive\n");
      return -1;
   }	
   return 0;
}

/* build in knowledge about each packet type
 */
static int radeon_emit_packet3( drm_radeon_cmd_buffer_t *cmdbuf )
{
   int cmdsz;
   int *cmd = (int *)cmdbuf->buf;
   int *tmp;
   int i, stride, size, start;

   cmdsz = 2 + ((cmd[0] & RADEON_CP_PACKET_COUNT_MASK) >> 16);

   if ((cmd[0] & RADEON_CP_PACKET_MASK) != RADEON_CP_PACKET3 ||
       cmdsz * 4 > cmdbuf->bufsz ||
       cmdsz > RADEON_CP_PACKET_MAX_DWORDS) {
      fprintf(stderr, "Bad packet\n");
      return -EINVAL;
   }

   switch( cmd[0] & ~RADEON_CP_PACKET_COUNT_MASK ) {
   case R200_CP_CMD_NOP:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_NOP, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_NEXT_CHAR:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_NEXT_CHAR, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_PLY_NEXTSCAN:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_PLY_NEXTSCAN, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_SET_SCISSORS:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_SET_SCISSORS, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_LOAD_MICROCODE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_LOAD_MICROCODE, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_WAIT_FOR_IDLE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_WAIT_FOR_IDLE, %d dwords\n", cmdsz);
      break;

   case R200_CP_CMD_3D_DRAW_VBUF:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_3D_DRAW_VBUF, %d dwords\n", cmdsz);
/*       print_vertex_format(cmd[1]); */
      if (print_prim_and_flags(cmd[2]))
	 return -EINVAL;
      break;

   case R200_CP_CMD_3D_DRAW_IMMD:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_3D_DRAW_IMMD, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_3D_DRAW_INDX: {
      int neltdwords;
      if (NORMAL)
	 fprintf(stderr, "PACKET3_3D_DRAW_INDX, %d dwords\n", cmdsz);
/*       print_vertex_format(cmd[1]); */
      if (print_prim_and_flags(cmd[2]))
	 return -EINVAL;
      neltdwords = cmd[2]>>16;
      neltdwords += neltdwords & 1;
      neltdwords /= 2;
      if (neltdwords + 3 != cmdsz)
	 fprintf(stderr, "Mismatch in DRAW_INDX, %d vs cmdsz %d\n",
		 neltdwords, cmdsz);
      break;
   }
   case R200_CP_CMD_LOAD_PALETTE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_LOAD_PALETTE, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_3D_LOAD_VBPNTR:
      if (NORMAL) {
	 fprintf(stderr, "PACKET3_3D_LOAD_VBPNTR, %d dwords\n", cmdsz);
	 fprintf(stderr, "   nr arrays: %d\n", cmd[1]);
      }

      if (((cmd[1]/2)*3) + ((cmd[1]%2)*2) != cmdsz - 2) {
	 fprintf(stderr, "  ****** MISMATCH %d/%d *******\n",
		 ((cmd[1]/2)*3) + ((cmd[1]%2)*2) + 2, cmdsz);
	 return -EINVAL;
      }

      if (NORMAL) {
	 tmp = cmd+2;
	 for (i = 0 ; i < cmd[1] ; i++) {
	    if (i & 1) {
	       stride = (tmp[0]>>24) & 0xff;
	       size = (tmp[0]>>16) & 0xff;
	       start = tmp[2];
	       tmp += 3;
	    }
	    else {
	       stride = (tmp[0]>>8) & 0xff;
	       size = (tmp[0]) & 0xff;
	       start = tmp[1];
	    }
	    fprintf(stderr, "   array %d: start 0x%x vsize %d vstride %d\n",
		    i, start, size, stride );
	 }
      }
      break;
   case R200_CP_CMD_PAINT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_PAINT, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_BITBLT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_BITBLT, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_SMALLTEXT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_SMALLTEXT, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_HOSTDATA_BLT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_HOSTDATA_BLT, %d dwords\n", 
	      cmdsz);
      break;
   case R200_CP_CMD_POLYLINE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_POLYLINE, %d dwords\n", cmdsz);
      break;
   case R200_CP_CMD_POLYSCANLINES:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_POLYSCANLINES, %d dwords\n", 
	      cmdsz);
      break;
   case R200_CP_CMD_PAINT_MULTI:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_PAINT_MULTI, %d dwords\n", 
	      cmdsz);
      break;
   case R200_CP_CMD_BITBLT_MULTI:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_BITBLT_MULTI, %d dwords\n", 
	      cmdsz);
      break;
   case R200_CP_CMD_TRANS_BITBLT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_TRANS_BITBLT, %d dwords\n", 
	      cmdsz);
      break;
   case R200_CP_CMD_3D_DRAW_VBUF_2:
      if (NORMAL)
	 fprintf(stderr, "R200_CP_CMD_3D_DRAW_VBUF_2, %d dwords\n", 
	      cmdsz);
      if (print_prim_and_flags(cmd[1]))
	 return -EINVAL;
      break;
   case R200_CP_CMD_3D_DRAW_IMMD_2:
      if (NORMAL)
	 fprintf(stderr, "R200_CP_CMD_3D_DRAW_IMMD_2, %d dwords\n", 
	      cmdsz);
      if (print_prim_and_flags(cmd[1]))
	 return -EINVAL;
      break;
   case R200_CP_CMD_3D_DRAW_INDX_2:
      if (NORMAL)
	 fprintf(stderr, "R200_CP_CMD_3D_DRAW_INDX_2, %d dwords\n", 
	      cmdsz);
      if (print_prim_and_flags(cmd[1]))
	 return -EINVAL;
      break;
   default:
      fprintf(stderr, "UNKNOWN PACKET, %d dwords\n", cmdsz);
      break;
   }
      
   cmdbuf->buf += cmdsz * 4;
   cmdbuf->bufsz -= cmdsz * 4;
   return 0;
}


/* Check cliprects for bounds, then pass on to above:
 */
static int radeon_emit_packet3_cliprect( drm_radeon_cmd_buffer_t *cmdbuf )
{   
   drm_clip_rect_t *boxes = (drm_clip_rect_t *)cmdbuf->boxes;
   int i = 0;

   if (VERBOSE && total_changed) {
      dump_state();
      total_changed = 0;
   }

   if (NORMAL) {
      do {
	 if ( i < cmdbuf->nbox ) {
	    fprintf(stderr, "Emit box %d/%d %d,%d %d,%d\n",
		    i, cmdbuf->nbox,
		    boxes[i].x1, boxes[i].y1, boxes[i].x2, boxes[i].y2);
	 }
      } while ( ++i < cmdbuf->nbox );
   }

   if (cmdbuf->nbox == 1)
      cmdbuf->nbox = 0;

   return radeon_emit_packet3( cmdbuf );
}


int r200SanityCmdBuffer( r200ContextPtr rmesa,
			   int nbox,
			   drm_clip_rect_t *boxes )
{
   int idx;
   drm_radeon_cmd_buffer_t cmdbuf;
   drm_radeon_cmd_header_t header;
   static int inited = 0;

   if (!inited) {
      init_regs();
      inited = 1;
   }


   cmdbuf.buf = rmesa->store.cmd_buf;
   cmdbuf.bufsz = rmesa->store.cmd_used;
   cmdbuf.boxes = (drm_clip_rect_t *)boxes;
   cmdbuf.nbox = nbox;

   while ( cmdbuf.bufsz >= sizeof(header) ) {
		
      header.i = *(int *)cmdbuf.buf;
      cmdbuf.buf += sizeof(header);
      cmdbuf.bufsz -= sizeof(header);

      switch (header.header.cmd_type) {
      case RADEON_CMD_PACKET: 
	 if (radeon_emit_packets( header, &cmdbuf )) {
	    fprintf(stderr,"radeon_emit_packets failed\n");
	    return -EINVAL;
	 }
	 break;

      case RADEON_CMD_SCALARS:
	 if (radeon_emit_scalars( header, &cmdbuf )) {
	    fprintf(stderr,"radeon_emit_scalars failed\n");
	    return -EINVAL;
	 }
	 break;

      case RADEON_CMD_SCALARS2:
	 if (radeon_emit_scalars2( header, &cmdbuf )) {
	    fprintf(stderr,"radeon_emit_scalars failed\n");
	    return -EINVAL;
	 }
	 break;

      case RADEON_CMD_VECTORS:
	 if (radeon_emit_vectors( header, &cmdbuf )) {
	    fprintf(stderr,"radeon_emit_vectors failed\n");
	    return -EINVAL;
	 }
	 break;

      case RADEON_CMD_DMA_DISCARD:
	 idx = header.dma.buf_idx;
	 if (NORMAL)
	    fprintf(stderr, "RADEON_CMD_DMA_DISCARD buf %d\n", idx);
	 bufs++;
	 break;

      case RADEON_CMD_PACKET3:
	 if (radeon_emit_packet3( &cmdbuf )) {
	    fprintf(stderr,"radeon_emit_packet3 failed\n");
	    return -EINVAL;
	 }
	 break;

      case RADEON_CMD_PACKET3_CLIP:
	 if (radeon_emit_packet3_cliprect( &cmdbuf )) {
	    fprintf(stderr,"radeon_emit_packet3_clip failed\n");
	    return -EINVAL;
	 }
	 break;

      case RADEON_CMD_WAIT:
	 break;

      case RADEON_CMD_VECLINEAR:
	 if (radeon_emit_veclinear( header, &cmdbuf )) {
	    fprintf(stderr,"radeon_emit_veclinear failed\n");
	    return -EINVAL;
	 }
	 break;

      default:
	 fprintf(stderr,"bad cmd_type %d at %p\n", 
		   header.header.cmd_type,
		   cmdbuf.buf - sizeof(header));
	 return -EINVAL;
      }
   }

   if (0)
   {
      static int n = 0;
      n++;
      if (n == 10) {
	 fprintf(stderr, "Bufs %d Total emitted %d real changes %d (%.2f%%)\n",
		 bufs,
		 total, total_changed, 
		 ((float)total_changed/(float)total*100.0));
	 fprintf(stderr, "Total emitted per buf: %.2f\n",
		 (float)total/(float)bufs);
	 fprintf(stderr, "Real changes per buf: %.2f\n",
		 (float)total_changed/(float)bufs);

	 bufs = n = total = total_changed = 0;
      }
   }

   fprintf(stderr, "leaving %s\n\n\n", __FUNCTION__);

   return 0;
}
