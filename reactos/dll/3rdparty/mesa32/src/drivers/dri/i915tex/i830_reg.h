/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#ifndef _I830_REG_H_
#define _I830_REG_H_


#include "intel_reg.h"

#define I830_SET_FIELD( var, mask, value ) (var &= ~(mask), var |= value)

#define _3DSTATE_AA_CMD			(CMD_3D | (0x06<<24))
#define AA_LINE_ECAAR_WIDTH_ENABLE	(1<<16)
#define AA_LINE_ECAAR_WIDTH_0_5 	0
#define AA_LINE_ECAAR_WIDTH_1_0		(1<<14)
#define AA_LINE_ECAAR_WIDTH_2_0 	(2<<14)
#define AA_LINE_ECAAR_WIDTH_4_0 	(3<<14)
#define AA_LINE_REGION_WIDTH_ENABLE	(1<<8)
#define AA_LINE_REGION_WIDTH_0_5	0
#define AA_LINE_REGION_WIDTH_1_0	(1<<6)
#define AA_LINE_REGION_WIDTH_2_0	(2<<6)
#define AA_LINE_REGION_WIDTH_4_0	(3<<6)
#define AA_LINE_ENABLE			((1<<1) | 1)
#define AA_LINE_DISABLE			(1<<1)

#define _3DSTATE_BUF_INFO_CMD	(CMD_3D | (0x1d<<24) | (0x8e<<16) | 1)
/* Dword 1 */
#define BUF_3D_ID_COLOR_BACK	(0x3<<24)
#define BUF_3D_ID_DEPTH 	(0x7<<24)
#define BUF_3D_USE_FENCE	(1<<23)
#define BUF_3D_TILED_SURFACE	(1<<22)
#define BUF_3D_TILE_WALK_X	0
#define BUF_3D_TILE_WALK_Y	(1<<21)
#define BUF_3D_PITCH(x)         (((x)/4)<<2)
/* Dword 2 */
#define BUF_3D_ADDR(x)		((x) & ~0x3)


#define _3DSTATE_COLOR_FACTOR_CMD	(CMD_3D | (0x1d<<24) | (0x1<<16))

#define _3DSTATE_COLOR_FACTOR_N_CMD(stage)	(CMD_3D | (0x1d<<24) | \
					         ((0x90+(stage))<<16))

#define _3DSTATE_CONST_BLEND_COLOR_CMD	(CMD_3D | (0x1d<<24) | (0x88<<16))

#define _3DSTATE_DFLT_DIFFUSE_CMD	(CMD_3D | (0x1d<<24) | (0x99<<16))

#define _3DSTATE_DFLT_SPEC_CMD		(CMD_3D | (0x1d<<24) | (0x9a<<16))

#define _3DSTATE_DFLT_Z_CMD		(CMD_3D | (0x1d<<24) | (0x98<<16))


#define _3DSTATE_DST_BUF_VARS_CMD	(CMD_3D | (0x1d<<24) | (0x85<<16))
/* Dword 1 */
#define DSTORG_HORT_BIAS(x)		((x)<<20)
#define DSTORG_VERT_BIAS(x)		((x)<<16)
#define COLOR_4_2_2_CHNL_WRT_ALL	0
#define COLOR_4_2_2_CHNL_WRT_Y		(1<<12)
#define COLOR_4_2_2_CHNL_WRT_CR		(2<<12)
#define COLOR_4_2_2_CHNL_WRT_CB		(3<<12)
#define COLOR_4_2_2_CHNL_WRT_CRCB	(4<<12)
#define COLR_BUF_8BIT			0
#define COLR_BUF_RGB555 		(1<<8)
#define COLR_BUF_RGB565 		(2<<8)
#define COLR_BUF_ARGB8888		(3<<8)
#define DEPTH_IS_Z			0
#define DEPTH_IS_W			(1<<6)
#define DEPTH_FRMT_16_FIXED		0
#define DEPTH_FRMT_16_FLOAT		(1<<2)
#define DEPTH_FRMT_24_FIXED_8_OTHER	(2<<2)
#define DEPTH_FRMT_24_FLOAT_8_OTHER	(3<<2)
#define VERT_LINE_STRIDE_1		(1<<1)
#define VERT_LINE_STRIDE_0		0
#define VERT_LINE_STRIDE_OFS_1		1
#define VERT_LINE_STRIDE_OFS_0		0


#define _3DSTATE_DRAW_RECT_CMD		(CMD_3D|(0x1d<<24)|(0x80<<16)|3)
/* Dword 1 */
#define DRAW_RECT_DIS_DEPTH_OFS 	(1<<30)
#define DRAW_DITHER_OFS_X(x)		((x)<<26)
#define DRAW_DITHER_OFS_Y(x)		((x)<<24)
/* Dword 2 */
#define DRAW_YMIN(x)			((x)<<16)
#define DRAW_XMIN(x)			(x)
/* Dword 3 */
#define DRAW_YMAX(x)			((x)<<16)
#define DRAW_XMAX(x)			(x)
/* Dword 4 */
#define DRAW_YORG(x)			((x)<<16)
#define DRAW_XORG(x)			(x)


#define _3DSTATE_ENABLES_1_CMD		(CMD_3D|(0x3<<24))
#define ENABLE_LOGIC_OP_MASK		((1<<23)|(1<<22))
#define ENABLE_LOGIC_OP 		((1<<23)|(1<<22))
#define DISABLE_LOGIC_OP		(1<<23)
#define ENABLE_STENCIL_TEST		((1<<21)|(1<<20))
#define DISABLE_STENCIL_TEST		(1<<21)
#define ENABLE_DEPTH_BIAS		((1<<11)|(1<<10))
#define DISABLE_DEPTH_BIAS		(1<<11)
#define ENABLE_SPEC_ADD_MASK		((1<<9)|(1<<8))
#define ENABLE_SPEC_ADD 		((1<<9)|(1<<8))
#define DISABLE_SPEC_ADD		(1<<9)
#define ENABLE_DIS_FOG_MASK		((1<<7)|(1<<6))
#define ENABLE_FOG			((1<<7)|(1<<6))
#define DISABLE_FOG			(1<<7)
#define ENABLE_DIS_ALPHA_TEST_MASK	((1<<5)|(1<<4))
#define ENABLE_ALPHA_TEST		((1<<5)|(1<<4))
#define DISABLE_ALPHA_TEST		(1<<5)
#define ENABLE_DIS_CBLEND_MASK		((1<<3)|(1<<2))
#define ENABLE_COLOR_BLEND		((1<<3)|(1<<2))
#define DISABLE_COLOR_BLEND		(1<<3)
#define ENABLE_DIS_DEPTH_TEST_MASK	((1<<1)|1)
#define ENABLE_DEPTH_TEST		((1<<1)|1)
#define DISABLE_DEPTH_TEST		(1<<1)

/* _3DSTATE_ENABLES_2, p138 */
#define _3DSTATE_ENABLES_2_CMD		(CMD_3D|(0x4<<24))
#define ENABLE_STENCIL_WRITE		((1<<21)|(1<<20))
#define DISABLE_STENCIL_WRITE		(1<<21)
#define ENABLE_TEX_CACHE		((1<<17)|(1<<16))
#define DISABLE_TEX_CACHE		(1<<17)
#define ENABLE_DITHER			((1<<9)|(1<<8))
#define DISABLE_DITHER			(1<<9)
#define ENABLE_COLOR_MASK		(1<<10)
#define WRITEMASK_ALPHA			(1<<7)
#define WRITEMASK_ALPHA_SHIFT		7
#define WRITEMASK_RED			(1<<6)
#define WRITEMASK_RED_SHIFT		6
#define WRITEMASK_GREEN 		(1<<5)
#define WRITEMASK_GREEN_SHIFT		5
#define WRITEMASK_BLUE			(1<<4)
#define WRITEMASK_BLUE_SHIFT		4
#define WRITEMASK_MASK			((1<<4)|(1<<5)|(1<<6)|(1<<7))
#define ENABLE_COLOR_WRITE		((1<<3)|(1<<2))
#define DISABLE_COLOR_WRITE		(1<<3)
#define ENABLE_DIS_DEPTH_WRITE_MASK	0x3
#define ENABLE_DEPTH_WRITE		((1<<1)|1)
#define DISABLE_DEPTH_WRITE		(1<<1)

/* _3DSTATE_FOG_COLOR, p139 */
#define _3DSTATE_FOG_COLOR_CMD		(CMD_3D|(0x15<<24))
#define FOG_COLOR_RED(x)		((x)<<16)
#define FOG_COLOR_GREEN(x)		((x)<<8)
#define FOG_COLOR_BLUE(x)		(x)

/* _3DSTATE_FOG_MODE, p140 */
#define _3DSTATE_FOG_MODE_CMD		(CMD_3D|(0x1d<<24)|(0x89<<16)|2)
/* Dword 1 */
#define FOGFUNC_ENABLE			(1<<31)
#define FOGFUNC_VERTEX			0
#define FOGFUNC_PIXEL_EXP		(1<<28)
#define FOGFUNC_PIXEL_EXP2		(2<<28)
#define FOGFUNC_PIXEL_LINEAR		(3<<28)
#define FOGSRC_INDEX_Z			(1<<27)
#define FOGSRC_INDEX_W			((1<<27)|(1<<25))
#define FOG_LINEAR_CONST		(1<<24)
#define FOG_CONST_1(x)			((x)<<4)
#define ENABLE_FOG_DENSITY		(1<<23)
/* Dword 2 */
#define FOG_CONST_2(x)			(x)
/* Dword 3 */
#define FOG_DENSITY(x)			(x)

/* _3DSTATE_INDEPENDENT_ALPHA_BLEND, p142 */
#define _3DSTATE_INDPT_ALPHA_BLEND_CMD	(CMD_3D|(0x0b<<24))
#define ENABLE_INDPT_ALPHA_BLEND	((1<<23)|(1<<22))
#define DISABLE_INDPT_ALPHA_BLEND	(1<<23)
#define ALPHA_BLENDFUNC_MASK		0x3f0000
#define ENABLE_ALPHA_BLENDFUNC		(1<<21)
#define ABLENDFUNC_ADD			0
#define ABLENDFUNC_SUB			(1<<16)
#define ABLENDFUNC_RVSE_SUB		(2<<16)
#define ABLENDFUNC_MIN			(3<<16)
#define ABLENDFUNC_MAX			(4<<16)
#define SRC_DST_ABLEND_MASK		0xfff
#define ENABLE_SRC_ABLEND_FACTOR	(1<<11)
#define SRC_ABLEND_FACT(x)		((x)<<6)
#define ENABLE_DST_ABLEND_FACTOR	(1<<5)
#define DST_ABLEND_FACT(x)		(x)


/* _3DSTATE_MAP_BLEND_ARG, p152 */
#define _3DSTATE_MAP_BLEND_ARG_CMD(stage)	(CMD_3D|(0x0e<<24)|((stage)<<20))

#define TEXPIPE_COLOR			0
#define TEXPIPE_ALPHA			(1<<18)
#define TEXPIPE_KILL			(2<<18)
#define TEXBLEND_ARG0			0
#define TEXBLEND_ARG1			(1<<15)
#define TEXBLEND_ARG2			(2<<15)
#define TEXBLEND_ARG3			(3<<15)
#define TEXBLENDARG_MODIFY_PARMS	(1<<6)
#define TEXBLENDARG_REPLICATE_ALPHA 	(1<<5)
#define TEXBLENDARG_INV_ARG 		(1<<4)
#define TEXBLENDARG_ONE 		0
#define TEXBLENDARG_FACTOR		0x01
#define TEXBLENDARG_ACCUM		0x02
#define TEXBLENDARG_DIFFUSE		0x03
#define TEXBLENDARG_SPEC		0x04
#define TEXBLENDARG_CURRENT		0x05
#define TEXBLENDARG_TEXEL0		0x06
#define TEXBLENDARG_TEXEL1		0x07
#define TEXBLENDARG_TEXEL2		0x08
#define TEXBLENDARG_TEXEL3		0x09
#define TEXBLENDARG_FACTOR_N		0x0e

/* _3DSTATE_MAP_BLEND_OP, p155 */
#define _3DSTATE_MAP_BLEND_OP_CMD(stage)	(CMD_3D|(0x0d<<24)|((stage)<<20))
#if 0
#   define TEXPIPE_COLOR		0
#   define TEXPIPE_ALPHA		(1<<18)
#   define TEXPIPE_KILL			(2<<18)
#endif
#define ENABLE_TEXOUTPUT_WRT_SEL	(1<<17)
#define TEXOP_OUTPUT_CURRENT		0
#define TEXOP_OUTPUT_ACCUM		(1<<15)
#define ENABLE_TEX_CNTRL_STAGE		((1<<12)|(1<<11))
#define DISABLE_TEX_CNTRL_STAGE		(1<<12)
#define TEXOP_SCALE_SHIFT		9
#define TEXOP_SCALE_1X			(0 << TEXOP_SCALE_SHIFT)
#define TEXOP_SCALE_2X			(1 << TEXOP_SCALE_SHIFT)
#define TEXOP_SCALE_4X			(2 << TEXOP_SCALE_SHIFT)
#define TEXOP_MODIFY_PARMS		(1<<8)
#define TEXOP_LAST_STAGE		(1<<7)
#define TEXBLENDOP_KILLPIXEL		0x02
#define TEXBLENDOP_ARG1 		0x01
#define TEXBLENDOP_ARG2 		0x02
#define TEXBLENDOP_MODULATE		0x03
#define TEXBLENDOP_ADD			0x06
#define TEXBLENDOP_ADDSIGNED		0x07
#define TEXBLENDOP_BLEND		0x08
#define TEXBLENDOP_BLEND_AND_ADD	0x09
#define TEXBLENDOP_SUBTRACT		0x0a
#define TEXBLENDOP_DOT3 		0x0b
#define TEXBLENDOP_DOT4 		0x0c
#define TEXBLENDOP_MODULATE_AND_ADD	0x0d
#define TEXBLENDOP_MODULATE_2X_AND_ADD	0x0e
#define TEXBLENDOP_MODULATE_4X_AND_ADD	0x0f

/* _3DSTATE_MAP_BUMP_TABLE, p160 TODO */
/* _3DSTATE_MAP_COLOR_CHROMA_KEY, p161 TODO */

#define _3DSTATE_MAP_COORD_TRANSFORM	((3<<29)|(0x1d<<24)|(0x8c<<16))
#define DISABLE_TEX_TRANSFORM		(1<<28)
#define TEXTURE_SET(x)			(x<<29)

#define _3DSTATE_VERTEX_TRANSFORM	((3<<29)|(0x1d<<24)|(0x8b<<16))
#define DISABLE_VIEWPORT_TRANSFORM	(1<<31)
#define DISABLE_PERSPECTIVE_DIVIDE	(1<<29)


/* _3DSTATE_MAP_COORD_SET_BINDINGS, p162 */
#define _3DSTATE_MAP_COORD_SETBIND_CMD	(CMD_3D|(0x1d<<24)|(0x02<<16))
#define TEXBIND_MASK3			((1<<15)|(1<<14)|(1<<13)|(1<<12))
#define TEXBIND_MASK2			((1<<11)|(1<<10)|(1<<9)|(1<<8))
#define TEXBIND_MASK1			((1<<7)|(1<<6)|(1<<5)|(1<<4))
#define TEXBIND_MASK0			((1<<3)|(1<<2)|(1<<1)|1)

#define TEXBIND_SET3(x) 		((x)<<12)
#define TEXBIND_SET2(x) 		((x)<<8)
#define TEXBIND_SET1(x) 		((x)<<4)
#define TEXBIND_SET0(x) 		(x)

#define TEXCOORDSRC_KEEP		0
#define TEXCOORDSRC_DEFAULT		0x01
#define TEXCOORDSRC_VTXSET_0		0x08
#define TEXCOORDSRC_VTXSET_1		0x09
#define TEXCOORDSRC_VTXSET_2		0x0a
#define TEXCOORDSRC_VTXSET_3		0x0b
#define TEXCOORDSRC_VTXSET_4		0x0c
#define TEXCOORDSRC_VTXSET_5		0x0d
#define TEXCOORDSRC_VTXSET_6		0x0e
#define TEXCOORDSRC_VTXSET_7		0x0f

#define MAP_UNIT(unit)			((unit)<<16)
#define MAP_UNIT_MASK			(0x7<<16)

/* _3DSTATE_MAP_COORD_SETS, p164 */
#define _3DSTATE_MAP_COORD_SET_CMD	(CMD_3D|(0x1c<<24)|(0x01<<19))
#define ENABLE_TEXCOORD_PARAMS		(1<<15)
#define TEXCOORDS_ARE_NORMAL		(1<<14)
#define TEXCOORDS_ARE_IN_TEXELUNITS	0
#define TEXCOORDTYPE_CARTESIAN		0
#define TEXCOORDTYPE_HOMOGENEOUS	(1<<11)
#define TEXCOORDTYPE_VECTOR		(2<<11)
#define TEXCOORDTYPE_MASK	        (0x7<<11)
#define ENABLE_ADDR_V_CNTL		(1<<7)
#define ENABLE_ADDR_U_CNTL		(1<<3)
#define TEXCOORD_ADDR_V_MODE(x) 	((x)<<4)
#define TEXCOORD_ADDR_U_MODE(x) 	(x)
#define TEXCOORDMODE_WRAP		0
#define TEXCOORDMODE_MIRROR		1
#define TEXCOORDMODE_CLAMP		2
#define TEXCOORDMODE_WRAP_SHORTEST	3
#define TEXCOORDMODE_CLAMP_BORDER	4
#define TEXCOORD_ADDR_V_MASK		0x70
#define TEXCOORD_ADDR_U_MASK		0x7

/* _3DSTATE_MAP_CUBE, p168 TODO */
#define _3DSTATE_MAP_CUBE		(CMD_3D|(0x1c<<24)|(0x0a<<19))
#define CUBE_NEGX_ENABLE                (1<<5)
#define CUBE_POSX_ENABLE                (1<<4)
#define CUBE_NEGY_ENABLE                (1<<3)
#define CUBE_POSY_ENABLE                (1<<2)
#define CUBE_NEGZ_ENABLE                (1<<1)
#define CUBE_POSZ_ENABLE                (1<<0)


/* _3DSTATE_MODES_1, p190 */
#define _3DSTATE_MODES_1_CMD		(CMD_3D|(0x08<<24))
#define BLENDFUNC_MASK			0x3f0000
#define ENABLE_COLR_BLND_FUNC		(1<<21)
#define BLENDFUNC_ADD			0
#define BLENDFUNC_SUB			(1<<16)
#define BLENDFUNC_RVRSE_SUB		(2<<16)
#define BLENDFUNC_MIN			(3<<16)
#define BLENDFUNC_MAX			(4<<16)
#define SRC_DST_BLND_MASK		0xfff
#define ENABLE_SRC_BLND_FACTOR		(1<<11)
#define ENABLE_DST_BLND_FACTOR		(1<<5)
#define SRC_BLND_FACT(x)		((x)<<6)
#define DST_BLND_FACT(x)		(x)


/* _3DSTATE_MODES_2, p192 */
#define _3DSTATE_MODES_2_CMD		(CMD_3D|(0x0f<<24))
#define ENABLE_GLOBAL_DEPTH_BIAS	(1<<22)
#define GLOBAL_DEPTH_BIAS(x)		((x)<<14)
#define ENABLE_ALPHA_TEST_FUNC		(1<<13)
#define ENABLE_ALPHA_REF_VALUE		(1<<8)
#define ALPHA_TEST_FUNC(x)		((x)<<9)
#define ALPHA_REF_VALUE(x)		(x)

#define ALPHA_TEST_REF_MASK		0x3fff

/* _3DSTATE_MODES_3, p193 */
#define _3DSTATE_MODES_3_CMD		(CMD_3D|(0x02<<24))
#define DEPTH_TEST_FUNC_MASK		0x1f0000
#define ENABLE_DEPTH_TEST_FUNC		(1<<20)
/* Uses COMPAREFUNC */
#define DEPTH_TEST_FUNC(x)		((x)<<16)
#define ENABLE_ALPHA_SHADE_MODE 	(1<<11)
#define ENABLE_FOG_SHADE_MODE		(1<<9)
#define ENABLE_SPEC_SHADE_MODE		(1<<7)
#define ENABLE_COLOR_SHADE_MODE 	(1<<5)
#define ALPHA_SHADE_MODE(x)		((x)<<10)
#define FOG_SHADE_MODE(x)		((x)<<8)
#define SPEC_SHADE_MODE(x)		((x)<<6)
#define COLOR_SHADE_MODE(x)		((x)<<4)
#define CULLMODE_MASK			0xf
#define ENABLE_CULL_MODE		(1<<3)
#define CULLMODE_BOTH			0
#define CULLMODE_NONE			1
#define CULLMODE_CW			2
#define CULLMODE_CCW			3

#define SHADE_MODE_LINEAR		0
#define SHADE_MODE_FLAT 		0x1

/* _3DSTATE_MODES_4, p195 */
#define _3DSTATE_MODES_4_CMD		(CMD_3D|(0x16<<24))
#define ENABLE_LOGIC_OP_FUNC		(1<<23)
#define LOGIC_OP_FUNC(x)		((x)<<18)
#define LOGICOP_MASK			((1<<18)|(1<<19)|(1<<20)|(1<<21))
#define LOGICOP_CLEAR			0
#define LOGICOP_NOR			0x1
#define LOGICOP_AND_INV 		0x2
#define LOGICOP_COPY_INV		0x3
#define LOGICOP_AND_RVRSE		0x4
#define LOGICOP_INV			0x5
#define LOGICOP_XOR			0x6
#define LOGICOP_NAND			0x7
#define LOGICOP_AND			0x8
#define LOGICOP_EQUIV			0x9
#define LOGICOP_NOOP			0xa
#define LOGICOP_OR_INV			0xb
#define LOGICOP_COPY			0xc
#define LOGICOP_OR_RVRSE		0xd
#define LOGICOP_OR			0xe
#define LOGICOP_SET			0xf
#define MODE4_ENABLE_STENCIL_TEST_MASK	((1<<17)|(0xff00))
#define ENABLE_STENCIL_TEST_MASK	(1<<17)
#define STENCIL_TEST_MASK(x)		(((x)&0xff)<<8)
#define MODE4_ENABLE_STENCIL_WRITE_MASK	((1<<16)|(0x00ff))
#define ENABLE_STENCIL_WRITE_MASK	(1<<16)
#define STENCIL_WRITE_MASK(x)		((x)&0xff)

/* _3DSTATE_MODES_5, p196 */
#define _3DSTATE_MODES_5_CMD		(CMD_3D|(0x0c<<24))
#define ENABLE_SPRITE_POINT_TEX 	(1<<23)
#define SPRITE_POINT_TEX_ON		(1<<22)
#define SPRITE_POINT_TEX_OFF		0
#define FLUSH_RENDER_CACHE		(1<<18)
#define FLUSH_TEXTURE_CACHE		(1<<16)
#define FIXED_LINE_WIDTH_MASK		0xfc00
#define ENABLE_FIXED_LINE_WIDTH 	(1<<15)
#define FIXED_LINE_WIDTH(x)		((x)<<10)
#define FIXED_POINT_WIDTH_MASK		0x3ff
#define ENABLE_FIXED_POINT_WIDTH	(1<<9)
#define FIXED_POINT_WIDTH(x)		(x)

/* _3DSTATE_RASTERIZATION_RULES, p198 */
#define _3DSTATE_RASTER_RULES_CMD	(CMD_3D|(0x07<<24))
#define ENABLE_POINT_RASTER_RULE	(1<<15)
#define OGL_POINT_RASTER_RULE		(1<<13)
#define ENABLE_LINE_STRIP_PROVOKE_VRTX	(1<<8)
#define ENABLE_TRI_FAN_PROVOKE_VRTX	(1<<5)
#define ENABLE_TRI_STRIP_PROVOKE_VRTX	(1<<2)
#define LINE_STRIP_PROVOKE_VRTX(x)	((x)<<6)
#define TRI_FAN_PROVOKE_VRTX(x) 	((x)<<3)
#define TRI_STRIP_PROVOKE_VRTX(x)	(x)

/* _3DSTATE_SCISSOR_ENABLE, p200 */
#define _3DSTATE_SCISSOR_ENABLE_CMD	(CMD_3D|(0x1c<<24)|(0x10<<19))
#define ENABLE_SCISSOR_RECT		((1<<1) | 1)
#define DISABLE_SCISSOR_RECT		(1<<1)

/* _3DSTATE_SCISSOR_RECTANGLE_0, p201 */
#define _3DSTATE_SCISSOR_RECT_0_CMD	(CMD_3D|(0x1d<<24)|(0x81<<16)|1)
/* Dword 1 */
#define SCISSOR_RECT_0_YMIN(x)		((x)<<16)
#define SCISSOR_RECT_0_XMIN(x)		(x)
/* Dword 2 */
#define SCISSOR_RECT_0_YMAX(x)		((x)<<16)
#define SCISSOR_RECT_0_XMAX(x)		(x)

/* _3DSTATE_STENCIL_TEST, p202 */
#define _3DSTATE_STENCIL_TEST_CMD	(CMD_3D|(0x09<<24))
#define ENABLE_STENCIL_PARMS		(1<<23)
#define STENCIL_OPS_MASK		(0xffc000)
#define STENCIL_FAIL_OP(x)		((x)<<20)
#define STENCIL_PASS_DEPTH_FAIL_OP(x)	((x)<<17)
#define STENCIL_PASS_DEPTH_PASS_OP(x)	((x)<<14)

#define ENABLE_STENCIL_TEST_FUNC_MASK	((1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9))
#define ENABLE_STENCIL_TEST_FUNC	(1<<13)
/* Uses COMPAREFUNC */
#define STENCIL_TEST_FUNC(x)		((x)<<9)
#define STENCIL_REF_VALUE_MASK		((1<<8)|0xff)
#define ENABLE_STENCIL_REF_VALUE	(1<<8)
#define STENCIL_REF_VALUE(x)		(x)

/* _3DSTATE_VERTEX_FORMAT, p204 */
#define _3DSTATE_VFT0_CMD	(CMD_3D|(0x05<<24))
#define VFT0_POINT_WIDTH	(1<<12)
#define VFT0_TEX_COUNT_MASK    	(7<<8)
#define VFT0_TEX_COUNT_SHIFT    8
#define VFT0_TEX_COUNT(x) 	((x)<<8)
#define VFT0_SPEC		(1<<7)
#define VFT0_DIFFUSE		(1<<6)
#define VFT0_DEPTH_OFFSET  	(1<<5)
#define VFT0_XYZ		(1<<1)
#define VFT0_XYZW		(2<<1)
#define VFT0_XY			(3<<1)
#define VFT0_XYW		(4<<1)
#define VFT0_XYZW_MASK          (7<<1)

/* _3DSTATE_VERTEX_FORMAT_2, p206 */
#define _3DSTATE_VFT1_CMD	(CMD_3D|(0x0a<<24))
#define VFT1_TEX7_FMT(x)	((x)<<14)
#define VFT1_TEX6_FMT(x)	((x)<<12)
#define VFT1_TEX5_FMT(x)	((x)<<10)
#define VFT1_TEX4_FMT(x)	((x)<<8)
#define VFT1_TEX3_FMT(x)	((x)<<6)
#define VFT1_TEX2_FMT(x)	((x)<<4)
#define VFT1_TEX1_FMT(x)	((x)<<2)
#define VFT1_TEX0_FMT(x)	(x)
#define VFT1_TEX0_MASK          3
#define VFT1_TEX1_SHIFT         2
#define TEXCOORDFMT_2D		0
#define TEXCOORDFMT_3D		1
#define TEXCOORDFMT_4D		2
#define TEXCOORDFMT_1D		3

/*New stuff picked up along the way */

#define MLC_LOD_BIAS_MASK ((1<<7)-1)


/* _3DSTATE_VERTEX_TRANSFORM, p207 */
#define _3DSTATE_VERTEX_TRANS_CMD	(CMD_3D|(0x1d<<24)|(0x8b<<16)|0)
#define _3DSTATE_VERTEX_TRANS_MTX_CMD	(CMD_3D|(0x1d<<24)|(0x8b<<16)|6)
/* Dword 1 */
#define ENABLE_VIEWPORT_TRANSFORM	((1<<31)|(1<<30))
#define DISABLE_VIEWPORT_TRANSFORM	(1<<31)
#define ENABLE_PERSP_DIVIDE		((1<<29)|(1<<28))
#define DISABLE_PERSP_DIVIDE		(1<<29)
#define VRTX_TRANS_LOAD_MATRICES	0x7421
#define VRTX_TRANS_NO_LOAD_MATRICES	0x0000
/* Dword 2 -> 7  are matrix elements */

/* _3DSTATE_W_STATE, p209 */
#define _3DSTATE_W_STATE_CMD		(CMD_3D|(0x1d<<24)|(0x8d<<16)|1)
/* Dword 1 */
#define MAGIC_W_STATE_DWORD1		0x00000008
/* Dword 2 */
#define WFAR_VALUE(x)			(x)


/* Stipple command, carried over from the i810, apparently:
 */
#define _3DSTATE_STIPPLE           ((0x3<<29)|(0x1d<<24)|(0x83<<16))
#define ST1_ENABLE               (1<<16)
#define ST1_MASK                 (0xffff)



#define _3DSTATE_LOAD_STATE_IMMEDIATE_2      ((0x3<<29)|(0x1d<<24)|(0x03<<16))
#define LOAD_TEXTURE_MAP0                   (1<<11)
#define LOAD_GLOBAL_COLOR_FACTOR            (1<<6)

#define TM0S0_ADDRESS_MASK              0xfffffffc
#define TM0S0_USE_FENCE                 (1<<1)

#define TM0S1_HEIGHT_SHIFT              21
#define TM0S1_WIDTH_SHIFT               10
#define TM0S1_PALETTE_SELECT            (1<<9)
#define TM0S1_MAPSURF_FORMAT_MASK       (0x7 << 6)
#define TM0S1_MAPSURF_FORMAT_SHIFT      6
#define    MAPSURF_8BIT_INDEXED		   (0<<6)
#define    MAPSURF_8BIT		 	   (1<<6)
#define    MAPSURF_16BIT		   (2<<6)
#define    MAPSURF_32BIT		   (3<<6)
#define    MAPSURF_411			   (4<<6)
#define    MAPSURF_422			   (5<<6)
#define    MAPSURF_COMPRESSED		   (6<<6)
#define    MAPSURF_4BIT_INDEXED		   (7<<6)
#define TM0S1_MT_FORMAT_MASK         (0x7 << 3)
#define TM0S1_MT_FORMAT_SHIFT        3
#define    MT_4BIT_IDX_ARGB8888	           (7<<3)       /* SURFACE_4BIT_INDEXED */
#define    MT_8BIT_IDX_RGB565	           (0<<3)       /* SURFACE_8BIT_INDEXED */
#define    MT_8BIT_IDX_ARGB1555	           (1<<3)
#define    MT_8BIT_IDX_ARGB4444	           (2<<3)
#define    MT_8BIT_IDX_AY88		   (3<<3)
#define    MT_8BIT_IDX_ABGR8888	           (4<<3)
#define    MT_8BIT_IDX_BUMP_88DVDU 	   (5<<3)
#define    MT_8BIT_IDX_BUMP_655LDVDU	   (6<<3)
#define    MT_8BIT_IDX_ARGB8888	           (7<<3)
#define    MT_8BIT_I8		           (0<<3)       /* SURFACE_8BIT */
#define    MT_8BIT_L8		           (1<<3)
#define    MT_16BIT_RGB565 		   (0<<3)       /* SURFACE_16BIT */
#define    MT_16BIT_ARGB1555		   (1<<3)
#define    MT_16BIT_ARGB4444		   (2<<3)
#define    MT_16BIT_AY88		   (3<<3)
#define    MT_16BIT_DIB_ARGB1555_8888      (4<<3)
#define    MT_16BIT_BUMP_88DVDU	           (5<<3)
#define    MT_16BIT_BUMP_655LDVDU	   (6<<3)
#define    MT_16BIT_DIB_RGB565_8888	   (7<<3)
#define    MT_32BIT_ARGB8888		   (0<<3)       /* SURFACE_32BIT */
#define    MT_32BIT_ABGR8888		   (1<<3)
#define    MT_32BIT_XRGB8888		   (2<<3)       /* XXX: Guess from i915_reg.h */
#define    MT_32BIT_BUMP_XLDVDU_8888	   (6<<3)
#define    MT_32BIT_DIB_8888		   (7<<3)
#define    MT_411_YUV411		   (0<<3)       /* SURFACE_411 */
#define    MT_422_YCRCB_SWAPY	           (0<<3)       /* SURFACE_422 */
#define    MT_422_YCRCB_NORMAL	           (1<<3)
#define    MT_422_YCRCB_SWAPUV	           (2<<3)
#define    MT_422_YCRCB_SWAPUVY	           (3<<3)
#define    MT_COMPRESS_DXT1		   (0<<3)       /* SURFACE_COMPRESSED */
#define    MT_COMPRESS_DXT2_3	           (1<<3)
#define    MT_COMPRESS_DXT4_5	           (2<<3)
#define    MT_COMPRESS_FXT1		   (3<<3)
#define TM0S1_COLORSPACE_CONVERSION     (1 << 2)
#define TM0S1_TILED_SURFACE             (1 << 1)
#define TM0S1_TILE_WALK                 (1 << 0)

#define TM0S2_PITCH_SHIFT               21
#define TM0S2_CUBE_FACE_ENA_SHIFT       15
#define TM0S2_CUBE_FACE_ENA_MASK        (1<<15)
#define TM0S2_MAP_FORMAT                (1<<14)
#define TM0S2_VERTICAL_LINE_STRIDE      (1<<13)
#define TM0S2_VERITCAL_LINE_STRIDE_OFF  (1<<12)
#define TM0S2_OUTPUT_CHAN_SHIFT         10
#define TM0S2_OUTPUT_CHAN_MASK          (3<<10)

#define TM0S3_MIP_FILTER_MASK           (0x3<<30)
#define TM0S3_MIP_FILTER_SHIFT          30
#define MIPFILTER_NONE		0
#define MIPFILTER_NEAREST	1
#define MIPFILTER_LINEAR	3
#define TM0S3_MAG_FILTER_MASK           (0x3<<28)
#define TM0S3_MAG_FILTER_SHIFT          28
#define TM0S3_MIN_FILTER_MASK           (0x3<<26)
#define TM0S3_MIN_FILTER_SHIFT          26
#define FILTER_NEAREST		0
#define FILTER_LINEAR		1
#define FILTER_ANISOTROPIC	2

#define TM0S3_LOD_BIAS_SHIFT		17
#define TM0S3_LOD_BIAS_MASK		(0x1ff<<17)
#define TM0S3_MAX_MIP_SHIFT		9
#define TM0S3_MAX_MIP_MASK		(0xff<<9)
#define TM0S3_MIN_MIP_SHIFT		3
#define TM0S3_MIN_MIP_MASK		(0x3f<<3)
#define TM0S3_KILL_PIXEL		(1<<2)
#define TM0S3_KEYED_FILTER		(1<<1)
#define TM0S3_CHROMA_KEY		(1<<0)


/* _3DSTATE_MAP_TEXEL_STREAM, p188 */
#define _3DSTATE_MAP_TEX_STREAM_CMD	(CMD_3D|(0x1c<<24)|(0x05<<19))
#define DISABLE_TEX_STREAM_BUMP 	(1<<12)
#define ENABLE_TEX_STREAM_BUMP		((1<<12)|(1<<11))
#define TEX_MODIFY_UNIT_0		0
#define TEX_MODIFY_UNIT_1		(1<<8)
#define ENABLE_TEX_STREAM_COORD_SET	(1<<7)
#define TEX_STREAM_COORD_SET(x) 	((x)<<4)
#define ENABLE_TEX_STREAM_MAP_IDX	(1<<3)
#define TEX_STREAM_MAP_IDX(x)		(x)


#define MI_FLUSH           ((0<<29)|(4<<23))
#define FLUSH_MAP_CACHE    (1<<0)

#endif
