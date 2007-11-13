/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_sanity.c,v 1.1 2002/10/30 12:51:55 alanh Exp $ */
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

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_sanity.h"

/* Set this '1' to get more verbiage.
 */
#define MORE_VERBOSE 1

#if MORE_VERBOSE
#define VERBOSE (RADEON_DEBUG & DEBUG_VERBOSE)
#define NORMAL  (1)
#else
#define VERBOSE 0
#define NORMAL  (RADEON_DEBUG & DEBUG_VERBOSE)
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
	{ 0, 4, "R200_PP_TXCBLEND_0" },
	{ 0, 4, "R200_PP_TXCBLEND_1" },
	{ 0, 4, "R200_PP_TXCBLEND_2" },
	{ 0, 4, "R200_PP_TXCBLEND_3" },
	{ 0, 4, "R200_PP_TXCBLEND_4" },
	{ 0, 4, "R200_PP_TXCBLEND_5" },
	{ 0, 4, "R200_PP_TXCBLEND_6" },
	{ 0, 4, "R200_PP_TXCBLEND_7" },
	{ 0, 6, "R200_SE_TCL_LIGHT_MODEL_CTL_0" },
	{ 0, 6, "R200_PP_TFACTOR_0" },
	{ 0, 4, "R200_SE_VTX_FMT_0" },
	{ 0, 1, "R200_SE_VAP_CNTL" },
	{ 0, 5, "R200_SE_TCL_MATRIX_SEL_0" },
	{ 0, 5, "R200_SE_TCL_TEX_PROC_CTL_2" },
	{ 0, 1, "R200_SE_TCL_UCP_VERT_BLEND_CTL" },
	{ 0, 6, "R200_PP_TXFILTER_0" },
	{ 0, 6, "R200_PP_TXFILTER_1" },
	{ 0, 6, "R200_PP_TXFILTER_2" },
	{ 0, 6, "R200_PP_TXFILTER_3" },
	{ 0, 6, "R200_PP_TXFILTER_4" },
	{ 0, 6, "R200_PP_TXFILTER_5" },
	{ 0, 1, "R200_PP_TXOFFSET_0" },
	{ 0, 1, "R200_PP_TXOFFSET_1" },
	{ 0, 1, "R200_PP_TXOFFSET_2" },
	{ 0, 1, "R200_PP_TXOFFSET_3" },
	{ 0, 1, "R200_PP_TXOFFSET_4" },
	{ 0, 1, "R200_PP_TXOFFSET_5" },
	{ 0, 1, "R200_SE_VTE_CNTL" },
	{ 0, 1, "R200_SE_TCL_OUTPUT_VTX_COMP_SEL" },
	{ 0, 1, "R200_PP_TAM_DEBUG3" },
	{ 0, 1, "R200_PP_CNTL_X" }, 
	{ 0, 1, "R200_RB3D_DEPTHXY_OFFSET" }, 
	{ 0, 1, "R200_RE_AUX_SCISSOR_CNTL" }, 
	{ 0, 2, "R200_RE_SCISSOR_TL_0" }, 
	{ 0, 2, "R200_RE_SCISSOR_TL_1" }, 
	{ 0, 2, "R200_RE_SCISSOR_TL_2" }, 
	{ 0, 1, "R200_SE_VAP_CNTL_STATUS" }, 
	{ 0, 1, "R200_SE_VTX_STATE_CNTL" }, 
	{ 0, 1, "R200_RE_POINTSIZE" }, 
	{ 0, 4, "R200_SE_TCL_INPUT_VTX_VECTOR_ADDR_0" },
	{ 0, 1, "R200_PP_CUBIC_FACES_0" }, /* 61 */
	{ 0, 5, "R200_PP_CUBIC_OFFSET_F1_0" }, /* 62 */
	{ 0, 1, "R200_PP_CUBIC_FACES_1" },
	{ 0, 5, "R200_PP_CUBIC_OFFSET_F1_1" },
	{ 0, 1, "R200_PP_CUBIC_FACES_2" },
	{ 0, 5, "R200_PP_CUBIC_OFFSET_F1_2" },
	{ 0, 1, "R200_PP_CUBIC_FACES_3" },
	{ 0, 5, "R200_PP_CUBIC_OFFSET_F1_3" },
	{ 0, 1, "R200_PP_CUBIC_FACES_4" },
	{ 0, 5, "R200_PP_CUBIC_OFFSET_F1_4" },
	{ 0, 1, "R200_PP_CUBIC_FACES_5" },
	{ 0, 5, "R200_PP_CUBIC_OFFSET_F1_5" },
   { RADEON_PP_TEX_SIZE_0, 2, "RADEON_PP_TEX_SIZE_0" },
   { RADEON_PP_TEX_SIZE_1, 2, "RADEON_PP_TEX_SIZE_1" },
   { RADEON_PP_TEX_SIZE_2, 2, "RADEON_PP_TEX_SIZE_2" },
	{ 0, 3, "R200_RB3D_BLENDCOLOR" },
	{ 0, 1, "R200_SE_TCL_POINT_SPRITE_CNTL" },
   { RADEON_PP_CUBIC_FACES_0, 1, "RADEON_PP_CUBIC_FACES_0" },
   { RADEON_PP_CUBIC_OFFSET_T0_0, 5, "RADEON_PP_CUBIC_OFFSET_T0_0" },
   { RADEON_PP_CUBIC_FACES_1, 1, "RADEON_PP_CUBIC_FACES_1" },
   { RADEON_PP_CUBIC_OFFSET_T1_0, 5, "RADEON_PP_CUBIC_OFFSET_T1_0" },
   { RADEON_PP_CUBIC_FACES_2, 1, "RADEON_PP_CUBIC_FACES_2" },
   { RADEON_PP_CUBIC_OFFSET_T2_0, 5, "RADEON_PP_CUBIC_OFFSET_T2_0" },
   { 0, 2, "R200_PP_TRI_PERF" },
   { 0, 32, "R200_PP_AFS_0"},   /* 85 */
   { 0, 32, "R200_PP_AFS_1"},
   { 0, 8, "R200_ATF_TFACTOR"},
   { 0, 8, "R200_PP_TXCTLALL_0"},
   { 0, 8, "R200_PP_TXCTLALL_1"},
   { 0, 8, "R200_PP_TXCTLALL_2"},
   { 0, 8, "R200_PP_TXCTLALL_3"},
   { 0, 8, "R200_PP_TXCTLALL_4"},
   { 0, 8, "R200_PP_TXCTLALL_5"},
   { 0, 2, "R200_VAP_PVS_CNTL"},
};

struct reg_names {
   int idx;
   const char *name;
};

static struct reg_names reg_names[] = {
   { RADEON_PP_MISC, "RADEON_PP_MISC" },
   { RADEON_PP_FOG_COLOR, "RADEON_PP_FOG_COLOR" },
   { RADEON_RE_SOLID_COLOR, "RADEON_RE_SOLID_COLOR" },
   { RADEON_RB3D_BLENDCNTL, "RADEON_RB3D_BLENDCNTL" },
   { RADEON_RB3D_DEPTHOFFSET, "RADEON_RB3D_DEPTHOFFSET" },
   { RADEON_RB3D_DEPTHPITCH, "RADEON_RB3D_DEPTHPITCH" },
   { RADEON_RB3D_ZSTENCILCNTL, "RADEON_RB3D_ZSTENCILCNTL" },
   { RADEON_PP_CNTL, "RADEON_PP_CNTL" },
   { RADEON_RB3D_CNTL, "RADEON_RB3D_CNTL" },
   { RADEON_RB3D_COLOROFFSET, "RADEON_RB3D_COLOROFFSET" },
   { RADEON_RB3D_COLORPITCH, "RADEON_RB3D_COLORPITCH" },
   { RADEON_SE_CNTL, "RADEON_SE_CNTL" },
   { RADEON_SE_COORD_FMT, "RADEON_SE_COORDFMT" },
   { RADEON_SE_CNTL_STATUS, "RADEON_SE_CNTL_STATUS" },
   { RADEON_RE_LINE_PATTERN, "RADEON_RE_LINE_PATTERN" },
   { RADEON_RE_LINE_STATE, "RADEON_RE_LINE_STATE" },
   { RADEON_SE_LINE_WIDTH, "RADEON_SE_LINE_WIDTH" },
   { RADEON_RB3D_STENCILREFMASK, "RADEON_RB3D_STENCILREFMASK" },
   { RADEON_RB3D_ROPCNTL, "RADEON_RB3D_ROPCNTL" },
   { RADEON_RB3D_PLANEMASK, "RADEON_RB3D_PLANEMASK" },
   { RADEON_SE_VPORT_XSCALE, "RADEON_SE_VPORT_XSCALE" },
   { RADEON_SE_VPORT_XOFFSET, "RADEON_SE_VPORT_XOFFSET" },
   { RADEON_SE_VPORT_YSCALE, "RADEON_SE_VPORT_YSCALE" },
   { RADEON_SE_VPORT_YOFFSET, "RADEON_SE_VPORT_YOFFSET" },
   { RADEON_SE_VPORT_ZSCALE, "RADEON_SE_VPORT_ZSCALE" },
   { RADEON_SE_VPORT_ZOFFSET, "RADEON_SE_VPORT_ZOFFSET" },
   { RADEON_RE_MISC, "RADEON_RE_MISC" },
   { RADEON_PP_TXFILTER_0, "RADEON_PP_TXFILTER_0" },
   { RADEON_PP_TXFILTER_1, "RADEON_PP_TXFILTER_1" },
   { RADEON_PP_TXFILTER_2, "RADEON_PP_TXFILTER_2" },
   { RADEON_PP_TXFORMAT_0, "RADEON_PP_TXFORMAT_0" },
   { RADEON_PP_TXFORMAT_1, "RADEON_PP_TXFORMAT_1" },
   { RADEON_PP_TXFORMAT_2, "RADEON_PP_TXFORMAT_2" },
   { RADEON_PP_TXOFFSET_0, "RADEON_PP_TXOFFSET_0" },
   { RADEON_PP_TXOFFSET_1, "RADEON_PP_TXOFFSET_1" },
   { RADEON_PP_TXOFFSET_2, "RADEON_PP_TXOFFSET_2" },
   { RADEON_PP_TXCBLEND_0, "RADEON_PP_TXCBLEND_0" },
   { RADEON_PP_TXCBLEND_1, "RADEON_PP_TXCBLEND_1" },
   { RADEON_PP_TXCBLEND_2, "RADEON_PP_TXCBLEND_2" },
   { RADEON_PP_TXABLEND_0, "RADEON_PP_TXABLEND_0" },
   { RADEON_PP_TXABLEND_1, "RADEON_PP_TXABLEND_1" },
   { RADEON_PP_TXABLEND_2, "RADEON_PP_TXABLEND_2" },
   { RADEON_PP_TFACTOR_0, "RADEON_PP_TFACTOR_0" },
   { RADEON_PP_TFACTOR_1, "RADEON_PP_TFACTOR_1" },
   { RADEON_PP_TFACTOR_2, "RADEON_PP_TFACTOR_2" },
   { RADEON_PP_BORDER_COLOR_0, "RADEON_PP_BORDER_COLOR_0" },
   { RADEON_PP_BORDER_COLOR_1, "RADEON_PP_BORDER_COLOR_1" },
   { RADEON_PP_BORDER_COLOR_2, "RADEON_PP_BORDER_COLOR_2" },
   { RADEON_SE_ZBIAS_FACTOR, "RADEON_SE_ZBIAS_FACTOR" },
   { RADEON_SE_ZBIAS_CONSTANT, "RADEON_SE_ZBIAS_CONSTANT" },
   { RADEON_SE_TCL_OUTPUT_VTX_FMT, "RADEON_SE_TCL_OUTPUT_VTXFMT" },
   { RADEON_SE_TCL_OUTPUT_VTX_SEL, "RADEON_SE_TCL_OUTPUT_VTXSEL" },
   { RADEON_SE_TCL_MATRIX_SELECT_0, "RADEON_SE_TCL_MATRIX_SELECT_0" },
   { RADEON_SE_TCL_MATRIX_SELECT_1, "RADEON_SE_TCL_MATRIX_SELECT_1" },
   { RADEON_SE_TCL_UCP_VERT_BLEND_CTL, "RADEON_SE_TCL_UCP_VERT_BLEND_CTL" },
   { RADEON_SE_TCL_TEXTURE_PROC_CTL, "RADEON_SE_TCL_TEXTURE_PROC_CTL" },
   { RADEON_SE_TCL_LIGHT_MODEL_CTL, "RADEON_SE_TCL_LIGHT_MODEL_CTL" },
   { RADEON_SE_TCL_PER_LIGHT_CTL_0, "RADEON_SE_TCL_PER_LIGHT_CTL_0" },
   { RADEON_SE_TCL_PER_LIGHT_CTL_1, "RADEON_SE_TCL_PER_LIGHT_CTL_1" },
   { RADEON_SE_TCL_PER_LIGHT_CTL_2, "RADEON_SE_TCL_PER_LIGHT_CTL_2" },
   { RADEON_SE_TCL_PER_LIGHT_CTL_3, "RADEON_SE_TCL_PER_LIGHT_CTL_3" },
   { RADEON_SE_TCL_MATERIAL_EMMISSIVE_RED, "RADEON_SE_TCL_EMMISSIVE_RED" },
   { RADEON_SE_TCL_MATERIAL_EMMISSIVE_GREEN, "RADEON_SE_TCL_EMMISSIVE_GREEN" },
   { RADEON_SE_TCL_MATERIAL_EMMISSIVE_BLUE, "RADEON_SE_TCL_EMMISSIVE_BLUE" },
   { RADEON_SE_TCL_MATERIAL_EMMISSIVE_ALPHA, "RADEON_SE_TCL_EMMISSIVE_ALPHA" },
   { RADEON_SE_TCL_MATERIAL_AMBIENT_RED, "RADEON_SE_TCL_AMBIENT_RED" },
   { RADEON_SE_TCL_MATERIAL_AMBIENT_GREEN, "RADEON_SE_TCL_AMBIENT_GREEN" },
   { RADEON_SE_TCL_MATERIAL_AMBIENT_BLUE, "RADEON_SE_TCL_AMBIENT_BLUE" },
   { RADEON_SE_TCL_MATERIAL_AMBIENT_ALPHA, "RADEON_SE_TCL_AMBIENT_ALPHA" },
   { RADEON_SE_TCL_MATERIAL_DIFFUSE_RED, "RADEON_SE_TCL_DIFFUSE_RED" },
   { RADEON_SE_TCL_MATERIAL_DIFFUSE_GREEN, "RADEON_SE_TCL_DIFFUSE_GREEN" },
   { RADEON_SE_TCL_MATERIAL_DIFFUSE_BLUE, "RADEON_SE_TCL_DIFFUSE_BLUE" },
   { RADEON_SE_TCL_MATERIAL_DIFFUSE_ALPHA, "RADEON_SE_TCL_DIFFUSE_ALPHA" },
   { RADEON_SE_TCL_MATERIAL_SPECULAR_RED, "RADEON_SE_TCL_SPECULAR_RED" },
   { RADEON_SE_TCL_MATERIAL_SPECULAR_GREEN, "RADEON_SE_TCL_SPECULAR_GREEN" },
   { RADEON_SE_TCL_MATERIAL_SPECULAR_BLUE, "RADEON_SE_TCL_SPECULAR_BLUE" },
   { RADEON_SE_TCL_MATERIAL_SPECULAR_ALPHA, "RADEON_SE_TCL_SPECULAR_ALPHA" },
   { RADEON_SE_TCL_SHININESS, "RADEON_SE_TCL_SHININESS" },
   { RADEON_SE_COORD_FMT, "RADEON_SE_COORD_FMT" },
   { RADEON_PP_TEX_SIZE_0, "RADEON_PP_TEX_SIZE_0" },
   { RADEON_PP_TEX_SIZE_1, "RADEON_PP_TEX_SIZE_1" },
   { RADEON_PP_TEX_SIZE_2, "RADEON_PP_TEX_SIZE_2" },
   { RADEON_PP_TEX_SIZE_0+4, "RADEON_PP_TEX_PITCH_0" },
   { RADEON_PP_TEX_SIZE_1+4, "RADEON_PP_TEX_PITCH_1" },
   { RADEON_PP_TEX_SIZE_2+4, "RADEON_PP_TEX_PITCH_2" },
   { RADEON_PP_CUBIC_FACES_0, "RADEON_PP_CUBIC_FACES_0" },
   { RADEON_PP_CUBIC_FACES_1, "RADEON_PP_CUBIC_FACES_1" },
   { RADEON_PP_CUBIC_FACES_2, "RADEON_PP_CUBIC_FACES_2" },
   { RADEON_PP_CUBIC_OFFSET_T0_0, "RADEON_PP_CUBIC_OFFSET_T0_0" },
   { RADEON_PP_CUBIC_OFFSET_T0_1, "RADEON_PP_CUBIC_OFFSET_T0_1" },
   { RADEON_PP_CUBIC_OFFSET_T0_2, "RADEON_PP_CUBIC_OFFSET_T0_2" },
   { RADEON_PP_CUBIC_OFFSET_T0_3, "RADEON_PP_CUBIC_OFFSET_T0_3" },
   { RADEON_PP_CUBIC_OFFSET_T0_4, "RADEON_PP_CUBIC_OFFSET_T0_4" },
   { RADEON_PP_CUBIC_OFFSET_T1_0, "RADEON_PP_CUBIC_OFFSET_T1_0" },
   { RADEON_PP_CUBIC_OFFSET_T1_1, "RADEON_PP_CUBIC_OFFSET_T1_1" },
   { RADEON_PP_CUBIC_OFFSET_T1_2, "RADEON_PP_CUBIC_OFFSET_T1_2" },
   { RADEON_PP_CUBIC_OFFSET_T1_3, "RADEON_PP_CUBIC_OFFSET_T1_3" },
   { RADEON_PP_CUBIC_OFFSET_T1_4, "RADEON_PP_CUBIC_OFFSET_T1_4" },
   { RADEON_PP_CUBIC_OFFSET_T2_0, "RADEON_PP_CUBIC_OFFSET_T2_0" },
   { RADEON_PP_CUBIC_OFFSET_T2_1, "RADEON_PP_CUBIC_OFFSET_T2_1" },
   { RADEON_PP_CUBIC_OFFSET_T2_2, "RADEON_PP_CUBIC_OFFSET_T2_2" },
   { RADEON_PP_CUBIC_OFFSET_T2_3, "RADEON_PP_CUBIC_OFFSET_T2_3" },
   { RADEON_PP_CUBIC_OFFSET_T2_4, "RADEON_PP_CUBIC_OFFSET_T2_4" },
};

static struct reg_names scalar_names[] = {
   { RADEON_SS_LIGHT_DCD_ADDR, "LIGHT_DCD" },
   { RADEON_SS_LIGHT_SPOT_EXPONENT_ADDR, "LIGHT_SPOT_EXPONENT" },
   { RADEON_SS_LIGHT_SPOT_CUTOFF_ADDR, "LIGHT_SPOT_CUTOFF" },
   { RADEON_SS_LIGHT_SPECULAR_THRESH_ADDR, "LIGHT_SPECULAR_THRESH" },
   { RADEON_SS_LIGHT_RANGE_CUTOFF_ADDR, "LIGHT_RANGE_CUTOFF" },
   { RADEON_SS_VERT_GUARD_CLIP_ADJ_ADDR, "VERT_GUARD_CLIP" },
   { RADEON_SS_VERT_GUARD_DISCARD_ADJ_ADDR, "VERT_GUARD_DISCARD" },
   { RADEON_SS_HORZ_GUARD_CLIP_ADJ_ADDR, "HORZ_GUARD_CLIP" },
   { RADEON_SS_HORZ_GUARD_DISCARD_ADJ_ADDR, "HORZ_GUARD_DISCARD" },
   { RADEON_SS_SHININESS, "SHININESS" },
   { 1000, "" },
};

/* Puff these out to make them look like normal (dword) registers.
 */
static struct reg_names vector_names[] = {
   { RADEON_VS_MATRIX_0_ADDR * 4, "MATRIX_0" },
   { RADEON_VS_MATRIX_1_ADDR * 4, "MATRIX_1" },
   { RADEON_VS_MATRIX_2_ADDR * 4, "MATRIX_2" },
   { RADEON_VS_MATRIX_3_ADDR * 4, "MATRIX_3" },
   { RADEON_VS_MATRIX_4_ADDR * 4, "MATRIX_4" },
   { RADEON_VS_MATRIX_5_ADDR * 4, "MATRIX_5" },
   { RADEON_VS_MATRIX_6_ADDR * 4, "MATRIX_6" },
   { RADEON_VS_MATRIX_7_ADDR * 4, "MATRIX_7" },
   { RADEON_VS_MATRIX_8_ADDR * 4, "MATRIX_8" },
   { RADEON_VS_MATRIX_9_ADDR * 4, "MATRIX_9" },
   { RADEON_VS_MATRIX_10_ADDR * 4, "MATRIX_10" },
   { RADEON_VS_MATRIX_11_ADDR * 4, "MATRIX_11" },
   { RADEON_VS_MATRIX_12_ADDR * 4, "MATRIX_12" },
   { RADEON_VS_MATRIX_13_ADDR * 4, "MATRIX_13" },
   { RADEON_VS_MATRIX_14_ADDR * 4, "MATRIX_14" },
   { RADEON_VS_MATRIX_15_ADDR * 4, "MATRIX_15" },
   { RADEON_VS_LIGHT_AMBIENT_ADDR * 4, "LIGHT_AMBIENT" },
   { RADEON_VS_LIGHT_DIFFUSE_ADDR * 4, "LIGHT_DIFFUSE" },
   { RADEON_VS_LIGHT_SPECULAR_ADDR * 4, "LIGHT_SPECULAR" },
   { RADEON_VS_LIGHT_DIRPOS_ADDR * 4, "LIGHT_DIRPOS" },
   { RADEON_VS_LIGHT_HWVSPOT_ADDR * 4, "LIGHT_HWVSPOT" },
   { RADEON_VS_LIGHT_ATTENUATION_ADDR * 4, "LIGHT_ATTENUATION" },
   { RADEON_VS_MATRIX_EYE2CLIP_ADDR * 4, "MATRIX_EYE2CLIP" },
   { RADEON_VS_UCP_ADDR * 4, "UCP" },
   { RADEON_VS_GLOBAL_AMBIENT_ADDR * 4, "GLOBAL_AMBIENT" },
   { RADEON_VS_FOG_PARAM_ADDR * 4, "FOG_PARAM" },
   { RADEON_VS_EYE_VECTOR_ADDR * 4, "EYE_VECTOR" },
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

   for (i = 0 ; i < Elements(regs)-1 ; i++) {
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

   if (start + stride * sz > 257) {
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


static int print_vertex_format( int vfmt )
{
   if (NORMAL) {
      fprintf(stderr, "   %s(%x): %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
	      "vertex format",
	      vfmt,
	      "xy,",
	      (vfmt & RADEON_CP_VC_FRMT_Z) ? "z," : "",
	      (vfmt & RADEON_CP_VC_FRMT_W0) ? "w0," : "",
	      (vfmt & RADEON_CP_VC_FRMT_FPCOLOR) ? "fpcolor," : "",
	      (vfmt & RADEON_CP_VC_FRMT_FPALPHA) ? "fpalpha," : "",
	      (vfmt & RADEON_CP_VC_FRMT_PKCOLOR) ? "pkcolor," : "",
	      (vfmt & RADEON_CP_VC_FRMT_FPSPEC) ? "fpspec," : "",
	      (vfmt & RADEON_CP_VC_FRMT_FPFOG) ? "fpfog," : "",
	      (vfmt & RADEON_CP_VC_FRMT_PKSPEC) ? "pkspec," : "",
	      (vfmt & RADEON_CP_VC_FRMT_ST0) ? "st0," : "",
	      (vfmt & RADEON_CP_VC_FRMT_ST1) ? "st1," : "",
	      (vfmt & RADEON_CP_VC_FRMT_Q1) ? "q1," : "",
	      (vfmt & RADEON_CP_VC_FRMT_ST2) ? "st2," : "",
	      (vfmt & RADEON_CP_VC_FRMT_Q2) ? "q2," : "",
	      (vfmt & RADEON_CP_VC_FRMT_ST3) ? "st3," : "",
	      (vfmt & RADEON_CP_VC_FRMT_Q3) ? "q3," : "",
	      (vfmt & RADEON_CP_VC_FRMT_Q0) ? "q0," : "",
	      (vfmt & RADEON_CP_VC_FRMT_N0) ? "n0," : "",
	      (vfmt & RADEON_CP_VC_FRMT_XY1) ? "xy1," : "",
	      (vfmt & RADEON_CP_VC_FRMT_Z1) ? "z1," : "",
	      (vfmt & RADEON_CP_VC_FRMT_W1) ? "w1," : "",
	      (vfmt & RADEON_CP_VC_FRMT_N1) ? "n1," : "");

   
/*       if (!find_or_add_value( &others[V_VTXFMT], vfmt )) */
/* 	 fprintf(stderr, " *** NEW VALUE"); */

      fprintf(stderr, "\n");
   }

   return 0;
}

static char *primname[0xf] = {
   "NONE",
   "POINTS",
   "LINES",
   "LINE_STRIP",
   "TRIANGLES",
   "TRIANGLE_FAN",
   "TRIANGLE_STRIP",
   "TRI_TYPE_2",
   "RECT_LIST",
   "3VRT_POINTS",
   "3VRT_LINES",
};

static int print_prim_and_flags( int prim )
{
   int numverts;
   
   if (NORMAL)
      fprintf(stderr, "   %s(%x): %s%s%s%s%s%s%s\n",
	      "prim flags",
	      prim,
	      ((prim & 0x30) == RADEON_CP_VC_CNTL_PRIM_WALK_IND) ? "IND," : "",
	      ((prim & 0x30) == RADEON_CP_VC_CNTL_PRIM_WALK_LIST) ? "LIST," : "",
	      ((prim & 0x30) == RADEON_CP_VC_CNTL_PRIM_WALK_RING) ? "RING," : "",
	      (prim & RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA) ? "RGBA," : "BGRA, ",
	      (prim & RADEON_CP_VC_CNTL_MAOS_ENABLE) ? "MAOS," : "",
	      (prim & RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE) ? "RADEON," : "",
	      (prim & RADEON_CP_VC_CNTL_TCL_ENABLE) ? "TCL," : "");

   if ((prim & 0xf) > RADEON_CP_VC_CNTL_PRIM_TYPE_3VRT_LINE_LIST) {
      fprintf(stderr, "   *** Bad primitive: %x\n", prim & 0xf);
      return -1;
   }

   numverts = prim>>16;
   
   if (NORMAL)
      fprintf(stderr, "   prim: %s numverts %d\n", primname[prim&0xf], numverts);

   switch (prim & 0xf) {
   case RADEON_CP_VC_CNTL_PRIM_TYPE_NONE:
   case RADEON_CP_VC_CNTL_PRIM_TYPE_POINT:
      if (numverts < 1) {
	 fprintf(stderr, "Bad nr verts for line %d\n", numverts);
	 return -1;
      }
      break;
   case RADEON_CP_VC_CNTL_PRIM_TYPE_LINE:
      if ((numverts & 1) || numverts == 0) {
	 fprintf(stderr, "Bad nr verts for line %d\n", numverts);
	 return -1;
      }
      break;
   case RADEON_CP_VC_CNTL_PRIM_TYPE_LINE_STRIP:
      if (numverts < 2) {
	 fprintf(stderr, "Bad nr verts for line_strip %d\n", numverts);
	 return -1;
      }
      break;
   case RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST:
   case RADEON_CP_VC_CNTL_PRIM_TYPE_3VRT_POINT_LIST:
   case RADEON_CP_VC_CNTL_PRIM_TYPE_3VRT_LINE_LIST:
   case RADEON_CP_VC_CNTL_PRIM_TYPE_RECT_LIST:
      if (numverts % 3 || numverts == 0) {
	 fprintf(stderr, "Bad nr verts for tri %d\n", numverts);
	 return -1;
      }
      break;
   case RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN:
   case RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_STRIP:
      if (numverts < 3) {
	 fprintf(stderr, "Bad nr verts for strip/fan %d\n", numverts);
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
   case RADEON_CP_PACKET3_NOP:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_NOP, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_NEXT_CHAR:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_NEXT_CHAR, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_PLY_NEXTSCAN:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_PLY_NEXTSCAN, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_SET_SCISSORS:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_SET_SCISSORS, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_3D_RNDR_GEN_INDX_PRIM, %d dwords\n",
	      cmdsz);
      break;
   case RADEON_CP_PACKET3_LOAD_MICROCODE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_LOAD_MICROCODE, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_WAIT_FOR_IDLE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_WAIT_FOR_IDLE, %d dwords\n", cmdsz);
      break;

   case RADEON_CP_PACKET3_3D_DRAW_VBUF:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_3D_DRAW_VBUF, %d dwords\n", cmdsz);
      print_vertex_format(cmd[1]);
      print_prim_and_flags(cmd[2]);
      break;

   case RADEON_CP_PACKET3_3D_DRAW_IMMD:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_3D_DRAW_IMMD, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_3D_DRAW_INDX: {
      int neltdwords;
      if (NORMAL)
	 fprintf(stderr, "PACKET3_3D_DRAW_INDX, %d dwords\n", cmdsz);
      print_vertex_format(cmd[1]);
      print_prim_and_flags(cmd[2]);
      neltdwords = cmd[2]>>16;
      neltdwords += neltdwords & 1;
      neltdwords /= 2;
      if (neltdwords + 3 != cmdsz)
	 fprintf(stderr, "Mismatch in DRAW_INDX, %d vs cmdsz %d\n",
		 neltdwords, cmdsz);
      break;
   }
   case RADEON_CP_PACKET3_LOAD_PALETTE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_LOAD_PALETTE, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_3D_LOAD_VBPNTR:
      if (NORMAL) {
	 fprintf(stderr, "PACKET3_3D_LOAD_VBPNTR, %d dwords\n", cmdsz);
	 fprintf(stderr, "   nr arrays: %d\n", cmd[1]);
      }

      if (cmd[1]/2 + cmd[1]%2 != cmdsz - 3) {
	 fprintf(stderr, "  ****** MISMATCH %d/%d *******\n",
		 cmd[1]/2 + cmd[1]%2 + 3, cmdsz);
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
   case RADEON_CP_PACKET3_CNTL_PAINT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_PAINT, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_BITBLT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_BITBLT, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_SMALLTEXT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_SMALLTEXT, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_HOSTDATA_BLT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_HOSTDATA_BLT, %d dwords\n", 
	      cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_POLYLINE:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_POLYLINE, %d dwords\n", cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_POLYSCANLINES:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_POLYSCANLINES, %d dwords\n", 
	      cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_PAINT_MULTI:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_PAINT_MULTI, %d dwords\n", 
	      cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_BITBLT_MULTI:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_BITBLT_MULTI, %d dwords\n", 
	      cmdsz);
      break;
   case RADEON_CP_PACKET3_CNTL_TRANS_BITBLT:
      if (NORMAL)
	 fprintf(stderr, "PACKET3_CNTL_TRANS_BITBLT, %d dwords\n", 
	      cmdsz);
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
   drm_clip_rect_t *boxes = cmdbuf->boxes;
   int i = 0;

   if (VERBOSE && total_changed) {
      dump_state();
      total_changed = 0;
   }
   else fprintf(stderr, "total_changed zero\n");

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


int radeonSanityCmdBuffer( radeonContextPtr rmesa,
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
   cmdbuf.boxes = boxes;
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

   return 0;
}
