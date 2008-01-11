/*
 * Acceleration for the Creator and Creator3D framebuffer - DAC register layout.
 *
 * Copyright (C) 2000 David S. Miller (davem@redhat.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID MILLER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sunffb/ffb_dac.h,v 1.2 2001/04/05 17:42:33 dawes Exp $ */

#ifndef _FFB_DAC_H
#define _FFB_DAC_H

#define Bool int

/* FFB utilizes two different ramdac chips:
 *
 *	1) BT9068 "Pacifica1", used in all FFB1 and
 *	   FFB2 boards.
 *
 *	2) BT498(a) "Pacifica2", used in FFB2+ and
 *	   AFB boards.
 *
 * They are mostly equivalent, except in a few key areas:
 *
 *	1) WID register layout
 *	2) Number of CLUT tables
 *	3) Presence of Window Address Mask register
 *	4) Method of GAMMA correction support
 */

/* NOTE: All addresses described in this file are DAC
 *       indirect addresses.
 */

/* DAC color values are in the following format. */
#define FFBDAC_COLOR_BLUE	0x00ff0000
#define FFBDAC_COLOR_BLUE_SHFT	16
#define FFBDAC_COLOR_GREEN	0x0000ff00
#define FFBDAC_COLOR_GREEN_SHFT	8
#define FFBDAC_COLOR_RED	0x000000ff
#define FFBDAC_COLOR_RED_SHFT	0

/* Cursor DAC register addresses. */
#define FFBDAC_CUR_BITMAP_P0	0x000 /* Plane 0 cursor bitmap	*/
#define FFBDAC_CUR_BITMAP_P1	0x080 /* Plane 1 cursor bitmap	*/
#define FFBDAC_CUR_CTRL		0x100 /* Cursor control		*/
#define FFBDAC_CUR_COLOR0	0x101 /* Cursor Color 0		*/
#define FFBDAC_CUR_COLOR1	0x102 /* Cursor Color 1 (bg)	*/
#define FFBDAC_CUR_COLOR2	0x103 /* Cursor Color 2	(fg)	*/
#define FFBDAC_CUR_POS		0x104 /* Active cursor position	*/

/* Cursor control register.
 * WARNING: Be careful, reverse logic on these bits.
 */
#define FFBDAC_CUR_CTRL_P0	0x00000001	/* Plane0 display disable	*/
#define FFBDAC_CUR_CTRL_P1	0x00000002	/* Plane1 display disable	*/

/* Active cursor position register */
#define FFBDAC_CUR_POS_Y_SIGN	0x80000000	/* Sign of Y position		*/
#define FFBDAC_CUR_POS_Y	0x0fff0000	/* Y position			*/
#define FFBDAC_CUR_POS_X_SIGN	0x00008000	/* Sign of X position		*/
#define FFBDAC_CUR_POS_X	0x00000fff	/* X position			*/

/* Configuration and Palette DAC register addresses. */
#define FFBDAC_CFG_PPLLCTRL	0x0000 /* Pixel PLL Control		*/
#define FFBDAC_CFG_GPLLCTRL	0x0001 /* General Purpose PLL Control	*/
#define FFBDAC_CFG_PFCTRL	0x1000 /* Pixel Format Control		*/
#define FFBDAC_CFG_UCTRL	0x1001 /* User Control			*/
#define FFBDAC_CFG_CLUP_BASE	0x2000 /* Color Lookup Palette		*/
#define FFBDAC_CFG_CLUP(entry) (FFBDAC_CFG_CLUP_BASE + ((entry) * 0x100))
#define FFBDAC_PAC2_SOVWLUT0	0x3100 /* Shadow Overlay Window Lookup 0*/
#define FFBDAC_PAC2_SOVWLUT1	0x3101 /* Shadow Overlay Window Lookup 1*/
#define FFBDAC_PAC2_SOVWLUT2	0x3102 /* Shadow Overlay Window Lookup 2*/
#define FFBDAC_PAC2_SOVWLUT3	0x3103 /* Shadow Overlay Window Lookup 3*/
#define FFBDAC_PAC2_AOVWLUT0	0x3210 /* Active Overlay Window Lookup 0*/
#define FFBDAC_PAC2_AOVWLUT1	0x3211 /* Active Overlay Window Lookup 1*/
#define FFBDAC_PAC2_AOVWLUT2	0x3212 /* Active Overlay Window Lookup 2*/
#define FFBDAC_PAC2_AOVWLUT3	0x3213 /* Active Overlay Window Lookup 3*/
#define FFBDAC_CFG_WTCTRL	0x3150 /* Window Transfer Control	*/
#define FFBDAC_CFG_TMCTRL	0x3151 /* Transparent Mask Control	*/
#define FFBDAC_CFG_TCOLORKEY	0x3152 /* Transparent Color Key		*/
#define FFBDAC_CFG_WAMASK	0x3153 /* Window Address Mask (PAC2 only) */
#define FFBDAC_PAC1_SPWLUT_BASE	0x3100 /* Shadow Primary Window Lookups	*/
#define FFBDAC_PAC1_SPWLUT(entry) (FFBDAC_PAC1_SPWLUT_BASE + (entry))
#define FFBDAC_PAC1_APWLUT_BASE	0x3120 /* Active Primary Window Lookups	*/
#define FFBDAC_PAC1_APWLUT(entry) (FFBDAC_PAC1_APWLUT_BASE + (entry))
#define FFBDAC_PAC2_SPWLUT_BASE	0x3200 /* Shadow Primary Window Lookups	*/
#define FFBDAC_PAC2_SPWLUT(entry) (FFBDAC_PAC2_SPWLUT_BASE + (entry))
#define FFBDAC_PAC2_APWLUT_BASE	0x3240 /* Active Primary Window Lookups	*/
#define FFBDAC_PAC2_APWLUT(entry) (FFBDAC_PAC2_APWLUT_BASE + (entry))
#define FFBDAC_CFG_SANAL	0x5000 /* Signature Analysis Control	*/
#define FFBDAC_CFG_DACCTRL	0x5001 /* DAC Control			*/
#define FFBDAC_CFG_TGEN		0x6000 /* Timing Generator Control	*/
#define FFBDAC_CFG_VBNP		0x6001 /* Vertical Blank Negation Point	*/
#define FFBDAC_CFG_VBAP		0x6002 /* Vertical Blank Assertion Point*/
#define FFBDAC_CFG_VSNP		0x6003 /* Vertical Sync Negation Point	*/
#define FFBDAC_CFG_VSAP		0x6004 /* Vertical Sync Assertion Point	*/
#define FFBDAC_CFG_HSNP		0x6005 /* Horz Serration Negation Point */
#define FFBDAC_CFG_HBNP		0x6006 /* Horz Blank Negation Point	*/
#define FFBDAC_CFG_HBAP		0x6007 /* Horz Blank Assertion Point	*/
#define FFBDAC_CFG_HSYNCNP	0x6008 /* Horz Sync Negation Point	*/
#define FFBDAC_CFG_HSYNCAP	0x6009 /* Horz Sync Assertion Point	*/
#define FFBDAC_CFG_HSCENNP	0x600A /* Horz SCEN Negation Point	*/
#define FFBDAC_CFG_HSCENAP	0x600B /* Horz SCEN Assertion Point	*/
#define FFBDAC_CFG_EPNP		0x600C /* Eql'zing Pulse Negation Point	*/
#define FFBDAC_CFG_EINP		0x600D /* Eql'zing Intvl Negation Point	*/
#define FFBDAC_CFG_EIAP		0x600E /* Eql'zing Intvl Assertion Point*/
#define FFBDAC_CFG_TGVC		0x600F /* Timing Generator Vert Counter	*/
#define FFBDAC_CFG_TGHC		0x6010 /* Timing Generator Horz Counter	*/
#define FFBDAC_CFG_DID		0x8000 /* Device Identification		*/
#define FFBDAC_CFG_MPDATA	0x8001 /* Monitor Port Data		*/
#define FFBDAC_CFG_MPSENSE	0x8002 /* Monitor Port Sense		*/

/* Pixel PLL Control Register */
#define FFBDAC_CFG_PPLLCTRL_M	0x0000007f	/* PLL VCO Multiplicand		 */
#define FFBDAC_CFG_PPLLCTRL_D	0x00000780	/* PLL VCO Divisor		 */
#define FFBDAC_CFG_PPLLCTRL_PFD	0x00001800	/* Post VCO Frequency Divider	 */
#define FFBDAC_CFG_PPLLCTRL_EN	0x00004000	/* Enable PLL as pixel clock src */

/* General Purpose PLL Control Register */
#define FFBDAC_CFG_GPLLCTRL_M	0x0000007f	/* PLL VCO Multiplicand		 */
#define FFBDAC_CFG_GPLLCTRL_D	0x00000780	/* PLL VCO Divisor		 */
#define FFBDAC_CFG_GPLLCTRL_PFD	0x00001800	/* Post VCO Frequency Divider	 */
#define FFBDAC_CFG_GPLLCTRL_EN	0x00004000	/* Enable PLL as Gen. Purpose clk*/

/* Pixel Format Control Register */
#define FFBDAC_CFG_PFCTRL_2_1	0x00000000	/* 2:1 pixel interleave format	 */
#define FFBDAC_CFG_PFCTRL_4_1	0x00000001	/* 4:1 pixel interleave format	 */
#define FFBDAC_CFG_PFCTRL_42_1	0x00000002	/* 4/2:1 pixel interleave format */
#define FFBDAC_CFG_PFCTRL_82_1	0x00000003	/* 8/2:1 pixel interleave format */

/* User Control Register */
#define FFBDAC_UCTRL_IPDISAB	0x00000001	/* Disable input pullup resistors*/
#define FFBDAC_UCTRL_ABLANK	0x00000002	/* Asynchronous Blank		 */
#define FFBDAC_UCTRL_DBENAB	0x00000004	/* Double-Buffer Enable		 */
#define FFBDAC_UCTRL_OVENAB	0x00000008	/* Overlay Enable		 */
#define FFBDAC_UCTRL_WMODE	0x00000030	/* Window Mode			 */
#define FFBDAC_UCTRL_WM_COMB	0x00000000	/* Window Mode = Combined	 */
#define FFBDAC_UCTRL_WM_S4	0x00000010	/* Window Mode = Seperate_4	 */
#define FFBDAC_UCTRL_WM_S8	0x00000020	/* Window Mode = Seperate_8	 */
#define FFBDAC_UCTRL_WM_RESV	0x00000030	/* Window Mode = reserved	 */
#define FFBDAC_UCTRL_MANREV	0x00000f00	/* 4-bit Manufacturing Revision	 */

/* Overlay Window Lookup Registers (PAC2 only) */
#define FFBDAC_CFG_OVWLUT_PSEL	0x0000000f	/* Palette Section, Seperate_4	 */
#define FFBDAC_CFG_OVWLUT_PTBL	0x00000030	/* Palette Table		 */
#define FFBDAC_CFG_OVWLUT_LKUP	0x00000100	/* 1 = Use palette, 0 = Bypass	 */
#define FFBDAC_CFG_OVWLUT_OTYP	0x00000c00	/* Overlay Type			 */
#define FFBDAC_CFG_OVWLUT_O_N	0x00000000	/* Overlay Type - None		 */
#define FFBDAC_CFG_OVWLUT_O_T	0x00000400	/* Overlay Type - Transparent	 */
#define FFBDAC_CFG_OVWLUT_O_O	0x00000800	/* Overlay Type - Opaque	 */
#define FFBDAC_CFG_OVWLUT_O_R	0x00000c00	/* Overlay Type - Reserved	 */
#define FFBDAC_CFG_OVWLUT_PCS	0x00003000	/* Psuedocolor Src		 */
#define FFBDAC_CFG_OVWLUT_P_XO	0x00000000	/* Psuedocolor Src - XO[7:0]	 */
#define FFBDAC_CFG_OVWLUT_P_R	0x00001000	/* Psuedocolor Src - R[7:0]	 */
#define FFBDAC_CFG_OVWLUT_P_G	0x00002000	/* Psuedocolor Src - G[7:0]	 */
#define FFBDAC_CFG_OVWLUT_P_B	0x00003000	/* Psuedocolor Src - B[7:0]	 */

/* Window Transfer Control Register */
#define FFBDAC_CFG_WTCTRL_DS	0x00000001	/* Device Status, 1 = Busy	 */
#define FFBDAC_CFG_WTCTRL_TCMD	0x00000002	/* Transfer Command
						 * 1 = Transfer, 0 = No Action
						 */
#define FFBDAC_CFG_WTCTRL_TE	0x00000004	/* Transfer Event
						 * 1 = Next Frame, 0 = Next Field
						 */
#define FFBDAC_CFG_WTCTRL_DRD	0x00000008	/* Drawing Data
						 * 1 = Local Drawing Active
						 * 0 = Local Drawing Idle
						 */
#define FFBDAC_CFG_WTCTRL_DRS	0x00000010	/* Drawing Status
						 * 1 = Network Drawing Active
						 * 0 = Network Drawing Idle
						 */

/* Transparent Mask Control Register */
#define FFBDAC_CFG_TMCTRL_OMSK	0x000000ff	/* Overlay Mask			 */

/* Transparent Color Key Register */
#define FFBDAC_CFG_TCOLORKEY_K	0x000000ff	/* Overlay Color Key		 */

/* Window Address Mask Register (PAC2 only) */
#define FFBDAC_CFG_WAMASK_PMSK	0x0000003f	/* PWLUT select PMASK		 */
#define FFBDAC_CFG_WAMASK_OMSK	0x00000300	/* OWLUT control OMASK		 */

/* (non-Overlay) Window Lookup Table Registers, PAC1 format */
#define FFBDAC_PAC1_WLUT_DB	0x00000020	/* 0 = Buffer A, 1 = Buffer B	 */
#define FFBDAC_PAC1_WLUT_C	0x0000001c	/* C: Color Model Selection	 */
#define FFBDAC_PAC1_WLUT_C_8P	0x00000000	/* C: 8bpp Pseudocolor		 */
#define FFBDAC_PAC1_WLUT_C_8LG	0x00000004	/* C: 8bpp Linear Grey		 */
#define FFBDAC_PAC1_WLUT_C_8NG	0x00000008	/* C: 8bpp Non-Linear Grey	 */
#define FFBDAC_PAC1_WLUT_C_24D	0x00000010	/* C: 24bpp Directcolor		 */
#define FFBDAC_PAC1_WLUT_C_24LT	0x00000014	/* C: 24bpp Linear Truecolor	 */
#define FFBDAC_PAC1_WLUT_C_24NT	0x00000018	/* C: 24bpp Non-Linear Truecolor */
#define FFBDAC_PAC1_WLUT_PCS	0x00000003	/* Pseudocolor Src		 */
#define FFBDAC_PAC1_WLUT_P_XO	0x00000000	/* Pseudocolor Src - XO[7:0]	 */
#define FFBDAC_PAC1_WLUT_P_R	0x00000001	/* Pseudocolor Src - R[7:0]	 */
#define FFBDAC_PAC1_WLUT_P_G	0x00000002	/* Pseudocolor Src - G[7:0]	 */
#define FFBDAC_PAC1_WLUT_P_B	0x00000003	/* Pseudocolor Src - B[7:0]	 */

/* (non-Overlay) Window Lookup Table Registers, PAC2 format */
#define FFBDAC_PAC2_WLUT_PTBL	0x00000030	/* Palette Table Entry		 */
#define FFBDAC_PAC2_WLUT_LKUP	0x00000100	/* 1 = Use palette, 0 = Bypass	 */
#define FFBDAC_PAC2_WLUT_PCS	0x00003000	/* Pseudocolor Src		 */
#define FFBDAC_PAC2_WLUT_P_XO	0x00000000	/* Pseudocolor Src - XO[7:0]	 */
#define FFBDAC_PAC2_WLUT_P_R	0x00001000	/* Pseudocolor Src - R[7:0]	 */
#define FFBDAC_PAC2_WLUT_P_G	0x00002000	/* Pseudocolor Src - G[7:0]	 */
#define FFBDAC_PAC2_WLUT_P_B	0x00003000	/* Pseudocolor Src - B[7:0]	 */
#define FFBDAC_PAC2_WLUT_DEPTH	0x00004000	/* 0 = Pseudocolor, 1 = Truecolor*/
#define FFBDAC_PAC2_WLUT_DB	0x00008000	/* 0 = Buffer A, 1 = Buffer B	 */

/* Signature Analysis Control Register */
#define FFBDAC_CFG_SANAL_SRR	0x000000ff	/* DAC Seed/Result for Red	 */
#define FFBDAC_CFG_SANAL_SRG	0x0000ff00	/* DAC Seed/Result for Green	 */
#define FFBDAC_CFG_SANAL_SRB	0x00ff0000	/* DAC Seed/Result for Blue	 */
#define FFBDAC_CFG_SANAL_RQST	0x01000000	/* Signature Capture Request	 */
#define FFBDAC_CFG_SANAL_BSY	0x02000000	/* Signature Analysis Busy	 */
#define FFBDAC_CFG_SANAL_DSM	0x04000000	/* Data Strobe Mode
						 * 0 = Signature Analysis Mode
						 * 1 = Data Strobe Mode
						 */

/* DAC Control Register */
#define FFBDAC_CFG_DACCTRL_O2	0x00000003	/* Operand 2 Select
						 * 00 = Normal Operation
						 * 01 = Select 145mv Reference
						 * 10 = Select Blue DAC Output
						 * 11 = Reserved
						 */
#define FFBDAC_CFG_DACCTRL_O1	0x0000000c	/* Operand 1 Select
						 * 00 = Normal Operation
						 * 01 = Select Green DAC Output
						 * 10 = Select Red DAC Output
						 * 11 = Reserved
						 */
#define FFBDAC_CFG_DACCTRL_CR	0x00000010	/* Comparator Result
						 * 0 = operand1 < operand2
						 * 1 = operand1 > operand2
						 */
#define FFBDAC_CFG_DACCTRL_SGE	0x00000020	/* Sync-on-Green Enable		 */
#define FFBDAC_CFG_DACCTRL_PE	0x00000040	/* Pedestal Enable		 */
#define FFBDAC_CFG_DACCTRL_VPD	0x00000080	/* VSYNC* Pin Disable		 */
#define FFBDAC_CFG_DACCTRL_SPB	0x00000100	/* Sync Polarity Bit
						 * 0 = VSYNC* and CSYNC* active low
						 * 1 = VSYNC* and CSYNC* active high
						 */

/* Timing Generator Control Register */
#define FFBDAC_CFG_TGEN_VIDE	0x00000001	/* Video Enable			 */
#define FFBDAC_CFG_TGEN_TGE	0x00000002	/* Timing Generator Enable	 */
#define FFBDAC_CFG_TGEN_HSD	0x00000004	/* HSYNC* Disabled		 */
#define FFBDAC_CFG_TGEN_VSD	0x00000008	/* VSYNC* Disabled		 */
#define FFBDAC_CFG_TGEN_EQD	0x00000010	/* Equalization Disabled	 */
#define FFBDAC_CFG_TGEN_MM	0x00000020	/* 0 = Slave, 1 = Master	 */
#define FFBDAC_CFG_TGEN_IM	0x00000040	/* 1 = Interlaced Mode		 */

/* Device Identification Register, should be 0xA236E1AD for FFB bt497/bt498 */
#define FFBDAC_CFG_DID_ONE	0x00000001	/* Always set			 */
#define FFBDAC_CFG_DID_MANUF	0x00000ffe	/* Manufacturer ID		 */
#define FFBDAC_CFG_DID_PNUM	0x0ffff000	/* Device Part Number		 */
#define FFBDAC_CFG_DID_REV	0xf0000000	/* Device Revision		 */

/* Monitor Port Data Register */
#define FFBDAC_CFG_MPDATA_SCL	0x00000001	/* SCL Data			 */
#define FFBDAC_CFG_MPDATA_SDA	0x00000002	/* SDA Data			 */

/* Monitor Port Sense Register */
#define FFBDAC_CFG_MPSENSE_SCL	0x00000001	/* SCL Sense			 */
#define FFBDAC_CFG_MPSENSE_SDA	0x00000002	/* SDA Sense			 */

/* DAC register access shorthands. */
#define DACCUR_READ(DAC, ADDR)		((DAC)->cur = (ADDR), (DAC)->curdata)
#define DACCUR_WRITE(DAC, ADDR, VAL)	((DAC)->cur = (ADDR), (DAC)->curdata = (VAL))
#define DACCFG_READ(DAC, ADDR)		((DAC)->cfg = (ADDR), (DAC)->cfgdata)
#define DACCFG_WRITE(DAC, ADDR, VAL)	((DAC)->cfg = (ADDR), (DAC)->cfgdata = (VAL))

typedef struct ffb_dac_hwstate {
	unsigned int ppllctrl;
	unsigned int gpllctrl;
	unsigned int pfctrl;
	unsigned int uctrl;
	unsigned int clut[256 * 4];	/* One 256 entry clut on PAC1, 4 on PAC2 */
	unsigned int ovluts[4];		/* Overlay WLUTS, PAC2 only */
	unsigned int wtctrl;
	unsigned int tmctrl;
	unsigned int tcolorkey;
	unsigned int wamask;
	unsigned int pwluts[64];
	unsigned int dacctrl;
	unsigned int tgen;
	unsigned int vbnp;
	unsigned int vbap;
	unsigned int vsnp;
	unsigned int vsap;
	unsigned int hsnp;
	unsigned int hbnp;
	unsigned int hbap;
	unsigned int hsyncnp;
	unsigned int hsyncap;
	unsigned int hscennp;
	unsigned int hscenap;
	unsigned int epnp;
	unsigned int einp;
	unsigned int eiap;
} ffb_dac_hwstate_t;

typedef struct {
	Bool		InUse;

	/* The following fields are undefined unless InUse is TRUE. */
	int		refcount;
	Bool		canshare;
	unsigned int	wlut_regval;
	int		buffer;		/* 0 = Buffer A, 1 = Buffer B	*/
	int		depth;		/* 8 or 32 bpp			*/
	int		greyscale;	/* 1 = greyscale, 0 = color	*/
	int		linear;		/* 1 = linear, 0 = non-linear	*/
	int		direct;		/* 1 = 24bpp directcolor	*/
	int		channel;	/* 0 = X, 1 = R, 2 = G, 3 = B	*/
	int		palette;	/* Only PAC2 has multiple CLUTs	*/
} ffb_wid_info_t;

#define FFB_MAX_PWIDS	64
typedef struct {
	int		num_wids;
	int		wid_shift;	/* To get X channel value	*/
	ffb_wid_info_t	wid_pool[FFB_MAX_PWIDS];
} ffb_wid_pool_t;

typedef struct ffb_dac_info {
	unsigned int flags;
#define FFB_DAC_PAC1	0x00000001	/* Pacifica1 DAC, BT9068	*/
#define FFB_DAC_PAC2	0x00000002	/* Pacifica2 DAC, BT498		*/
#define FFB_DAC_ICURCTL	0x00000004	/* Inverted CUR_CTRL bits	*/

	unsigned int kernel_wid;

	/* These registers need to be modified when changing DAC
	 * timing state, so at init time we capture their values.
	 */
	unsigned int ffbcfg0;
	unsigned int ffbcfg2;
	unsigned int ffb_passin_ctrl;	/* FFB2+/AFB only */

	ffb_dac_hwstate_t	kern_dac_state;
	ffb_dac_hwstate_t	x_dac_state;

	ffb_wid_pool_t		wid_table;
} ffb_dac_info_t;

#endif /* _FFB_DAC_H */
