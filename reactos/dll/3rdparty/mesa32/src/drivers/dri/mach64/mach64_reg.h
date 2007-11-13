/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#ifndef __MACH64_REG_H__
#define __MACH64_REG_H__

/*
 * Not sure how this compares with the G200, but the Rage Pro has two
 * banks of registers, with bank 0 at (aperture base + memmap offset - 1KB)
 * and bank 1 at (aperture base + memmap offset - 2KB).  But, to send them
 * via DMA, we need to encode them as memory map select rather than physical
 * offsets.
 */
#define DWMREG0		0x0400
#define DWMREG0_END	0x07ff
#define DWMREG1		0x0000
#define DWMREG1_END	0x03ff

#define ISREG0(r)	( ( (r) >= DWMREG0 ) && ( (r) <= DWMREG0_END ) )
#define ADRINDEX0(r)	( ((r) - DWMREG0) >> 2 )
#define ADRINDEX1(r)	( ( ((r) - DWMREG1) >> 2 ) | 0x0100 )
#define ADRINDEX(r)	( ISREG0(r) ? ADRINDEX0(r) : ADRINDEX1(r) )

#define MMREG0		0x0000
#define MMREG0_END	0x00ff

#define ISMMREG0(r)	( ( (r) >= MMREG0 ) && ( (r) <= MMREG0_END ) )
#define MMSELECT0(r)	( ((r)<<2) + DWMREG0 )
#define MMSELECT1(r)	( ( (((r) & 0xff)<<2) + DWMREG1 ) )
#define MMSELECT(r)	( ISMMREG0(r) ? MMSELECT0(r) : MMSELECT1(r) )

/* FIXME: If register reads are necessary, we should account for endianess here */
#define MACH64_BASE(reg)	((CARD32)(mmesa->mach64Screen->mmio.map))
#define MACH64_ADDR(reg)	(MACH64_BASE(reg) + reg)

#define MACH64_DEREF(reg)	*(__volatile__ CARD32 *)MACH64_ADDR(reg)
#define MACH64_READ(reg)	MACH64_DEREF(reg)


/* ================================================================
 * Registers
 */

#define MACH64_ALPHA_TST_CNTL			0x0550
#	define MACH64_ALPHA_TEST_EN			(1 << 0)
#	define MACH64_ALPHA_TEST_MASK			(7 << 4)
#	define MACH64_ALPHA_TEST_NEVER			(0 << 4)
#	define MACH64_ALPHA_TEST_LESS			(1 << 4)
#	define MACH64_ALPHA_TEST_LEQUAL			(2 << 4)
#	define MACH64_ALPHA_TEST_EQUAL			(3 << 4)
#	define MACH64_ALPHA_TEST_GEQUAL			(4 << 4)
#	define MACH64_ALPHA_TEST_GREATER		(5 << 4)
#	define MACH64_ALPHA_TEST_NOTEQUAL		(6 << 4)
#	define MACH64_ALPHA_TEST_ALWAYS			(7 << 4)
#	define MACH64_ALPHA_MOD_MSB			(1 << 7)
#	define MACH64_ALPHA_DST_MASK			(7 << 8)
#	define MACH64_ALPHA_DST_ZERO			(0 << 8)
#	define MACH64_ALPHA_DST_ONE			(1 << 8)
#	define MACH64_ALPHA_DST_SRCALPHA		(4 << 8)
#	define MACH64_ALPHA_DST_INVSRCALPHA		(5 << 8)
#	define MACH64_ALPHA_DST_DSTALPHA		(6 << 8)
#	define MACH64_ALPHA_DST_INVDSTALPHA		(7 << 8)
#	define MACH64_ALPHA_TST_SRC_TEXEL		(0 << 12)
#	define MACH64_ALPHA_TST_SRC_SRCALPHA		(1 << 12)
#	define MACH64_REF_ALPHA_MASK			(0xff << 16)
#	define MACH64_REF_ALPHA_SHIFT			16
#	define MACH64_COMPOSITE_SHADOW			(1 << 30)
#	define MACH64_SPECULAR_LIGHT_EN			(1 << 31)

#define MACH64_BUS_CNTL				0x04a0
#	define MACH64_BUS_MSTR_RESET			(1 << 1)
#	define MACH64_BUS_FLUSH_BUF			(1 << 2)
#	define MACH64_BUS_MASTER_DIS			(1 << 6)
#	define MACH64_BUS_EXT_REG_EN			(1 << 27)

#define MACH64_COMPOSITE_SHADOW_ID		0x0798

#define MACH64_CLR_CMP_CLR			0x0700
#define MACH64_CLR_CMP_CNTL			0x0708
#define MACH64_CLR_CMP_MASK			0x0704

#define MACH64_DP_BKGD_CLR			0x06c0
#define MACH64_DP_FOG_CLR			0x06c4
#define MACH64_DP_FGRD_BKGD_CLR			0x06e0
#define MACH64_DP_FRGD_CLR			0x06c4
#define MACH64_DP_FGRD_CLR_MIX			0x06dc

#define MACH64_DP_MIX				0x06d4
#	define BKGD_MIX_NOT_D				(0 << 0)
#	define BKGD_MIX_ZERO				(1 << 0)
#	define BKGD_MIX_ONE				(2 << 0)
#	define MACH64_BKGD_MIX_D			(3 << 0)
#	define BKGD_MIX_NOT_S				(4 << 0)
#	define BKGD_MIX_D_XOR_S				(5 << 0)
#	define BKGD_MIX_NOT_D_XOR_S			(6 << 0)
#	define MACH64_BKGD_MIX_S			(7 << 0)
#	define BKGD_MIX_NOT_D_OR_NOT_S			(8 << 0)
#	define BKGD_MIX_D_OR_NOT_S			(9 << 0)
#	define BKGD_MIX_NOT_D_OR_S			(10 << 0)
#	define BKGD_MIX_D_OR_S				(11 << 0)
#	define BKGD_MIX_D_AND_S				(12 << 0)
#	define BKGD_MIX_NOT_D_AND_S			(13 << 0)
#	define BKGD_MIX_D_AND_NOT_S			(14 << 0)
#	define BKGD_MIX_NOT_D_AND_NOT_S			(15 << 0)
#	define BKGD_MIX_D_PLUS_S_DIV2			(23 << 0)
#	define FRGD_MIX_NOT_D				(0 << 16)
#	define FRGD_MIX_ZERO				(1 << 16)
#	define FRGD_MIX_ONE				(2 << 16)
#	define FRGD_MIX_D				(3 << 16)
#	define FRGD_MIX_NOT_S				(4 << 16)
#	define FRGD_MIX_D_XOR_S				(5 << 16)
#	define FRGD_MIX_NOT_D_XOR_S			(6 << 16)
#	define MACH64_FRGD_MIX_S			(7 << 16)
#	define FRGD_MIX_NOT_D_OR_NOT_S			(8 << 16)
#	define FRGD_MIX_D_OR_NOT_S			(9 << 16)
#	define FRGD_MIX_NOT_D_OR_S			(10 << 16)
#	define FRGD_MIX_D_OR_S				(11 << 16)
#	define FRGD_MIX_D_AND_S				(12 << 16)
#	define FRGD_MIX_NOT_D_AND_S			(13 << 16)
#	define FRGD_MIX_D_AND_NOT_S			(14 << 16)
#	define FRGD_MIX_NOT_D_AND_NOT_S			(15 << 16)
#	define FRGD_MIX_D_PLUS_S_DIV2			(23 << 16)

#define MACH64_DP_PIX_WIDTH			0x06d0
#	define MACH64_COMPOSITE_PIX_WIDTH_MASK		(0xf << 4)
#	define MACH64_HOST_TRIPLE_ENABLE		(1 << 13)
#	define MACH64_BYTE_ORDER_MSB_TO_LSB		(0 << 24)
#	define MACH64_BYTE_ORDER_LSB_TO_MSB		(1 << 24)
#	define MACH64_SCALE_PIX_WIDTH_MASK		(0xf << 28)

#define MACH64_DP_SRC				0x06d8
#	define MACH64_BKGD_SRC_BKGD_CLR			(0 << 0)
#	define MACH64_BKGD_SRC_FRGD_CLR			(1 << 0)
#	define MACH64_BKGD_SRC_HOST			(2 << 0)
#	define MACH64_BKGD_SRC_BLIT			(3 << 0)
#	define MACH64_BKGD_SRC_PATTERN			(4 << 0)
#	define MACH64_BKGD_SRC_3D			(5 << 0)
#	define MACH64_FRGD_SRC_BKGD_CLR			(0 << 8)
#	define MACH64_FRGD_SRC_FRGD_CLR			(1 << 8)
#	define MACH64_FRGD_SRC_HOST			(2 << 8)
#	define MACH64_FRGD_SRC_BLIT			(3 << 8)
#	define MACH64_FRGD_SRC_PATTERN			(4 << 8)
#	define MACH64_FRGD_SRC_3D			(5 << 8)
#	define MACH64_MONO_SRC_ONE			(0 << 16)
#	define MACH64_MONO_SRC_PATTERN			(1 << 16)
#	define MACH64_MONO_SRC_HOST			(2 << 16)
#	define MACH64_MONO_SRC_BLIT			(3 << 16)

#define MACH64_DP_WRITE_MASK			0x06c8

#define MACH64_DST_CNTL				0x0530
#	define MACH64_DST_X_RIGHT_TO_LEFT		(0 << 0)
#	define MACH64_DST_X_LEFT_TO_RIGHT		(1 << 0)
#	define MACH64_DST_Y_BOTTOM_TO_TOP		(0 << 1)
#	define MACH64_DST_Y_TOP_TO_BOTTOM		(1 << 1)
#	define MACH64_DST_X_MAJOR			(0 << 2)
#	define MACH64_DST_Y_MAJOR			(1 << 2)
#	define MACH64_DST_X_TILE			(1 << 3)
#	define MACH64_DST_Y_TILE			(1 << 4)
#	define MACH64_DST_LAST_PEL			(1 << 5)
#	define MACH64_DST_POLYGON_ENABLE		(1 << 6)
#	define MACH64_DST_24_ROTATION_ENABLE		(1 << 7)

#define MACH64_DST_HEIGHT_WIDTH			0x0518
#define MACH64_DST_OFF_PITCH			0x0500
#define MACH64_DST_WIDTH_HEIGHT			0x06ec
#define MACH64_DST_X_Y				0x06e8
#define MACH64_DST_Y_X				0x050c

#define MACH64_FIFO_STAT			0x0710
#	define MACH64_FIFO_SLOT_MASK			0x0000ffff
#	define MACH64_FIFO_ERR				(1 << 31)

#define MACH64_GEN_TEST_CNTL			0x04d0
#define MACH64_GUI_CMDFIFO_DEBUG		0x0170
#define MACH64_GUI_CMDFIFO_DATA			0x0174
#define MACH64_GUI_CNTL				0x0178
#define MACH64_GUI_STAT				0x0738
#	define MACH64_GUI_ACTIVE			(1 << 0)
#define MACH64_GUI_TRAJ_CNTL			0x0730

#define MACH64_HOST_CNTL			0x0640
#define MACH64_HOST_DATA0			0x0600
#define MACH64_HW_DEBUG				0x047c

#define MACH64_ONE_OVER_AREA			0x029c
#define MACH64_ONE_OVER_AREA_UC			0x0300

#define MACH64_PAT_REG0				0x0680
#define MACH64_PAT_REG1				0x0684

#define MACH64_SC_LEFT_RIGHT			0x06a8
#define MACH64_SC_TOP_BOTTOM			0x06b4
#define MACH64_SCALE_3D_CNTL			0x05fc
#	define MACH64_SCALE_PIX_EXPAND_ZERO_EXTEND	(0 << 0)
#	define MACH64_SCALE_PIX_EXPAND_DYNAMIC_RANGE	(1 << 0)
#	define MACH64_SCALE_DITHER_ERROR_DIFFUSE	(0 << 1)
#	define MACH64_SCALE_DITHER_2D_TABLE		(1 << 1)
#	define MACH64_DITHER_EN				(1 << 2)
#	define MACH64_DITHER_INIT_CURRENT		(O << 3)
#	define MACH64_DITHER_INIT_RESET			(1 << 3)
#	define MACH64_ROUND_EN				(1 << 4)
#	define MACH64_TEX_CACHE_DIS			(1 << 5)
#	define MACH64_SCALE_3D_FCN_MASK			(3 << 6)
#	define MACH64_SCALE_3D_FCN_NOP			(0 << 6)
#	define MACH64_SCALE_3D_FCN_SCALE		(1 << 6)
#	define MACH64_SCALE_3D_FCN_TEXTURE		(2 << 6)
#	define MACH64_SCALE_3D_FCN_SHADE		(3 << 6)
#	define MACH64_TEXTURE_DISABLE			(1 << 6)
#	define MACH64_EDGE_ANTI_ALIAS			(1 << 8)
#	define MACH64_TEX_CACHE_SPLIT			(1 << 9)
#	define MACH64_APPLE_YUV_MODE			(1 << 10)
#	define MACH64_ALPHA_FOG_EN_MASK			(3 << 11)
#	define MACH64_ALPHA_FOG_DIS			(0 << 11)
#	define MACH64_ALPHA_FOG_EN_ALPHA		(1 << 11)
#	define MACH64_ALPHA_FOG_EN_FOG			(2 << 11)
#	define MACH64_ALPHA_BLEND_SAT			(1 << 13)
#	define MACH64_RED_DITHER_MAX			(1 << 14)
#	define MACH64_SIGNED_DST_CLAMP			(1 << 15)
#	define MACH64_ALPHA_BLEND_SRC_MASK		(7 << 16)
#	define MACH64_ALPHA_BLEND_SRC_ZERO		(0 << 16)
#	define MACH64_ALPHA_BLEND_SRC_ONE		(1 << 16)
#	define MACH64_ALPHA_BLEND_SRC_DSTCOLOR		(2 << 16)
#	define MACH64_ALPHA_BLEND_SRC_INVDSTCOLOR	(3 << 16)
#	define MACH64_ALPHA_BLEND_SRC_SRCALPHA		(4 << 16)
#	define MACH64_ALPHA_BLEND_SRC_INVSRCALPHA	(5 << 16)
#	define MACH64_ALPHA_BLEND_SRC_DSTALPHA		(6 << 16)
#	define MACH64_ALPHA_BLEND_SRC_INVDSTALPHA	(7 << 16)
#	define MACH64_ALPHA_BLEND_DST_MASK		(7 << 19)
#	define MACH64_ALPHA_BLEND_DST_ZERO		(0 << 19)
#	define MACH64_ALPHA_BLEND_DST_ONE		(1 << 19)
#	define MACH64_ALPHA_BLEND_DST_SRCCOLOR		(2 << 19)
#	define MACH64_ALPHA_BLEND_DST_INVSRCCOLOR	(3 << 19)
#	define MACH64_ALPHA_BLEND_DST_SRCALPHA		(4 << 19)
#	define MACH64_ALPHA_BLEND_DST_INVSRCALPHA	(5 << 19)
#	define MACH64_ALPHA_BLEND_DST_DSTALPHA		(6 << 19)
#	define MACH64_ALPHA_BLEND_DST_INVDSTALPHA	(7 << 19)
#	define MACH64_TEX_LIGHT_FCN_MASK		(3 << 22)
#	define MACH64_TEX_LIGHT_FCN_REPLACE		(0 << 22)
#	define MACH64_TEX_LIGHT_FCN_MODULATE		(1 << 22)
#	define MACH64_TEX_LIGHT_FCN_ALPHA_DECAL		(2 << 22)
#	define MACH64_MIP_MAP_DISABLE			(1 << 24)
#	define MACH64_BILINEAR_TEX_EN			(1 << 25)
#	define MACH64_TEX_BLEND_FCN_MASK		(3 << 26)
#	define MACH64_TEX_BLEND_FCN_NEAREST		(0 << 26)
#	define MACH64_TEX_BLEND_FCN_LINEAR		(2 << 26)
#	define MACH64_TEX_BLEND_FCN_TRILINEAR		(3 << 26)
#	define MACH64_TEX_AMASK_AEN			(1 << 28)
#	define MACH64_TEX_AMASK_BLEND_EDGE		(1 << 29)
#	define MACH64_TEX_MAP_AEN			(1 << 30)
#	define MACH64_SRC_3D_HOST_FIFO			(1 << 31)
#define MACH64_SCRATCH_REG0			0x0480
#define MACH64_SCRATCH_REG1			0x0484
#define MACH64_SECONDARY_TEX_OFF		0x0778
#define MACH64_SETUP_CNTL			0x0304
#	define MACH64_DONT_START_TRI			(1 << 0)
#	define MACH64_DONT_START_ANY			(1 << 2)
#	define MACH64_FLAT_SHADE_MASK			(3 << 3)
#	define MACH64_FLAT_SHADE_OFF			(0 << 3)
#	define MACH64_FLAT_SHADE_VERTEX_1		(1 << 3)
#	define MACH64_FLAT_SHADE_VERTEX_2		(2 << 3)
#	define MACH64_FLAT_SHADE_VERTEX_3		(3 << 3)
#	define MACH64_SOLID_MODE_OFF			(0 << 5)
#	define MACH64_SOLID_MODE_ON			(1 << 5)
#	define MACH64_LOG_MAX_INC_ADJ			(1 << 6)
#	define MACH64_SET_UP_CONTINUE			(1 << 31)
#define MACH64_SRC_CNTL				0x05b4
#define MACH64_SRC_HEIGHT1			0x0594
#define MACH64_SRC_HEIGHT2			0x05ac
#define MACH64_SRC_HEIGHT1_WIDTH1		0x0598
#define MACH64_SRC_HEIGHT2_WIDTH2		0x05b0
#define MACH64_SRC_OFF_PITCH			0x0580
#define MACH64_SRC_WIDTH1			0x0590
#define MACH64_SRC_Y_X				0x058c

#define MACH64_TEX_0_OFF			0x05c0
#define MACH64_TEX_CNTL				0x0774
#	define MACH64_LOD_BIAS_SHIFT			0
#	define MACH64_LOD_BIAS_MASK			(0xf << 0)
#	define MACH64_COMP_FACTOR_SHIFT			4
#	define MACH64_COMP_FACTOR_MASK			(0xf << 4)
#	define MACH64_TEXTURE_COMPOSITE			(1 << 8)
#	define MACH64_COMP_COMBINE_BLEND		(0 << 9)
#	define MACH64_COMP_COMBINE_MODULATE		(1 << 9)
#	define MACH64_COMP_BLEND_NEAREST		(0 << 11)
#	define MACH64_COMP_BLEND_BILINEAR		(1 << 11)
#	define MACH64_COMP_FILTER_NEAREST		(0 << 12)
#	define MACH64_COMP_FILTER_BILINEAR		(1 << 12)
#	define MACH64_COMP_ALPHA			(1 << 13)
#	define MACH64_TEXTURE_TILING			(1 << 14)
#	define MACH64_COMPOSITE_TEX_TILING		(1 << 15)
#	define MACH64_TEX_COLLISION_DISABLE		(1 << 16)
#	define MACH64_TEXTURE_CLAMP_S			(1 << 17)
#	define MACH64_TEXTURE_CLAMP_T			(1 << 18)
#	define MACH64_TEX_ST_MULT_W			(0 << 19)
#	define MACH64_TEX_ST_DIRECT			(1 << 19)
#	define MACH64_TEX_SRC_LOCAL			(0 << 20)
#	define MACH64_TEX_SRC_AGP			(1 << 20)
#	define MACH64_TEX_UNCOMPRESSED			(0 << 21)
#	define MACH64_TEX_VQ_COMPRESSED			(1 << 21)
#	define MACH64_COMP_TEX_UNCOMPRESSED		(0 << 22)
#	define MACH64_COMP_TEX_VQ_COMPRESSED		(1 << 22)
#	define MACH64_TEX_CACHE_FLUSH			(1 << 23)
#	define MACH64_SEC_TEX_CLAMP_S			(1 << 24)
#	define MACH64_SEC_TEX_CLAMP_T			(1 << 25)
#	define MACH64_TEX_WRAP_S			(1 << 28)
#	define MACH64_TEX_WRAP_T			(1 << 29)
#	define MACH64_TEX_CACHE_SIZE_4K			(1 << 30)
#	define MACH64_TEX_CACHE_SIZE_2K			(1 << 30)
#	define MACH64_SECONDARY_STW			(1 << 31)
#define MACH64_TEX_PALETTE			0x077c
#define MACH64_TEX_PALETTE_INDEX		0x0740
#define MACH64_TEX_SIZE_PITCH			0x0770

#define MACH64_VERTEX_1_ARGB			0x0254
#define MACH64_VERTEX_1_S			0x0240
#define MACH64_VERTEX_1_SECONDARY_S		0x0328
#define MACH64_VERTEX_1_SECONDARY_T		0x032c
#define MACH64_VERTEX_1_SECONDARY_W		0x0330
#define MACH64_VERTEX_1_SPEC_ARGB		0x024c
#define MACH64_VERTEX_1_T			0x0244
#define MACH64_VERTEX_1_W			0x0248
#define MACH64_VERTEX_1_X_Y			0x0258
#define MACH64_VERTEX_1_Z			0x0250
#define MACH64_VERTEX_2_ARGB			0x0274
#define MACH64_VERTEX_2_S			0x0260
#define MACH64_VERTEX_2_SECONDARY_S		0x0334
#define MACH64_VERTEX_2_SECONDARY_T		0x0338
#define MACH64_VERTEX_2_SECONDARY_W		0x033c
#define MACH64_VERTEX_2_SPEC_ARGB		0x026c
#define MACH64_VERTEX_2_T			0x0264
#define MACH64_VERTEX_2_W			0x0268
#define MACH64_VERTEX_2_X_Y			0x0278
#define MACH64_VERTEX_2_Z			0x0270
#define MACH64_VERTEX_3_ARGB			0x0294
#define MACH64_VERTEX_3_S			0x0280
#define MACH64_VERTEX_3_SECONDARY_S		0x02a0
#define MACH64_VERTEX_3_SECONDARY_T		0x02a4
#define MACH64_VERTEX_3_SECONDARY_W		0x02a8
#define MACH64_VERTEX_3_SPEC_ARGB		0x028c
#define MACH64_VERTEX_3_T			0x0284
#define MACH64_VERTEX_3_W			0x0288
#define MACH64_VERTEX_3_X_Y			0x0298
#define MACH64_VERTEX_3_Z			0x0290

#define MACH64_Z_CNTL				0x054c
#	define MACH64_Z_EN				(1 << 0)
#	define MACH64_Z_SRC_2D				(1 << 1)
#	define MACH64_Z_TEST_MASK			(7 << 4)
#	define MACH64_Z_TEST_NEVER			(0 << 4)
#	define MACH64_Z_TEST_LESS			(1 << 4)
#	define MACH64_Z_TEST_LEQUAL			(2 << 4)
#	define MACH64_Z_TEST_EQUAL			(3 << 4)
#	define MACH64_Z_TEST_GEQUAL			(4 << 4)
#	define MACH64_Z_TEST_GREATER			(5 << 4)
#	define MACH64_Z_TEST_NOTEQUAL			(6 << 4)
#	define MACH64_Z_TEST_ALWAYS			(7 << 4)
#	define MACH64_Z_MASK_EN				(1 << 8)
#define MACH64_Z_OFF_PITCH			0x0548



#define MACH64_DATATYPE_CI8				2
#define MACH64_DATATYPE_ARGB1555			3
#define MACH64_DATATYPE_RGB565				4
#define MACH64_DATATYPE_ARGB8888			6
#define MACH64_DATATYPE_RGB332				7
#define MACH64_DATATYPE_Y8				8
#define MACH64_DATATYPE_RGB8				9
#define MACH64_DATATYPE_VYUY422				11
#define MACH64_DATATYPE_YVYU422				12
#define MACH64_DATATYPE_AYUV444				14
#define MACH64_DATATYPE_ARGB4444			15

#define MACH64_LAST_FRAME_REG			MACH64_PAT_REG0
#define MACH64_LAST_DISPATCH_REG		MACH64_PAT_REG1

#endif /* __MACH64_REG_H__ */
