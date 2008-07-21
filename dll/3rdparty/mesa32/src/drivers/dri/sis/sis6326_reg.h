/*
 * Copyright 2005 Eric Anholt
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#ifndef _sis6326_reg_h_
#define _sis6326_reg_h_

#define REG_6326_BitBlt_SrcAddr		0x8280
#define REG_6326_BitBlt_DstAddr		0x8284
#define REG_6326_BitBlt_DstSrcPitch	0x8288
#define REG_6326_BitBlt_HeightWidth	0x828c
#define REG_6326_BitBlt_fgColor		0x8290
#define REG_6326_BitBlt_bgColor		0x8294
#define REG_6326_BitBlt_Mask30		0x8298
#define REG_6326_BitBlt_Mask74		0x829c
#define REG_6326_BitBlt_ClipTopLeft	0x82a0
#define REG_6326_BitBlt_ClitBottomRight 0x82a4
#define REG_6326_BitBlt_Cmd		0x82a8
#define REG_6326_BitBlt_Pat		0x82ac

#define REG_6326_3D_TSFSa		0x8800
#define REG_6326_3D_TSZa		0x8804
#define REG_6326_3D_TSXa		0x8808
#define REG_6326_3D_TSYa		0x880C
#define REG_6326_3D_TSARGBa		0x8810
#define REG_6326_3D_TSUa		0x8814
#define REG_6326_3D_TSVa		0x8818
#define REG_6326_3D_TSWa		0x881C

#define REG_6326_3D_TSFSb		0x8820
#define REG_6326_3D_TSZb		0x8824
#define REG_6326_3D_TSXb		0x8828
#define REG_6326_3D_TSYb		0x882C
#define REG_6326_3D_TSARGBb		0x8830
#define REG_6326_3D_TSUb		0x8834
#define REG_6326_3D_TSVb		0x8838
#define REG_6326_3D_TSWb		0x883C

#define REG_6326_3D_TSFSc		0x8840
#define REG_6326_3D_TSZc		0x8844
#define REG_6326_3D_TSXc		0x8848
#define REG_6326_3D_TSYc		0x884C
#define REG_6326_3D_TSARGBc		0x8850
#define REG_6326_3D_TSUc		0x8854
#define REG_6326_3D_TSVc		0x8858
#define REG_6326_3D_TSWc		0x885C

#define REG_6326_3D_TEnable		0x8A00
#define REG_6326_3D_ZSet		0x8A04
#define REG_6326_3D_ZAddress		0x8A08

#define REG_6326_3D_AlphaSet		0x8A0C
#define REG_6326_3D_AlphaAddress	0x8A10
#define REG_6326_3D_DstSet		0x8A14
#define REG_6326_3D_DstAddress		0x8A18
#define REG_6326_3D_LinePattern		0x8A1C
#define REG_6326_3D_FogSet		0x8A20

#define REG_6326_3D_DstSrcBlendMode	0x8A28

#define REG_6326_3D_ClipTopBottom	0x8A30
#define REG_6326_3D_ClipLeftRight	0x8A34

#define REG_6326_3D_TextureSet		0x8A38
#define REG_6326_3D_TextureBlendSet	0x8A3C
/* Low transparency value is in TextureBlendSet */
#define REG_6326_3D_TextureTransparencyColorHigh	0x8A40

#define REG_6326_3D_TextureAddress0	0x8A44
#define REG_6326_3D_TextureAddress1	0x8A48
#define REG_6326_3D_TextureAddress2	0x8A4C
#define REG_6326_3D_TextureAddress3	0x8A50
#define REG_6326_3D_TextureAddress4	0x8A54
#define REG_6326_3D_TextureAddress5	0x8A58
#define REG_6326_3D_TextureAddress6	0x8A5C
#define REG_6326_3D_TextureAddress7	0x8A60
#define REG_6326_3D_TextureAddress8	0x8A64
#define REG_6326_3D_TextureAddress9	0x8A68

#define REG_6326_3D_TexturePitch01	0x8A6C
#define REG_6326_3D_TexturePitch23	0x8A70
#define REG_6326_3D_TexturePitch45	0x8A74
#define REG_6326_3D_TexturePitch67	0x8A78
#define REG_6326_3D_TexturePitch89	0x8A7C

#define REG_6326_3D_TextureWidthHeight	0x8A80
#define REG_6326_3D_TextureBorderColor	0x8A90

#define REG_6326_3D_EndPrimitiveList	0x8Aff

/*
 * REG_6326_BitBlt_fgColor		(0x8290-0x8293)
 * REG_6326_BitBlt_bgColor		(0x8294-0x8297)
 */
#define MASK_BltRop			0xff000000
#define MASK_BltColor			0x00ffffff

#define SiS_ROP_SRCCOPY			0xcc000000
#define SiS_ROP_PATCOPY			0xf0000000

/*
 * REG_6326_BitBlt_Cmd			(0x82a8-0x82ab)
 */
#define MASK_QueueStatus		0x0000ffff
#define MASK_BltCmd0			0x00ff0000
#define MASK_BltCmd1			0xff000000

#define BLT_SRC_BG			0x00000000
#define BLT_SRC_FG			0x00010000
#define BLT_SRC_VID			0x00020000
#define BLT_SRC_CPU			0x00030000
#define BLT_PAT_BG			0x00000000
#define BLT_PAT_FG			0x00040000
#define BLT_PAT_PAT			0x000b0000
#define BLT_XINC			0x00100000
#define BLT_YINC			0x00200000
#define BLT_CLIP			0x00400000
#define BLT_BUSY			0x04000000

/*
 * REG_3D_PrimitiveSet -- Define Fire Primitive Mask (89F8h-89FBh)
 */
#define MASK_6326_DrawPrimitiveCommand	0x00000007
#define MASK_6326_SetFirePosition	0x00000F00
#define MASK_6326_ShadingMode		0x001c0000
#define MASK_6326_Direction		0x0003f000

/* OP_3D_{POINT,LINE,TRIANGLE}_DRAW same as 300-series */
/* OP_3D_DIRECTION*_ same as 300-series */

#define OP_6326_3D_FIRE_TFIRE		0x00000000
#define OP_6326_3D_FIRE_TSARGBa		0x00000100
#define OP_6326_3D_FIRE_TSWa		0x00000200
#define OP_6326_3D_FIRE_TSARGBb		0x00000300
#define OP_6326_3D_FIRE_TSWb		0x00000400
#define OP_6326_3D_FIRE_TSARGBc		0x00000500
#define OP_6326_3D_FIRE_TSWc		0x00000600
#define OP_6326_3D_FIRE_TSVc		0x00000700

#define OP_6326_3D_ATOP			0x00000000
#define OP_6326_3D_BTOP			0x00010000
#define OP_6326_3D_CTOP			0x00020000
#define OP_6326_3D_AMID			0x00000000
#define OP_6326_3D_BMID			0x00004000
#define OP_6326_3D_CMID			0x00008000
#define OP_6326_3D_ABOT			0x00000000
#define OP_6326_3D_BBOT			0x00001000
#define OP_6326_3D_CBOT			0x00002000

#define OP_6326_3D_SHADE_FLAT_TOP	0x00040000
#define OP_6326_3D_SHADE_FLAT_MID	0x00080000
#define OP_6326_3D_SHADE_FLAT_BOT	0x000c0000
#define OP_6326_3D_SHADE_FLAT_GOURAUD	0x00100000


/*
 * REG_6326_3D_EngineFire
 */
#define MASK_CmdQueueLen		0x0FFF0000
#define ENG_3DIDLEQE			0x00000002
#define ENG_3DIDLE			0x00000001

/*
 * REG_6326_3D_TEnable -- Define Capility Enable Mask (8A00h-8A03h)
 */
#define S_ENABLE_Dither			(1 << 0)
#define S_ENABLE_Transparency		(1 << 1)
#define S_ENABLE_Blend			(1 << 2)
#define S_ENABLE_Fog			(1 << 3)
#define S_ENABLE_Specular		(1 << 4)
#define S_ENABLE_LargeCache		(1 << 5)
#define S_ENABLE_TextureCache		(1 << 7)
#define S_ENABLE_TextureTransparency	(1 << 8)
#define S_ENABLE_TexturePerspective	(1 << 9)
#define S_ENABLE_Texture		(1 << 10)
#define S_ENABLE_PrimSetup		(1 << 11)
#define S_ENABLE_LinePattern		(1 << 12)
#define S_ENABLE_StippleAlpha		(1 << 13) /* requires S_ENABLE_Stipple */
#define S_ENABLE_Stipple		(1 << 14)
#define S_ENABLE_AlphaBuffer		(1 << 16)
#define S_ENABLE_AlphaTest		(1 << 17)
#define S_ENABLE_AlphaWrite		(1 << 18)
#define S_ENABLE_ZTest			(1 << 20)
#define S_ENABLE_ZWrite			(1 << 21)

/*
 * REG_3D_ZSet -- Define Z Buffer Setting Mask (8A08h-8A0Bh)
 */
#define MASK_6326_ZBufferPitch		0x00003FFF
#define MASK_6326_ZTestMode		0x00070000
#define MASK_6326_ZBufferFormat		0x00100000

#define S_ZSET_FORMAT_8			0x00000000
#define S_ZSET_FORMAT_16		0x00100000

#define S_ZSET_PASS_NEVER		0x00000000
#define S_ZSET_PASS_LESS		0x00010000
#define S_ZSET_PASS_EQUAL		0x00020000
#define S_ZSET_PASS_LEQUAL		0x00030000
#define S_ZSET_PASS_GREATER		0x00040000
#define S_ZSET_PASS_NOTEQUAL		0x00050000
#define S_ZSET_PASS_GEQUAL		0x00060000
#define S_ZSET_PASS_ALWAYS		0x00070000

/*
 * REG_3D_AlphaSet -- Define Alpha Buffer Setting Mask (8A0Ch-8A0Fh)
 */
#define MASK_AlphaBufferPitch		0x000003FF
#define MASK_AlphaRefValue		0x00FF0000
#define MASK_AlphaTestMode		0x07000000
#define MASK_AlphaBufferFormat		0x30000000

#define S_ASET_FORMAT_8			0x30000000

#define S_ASET_PASS_NEVER		0x00000000
#define S_ASET_PASS_LESS		0x01000000
#define S_ASET_PASS_EQUAL		0x02000000
#define S_ASET_PASS_LEQUAL		0x03000000
#define S_ASET_PASS_GREATER		0x04000000
#define S_ASET_PASS_NOTEQUAL		0x05000000
#define S_ASET_PASS_GEQUAL		0x06000000
#define S_ASET_PASS_ALWAYS		0x07000000

/*
 * REG_3D_DstSet -- Define Destination Buffer Setting Mask (8A14h-8A17h)
 */
/* pitch, format, depth, rgborder, rop bits same as 300-series */

/*
 * REG_6326_3D_FogSet -- Define Fog Mask (8A20h-8A23h)
 */
#define MASK_6326_FogColor		0x00FFFFFF
#define MASK_6326_FogMode		0x01000000

#define FOGMODE_6326_CONST		0x00000000
#define FOGMODE_6326_LINEAR		0x01000000

/*
 * REG_6326_3D_DstSrcBlendMode		(0x8A28 - 0x8A2B)
 */
#define MASK_6326_SrcBlendMode		0xf0000000
#define MASK_6326_DstBlendMode		0x0f000000
#define MASK_6326_TransparencyColor	0x00ffffff

#define S_DBLEND_ZERO			0x00000000
#define S_DBLEND_ONE			0x10000000
#define S_DBLEND_SRC_COLOR		0x20000000
#define S_DBLEND_INV_SRC_COLOR		0x30000000
#define S_DBLEND_SRC_ALPHA		0x40000000
#define S_DBLEND_INV_SRC_ALPHA		0x50000000
#define S_DBLEND_DST_ALPHA		0x60000000
#define S_DBLEND_INV_DST_ALPHA		0x70000000

#define S_SBLEND_ZERO			0x00000000
#define S_SBLEND_ONE			0x01000000
#define S_SBLEND_SRC_ALPHA		0x04000000
#define S_SBLEND_INV_SRC_ALPHA		0x05000000
#define S_SBLEND_DST_ALPHA		0x06000000
#define S_SBLEND_INV_DST_ALPHA		0x07000000
#define S_SBLEND_DST_COLOR		0x08000000
#define S_SBLEND_INV_DST_COLOR		0x09000000
#define S_SBLEND_SRC_ALPHA_SAT		0x0A000000
#define S_SBLEND_BOTH_SRC_ALPHA		0x0B000000
#define S_SBLEND_BOTH_INV_SRC_ALPHA	0x0C000000

/* 
 * REG_6326_3D_TextureSet		(0x8A38 - 0x8A3B)
 */
#define MASK_6326_TextureMinFilter	0x00000007
#define MASK_6326_TextureMagFilter	0x00000008
#define MASK_6326_ClearTexCache		0x00000010
#define MASK_6326_TextureInSystem	0x00000020
#define MASK_6326_TextureLevel		0x00000F00
#define MASK_6326_TextureSignYUVFormat	0x00008000
#define MASK_6326_TextureMappingMode	0x00FF0000

#define TEXEL_6326_BGR_ORDER		0x80000000

#define TEXEL_6326_INDEX1		0x00000000
#define TEXEL_6326_INDEX2		0x01000000
#define TEXEL_6326_INDEX4		0x02000000

#define TEXEL_6326_M4			0x10000000
#define TEXEL_6326_AM44			0x16000000

#define TEXEL_6326_YUV422		0x20000000 /* YUYV */
#define TEXEL_6326_YVU422		0x21000000 /* YVYU */
#define TEXEL_6326_UVY422		0x22000000 /* UYVY */
#define TEXEL_6326_VUY422		0x23000000 /* VYUY */

#define TEXEL_6326_L1			0x30000000
#define TEXEL_6326_L2			0x31000000
#define TEXEL_6326_L4			0x32000000
#define TEXEL_6326_L8			0x33000000

#define TEXEL_6326_AL22			0x35000000
#define TEXEL_6326_AL44			0x38000000
#define TEXEL_6326_AL88			0x3c000000

#define TEXEL_6326_RGB_332_8		0x40000000
#define TEXEL_6326_RGB_233_8		0x41000000
#define TEXEL_6326_RGB_232_8		0x42000000
#define TEXEL_6326_ARGB_1232_8		0x43000000

#define TEXEL_6326_RGB_555_16		0x50000000
#define TEXEL_6326_RGB_565_16		0x51000000
#define TEXEL_6326_ARGB_1555_16		0x52000000
#define TEXEL_6326_ARGB_4444_16		0x53000000
#define TEXEL_6326_ARGB_8332_16		0x54000000
#define TEXEL_6326_ARGB_8233_16		0x55000000
#define TEXEL_6326_ARGB_8232_16		0x56000000

#define TEXEL_6326_ARGB_8565_24		0x63000000
#define TEXEL_6326_ARGB_8555_24		0x67000000
#define TEXEL_6326_RGB_888_24		0x68000000

#define TEXEL_6326_ARGB_8888_32		0x73000000
#define TEXEL_6326_ARGB_0888_32		0x74000000

#define TEX_MAP_WRAP_U			0x00010000
#define TEX_MAP_WRAP_V			0x00020000
#define TEX_MAP_MIRROR_U		0x00040000
#define TEX_MAP_MIRROR_V		0x00080000
#define TEX_MAP_CLAMP_U			0x00100000
#define TEX_MAP_CLAMP_V			0x00200000
#define TEX_MAP_USE_CTB_SMOOTH		0x00400000
#define TEX_MAP_USE_CTB			0x00800000

#define TEX_FILTER_NEAREST		0x00000000
#define TEX_FILTER_LINEAR		0x00000001
#define TEX_FILTER_NEAREST_MIP_NEAREST	0x00000002
#define TEX_FILTER_NEAREST_MIP_LINEAR	0x00000003
#define TEX_FILTER_LINEAR_MIP_NEAREST	0x00000004
#define TEX_FILTER_LINEAR_MIP_LINEAR	0x00000005
#define TEX_FILTER_MAG_NEAREST		0x00000000
#define TEX_FILTER_MAG_LINEAR		0x00000008

/* 
 * REG_6326_3D_TextureBlendSet		(0x8A3C - 0x8A3F)
 */
#define MASK_TextureTransparencyLowB	0x000000ff
#define MASK_TextureTransparencyLowG	0x0000FF00
#define MASK_TextureTransparencyLowR	0x00ff0000
#define MASK_TextureBlend		0x0f000000

#define TB_C_CS				(0 << 26)
#define TB_C_CF				(1 << 26)
#define TB_C_CFCS			(2 << 26) /* also 3 << 26 */
#define TB_C_CFOMAS_ASCS		(4 << 26)
#define TB_C_CSOMAF_AFCF		(6 << 26) /* also 7 << 26 */

#define TB_A_AS				(0 << 24)
#define TB_A_AF				(1 << 24)
#define TB_A_AFAS			(1 << 24)

/* 
 * REG_6326_3D_TextureTransparencyColorHigh	(0x8A40 - 0x8A43)
 */
#define MASK_TextureTransparencyHighB	0x000000FF
#define MASK_TextureTransparencyHighG	0x0000FF00
#define MASK_TextureTransparencyHighR	0x00FF0000

/*
 * REG_3D_TexturePitch01-89		(0x8A6C - 0x8A7F)
 */
#define MASK_TexturePitchOdd		0x000003FF
#define MASK_TexturePitchEven		0x03FF0000
#define SHIFT_TexturePitchEven		16

/* 
 * REG_3D_TextureWidthHeightMix		(0x8A80 - 0x8A83)
 */
#define MASK_TextureWidthLog2		0xf0000000
#define MASK_TextureHeightLog2		0x0f000000

/* 
 * REG_3D_TextureBorderColor		(0x8A90 - 0x8A93)
 */
#define MASK_TextureBorderColorB	0x000000FF
#define MASK_TextureBorderColorG	0x0000FF00
#define MASK_TextureBorderColorR	0x00FF0000
#define MASK_TextureBorderColorA	0xFF000000

#endif /* _sis6326_reg_h_ */
