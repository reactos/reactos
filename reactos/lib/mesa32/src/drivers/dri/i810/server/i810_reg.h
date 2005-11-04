/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810_reg.h,v 1.13 2003/02/06 04:18:04 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 *   based on the i740 driver by
 *        Kevin E. Martin <kevin@precisioninsight.com> 
 *   
 *
 */

#ifndef _I810_REG_H
#define _I810_REG_H

/* I/O register offsets
 */
#define SRX 0x3C4		/* p208 */
#define GRX 0x3CE		/* p213 */
#define ARX 0x3C0		/* p224 */

/* VGA Color Palette Registers */
#define DACMASK  0x3C6		/* p232 */
#define DACSTATE 0x3C7		/* p232 */
#define DACRX    0x3C7		/* p233 */
#define DACWX    0x3C8		/* p233 */
#define DACDATA  0x3C9		/* p233 */

/* CRT Controller Registers (CRX) */
#define START_ADDR_HI        0x0C /* p246 */
#define START_ADDR_LO        0x0D /* p247 */
#define VERT_SYNC_END        0x11 /* p249 */
#define EXT_VERT_TOTAL       0x30 /* p257 */
#define EXT_VERT_DISPLAY     0x31 /* p258 */
#define EXT_VERT_SYNC_START  0x32 /* p259 */
#define EXT_VERT_BLANK_START 0x33 /* p260 */
#define EXT_HORIZ_TOTAL      0x35 /* p261 */
#define EXT_HORIZ_BLANK      0x39 /* p261 */
#define EXT_START_ADDR       0x40 /* p262 */
#define EXT_START_ADDR_ENABLE    0x80 
#define EXT_OFFSET           0x41 /* p263 */
#define EXT_START_ADDR_HI    0x42 /* p263 */
#define INTERLACE_CNTL       0x70 /* p264 */
#define INTERLACE_ENABLE         0x80 
#define INTERLACE_DISABLE        0x00 

/* Miscellaneous Output Register 
 */
#define MSR_R          0x3CC	/* p207 */
#define MSR_W          0x3C2	/* p207 */
#define IO_ADDR_SELECT     0x01

#define MDA_BASE       0x3B0	/* p207 */
#define CGA_BASE       0x3D0	/* p207 */

/* CR80 - IO Control, p264
 */
#define IO_CTNL            0x80
#define EXTENDED_ATTR_CNTL     0x02
#define EXTENDED_CRTC_CNTL     0x01

/* GR10 - Address mapping, p221
 */
#define ADDRESS_MAPPING    0x10
#define PAGE_TO_LOCAL_MEM_ENABLE 0x10
#define GTT_MEM_MAP_ENABLE     0x08
#define PACKED_MODE_ENABLE     0x04
#define LINEAR_MODE_ENABLE     0x02
#define PAGE_MAPPING_ENABLE    0x01

/* Blitter control, p378
 */
#define BITBLT_CNTL        0x7000c
#define COLEXP_MODE            0x30
#define COLEXP_8BPP            0x00
#define COLEXP_16BPP           0x10
#define COLEXP_24BPP           0x20
#define COLEXP_RESERVED        0x30
#define BITBLT_STATUS          0x01

/* p375. 
 */
#define DISPLAY_CNTL       0x70008
#define VGA_WRAP_MODE          0x02
#define VGA_WRAP_AT_256KB      0x00
#define VGA_NO_WRAP            0x02
#define GUI_MODE               0x01
#define STANDARD_VGA_MODE      0x00
#define HIRES_MODE             0x01

/* p375
 */
#define PIXPIPE_CONFIG_0   0x70009
#define DAC_8_BIT              0x80
#define DAC_6_BIT              0x00
#define HW_CURSOR_ENABLE       0x10
#define EXTENDED_PALETTE       0x01

/* p375
 */
#define PIXPIPE_CONFIG_1   0x7000a
#define DISPLAY_COLOR_MODE     0x0F
#define DISPLAY_VGA_MODE       0x00
#define DISPLAY_8BPP_MODE      0x02
#define DISPLAY_15BPP_MODE     0x04
#define DISPLAY_16BPP_MODE     0x05
#define DISPLAY_24BPP_MODE     0x06
#define DISPLAY_32BPP_MODE     0x07

/* p375
 */
#define PIXPIPE_CONFIG_2   0x7000b
#define DISPLAY_GAMMA_ENABLE   0x08
#define DISPLAY_GAMMA_DISABLE  0x00
#define OVERLAY_GAMMA_ENABLE   0x04
#define OVERLAY_GAMMA_DISABLE  0x00


/* p380
 */
#define DISPLAY_BASE       0x70020
#define DISPLAY_BASE_MASK  0x03fffffc


/* Cursor control registers, pp383-384
 */
/* Desktop (845G, 865G) */
#define CURSOR_CONTROL     0x70080
#define CURSOR_ENABLE          0x80000000
#define CURSOR_GAMMA_ENABLE    0x40000000
#define CURSOR_STRIDE_MASK     0x30000000
#define CURSOR_FORMAT_SHIFT    24
#define CURSOR_FORMAT_MASK     (0x07 << CURSOR_FORMAT_SHIFT)
#define CURSOR_FORMAT_2C       (0x00 << CURSOR_FORMAT_SHIFT)
#define CURSOR_FORMAT_3C       (0x01 << CURSOR_FORMAT_SHIFT)
#define CURSOR_FORMAT_4C       (0x02 << CURSOR_FORMAT_SHIFT)
#define CURSOR_FORMAT_ARGB     (0x04 << CURSOR_FORMAT_SHIFT)
#define CURSOR_FORMAT_XRGB     (0x05 << CURSOR_FORMAT_SHIFT)

/* Mobile and i810 */
#define CURSOR_A_CONTROL   CURSOR_CONTROL
#define CURSOR_ORIGIN_SCREEN   0x00	/* i810 only */
#define CURSOR_ORIGIN_DISPLAY  0x1	/* i810 only */
#define CURSOR_MODE            0x27
#define CURSOR_MODE_DISABLE    0x00
#define CURSOR_MODE_32_4C_AX   0x01	/* i810 only */
#define CURSOR_MODE_64_3C      0x04
#define CURSOR_MODE_64_4C_AX   0x05
#define CURSOR_MODE_64_4C      0x06
#define CURSOR_MODE_64_32B_AX  0x07
#define CURSOR_MODE_64_ARGB_AX (0x20 | CURSOR_MODE_64_32B_AX)
#define MCURSOR_PIPE_SELECT    (1 << 28)
#define MCURSOR_PIPE_A         0x00
#define MCURSOR_PIPE_B         (1 << 28)
#define MCURSOR_GAMMA_ENABLE   (1 << 26)
#define MCURSOR_MEM_TYPE_LOCAL (1 << 25)


#define CURSOR_BASEADDR    0x70084
#define CURSOR_A_BASE      CURSOR_BASEADDR
#define CURSOR_BASEADDR_MASK 0x1FFFFF00
#define CURSOR_A_POSITION  0x70088
#define CURSOR_POS_SIGN        0x8000
#define CURSOR_POS_MASK        0x007FF
#define CURSOR_X_SHIFT	       0
#define CURSOR_Y_SHIFT         16
#define CURSOR_X_LO        0x70088
#define CURSOR_X_HI        0x70089
#define CURSOR_X_POS           0x00
#define CURSOR_X_NEG           0x80
#define CURSOR_Y_LO        0x7008A
#define CURSOR_Y_HI        0x7008B
#define CURSOR_Y_POS           0x00
#define CURSOR_Y_NEG           0x80

#define CURSOR_A_PALETTE0  0x70090
#define CURSOR_A_PALETTE1  0x70094
#define CURSOR_A_PALETTE2  0x70098
#define CURSOR_A_PALETTE3  0x7009C

#define CURSOR_SIZE	   0x700A0
#define CURSOR_SIZE_MASK       0x3FF
#define CURSOR_SIZE_HSHIFT     0
#define CURSOR_SIZE_VSHIFT     12


/* Similar registers exist in Device 0 on the i810 (pp55-65), but I'm
 * not sure they refer to local (graphics) memory.
 *
 * These details are for the local memory control registers,
 * (pp301-310).  The test machines are not equiped with local memory,
 * so nothing is tested.  Only a single row seems to be supported.
 */
#define DRAM_ROW_TYPE      0x3000
#define DRAM_ROW_0             0x01
#define DRAM_ROW_0_SDRAM       0x01
#define DRAM_ROW_0_EMPTY       0x00
#define DRAM_ROW_CNTL_LO   0x3001
#define DRAM_PAGE_MODE_CTRL    0x10
#define DRAM_RAS_TO_CAS_OVRIDE 0x08
#define DRAM_CAS_LATENCY       0x04
#define DRAM_RAS_TIMING        0x02
#define DRAM_RAS_PRECHARGE     0x01
#define DRAM_ROW_CNTL_HI   0x3002
#define DRAM_REFRESH_RATE      0x18
#define DRAM_REFRESH_DISABLE   0x00
#define DRAM_REFRESH_60HZ      0x08
#define DRAM_REFRESH_FAST_TEST 0x10
#define DRAM_REFRESH_RESERVED  0x18
#define DRAM_SMS               0x07
#define DRAM_SMS_NORMAL        0x00
#define DRAM_SMS_NOP_ENABLE    0x01
#define DRAM_SMS_ABPCE         0x02
#define DRAM_SMS_MRCE          0x03
#define DRAM_SMS_CBRCE         0x04

/* p307
 */
#define DPMS_SYNC_SELECT   0x5002
#define VSYNC_CNTL             0x08
#define VSYNC_ON               0x00
#define VSYNC_OFF              0x08
#define HSYNC_CNTL             0x02
#define HSYNC_ON               0x00
#define HSYNC_OFF              0x02



/* p317, 319
 */
#define VCLK2_VCO_M        0x6008 /* treat as 16 bit? (includes msbs) */
#define VCLK2_VCO_N        0x600a
#define VCLK2_VCO_DIV_SEL  0x6012

#define VCLK_DIVISOR_VGA0   0x6000
#define VCLK_DIVISOR_VGA1   0x6004
#define VCLK_POST_DIV	    0x6010

#define POST_DIV_SELECT        0x70
#define POST_DIV_1             0x00
#define POST_DIV_2             0x10
#define POST_DIV_4             0x20
#define POST_DIV_8             0x30
#define POST_DIV_16            0x40
#define POST_DIV_32            0x50
#define VCO_LOOP_DIV_BY_4M     0x00
#define VCO_LOOP_DIV_BY_16M    0x04


/* Instruction Parser Mode Register 
 *    - p281
 *    - 2 new bits.
 */
#define INST_PM                  0x20c0	
#define AGP_SYNC_PACKET_FLUSH_ENABLE 0x20 /* reserved */
#define SYNC_PACKET_FLUSH_ENABLE     0x10
#define TWO_D_INST_DISABLE           0x08
#define THREE_D_INST_DISABLE         0x04
#define STATE_VAR_UPDATE_DISABLE     0x02
#define PAL_STIP_DISABLE             0x01

#define INST_DONE                0x2090
#define INST_PS                  0x20c4

#define MEMMODE                  0x20dc


/* Instruction parser error register.  p279
 */
#define IPEIR                  0x2088
#define IPEHR                  0x208C


/* General error reporting regs, p296
 */
#define EIR               0x20B0
#define EMR               0x20B4
#define ESR               0x20B8
#define IP_ERR                    0x0001
#define ERROR_RESERVED            0xffc6


/* Interrupt Control Registers 
 *   - new bits for i810
 *   - new register hwstam (mask)
 */
#define HWSTAM               0x2098 /* p290 */
#define IER                  0x20a0 /* p291 */
#define IIR                  0x20a4 /* p292 */
#define IMR                  0x20a8 /* p293 */
#define ISR                  0x20ac /* p294 */
#define HW_ERROR                 0x8000
#define SYNC_STATUS_TOGGLE       0x1000
#define DPY_0_FLIP_PENDING       0x0800
#define DPY_1_FLIP_PENDING       0x0400	/* not implemented on i810 */
#define OVL_0_FLIP_PENDING       0x0200
#define OVL_1_FLIP_PENDING       0x0100	/* not implemented on i810 */
#define DPY_0_VBLANK             0x0080
#define DPY_0_EVENT              0x0040
#define DPY_1_VBLANK             0x0020	/* not implemented on i810 */
#define DPY_1_EVENT              0x0010	/* not implemented on i810 */
#define HOST_PORT_EVENT          0x0008	/*  */
#define CAPTURE_EVENT            0x0004	/*  */
#define USER_DEFINED             0x0002
#define BREAKPOINT               0x0001


#define INTR_RESERVED            (0x6000 | 		\
				  DPY_1_FLIP_PENDING |	\
				  OVL_1_FLIP_PENDING |	\
				  DPY_1_VBLANK |	\
				  DPY_1_EVENT |		\
				  HOST_PORT_EVENT |	\
				  CAPTURE_EVENT )

/* FIFO Watermark and Burst Length Control Register 
 *
 * - different offset and contents on i810 (p299) (fewer bits per field)
 * - some overlay fields added
 * - what does it all mean?
 */
#define FWATER_BLC       0x20d8
#define FWATER_BLC2	 0x20dc
#define MM_BURST_LENGTH     0x00700000
#define MM_FIFO_WATERMARK   0x0001F000
#define LM_BURST_LENGTH     0x00000700
#define LM_FIFO_WATERMARK   0x0000001F


/* Fence/Tiling ranges [0..7]
 */
#define FENCE            0x2000
#define FENCE_NR         8

#define I830_FENCE_START_MASK	0x07f80000

#define FENCE_START_MASK    0x03F80000
#define FENCE_X_MAJOR       0x00000000
#define FENCE_Y_MAJOR       0x00001000
#define FENCE_SIZE_MASK     0x00000700
#define FENCE_SIZE_512K     0x00000000
#define FENCE_SIZE_1M       0x00000100
#define FENCE_SIZE_2M       0x00000200
#define FENCE_SIZE_4M       0x00000300
#define FENCE_SIZE_8M       0x00000400
#define FENCE_SIZE_16M      0x00000500
#define FENCE_SIZE_32M      0x00000600
#define FENCE_SIZE_64M	    0x00000700
#define FENCE_PITCH_MASK    0x00000070
#define FENCE_PITCH_1       0x00000000
#define FENCE_PITCH_2       0x00000010
#define FENCE_PITCH_4       0x00000020
#define FENCE_PITCH_8       0x00000030
#define FENCE_PITCH_16      0x00000040
#define FENCE_PITCH_32      0x00000050
#define FENCE_PITCH_64	    0x00000060
#define FENCE_VALID         0x00000001


/* Registers to control page table, p274
 */
#define PGETBL_CTL       0x2020
#define PGETBL_ADDR_MASK    0xFFFFF000
#define PGETBL_ENABLE_MASK  0x00000001
#define PGETBL_ENABLED      0x00000001

/* Register containing pge table error results, p276
 */
#define PGE_ERR          0x2024
#define PGE_ERR_ADDR_MASK   0xFFFFF000
#define PGE_ERR_ID_MASK     0x00000038
#define PGE_ERR_CAPTURE     0x00000000
#define PGE_ERR_OVERLAY     0x00000008
#define PGE_ERR_DISPLAY     0x00000010
#define PGE_ERR_HOST        0x00000018
#define PGE_ERR_RENDER      0x00000020
#define PGE_ERR_BLITTER     0x00000028
#define PGE_ERR_MAPPING     0x00000030
#define PGE_ERR_CMD_PARSER  0x00000038
#define PGE_ERR_TYPE_MASK   0x00000007
#define PGE_ERR_INV_TABLE   0x00000000
#define PGE_ERR_INV_PTE     0x00000001
#define PGE_ERR_MIXED_TYPES 0x00000002
#define PGE_ERR_PAGE_MISS   0x00000003
#define PGE_ERR_ILLEGAL_TRX 0x00000004
#define PGE_ERR_LOCAL_MEM   0x00000005
#define PGE_ERR_TILED       0x00000006



/* Page table entries loaded via mmio region, p323
 */
#define PTE_BASE         0x10000
#define PTE_ADDR_MASK       0x3FFFF000
#define PTE_TYPE_MASK       0x00000006
#define PTE_LOCAL           0x00000002
#define PTE_MAIN_UNCACHED   0x00000000
#define PTE_MAIN_CACHED     0x00000006
#define PTE_VALID_MASK      0x00000001
#define PTE_VALID           0x00000001


/* Ring buffer registers, p277, overview p19
 */
#define LP_RING     0x2030
#define HP_RING     0x2040

#define RING_TAIL      0x00
#define TAIL_ADDR           0x000FFFF8
#define I830_TAIL_MASK	    0x001FFFF8

#define RING_HEAD      0x04
#define HEAD_WRAP_COUNT     0xFFE00000
#define HEAD_WRAP_ONE       0x00200000
#define HEAD_ADDR           0x001FFFFC
#define I830_HEAD_MASK      0x001FFFFC

#define RING_START     0x08
#define START_ADDR          0x00FFFFF8
#define I830_RING_START_MASK	0xFFFFF000

#define RING_LEN       0x0C
#define RING_NR_PAGES       0x000FF000 
#define I830_RING_NR_PAGES	0x001FF000
#define RING_REPORT_MASK    0x00000006
#define RING_REPORT_64K     0x00000002
#define RING_REPORT_128K    0x00000004
#define RING_NO_REPORT      0x00000000
#define RING_VALID_MASK     0x00000001
#define RING_VALID          0x00000001
#define RING_INVALID        0x00000000



/* BitBlt Instructions
 *
 * There are many more masks & ranges yet to add.
 */
#define BR00_BITBLT_CLIENT   0x40000000
#define BR00_OP_COLOR_BLT    0x10000000
#define BR00_OP_SRC_COPY_BLT 0x10C00000
#define BR00_OP_FULL_BLT     0x11400000
#define BR00_OP_MONO_SRC_BLT 0x11800000
#define BR00_OP_MONO_SRC_COPY_BLT 0x11000000
#define BR00_OP_MONO_PAT_BLT 0x11C00000
#define BR00_OP_MONO_SRC_COPY_IMMEDIATE_BLT (0x61 << 22)
#define BR00_OP_TEXT_IMMEDIATE_BLT 0xc000000


#define BR00_TPCY_DISABLE    0x00000000
#define BR00_TPCY_ENABLE     0x00000010

#define BR00_TPCY_ROP        0x00000000
#define BR00_TPCY_NO_ROP     0x00000020
#define BR00_TPCY_EQ         0x00000000
#define BR00_TPCY_NOT_EQ     0x00000040

#define BR00_PAT_MSB_FIRST   0x00000000	/* ? */

#define BR00_PAT_VERT_ALIGN  0x000000e0

#define BR00_LENGTH          0x0000000F

#define BR09_DEST_ADDR       0x03FFFFFF

#define BR11_SOURCE_PITCH    0x00003FFF

#define BR12_SOURCE_ADDR     0x03FFFFFF

#define BR13_SOLID_PATTERN   0x80000000
#define BR13_RIGHT_TO_LEFT   0x40000000
#define BR13_LEFT_TO_RIGHT   0x00000000
#define BR13_MONO_TRANSPCY   0x20000000
#define BR13_USE_DYN_DEPTH   0x04000000
#define BR13_DYN_8BPP        0x00000000
#define BR13_DYN_16BPP       0x01000000
#define BR13_DYN_24BPP       0x02000000
#define BR13_ROP_MASK        0x00FF0000
#define BR13_DEST_PITCH      0x0000FFFF
#define BR13_PITCH_SIGN_BIT  0x00008000

#define BR14_DEST_HEIGHT     0xFFFF0000
#define BR14_DEST_WIDTH      0x0000FFFF

#define BR15_PATTERN_ADDR    0x03FFFFFF

#define BR16_SOLID_PAT_COLOR 0x00FFFFFF
#define BR16_BACKGND_PAT_CLR 0x00FFFFFF

#define BR17_FGND_PAT_CLR    0x00FFFFFF

#define BR18_SRC_BGND_CLR    0x00FFFFFF
#define BR19_SRC_FGND_CLR    0x00FFFFFF


/* Instruction parser instructions
 */

#define INST_PARSER_CLIENT   0x00000000
#define INST_OP_FLUSH        0x02000000
#define INST_FLUSH_MAP_CACHE 0x00000001


#define GFX_OP_USER_INTERRUPT ((0<<29)|(2<<23))


/* Registers in the i810 host-pci bridge pci config space which affect
 * the i810 graphics operations.  
 */
#define SMRAM_MISCC         0x70
#define GMS                    0x000000c0
#define GMS_DISABLE            0x00000000
#define GMS_ENABLE_BARE        0x00000040
#define GMS_ENABLE_512K        0x00000080
#define GMS_ENABLE_1M          0x000000c0
#define USMM                   0x00000030 
#define USMM_DISABLE           0x00000000
#define USMM_TSEG_ZERO         0x00000010
#define USMM_TSEG_512K         0x00000020
#define USMM_TSEG_1M           0x00000030  
#define GFX_MEM_WIN_SIZE       0x00010000
#define GFX_MEM_WIN_32M        0x00010000
#define GFX_MEM_WIN_64M        0x00000000

/* Overkill?  I don't know.  Need to figure out top of mem to make the
 * SMRAM calculations come out.  Linux seems to have problems
 * detecting it all on its own, so this seems a reasonable double
 * check to any user supplied 'mem=...' boot param.
 *
 * ... unfortunately this reg doesn't work according to spec on the
 * test hardware.
 */
#define WHTCFG_PAMR_DRP      0x50
#define SYS_DRAM_ROW_0_SHIFT    16
#define SYS_DRAM_ROW_1_SHIFT    20
#define DRAM_MASK           0x0f
#define DRAM_VALUE_0        0
#define DRAM_VALUE_1        8
/* No 2 value defined */
#define DRAM_VALUE_3        16
#define DRAM_VALUE_4        16
#define DRAM_VALUE_5        24
#define DRAM_VALUE_6        32
#define DRAM_VALUE_7        32
#define DRAM_VALUE_8        48
#define DRAM_VALUE_9        64
#define DRAM_VALUE_A        64
#define DRAM_VALUE_B        96
#define DRAM_VALUE_C        128
#define DRAM_VALUE_D        128
#define DRAM_VALUE_E        192
#define DRAM_VALUE_F        256	/* nice one, geezer */
#define LM_FREQ_MASK        0x10
#define LM_FREQ_133         0x10
#define LM_FREQ_100         0x00




/* These are 3d state registers, but the state is invarient, so we let
 * the X server handle it:
 */



/* GFXRENDERSTATE_COLOR_CHROMA_KEY, p135
 */
#define GFX_OP_COLOR_CHROMA_KEY  ((0x3<<29)|(0x1d<<24)|(0x2<<16)|0x1)
#define CC1_UPDATE_KILL_WRITE    (1<<28)
#define CC1_ENABLE_KILL_WRITE    (1<<27)
#define CC1_DISABLE_KILL_WRITE    0
#define CC1_UPDATE_COLOR_IDX     (1<<26)
#define CC1_UPDATE_CHROMA_LOW    (1<<25)
#define CC1_UPDATE_CHROMA_HI     (1<<24)
#define CC1_CHROMA_LOW_MASK      ((1<<24)-1)
#define CC2_COLOR_IDX_SHIFT      24
#define CC2_COLOR_IDX_MASK       (0xff<<24)
#define CC2_CHROMA_HI_MASK       ((1<<24)-1)


#define GFX_CMD_CONTEXT_SEL      ((0<<29)|(0x5<<23))
#define CS_UPDATE_LOAD           (1<<17)
#define CS_UPDATE_USE            (1<<16)
#define CS_UPDATE_LOAD           (1<<17)
#define CS_LOAD_CTX0             0
#define CS_LOAD_CTX1             (1<<8)
#define CS_USE_CTX0              0
#define CS_USE_CTX1              (1<<0)

/* I810 LCD/TV registers */
#define LCD_TV_HTOTAL	0x60000
#define LCD_TV_C	0x60018
#define LCD_TV_OVRACT   0x6001C

#define LCD_TV_ENABLE (1 << 31)
#define LCD_TV_VGAMOD (1 << 28)

/* I830 CRTC registers */
#define HTOTAL_A	0x60000
#define HBLANK_A	0x60004
#define HSYNC_A 	0x60008
#define VTOTAL_A	0x6000c
#define VBLANK_A	0x60010
#define VSYNC_A 	0x60014
#define PIPEASRC	0x6001c
#define BCLRPAT_A	0x60020

#define HTOTAL_B	0x61000
#define HBLANK_B	0x61004
#define HSYNC_B 	0x61008
#define VTOTAL_B	0x6100c
#define VBLANK_B	0x61010
#define VSYNC_B 	0x61014
#define PIPEBSRC	0x6101c
#define BCLRPAT_B	0x61020

#define DPLL_A		0x06014
#define DPLL_B		0x06018
#define FPA0		0x06040
#define FPA1		0x06044

#define I830_HTOTAL_MASK 	0xfff0000
#define I830_HACTIVE_MASK	0x7ff

#define I830_HBLANKEND_MASK	0xfff0000
#define I830_HBLANKSTART_MASK    0xfff

#define I830_HSYNCEND_MASK	0xfff0000
#define I830_HSYNCSTART_MASK    0xfff

#define I830_VTOTAL_MASK 	0xfff0000
#define I830_VACTIVE_MASK	0x7ff

#define I830_VBLANKEND_MASK	0xfff0000
#define I830_VBLANKSTART_MASK    0xfff

#define I830_VSYNCEND_MASK	0xfff0000
#define I830_VSYNCSTART_MASK    0xfff

#define I830_PIPEA_HORZ_MASK	0x7ff0000
#define I830_PIPEA_VERT_MASK	0x7ff

#define ADPA			0x61100
#define ADPA_DAC_ENABLE 	(1<<31)
#define ADPA_DAC_DISABLE	0
#define ADPA_PIPE_SELECT_MASK	(1<<30)
#define ADPA_PIPE_A_SELECT	0
#define ADPA_PIPE_B_SELECT	(1<<30)
#define ADPA_USE_VGA_HVPOLARITY (1<<15)
#define ADPA_SETS_HVPOLARITY	0
#define ADPA_VSYNC_CNTL_DISABLE (1<<11)
#define ADPA_VSYNC_CNTL_ENABLE	0
#define ADPA_HSYNC_CNTL_DISABLE (1<<10)
#define ADPA_HSYNC_CNTL_ENABLE	0
#define ADPA_VSYNC_ACTIVE_HIGH	(1<<4)
#define ADPA_VSYNC_ACTIVE_LOW	0
#define ADPA_HSYNC_ACTIVE_HIGH	(1<<3)
#define ADPA_HSYNC_ACTIVE_LOW	0


#define DVOA			0x61120
#define DVOB			0x61140
#define DVOC			0x61160
#define DVO_ENABLE		(1<<31)

#define DVOA_SRCDIM		0x61124
#define DVOB_SRCDIM		0x61144
#define DVOC_SRCDIM		0x61164

#define LVDS			0x61180

#define PIPEACONF 0x70008
#define PIPEACONF_ENABLE	(1<<31)
#define PIPEACONF_DISABLE	0
#define PIPEACONF_DOUBLE_WIDE	(1<<30)
#define PIPEACONF_SINGLE_WIDE	0
#define PIPEACONF_PIPE_UNLOCKED 0
#define PIPEACONF_PIPE_LOCKED	(1<<25)
#define PIPEACONF_PALETTE	0
#define PIPEACONF_GAMMA 	(1<<24)

#define PIPEBCONF 0x71008
#define PIPEBCONF_ENABLE	(1<<31)
#define PIPEBCONF_DISABLE	0
#define PIPEBCONF_GAMMA 	(1<<24)
#define PIPEBCONF_PALETTE	0

#define DSPACNTR		0x70180
#define DSPBCNTR		0x71180
#define DISPLAY_PLANE_ENABLE 			(1<<31)
#define DISPLAY_PLANE_DISABLE			0
#define DISPPLANE_GAMMA_ENABLE			(1<<30)
#define DISPPLANE_GAMMA_DISABLE			0
#define DISPPLANE_PIXFORMAT_MASK		(0xf<<26)
#define DISPPLANE_8BPP				(0x2<<26)
#define DISPPLANE_15_16BPP			(0x4<<26)
#define DISPPLANE_16BPP				(0x5<<26)
#define DISPPLANE_32BPP_NO_ALPHA 		(0x6<<26)
#define DISPPLANE_32BPP				(0x7<<26)
#define DISPPLANE_STEREO_ENABLE			(1<<25)
#define DISPPLANE_STEREO_DISABLE		0
#define DISPPLANE_SEL_PIPE_MASK			(1<<24)
#define DISPPLANE_SEL_PIPE_A			0
#define DISPPLANE_SEL_PIPE_B			(1<<24)
#define DISPPLANE_SRC_KEY_ENABLE		(1<<22)
#define DISPPLANE_SRC_KEY_DISABLE		0
#define DISPPLANE_LINE_DOUBLE			(1<<20)
#define DISPPLANE_NO_LINE_DOUBLE		0
#define DISPPLANE_STEREO_POLARITY_FIRST		0
#define DISPPLANE_STEREO_POLARITY_SECOND	(1<<18)
/* plane B only */
#define DISPPLANE_ALPHA_TRANS_ENABLE		(1<<15)
#define DISPPLANE_ALPHA_TRANS_DISABLE		0
#define DISPPLANE_SPRITE_ABOVE_DISPLAYA		0
#define DISPPLANE_SPRITE_ABOVE_OVERLAY		(1)

#define DSPABASE		0x70184
#define DSPASTRIDE		0x70188

#define DSPBBASE		0x71184
#define DSPBADDR		DSPBBASE
#define DSPBSTRIDE		0x71188

/* Various masks for reserved bits, etc. */
#define I830_FWATER1_MASK        (~((1<<11)|(1<<10)|(1<<9)|      \
        (1<<8)|(1<<26)|(1<<25)|(1<<24)|(1<<5)|(1<<4)|(1<<3)|    \
        (1<<2)|(1<<1)|1|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16)))
#define I830_FWATER2_MASK ~(0)

#define DV0A_RESERVED ((1<<26)|(1<<25)|(1<<24)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<16)|(1<<5)|(1<<1)|1)
#define DV0B_RESERVED ((1<<27)|(1<<26)|(1<<25)|(1<<24)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<16)|(1<<5)|(1<<1)|1)
#define VGA0_N_DIVISOR_MASK     ((1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16))
#define VGA0_M1_DIVISOR_MASK    ((1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9)|(1<<8))
#define VGA0_M2_DIVISOR_MASK    ((1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|1)
#define VGA0_M1M2N_RESERVED	~(VGA0_N_DIVISOR_MASK|VGA0_M1_DIVISOR_MASK|VGA0_M2_DIVISOR_MASK)
#define VGA0_POSTDIV_MASK       ((1<<7)|(1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|1)
#define VGA1_POSTDIV_MASK       ((1<<15)|(1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9)|(1<<8))
#define VGA_POSTDIV_RESERVED	~(VGA0_POSTDIV_MASK|VGA1_POSTDIV_MASK|(1<<7)|(1<<15))
#define DPLLA_POSTDIV_MASK ((1<<23)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16))
#define DPLLA_RESERVED     ((1<<27)|(1<<26)|(1<<25)|(1<<24)|(1<<22)|(1<<15)|(1<<12)|(1<<11)|(1<<10)|(1<<9)|(1<<8)|(1<<7)|(1<<6)|(1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|1)
#define ADPA_RESERVED	((1<<2)|(1<<1)|1|(1<<9)|(1<<8)|(1<<7)|(1<<6)|(1<<5)|(1<<30)|(1<<29)|(1<<28)|(1<<27)|(1<<26)|(1<<25)|(1<<24)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16))
#define SUPER_WORD              32
#define BURST_A_MASK    ((1<<11)|(1<<10)|(1<<9)|(1<<8))
#define BURST_B_MASK    ((1<<26)|(1<<25)|(1<<24))
#define WATER_A_MASK    ((1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|1)
#define WATER_B_MASK    ((1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16))
#define WATER_RESERVED	((1<<31)|(1<<30)|(1<<29)|(1<<28)|(1<<27)|(1<<23)|(1<<22)|(1<<21)|(1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<7)|(1<<6))
#define PIPEACONF_RESERVED ((1<<29)|(1<<28)|(1<<27)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16)|0xffff)
#define PIPEBCONF_RESERVED ((1<<30)|(1<<29)|(1<<28)|(1<<27)|(1<<26)|(1<<25)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16)|0xffff)
#define DSPACNTR_RESERVED ((1<<23)|(1<<19)|(1<<17)|(1<<16)|0xffff)
#define DSPBCNTR_RESERVED ((1<<23)|(1<<19)|(1<<17)|(1<<16)|0x7ffe)

#define I830_GMCH_CTRL		0x52

#define I830_GMCH_ENABLED	0x4
#define I830_GMCH_MEM_MASK	0x1
#define I830_GMCH_MEM_64M	0x1
#define I830_GMCH_MEM_128M	0

#define I830_GMCH_GMS_MASK			0x70
#define I830_GMCH_GMS_DISABLED		0x00
#define I830_GMCH_GMS_LOCAL			0x10
#define I830_GMCH_GMS_STOLEN_512	0x20
#define I830_GMCH_GMS_STOLEN_1024	0x30
#define I830_GMCH_GMS_STOLEN_8192	0x40

#define I830_RDRAM_CHANNEL_TYPE		0x03010
#define I830_RDRAM_ND(x)			(((x) & 0x20) >> 5)
#define I830_RDRAM_DDT(x)			(((x) & 0x18) >> 3)

#define I855_GMCH_GMS_MASK			(0x7 << 4)
#define I855_GMCH_GMS_DISABLED			0x00
#define I855_GMCH_GMS_STOLEN_1M			(0x1 << 4)
#define I855_GMCH_GMS_STOLEN_4M			(0x2 << 4)
#define I855_GMCH_GMS_STOLEN_8M			(0x3 << 4)
#define I855_GMCH_GMS_STOLEN_16M		(0x4 << 4)
#define I855_GMCH_GMS_STOLEN_32M		(0x5 << 4)

#define I85X_CAPID			0x44
#define I85X_VARIANT_MASK			0x7
#define I85X_VARIANT_SHIFT			5
#define I855_GME				0x0
#define I855_GM					0x4
#define I852_GME				0x2
#define I852_GM					0x5

/* BLT commands */
#define COLOR_BLT_CMD		((2<<29)|(0x40<<22)|(0x3))
#define COLOR_BLT_WRITE_ALPHA	(1<<21)
#define COLOR_BLT_WRITE_RGB	(1<<20)

#define XY_COLOR_BLT_CMD		((2<<29)|(0x50<<22)|(0x4))
#define XY_COLOR_BLT_WRITE_ALPHA	(1<<21)
#define XY_COLOR_BLT_WRITE_RGB		(1<<20)

#define XY_SETUP_CLIP_BLT_CMD		((2<<29)|(3<<22)|1)

#define XY_SRC_COPY_BLT_CMD		((2<<29)|(0x53<<22)|6)
#define XY_SRC_COPY_BLT_WRITE_ALPHA	(1<<21)
#define XY_SRC_COPY_BLT_WRITE_RGB	(1<<20)

#define SRC_COPY_BLT_CMD		((2<<29)|(0x43<<22)|0x4)
#define SRC_COPY_BLT_WRITE_ALPHA	(1<<21)
#define SRC_COPY_BLT_WRITE_RGB		(1<<20)

#define XY_MONO_PAT_BLT_CMD		((0x2<<29)|(0x52<<22)|0x7)
#define XY_MONO_PAT_VERT_SEED		((1<<10)|(1<<9)|(1<<8))
#define XY_MONO_PAT_HORT_SEED		((1<<14)|(1<<13)|(1<<12))
#define XY_MONO_PAT_BLT_WRITE_ALPHA	(1<<21)
#define XY_MONO_PAT_BLT_WRITE_RGB	(1<<20)

#define XY_MONO_SRC_BLT_CMD		((0x2<<29)|(0x54<<22)|(0x6))
#define XY_MONO_SRC_BLT_WRITE_ALPHA	(1<<21)
#define XY_MONO_SRC_BLT_WRITE_RGB	(1<<20)

/* 3d state */
#define STATE3D_FOG_MODE		((3<<29)|(0x1d<<24)|(0x89<<16)|2)
#define FOG_MODE_VERTEX 		(1<<31)
#define STATE3D_MAP_COORD_TRANSFORM	((3<<29)|(0x1d<<24)|(0x8c<<16))
#define DISABLE_TEX_TRANSFORM		(1<<28)
#define TEXTURE_SET(x)			(x<<29)
#define STATE3D_RASTERIZATION_RULES	((3<<29)|(0x07<<24))
#define POINT_RASTER_ENABLE		(1<<15)
#define POINT_RASTER_OGL		(1<<13)
#define STATE3D_VERTEX_TRANSFORM	((3<<29)|(0x1d<<24)|(0x8b<<16))
#define DISABLE_VIEWPORT_TRANSFORM	(1<<31)
#define DISABLE_PERSPECTIVE_DIVIDE	(1<<29)

#define MI_SET_CONTEXT			(0x18<<23)
#define CTXT_NO_RESTORE 		(1)
#define CTXT_PALETTE_SAVE_DISABLE	(1<<3)
#define CTXT_PALETTE_RESTORE_DISABLE	(1<<2)

/* Dword 0 */
#define MI_VERTEX_BUFFER		(0x17<<23)
#define MI_VERTEX_BUFFER_IDX(x) 	(x<<20)
#define MI_VERTEX_BUFFER_PITCH(x)	(x<<13)
#define MI_VERTEX_BUFFER_WIDTH(x)	(x<<6)
/* Dword 1 */
#define MI_VERTEX_BUFFER_DISABLE	(1)

/* Overlay Flip */
#define MI_OVERLAY_FLIP			(0x11<<23)
#define MI_OVERLAY_FLIP_CONTINUE	(0<<21)
#define MI_OVERLAY_FLIP_ON		(1<<21)
#define MI_OVERLAY_FLIP_OFF		(2<<21)

/* Wait for Events */
#define MI_WAIT_FOR_EVENT		(0x03<<23)
#define MI_WAIT_FOR_OVERLAY_FLIP	(1<<16)

/* Flush */
#define MI_FLUSH			(0x04<<23)
#define MI_WRITE_DIRTY_STATE		(1<<4)
#define MI_END_SCENE			(1<<3)
#define MI_INHIBIT_RENDER_CACHE_FLUSH	(1<<2)
#define MI_INVALIDATE_MAP_CACHE		(1<<0)

/* Noop */
#define MI_NOOP				0x00
#define MI_NOOP_WRITE_ID		(1<<22)
#define MI_NOOP_ID_MASK			(1<<22 - 1)

#define STATE3D_COLOR_FACTOR	((0x3<<29)|(0x1d<<24)|(0x01<<16))

/* STATE3D_FOG_MODE stuff */
#define ENABLE_FOG_SOURCE	(1<<27)
#define ENABLE_FOG_CONST	(1<<24)
#define ENABLE_FOG_DENSITY	(1<<23)


#define MAX_DISPLAY_PIPES	2

typedef enum {
   CrtIndex = 0,
   TvIndex,
   DfpIndex,
   LfpIndex,
   Tv2Index,
   Dfp2Index,
   UnknownIndex,
   Unknown2Index,
   NumDisplayTypes,
   NumKnownDisplayTypes = UnknownIndex
} DisplayType;

/* What's connected to the pipes (as reported by the BIOS) */
#define PIPE_ACTIVE_MASK		0xff
#define PIPE_CRT_ACTIVE			(1 << CrtIndex)
#define PIPE_TV_ACTIVE			(1 << TvIndex)
#define PIPE_DFP_ACTIVE			(1 << DfpIndex)
#define PIPE_LCD_ACTIVE			(1 << LfpIndex)
#define PIPE_TV2_ACTIVE			(1 << Tv2Index)
#define PIPE_DFP2_ACTIVE		(1 << Dfp2Index)
#define PIPE_UNKNOWN_ACTIVE		((1 << UnknownIndex) |	\
					 (1 << Unknown2Index))

#define PIPE_SIZED_DISP_MASK		(PIPE_DFP_ACTIVE |	\
					 PIPE_LCD_ACTIVE |	\
					 PIPE_DFP2_ACTIVE)

#define PIPE_A_SHIFT			0
#define PIPE_B_SHIFT			8
#define PIPE_SHIFT(n)			((n) == 0 ? \
					 PIPE_A_SHIFT : PIPE_B_SHIFT)

/*
 * Some BIOS scratch area registers.  The 845 (and 830?) store the amount
 * of video memory available to the BIOS in SWF1.
 */

#define SWF0			0x71410
#define SWF1			0x71414
#define SWF2			0x71418
#define SWF3			0x7141c
#define SWF4			0x71420
#define SWF5			0x71424
#define SWF6			0x71428

/*
 * 855 scratch registers.
 */
#define SWF00			0x70410
#define SWF01			0x70414
#define SWF02			0x70418
#define SWF03			0x7041c
#define SWF04			0x70420
#define SWF05			0x70424
#define SWF06			0x70428

#define SWF10			SWF0
#define SWF11			SWF1
#define SWF12			SWF2
#define SWF13			SWF3
#define SWF14			SWF4
#define SWF15			SWF5
#define SWF16			SWF6

#define SWF30			0x72414
#define SWF31			0x72418
#define SWF32			0x7241c

/*
 * Overlay registers.  These are overlay registers accessed via MMIO.
 * Those loaded via the overlay register page are defined in i830_video.c.
 */
#define OVADD			0x30000

#define DOVSTA			0x30008
#define OC_BUF			(0x3<<20)

#define OGAMC5			0x30010
#define OGAMC4			0x30014
#define OGAMC3			0x30018
#define OGAMC2			0x3001c
#define OGAMC1			0x30020
#define OGAMC0			0x30024


/*
 * Palette registers
 */
#define PALETTE_A		0x0a000
#define PALETTE_B		0x0a800

#endif /* _I810_REG_H */
