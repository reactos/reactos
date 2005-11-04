/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810_3d_reg.h,v 1.7 2002/02/22 21:33:03 dawes Exp $ */

#ifndef I810_3D_REG_H
#define I810_3D_REG_H

#include "i810_reg.h"

/* Registers not used in the X server
 */

#define I810_NOP_ID           0x2094
#define I810_NOP_ID_MASK        ((1<<22)-1)


/* 3D instructions
 */


/* GFXRENDERSTATE_PV_PIXELIZATION_RULE, p149
 *
 * Format:
 *     0: GFX_OP_PV_RULE | PV_*
 *
 */
#define GFX_OP_PV_RULE           ((0x3<<29)|(0x7<<24))
#define PV_SMALL_TRI_FILTER_ENABLE   (0x1<<11)
#define PV_UPDATE_PIXRULE            (0x1<<10)
#define PV_PIXRULE_ENABLE            (0x1<<9)
#define PV_UPDATE_LINELIST           (0x1<<8)
#define PV_LINELIST_MASK             (0x3<<6)
#define PV_LINELIST_PV0              (0x0<<6)
#define PV_LINELIST_PV1              (0x1<<6)
#define PV_UPDATE_TRIFAN             (0x1<<5)
#define PV_TRIFAN_MASK               (0x3<<3)
#define PV_TRIFAN_PV0                (0x0<<3)
#define PV_TRIFAN_PV1                (0x1<<3)
#define PV_TRIFAN_PV2                (0x2<<3)
#define PV_UPDATE_TRISTRIP           (0x1<<2)
#define PV_TRISTRIP_MASK             (0x3<<0)
#define PV_TRISTRIP_PV0              (0x0<<0)
#define PV_TRISTRIP_PV1              (0x1<<0)
#define PV_TRISTRIP_PV2              (0x2<<0)


/* GFXRENDERSTATE_SCISSOR_ENABLE, p146
 *
 * Format:
 *     0: GFX_OP_SCISSOR | SC_*
 */
#define GFX_OP_SCISSOR         ((0x3<<29)|(0x1c<<24)|(0x10<<19))
#define SC_UPDATE_SCISSOR       (0x1<<1)
#define SC_ENABLE_MASK          (0x1<<0)
#define SC_ENABLE               (0x1<<0)

/* GFXRENDERSTATE_SCISSOR_INFO, p147
 *
 * Format:
 *     0: GFX_OP_SCISSOR_INFO
 *     1: SCI_MIN_*
 *     2: SCI_MAX_*
 */
#define GFX_OP_SCISSOR_INFO    ((0x3<<29)|(0x1d<<24)|(0x81<<16)|(0x1))
#define SCI_YMIN_MASK      (0xffff<<16)
#define SCI_XMIN_MASK      (0xffff<<0)
#define SCI_YMAX_MASK      (0xffff<<16)
#define SCI_XMAX_MASK      (0xffff<<0)

/* GFXRENDERSTATE_DRAWING_RECT_INFO, p144
 *
 * Format:
 *     0: GFX_OP_DRAWRECT_INFO
 *     1: DR1_*
 *     2: DR2_*
 *     3: DR3_*
 *     4: DR4_*
 */
#define GFX_OP_DRAWRECT_INFO   ((0x3<<29)|(0x1d<<24)|(0x80<<16)|(0x3))
#define DR1_RECT_CLIP_ENABLE   (0x0<<31)
#define DR1_RECT_CLIP_DISABLE  (0x1<<31)
#define DR1_X_DITHER_BIAS_MASK (0x3<<26)
#define DR1_X_DITHER_BIAS_SHIFT      26
#define DR1_Y_DITHER_BIAS_MASK (0x3<<24)
#define DR1_Y_DITHER_BIAS_SHIFT      24
#define DR2_YMIN_MASK          (0xffff<<16)
#define DR2_XMIN_MASK          (0xffff<<0)
#define DR3_YMAX_MASK          (0xffff<<16)
#define DR3_XMAX_MASK          (0xffff<<0)
#define DR4_YORG_MASK          (0x3ff<<16)
#define DR4_XORG_MASK          (0x7ff<<0)


/* GFXRENDERSTATE_LINEWIDTH_CULL_SHADE_MODE, p140
 *
 * Format:
 *     0: GFX_OP_LINEWIDTH_CULL_SHADE_MODE | LCS_*
 */
#define GFX_OP_LINEWIDTH_CULL_SHADE_MODE  ((0x3<<29)|(0x2<<24))
#define LCS_UPDATE_ZMODE        (0x1<<20)
#define LCS_Z_MASK              (0xf<<16)
#define LCS_Z_NEVER             (0x1<<16)
#define LCS_Z_LESS              (0x2<<16)
#define LCS_Z_EQUAL             (0x3<<16)
#define LCS_Z_LEQUAL            (0x4<<16)
#define LCS_Z_GREATER           (0x5<<16)
#define LCS_Z_NOTEQUAL          (0x6<<16)
#define LCS_Z_GEQUAL            (0x7<<16)
#define LCS_Z_ALWAYS            (0x8<<16)
#define LCS_UPDATE_LINEWIDTH    (0x1<<15)
#define LCS_LINEWIDTH_MASK      (0x7<<12)
#define LCS_LINEWIDTH_SHIFT           12
#define LCS_LINEWIDTH_0_5       (0x1<<12)
#define LCS_LINEWIDTH_1_0       (0x2<<12)
#define LCS_LINEWIDTH_2_0       (0x4<<12)
#define LCS_LINEWIDTH_3_0       (0x6<<12)
#define LCS_UPDATE_ALPHA_INTERP (0x1<<11)
#define LCS_ALPHA_FLAT          (0x1<<10)
#define LCS_ALPHA_INTERP        (0x0<<10)
#define LCS_UPDATE_FOG_INTERP   (0x1<<9)
#define LCS_FOG_INTERP          (0x0<<8)
#define LCS_FOG_FLAT            (0x1<<8)
#define LCS_UPDATE_SPEC_INTERP  (0x1<<7)
#define LCS_SPEC_INTERP         (0x0<<6)
#define LCS_SPEC_FLAT           (0x1<<6)
#define LCS_UPDATE_RGB_INTERP   (0x1<<5)
#define LCS_RGB_INTERP          (0x0<<4)
#define LCS_RGB_FLAT            (0x1<<4)
#define LCS_UPDATE_CULL_MODE    (0x1<<3)
#define LCS_CULL_MASK           (0x7<<0)
#define LCS_CULL_DISABLE        (0x1<<0)
#define LCS_CULL_CW             (0x2<<0)
#define LCS_CULL_CCW            (0x3<<0)
#define LCS_CULL_BOTH           (0x4<<0)

#define LCS_INTERP_FLAT (LCS_ALPHA_FLAT|LCS_RGB_FLAT|LCS_SPEC_FLAT)
#define LCS_UPDATE_INTERP (LCS_UPDATE_ALPHA_INTERP| 	\
			   LCS_UPDATE_RGB_INTERP|	\
			   LCS_UPDATE_SPEC_INTERP)


/* GFXRENDERSTATE_BOOLEAN_ENA_1, p142
 *
 */
#define GFX_OP_BOOL_1           ((0x3<<29)|(0x3<<24))
#define B1_UPDATE_SPEC_SETUP_ENABLE   (1<<19)
#define B1_SPEC_SETUP_ENABLE          (1<<18)
#define B1_UPDATE_ALPHA_SETUP_ENABLE  (1<<17)
#define B1_ALPHA_SETUP_ENABLE         (1<<16)
#define B1_UPDATE_CI_KEY_ENABLE       (1<<15)
#define B1_CI_KEY_ENABLE              (1<<14)
#define B1_UPDATE_CHROMAKEY_ENABLE    (1<<13)
#define B1_CHROMAKEY_ENABLE           (1<<12)
#define B1_UPDATE_Z_BIAS_ENABLE       (1<<11)
#define B1_Z_BIAS_ENABLE              (1<<10)
#define B1_UPDATE_SPEC_ENABLE         (1<<9)
#define B1_SPEC_ENABLE                (1<<8)
#define B1_UPDATE_FOG_ENABLE          (1<<7)
#define B1_FOG_ENABLE                 (1<<6)
#define B1_UPDATE_ALPHA_TEST_ENABLE   (1<<5)
#define B1_ALPHA_TEST_ENABLE          (1<<4)
#define B1_UPDATE_BLEND_ENABLE        (1<<3)
#define B1_BLEND_ENABLE               (1<<2)
#define B1_UPDATE_Z_TEST_ENABLE       (1<<1)
#define B1_Z_TEST_ENABLE              (1<<0)

/* GFXRENDERSTATE_BOOLEAN_ENA_2, p143
 *
 */
#define GFX_OP_BOOL_2          ((0x3<<29)|(0x4<<24))
#define B2_UPDATE_MAP_CACHE_ENABLE     (1<<17)
#define B2_MAP_CACHE_ENABLE            (1<<16)
#define B2_UPDATE_ALPHA_DITHER_ENABLE  (1<<15)
#define B2_ALPHA_DITHER_ENABLE         (1<<14)
#define B2_UPDATE_FOG_DITHER_ENABLE    (1<<13)
#define B2_FOG_DITHER_ENABLE           (1<<12)
#define B2_UPDATE_SPEC_DITHER_ENABLE   (1<<11)
#define B2_SPEC_DITHER_ENABLE          (1<<10)
#define B2_UPDATE_RGB_DITHER_ENABLE    (1<<9)
#define B2_RGB_DITHER_ENABLE           (1<<8)
#define B2_UPDATE_FB_WRITE_ENABLE      (1<<3)
#define B2_FB_WRITE_ENABLE             (1<<2)
#define B2_UPDATE_ZB_WRITE_ENABLE      (1<<1)
#define B2_ZB_WRITE_ENABLE             (1<<0)


/* GFXRENDERSTATE_FOG_COLOR, p144
 */
#define GFX_OP_FOG_COLOR       ((0x3<<29)|(0x15<<24))
#define FOG_RED_SHIFT          16
#define FOG_GREEN_SHIFT        8
#define FOG_BLUE_SHIFT         0
#define FOG_RESERVED_MASK      ((0x7<<16)|(0x3<<8)|(0x3))


/* GFXRENDERSTATE_Z_BIAS_ALPHA_FUNC_REF, p139
 */
#define GFX_OP_ZBIAS_ALPHAFUNC ((0x3<<29)|(0x14<<24))
#define ZA_UPDATE_ZBIAS        (1<<22)
#define ZA_ZBIAS_SHIFT         14
#define ZA_ZBIAS_MASK          (0xff<<14)
#define ZA_UPDATE_ALPHAFUNC    (1<<13)
#define ZA_ALPHA_MASK          (0xf<<9)
#define ZA_ALPHA_NEVER         (1<<9)
#define ZA_ALPHA_LESS          (2<<9)
#define ZA_ALPHA_EQUAL         (3<<9)
#define ZA_ALPHA_LEQUAL        (4<<9)
#define ZA_ALPHA_GREATER       (5<<9)
#define ZA_ALPHA_NOTEQUAL      (6<<9)
#define ZA_ALPHA_GEQUAL        (7<<9)
#define ZA_ALPHA_ALWAYS        (8<<9)
#define ZA_UPDATE_ALPHAREF     (1<<8)
#define ZA_ALPHAREF_MASK       (0xff<<0)
#define ZA_ALPHAREF_SHIFT      0
#define ZA_ALPHAREF_RESERVED   (0x7<<0)


/* GFXRENDERSTATE_SRC_DST_BLEND_MONO, p136
 */
#define GFX_OP_SRC_DEST_MONO    ((0x3<<29)|(0x8<<24))
#define SDM_UPDATE_MONO_ENABLE      (1<<13)
#define SDM_MONO_ENABLE             (1<<12)
#define SDM_UPDATE_SRC_BLEND        (1<<11)
#define SDM_SRC_MASK               (0xf<<6)
#define SDM_SRC_ZERO               (0x1<<6)
#define SDM_SRC_ONE                (0x2<<6)
#define SDM_SRC_SRC_COLOR          (0x3<<6)
#define SDM_SRC_INV_SRC_COLOR      (0x4<<6)
#define SDM_SRC_SRC_ALPHA          (0x5<<6)
#define SDM_SRC_INV_SRC_ALPHA      (0x6<<6)
#define SDM_SRC_DST_COLOR          (0x9<<6)
#define SDM_SRC_INV_DST_COLOR      (0xa<<6)
#define SDM_SRC_BOTH_SRC_ALPHA     (0xc<<6)
#define SDM_SRC_BOTH_INV_SRC_ALPHA (0xd<<6)
#define SDM_UPDATE_DST_BLEND        (1<<5)
#define SDM_DST_MASK               (0xf<<0)
#define SDM_DST_ZERO               (0x1<<0)
#define SDM_DST_ONE                (0x2<<0)
#define SDM_DST_SRC_COLOR          (0x3<<0)
#define SDM_DST_INV_SRC_COLOR      (0x4<<0)
#define SDM_DST_SRC_ALPHA          (0x5<<0)
#define SDM_DST_INV_SRC_ALPHA      (0x6<<0)
#define SDM_DST_DST_COLOR          (0x9<<0)
#define SDM_DST_INV_DST_COLOR      (0xa<<0)
#define SDM_DST_BOTH_SRC_ALPHA     (0xc<<0)
#define SDM_DST_BOTH_INV_SRC_ALPHA (0xd<<0)


/* GFXRENDERSTATE_COLOR_FACTOR, p134
 *
 * Format:
 *     0: GFX_OP_COLOR_FACTOR
 *     1: ARGB8888 color factor
 */
#define GFX_OP_COLOR_FACTOR      ((0x3<<29)|(0x1d<<24)|(0x1<<16)|0x0)

/* GFXRENDERSTATE_MAP_ALPHA_BLEND_STAGES, p132
 */
#define GFX_OP_MAP_ALPHA_STAGES  ((0x3<<29)|(0x1<<24))
#define MA_STAGE_SHIFT           20
#define MA_STAGE_0               (0<<20)
#define MA_STAGE_1               (1<<20)
#define MA_STAGE_2               (2<<20)
#define MA_UPDATE_ARG1           (1<<18)
#define MA_ARG1_MASK             ((0x7<<15)|(0x1<<13))
#define MA_ARG1_ALPHA_FACTOR     (0x1<<15)
#define MA_ARG1_ITERATED_ALPHA   (0x3<<15)
#define MA_ARG1_CURRENT_ALPHA    (0x5<<15)
#define MA_ARG1_TEX0_ALPHA       (0x6<<15)
#define MA_ARG1_TEX1_ALPHA       (0x7<<15)
#define MA_ARG1_INVERT           (0x1<<13)
#define MA_ARG1_DONT_INVERT      (0x0<<13)
#define MA_UPDATE_ARG2           (1<<12)
#define MA_ARG2_MASK             ((0x7<<8)|(0x1<<6))
#define MA_ARG2_ALPHA_FACTOR     (0x1<<8)
#define MA_ARG2_ITERATED_ALPHA   (0x3<<8)
#define MA_ARG2_CURRENT_ALPHA    (0x5<<8)
#define MA_ARG2_TEX0_ALPHA       (0x6<<8)
#define MA_ARG2_TEX1_ALPHA       (0x7<<8)
#define MA_ARG2_INVERT           (0x1<<6)
#define MA_ARG2_DONT_INVERT      (0x0<<6)
#define MA_UPDATE_OP             (1<<5)
#define MA_OP_MASK                   (0xf)
#define MA_OP_ARG1                   (0x1)
#define MA_OP_ARG2                   (0x2)
#define MA_OP_MODULATE               (0x3)
#define MA_OP_MODULATE_X2            (0x4)
#define MA_OP_MODULATE_X4            (0x5)
#define MA_OP_ADD                    (0x6)
#define MA_OP_ADD_SIGNED             (0x7)
#define MA_OP_LIN_BLEND_ITER_ALPHA   (0x8)
#define MA_OP_LIN_BLEND_ALPHA_FACTOR (0xa)
#define MA_OP_LIN_BLEND_TEX0_ALPHA   (0x10)
#define MA_OP_LIN_BLEND_TEX1_ALPHA   (0x11)


/* GFXRENDERSTATE_MAP_COLOR_BLEND_STAGES, p129
 */
#define GFX_OP_MAP_COLOR_STAGES  ((0x3<<29)|(0x0<<24))
#define MC_STAGE_SHIFT           20
#define MC_STAGE_0               (0<<20)
#define MC_STAGE_1               (1<<20)
#define MC_STAGE_2               (2<<20)
#define MC_UPDATE_DEST           (1<<19)
#define MC_DEST_MASK             (1<<18)
#define MC_DEST_CURRENT          (0<<18)
#define MC_DEST_ACCUMULATOR      (1<<18)
#define MC_UPDATE_ARG1           (1<<17)
#define MC_ARG1_MASK             ((0x7<<14)|(0x1<<13)|(0x1<<12))
#define MC_ARG1_ONE              (0x0<<14)
#define MC_ARG1_COLOR_FACTOR     (0x1<<14)
#define MC_ARG1_ACCUMULATOR      (0x2<<14)
#define MC_ARG1_ITERATED_COLOR   (0x3<<14)
#define MC_ARG1_SPECULAR_COLOR   (0x4<<14)
#define MC_ARG1_CURRENT_COLOR    (0x5<<14)
#define MC_ARG1_TEX0_COLOR       (0x6<<14)
#define MC_ARG1_TEX1_COLOR       (0x7<<14)
#define MC_ARG1_DONT_REPLICATE_ALPHA   (0x0<<13)
#define MC_ARG1_REPLICATE_ALPHA        (0x1<<13)
#define MC_ARG1_DONT_INVERT      (0x0<<12)
#define MC_ARG1_INVERT           (0x1<<12)
#define MC_UPDATE_ARG2           (1<<11)
#define MC_ARG2_MASK             ((0x7<<8)|(0x1<<7)|(0x1<<6))
#define MC_ARG2_ONE              (0x0<<8)
#define MC_ARG2_COLOR_FACTOR     (0x1<<8)
#define MC_ARG2_ACCUMULATOR      (0x2<<8)
#define MC_ARG2_ITERATED_COLOR   (0x3<<8)
#define MC_ARG2_SPECULAR_COLOR   (0x4<<8)
#define MC_ARG2_CURRENT_COLOR    (0x5<<8)
#define MC_ARG2_TEX0_COLOR       (0x6<<8)
#define MC_ARG2_TEX1_COLOR       (0x7<<8)
#define MC_ARG2_DONT_REPLICATE_ALPHA   (0x0<<7)
#define MC_ARG2_REPLICATE_ALPHA        (0x1<<7)
#define MC_ARG2_DONT_INVERT      (0x0<<6)
#define MC_ARG2_INVERT           (0x1<<6)
#define MC_UPDATE_OP             (1<<5)
#define MC_OP_MASK                   (0xf)
#define MC_OP_DISABLE                (0x0)
#define MC_OP_ARG1                   (0x1)
#define MC_OP_ARG2                   (0x2)
#define MC_OP_MODULATE               (0x3)
#define MC_OP_MODULATE_X2            (0x4)
#define MC_OP_MODULATE_X4            (0x5)
#define MC_OP_ADD                    (0x6)
#define MC_OP_ADD_SIGNED             (0x7)
#define MC_OP_LIN_BLEND_ITER_ALPHA   (0x8)
#define MC_OP_LIN_BLEND_ALPHA_FACTOR (0xa)
#define MC_OP_LIN_BLEND_TEX0_ALPHA   (0x10)
#define MC_OP_LIN_BLEND_TEX1_ALPHA   (0x11)
#define MC_OP_LIN_BLEND_TEX0_COLOR   (0x12)
#define MC_OP_LIN_BLEND_TEX1_COLOR   (0x13)
#define MC_OP_SUBTRACT               (0x14)

/* GFXRENDERSTATE_MAP_PALETTE_LOAD, p128
 *
 * Format:
 *     0:  GFX_OP_MAP_PALETTE_LOAD
 *     1:  16bpp color[0]
 *     ...
 *     256: 16bpp color[255]
 */
#define GFX_OP_MAP_PALETTE_LOAD ((0x3<<29)|(0x1d<<24)|(0x82<<16)|0xff)

/* GFXRENDERSTATE_MAP_LOD_CONTROL, p127
 */
#define GFX_OP_MAP_LOD_CTL       ((0x3<<29)|(0x1c<<24)|(0x4<<19))
#define MLC_MAP_ID_SHIFT         16
#define MLC_MAP_0                (0<<16)
#define MLC_MAP_1                (1<<16)
#define MLC_UPDATE_DITHER_WEIGHT (1<<10)
#define MLC_DITHER_WEIGHT_MASK   (0x3<<8)
#define MLC_DITHER_WEIGHT_FULL   (0x0<<8)
#define MLC_DITHER_WEIGHT_50     (0x1<<8)
#define MLC_DITHER_WEIGHT_25     (0x2<<8)
#define MLC_DITHER_WEIGHT_12     (0x3<<8)
#define MLC_UPDATE_LOD_BIAS      (1<<7)
#define MLC_LOD_BIAS_MASK        ((1<<7)-1)

/* GFXRENDERSTATE_MAP_LOD_LIMITS, p126
 */
#define GFX_OP_MAP_LOD_LIMITS   ((0x3<<29)|(0x1c<<24)|(0x3<<19))
#define MLL_MAP_ID_SHIFT         16
#define MLL_MAP_0                (0<<16)
#define MLL_MAP_1                (1<<16)
#define MLL_UPDATE_MAX_MIP       (1<<13)
#define MLL_MAX_MIP_SHIFT        5
#define MLL_MAX_MIP_MASK         (0xff<<5)
#define MLL_MAX_MIP_ONE          (0x10<<5)
#define MLL_UPDATE_MIN_MIP       (1<<4)
#define MLL_MIN_MIP_SHIFT        0
#define MLL_MIN_MIP_MASK         (0xf<<0)

/* GFXRENDERSTATE_MAP_FILTER, p124
 */
#define GFX_OP_MAP_FILTER       ((0x3<<29)|(0x1c<<24)|(0x2<<19))
#define MF_MAP_ID_SHIFT         16
#define MF_MAP_0                (0<<16)
#define MF_MAP_1                (1<<16)
#define MF_UPDATE_ANISOTROPIC   (1<<12)
#define MF_ANISOTROPIC_MASK     (1<<10)
#define MF_ANISOTROPIC_ENABLE   (1<<10)
#define MF_UPDATE_MIP_FILTER    (1<<9)
#define MF_MIP_MASK             (0x3<<6)
#define MF_MIP_NONE             (0x0<<6)
#define MF_MIP_NEAREST          (0x1<<6)
#define MF_MIP_DITHER           (0x2<<6)
#define MF_MIP_LINEAR           (0x3<<6)
#define MF_UPDATE_MAG_FILTER    (1<<5)
#define MF_MAG_MASK             (1<<3)
#define MF_MAG_LINEAR           (1<<3)
#define MF_MAG_NEAREST          (0<<3)
#define MF_UPDATE_MIN_FILTER    (1<<2)
#define MF_MIN_MASK             (1<<0)
#define MF_MIN_LINEAR           (1<<0)
#define MF_MIN_NEAREST          (0<<0)

/* GFXRENDERSTATE_MAP_INFO, p118
 */
#define GFX_OP_MAP_INFO      ((0x3<<29)|(0x1d<<24)|0x2)
#define MI1_MAP_ID_SHIFT         28
#define MI1_MAP_0                (0<<28)
#define MI1_MAP_1                (1<<28)
#define MI1_FMT_MASK             (0x7<<24)
#define MI1_FMT_8CI              (0x0<<24)
#define MI1_FMT_8BPP             (0x1<<24)
#define MI1_FMT_16BPP            (0x2<<24)
#define MI1_FMT_422              (0x5<<24)
#define MI1_PF_MASK              (0x3<<21)
#define MI1_PF_8CI_RGB565         (0x0<<21)
#define MI1_PF_8CI_ARGB1555       (0x1<<21)
#define MI1_PF_8CI_ARGB4444       (0x2<<21)
#define MI1_PF_8CI_AY88           (0x3<<21)
#define MI1_PF_16BPP_RGB565       (0x0<<21)
#define MI1_PF_16BPP_ARGB1555     (0x1<<21)
#define MI1_PF_16BPP_ARGB4444     (0x2<<21)
#define MI1_PF_16BPP_AY88         (0x3<<21)
#define MI1_PF_422_YCRCB_SWAP_Y   (0x0<<21)
#define MI1_PF_422_YCRCB          (0x1<<21)
#define MI1_PF_422_YCRCB_SWAP_UV  (0x2<<21)
#define MI1_PF_422_YCRCB_SWAP_YUV (0x3<<21)
#define MI1_OUTPUT_CHANNEL_MASK   (0x3<<19)
#define MI1_COLOR_CONV_ENABLE     (1<<18)
#define MI1_VERT_STRIDE_MASK      (1<<17)
#define MI1_VERT_STRIDE_1         (1<<17)
#define MI1_VERT_OFFSET_MASK      (1<<16)
#define MI1_VERT_OFFSET_1         (1<<16)
#define MI1_ENABLE_FENCE_REGS     (1<<10)
#define MI1_TILED_SURFACE         (1<<9)
#define MI1_TILE_WALK_X           (0<<8)
#define MI1_TILE_WALK_Y           (1<<8)
#define MI1_PITCH_MASK            (0xf<<0)
#define MI2_DIMENSIONS_ARE_LOG2   (1<<31)
#define MI2_DIMENSIONS_ARE_EXACT  (0<<31)
#define MI2_HEIGHT_SHIFT          16
#define MI2_HEIGHT_MASK           (0x1ff<<16)
#define MI2_WIDTH_SHIFT           0
#define MI2_WIDTH_MASK            (0x1ff<<0)
#define MI3_BASE_ADDR_MASK        (~0xf)

/* GFXRENDERSTATE_MAP_COORD_SETS, p116
 */
#define GFX_OP_MAP_COORD_SETS ((0x3<<29)|(0x1c<<24)|(0x1<<19))
#define MCS_COORD_ID_SHIFT         16
#define MCS_COORD_0                (0<<16)
#define MCS_COORD_1                (1<<16)
#define MCS_UPDATE_NORMALIZED      (1<<15)
#define MCS_NORMALIZED_COORDS_MASK (1<<14)
#define MCS_NORMALIZED_COORDS      (1<<14)
#define MCS_UPDATE_V_STATE         (1<<7)
#define MCS_V_STATE_MASK           (0x3<<4)
#define MCS_V_WRAP                 (0x0<<4)
#define MCS_V_MIRROR               (0x1<<4)
#define MCS_V_CLAMP                (0x2<<4)
#define MCS_V_WRAP_SHORTEST        (0x3<<4)
#define MCS_UPDATE_U_STATE         (1<<3)
#define MCS_U_STATE_MASK           (0x3<<0)
#define MCS_U_WRAP                 (0x0<<0)
#define MCS_U_MIRROR               (0x1<<0)
#define MCS_U_CLAMP                (0x2<<0)
#define MCS_U_WRAP_SHORTEST        (0x3<<0)

/* GFXRENDERSTATE_MAP_TEXELS, p115
 */
#define GFX_OP_MAP_TEXELS   ((0x3<<29)|(0x1c<<24)|(0x0<<19))
#define MT_UPDATE_TEXEL1_STATE     (1<<15)
#define MT_TEXEL1_DISABLE          (0<<14)
#define MT_TEXEL1_ENABLE           (1<<14)
#define MT_TEXEL1_COORD0           (0<<11)
#define MT_TEXEL1_COORD1           (1<<11)
#define MT_TEXEL1_MAP0             (0<<8)
#define MT_TEXEL1_MAP1             (1<<8)
#define MT_UPDATE_TEXEL0_STATE     (1<<7)
#define MT_TEXEL0_DISABLE          (0<<6)
#define MT_TEXEL0_ENABLE           (1<<6)
#define MT_TEXEL0_COORD0           (0<<3)
#define MT_TEXEL0_COORD1           (1<<3)
#define MT_TEXEL0_MAP0             (0<<0)
#define MT_TEXEL0_MAP1             (1<<0)

/* GFXRENDERSTATE_VERTEX_FORMAT, p110
 */
#define GFX_OP_VERTEX_FMT  ((0x3<<29)|(0x5<<24))
#define VF_TEXCOORD_COUNT_SHIFT    8
#define VF_TEXCOORD_COUNT_0        (0<<8)
#define VF_TEXCOORD_COUNT_1        (1<<8)
#define VF_TEXCOORD_COUNT_2        (2<<8)
#define VF_SPEC_FOG_ENABLE         (1<<7)
#define VF_RGBA_ENABLE             (1<<6)
#define VF_Z_OFFSET_ENABLE         (1<<5)
#define VF_XYZ                     (0x1<<1)
#define VF_XYZW                    (0x2<<1)
#define VF_XY                      (0x3<<1)
#define VF_XYW                     (0x4<<1)


#define VERT_X_MASK       (~0xf)
#define VERT_X_EDGE_V2V0  (1<<2)
#define VERT_X_EDGE_V1V2  (1<<1)
#define VERT_X_EDGE_V0V1  (1<<0)

/* Not enabled fields should not be sent to hardware:
 */
typedef struct {
   union {
      float x;
      unsigned int edge_flags;
   } x;
   float y;
   float z;
   float z_bias;
   float oow;
   unsigned int argb;
   unsigned int fog_spec_rgb;	/* spec g and r ignored. */
   float tu0;
   float tv0;
   float tu1;
   float tv1;
} i810_full_vertex;



/* GFXCMDPARSER_BATCH_BUFFER, p105
 *
 * Not clear whether start address must be shifted or not.  Not clear
 * whether address is physical system memory, or subject to GTT
 * translation.  Because the address appears to be 32 bits long,
 * perhaps it refers to physical system memory...
 */
#define CMD_OP_BATCH_BUFFER  ((0x0<<29)|(0x30<<23)|0x1)
#define BB1_START_ADDR_MASK   (~0x7)
#define BB1_PROTECTED         (1<<0)
#define BB1_UNPROTECTED       (0<<0)
#define BB2_END_ADDR_MASK     (~0x7)

/* Hardware seems to barf on buffers larger than this (in strange ways)...
 */
#define MAX_BATCH (512*1024)


/* GFXCMDPARSER_Z_BUFFER_INFO, p98
 *
 * Base address is in GTT space, and must be 4K aligned
 */
#define CMD_OP_Z_BUFFER_INFO  ((0x0<<29)|(0x16<<23))
#define ZB_BASE_ADDR_SHIFT     0
#define ZB_BASE_ADDR_MASK     (~((1<<12)-1))
#define ZB_PITCH_512B         (0x0<<0)
#define ZB_PITCH_1K           (0x1<<0)
#define ZB_PITCH_2K           (0x2<<0)
#define ZB_PITCH_4K           (0x3<<0)

/* GFXCMDPARSER_FRONT_BUFFER_INFO, p97
 *
 * Format:
 *     0:  CMD_OP_FRONT_BUFFER_INFO | (pitch<<FB0_PITCH_SHIFT) | FB0_*
 *     1:  FB1_*
 */
#define CMD_OP_FRONT_BUFFER_INFO ((0x0<<29)|(0x14<<23))
#define FB0_PITCH_SHIFT           8
#define FB0_FLIP_SYNC            (0<<6)
#define FB0_FLIP_ASYNC           (1<<6)
#define FB0_BASE_ADDR_SHIFT       0
#define FB0_BASE_ADDR_MASK        0x03FFFFF8

/* GFXCMDPARSER_DEST_BUFFER_INFO, p96
 *
 * Format:
 */
#define CMD_OP_DESTBUFFER_INFO ((0x0<<29)|(0x15<<23))
#define DB1_BASE_ADDR_SHIFT       0
#define DB1_BASE_ADDR_MASK        0x03FFF000
#define DB1_PITCH_512B            (0x0<<0)
#define DB1_PITCH_1K              (0x1<<0)
#define DB1_PITCH_2K              (0x2<<0)
#define DB1_PITCH_4K              (0x4<<0)


/* GFXRENDERSTATE_DEST_BUFFER_VARIABLES, p152
 *
 * Format:
 *     0:  GFX_OP_DESTBUFFER_VARS
 *     1:  DEST_*
 */
#define GFX_OP_DESTBUFFER_VARS   ((0x3<<29)|(0x1d<<24)|(0x85<<16)|0x0)
#define DV_HORG_BIAS_MASK      (0xf<<20)
#define DV_HORG_BIAS_OGL       (0x0<<20)
#define DV_VORG_BIAS_MASK      (0xf<<16)
#define DV_VORG_BIAS_OGL       (0x0<<16)
#define DV_PF_MASK             (0x7<<8)
#define DV_PF_INDEX            (0x0<<8)
#define DV_PF_555           (0x1<<8)
#define DV_PF_565           (0x2<<8)

#define GFX_OP_ANTIALIAS         ((0x3<<29)|(0x6<<24))
#define AA_UPDATE_EDGEFLAG       (1<<13)
#define AA_ENABLE_EDGEFLAG       (1<<12)
#define AA_UPDATE_POLYWIDTH      (1<<11)
#define AA_POLYWIDTH_05          (1<<9)
#define AA_POLYWIDTH_10          (2<<9)
#define AA_POLYWIDTH_20          (3<<9)
#define AA_POLYWIDTH_40          (4<<9)
#define AA_UPDATE_LINEWIDTH      (1<<8)
#define AA_LINEWIDTH_05          (1<<6)
#define AA_LINEWIDTH_10          (2<<6)
#define AA_LINEWIDTH_20          (3<<6)
#define AA_LINEWIDTH_40          (4<<6)
#define AA_UPDATE_BB_EXPANSION   (1<<5)
#define AA_BB_EXPANSION_SHIFT    2
#define AA_UPDATE_AA_ENABLE      (1<<1)
#define AA_ENABLE                (1<<0)

#define GFX_OP_STIPPLE           ((0x3<<29)|(0x1d<<24)|(0x83<<16))
#define ST1_ENABLE               (1<<16)
#define ST1_MASK                 (0xffff)

#define I810_SET_FIELD( var, mask, value ) (var &= ~(mask), var |= value)

#endif
