/* $XConsortium: mgareg.h /main/2 1996/10/25 10:33:21 kaleb $ */



/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_reg.h,v 1.18 2001/09/26 12:59:18 alanh Exp $ */



/*
 * MGA Millennium (MGA2064W) functions
 * MGA Mystique (MGA1064SG) functions
 *
 * Copyright 1996 The XFree86 Project, Inc.
 *
 * Authors
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		David Dawes
 *			dawes@XFree86.Org
 * Contributors:
 *		Guy DESBIEF, Aix-en-provence, France
 *			g.desbief@aix.pacwan.net
 *		MGA1064SG Mystique register file
 */


#ifndef _MGA_REG_H_
#define _MGA_REG_H_

#define	MGAREG_DWGCTL		0x1c00
#define	MGAREG_MACCESS		0x1c04
#define	MGA_MACCESS_PW16        0x00000001
#define	MGA_MACCESS_PW32        0x00000002
/* the following is a mystique only register */
#define MGAREG_MCTLWTST		0x1c08
#define	MGAREG_ZORG		0x1c0c

#define	MGAREG_PAT0		0x1c10
#define	MGAREG_PAT1		0x1c14
#define	MGAREG_PLNWT		0x1c1c

#define	MGAREG_BCOL		0x1c20
#define	MGAREG_FCOL		0x1c24

#define	MGAREG_SRC0		0x1c30
#define	MGAREG_SRC1		0x1c34
#define	MGAREG_SRC2		0x1c38
#define	MGAREG_SRC3		0x1c3c

#define	MGAREG_XYSTRT		0x1c40
#define	MGAREG_XYEND		0x1c44

#define	MGAREG_SHIFT		0x1c50
/* the following is a mystique only register */
#define MGAREG_DMAPAD		0x1c54
#define	MGAREG_SGN		0x1c58
#define	MGAREG_LEN		0x1c5c

#define	MGAREG_AR0		0x1c60
#define	MGAREG_AR1		0x1c64
#define	MGAREG_AR2		0x1c68
#define	MGAREG_AR3		0x1c6c
#define	MGAREG_AR4		0x1c70
#define	MGAREG_AR5		0x1c74
#define	MGAREG_AR6		0x1c78

#define	MGAREG_CXBNDRY		0x1c80
#define	MGAREG_FXBNDRY		0x1c84
#define	MGAREG_YDSTLEN		0x1c88
#define	MGAREG_PITCH		0x1c8c

#define	MGAREG_YDST		0x1c90
#define	MGAREG_YDSTORG		0x1c94
#define	MGAREG_YTOP		0x1c98
#define	MGAREG_YBOT		0x1c9c

#define	MGAREG_CXLEFT		0x1ca0
#define	MGAREG_CXRIGHT		0x1ca4
#define	MGAREG_FXLEFT		0x1ca8
#define	MGAREG_FXRIGHT		0x1cac

#define	MGAREG_XDST		0x1cb0

#define	MGAREG_DR0		0x1cc0
#define	MGAREG_DR1		0x1cc4
#define	MGAREG_DR2		0x1cc8
#define	MGAREG_DR3		0x1ccc

#define	MGAREG_DR4		0x1cd0
#define	MGAREG_DR5		0x1cd4
#define	MGAREG_DR6		0x1cd8
#define	MGAREG_DR7		0x1cdc

#define	MGAREG_DR8		0x1ce0
#define	MGAREG_DR9		0x1ce4
#define	MGAREG_DR10		0x1ce8
#define	MGAREG_DR11		0x1cec

#define	MGAREG_DR12		0x1cf0
#define	MGAREG_DR13		0x1cf4
#define	MGAREG_DR14		0x1cf8
#define	MGAREG_DR15		0x1cfc

#define MGAREG_SRCORG		0x2cb4
#define MGAREG_DSTORG		0x2cb8

/* add or or this to one of the previous "power registers" to start
   the drawing engine */

#define MGAREG_EXEC		0x0100

#define	MGAREG_FIFOSTATUS	0x1e10
#define	MGAREG_Status		0x1e14
#define	MGAREG_ICLEAR		0x1e18
#define	MGAREG_IEN		0x1e1c

#define	MGAREG_VCOUNT		0x1e20

#define	MGAREG_Reset		0x1e40

#define	MGAREG_OPMODE		0x1e54

/* Warp Registers */
#define MGAREG_WIADDR           0x1dc0
#define MGAREG_WIADDR2          0x1dd8
#define MGAREG_WGETMSB          0x1dc8
#define MGAREG_WVRTXSZ          0x1dcc
#define MGAREG_WACCEPTSEQ       0x1dd4
#define MGAREG_WMISC            0x1e70

/* OPMODE register additives */

#define MGAOPM_DMA_GENERAL	(0x00 << 2)
#define MGAOPM_DMA_BLIT		(0x01 << 2)
#define MGAOPM_DMA_VECTOR	(0x10 << 2)

/* DWGCTL register additives */

/* Lines */

#define MGADWG_LINE_OPEN	0x00
#define MGADWG_AUTOLINE_OPEN	0x01
#define MGADWG_LINE_CLOSE	0x02
#define MGADWG_AUTOLINE_CLOSE	0x03

/* Trapezoids */
#define MGADWG_TRAP		0x04
#define MGADWG_TEXTURE_TRAP	0x05

/* BitBlts */

#define MGADWG_BITBLT		0x08
#define MGADWG_FBITBLT		0x0c
#define MGADWG_ILOAD		0x09
#define MGADWG_ILOAD_SCALE	0x0d
#define MGADWG_ILOAD_FILTER	0x0f
#define MGADWG_ILOAD_HIQH	0x07
#define MGADWG_ILOAD_HIQHV	0x0e
#define MGADWG_IDUMP		0x0a

/* atype access to WRAM */

#define MGADWG_RPL		( 0x00 << 4 )
#define MGADWG_RSTR		( 0x01 << 4 )
#define MGADWG_ZI		( 0x03 << 4 )
#define MGADWG_BLK 		( 0x04 << 4 )
#define MGADWG_I		( 0x07 << 4 )

/* specifies whether bit blits are linear or xy */
#define MGADWG_LINEAR		( 0x01 << 7 )

/* z drawing mode. use MGADWG_NOZCMP for always */

#define MGADWG_NOZCMP		( 0x00 << 8 )
#define MGADWG_ZE		( 0x02 << 8 )
#define MGADWG_ZNE		( 0x03 << 8 )
#define MGADWG_ZLT		( 0x04 << 8 )
#define MGADWG_ZLTE		( 0x05 << 8 )
#define MGADWG_GT		( 0x06 << 8 )
#define MGADWG_GTE		( 0x07 << 8 )

/* use this to force colour expansion circuitry to do its stuff */

#define MGADWG_SOLID		( 0x01 << 11 )

/* ar register at zero */

#define MGADWG_ARZERO		( 0x01 << 12 )

#define MGADWG_SGNZERO		( 0x01 << 13 )

#define MGADWG_SHIFTZERO	( 0x01 << 14 )

/* See table on 4-43 for bop ALU operations */

/* See table on 4-44 for translucidity masks */

#define MGADWG_BMONOLEF		( 0x00 << 25 )
#define MGADWG_BMONOWF		( 0x04 << 25 )
#define MGADWG_BPLAN		( 0x01 << 25 )

/* note that if bfcol is specified and you're doing a bitblt, it causes
   a fbitblt to be performed, so check that you obey the fbitblt rules */

#define MGADWG_BFCOL   		( 0x02 << 25 )
#define MGADWG_BUYUV		( 0x0e << 25 )
#define MGADWG_BU32BGR		( 0x03 << 25 )
#define MGADWG_BU32RGB		( 0x07 << 25 )
#define MGADWG_BU24BGR		( 0x0b << 25 )
#define MGADWG_BU24RGB		( 0x0f << 25 )

#define MGADWG_PATTERN		( 0x01 << 29 )
#define MGADWG_TRANSC		( 0x01 << 30 )
#define MGAREG_MISC_WRITE	0x3c2
#define MGAREG_MISC_READ	0x3cc
#define MGAREG_MISC_IOADSEL	(0x1 << 0)
#define MGAREG_MISC_RAMMAPEN	(0x1 << 1)
#define MGAREG_MISC_CLK_SEL_VGA25	(0x0 << 2)
#define MGAREG_MISC_CLK_SEL_VGA28	(0x1 << 2)
#define MGAREG_MISC_CLK_SEL_MGA_PIX	(0x2 << 2)
#define MGAREG_MISC_CLK_SEL_MGA_MSK	(0x3 << 2)
#define MGAREG_MISC_VIDEO_DIS	(0x1 << 4)
#define MGAREG_MISC_HIGH_PG_SEL	(0x1 << 5)

/* MMIO VGA registers */
#define MGAREG_SEQ_INDEX	0x1fc4
#define MGAREG_SEQ_DATA		0x1fc5
#define MGAREG_CRTC_INDEX	0x1fd4
#define MGAREG_CRTC_DATA	0x1fd5
#define MGAREG_CRTCEXT_INDEX	0x1fde
#define MGAREG_CRTCEXT_DATA	0x1fdf



/* MGA bits for registers PCI_OPTION_REG */
#define MGA1064_OPT_SYS_CLK_PCI   		( 0x00 << 0 )
#define MGA1064_OPT_SYS_CLK_PLL   		( 0x01 << 0 )
#define MGA1064_OPT_SYS_CLK_EXT   		( 0x02 << 0 )
#define MGA1064_OPT_SYS_CLK_MSK   		( 0x03 << 0 )

#define MGA1064_OPT_SYS_CLK_DIS   		( 0x01 << 2 )
#define MGA1064_OPT_G_CLK_DIV_1   		( 0x01 << 3 )
#define MGA1064_OPT_M_CLK_DIV_1   		( 0x01 << 4 )

#define MGA1064_OPT_SYS_PLL_PDN   		( 0x01 << 5 )
#define MGA1064_OPT_VGA_ION   		( 0x01 << 8 )

/* MGA registers in PCI config space */
#define PCI_MGA_INDEX		0x44
#define PCI_MGA_DATA		0x48
#define PCI_MGA_OPTION2		0x50
#define PCI_MGA_OPTION3		0x54

#define RAMDAC_OFFSET		0x3c00

/* TVP3026 direct registers */

#define TVP3026_INDEX		0x00
#define TVP3026_WADR_PAL	0x00
#define TVP3026_COL_PAL		0x01
#define TVP3026_PIX_RD_MSK	0x02
#define TVP3026_RADR_PAL	0x03
#define TVP3026_CUR_COL_ADDR	0x04
#define TVP3026_CUR_COL_DATA	0x05
#define TVP3026_DATA		0x0a
#define TVP3026_CUR_RAM		0x0b
#define TVP3026_CUR_XLOW	0x0c
#define TVP3026_CUR_XHI		0x0d
#define TVP3026_CUR_YLOW	0x0e
#define TVP3026_CUR_YHI		0x0f

/* TVP3026 indirect registers */

#define TVP3026_SILICON_REV	0x01
#define TVP3026_CURSOR_CTL	0x06
#define TVP3026_LATCH_CTL	0x0f
#define TVP3026_TRUE_COLOR_CTL	0x18
#define TVP3026_MUX_CTL		0x19
#define TVP3026_CLK_SEL		0x1a
#define TVP3026_PAL_PAGE	0x1c
#define TVP3026_GEN_CTL		0x1d
#define TVP3026_MISC_CTL	0x1e
#define TVP3026_GEN_IO_CTL	0x2a
#define TVP3026_GEN_IO_DATA	0x2b
#define TVP3026_PLL_ADDR	0x2c
#define TVP3026_PIX_CLK_DATA	0x2d
#define TVP3026_MEM_CLK_DATA	0x2e
#define TVP3026_LOAD_CLK_DATA	0x2f
#define TVP3026_KEY_RED_LOW	0x32
#define TVP3026_KEY_RED_HI	0x33
#define TVP3026_KEY_GREEN_LOW	0x34
#define TVP3026_KEY_GREEN_HI	0x35
#define TVP3026_KEY_BLUE_LOW	0x36
#define TVP3026_KEY_BLUE_HI	0x37
#define TVP3026_KEY_CTL		0x38
#define TVP3026_MCLK_CTL	0x39
#define TVP3026_SENSE_TEST	0x3a
#define TVP3026_TEST_DATA	0x3b
#define TVP3026_CRC_LSB		0x3c
#define TVP3026_CRC_MSB		0x3d
#define TVP3026_CRC_CTL		0x3e
#define TVP3026_ID		0x3f
#define TVP3026_RESET		0xff


/* MGA1064 DAC Register file */
/* MGA1064 direct registers */

#define MGA1064_INDEX		0x00
#define MGA1064_WADR_PAL	0x00
#define MGA1064_COL_PAL		0x01
#define MGA1064_PIX_RD_MSK	0x02
#define MGA1064_RADR_PAL	0x03
#define MGA1064_DATA		0x0a

#define MGA1064_CUR_XLOW	0x0c
#define MGA1064_CUR_XHI		0x0d
#define MGA1064_CUR_YLOW	0x0e
#define MGA1064_CUR_YHI		0x0f

/* MGA1064 indirect registers */
#define MGA1064_DVI_PIPE_CTL    0x03
#define MGA1064_CURSOR_BASE_ADR_LOW	0x04
#define MGA1064_CURSOR_BASE_ADR_HI	0x05
#define MGA1064_CURSOR_CTL	0x06
#define MGA1064_CURSOR_COL0_RED	0x08
#define MGA1064_CURSOR_COL0_GREEN	0x09
#define MGA1064_CURSOR_COL0_BLUE	0x0a

#define MGA1064_CURSOR_COL1_RED	0x0c
#define MGA1064_CURSOR_COL1_GREEN	0x0d
#define MGA1064_CURSOR_COL1_BLUE	0x0e

#define MGA1064_CURSOR_COL2_RED	0x010
#define MGA1064_CURSOR_COL2_GREEN	0x011
#define MGA1064_CURSOR_COL2_BLUE	0x012

#define MGA1064_VREF_CTL	0x018

#define MGA1064_MUL_CTL		0x19
#define MGA1064_MUL_CTL_8bits		0x0
#define MGA1064_MUL_CTL_15bits		0x01
#define MGA1064_MUL_CTL_16bits		0x02
#define MGA1064_MUL_CTL_24bits		0x03
#define MGA1064_MUL_CTL_32bits		0x04
#define MGA1064_MUL_CTL_2G8V16bits		0x05
#define MGA1064_MUL_CTL_G16V16bits		0x06
#define MGA1064_MUL_CTL_32_24bits		0x07

#define MGAGDAC_XVREFCTRL		0x18
#define MGA1064_PIX_CLK_CTL		0x1a
#define MGA1064_PIX_CLK_CTL_CLK_DIS   		( 0x01 << 2 )
#define MGA1064_PIX_CLK_CTL_CLK_POW_DOWN   	( 0x01 << 3 )
#define MGA1064_PIX_CLK_CTL_SEL_PCI   		( 0x00 << 0 )
#define MGA1064_PIX_CLK_CTL_SEL_PLL   		( 0x01 << 0 )
#define MGA1064_PIX_CLK_CTL_SEL_EXT   		( 0x02 << 0 )
#define MGA1064_PIX_CLK_CTL_SEL_MSK   		( 0x03 << 0 )

#define MGA1064_GEN_CTL		0x1d
#define MGA1064_MISC_CTL	0x1e
#define MGA1064_MISC_CTL_DAC_POW_DN   		( 0x01 << 0 )
#define MGA1064_MISC_CTL_VGA   		( 0x01 << 1 )
#define MGA1064_MISC_CTL_DIS_CON   		( 0x03 << 1 )
#define MGA1064_MISC_CTL_MAFC   		( 0x02 << 1 )
#define MGA1064_MISC_CTL_VGA8   		( 0x01 << 3 )
#define MGA1064_MISC_CTL_DAC_RAM_CS   		( 0x01 << 4 )

#define MGA1064_GEN_IO_CTL	0x2a
#define MGA1064_GEN_IO_DATA	0x2b
#define MGA1064_SYS_PLL_M	0x2c
#define MGA1064_SYS_PLL_N	0x2d
#define MGA1064_SYS_PLL_P	0x2e
#define MGA1064_SYS_PLL_STAT	0x2f
#define MGA1064_ZOOM_CTL	0x38
#define MGA1064_SENSE_TST	0x3a

#define MGA1064_CRC_LSB		0x3c
#define MGA1064_CRC_MSB		0x3d
#define MGA1064_CRC_CTL		0x3e
#define MGA1064_COL_KEY_MSK_LSB		0x40
#define MGA1064_COL_KEY_MSK_MSB		0x41
#define MGA1064_COL_KEY_LSB		0x42
#define MGA1064_COL_KEY_MSB		0x43
#define MGA1064_PIX_PLLA_M	0x44
#define MGA1064_PIX_PLLA_N	0x45
#define MGA1064_PIX_PLLA_P	0x46
#define MGA1064_PIX_PLLB_M	0x48
#define MGA1064_PIX_PLLB_N	0x49
#define MGA1064_PIX_PLLB_P	0x4a
#define MGA1064_PIX_PLLC_M	0x4c
#define MGA1064_PIX_PLLC_N	0x4d
#define MGA1064_PIX_PLLC_P	0x4e

#define MGA1064_PIX_PLL_STAT	0x4f

/*Added for G450 dual head*/
/* Supported PLL*/
#define __PIXEL_PLL                 1
#define __SYSTEM_PLL                2
#define __VIDEO_PLL                 3

#define MGA1064_VID_PLL_P       0x8D
#define MGA1064_VID_PLL_M       0x8E
#define MGA1064_VID_PLL_N       0x8F

#define MGA1064_DISP_CTL        0x8a
#define MGA1064_SYNC_CTL        0x8b
#define MGA1064_PWR_CTL         0xa0
#define MGA1064_PAN_CTL         0xa2

/* Using crtc2 */
#define MGAREG2_C2CTL            0x10
#define MGAREG2_C2HPARAM         0x14
#define MGAREG2_C2HSYNC          0x18
#define MGAREG2_C2VPARAM         0x1c
#define MGAREG2_C2VSYNC          0x20
#define MGAREG2_C2STARTADD0      0x28

#define MGAREG2_C2OFFSET         0x40
#define MGAREG2_C2DATACTL        0x4c

#define MGAREG_C2CTL            0x3c10
#define MGAREG_C2HPARAM         0x3c14
#define MGAREG_C2HSYNC          0x3c18
#define MGAREG_C2VPARAM         0x3c1c
#define MGAREG_C2VSYNC          0x3c20
#define MGAREG_C2STARTADD0      0x3c28

#define MGAREG_C2OFFSET         0x3c40
#define MGAREG_C2DATACTL        0x3c4c

#define MGA1064_DISP_CTL        0x8a
#define MGA1064_SYNC_CTL        0x8b
#define MGA1064_PWR_CTL         0xa0

/* video register */

#define MGAREG_BESA1C3ORG	0x3d60
#define MGAREG_BESA1CORG	0x3d10
#define MGAREG_BESA1ORG		0x3d00
#define MGAREG_BESCTL		0x3d20
#define MGAREG_BESGLOBCTL	0x3dc0
#define MGAREG_BESHCOORD	0x3d28
#define MGAREG_BESHISCAL	0x3d30
#define MGAREG_BESHSRCEND	0x3d3c
#define MGAREG_BESHSRCLST	0x3d50
#define MGAREG_BESHSRCST	0x3d38
#define MGAREG_BESLUMACTL	0x3d40
#define MGAREG_BESPITCH		0x3d24
#define MGAREG_BESV1SRCLST	0x3d54
#define MGAREG_BESV1WGHT	0x3d48
#define MGAREG_BESVCOORD	0x3d2c
#define MGAREG_BESVISCAL	0x3d34

/* texture engine registers */

#define MGAREG_TMR0		0x2c00
#define MGAREG_TMR1		0x2c04
#define MGAREG_TMR2		0x2c08
#define MGAREG_TMR3		0x2c0c
#define MGAREG_TMR4		0x2c10
#define MGAREG_TMR5		0x2c14
#define MGAREG_TMR6		0x2c18
#define MGAREG_TMR7		0x2c1c
#define MGAREG_TMR8		0x2c20
#define MGAREG_TEXORG		0x2c24
#define MGAREG_TEXWIDTH		0x2c28
#define MGAREG_TEXHEIGHT	0x2c2c
#define MGAREG_TEXCTL		0x2c30
#define MGAREG_TEXCTL2		0x2c3c
#define MGAREG_TEXTRANS		0x2c34
#define MGAREG_TEXTRANSHIGH	0x2c38
#define MGAREG_TEXFILTER	0x2c58
#define MGAREG_ALPHASTART	0x2c70
#define MGAREG_ALPHAXINC	0x2c74
#define MGAREG_ALPHAYINC	0x2c78
#define MGAREG_ALPHACTRL	0x2c7c
#define MGAREG_DWGSYNC		0x2c4c

#define MGAREG_AGP_PLL		0x1e4c
#define MGA_AGP2XPLL_ENABLE		0x1
#define MGA_AGP2XPLL_DISABLE		0x0

#endif
