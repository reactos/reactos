/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_reg.h,v 1.15 2002/12/16 16:19:11 dawes Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Rickard E. Faith <faith@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 * References:
 *
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 */

#ifndef _R128_REG_H_
#define _R128_REG_H_

#define R128_ADAPTER_ID                   0x0f2c /* PCI */
#define R128_AGP_APER_OFFSET              0x0178
#define R128_AGP_BASE                     0x0170
#define R128_AGP_CNTL                     0x0174
#       define R128_AGP_APER_SIZE_256MB   (0x00 << 0)
#       define R128_AGP_APER_SIZE_128MB   (0x20 << 0)
#       define R128_AGP_APER_SIZE_64MB    (0x30 << 0)
#       define R128_AGP_APER_SIZE_32MB    (0x38 << 0)
#       define R128_AGP_APER_SIZE_16MB    (0x3c << 0)
#       define R128_AGP_APER_SIZE_8MB     (0x3e << 0)
#       define R128_AGP_APER_SIZE_4MB     (0x3f << 0)
#       define R128_AGP_APER_SIZE_MASK    (0x3f << 0)
#define R128_AGP_CNTL_B                   0x0b44
#define R128_AGP_COMMAND                  0x0f58 /* PCI */
#define R128_AGP_PLL_CNTL                 0x0010 /* PLL */
#define R128_AGP_STATUS                   0x0f54 /* PCI */
#       define R128_AGP_1X_MODE           0x01
#       define R128_AGP_2X_MODE           0x02
#       define R128_AGP_4X_MODE           0x04
#       define R128_AGP_MODE_MASK         0x07
#define R128_AMCGPIO_A_REG                0x01a0
#define R128_AMCGPIO_EN_REG               0x01a8
#define R128_AMCGPIO_MASK                 0x0194
#define R128_AMCGPIO_Y_REG                0x01a4
#define R128_ATTRDR                       0x03c1 /* VGA */
#define R128_ATTRDW                       0x03c0 /* VGA */
#define R128_ATTRX                        0x03c0 /* VGA */
#define R128_AUX_SC_CNTL                  0x1660
#       define R128_AUX1_SC_EN            (1 << 0)
#       define R128_AUX1_SC_MODE_OR       (0 << 1)
#       define R128_AUX1_SC_MODE_NAND     (1 << 1)
#       define R128_AUX2_SC_EN            (1 << 2)
#       define R128_AUX2_SC_MODE_OR       (0 << 3)
#       define R128_AUX2_SC_MODE_NAND     (1 << 3)
#       define R128_AUX3_SC_EN            (1 << 4)
#       define R128_AUX3_SC_MODE_OR       (0 << 5)
#       define R128_AUX3_SC_MODE_NAND     (1 << 5)
#define R128_AUX1_SC_BOTTOM               0x1670
#define R128_AUX1_SC_LEFT                 0x1664
#define R128_AUX1_SC_RIGHT                0x1668
#define R128_AUX1_SC_TOP                  0x166c
#define R128_AUX2_SC_BOTTOM               0x1680
#define R128_AUX2_SC_LEFT                 0x1674
#define R128_AUX2_SC_RIGHT                0x1678
#define R128_AUX2_SC_TOP                  0x167c
#define R128_AUX3_SC_BOTTOM               0x1690
#define R128_AUX3_SC_LEFT                 0x1684
#define R128_AUX3_SC_RIGHT                0x1688
#define R128_AUX3_SC_TOP                  0x168c
#define R128_AUX_WINDOW_HORZ_CNTL         0x02d8
#define R128_AUX_WINDOW_VERT_CNTL         0x02dc

#define R128_BASE_CODE                    0x0f0b
#define R128_BIOS_0_SCRATCH               0x0010
#define R128_BIOS_1_SCRATCH               0x0014
#define R128_BIOS_2_SCRATCH               0x0018
#define R128_BIOS_3_SCRATCH               0x001c
#define R128_BIOS_4_SCRATCH               0x0020
#define R128_BIOS_5_SCRATCH               0x0024
#       define R128_BIOS_DISPLAY_FP       (1 << 0)
#       define R128_BIOS_DISPLAY_CRT      (2 << 0)
#       define R128_BIOS_DISPLAY_FP_CRT   (3 << 0)
#define R128_BIOS_6_SCRATCH               0x0028
#define R128_BIOS_7_SCRATCH               0x002c
#define R128_BIOS_ROM                     0x0f30 /* PCI */
#define R128_BIST                         0x0f0f /* PCI */
#define R128_BM_CHUNK_0_VAL               0x0a18
#       define R128_BM_PTR_FORCE_TO_PCI    (1 << 21)
#       define R128_BM_PM4_RD_FORCE_TO_PCI (1 << 22)
#       define R128_BM_GLOBAL_FORCE_TO_PCI (1 << 23)
#define R128_BRUSH_DATA0                  0x1480
#define R128_BRUSH_DATA1                  0x1484
#define R128_BRUSH_DATA10                 0x14a8
#define R128_BRUSH_DATA11                 0x14ac
#define R128_BRUSH_DATA12                 0x14b0
#define R128_BRUSH_DATA13                 0x14b4
#define R128_BRUSH_DATA14                 0x14b8
#define R128_BRUSH_DATA15                 0x14bc
#define R128_BRUSH_DATA16                 0x14c0
#define R128_BRUSH_DATA17                 0x14c4
#define R128_BRUSH_DATA18                 0x14c8
#define R128_BRUSH_DATA19                 0x14cc
#define R128_BRUSH_DATA2                  0x1488
#define R128_BRUSH_DATA20                 0x14d0
#define R128_BRUSH_DATA21                 0x14d4
#define R128_BRUSH_DATA22                 0x14d8
#define R128_BRUSH_DATA23                 0x14dc
#define R128_BRUSH_DATA24                 0x14e0
#define R128_BRUSH_DATA25                 0x14e4
#define R128_BRUSH_DATA26                 0x14e8
#define R128_BRUSH_DATA27                 0x14ec
#define R128_BRUSH_DATA28                 0x14f0
#define R128_BRUSH_DATA29                 0x14f4
#define R128_BRUSH_DATA3                  0x148c
#define R128_BRUSH_DATA30                 0x14f8
#define R128_BRUSH_DATA31                 0x14fc
#define R128_BRUSH_DATA32                 0x1500
#define R128_BRUSH_DATA33                 0x1504
#define R128_BRUSH_DATA34                 0x1508
#define R128_BRUSH_DATA35                 0x150c
#define R128_BRUSH_DATA36                 0x1510
#define R128_BRUSH_DATA37                 0x1514
#define R128_BRUSH_DATA38                 0x1518
#define R128_BRUSH_DATA39                 0x151c
#define R128_BRUSH_DATA4                  0x1490
#define R128_BRUSH_DATA40                 0x1520
#define R128_BRUSH_DATA41                 0x1524
#define R128_BRUSH_DATA42                 0x1528
#define R128_BRUSH_DATA43                 0x152c
#define R128_BRUSH_DATA44                 0x1530
#define R128_BRUSH_DATA45                 0x1534
#define R128_BRUSH_DATA46                 0x1538
#define R128_BRUSH_DATA47                 0x153c
#define R128_BRUSH_DATA48                 0x1540
#define R128_BRUSH_DATA49                 0x1544
#define R128_BRUSH_DATA5                  0x1494
#define R128_BRUSH_DATA50                 0x1548
#define R128_BRUSH_DATA51                 0x154c
#define R128_BRUSH_DATA52                 0x1550
#define R128_BRUSH_DATA53                 0x1554
#define R128_BRUSH_DATA54                 0x1558
#define R128_BRUSH_DATA55                 0x155c
#define R128_BRUSH_DATA56                 0x1560
#define R128_BRUSH_DATA57                 0x1564
#define R128_BRUSH_DATA58                 0x1568
#define R128_BRUSH_DATA59                 0x156c
#define R128_BRUSH_DATA6                  0x1498
#define R128_BRUSH_DATA60                 0x1570
#define R128_BRUSH_DATA61                 0x1574
#define R128_BRUSH_DATA62                 0x1578
#define R128_BRUSH_DATA63                 0x157c
#define R128_BRUSH_DATA7                  0x149c
#define R128_BRUSH_DATA8                  0x14a0
#define R128_BRUSH_DATA9                  0x14a4
#define R128_BRUSH_SCALE                  0x1470
#define R128_BRUSH_Y_X                    0x1474
#define R128_BUS_CNTL                     0x0030
#       define R128_BUS_MASTER_DIS         (1 << 6)
#       define R128_BUS_RD_DISCARD_EN      (1 << 24)
#       define R128_BUS_RD_ABORT_EN        (1 << 25)
#       define R128_BUS_MSTR_DISCONNECT_EN (1 << 28)
#       define R128_BUS_WRT_BURST          (1 << 29)
#       define R128_BUS_READ_BURST         (1 << 30)
#define R128_BUS_CNTL1                    0x0034
#       define R128_BUS_WAIT_ON_LOCK_EN    (1 << 4)

#define R128_CACHE_CNTL                   0x1724
#define R128_CACHE_LINE                   0x0f0c /* PCI */
#define R128_CAP0_TRIG_CNTL               0x0950 /* ? */
#define R128_CAP1_TRIG_CNTL               0x09c0 /* ? */
#define R128_CAPABILITIES_ID              0x0f50 /* PCI */
#define R128_CAPABILITIES_PTR             0x0f34 /* PCI */
#define R128_CLK_PIN_CNTL                 0x0001 /* PLL */
#define R128_CLOCK_CNTL_DATA              0x000c
#define R128_CLOCK_CNTL_INDEX             0x0008
#       define R128_PLL_WR_EN             (1 << 7)
#       define R128_PLL_DIV_SEL           (3 << 8)
#define R128_CLR_CMP_CLR_3D               0x1a24
#define R128_CLR_CMP_CLR_DST              0x15c8
#define R128_CLR_CMP_CLR_SRC              0x15c4
#define R128_CLR_CMP_CNTL                 0x15c0
#       define R128_SRC_CMP_EQ_COLOR      (4 <<  0)
#       define R128_SRC_CMP_NEQ_COLOR     (5 <<  0)
#       define R128_CLR_CMP_SRC_SOURCE    (1 << 24)
#define R128_CLR_CMP_MASK                 0x15cc
#       define R128_CLR_CMP_MSK           0xffffffff
#define R128_CLR_CMP_MASK_3D              0x1A28
#define R128_COMMAND                      0x0f04 /* PCI */
#define R128_COMPOSITE_SHADOW_ID          0x1a0c
#define R128_CONFIG_APER_0_BASE           0x0100
#define R128_CONFIG_APER_1_BASE           0x0104
#define R128_CONFIG_APER_SIZE             0x0108
#define R128_CONFIG_BONDS                 0x00e8
#define R128_CONFIG_CNTL                  0x00e0
#       define APER_0_BIG_ENDIAN_16BPP_SWAP (1 << 0)
#       define APER_0_BIG_ENDIAN_32BPP_SWAP (2 << 0)
#define R128_CONFIG_MEMSIZE               0x00f8
#define R128_CONFIG_MEMSIZE_EMBEDDED      0x0114
#define R128_CONFIG_REG_1_BASE            0x010c
#define R128_CONFIG_REG_APER_SIZE         0x0110
#define R128_CONFIG_XSTRAP                0x00e4
#define R128_CONSTANT_COLOR_C             0x1d34
#       define R128_CONSTANT_COLOR_MASK   0x00ffffff
#       define R128_CONSTANT_COLOR_ONE    0x00ffffff
#       define R128_CONSTANT_COLOR_ZERO   0x00000000
#define R128_CRC_CMDFIFO_ADDR             0x0740
#define R128_CRC_CMDFIFO_DOUT             0x0744
#define R128_CRTC_CRNT_FRAME              0x0214
#define R128_CRTC_DEBUG                   0x021c
#define R128_CRTC_EXT_CNTL                0x0054
#       define R128_CRTC_VGA_XOVERSCAN    (1 <<  0)
#       define R128_VGA_ATI_LINEAR        (1 <<  3)
#       define R128_XCRT_CNT_EN           (1 <<  6)
#       define R128_CRTC_HSYNC_DIS        (1 <<  8)
#       define R128_CRTC_VSYNC_DIS        (1 <<  9)
#       define R128_CRTC_DISPLAY_DIS      (1 << 10)
#       define R128_CRTC_CRT_ON           (1 << 15)
#       define R128_FP_OUT_EN             (1 << 22)
#       define R128_FP_ACTIVE             (1 << 23)
#define R128_CRTC_EXT_CNTL_DPMS_BYTE      0x0055
#       define R128_CRTC_HSYNC_DIS_BYTE   (1 <<  0)
#       define R128_CRTC_VSYNC_DIS_BYTE   (1 <<  1)
#       define R128_CRTC_DISPLAY_DIS_BYTE (1 <<  2)
#define R128_CRTC_GEN_CNTL                0x0050
#       define R128_CRTC_DBL_SCAN_EN      (1 <<  0)
#       define R128_CRTC_INTERLACE_EN     (1 <<  1)
#       define R128_CRTC_CSYNC_EN         (1 <<  4)
#       define R128_CRTC_CUR_EN           (1 << 16)
#       define R128_CRTC_CUR_MODE_MASK    (7 << 17)
#       define R128_CRTC_ICON_EN          (1 << 20)
#       define R128_CRTC_EXT_DISP_EN      (1 << 24)
#       define R128_CRTC_EN               (1 << 25)
#       define R128_CRTC_DISP_REQ_EN_B    (1 << 26)
#define R128_CRTC_GUI_TRIG_VLINE          0x0218
#define R128_CRTC_H_SYNC_STRT_WID         0x0204
#       define R128_CRTC_H_SYNC_STRT_PIX        (0x07  <<  0)
#       define R128_CRTC_H_SYNC_STRT_CHAR       (0x1ff <<  3)
#       define R128_CRTC_H_SYNC_STRT_CHAR_SHIFT 3
#       define R128_CRTC_H_SYNC_WID             (0x3f  << 16)
#       define R128_CRTC_H_SYNC_WID_SHIFT       16
#       define R128_CRTC_H_SYNC_POL             (1     << 23)
#define R128_CRTC_H_TOTAL_DISP            0x0200
#       define R128_CRTC_H_TOTAL          (0x01ff << 0)
#       define R128_CRTC_H_TOTAL_SHIFT    0
#       define R128_CRTC_H_DISP           (0x00ff << 16)
#       define R128_CRTC_H_DISP_SHIFT     16
#define R128_CRTC_OFFSET                  0x0224
#define R128_CRTC_OFFSET_CNTL             0x0228
#define R128_CRTC_PITCH                   0x022c
#define R128_CRTC_STATUS                  0x005c
#       define R128_CRTC_VBLANK_SAVE      (1 <<  1)
#define R128_CRTC_V_SYNC_STRT_WID         0x020c
#       define R128_CRTC_V_SYNC_STRT       (0x7ff <<  0)
#       define R128_CRTC_V_SYNC_STRT_SHIFT 0
#       define R128_CRTC_V_SYNC_WID        (0x1f  << 16)
#       define R128_CRTC_V_SYNC_WID_SHIFT  16
#       define R128_CRTC_V_SYNC_POL        (1     << 23)
#define R128_CRTC_V_TOTAL_DISP            0x0208
#       define R128_CRTC_V_TOTAL          (0x07ff << 0)
#       define R128_CRTC_V_TOTAL_SHIFT    0
#       define R128_CRTC_V_DISP           (0x07ff << 16)
#       define R128_CRTC_V_DISP_SHIFT     16
#define R128_CRTC_VLINE_CRNT_VLINE        0x0210
#       define R128_CRTC_CRNT_VLINE_MASK  (0x7ff << 16)
#define R128_CRTC2_CRNT_FRAME             0x0314
#define R128_CRTC2_DEBUG                  0x031c
#define R128_CRTC2_GEN_CNTL               0x03f8
#define R128_CRTC2_GUI_TRIG_VLINE         0x0318
#define R128_CRTC2_H_SYNC_STRT_WID        0x0304
#define R128_CRTC2_H_TOTAL_DISP           0x0300
#define R128_CRTC2_OFFSET                 0x0324
#define R128_CRTC2_OFFSET_CNTL            0x0328
#define R128_CRTC2_PITCH                  0x032c
#define R128_CRTC2_STATUS                 0x03fc
#define R128_CRTC2_V_SYNC_STRT_WID        0x030c
#define R128_CRTC2_V_TOTAL_DISP           0x0308
#define R128_CRTC2_VLINE_CRNT_VLINE       0x0310
#define R128_CRTC8_DATA                   0x03d5 /* VGA, 0x3b5 */
#define R128_CRTC8_IDX                    0x03d4 /* VGA, 0x3b4 */
#define R128_CUR_CLR0                     0x026c
#define R128_CUR_CLR1                     0x0270
#define R128_CUR_HORZ_VERT_OFF            0x0268
#define R128_CUR_HORZ_VERT_POSN           0x0264
#define R128_CUR_OFFSET                   0x0260
#       define R128_CUR_LOCK              (1 << 31)

#define R128_DAC_CNTL                     0x0058
#       define R128_DAC_RANGE_CNTL        (3 <<  0)
#       define R128_DAC_BLANKING          (1 <<  2)
#       define R128_DAC_CRT_SEL_CRTC2     (1 <<  4)
#       define R128_DAC_PALETTE_ACC_CTL   (1 <<  5)
#       define R128_DAC_8BIT_EN           (1 <<  8)
#       define R128_DAC_VGA_ADR_EN        (1 << 13)
#       define R128_DAC_MASK_ALL          (0xff << 24)
#define R128_DAC_CRC_SIG                  0x02cc
#define R128_DAC_DATA                     0x03c9 /* VGA */
#define R128_DAC_MASK                     0x03c6 /* VGA */
#define R128_DAC_R_INDEX                  0x03c7 /* VGA */
#define R128_DAC_W_INDEX                  0x03c8 /* VGA */
#define R128_DDA_CONFIG                   0x02e0
#define R128_DDA_ON_OFF                   0x02e4
#define R128_DEFAULT_OFFSET               0x16e0
#define R128_DEFAULT_PITCH                0x16e4
#define R128_DEFAULT_SC_BOTTOM_RIGHT      0x16e8
#       define R128_DEFAULT_SC_RIGHT_MAX  (0x1fff <<  0)
#       define R128_DEFAULT_SC_BOTTOM_MAX (0x1fff << 16)
#define R128_DESTINATION_3D_CLR_CMP_VAL   0x1820
#define R128_DESTINATION_3D_CLR_CMP_MSK   0x1824
#define R128_DEVICE_ID                    0x0f02 /* PCI */
#define R128_DP_BRUSH_BKGD_CLR            0x1478
#define R128_DP_BRUSH_FRGD_CLR            0x147c
#define R128_DP_CNTL                      0x16c0
#       define R128_DST_X_LEFT_TO_RIGHT   (1 <<  0)
#       define R128_DST_Y_TOP_TO_BOTTOM   (1 <<  1)
#define R128_DP_CNTL_XDIR_YDIR_YMAJOR     0x16d0
#       define R128_DST_Y_MAJOR             (1 <<  2)
#       define R128_DST_Y_DIR_TOP_TO_BOTTOM (1 << 15)
#       define R128_DST_X_DIR_LEFT_TO_RIGHT (1 << 31)
#define R128_DP_DATATYPE                  0x16c4
#       define R128_HOST_BIG_ENDIAN_EN    (1 << 29)
#define R128_DP_GUI_MASTER_CNTL           0x146c
#       define R128_GMC_SRC_PITCH_OFFSET_CNTL (1    <<  0)
#       define R128_GMC_DST_PITCH_OFFSET_CNTL (1    <<  1)
#       define R128_GMC_SRC_CLIPPING          (1    <<  2)
#       define R128_GMC_DST_CLIPPING          (1    <<  3)
#       define R128_GMC_BRUSH_DATATYPE_MASK   (0x0f <<  4)
#       define R128_GMC_BRUSH_8X8_MONO_FG_BG  (0    <<  4)
#       define R128_GMC_BRUSH_8X8_MONO_FG_LA  (1    <<  4)
#       define R128_GMC_BRUSH_1X8_MONO_FG_BG  (4    <<  4)
#       define R128_GMC_BRUSH_1X8_MONO_FG_LA  (5    <<  4)
#       define R128_GMC_BRUSH_32x1_MONO_FG_BG (6    <<  4)
#       define R128_GMC_BRUSH_32x1_MONO_FG_LA (7    <<  4)
#       define R128_GMC_BRUSH_32x32_MONO_FG_BG (8    <<  4)
#       define R128_GMC_BRUSH_32x32_MONO_FG_LA (9    <<  4)
#       define R128_GMC_BRUSH_8x8_COLOR       (10   <<  4)
#       define R128_GMC_BRUSH_1X8_COLOR       (12   <<  4)
#       define R128_GMC_BRUSH_SOLID_COLOR     (13   <<  4)
#       define R128_GMC_BRUSH_NONE            (15   <<  4)
#       define R128_GMC_DST_8BPP_CI           (2    <<  8)
#       define R128_GMC_DST_15BPP             (3    <<  8)
#       define R128_GMC_DST_16BPP             (4    <<  8)
#       define R128_GMC_DST_24BPP             (5    <<  8)
#       define R128_GMC_DST_32BPP             (6    <<  8)
#       define R128_GMC_DST_8BPP_RGB          (7    <<  8)
#       define R128_GMC_DST_Y8                (8    <<  8)
#       define R128_GMC_DST_RGB8              (9    <<  8)
#       define R128_GMC_DST_VYUY              (11   <<  8)
#       define R128_GMC_DST_YVYU              (12   <<  8)
#       define R128_GMC_DST_AYUV444           (14   <<  8)
#       define R128_GMC_DST_ARGB4444          (15   <<  8)
#       define R128_GMC_DST_DATATYPE_MASK     (0x0f <<  8)
#       define R128_GMC_DST_DATATYPE_SHIFT    8
#       define R128_GMC_SRC_DATATYPE_MASK       (3    << 12)
#       define R128_GMC_SRC_DATATYPE_MONO_FG_BG (0    << 12)
#       define R128_GMC_SRC_DATATYPE_MONO_FG_LA (1    << 12)
#       define R128_GMC_SRC_DATATYPE_COLOR      (3    << 12)
#       define R128_GMC_BYTE_PIX_ORDER        (1    << 14)
#       define R128_GMC_BYTE_MSB_TO_LSB       (0    << 14)
#       define R128_GMC_BYTE_LSB_TO_MSB       (1    << 14)
#       define R128_GMC_CONVERSION_TEMP       (1    << 15)
#       define R128_GMC_CONVERSION_TEMP_6500  (0    << 15)
#       define R128_GMC_CONVERSION_TEMP_9300  (1    << 15)
#       define R128_GMC_ROP3_MASK             (0xff << 16)
#       define R128_DP_SRC_SOURCE_MASK        (7    << 24)
#       define R128_DP_SRC_SOURCE_MEMORY      (2    << 24)
#       define R128_DP_SRC_SOURCE_HOST_DATA   (3    << 24)
#       define R128_GMC_3D_FCN_EN             (1    << 27)
#       define R128_GMC_CLR_CMP_CNTL_DIS      (1    << 28)
#       define R128_GMC_AUX_CLIP_DIS          (1    << 29)
#       define R128_GMC_WR_MSK_DIS            (1    << 30)
#       define R128_GMC_LD_BRUSH_Y_X          (1    << 31)
#       define R128_ROP3_ZERO             0x00000000
#       define R128_ROP3_DSa              0x00880000
#       define R128_ROP3_SDna             0x00440000
#       define R128_ROP3_S                0x00cc0000
#       define R128_ROP3_DSna             0x00220000
#       define R128_ROP3_D                0x00aa0000
#       define R128_ROP3_DSx              0x00660000
#       define R128_ROP3_DSo              0x00ee0000
#       define R128_ROP3_DSon             0x00110000
#       define R128_ROP3_DSxn             0x00990000
#       define R128_ROP3_Dn               0x00550000
#       define R128_ROP3_SDno             0x00dd0000
#       define R128_ROP3_Sn               0x00330000
#       define R128_ROP3_DSno             0x00bb0000
#       define R128_ROP3_DSan             0x00770000
#       define R128_ROP3_ONE              0x00ff0000
#       define R128_ROP3_DPa              0x00a00000
#       define R128_ROP3_PDna             0x00500000
#       define R128_ROP3_P                0x00f00000
#       define R128_ROP3_DPna             0x000a0000
#       define R128_ROP3_D                0x00aa0000
#       define R128_ROP3_DPx              0x005a0000
#       define R128_ROP3_DPo              0x00fa0000
#       define R128_ROP3_DPon             0x00050000
#       define R128_ROP3_PDxn             0x00a50000
#       define R128_ROP3_PDno             0x00f50000
#       define R128_ROP3_Pn               0x000f0000
#       define R128_ROP3_DPno             0x00af0000
#       define R128_ROP3_DPan             0x005f0000


#define R128_DP_GUI_MASTER_CNTL_C         0x1c84
#define R128_DP_MIX                       0x16c8
#define R128_DP_SRC_BKGD_CLR              0x15dc
#define R128_DP_SRC_FRGD_CLR              0x15d8
#define R128_DP_WRITE_MASK                0x16cc
#define R128_DST_BRES_DEC                 0x1630
#define R128_DST_BRES_ERR                 0x1628
#define R128_DST_BRES_INC                 0x162c
#define R128_DST_BRES_LNTH                0x1634
#define R128_DST_BRES_LNTH_SUB            0x1638
#define R128_DST_HEIGHT                   0x1410
#define R128_DST_HEIGHT_WIDTH             0x143c
#define R128_DST_HEIGHT_WIDTH_8           0x158c
#define R128_DST_HEIGHT_WIDTH_BW          0x15b4
#define R128_DST_HEIGHT_Y                 0x15a0
#define R128_DST_OFFSET                   0x1404
#define R128_DST_PITCH                    0x1408
#define R128_DST_PITCH_OFFSET             0x142c
#define R128_DST_PITCH_OFFSET_C           0x1c80
#       define R128_PITCH_SHIFT               21
#       define R128_DST_TILE                 (1 << 31)
#define R128_DST_WIDTH                    0x140c
#define R128_DST_WIDTH_HEIGHT             0x1598
#define R128_DST_WIDTH_X                  0x1588
#define R128_DST_WIDTH_X_INCY             0x159c
#define R128_DST_X                        0x141c
#define R128_DST_X_SUB                    0x15a4
#define R128_DST_X_Y                      0x1594
#define R128_DST_Y                        0x1420
#define R128_DST_Y_SUB                    0x15a8
#define R128_DST_Y_X                      0x1438

#define R128_EXT_MEM_CNTL                 0x0144

#define R128_FCP_CNTL                     0x0012 /* PLL */
#define R128_FLUSH_1                      0x1704
#define R128_FLUSH_2                      0x1708
#define R128_FLUSH_3                      0x170c
#define R128_FLUSH_4                      0x1710
#define R128_FLUSH_5                      0x1714
#define R128_FLUSH_6                      0x1718
#define R128_FLUSH_7                      0x171c
#define R128_FOG_3D_TABLE_START           0x1810
#define R128_FOG_3D_TABLE_END             0x1814
#define R128_FOG_3D_TABLE_DENSITY         0x181c
#define R128_FOG_TABLE_INDEX              0x1a14
#define R128_FOG_TABLE_DATA               0x1a18
#define R128_FP_CRTC_H_TOTAL_DISP         0x0250
#define R128_FP_CRTC_V_TOTAL_DISP         0x0254
#define R128_FP_GEN_CNTL                  0x0284
#       define R128_FP_FPON                  (1 << 0)
#       define R128_FP_BLANK_DIS             (1 << 1)
#       define R128_FP_TDMS_EN               (1 <<  2)
#       define R128_FP_DETECT_SENSE          (1 <<  8)
#       define R128_FP_SEL_CRTC2             (1 << 13)
#       define R128_FP_CRTC_DONT_SHADOW_VPAR (1 << 16)
#       define R128_FP_CRTC_DONT_SHADOW_HEND (1 << 17)
#       define R128_FP_CRTC_USE_SHADOW_VEND  (1 << 18)
#       define R128_FP_CRTC_USE_SHADOW_ROWCUR (1 << 19)
#       define R128_FP_CRTC_HORZ_DIV2_EN     (1 << 20)
#       define R128_FP_CRTC_HOR_CRT_DIV2_DIS (1 << 21)
#       define R128_FP_CRT_SYNC_SEL          (1 << 23)
#       define R128_FP_USE_SHADOW_EN         (1 << 24)
#define R128_FP_H_SYNC_STRT_WID           0x02c4
#define R128_FP_HORZ_STRETCH              0x028c
#       define R128_HORZ_STRETCH_RATIO_MASK  0xffff
#       define R128_HORZ_STRETCH_RATIO_SHIFT 0
#       define R128_HORZ_STRETCH_RATIO_MAX   4096
#       define R128_HORZ_PANEL_SIZE          (0xff   << 16)
#       define R128_HORZ_PANEL_SHIFT         16
#       define R128_AUTO_HORZ_RATIO          (0      << 24)
#       define R128_HORZ_STRETCH_PIXREP      (0      << 25)
#       define R128_HORZ_STRETCH_BLEND       (1      << 25)
#       define R128_HORZ_STRETCH_ENABLE      (1      << 26)
#       define R128_HORZ_FP_LOOP_STRETCH     (0x7    << 27)
#       define R128_HORZ_STRETCH_RESERVED    (1      << 30)
#       define R128_HORZ_AUTO_RATIO_FIX_EN   (1      << 31)

#define R128_FP_PANEL_CNTL                0x0288
#       define R128_FP_DIGON              (1 << 0)
#       define R128_FP_BLON               (1 << 1)
#define R128_FP_V_SYNC_STRT_WID           0x02c8
#define R128_FP_VERT_STRETCH              0x0290
#       define R128_VERT_PANEL_SIZE          (0x7ff <<  0)
#       define R128_VERT_PANEL_SHIFT         0
#       define R128_VERT_STRETCH_RATIO_MASK  0x3ff
#       define R128_VERT_STRETCH_RATIO_SHIFT 11
#       define R128_VERT_STRETCH_RATIO_MAX   1024
#       define R128_VERT_STRETCH_ENABLE      (1     << 24)
#       define R128_VERT_STRETCH_LINEREP     (0     << 25)
#       define R128_VERT_STRETCH_BLEND       (1     << 25)
#       define R128_VERT_AUTO_RATIO_EN       (1     << 26)
#       define R128_VERT_STRETCH_RESERVED    0xf8e00000

#define R128_GEN_INT_CNTL                 0x0040
#define R128_GEN_INT_STATUS               0x0044
#       define R128_VSYNC_INT_AK          (1 <<  2)
#       define R128_VSYNC_INT             (1 <<  2)
#define R128_GEN_RESET_CNTL               0x00f0
#       define R128_SOFT_RESET_GUI          (1 <<  0)
#       define R128_SOFT_RESET_VCLK         (1 <<  8)
#       define R128_SOFT_RESET_PCLK         (1 <<  9)
#       define R128_SOFT_RESET_DISPENG_XCLK (1 << 11)
#       define R128_SOFT_RESET_MEMCTLR_XCLK (1 << 12)
#define R128_GENENB                       0x03c3 /* VGA */
#define R128_GENFC_RD                     0x03ca /* VGA */
#define R128_GENFC_WT                     0x03da /* VGA, 0x03ba */
#define R128_GENMO_RD                     0x03cc /* VGA */
#define R128_GENMO_WT                     0x03c2 /* VGA */
#define R128_GENS0                        0x03c2 /* VGA */
#define R128_GENS1                        0x03da /* VGA, 0x03ba */
#define R128_GPIO_MONID                   0x0068
#       define R128_GPIO_MONID_A_0        (1 <<  0)
#       define R128_GPIO_MONID_A_1        (1 <<  1)
#       define R128_GPIO_MONID_A_2        (1 <<  2)
#       define R128_GPIO_MONID_A_3        (1 <<  3)
#       define R128_GPIO_MONID_Y_0        (1 <<  8)
#       define R128_GPIO_MONID_Y_1        (1 <<  9)
#       define R128_GPIO_MONID_Y_2        (1 << 10)
#       define R128_GPIO_MONID_Y_3        (1 << 11)
#       define R128_GPIO_MONID_EN_0       (1 << 16)
#       define R128_GPIO_MONID_EN_1       (1 << 17)
#       define R128_GPIO_MONID_EN_2       (1 << 18)
#       define R128_GPIO_MONID_EN_3       (1 << 19)
#       define R128_GPIO_MONID_MASK_0     (1 << 24)
#       define R128_GPIO_MONID_MASK_1     (1 << 25)
#       define R128_GPIO_MONID_MASK_2     (1 << 26)
#       define R128_GPIO_MONID_MASK_3     (1 << 27)
#define R128_GPIO_MONIDB                  0x006c
#define R128_GRPH8_DATA                   0x03cf /* VGA */
#define R128_GRPH8_IDX                    0x03ce /* VGA */
#define R128_GUI_DEBUG0                   0x16a0
#define R128_GUI_DEBUG1                   0x16a4
#define R128_GUI_DEBUG2                   0x16a8
#define R128_GUI_DEBUG3                   0x16ac
#define R128_GUI_DEBUG4                   0x16b0
#define R128_GUI_DEBUG5                   0x16b4
#define R128_GUI_DEBUG6                   0x16b8
#define R128_GUI_PROBE                    0x16bc
#define R128_GUI_SCRATCH_REG0             0x15e0
#define R128_GUI_SCRATCH_REG1             0x15e4
#define R128_GUI_SCRATCH_REG2             0x15e8
#define R128_GUI_SCRATCH_REG3             0x15ec
#define R128_GUI_SCRATCH_REG4             0x15f0
#define R128_GUI_SCRATCH_REG5             0x15f4
#define R128_GUI_STAT                     0x1740
#       define R128_GUI_FIFOCNT_MASK      0x0fff
#       define R128_GUI_ACTIVE            (1 << 31)

#define R128_HEADER                       0x0f0e /* PCI */
#define R128_HOST_DATA0                   0x17c0
#define R128_HOST_DATA1                   0x17c4
#define R128_HOST_DATA2                   0x17c8
#define R128_HOST_DATA3                   0x17cc
#define R128_HOST_DATA4                   0x17d0
#define R128_HOST_DATA5                   0x17d4
#define R128_HOST_DATA6                   0x17d8
#define R128_HOST_DATA7                   0x17dc
#define R128_HOST_DATA_LAST               0x17e0
#define R128_HOST_PATH_CNTL               0x0130
#define R128_HTOTAL_CNTL                  0x0009 /* PLL */
#define R128_HW_DEBUG                     0x0128
#define R128_HW_DEBUG2                    0x011c

#define R128_I2C_CNTL_1                   0x0094 /* ? */
#define R128_INTERRUPT_LINE               0x0f3c /* PCI */
#define R128_INTERRUPT_PIN                0x0f3d /* PCI */
#define R128_IO_BASE                      0x0f14 /* PCI */

#define R128_LATENCY                      0x0f0d /* PCI */
#define R128_LEAD_BRES_DEC                0x1608
#define R128_LEAD_BRES_ERR                0x1600
#define R128_LEAD_BRES_INC                0x1604
#define R128_LEAD_BRES_LNTH               0x161c
#define R128_LEAD_BRES_LNTH_SUB           0x1624
#define R128_LVDS_GEN_CNTL                0x02d0
#       define R128_LVDS_ON               (1   <<  0)
#       define R128_LVDS_DISPLAY_DIS      (1   <<  1)
#       define R128_LVDS_EN               (1   <<  7)
#       define R128_LVDS_DIGON            (1   << 18)
#       define R128_LVDS_BLON             (1   << 19)
#       define R128_LVDS_SEL_CRTC2        (1   << 23)
#       define R128_HSYNC_DELAY_SHIFT     28
#       define R128_HSYNC_DELAY_MASK      (0xf << 28)

#define R128_MAX_LATENCY                  0x0f3f /* PCI */
#define R128_MCLK_CNTL                    0x000f /* PLL */
#       define R128_FORCE_GCP             (1 << 16)
#       define R128_FORCE_PIPE3D_CP       (1 << 17)
#       define R128_FORCE_RCP             (1 << 18)
#define R128_MDGPIO_A_REG                 0x01ac
#define R128_MDGPIO_EN_REG                0x01b0
#define R128_MDGPIO_MASK                  0x0198
#define R128_MDGPIO_Y_REG                 0x01b4
#define R128_MEM_ADDR_CONFIG              0x0148
#define R128_MEM_BASE                     0x0f10 /* PCI */
#define R128_MEM_CNTL                     0x0140
#define R128_MEM_INIT_LAT_TIMER           0x0154
#define R128_MEM_INTF_CNTL                0x014c
#define R128_MEM_SDRAM_MODE_REG           0x0158
#define R128_MEM_STR_CNTL                 0x0150
#define R128_MEM_VGA_RP_SEL               0x003c
#define R128_MEM_VGA_WP_SEL               0x0038
#define R128_MIN_GRANT                    0x0f3e /* PCI */
#define R128_MM_DATA                      0x0004
#define R128_MM_INDEX                     0x0000
#define R128_MPLL_CNTL                    0x000e /* PLL */
#define R128_MPP_TB_CONFIG                0x01c0 /* ? */
#define R128_MPP_GP_CONFIG                0x01c8 /* ? */

#define R128_N_VIF_COUNT                  0x0248

#define R128_OVR_CLR                      0x0230
#define R128_OVR_WID_LEFT_RIGHT           0x0234
#define R128_OVR_WID_TOP_BOTTOM           0x0238

/* first overlay unit (there is only one) */

#define R128_OV0_Y_X_START                0x0400
#define R128_OV0_Y_X_END                  0x0404
#define R128_OV0_EXCLUSIVE_HORZ           0x0408
#       define  R128_EXCL_HORZ_START_MASK        0x000000ff
#       define  R128_EXCL_HORZ_END_MASK          0x0000ff00
#       define  R128_EXCL_HORZ_BACK_PORCH_MASK   0x00ff0000
#       define  R128_EXCL_HORZ_EXCLUSIVE_EN      0x80000000
#define R128_OV0_EXCLUSIVE_VERT           0x040C
#       define  R128_EXCL_VERT_START_MASK        0x000003ff
#       define  R128_EXCL_VERT_END_MASK          0x03ff0000
#define R128_OV0_REG_LOAD_CNTL            0x0410
#       define  R128_REG_LD_CTL_LOCK                 0x00000001L
#       define  R128_REG_LD_CTL_VBLANK_DURING_LOCK   0x00000002L
#       define  R128_REG_LD_CTL_STALL_GUI_UNTIL_FLIP 0x00000004L
#       define  R128_REG_LD_CTL_LOCK_READBACK        0x00000008L
#define R128_OV0_SCALE_CNTL               0x0420
#       define  R128_SCALER_PIX_EXPAND           0x00000001L
#       define  R128_SCALER_Y2R_TEMP             0x00000002L
#       define  R128_SCALER_HORZ_PICK_NEAREST    0x00000003L
#       define  R128_SCALER_VERT_PICK_NEAREST    0x00000004L
#       define  R128_SCALER_SIGNED_UV            0x00000010L
#       define  R128_SCALER_GAMMA_SEL_MASK       0x00000060L
#       define  R128_SCALER_GAMMA_SEL_BRIGHT     0x00000000L
#       define  R128_SCALER_GAMMA_SEL_G22        0x00000020L
#       define  R128_SCALER_GAMMA_SEL_G18        0x00000040L
#       define  R128_SCALER_GAMMA_SEL_G14        0x00000060L
#       define  R128_SCALER_COMCORE_SHIFT_UP_ONE 0x00000080L
#       define  R128_SCALER_SURFAC_FORMAT        0x00000f00L
#       define  R128_SCALER_SOURCE_15BPP         0x00000300L
#       define  R128_SCALER_SOURCE_16BPP         0x00000400L
#       define  R128_SCALER_SOURCE_32BPP         0x00000600L
#       define  R128_SCALER_SOURCE_YUV9          0x00000900L
#       define  R128_SCALER_SOURCE_YUV12         0x00000A00L
#       define  R128_SCALER_SOURCE_VYUY422       0x00000B00L
#       define  R128_SCALER_SOURCE_YVYU422       0x00000C00L
#       define  R128_SCALER_SMART_SWITCH         0x00008000L
#       define  R128_SCALER_BURST_PER_PLANE      0x00ff0000L
#       define  R128_SCALER_DOUBLE_BUFFER        0x01000000L
#       define  R128_SCALER_DIS_LIMIT            0x08000000L
#       define  R128_SCALER_PRG_LOAD_START       0x10000000L
#       define  R128_SCALER_INT_EMU              0x20000000L
#       define  R128_SCALER_ENABLE               0x40000000L
#       define  R128_SCALER_SOFT_RESET           0x80000000L
#define R128_OV0_V_INC                    0x0424
#define R128_OV0_P1_V_ACCUM_INIT          0x0428
#       define  R128_OV0_P1_MAX_LN_IN_PER_LN_OUT        0x00000003L
#       define  R128_OV0_P1_V_ACCUM_INIT_MASK           0x01ff8000L
#define R128_OV0_P23_V_ACCUM_INIT         0x042C
#define R128_OV0_P1_BLANK_LINES_AT_TOP    0x0430
#       define  R128_P1_BLNK_LN_AT_TOP_M1_MASK   0x00000fffL
#       define  R128_P1_ACTIVE_LINES_M1          0x0fff0000L
#define R128_OV0_P23_BLANK_LINES_AT_TOP   0x0434
#       define  R128_P23_BLNK_LN_AT_TOP_M1_MASK  0x000007ffL
#       define  R128_P23_ACTIVE_LINES_M1         0x07ff0000L
#define R128_OV0_VID_BUF0_BASE_ADRS       0x0440
#       define  R128_VIF_BUF0_PITCH_SEL          0x00000001L
#       define  R128_VIF_BUF0_TILE_ADRS          0x00000002L
#       define  R128_VIF_BUF0_BASE_ADRS_MASK     0x03fffff0L
#       define  R128_VIF_BUF0_1ST_LINE_LSBS_MASK 0x48000000L
#define R128_OV0_VID_BUF1_BASE_ADRS       0x0444
#       define  R128_VIF_BUF1_PITCH_SEL          0x00000001L
#       define  R128_VIF_BUF1_TILE_ADRS          0x00000002L
#       define  R128_VIF_BUF1_BASE_ADRS_MASK     0x03fffff0L
#       define  R128_VIF_BUF1_1ST_LINE_LSBS_MASK 0x48000000L
#define R128_OV0_VID_BUF2_BASE_ADRS       0x0448
#       define  R128_VIF_BUF2_PITCH_SEL          0x00000001L
#       define  R128_VIF_BUF2_TILE_ADRS          0x00000002L
#       define  R128_VIF_BUF2_BASE_ADRS_MASK     0x03fffff0L
#       define  R128_VIF_BUF2_1ST_LINE_LSBS_MASK 0x48000000L
#define R128_OV0_VID_BUF3_BASE_ADRS       0x044C
#define R128_OV0_VID_BUF4_BASE_ADRS       0x0450
#define R128_OV0_VID_BUF5_BASE_ADRS       0x0454
#define R128_OV0_VID_BUF_PITCH0_VALUE     0x0460
#define R128_OV0_VID_BUF_PITCH1_VALUE     0x0464
#define R128_OV0_AUTO_FLIP_CNTL           0x0470
#define R128_OV0_DEINTERLACE_PATTERN      0x0474
#define R128_OV0_H_INC                    0x0480
#define R128_OV0_STEP_BY                  0x0484
#define R128_OV0_P1_H_ACCUM_INIT          0x0488
#define R128_OV0_P23_H_ACCUM_INIT         0x048C
#define R128_OV0_P1_X_START_END           0x0494
#define R128_OV0_P2_X_START_END           0x0498
#define R128_OV0_P3_X_START_END           0x049C
#define R128_OV0_FILTER_CNTL              0x04A0
#define R128_OV0_FOUR_TAP_COEF_0          0x04B0
#define R128_OV0_FOUR_TAP_COEF_1          0x04B4
#define R128_OV0_FOUR_TAP_COEF_2          0x04B8
#define R128_OV0_FOUR_TAP_COEF_3          0x04BC
#define R128_OV0_FOUR_TAP_COEF_4          0x04C0
#define R128_OV0_COLOUR_CNTL              0x04E0
#define R128_OV0_VIDEO_KEY_CLR            0x04E4
#define R128_OV0_VIDEO_KEY_MSK            0x04E8
#define R128_OV0_GRAPHICS_KEY_CLR         0x04EC
#define R128_OV0_GRAPHICS_KEY_MSK         0x04F0
#define R128_OV0_KEY_CNTL                 0x04F4
#       define  R128_VIDEO_KEY_FN_MASK           0x00000007L
#       define  R128_VIDEO_KEY_FN_FALSE          0x00000000L
#       define  R128_VIDEO_KEY_FN_TRUE           0x00000001L
#       define  R128_VIDEO_KEY_FN_EQ             0x00000004L
#       define  R128_VIDEO_KEY_FN_NE             0x00000005L
#       define  R128_GRAPHIC_KEY_FN_MASK         0x00000070L
#       define  R128_GRAPHIC_KEY_FN_FALSE        0x00000000L
#       define  R128_GRAPHIC_KEY_FN_TRUE         0x00000010L
#       define  R128_GRAPHIC_KEY_FN_EQ           0x00000040L
#       define  R128_GRAPHIC_KEY_FN_NE           0x00000050L
#       define  R128_CMP_MIX_MASK                0x00000100L
#       define  R128_CMP_MIX_OR                  0x00000000L
#       define  R128_CMP_MIX_AND                 0x00000100L
#define R128_OV0_TEST                     0x04F8


#define R128_PALETTE_DATA                 0x00b4
#define R128_PALETTE_INDEX                0x00b0
#define R128_PC_DEBUG_MODE                0x1760
#define R128_PC_GUI_CTLSTAT               0x1748
#define R128_PC_GUI_MODE                  0x1744
#       define R128_PC_IGNORE_UNIFY       (1 << 5)
#define R128_PC_MISC_CNTL                 0x0188
#define R128_PC_NGUI_CTLSTAT              0x0184
#       define R128_PC_FLUSH_GUI          (3 << 0)
#       define R128_PC_RI_GUI             (1 << 2)
#       define R128_PC_FLUSH_ALL          0x00ff
#       define R128_PC_BUSY               (1 << 31)
#define R128_PC_NGUI_MODE                 0x0180
#define R128_PCI_GART_PAGE                0x017c
#define R128_PLANE_3D_MASK_C              0x1d44
#define R128_PLL_TEST_CNTL                0x0013 /* PLL */
#define R128_PMI_CAP_ID                   0x0f5c /* PCI */
#define R128_PMI_DATA                     0x0f63 /* PCI */
#define R128_PMI_NXT_CAP_PTR              0x0f5d /* PCI */
#define R128_PMI_PMC_REG                  0x0f5e /* PCI */
#define R128_PMI_PMCSR_REG                0x0f60 /* PCI */
#define R128_PMI_REGISTER                 0x0f5c /* PCI */
#define R128_PPLL_CNTL                    0x0002 /* PLL */
#       define R128_PPLL_RESET                (1 <<  0)
#       define R128_PPLL_SLEEP                (1 <<  1)
#       define R128_PPLL_ATOMIC_UPDATE_EN     (1 << 16)
#       define R128_PPLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#define R128_PPLL_DIV_0                   0x0004 /* PLL */
#define R128_PPLL_DIV_1                   0x0005 /* PLL */
#define R128_PPLL_DIV_2                   0x0006 /* PLL */
#define R128_PPLL_DIV_3                   0x0007 /* PLL */
#       define R128_PPLL_FB3_DIV_MASK     0x07ff
#       define R128_PPLL_POST3_DIV_MASK   0x00070000
#define R128_PPLL_REF_DIV                 0x0003 /* PLL */
#       define R128_PPLL_REF_DIV_MASK     0x03ff
#       define R128_PPLL_ATOMIC_UPDATE_R  (1 << 15) /* same as _W */
#       define R128_PPLL_ATOMIC_UPDATE_W  (1 << 15) /* same as _R */
#define R128_PWR_MNGMT_CNTL_STATUS        0x0f60 /* PCI */
#define R128_REG_BASE                     0x0f18 /* PCI */
#define R128_REGPROG_INF                  0x0f09 /* PCI */
#define R128_REVISION_ID                  0x0f08 /* PCI */

#define R128_SC_BOTTOM                    0x164c
#define R128_SC_BOTTOM_RIGHT              0x16f0
#define R128_SC_BOTTOM_RIGHT_C            0x1c8c
#define R128_SC_LEFT                      0x1640
#define R128_SC_RIGHT                     0x1644
#define R128_SC_TOP                       0x1648
#define R128_SC_TOP_LEFT                  0x16ec
#define R128_SC_TOP_LEFT_C                0x1c88
#define R128_SEQ8_DATA                    0x03c5 /* VGA */
#define R128_SEQ8_IDX                     0x03c4 /* VGA */
#define R128_SNAPSHOT_F_COUNT             0x0244
#define R128_SNAPSHOT_VH_COUNTS           0x0240
#define R128_SNAPSHOT_VIF_COUNT           0x024c
#define R128_SRC_OFFSET                   0x15ac
#define R128_SRC_PITCH                    0x15b0
#define R128_SRC_PITCH_OFFSET             0x1428
#define R128_SRC_SC_BOTTOM                0x165c
#define R128_SRC_SC_BOTTOM_RIGHT          0x16f4
#define R128_SRC_SC_RIGHT                 0x1654
#define R128_SRC_X                        0x1414
#define R128_SRC_X_Y                      0x1590
#define R128_SRC_Y                        0x1418
#define R128_SRC_Y_X                      0x1434
#define R128_STATUS                       0x0f06 /* PCI */
#define R128_SUBPIC_CNTL                  0x0540 /* ? */
#define R128_SUB_CLASS                    0x0f0a /* PCI */
#define R128_SURFACE_DELAY                0x0b00
#define R128_SURFACE0_INFO                0x0b0c
#define R128_SURFACE0_LOWER_BOUND         0x0b04
#define R128_SURFACE0_UPPER_BOUND         0x0b08
#define R128_SURFACE1_INFO                0x0b1c
#define R128_SURFACE1_LOWER_BOUND         0x0b14
#define R128_SURFACE1_UPPER_BOUND         0x0b18
#define R128_SURFACE2_INFO                0x0b2c
#define R128_SURFACE2_LOWER_BOUND         0x0b24
#define R128_SURFACE2_UPPER_BOUND         0x0b28
#define R128_SURFACE3_INFO                0x0b3c
#define R128_SURFACE3_LOWER_BOUND         0x0b34
#define R128_SURFACE3_UPPER_BOUND         0x0b38
#define R128_SW_SEMAPHORE                 0x013c

#define R128_TEST_DEBUG_CNTL              0x0120
#define R128_TEST_DEBUG_MUX               0x0124
#define R128_TEST_DEBUG_OUT               0x012c
#define R128_TMDS_CRC                     0x02a0
#define R128_TMDS_TRANSMITTER_CNTL        0x02a4
#       define R128_TMDS_PLLEN            (1 << 0)
#       define R128_TMDS_PLLRST           (1 << 1)
#define R128_TRAIL_BRES_DEC               0x1614
#define R128_TRAIL_BRES_ERR               0x160c
#define R128_TRAIL_BRES_INC               0x1610
#define R128_TRAIL_X                      0x1618
#define R128_TRAIL_X_SUB                  0x1620

#define R128_VCLK_ECP_CNTL                0x0008 /* PLL */
#define R128_VENDOR_ID                    0x0f00 /* PCI */
#define R128_VGA_DDA_CONFIG               0x02e8
#define R128_VGA_DDA_ON_OFF               0x02ec
#define R128_VID_BUFFER_CONTROL           0x0900
#define R128_VIDEOMUX_CNTL                0x0190
#define R128_VIPH_CONTROL                 0x01D0 /* ? */

#define R128_WAIT_UNTIL                   0x1720

#define R128_X_MPLL_REF_FB_DIV            0x000a /* PLL */
#define R128_XCLK_CNTL                    0x000d /* PLL */
#define R128_XDLL_CNTL                    0x000c /* PLL */
#define R128_XPLL_CNTL                    0x000b /* PLL */

				/* Registers for CCE and Microcode Engine */
#define R128_PM4_MICROCODE_ADDR           0x07d4
#define R128_PM4_MICROCODE_RADDR          0x07d8
#define R128_PM4_MICROCODE_DATAH          0x07dc
#define R128_PM4_MICROCODE_DATAL          0x07e0

#define R128_PM4_BUFFER_OFFSET            0x0700
#define R128_PM4_BUFFER_CNTL              0x0704
#       define R128_PM4_NONPM4                 (0  << 28)
#       define R128_PM4_192PIO                 (1  << 28)
#       define R128_PM4_192BM                  (2  << 28)
#       define R128_PM4_128PIO_64INDBM         (3  << 28)
#       define R128_PM4_128BM_64INDBM          (4  << 28)
#       define R128_PM4_64PIO_128INDBM         (5  << 28)
#       define R128_PM4_64BM_128INDBM          (6  << 28)
#       define R128_PM4_64PIO_64VCBM_64INDBM   (7  << 28)
#       define R128_PM4_64BM_64VCBM_64INDBM    (8  << 28)
#       define R128_PM4_64PIO_64VCPIO_64INDPIO (15 << 28)
#define R128_PM4_BUFFER_WM_CNTL           0x0708
#       define R128_WMA_SHIFT                  0
#       define R128_WMB_SHIFT                  8
#       define R128_WMC_SHIFT                 16
#       define R128_WB_WM_SHIFT               24
#define R128_PM4_BUFFER_DL_RPTR_ADDR      0x070c
#define R128_PM4_BUFFER_DL_RPTR           0x0710
#define R128_PM4_BUFFER_DL_WPTR           0x0714
#       define R128_PM4_BUFFER_DL_DONE    (1 << 31)
#define R128_PM4_BUFFER_DL_WPTR_DELAY     0x0718
#       define R128_PRE_WRITE_TIMER_SHIFT      0
#       define R128_PRE_WRITE_LIMIT_SHIFT     23
#define R128_PM4_VC_FPU_SETUP             0x071c
#       define R128_FRONT_DIR_CW          (0 <<  0)
#       define R128_FRONT_DIR_CCW         (1 <<  0)
#       define R128_FRONT_DIR_MASK        (1 <<  0)
#       define R128_BACKFACE_CULL         (0 <<  1)
#       define R128_BACKFACE_POINTS       (1 <<  1)
#       define R128_BACKFACE_LINES        (2 <<  1)
#       define R128_BACKFACE_SOLID        (3 <<  1)
#       define R128_BACKFACE_MASK         (3 <<  1)
#       define R128_FRONTFACE_CULL        (0 <<  3)
#       define R128_FRONTFACE_POINTS      (1 <<  3)
#       define R128_FRONTFACE_LINES       (2 <<  3)
#       define R128_FRONTFACE_SOLID       (3 <<  3)
#       define R128_FRONTFACE_MASK        (3 <<  3)
#       define R128_FPU_COLOR_SOLID       (0 <<  5)
#       define R128_FPU_COLOR_FLAT        (1 <<  5)
#       define R128_FPU_COLOR_GOURAUD     (2 <<  5)
#       define R128_FPU_COLOR_GOURAUD2    (3 <<  5)
#       define R128_FPU_COLOR_MASK        (3 <<  5)
#       define R128_FPU_SUB_PIX_2BITS     (0 <<  7)
#       define R128_FPU_SUB_PIX_4BITS     (1 <<  7)
#       define R128_FPU_MODE_2D           (0 <<  8)
#       define R128_FPU_MODE_3D           (1 <<  8)
#       define R128_TRAP_BITS_DISABLE     (1 <<  9)
#       define R128_EDGE_ANTIALIAS        (1 << 10)
#       define R128_SUPERSAMPLE           (1 << 11)
#       define R128_XFACTOR_2             (0 << 12)
#       define R128_XFACTOR_4             (1 << 12)
#       define R128_YFACTOR_2             (0 << 13)
#       define R128_YFACTOR_4             (1 << 13)
#       define R128_FLAT_SHADE_VERTEX_D3D (0 << 14)
#       define R128_FLAT_SHADE_VERTEX_OGL (1 << 14)
#       define R128_FPU_ROUND_TRUNCATE    (0 << 15)
#       define R128_FPU_ROUND_NEAREST     (1 << 15)
#       define R128_WM_SEL_8DW            (0 << 16)
#       define R128_WM_SEL_16DW           (1 << 16)
#       define R128_WM_SEL_32DW           (2 << 16)
#define R128_PM4_VC_DEBUG_CONFIG          0x07a4
#define R128_PM4_VC_STAT                  0x07a8
#define R128_PM4_VC_TIMESTAMP0            0x07b0
#define R128_PM4_VC_TIMESTAMP1            0x07b4
#define R128_PM4_STAT                     0x07b8
#       define R128_PM4_FIFOCNT_MASK      0x0fff
#       define R128_PM4_BUSY              (1 << 16)
#       define R128_PM4_GUI_ACTIVE        (1 << 31)
#define R128_PM4_BUFFER_ADDR              0x07f0
#define R128_PM4_MICRO_CNTL               0x07fc
#       define R128_PM4_MICRO_FREERUN     (1 << 30)
#define R128_PM4_FIFO_DATA_EVEN           0x1000
#define R128_PM4_FIFO_DATA_ODD            0x1004

#define R128_SCALE_3D_CNTL                0x1a00
#       define R128_SCALE_DITHER_ERR_DIFF         (0  <<  1)
#       define R128_SCALE_DITHER_TABLE            (1  <<  1)
#       define R128_TEX_CACHE_SIZE_FULL           (0  <<  2)
#       define R128_TEX_CACHE_SIZE_HALF           (1  <<  2)
#       define R128_DITHER_INIT_CURR              (0  <<  3)
#       define R128_DITHER_INIT_RESET             (1  <<  3)
#       define R128_ROUND_24BIT                   (1  <<  4)
#       define R128_TEX_CACHE_DISABLE             (1  <<  5)
#       define R128_SCALE_3D_NOOP                 (0  <<  6)
#       define R128_SCALE_3D_SCALE                (1  <<  6)
#       define R128_SCALE_3D_TEXMAP_SHADE         (2  <<  6)
#       define R128_SCALE_PIX_BLEND               (0  <<  8)
#       define R128_SCALE_PIX_REPLICATE           (1  <<  8)
#       define R128_TEX_CACHE_SPLIT               (1  <<  9)
#       define R128_APPLE_YUV_MODE                (1  << 10)
#       define R128_TEX_CACHE_PALLETE_MODE        (1  << 11)
#       define R128_ALPHA_COMB_ADD_CLAMP          (0  << 12)
#       define R128_ALPHA_COMB_ADD_NCLAMP         (1  << 12)
#       define R128_ALPHA_COMB_SUB_SRC_DST_CLAMP  (2  << 12)
#       define R128_ALPHA_COMB_SUB_SRC_DST_NCLAMP (3  << 12)
#       define R128_ALPHA_COMB_FCN_MASK           (3  << 12)
#       define R128_FOG_VERTEX                    (0  << 14)
#       define R128_FOG_TABLE                     (1  << 14)
#       define R128_SIGNED_DST_CLAMP              (1  << 15)

#       define R128_ALPHA_BLEND_ZERO              (0 )
#       define R128_ALPHA_BLEND_ONE               (1 )
#       define R128_ALPHA_BLEND_SRCCOLOR          (2 )
#       define R128_ALPHA_BLEND_INVSRCCOLOR       (3 )
#       define R128_ALPHA_BLEND_SRCALPHA          (4 )
#       define R128_ALPHA_BLEND_INVSRCALPHA       (5 )
#       define R128_ALPHA_BLEND_DSTALPHA          (6 )
#       define R128_ALPHA_BLEND_INVDSTALPHA       (7 )
#       define R128_ALPHA_BLEND_DSTCOLOR          (8 )
#       define R128_ALPHA_BLEND_INVDSTCOLOR       (9 )
#       define R128_ALPHA_BLEND_SAT               (10) /* aka SRCALPHASAT */
#       define R128_ALPHA_BLEND_BLEND             (11) /* aka BOTHSRCALPHA */
#       define R128_ALPHA_BLEND_INVBLEND          (12) /* aka BOTHINVSRCALPHA */
#       define R128_ALPHA_BLEND_MASK              (15)

#       define R128_ALPHA_BLEND_SRC_SHIFT         (16)
#       define R128_ALPHA_BLEND_DST_SHIFT         (20)

#       define R128_ALPHA_TEST_NEVER              (0  << 24)
#       define R128_ALPHA_TEST_LESS               (1  << 24)
#       define R128_ALPHA_TEST_LESSEQUAL          (2  << 24)
#       define R128_ALPHA_TEST_EQUAL              (3  << 24)
#       define R128_ALPHA_TEST_GREATEREQUAL       (4  << 24)
#       define R128_ALPHA_TEST_GREATER            (5  << 24)
#       define R128_ALPHA_TEST_NEQUAL             (6  << 24)
#       define R128_ALPHA_TEST_ALWAYS             (7  << 24)
#       define R128_ALPHA_TEST_MASK               (7  << 24)
#       define R128_COMPOSITE_SHADOW_CMP_EQUAL    (0  << 28)
#       define R128_COMPOSITE_SHADOW_CMP_NEQUAL   (1  << 28)
#       define R128_COMPOSITE_SHADOW              (1  << 29)
#       define R128_TEX_MAP_ALPHA_IN_TEXTURE      (1  << 30)
#       define R128_TEX_CACHE_LINE_SIZE_8QW       (0  << 31)
#       define R128_TEX_CACHE_LINE_SIZE_4QW       (1  << 31)
#define R128_SCALE_3D_DATATYPE            0x1a20

#define R128_SETUP_CNTL                   0x1bc4
#       define R128_DONT_START_TRIANGLE   (1 <<  0)
#       define R128_Z_BIAS                (0 <<  1)
#       define R128_DONT_START_ANY_ON     (1 <<  2)
#       define R128_COLOR_SOLID_COLOR     (0 <<  3)
#       define R128_COLOR_FLAT_VERT_1     (1 <<  3)
#       define R128_COLOR_FLAT_VERT_2     (2 <<  3)
#       define R128_COLOR_FLAT_VERT_3     (3 <<  3)
#       define R128_COLOR_GOURAUD         (4 <<  3)
#       define R128_PRIM_TYPE_TRI         (0 <<  7)
#       define R128_PRIM_TYPE_LINE        (1 <<  7)
#       define R128_PRIM_TYPE_POINT       (2 <<  7)
#       define R128_PRIM_TYPE_POLY_EDGE   (3 <<  7)
#       define R128_TEXTURE_ST_MULT_W     (0 <<  9)
#       define R128_TEXTURE_ST_DIRECT     (1 <<  9)
#       define R128_STARTING_VERTEX_1     (1 << 14)
#       define R128_STARTING_VERTEX_2     (2 << 14)
#       define R128_STARTING_VERTEX_3     (3 << 14)
#       define R128_ENDING_VERTEX_1       (1 << 16)
#       define R128_ENDING_VERTEX_2       (2 << 16)
#       define R128_ENDING_VERTEX_3       (3 << 16)
#       define R128_SU_POLY_LINE_LAST     (0 << 18)
#       define R128_SU_POLY_LINE_NOT_LAST (1 << 18)
#       define R128_SUB_PIX_2BITS         (0 << 19)
#       define R128_SUB_PIX_4BITS         (1 << 19)
#       define R128_SET_UP_CONTINUE       (1 << 31)

#define R128_WINDOW_XY_OFFSET             0x1bcc
#       define R128_WINDOW_Y_SHIFT        4
#       define R128_WINDOW_X_SHIFT        20

#define R128_Z_OFFSET_C                   0x1c90
#define R128_Z_PITCH_C                    0x1c94
#       define R128_Z_TILE                    (1 << 16)
#define R128_Z_STEN_CNTL_C                0x1c98
#       define R128_Z_PIX_WIDTH_16            (0 <<  1)
#       define R128_Z_PIX_WIDTH_24            (1 <<  1)
#       define R128_Z_PIX_WIDTH_32            (2 <<  1)
#       define R128_Z_PIX_WIDTH_MASK          (3 <<  1)
#       define R128_Z_TEST_NEVER              (0 <<  4)
#       define R128_Z_TEST_LESS               (1 <<  4)
#       define R128_Z_TEST_LESSEQUAL          (2 <<  4)
#       define R128_Z_TEST_EQUAL              (3 <<  4)
#       define R128_Z_TEST_GREATEREQUAL       (4 <<  4)
#       define R128_Z_TEST_GREATER            (5 <<  4)
#       define R128_Z_TEST_NEQUAL             (6 <<  4)
#       define R128_Z_TEST_ALWAYS             (7 <<  4)
#       define R128_Z_TEST_MASK               (7 <<  4)
#       define R128_STENCIL_TEST_NEVER        (0 << 12)
#       define R128_STENCIL_TEST_LESS         (1 << 12)
#       define R128_STENCIL_TEST_LESSEQUAL    (2 << 12)
#       define R128_STENCIL_TEST_EQUAL        (3 << 12)
#       define R128_STENCIL_TEST_GREATEREQUAL (4 << 12)
#       define R128_STENCIL_TEST_GREATER      (5 << 12)
#       define R128_STENCIL_TEST_NEQUAL       (6 << 12)
#       define R128_STENCIL_TEST_ALWAYS       (7 << 12)
#       define R128_STENCIL_TEST_MASK         (7 << 12)
#       define R128_STENCIL_S_FAIL_KEEP       (0 << 16)
#       define R128_STENCIL_S_FAIL_ZERO       (1 << 16)
#       define R128_STENCIL_S_FAIL_REPLACE    (2 << 16)
#       define R128_STENCIL_S_FAIL_INC        (3 << 16)
#       define R128_STENCIL_S_FAIL_DEC        (4 << 16)
#       define R128_STENCIL_S_FAIL_INV        (5 << 16)
#       define R128_STENCIL_S_FAIL_INC_WRAP   (6 << 16)	/* GUESS */
#       define R128_STENCIL_S_FAIL_DEC_WRAP   (7 << 16)	/* GUESS */
#       define R128_STENCIL_S_FAIL_MASK       (7 << 16)
#       define R128_STENCIL_ZPASS_KEEP        (0 << 20)
#       define R128_STENCIL_ZPASS_ZERO        (1 << 20)
#       define R128_STENCIL_ZPASS_REPLACE     (2 << 20)
#       define R128_STENCIL_ZPASS_INC         (3 << 20)
#       define R128_STENCIL_ZPASS_DEC         (4 << 20)
#       define R128_STENCIL_ZPASS_INV         (5 << 20)
#       define R128_STENCIL_ZPASS_INC_WRAP    (6 << 20)	/* GUESS */
#       define R128_STENCIL_ZPASS_DEC_WRAP    (7 << 20)	/* GUESS */
#       define R128_STENCIL_ZPASS_MASK        (7 << 20)
#       define R128_STENCIL_ZFAIL_KEEP        (0 << 24)
#       define R128_STENCIL_ZFAIL_ZERO        (1 << 24)
#       define R128_STENCIL_ZFAIL_REPLACE     (2 << 24)
#       define R128_STENCIL_ZFAIL_INC         (3 << 24)
#       define R128_STENCIL_ZFAIL_DEC         (4 << 24)
#       define R128_STENCIL_ZFAIL_INV         (5 << 24)
#       define R128_STENCIL_ZFAIL_INC_WRAP    (6 << 24)	/* GUESS */
#       define R128_STENCIL_ZFAIL_DEC_WRAP    (7 << 24)	/* GUESS */
#       define R128_STENCIL_ZFAIL_MASK        (7 << 24)
#define R128_TEX_CNTL_C                   0x1c9c
#       define R128_Z_ENABLE                   (1 <<  0)
#       define R128_Z_WRITE_ENABLE             (1 <<  1)
#       define R128_STENCIL_ENABLE             (1 <<  3)
#       define R128_SHADE_ENABLE               (0 <<  4)
#       define R128_TEXMAP_ENABLE              (1 <<  4)
#       define R128_SEC_TEXMAP_ENABLE          (1 <<  5)
#       define R128_FOG_ENABLE                 (1 <<  7)
#       define R128_DITHER_ENABLE              (1 <<  8)
#       define R128_ALPHA_ENABLE               (1 <<  9)
#       define R128_ALPHA_TEST_ENABLE          (1 << 10)
#       define R128_SPEC_LIGHT_ENABLE          (1 << 11)
#       define R128_TEX_CHROMA_KEY_ENABLE      (1 << 12)
#       define R128_ALPHA_IN_TEX_COMPLETE_A    (0 << 13)
#       define R128_ALPHA_IN_TEX_LSB_A         (1 << 13)
#       define R128_LIGHT_DIS                  (0 << 14)
#       define R128_LIGHT_COPY                 (1 << 14)
#       define R128_LIGHT_MODULATE             (2 << 14)
#       define R128_LIGHT_ADD                  (3 << 14)
#       define R128_LIGHT_BLEND_CONSTANT       (4 << 14)
#       define R128_LIGHT_BLEND_TEXTURE        (5 << 14)
#       define R128_LIGHT_BLEND_VERTEX         (6 << 14)
#       define R128_LIGHT_BLEND_CONST_COLOR    (7 << 14)
#       define R128_ALPHA_LIGHT_DIS            (0 << 18)
#       define R128_ALPHA_LIGHT_COPY           (1 << 18)
#       define R128_ALPHA_LIGHT_MODULATE       (2 << 18)
#       define R128_ALPHA_LIGHT_ADD            (3 << 18)
#       define R128_ANTI_ALIAS                 (1 << 21)
#       define R128_TEX_CACHE_FLUSH            (1 << 23)
#       define R128_LOD_BIAS_SHIFT             24
#       define R128_LOD_BIAS_MASK              (0xff << 24)
#define R128_MISC_3D_STATE_CNTL_REG       0x1ca0
#       define R128_REF_ALPHA_MASK                  0xff
#       define R128_MISC_SCALE_3D_NOOP              (0  <<  8)
#       define R128_MISC_SCALE_3D_SCALE             (1  <<  8)
#       define R128_MISC_SCALE_3D_TEXMAP_SHADE      (2  <<  8)
#       define R128_MISC_SCALE_PIX_BLEND            (0  << 10)
#       define R128_MISC_SCALE_PIX_REPLICATE        (1  << 10)
/* Bits [14:12] are the same as R128_SCALE_3D_CNTL */
/* Bit  [15]    is unknown */
/* Bits [26:16] are the same as R128_SCALE_3D_CNTL */
/* Bits [31:27] are unknown */

#define R128_TEXTURE_CLR_CMP_CLR_C        0x1ca4
#define R128_TEXTURE_CLR_CMP_MSK_C        0x1ca8
#define R128_FOG_COLOR_C                  0x1cac
#       define R128_FOG_BLUE_SHIFT             0
#       define R128_FOG_GREEN_SHIFT            8
#       define R128_FOG_RED_SHIFT             16
#define R128_PRIM_TEX_CNTL_C              0x1cb0
#       define R128_MIN_BLEND_NEAREST          (0  <<  1)
#       define R128_MIN_BLEND_LINEAR           (1  <<  1)
#       define R128_MIN_BLEND_MIPNEAREST       (2  <<  1)
#       define R128_MIN_BLEND_MIPLINEAR        (3  <<  1)
#       define R128_MIN_BLEND_LINEARMIPNEAREST (4  <<  1)
#       define R128_MIN_BLEND_LINEARMIPLINEAR  (5  <<  1)
#       define R128_MIN_BLEND_MASK             (7  <<  1)
#       define R128_MAG_BLEND_NEAREST          (0  <<  4)
#       define R128_MAG_BLEND_LINEAR           (1  <<  4)
#       define R128_MAG_BLEND_MASK             (7  <<  4)
#       define R128_MIP_MAP_DISABLE            (1  <<  7)
#       define R128_TEX_CLAMP_S_WRAP           (0  <<  8)
#       define R128_TEX_CLAMP_S_MIRROR         (1  <<  8)
#       define R128_TEX_CLAMP_S_CLAMP          (2  <<  8)
#       define R128_TEX_CLAMP_S_BORDER_COLOR   (3  <<  8)
#       define R128_TEX_CLAMP_S_MASK           (3  <<  8)
#       define R128_TEX_WRAP_S                 (1  << 10)
#       define R128_TEX_CLAMP_T_WRAP           (0  << 11)
#       define R128_TEX_CLAMP_T_MIRROR         (1  << 11)
#       define R128_TEX_CLAMP_T_CLAMP          (2  << 11)
#       define R128_TEX_CLAMP_T_BORDER_COLOR   (3  << 11)
#       define R128_TEX_CLAMP_T_MASK           (3  << 11)
#       define R128_TEX_WRAP_T                 (1  << 13)
#       define R128_TEX_PERSPECTIVE_DISABLE    (1  << 14)
#       define R128_DATATYPE_VQ                (0  << 16)
#       define R128_DATATYPE_CI4               (1  << 16)
#       define R128_DATATYPE_CI8               (2  << 16)
#       define R128_DATATYPE_ARGB1555          (3  << 16)
#       define R128_DATATYPE_RGB565            (4  << 16)
#       define R128_DATATYPE_RGB888            (5  << 16)
#       define R128_DATATYPE_ARGB8888          (6  << 16)
#       define R128_DATATYPE_RGB332            (7  << 16)
#       define R128_DATATYPE_Y8                (8  << 16)
#       define R128_DATATYPE_RGB8              (9  << 16)
#       define R128_DATATYPE_CI16              (10 << 16)
#       define R128_DATATYPE_YVYU422           (11 << 16)
#       define R128_DATATYPE_VYUY422           (12 << 16)
#       define R128_DATATYPE_AYUV444           (14 << 16)
#       define R128_DATATYPE_ARGB4444          (15 << 16)
#       define R128_PALLETE_EITHER             (0  << 20)
#       define R128_PALLETE_1                  (1  << 20)
#       define R128_PALLETE_2                  (2  << 20)
#       define R128_PSEUDOCOLOR_DT_RGB565      (0  << 24)
#       define R128_PSEUDOCOLOR_DT_ARGB1555    (1  << 24)
#       define R128_PSEUDOCOLOR_DT_ARGB4444    (2  << 24)
#define R128_PRIM_TEXTURE_COMBINE_CNTL_C  0x1cb4
#       define R128_COMB_DIS                   (0  <<  0)
#       define R128_COMB_COPY                  (1  <<  0)
#       define R128_COMB_COPY_INP              (2  <<  0)
#       define R128_COMB_MODULATE              (3  <<  0)
#       define R128_COMB_MODULATE2X            (4  <<  0)
#       define R128_COMB_MODULATE4X            (5  <<  0)
#       define R128_COMB_ADD                   (6  <<  0)
#       define R128_COMB_ADD_SIGNED            (7  <<  0)
#       define R128_COMB_BLEND_VERTEX          (8  <<  0)
#       define R128_COMB_BLEND_TEXTURE         (9  <<  0)
#       define R128_COMB_BLEND_CONST           (10 <<  0)
#       define R128_COMB_BLEND_PREMULT         (11 <<  0)
#       define R128_COMB_BLEND_PREV            (12 <<  0)
#       define R128_COMB_BLEND_PREMULT_INV     (13 <<  0)
#       define R128_COMB_ADD_SIGNED2X          (14 <<  0)
#       define R128_COMB_BLEND_CONST_COLOR     (15 <<  0)
#       define R128_COMB_MASK                  (15 <<  0)
#       define R128_COLOR_FACTOR_CONST_COLOR   (0  <<  4)
#       define R128_COLOR_FACTOR_NCONST_COLOR  (1  <<  4)
#       define R128_COLOR_FACTOR_TEX           (4  <<  4)
#       define R128_COLOR_FACTOR_NTEX          (5  <<  4)
#       define R128_COLOR_FACTOR_ALPHA         (6  <<  4)
#       define R128_COLOR_FACTOR_NALPHA        (7  <<  4)
#       define R128_COLOR_FACTOR_PREV_COLOR    (8  <<  4)
#       define R128_COLOR_FACTOR_MASK          (15 <<  4)
#       define R128_COMB_FCN_MSB               (1  <<  8)
#       define R128_INPUT_FACTOR_CONST_COLOR   (2  << 10)
#       define R128_INPUT_FACTOR_CONST_ALPHA   (3  << 10)
#       define R128_INPUT_FACTOR_INT_COLOR     (4  << 10)
#       define R128_INPUT_FACTOR_INT_ALPHA     (5  << 10)
#       define R128_INPUT_FACTOR_MASK          (15 << 10)
#       define R128_COMB_ALPHA_DIS             (0  << 14)
#       define R128_COMB_ALPHA_COPY            (1  << 14)
#       define R128_COMB_ALPHA_COPY_INP        (2  << 14)
#       define R128_COMB_ALPHA_MODULATE        (3  << 14)
#       define R128_COMB_ALPHA_MODULATE2X      (4  << 14)
#       define R128_COMB_ALPHA_MODULATE4X      (5  << 14)
#       define R128_COMB_ALPHA_ADD             (6  << 14)
#       define R128_COMB_ALPHA_ADD_SIGNED      (7  << 14)
#       define R128_COMB_ALPHA_ADD_SIGNED2X    (14 << 14)
#       define R128_COMB_ALPHA_MASK            (15 << 14)
#       define R128_ALPHA_FACTOR_TEX_ALPHA     (6  << 18)
#       define R128_ALPHA_FACTOR_NTEX_ALPHA    (7  << 18)
#       define R128_ALPHA_FACTOR_MASK          (15 << 18)
#       define R128_INP_FACTOR_A_CONST_ALPHA   (1  << 25)
#       define R128_INP_FACTOR_A_INT_ALPHA     (2  << 25)
#       define R128_INP_FACTOR_A_MASK          (7  << 25)
#define R128_TEX_SIZE_PITCH_C             0x1cb8
#       define R128_TEX_PITCH_SHIFT           0
#       define R128_TEX_SIZE_SHIFT            4
#       define R128_TEX_HEIGHT_SHIFT          8
#       define R128_TEX_MIN_SIZE_SHIFT       12
#       define R128_SEC_TEX_PITCH_SHIFT      16
#       define R128_SEC_TEX_SIZE_SHIFT       20
#       define R128_SEC_TEX_HEIGHT_SHIFT     24
#       define R128_SEC_TEX_MIN_SIZE_SHIFT   28
#       define R128_TEX_PITCH_MASK           (0x0f <<  0)
#       define R128_TEX_SIZE_MASK            (0x0f <<  4)
#       define R128_TEX_HEIGHT_MASK          (0x0f <<  8)
#       define R128_TEX_MIN_SIZE_MASK        (0x0f << 12)
#       define R128_SEC_TEX_PITCH_MASK       (0x0f << 16)
#       define R128_SEC_TEX_SIZE_MASK        (0x0f << 20)
#       define R128_SEC_TEX_HEIGHT_MASK      (0x0f << 24)
#       define R128_SEC_TEX_MIN_SIZE_MASK    (0x0f << 28)
#       define R128_TEX_SIZE_PITCH_SHIFT      0
#       define R128_SEC_TEX_SIZE_PITCH_SHIFT 16
#       define R128_TEX_SIZE_PITCH_MASK      (0xffff <<  0)
#       define R128_SEC_TEX_SIZE_PITCH_MASK  (0xffff << 16)
#define R128_PRIM_TEX_0_OFFSET_C          0x1cbc
#define R128_PRIM_TEX_1_OFFSET_C          0x1cc0
#define R128_PRIM_TEX_2_OFFSET_C          0x1cc4
#define R128_PRIM_TEX_3_OFFSET_C          0x1cc8
#define R128_PRIM_TEX_4_OFFSET_C          0x1ccc
#define R128_PRIM_TEX_5_OFFSET_C          0x1cd0
#define R128_PRIM_TEX_6_OFFSET_C          0x1cd4
#define R128_PRIM_TEX_7_OFFSET_C          0x1cd8
#define R128_PRIM_TEX_8_OFFSET_C          0x1cdc
#define R128_PRIM_TEX_9_OFFSET_C          0x1ce0
#define R128_PRIM_TEX_10_OFFSET_C         0x1ce4
#       define R128_TEX_NO_TILE           (0 << 30)
#       define R128_TEX_TILED_BY_HOST     (1 << 30)
#       define R128_TEX_TILED_BY_STORAGE  (2 << 30)
#       define R128_TEX_TILED_BY_STORAGE2 (3 << 30)

#define R128_SEC_TEX_CNTL_C               0x1d00
#       define R128_SEC_SELECT_PRIM_ST    (0  <<  0)
#       define R128_SEC_SELECT_SEC_ST     (1  <<  0)
#define R128_SEC_TEX_COMBINE_CNTL_C       0x1d04
#       define R128_INPUT_FACTOR_PREV_COLOR (8  << 10)
#       define R128_INPUT_FACTOR_PREV_ALPHA (9  << 10)
#       define R128_INP_FACTOR_A_PREV_ALPHA (4  << 25)
#define R128_SEC_TEX_0_OFFSET_C           0x1d08
#define R128_SEC_TEX_1_OFFSET_C           0x1d0c
#define R128_SEC_TEX_2_OFFSET_C           0x1d10
#define R128_SEC_TEX_3_OFFSET_C           0x1d14
#define R128_SEC_TEX_4_OFFSET_C           0x1d18
#define R128_SEC_TEX_5_OFFSET_C           0x1d1c
#define R128_SEC_TEX_6_OFFSET_C           0x1d20
#define R128_SEC_TEX_7_OFFSET_C           0x1d24
#define R128_SEC_TEX_8_OFFSET_C           0x1d28
#define R128_SEC_TEX_9_OFFSET_C           0x1d2c
#define R128_SEC_TEX_10_OFFSET_C          0x1d30
#define R128_CONSTANT_COLOR_C             0x1d34
#       define R128_CONSTANT_BLUE_SHIFT        0
#       define R128_CONSTANT_GREEN_SHIFT       8
#       define R128_CONSTANT_RED_SHIFT        16
#       define R128_CONSTANT_ALPHA_SHIFT      24
#define R128_PRIM_TEXTURE_BORDER_COLOR_C  0x1d38
#       define R128_PRIM_TEX_BORDER_BLUE_SHIFT   0
#       define R128_PRIM_TEX_BORDER_GREEN_SHIFT  8
#       define R128_PRIM_TEX_BORDER_RED_SHIFT   16
#       define R128_PRIM_TEX_BORDER_ALPHA_SHIFT 24
#define R128_SEC_TEXTURE_BORDER_COLOR_C   0x1d3c
#       define R128_SEC_TEX_BORDER_BLUE_SHIFT   0
#       define R128_SEC_TEX_BORDER_GREEN_SHIFT  8
#       define R128_SEC_TEX_BORDER_RED_SHIFT   16
#       define R128_SEC_TEX_BORDER_ALPHA_SHIFT 24
#define R128_STEN_REF_MASK_C              0x1d40
#       define R128_STEN_REFERENCE_SHIFT       0
#       define R128_STEN_MASK_SHIFT           16
#       define R128_STEN_WRITE_MASK_SHIFT     24
#define R128_PLANE_3D_MASK_C              0x1d44
#define R128_TEX_CACHE_STAT_COUNT         0x1974


				/* Constants */
#define R128_AGP_TEX_OFFSET               0x02000000

#define R128_LAST_FRAME_REG               R128_GUI_SCRATCH_REG0

				/* CCE packet types */
#define R128_CCE_PACKET0                         0x00000000
#define R128_CCE_PACKET0_ONE_REG_WR              0x00008000
#define R128_CCE_PACKET1                         0x40000000
#define R128_CCE_PACKET2                         0x80000000
#define R128_CCE_PACKET3                         0xC0000000
#define R128_CCE_PACKET3_NOP                     0xC0001000
#define R128_CCE_PACKET3_PAINT                   0xC0001100
#define R128_CCE_PACKET3_BITBLT                  0xC0001200
#define R128_CCE_PACKET3_SMALLTEXT               0xC0001300
#define R128_CCE_PACKET3_HOSTDATA_BLT            0xC0001400
#define R128_CCE_PACKET3_POLYLINE                0xC0001500
#define R128_CCE_PACKET3_SCALING                 0xC0001600
#define R128_CCE_PACKET3_TRANS_SCALING           0xC0001700
#define R128_CCE_PACKET3_POLYSCANLINES           0xC0001800
#define R128_CCE_PACKET3_NEXT_CHAR               0xC0001900
#define R128_CCE_PACKET3_PAINT_MULTI             0xC0001A00
#define R128_CCE_PACKET3_BITBLT_MULTI            0xC0001B00
#define R128_CCE_PACKET3_PLY_NEXTSCAN            0xC0001D00
#define R128_CCE_PACKET3_SET_SCISSORS            0xC0001E00
#define R128_CCE_PACKET3_SET_MODE24BPP           0xC0001F00
#define R128_CCE_PACKET3_CNTL_PAINT              0xC0009100
#define R128_CCE_PACKET3_CNTL_BITBLT             0xC0009200
#define R128_CCE_PACKET3_CNTL_SMALLTEXT          0xC0009300
#define R128_CCE_PACKET3_CNTL_HOSTDATA_BLT       0xC0009400
#define R128_CCE_PACKET3_CNTL_POLYLINE           0xC0009500
#define R128_CCE_PACKET3_CNTL_SCALING            0xC0009600
#define R128_CCE_PACKET3_CNTL_TRANS_SCALING      0xC0009700
#define R128_CCE_PACKET3_CNTL_POLYSCANLINES      0xC0009800
#define R128_CCE_PACKET3_CNTL_NEXT_CHAR          0xC0009900
#define R128_CCE_PACKET3_CNTL_PAINT_MULTI        0xC0009A00
#define R128_CCE_PACKET3_CNTL_BITBLT_MULTI       0xC0009B00
#define R128_CCE_PACKET3_CNTL_TRANS_BITBLT       0xC0009C00
#define R128_CCE_PACKET3_3D_SAVE_CONTEXT         0xC0002000
#define R128_CCE_PACKET3_3D_PLAY_CONTEXT         0xC0002100
#define R128_CCE_PACKET3_3D_RNDR_GEN_INDX_PRIM   0xC0002300
#define R128_CCE_PACKET3_3D_RNDR_GEN_PRIM        0xC0002500
#define R128_CCE_PACKET3_LOAD_PALETTE            0xC0002C00
#define R128_CCE_PACKET3_PURGE                   0xC0002D00
#define R128_CCE_PACKET3_NEXT_VERTEX_BUNDLE      0xC0002E00
#       define R128_CCE_PACKET_MASK              0xC0000000
#       define R128_CCE_PACKET_COUNT_MASK        0x3fff0000
#       define R128_CCE_PACKET_MAX_DWORDS        (1 << 12)
#       define R128_CCE_PACKET0_REG_MASK         0x000007ff
#       define R128_CCE_PACKET1_REG0_MASK        0x000007ff
#       define R128_CCE_PACKET1_REG1_MASK        0x003ff800

#define R128_CCE_VC_FRMT_RHW                     0x00000001
#define R128_CCE_VC_FRMT_DIFFUSE_BGR             0x00000002
#define R128_CCE_VC_FRMT_DIFFUSE_A               0x00000004
#define R128_CCE_VC_FRMT_DIFFUSE_ARGB            0x00000008
#define R128_CCE_VC_FRMT_SPEC_BGR                0x00000010
#define R128_CCE_VC_FRMT_SPEC_F                  0x00000020
#define R128_CCE_VC_FRMT_SPEC_FRGB               0x00000040
#define R128_CCE_VC_FRMT_S_T                     0x00000080
#define R128_CCE_VC_FRMT_S2_T2                   0x00000100
#define R128_CCE_VC_FRMT_RHW2                    0x00000200

#define R128_CCE_VC_CNTL_PRIM_TYPE_NONE          0x00000000
#define R128_CCE_VC_CNTL_PRIM_TYPE_POINT         0x00000001
#define R128_CCE_VC_CNTL_PRIM_TYPE_LINE          0x00000002
#define R128_CCE_VC_CNTL_PRIM_TYPE_POLY_LINE     0x00000003
#define R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST      0x00000004
#define R128_CCE_VC_CNTL_PRIM_TYPE_TRI_FAN       0x00000005
#define R128_CCE_VC_CNTL_PRIM_TYPE_TRI_STRIP     0x00000006
#define R128_CCE_VC_CNTL_PRIM_TYPE_TRI_TYPE2     0x00000007
#define R128_CCE_VC_CNTL_PRIM_WALK_IND           0x00000010
#define R128_CCE_VC_CNTL_PRIM_WALK_LIST          0x00000020
#define R128_CCE_VC_CNTL_PRIM_WALK_RING          0x00000030
#define R128_CCE_VC_CNTL_NUM_SHIFT               16

/* hmm copyed blindly (no specs) from radeon.h ... */
#define R128_RE_TOP_LEFT                  0x26c0
#       define R128_RE_LEFT_SHIFT         0
#       define R128_RE_TOP_SHIFT          16
#define R128_RE_WIDTH_HEIGHT              0x1c44
#       define R128_RE_WIDTH_SHIFT        0
#       define R128_RE_HEIGHT_SHIFT       16

#endif
