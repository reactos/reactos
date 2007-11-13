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


#ifndef _I915_REG_H_
#define _I915_REG_H_


#include "intel_reg.h"

#define I915_SET_FIELD( var, mask, value ) (var &= ~(mask), var |= value)

#define CMD_3D (0x3<<29)

#define PRIM3D_INLINE		(CMD_3D | (0x1f<<24))
#define PRIM3D_TRILIST		(0x0<<18)
#define PRIM3D_TRISTRIP 	(0x1<<18)
#define PRIM3D_TRISTRIP_RVRSE	(0x2<<18)
#define PRIM3D_TRIFAN		(0x3<<18)
#define PRIM3D_POLY		(0x4<<18)
#define PRIM3D_LINELIST 	(0x5<<18)
#define PRIM3D_LINESTRIP	(0x6<<18)
#define PRIM3D_RECTLIST 	(0x7<<18)
#define PRIM3D_POINTLIST	(0x8<<18)
#define PRIM3D_DIB		(0x9<<18)
#define PRIM3D_CLEAR_RECT	(0xa<<18)
#define PRIM3D_ZONE_INIT	(0xd<<18)
#define PRIM3D_MASK		(0x1f<<18)

/* p137 */
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

/* 3DSTATE_BACKFACE_STENCIL_OPS, p138*/
#define _3DSTATE_BACKFACE_STENCIL_OPS    (CMD_3D | (0x8<<24))
#define BFO_ENABLE_STENCIL_REF          (1<<23)
#define BFO_STENCIL_REF_SHIFT           15
#define BFO_STENCIL_REF_MASK            (0xff<<15)
#define BFO_ENABLE_STENCIL_FUNCS        (1<<14)
#define BFO_STENCIL_TEST_SHIFT          11
#define BFO_STENCIL_TEST_MASK           (0x7<<11)
#define BFO_STENCIL_FAIL_SHIFT          8
#define BFO_STENCIL_FAIL_MASK           (0x7<<8)
#define BFO_STENCIL_PASS_Z_FAIL_SHIFT   5
#define BFO_STENCIL_PASS_Z_FAIL_MASK    (0x7<<5)
#define BFO_STENCIL_PASS_Z_PASS_SHIFT   2
#define BFO_STENCIL_PASS_Z_PASS_MASK    (0x7<<2)
#define BFO_ENABLE_STENCIL_TWO_SIDE     (1<<1)
#define BFO_STENCIL_TWO_SIDE            (1<<0)


/* 3DSTATE_BACKFACE_STENCIL_MASKS, p140 */
#define _3DSTATE_BACKFACE_STENCIL_MASKS    (CMD_3D | (0x9<<24))
#define BFM_ENABLE_STENCIL_TEST_MASK      (1<<17)
#define BFM_ENABLE_STENCIL_WRITE_MASK     (1<<16)
#define BFM_STENCIL_TEST_MASK_SHIFT       8
#define BFM_STENCIL_TEST_MASK_MASK        (0xff<<8)
#define BFM_STENCIL_WRITE_MASK_SHIFT      0
#define BFM_STENCIL_WRITE_MASK_MASK       (0xff<<0)



/* 3DSTATE_BIN_CONTROL p141 */

/* p143 */
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


/* 3DSTATE_CHROMA_KEY */

/* 3DSTATE_CLEAR_PARAMETERS, p150 */

/* 3DSTATE_CONSTANT_BLEND_COLOR, p153 */
#define _3DSTATE_CONST_BLEND_COLOR_CMD	(CMD_3D | (0x1d<<24) | (0x88<<16))



/* 3DSTATE_COORD_SET_BINDINGS, p154 */
#define _3DSTATE_COORD_SET_BINDINGS      (CMD_3D | (0x16<<24))
#define CSB_TCB(iunit, eunit)           ((eunit)<<(iunit*3))

/* p156 */
#define _3DSTATE_DFLT_DIFFUSE_CMD	(CMD_3D | (0x1d<<24) | (0x99<<16))

/* p157 */
#define _3DSTATE_DFLT_SPEC_CMD		(CMD_3D | (0x1d<<24) | (0x9a<<16))

/* p158 */
#define _3DSTATE_DFLT_Z_CMD		(CMD_3D | (0x1d<<24) | (0x98<<16))


/* 3DSTATE_DEPTH_OFFSET_SCALE, p159 */
#define _3DSTATE_DEPTH_OFFSET_SCALE       (CMD_3D | (0x1d<<24) | (0x97<<16))
/* scale in dword 1 */


/* 3DSTATE_DEPTH_SUBRECT_DISABLE, p160 */
#define _3DSTATE_DEPTH_SUBRECT_DISABLE    (CMD_3D | (0x1c<<24) | (0x11<<19) | 0x2)

/* p161 */
#define _3DSTATE_DST_BUF_VARS_CMD	(CMD_3D | (0x1d<<24) | (0x85<<16))
/* Dword 1 */
#define TEX_DEFAULT_COLOR_OGL           (0<<30)
#define TEX_DEFAULT_COLOR_D3D           (1<<30)
#define ZR_EARLY_DEPTH                  (1<<29)
#define LOD_PRECLAMP_OGL                (1<<28)
#define LOD_PRECLAMP_D3D                (0<<28)
#define DITHER_FULL_ALWAYS              (0<<26)
#define DITHER_FULL_ON_FB_BLEND         (1<<26)
#define DITHER_CLAMPED_ALWAYS           (2<<26)
#define LINEAR_GAMMA_BLEND_32BPP        (1<<25)
#define DEBUG_DISABLE_ENH_DITHER        (1<<24)
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
#define DEPTH_FRMT_16_FIXED		0
#define DEPTH_FRMT_16_FLOAT		(1<<2)
#define DEPTH_FRMT_24_FIXED_8_OTHER	(2<<2)
#define VERT_LINE_STRIDE_1		(1<<1)
#define VERT_LINE_STRIDE_0		(0<<1)
#define VERT_LINE_STRIDE_OFS_1		1
#define VERT_LINE_STRIDE_OFS_0		0

/* p166 */
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


/* 3DSTATE_FILTER_COEFFICIENTS_4X4, p170 */

/* 3DSTATE_FILTER_COEFFICIENTS_6X5, p172 */


/* _3DSTATE_FOG_COLOR, p173 */
#define _3DSTATE_FOG_COLOR_CMD		(CMD_3D|(0x15<<24))
#define FOG_COLOR_RED(x)		((x)<<16)
#define FOG_COLOR_GREEN(x)		((x)<<8)
#define FOG_COLOR_BLUE(x)		(x)

/* _3DSTATE_FOG_MODE, p174 */
#define _3DSTATE_FOG_MODE_CMD		(CMD_3D|(0x1d<<24)|(0x89<<16)|2)
/* Dword 1 */
#define FMC1_FOGFUNC_MODIFY_ENABLE	(1<<31)
#define FMC1_FOGFUNC_VERTEX		(0<<28)
#define FMC1_FOGFUNC_PIXEL_EXP		(1<<28)
#define FMC1_FOGFUNC_PIXEL_EXP2		(2<<28)
#define FMC1_FOGFUNC_PIXEL_LINEAR	(3<<28)
#define FMC1_FOGFUNC_MASK		(3<<28)
#define FMC1_FOGINDEX_MODIFY_ENABLE     (1<<27)
#define FMC1_FOGINDEX_Z		        (0<<25)
#define FMC1_FOGINDEX_W   		(1<<25)
#define FMC1_C1_C2_MODIFY_ENABLE	(1<<24)
#define FMC1_DENSITY_MODIFY_ENABLE	(1<<23)
#define FMC1_C1_ONE      	        (1<<13)
#define FMC1_C1_MASK		        (0xffff<<4)
/* Dword 2 */
#define FMC2_C2_ONE		        (1<<16)
/* Dword 3 */
#define FMC3_D_ONE      		(1<<16)



/* _3DSTATE_INDEPENDENT_ALPHA_BLEND, p177 */
#define _3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD	(CMD_3D|(0x0b<<24))
#define IAB_MODIFY_ENABLE	        (1<<23)
#define IAB_ENABLE       	        (1<<22)
#define IAB_MODIFY_FUNC         	(1<<21)
#define IAB_FUNC_SHIFT          	16
#define IAB_MODIFY_SRC_FACTOR   	(1<<11)
#define IAB_SRC_FACTOR_SHIFT		6
#define IAB_SRC_FACTOR_MASK		(BLENDFACT_MASK<<6)
#define IAB_MODIFY_DST_FACTOR	        (1<<5)
#define IAB_DST_FACTOR_SHIFT		0
#define IAB_DST_FACTOR_MASK		(BLENDFACT_MASK<<0)


#define BLENDFUNC_ADD			0x0
#define BLENDFUNC_SUBTRACT		0x1
#define BLENDFUNC_REVERSE_SUBTRACT	0x2
#define BLENDFUNC_MIN			0x3
#define BLENDFUNC_MAX			0x4
#define BLENDFUNC_MASK			0x7

/* 3DSTATE_LOAD_INDIRECT, p180 */

#define _3DSTATE_LOAD_INDIRECT	        (CMD_3D|(0x1d<<24)|(0x7<<16))
#define LI0_STATE_STATIC_INDIRECT       (0x01<<8)
#define LI0_STATE_DYNAMIC_INDIRECT      (0x02<<8)
#define LI0_STATE_SAMPLER               (0x04<<8)
#define LI0_STATE_MAP                   (0x08<<8)
#define LI0_STATE_PROGRAM               (0x10<<8)
#define LI0_STATE_CONSTANTS             (0x20<<8)

#define SIS0_BUFFER_ADDRESS(x)          ((x)&~0x3)
#define SIS0_FORCE_LOAD                 (1<<1)
#define SIS0_BUFFER_VALID               (1<<0)
#define SIS1_BUFFER_LENGTH(x)           ((x)&0xff)

#define DIS0_BUFFER_ADDRESS(x)          ((x)&~0x3)
#define DIS0_BUFFER_RESET               (1<<1)
#define DIS0_BUFFER_VALID               (1<<0)

#define SSB0_BUFFER_ADDRESS(x)          ((x)&~0x3)
#define SSB0_FORCE_LOAD                 (1<<1)
#define SSB0_BUFFER_VALID               (1<<0)
#define SSB1_BUFFER_LENGTH(x)           ((x)&0xff)

#define MSB0_BUFFER_ADDRESS(x)          ((x)&~0x3)
#define MSB0_FORCE_LOAD                 (1<<1)
#define MSB0_BUFFER_VALID               (1<<0)
#define MSB1_BUFFER_LENGTH(x)           ((x)&0xff)

#define PSP0_BUFFER_ADDRESS(x)          ((x)&~0x3)
#define PSP0_FORCE_LOAD                 (1<<1)
#define PSP0_BUFFER_VALID               (1<<0)
#define PSP1_BUFFER_LENGTH(x)           ((x)&0xff)

#define PSC0_BUFFER_ADDRESS(x)          ((x)&~0x3)
#define PSC0_FORCE_LOAD                 (1<<1)
#define PSC0_BUFFER_VALID               (1<<0)
#define PSC1_BUFFER_LENGTH(x)           ((x)&0xff)





/* _3DSTATE_RASTERIZATION_RULES */
#define _3DSTATE_RASTER_RULES_CMD	(CMD_3D|(0x07<<24))
#define ENABLE_POINT_RASTER_RULE	(1<<15)
#define OGL_POINT_RASTER_RULE		(1<<13)
#define ENABLE_TEXKILL_3D_4D            (1<<10)
#define TEXKILL_3D                      (0<<9)
#define TEXKILL_4D                      (1<<9)
#define ENABLE_LINE_STRIP_PROVOKE_VRTX	(1<<8)
#define ENABLE_TRI_FAN_PROVOKE_VRTX	(1<<5)
#define LINE_STRIP_PROVOKE_VRTX(x)	((x)<<6)
#define TRI_FAN_PROVOKE_VRTX(x) 	((x)<<3)

/* _3DSTATE_SCISSOR_ENABLE, p256 */
#define _3DSTATE_SCISSOR_ENABLE_CMD	(CMD_3D|(0x1c<<24)|(0x10<<19))
#define ENABLE_SCISSOR_RECT		((1<<1) | 1)
#define DISABLE_SCISSOR_RECT		(1<<1)

/* _3DSTATE_SCISSOR_RECTANGLE_0, p257 */
#define _3DSTATE_SCISSOR_RECT_0_CMD	(CMD_3D|(0x1d<<24)|(0x81<<16)|1)
/* Dword 1 */
#define SCISSOR_RECT_0_YMIN(x)		((x)<<16)
#define SCISSOR_RECT_0_XMIN(x)		(x)
/* Dword 2 */
#define SCISSOR_RECT_0_YMAX(x)		((x)<<16)
#define SCISSOR_RECT_0_XMAX(x)		(x)

/* p189 */
#define _3DSTATE_LOAD_STATE_IMMEDIATE_1   ((0x3<<29)|(0x1d<<24)|(0x04<<16))
#define I1_LOAD_S(n)                      (1<<(4+n))

#define S0_VB_OFFSET_MASK              0xffffffc
#define S0_AUTO_CACHE_INV_DISABLE      (1<<0)

#define S1_VERTEX_WIDTH_SHIFT          24
#define S1_VERTEX_WIDTH_MASK           (0x3f<<24)
#define S1_VERTEX_PITCH_SHIFT          16
#define S1_VERTEX_PITCH_MASK           (0x3f<<16)

#define TEXCOORDFMT_2D                 0x0
#define TEXCOORDFMT_3D                 0x1
#define TEXCOORDFMT_4D                 0x2
#define TEXCOORDFMT_1D                 0x3
#define TEXCOORDFMT_2D_16              0x4
#define TEXCOORDFMT_4D_16              0x5
#define TEXCOORDFMT_NOT_PRESENT        0xf
#define S2_TEXCOORD_FMT0_MASK            0xf
#define S2_TEXCOORD_FMT1_SHIFT           4
#define S2_TEXCOORD_FMT(unit, type)    ((type)<<(unit*4))
#define S2_TEXCOORD_NONE               (~0)

/* S3 not interesting */

#define S4_POINT_WIDTH_SHIFT           23
#define S4_POINT_WIDTH_MASK            (0x1ff<<23)
#define S4_LINE_WIDTH_SHIFT            19
#define S4_LINE_WIDTH_ONE              (0x2<<19)
#define S4_LINE_WIDTH_MASK             (0xf<<19)
#define S4_FLATSHADE_ALPHA             (1<<18)
#define S4_FLATSHADE_FOG               (1<<17)
#define S4_FLATSHADE_SPECULAR          (1<<16)
#define S4_FLATSHADE_COLOR             (1<<15)
#define S4_CULLMODE_BOTH	       (0<<13)
#define S4_CULLMODE_NONE	       (1<<13)
#define S4_CULLMODE_CW		       (2<<13)
#define S4_CULLMODE_CCW		       (3<<13)
#define S4_CULLMODE_MASK	       (3<<13)
#define S4_VFMT_POINT_WIDTH            (1<<12)
#define S4_VFMT_SPEC_FOG               (1<<11)
#define S4_VFMT_COLOR                  (1<<10)
#define S4_VFMT_DEPTH_OFFSET           (1<<9)
#define S4_VFMT_XYZ     	       (1<<6)
#define S4_VFMT_XYZW     	       (2<<6)
#define S4_VFMT_XY     		       (3<<6)
#define S4_VFMT_XYW     	       (4<<6)
#define S4_VFMT_XYZW_MASK              (7<<6)
#define S4_FORCE_DEFAULT_DIFFUSE       (1<<5)
#define S4_FORCE_DEFAULT_SPECULAR      (1<<4)
#define S4_LOCAL_DEPTH_OFFSET_ENABLE   (1<<3)
#define S4_VFMT_FOG_PARAM              (1<<2)
#define S4_SPRITE_POINT_ENABLE         (1<<1)
#define S4_LINE_ANTIALIAS_ENABLE       (1<<0)

#define S4_VFMT_MASK (S4_VFMT_POINT_WIDTH   | 	\
		      S4_VFMT_SPEC_FOG      |	\
		      S4_VFMT_COLOR         |	\
		      S4_VFMT_DEPTH_OFFSET  |	\
		      S4_VFMT_XYZW_MASK     |	\
		      S4_VFMT_FOG_PARAM)


#define S5_WRITEDISABLE_ALPHA          (1<<31)
#define S5_WRITEDISABLE_RED            (1<<30)
#define S5_WRITEDISABLE_GREEN          (1<<29)
#define S5_WRITEDISABLE_BLUE           (1<<28)
#define S5_WRITEDISABLE_MASK           (0xf<<28)
#define S5_FORCE_DEFAULT_POINT_SIZE    (1<<27)
#define S5_LAST_PIXEL_ENABLE           (1<<26)
#define S5_GLOBAL_DEPTH_OFFSET_ENABLE  (1<<25)
#define S5_FOG_ENABLE                  (1<<24)
#define S5_STENCIL_REF_SHIFT           16
#define S5_STENCIL_REF_MASK            (0xff<<16)
#define S5_STENCIL_TEST_FUNC_SHIFT     13
#define S5_STENCIL_TEST_FUNC_MASK      (0x7<<13)
#define S5_STENCIL_FAIL_SHIFT          10
#define S5_STENCIL_FAIL_MASK           (0x7<<10)
#define S5_STENCIL_PASS_Z_FAIL_SHIFT   7
#define S5_STENCIL_PASS_Z_FAIL_MASK    (0x7<<7)
#define S5_STENCIL_PASS_Z_PASS_SHIFT   4
#define S5_STENCIL_PASS_Z_PASS_MASK    (0x7<<4)
#define S5_STENCIL_WRITE_ENABLE        (1<<3)
#define S5_STENCIL_TEST_ENABLE         (1<<2)
#define S5_COLOR_DITHER_ENABLE         (1<<1)
#define S5_LOGICOP_ENABLE              (1<<0)


#define S6_ALPHA_TEST_ENABLE           (1<<31)
#define S6_ALPHA_TEST_FUNC_SHIFT       28
#define S6_ALPHA_TEST_FUNC_MASK        (0x7<<28)
#define S6_ALPHA_REF_SHIFT             20
#define S6_ALPHA_REF_MASK              (0xff<<20)
#define S6_DEPTH_TEST_ENABLE           (1<<19)
#define S6_DEPTH_TEST_FUNC_SHIFT       16
#define S6_DEPTH_TEST_FUNC_MASK        (0x7<<16)
#define S6_CBUF_BLEND_ENABLE           (1<<15)
#define S6_CBUF_BLEND_FUNC_SHIFT       12
#define S6_CBUF_BLEND_FUNC_MASK        (0x7<<12)
#define S6_CBUF_SRC_BLEND_FACT_SHIFT   8
#define S6_CBUF_SRC_BLEND_FACT_MASK    (0xf<<8)
#define S6_CBUF_DST_BLEND_FACT_SHIFT   4
#define S6_CBUF_DST_BLEND_FACT_MASK    (0xf<<4)
#define S6_DEPTH_WRITE_ENABLE          (1<<3)
#define S6_COLOR_WRITE_ENABLE          (1<<2)
#define S6_TRISTRIP_PV_SHIFT           0
#define S6_TRISTRIP_PV_MASK            (0x3<<0)

#define S7_DEPTH_OFFSET_CONST_MASK     ~0

/* 3DSTATE_MAP_DEINTERLACER_PARAMETERS */

/* 3DSTATE_MAP_PALETTE_LOAD_32, p206 */
#define _3DSTATE_MAP_PALETTE_LOAD_32    (CMD_3D|(0x1d<<24)|(0x8f<<16))
/* subsequent dwords up to length (max 16) are ARGB8888 color values */

/* _3DSTATE_MODES_4, p218 */
#define _3DSTATE_MODES_4_CMD		(CMD_3D|(0x0d<<24))
#define ENABLE_LOGIC_OP_FUNC		(1<<23)
#define LOGIC_OP_FUNC(x)		((x)<<18)
#define LOGICOP_MASK			(0xf<<18)
#define MODE4_ENABLE_STENCIL_TEST_MASK	((1<<17)|(0xff00))
#define ENABLE_STENCIL_TEST_MASK	(1<<17)
#define STENCIL_TEST_MASK(x)		(((x)&0xff)<<8)
#define MODE4_ENABLE_STENCIL_WRITE_MASK	((1<<16)|(0x00ff))
#define ENABLE_STENCIL_WRITE_MASK	(1<<16)
#define STENCIL_WRITE_MASK(x)		((x)&0xff)

/* _3DSTATE_MODES_5, p220 */
#define _3DSTATE_MODES_5_CMD		(CMD_3D|(0x0c<<24))
#define PIPELINE_FLUSH_RENDER_CACHE	(1<<18)
#define PIPELINE_FLUSH_TEXTURE_CACHE	(1<<16)


/* p221 */
#define _3DSTATE_PIXEL_SHADER_CONSTANTS  (CMD_3D|(0x1d<<24)|(0x6<<16))
#define PS1_REG(n)                      (1<<(n))
#define PS2_CONST_X(n)                  (n)
#define PS3_CONST_Y(n)                  (n)
#define PS4_CONST_Z(n)                  (n)
#define PS5_CONST_W(n)                  (n)

/* p222 */


#define I915_MAX_TEX_INDIRECT 4
#define I915_MAX_TEX_INSN     32
#define I915_MAX_ALU_INSN     64
#define I915_MAX_DECL_INSN    27
#define I915_MAX_TEMPORARY    16


/* Each instruction is 3 dwords long, though most don't require all
 * this space.  Maximum of 123 instructions.  Smaller maxes per insn
 * type.
 */
#define _3DSTATE_PIXEL_SHADER_PROGRAM    (CMD_3D|(0x1d<<24)|(0x5<<16))

#define REG_TYPE_R                 0    /* temporary regs, no need to
                                         * dcl, must be written before
                                         * read -- Preserved between
                                         * phases. 
                                         */
#define REG_TYPE_T                 1    /* Interpolated values, must be
                                         * dcl'ed before use.
                                         *
                                         * 0..7: texture coord,
                                         * 8: diffuse spec,
                                         * 9: specular color,
                                         * 10: fog parameter in w.
                                         */
#define REG_TYPE_CONST             2    /* Restriction: only one const
                                         * can be referenced per
                                         * instruction, though it may be
                                         * selected for multiple inputs.
                                         * Constants not initialized
                                         * default to zero.
                                         */
#define REG_TYPE_S                 3    /* sampler */
#define REG_TYPE_OC                4    /* output color (rgba) */
#define REG_TYPE_OD                5    /* output depth (w), xyz are
                                         * temporaries.  If not written,
                                         * interpolated depth is used?
                                         */
#define REG_TYPE_U                 6    /* unpreserved temporaries */
#define REG_TYPE_MASK              0x7
#define REG_NR_MASK                0xf


/* REG_TYPE_T:
 */
#define T_TEX0     0
#define T_TEX1     1
#define T_TEX2     2
#define T_TEX3     3
#define T_TEX4     4
#define T_TEX5     5
#define T_TEX6     6
#define T_TEX7     7
#define T_DIFFUSE  8
#define T_SPECULAR 9
#define T_FOG_W    10           /* interpolated fog is in W coord */

/* Arithmetic instructions */

/* .replicate_swizzle == selection and replication of a particular
 * scalar channel, ie., .xxxx, .yyyy, .zzzz or .wwww 
 */
#define A0_NOP    (0x0<<24)     /* no operation */
#define A0_ADD    (0x1<<24)     /* dst = src0 + src1 */
#define A0_MOV    (0x2<<24)     /* dst = src0 */
#define A0_MUL    (0x3<<24)     /* dst = src0 * src1 */
#define A0_MAD    (0x4<<24)     /* dst = src0 * src1 + src2 */
#define A0_DP2ADD (0x5<<24)     /* dst.xyzw = src0.xy dot src1.xy + src2.replicate_swizzle */
#define A0_DP3    (0x6<<24)     /* dst.xyzw = src0.xyz dot src1.xyz */
#define A0_DP4    (0x7<<24)     /* dst.xyzw = src0.xyzw dot src1.xyzw */
#define A0_FRC    (0x8<<24)     /* dst = src0 - floor(src0) */
#define A0_RCP    (0x9<<24)     /* dst.xyzw = 1/(src0.replicate_swizzle) */
#define A0_RSQ    (0xa<<24)     /* dst.xyzw = 1/(sqrt(abs(src0.replicate_swizzle))) */
#define A0_EXP    (0xb<<24)     /* dst.xyzw = exp2(src0.replicate_swizzle) */
#define A0_LOG    (0xc<<24)     /* dst.xyzw = log2(abs(src0.replicate_swizzle)) */
#define A0_CMP    (0xd<<24)     /* dst = (src0 >= 0.0) ? src1 : src2 */
#define A0_MIN    (0xe<<24)     /* dst = (src0 < src1) ? src0 : src1 */
#define A0_MAX    (0xf<<24)     /* dst = (src0 >= src1) ? src0 : src1 */
#define A0_FLR    (0x10<<24)    /* dst = floor(src0) */
#define A0_MOD    (0x11<<24)    /* dst = src0 fmod 1.0 */
#define A0_TRC    (0x12<<24)    /* dst = int(src0) */
#define A0_SGE    (0x13<<24)    /* dst = src0 >= src1 ? 1.0 : 0.0 */
#define A0_SLT    (0x14<<24)    /* dst = src0 < src1 ? 1.0 : 0.0 */
#define A0_DEST_SATURATE                 (1<<22)
#define A0_DEST_TYPE_SHIFT                19
/* Allow: R, OC, OD, U */
#define A0_DEST_NR_SHIFT                 14
/* Allow R: 0..15, OC,OD: 0..0, U: 0..2 */
#define A0_DEST_CHANNEL_X                (1<<10)
#define A0_DEST_CHANNEL_Y                (2<<10)
#define A0_DEST_CHANNEL_Z                (4<<10)
#define A0_DEST_CHANNEL_W                (8<<10)
#define A0_DEST_CHANNEL_ALL              (0xf<<10)
#define A0_DEST_CHANNEL_SHIFT            10
#define A0_SRC0_TYPE_SHIFT               7
#define A0_SRC0_NR_SHIFT                 2

#define A0_DEST_CHANNEL_XY              (A0_DEST_CHANNEL_X|A0_DEST_CHANNEL_Y)
#define A0_DEST_CHANNEL_XYZ             (A0_DEST_CHANNEL_XY|A0_DEST_CHANNEL_Z)


#define SRC_X        0
#define SRC_Y        1
#define SRC_Z        2
#define SRC_W        3
#define SRC_ZERO     4
#define SRC_ONE      5

#define A1_SRC0_CHANNEL_X_NEGATE         (1<<31)
#define A1_SRC0_CHANNEL_X_SHIFT          28
#define A1_SRC0_CHANNEL_Y_NEGATE         (1<<27)
#define A1_SRC0_CHANNEL_Y_SHIFT          24
#define A1_SRC0_CHANNEL_Z_NEGATE         (1<<23)
#define A1_SRC0_CHANNEL_Z_SHIFT          20
#define A1_SRC0_CHANNEL_W_NEGATE         (1<<19)
#define A1_SRC0_CHANNEL_W_SHIFT          16
#define A1_SRC1_TYPE_SHIFT               13
#define A1_SRC1_NR_SHIFT                 8
#define A1_SRC1_CHANNEL_X_NEGATE         (1<<7)
#define A1_SRC1_CHANNEL_X_SHIFT          4
#define A1_SRC1_CHANNEL_Y_NEGATE         (1<<3)
#define A1_SRC1_CHANNEL_Y_SHIFT          0

#define A2_SRC1_CHANNEL_Z_NEGATE         (1<<31)
#define A2_SRC1_CHANNEL_Z_SHIFT          28
#define A2_SRC1_CHANNEL_W_NEGATE         (1<<27)
#define A2_SRC1_CHANNEL_W_SHIFT          24
#define A2_SRC2_TYPE_SHIFT               21
#define A2_SRC2_NR_SHIFT                 16
#define A2_SRC2_CHANNEL_X_NEGATE         (1<<15)
#define A2_SRC2_CHANNEL_X_SHIFT          12
#define A2_SRC2_CHANNEL_Y_NEGATE         (1<<11)
#define A2_SRC2_CHANNEL_Y_SHIFT          8
#define A2_SRC2_CHANNEL_Z_NEGATE         (1<<7)
#define A2_SRC2_CHANNEL_Z_SHIFT          4
#define A2_SRC2_CHANNEL_W_NEGATE         (1<<3)
#define A2_SRC2_CHANNEL_W_SHIFT          0



/* Texture instructions */
#define T0_TEXLD     (0x15<<24) /* Sample texture using predeclared
                                 * sampler and address, and output
                                 * filtered texel data to destination
                                 * register */
#define T0_TEXLDP    (0x16<<24) /* Same as texld but performs a
                                 * perspective divide of the texture
                                 * coordinate .xyz values by .w before
                                 * sampling. */
#define T0_TEXLDB    (0x17<<24) /* Same as texld but biases the
                                 * computed LOD by w.  Only S4.6 two's
                                 * comp is used.  This implies that a
                                 * float to fixed conversion is
                                 * done. */
#define T0_TEXKILL   (0x18<<24) /* Does not perform a sampling
                                 * operation.  Simply kills the pixel
                                 * if any channel of the address
                                 * register is < 0.0. */
#define T0_DEST_TYPE_SHIFT                19
/* Allow: R, OC, OD, U */
/* Note: U (unpreserved) regs do not retain their values between
 * phases (cannot be used for feedback) 
 *
 * Note: oC and OD registers can only be used as the destination of a
 * texture instruction once per phase (this is an implementation
 * restriction). 
 */
#define T0_DEST_NR_SHIFT                 14
/* Allow R: 0..15, OC,OD: 0..0, U: 0..2 */
#define T0_SAMPLER_NR_SHIFT              0      /* This field ignored for TEXKILL */
#define T0_SAMPLER_NR_MASK               (0xf<<0)

#define T1_ADDRESS_REG_TYPE_SHIFT        24     /* Reg to use as texture coord */
/* Allow R, T, OC, OD -- R, OC, OD are 'dependent' reads, new program phase */
#define T1_ADDRESS_REG_NR_SHIFT          17
#define T2_MBZ                           0

/* Declaration instructions */
#define D0_DCL       (0x19<<24) /* Declare a t (interpolated attrib)
                                 * register or an s (sampler)
                                 * register. */
#define D0_SAMPLE_TYPE_SHIFT              22
#define D0_SAMPLE_TYPE_2D                 (0x0<<22)
#define D0_SAMPLE_TYPE_CUBE               (0x1<<22)
#define D0_SAMPLE_TYPE_VOLUME             (0x2<<22)
#define D0_SAMPLE_TYPE_MASK               (0x3<<22)

#define D0_TYPE_SHIFT                19
/* Allow: T, S */
#define D0_NR_SHIFT                  14
/* Allow T: 0..10, S: 0..15 */
#define D0_CHANNEL_X                (1<<10)
#define D0_CHANNEL_Y                (2<<10)
#define D0_CHANNEL_Z                (4<<10)
#define D0_CHANNEL_W                (8<<10)
#define D0_CHANNEL_ALL              (0xf<<10)
#define D0_CHANNEL_NONE             (0<<10)

#define D0_CHANNEL_XY               (D0_CHANNEL_X|D0_CHANNEL_Y)
#define D0_CHANNEL_XYZ              (D0_CHANNEL_XY|D0_CHANNEL_Z)

/* I915 Errata: Do not allow (xz), (xw), (xzw) combinations for diffuse
 * or specular declarations. 
 *
 * For T dcls, only allow: (x), (xy), (xyz), (w), (xyzw) 
 *
 * Must be zero for S (sampler) dcls
 */
#define D1_MBZ                          0
#define D2_MBZ                          0



/* p207 */
#define _3DSTATE_MAP_STATE               (CMD_3D|(0x1d<<24)|(0x0<<16))

#define MS1_MAPMASK_SHIFT               0
#define MS1_MAPMASK_MASK                (0x8fff<<0)

#define MS2_UNTRUSTED_SURFACE           (1<<31)
#define MS2_ADDRESS_MASK                0xfffffffc
#define MS2_VERTICAL_LINE_STRIDE        (1<<1)
#define MS2_VERTICAL_OFFSET             (1<<1)

#define MS3_HEIGHT_SHIFT              21
#define MS3_WIDTH_SHIFT               10
#define MS3_PALETTE_SELECT            (1<<9)
#define MS3_MAPSURF_FORMAT_SHIFT      7
#define MS3_MAPSURF_FORMAT_MASK       (0x7<<7)
#define    MAPSURF_8BIT		 	   (1<<7)
#define    MAPSURF_16BIT		   (2<<7)
#define    MAPSURF_32BIT		   (3<<7)
#define    MAPSURF_422			   (5<<7)
#define    MAPSURF_COMPRESSED		   (6<<7)
#define    MAPSURF_4BIT_INDEXED		   (7<<7)
#define MS3_MT_FORMAT_MASK         (0x7 << 3)
#define MS3_MT_FORMAT_SHIFT        3
#define    MT_4BIT_IDX_ARGB8888	           (7<<3)       /* SURFACE_4BIT_INDEXED */
#define    MT_8BIT_I8		           (0<<3)       /* SURFACE_8BIT */
#define    MT_8BIT_L8		           (1<<3)
#define    MT_8BIT_A8		           (4<<3)
#define    MT_8BIT_MONO8	           (5<<3)
#define    MT_16BIT_RGB565 		   (0<<3)       /* SURFACE_16BIT */
#define    MT_16BIT_ARGB1555		   (1<<3)
#define    MT_16BIT_ARGB4444		   (2<<3)
#define    MT_16BIT_AY88		   (3<<3)
#define    MT_16BIT_88DVDU	           (5<<3)
#define    MT_16BIT_BUMP_655LDVDU	   (6<<3)
#define    MT_16BIT_I16	                   (7<<3)
#define    MT_16BIT_L16	                   (8<<3)
#define    MT_16BIT_A16	                   (9<<3)
#define    MT_32BIT_ARGB8888		   (0<<3)       /* SURFACE_32BIT */
#define    MT_32BIT_ABGR8888		   (1<<3)
#define    MT_32BIT_XRGB8888		   (2<<3)
#define    MT_32BIT_XBGR8888		   (3<<3)
#define    MT_32BIT_QWVU8888		   (4<<3)
#define    MT_32BIT_AXVU8888		   (5<<3)
#define    MT_32BIT_LXVU8888	           (6<<3)
#define    MT_32BIT_XLVU8888	           (7<<3)
#define    MT_32BIT_ARGB2101010	           (8<<3)
#define    MT_32BIT_ABGR2101010	           (9<<3)
#define    MT_32BIT_AWVU2101010	           (0xA<<3)
#define    MT_32BIT_GR1616	           (0xB<<3)
#define    MT_32BIT_VU1616	           (0xC<<3)
#define    MT_32BIT_xI824	           (0xD<<3)
#define    MT_32BIT_xA824	           (0xE<<3)
#define    MT_32BIT_xL824	           (0xF<<3)
#define    MT_422_YCRCB_SWAPY	           (0<<3)       /* SURFACE_422 */
#define    MT_422_YCRCB_NORMAL	           (1<<3)
#define    MT_422_YCRCB_SWAPUV	           (2<<3)
#define    MT_422_YCRCB_SWAPUVY	           (3<<3)
#define    MT_COMPRESS_DXT1		   (0<<3)       /* SURFACE_COMPRESSED */
#define    MT_COMPRESS_DXT2_3	           (1<<3)
#define    MT_COMPRESS_DXT4_5	           (2<<3)
#define    MT_COMPRESS_FXT1		   (3<<3)
#define    MT_COMPRESS_DXT1_RGB		   (4<<3)
#define MS3_USE_FENCE_REGS              (1<<2)
#define MS3_TILED_SURFACE             (1<<1)
#define MS3_TILE_WALK                 (1<<0)

#define MS4_PITCH_SHIFT                 21
#define MS4_CUBE_FACE_ENA_NEGX          (1<<20)
#define MS4_CUBE_FACE_ENA_POSX          (1<<19)
#define MS4_CUBE_FACE_ENA_NEGY          (1<<18)
#define MS4_CUBE_FACE_ENA_POSY          (1<<17)
#define MS4_CUBE_FACE_ENA_NEGZ          (1<<16)
#define MS4_CUBE_FACE_ENA_POSZ          (1<<15)
#define MS4_CUBE_FACE_ENA_MASK          (0x3f<<15)
#define MS4_MAX_LOD_SHIFT		9
#define MS4_MAX_LOD_MASK		(0x3f<<9)
#define MS4_MIP_LAYOUT_LEGACY           (0<<8)
#define MS4_MIP_LAYOUT_BELOW_LPT        (0<<8)
#define MS4_MIP_LAYOUT_RIGHT_LPT        (1<<8)
#define MS4_VOLUME_DEPTH_SHIFT          0
#define MS4_VOLUME_DEPTH_MASK           (0xff<<0)

/* p244 */
#define _3DSTATE_SAMPLER_STATE         (CMD_3D|(0x1d<<24)|(0x1<<16))

#define SS1_MAPMASK_SHIFT               0
#define SS1_MAPMASK_MASK                (0x8fff<<0)

#define SS2_REVERSE_GAMMA_ENABLE        (1<<31)
#define SS2_PACKED_TO_PLANAR_ENABLE     (1<<30)
#define SS2_COLORSPACE_CONVERSION       (1<<29)
#define SS2_CHROMAKEY_SHIFT             27
#define SS2_BASE_MIP_LEVEL_SHIFT        22
#define SS2_BASE_MIP_LEVEL_MASK         (0x1f<<22)
#define SS2_MIP_FILTER_SHIFT            20
#define SS2_MIP_FILTER_MASK             (0x3<<20)
#define   MIPFILTER_NONE       	0
#define   MIPFILTER_NEAREST	1
#define   MIPFILTER_LINEAR	3
#define SS2_MAG_FILTER_SHIFT          17
#define SS2_MAG_FILTER_MASK           (0x7<<17)
#define   FILTER_NEAREST	0
#define   FILTER_LINEAR		1
#define   FILTER_ANISOTROPIC	2
#define   FILTER_4X4_1    	3
#define   FILTER_4X4_2    	4
#define   FILTER_4X4_FLAT 	5
#define   FILTER_6X5_MONO   	6       /* XXX - check */
#define SS2_MIN_FILTER_SHIFT          14
#define SS2_MIN_FILTER_MASK           (0x7<<14)
#define SS2_LOD_BIAS_SHIFT            5
#define SS2_LOD_BIAS_ONE              (0x10<<5)
#define SS2_LOD_BIAS_MASK             (0x1ff<<5)
/* Shadow requires:
 *  MT_X8{I,L,A}24 or MT_{I,L,A}16 texture format
 *  FILTER_4X4_x  MIN and MAG filters
 */
#define SS2_SHADOW_ENABLE             (1<<4)
#define SS2_MAX_ANISO_MASK            (1<<3)
#define SS2_MAX_ANISO_2               (0<<3)
#define SS2_MAX_ANISO_4               (1<<3)
#define SS2_SHADOW_FUNC_SHIFT         0
#define SS2_SHADOW_FUNC_MASK          (0x7<<0)
/* SS2_SHADOW_FUNC values: see COMPAREFUNC_* */

#define SS3_MIN_LOD_SHIFT            24
#define SS3_MIN_LOD_ONE              (0x10<<24)
#define SS3_MIN_LOD_MASK             (0xff<<24)
#define SS3_KILL_PIXEL_ENABLE        (1<<17)
#define SS3_TCX_ADDR_MODE_SHIFT      12
#define SS3_TCX_ADDR_MODE_MASK       (0x7<<12)
#define   TEXCOORDMODE_WRAP		0
#define   TEXCOORDMODE_MIRROR		1
#define   TEXCOORDMODE_CLAMP_EDGE	2
#define   TEXCOORDMODE_CUBE       	3
#define   TEXCOORDMODE_CLAMP_BORDER	4
#define   TEXCOORDMODE_MIRROR_ONCE      5
#define SS3_TCY_ADDR_MODE_SHIFT      9
#define SS3_TCY_ADDR_MODE_MASK       (0x7<<9)
#define SS3_TCZ_ADDR_MODE_SHIFT      6
#define SS3_TCZ_ADDR_MODE_MASK       (0x7<<6)
#define SS3_NORMALIZED_COORDS        (1<<5)
#define SS3_TEXTUREMAP_INDEX_SHIFT   1
#define SS3_TEXTUREMAP_INDEX_MASK    (0xf<<1)
#define SS3_DEINTERLACER_ENABLE      (1<<0)

#define SS4_BORDER_COLOR_MASK        (~0)

/* 3DSTATE_SPAN_STIPPLE, p258
 */
#define _3DSTATE_STIPPLE           ((0x3<<29)|(0x1d<<24)|(0x83<<16))
#define ST1_ENABLE               (1<<16)
#define ST1_MASK                 (0xffff)

#define _3DSTATE_DEFAULT_Z          ((0x3<<29)|(0x1d<<24)|(0x98<<16))
#define _3DSTATE_DEFAULT_DIFFUSE    ((0x3<<29)|(0x1d<<24)|(0x99<<16))
#define _3DSTATE_DEFAULT_SPECULAR   ((0x3<<29)|(0x1d<<24)|(0x9a<<16))


#define MI_FLUSH                   ((0<<29)|(4<<23))
#define FLUSH_MAP_CACHE            (1<<0)
#define INHIBIT_FLUSH_RENDER_CACHE (1<<2)


#endif
