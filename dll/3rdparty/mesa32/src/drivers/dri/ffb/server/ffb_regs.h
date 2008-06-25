/*
 * Acceleration for the Creator and Creator3D framebuffer - register layout.
 *
 * Copyright (C) 1998,1999,2000 Jakub Jelinek (jakub@redhat.com)
 * Copyright (C) 1998 Michal Rehacek (majkl@iname.com)
 * Copyright (C) 1999 David S. Miller (davem@redhat.com)
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
 * JAKUB JELINEK, MICHAL REHACEK, OR DAVID MILLER BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sunffb/ffb_regs.h,v 1.1 2000/05/18 23:21:37 dawes Exp $ */

#ifndef FFBREGS_H
#define FFBREGS_H

/* Auxilliary clips. */
typedef struct  {
	volatile unsigned int min;
	volatile unsigned int max;
} ffb_auxclip, *ffb_auxclipPtr;

/* FFB register set. */
typedef struct _ffb_fbc {
	/* Next vertex registers, on the right we list which drawops
	 * use said register and the logical name the register has in
	 * that context.
	 */					/* DESCRIPTION		DRAWOP(NAME)	*/
/*0x00*/unsigned int		pad1[3];	/* Reserved				*/
/*0x0c*/volatile unsigned int	alpha;		/* ALPHA Transparency			*/
/*0x10*/volatile unsigned int	red;		/* RED					*/
/*0x14*/volatile unsigned int	green;		/* GREEN				*/
/*0x18*/volatile unsigned int	blue;		/* BLUE					*/
/*0x1c*/volatile unsigned int	z;		/* DEPTH				*/
/*0x20*/volatile unsigned int	y;		/* Y			triangle(DOYF)	*/
						/*                      aadot(DYF)	*/
						/*                      ddline(DYF)	*/
						/*                      aaline(DYF)	*/
/*0x24*/volatile unsigned int	x;		/* X			triangle(DOXF)	*/
						/*                      aadot(DXF)	*/
						/*                      ddline(DXF)	*/
						/*                      aaline(DXF)	*/
/*0x28*/unsigned int		pad2[2];	/* Reserved				*/
/*0x30*/volatile unsigned int	ryf;		/* Y (alias to DOYF)	ddline(RYF)	*/
						/*			aaline(RYF)	*/
						/*			triangle(RYF)	*/
/*0x34*/volatile unsigned int	rxf;		/* X			ddline(RXF)	*/
						/*			aaline(RXF)	*/
						/*			triangle(RXF)	*/
/*0x38*/unsigned int		pad3[2];	/* Reserved				*/
/*0x40*/volatile unsigned int	dmyf;		/* Y (alias to DOYF)	triangle(DMYF)	*/
/*0x44*/volatile unsigned int	dmxf;		/* X			triangle(DMXF)	*/
/*0x48*/unsigned int		pad4[2];	/* Reserved				*/
/*0x50*/volatile unsigned int	ebyi;		/* Y (alias to RYI)	polygon(EBYI)	*/
/*0x54*/volatile unsigned int	ebxi;		/* X			polygon(EBXI)	*/
/*0x58*/unsigned int		pad5[2];	/* Reserved				*/
/*0x60*/volatile unsigned int	by;		/* Y			brline(RYI)	*/
						/*			fastfill(OP)	*/
						/*			polygon(YI)	*/
						/*			rectangle(YI)	*/
						/*			bcopy(SRCY)	*/
						/*			vscroll(SRCY)	*/
/*0x64*/volatile unsigned int	bx;		/* X			brline(RXI)	*/
						/*			polygon(XI)	*/
						/*			rectangle(XI)	*/
						/*			bcopy(SRCX)	*/
						/*			vscroll(SRCX)	*/
						/*			fastfill(GO)	*/
/*0x68*/volatile unsigned int	dy;		/* destination Y	fastfill(DSTY)	*/
						/*			bcopy(DSRY)	*/
						/*			vscroll(DSRY)	*/
/*0x6c*/volatile unsigned int	dx;		/* destination X	fastfill(DSTX)	*/
						/*			bcopy(DSTX)	*/
						/*			vscroll(DSTX)	*/
/*0x70*/volatile unsigned int	bh;		/* Y (alias to RYI)	brline(DYI)	*/
						/*			dot(DYI)	*/
						/*			polygon(ETYI)	*/
						/* Height		fastfill(H)	*/
						/*			bcopy(H)	*/
						/*			vscroll(H)	*/
						/* Y count		fastfill(NY)	*/
/*0x74*/volatile unsigned int	bw;		/* X			dot(DXI)	*/
						/*			brline(DXI)	*/
						/*			polygon(ETXI)	*/
						/*			fastfill(W)	*/
						/*			bcopy(W)	*/
						/*			vscroll(W)	*/
						/*			fastfill(NX)	*/
/*0x78*/unsigned int		pad6[2];	/* Reserved				*/
/*0x80*/unsigned int		pad7[32];	/* Reserved				*/
	
	/* Setup Unit's vertex state register */
/*100*/	volatile unsigned int	suvtx;
/*104*/	unsigned int		pad8[63];	/* Reserved				*/
	
	/* Frame Buffer Control Registers */
/*200*/	volatile unsigned int	ppc;		/* Pixel Processor Control		*/
/*204*/	volatile unsigned int	wid;		/* Current WID				*/
/*208*/	volatile unsigned int	fg;		/* FG data				*/
/*20c*/	volatile unsigned int	bg;		/* BG data				*/
/*210*/	volatile unsigned int	consty;		/* Constant Y				*/
/*214*/	volatile unsigned int	constz;		/* Constant Z				*/
/*218*/	volatile unsigned int	xclip;		/* X Clip				*/
/*21c*/	volatile unsigned int	dcss;		/* Depth Cue Scale Slope		*/
/*220*/	volatile unsigned int	vclipmin;	/* Viewclip XY Min Bounds		*/
/*224*/	volatile unsigned int	vclipmax;	/* Viewclip XY Max Bounds		*/
/*228*/	volatile unsigned int	vclipzmin;	/* Viewclip Z Min Bounds		*/
/*22c*/	volatile unsigned int	vclipzmax;	/* Viewclip Z Max Bounds		*/
/*230*/	volatile unsigned int	dcsf;		/* Depth Cue Scale Front Bound		*/
/*234*/	volatile unsigned int	dcsb;		/* Depth Cue Scale Back Bound		*/
/*238*/	volatile unsigned int	dczf;		/* Depth Cue Z Front			*/
/*23c*/	volatile unsigned int	dczb;		/* Depth Cue Z Back			*/
/*240*/	unsigned int		pad9;		/* Reserved				*/
/*244*/	volatile unsigned int	blendc;		/* Alpha Blend Control			*/
/*248*/	volatile unsigned int	blendc1;	/* Alpha Blend Color 1			*/
/*24c*/	volatile unsigned int	blendc2;	/* Alpha Blend Color 2			*/
/*250*/	volatile unsigned int	fbramitc;	/* FB RAM Interleave Test Control	*/
/*254*/	volatile unsigned int	fbc;		/* Frame Buffer Control			*/
/*258*/	volatile unsigned int	rop;		/* Raster OPeration			*/
/*25c*/	volatile unsigned int	cmp;		/* Frame Buffer Compare			*/
/*260*/	volatile unsigned int	matchab;	/* Buffer AB Match Mask			*/
/*264*/	volatile unsigned int	matchc;		/* Buffer C(YZ) Match Mask		*/
/*268*/	volatile unsigned int	magnab;		/* Buffer AB Magnitude Mask		*/
/*26c*/	volatile unsigned int	magnc;		/* Buffer C(YZ) Magnitude Mask		*/
/*270*/	volatile unsigned int	fbcfg0;		/* Frame Buffer Config 0		*/
/*274*/	volatile unsigned int	fbcfg1;		/* Frame Buffer Config 1		*/
/*278*/	volatile unsigned int	fbcfg2;		/* Frame Buffer Config 2		*/
/*27c*/	volatile unsigned int	fbcfg3;		/* Frame Buffer Config 3		*/
/*280*/	volatile unsigned int	ppcfg;		/* Pixel Processor Config		*/
/*284*/	volatile unsigned int	pick;		/* Picking Control			*/
/*288*/	volatile unsigned int	fillmode;	/* FillMode				*/
/*28c*/	volatile unsigned int	fbramwac;	/* FB RAM Write Address Control		*/
/*290*/	volatile unsigned int	pmask;		/* RGB PlaneMask			*/
/*294*/	volatile unsigned int	xpmask;		/* X PlaneMask				*/
/*298*/	volatile unsigned int	ypmask;		/* Y PlaneMask				*/
/*29c*/	volatile unsigned int	zpmask;		/* Z PlaneMask				*/
/*2a0*/	ffb_auxclip		auxclip[4]; 	/* Auxilliary Viewport Clip		*/
	
	/* New 3dRAM III support regs */
/*2c0*/	volatile unsigned int	rawblend2;
/*2c4*/	volatile unsigned int	rawpreblend;
/*2c8*/	volatile unsigned int	rawstencil;
/*2cc*/	volatile unsigned int	rawstencilctl;
/*2d0*/	volatile unsigned int	threedram1;
/*2d4*/	volatile unsigned int	threedram2;
/*2d8*/	volatile unsigned int	passin;
/*2dc*/	volatile unsigned int	rawclrdepth;
/*2e0*/	volatile unsigned int	rawpmask;
/*2e4*/	volatile unsigned int	rawcsrc;
/*2e8*/	volatile unsigned int	rawmatch;
/*2ec*/	volatile unsigned int	rawmagn;
/*2f0*/	volatile unsigned int	rawropblend;
/*2f4*/	volatile unsigned int	rawcmp;
/*2f8*/	volatile unsigned int	rawwac;
/*2fc*/	volatile unsigned int	fbramid;
	
/*300*/	volatile unsigned int	drawop;		/* Draw OPeration			*/
/*304*/	unsigned int		pad10[2];	/* Reserved				*/
/*30c*/	volatile unsigned int	lpat;		/* Line Pattern control			*/
/*310*/	unsigned int		pad11;		/* Reserved				*/
/*314*/	volatile unsigned int	fontxy;		/* XY Font coordinate			*/
/*318*/	volatile unsigned int	fontw;		/* Font Width				*/
/*31c*/	volatile unsigned int	fontinc;	/* Font Increment			*/
/*320*/	volatile unsigned int	font;		/* Font bits				*/
/*324*/	unsigned int		pad12[3];	/* Reserved				*/
/*330*/	volatile unsigned int	blend2;
/*334*/	volatile unsigned int	preblend;
/*338*/	volatile unsigned int	stencil;
/*33c*/	volatile unsigned int	stencilctl;

/*340*/	unsigned int		pad13[4];	/* Reserved				*/
/*350*/	volatile unsigned int	dcss1;		/* Depth Cue Scale Slope 1		*/
/*354*/	volatile unsigned int	dcss2;		/* Depth Cue Scale Slope 2		*/
/*358*/	volatile unsigned int	dcss3;		/* Depth Cue Scale Slope 3		*/
/*35c*/	volatile unsigned int	widpmask;
/*360*/	volatile unsigned int	dcs2;
/*364*/	volatile unsigned int	dcs3;
/*368*/	volatile unsigned int	dcs4;
/*36c*/	unsigned int		pad14;		/* Reserved				*/
/*370*/	volatile unsigned int	dcd2;
/*374*/	volatile unsigned int	dcd3;
/*378*/	volatile unsigned int	dcd4;
/*37c*/	unsigned int		pad15;		/* Reserved				*/
/*380*/	volatile unsigned int	pattern[32];	/* area Pattern				*/
/*400*/	unsigned int		pad16[8];	/* Reserved				*/
/*420*/	volatile unsigned int	reset;		/* chip RESET				*/
/*424*/	unsigned int		pad17[247];	/* Reserved				*/
/*800*/	volatile unsigned int	devid;		/* Device ID				*/
/*804*/	unsigned int		pad18[63];	/* Reserved				*/
/*900*/	volatile unsigned int	ucsr;		/* User Control & Status Register	*/
/*904*/	unsigned int		pad19[31];	/* Reserved				*/
/*980*/	volatile unsigned int	mer;		/* Mode Enable Register			*/
/*984*/	unsigned int		pad20[1439];	/* Reserved				*/
} ffb_fbc, *ffb_fbcPtr;

/* Draw operations */
#define FFB_DRAWOP_DOT		0x00
#define FFB_DRAWOP_AADOT	0x01
#define FFB_DRAWOP_BRLINECAP	0x02
#define FFB_DRAWOP_BRLINEOPEN	0x03
#define FFB_DRAWOP_DDLINE	0x04
#define FFB_DRAWOP_AALINE	0x05
#define FFB_DRAWOP_TRIANGLE	0x06
#define FFB_DRAWOP_POLYGON	0x07
#define FFB_DRAWOP_RECTANGLE	0x08
#define FFB_DRAWOP_FASTFILL	0x09
#define FFB_DRAWOP_BCOPY	0x0a	/* Not implemented in any FFB, VIS is faster		*/
#define FFB_DRAWOP_VSCROLL	0x0b	/* Up to 12x faster than BCOPY, 3-4x faster than VIS	*/

/* FastFill operation codes. */
#define FFB_FASTFILL_PAGE	0x01
#define FFB_FASTFILL_BLOCK	0x02
#define FFB_FASTFILL_COLOR_BLK	0x03
#define FFB_FASTFILL_BLOCK_X	0x04

/* Spanfill Unit Line Pattern */
#define FFB_LPAT_SCALEPTR	0xf0000000
#define FFB_LPAT_SCALEPTR_SHIFT	28
#define FFB_LPAT_PATPTR		0x0f000000
#define FFB_LPAT_PATPTR_SHIFT	24
#define FFB_LPAT_SCALEVAL	0x00f00000
#define FFB_LPAT_SCALEVAL_SHIFT	20
#define FFB_LPAT_PATLEN		0x000f0000
#define FFB_LPAT_PATLEN_SHIFT	16
#define FFB_LPAT_PATTERN	0x0000ffff
#define FFB_LPAT_PATTERN_SHIFT	0

/* Pixel processor control */
/* Force WID */
#define FFB_PPC_FW_DISABLE	0x800000
#define FFB_PPC_FW_ENABLE	0xc00000
#define FFB_PPC_FW_MASK		0xc00000
/* Auxiliary clip */
#define FFB_PPC_ACE_DISABLE	0x040000
#define FFB_PPC_ACE_AUX_SUB	0x080000
#define FFB_PPC_ACE_AUX_ADD	0x0c0000
#define FFB_PPC_ACE_MASK	0x0c0000
/* Depth cue */
#define FFB_PPC_DCE_DISABLE	0x020000
#define FFB_PPC_DCE_ENABLE	0x030000
#define FFB_PPC_DCE_MASK	0x030000
/* Alpha blend */
#define FFB_PPC_ABE_DISABLE	0x008000
#define FFB_PPC_ABE_ENABLE	0x00c000
#define FFB_PPC_ABE_MASK	0x00c000
/* View clip */
#define FFB_PPC_VCE_DISABLE	0x001000
#define FFB_PPC_VCE_2D		0x002000
#define FFB_PPC_VCE_3D		0x003000
#define FFB_PPC_VCE_MASK	0x003000
/* Area pattern */
#define FFB_PPC_APE_DISABLE	0x000800
#define FFB_PPC_APE_ENABLE	0x000c00
#define FFB_PPC_APE_MASK	0x000c00
/* Transparent background */
#define FFB_PPC_TBE_OPAQUE	0x000200
#define FFB_PPC_TBE_TRANSPARENT	0x000300
#define FFB_PPC_TBE_MASK	0x000300
/* Z source */
#define FFB_PPC_ZS_VAR		0x000080
#define FFB_PPC_ZS_CONST	0x0000c0
#define FFB_PPC_ZS_MASK		0x0000c0
/* Y source */
#define FFB_PPC_YS_VAR		0x000020
#define FFB_PPC_YS_CONST	0x000030
#define FFB_PPC_YS_MASK		0x000030
/* X source */
#define FFB_PPC_XS_WID		0x000004
#define FFB_PPC_XS_VAR		0x000008
#define FFB_PPC_XS_CONST	0x00000c
#define FFB_PPC_XS_MASK		0x00000c
/* Color (BGR) source */
#define FFB_PPC_CS_VAR		0x000002
#define FFB_PPC_CS_CONST	0x000003
#define FFB_PPC_CS_MASK		0x000003

/* X Clip */
#define FFB_XCLIP_XREF		0x000000ff
#define FFB_XCLIP_TEST_MASK	0x00070000
#define FFB_XCLIP_TEST_ALWAYS	0x00000000
#define FFB_XCLIP_TEST_GT	0x00010000
#define FFB_XCLIP_TEST_EQ	0x00020000
#define FFB_XCLIP_TEST_GE	0x00030000
#define FFB_XCLIP_TEST_NEVER	0x00040000
#define FFB_XCLIP_TEST_LE	0x00050000
#define FFB_XCLIP_TEST_NE	0x00060000
#define FFB_XCLIP_TEST_LT	0x00070000

/* FB Control register */
/* Write buffer dest */
#define FFB_FBC_WB_A		0x20000000
#define FFB_FBC_WB_B		0x40000000
#define FFB_FBC_WB_AB		0x60000000
#define FFB_FBC_WB_C		0x80000000
#define FFB_FBC_WB_AC		0xa0000000
#define FFB_FBC_WB_BC		0xc0000000
#define FFB_FBC_WB_ABC		0xe0000000
#define FFB_FBC_WB_MASK		0xe0000000
/* Write enable */
#define FFB_FBC_WE_FORCEOFF	0x00100000
#define FFB_FBC_WE_FORCEON	0x00200000
#define FFB_FBC_WE_USE_WMASK	0x00300000
#define FFB_FBC_WE_MASK		0x00300000
/* Write group mode */
#define FFB_FBC_WM_RSVD		0x00040000
#define FFB_FBC_WM_COMBINED	0x00080000
#define FFB_FBC_WM_SEPARATE	0x000c0000
#define FFB_FBC_WM_MASK		0x000c0000
/* Read buffer src */
#define FFB_FBC_RB_A		0x00004000
#define FFB_FBC_RB_B		0x00008000
#define FFB_FBC_RB_C		0x0000c000
#define FFB_FBC_RB_MASK		0x0000c000
/* Stereo buf dest */
#define FFB_FBC_SB_LEFT		0x00001000
#define FFB_FBC_SB_RIGHT	0x00002000
#define FFB_FBC_SB_BOTH		0x00003000
#define FFB_FBC_SB_MASK		0x00003000
/* Z plane group enable */
#define FFB_FBC_ZE_OFF		0x00000400
#define FFB_FBC_ZE_ON		0x00000800
#define FFB_FBC_ZE_MASK		0x00000c00
/* Y plane group enable */
#define FFB_FBC_YE_OFF		0x00000100
#define FFB_FBC_YE_ON		0x00000200
#define FFB_FBC_YE_MASK		0x00000300
/* X plane group enable */
#define FFB_FBC_XE_OFF		0x00000040
#define FFB_FBC_XE_ON		0x00000080
#define FFB_FBC_XE_MASK		0x000000c0
/* B plane group enable */
#define FFB_FBC_BE_OFF		0x00000010
#define FFB_FBC_BE_ON		0x00000020
#define FFB_FBC_BE_MASK		0x00000030
/* G plane group enable */
#define FFB_FBC_GE_OFF		0x00000004
#define FFB_FBC_GE_ON		0x00000008
#define FFB_FBC_GE_MASK		0x0000000c
/* R plane group enable */
#define FFB_FBC_RE_OFF		0x00000001
#define FFB_FBC_RE_ON		0x00000002
#define FFB_FBC_RE_MASK		0x00000003
/* Combined */
#define FFB_FBC_RGBE_OFF	0x00000015
#define FFB_FBC_RGBE_ON		0x0000002a
#define FFB_FBC_RGBE_MASK	0x0000003f

/* Raster OP */
#define FFB_ROP_YZ_MASK		0x008f0000
#define FFB_ROP_X_MASK		0x00008f00
#define FFB_ROP_RGB_MASK	0x0000008f

/* Now the rops themselves which get shifted into the
 * above fields.
 */
#define FFB_ROP_EDIT_BIT	0x80
#define FFB_ROP_ZERO		0x80
#define FFB_ROP_NEW_AND_OLD	0x81
#define FFB_ROP_NEW_AND_NOLD	0x82
#define FFB_ROP_NEW		0x83
#define FFB_ROP_NNEW_AND_OLD	0x84
#define FFB_ROP_OLD		0x85
#define FFB_ROP_NEW_XOR_OLD	0x86
#define FFB_ROP_NEW_OR_OLD	0x87
#define FFB_ROP_NNEW_AND_NOLD	0x88
#define FFB_ROP_NNEW_XOR_NOLD	0x89
#define FFB_ROP_NOLD		0x8a
#define FFB_ROP_NEW_OR_NOLD	0x8b
#define FFB_ROP_NNEW		0x8c
#define FFB_ROP_NNEW_OR_OLD	0x8d
#define FFB_ROP_NNEW_OR_NOLD	0x8e
#define FFB_ROP_ONES		0x8f

/* FB Compare */
#define FFB_CMP_MATCHC_MASK	0x8f000000
#define FFB_CMP_MAGNC_MASK	0x00870000
#define FFB_CMP_MATCHAB_MASK	0x0000ff00
#define FFB_CMP_MAGNAB_MASK	0x000000ff

/* Compare Match codes */
#define FFB_CMP_MATCH_EDIT_BIT	0x80
#define FFB_CMP_MATCH_ALWAYS	0x80
#define FFB_CMP_MATCH_NEVER	0x81
#define FFB_CMP_MATCH_EQ	0x82
#define FFB_CMP_MATCH_NE	0x83
#define FFB_CMP_MATCH_A_ALWAYS	0xc0
#define FFB_CMP_MATCH_B_ALWAYS	0xa0

/* Compare Magnitude codes */
#define FFB_CMP_MAGN_EDIT_BIT	0x80
#define FFB_CMP_MAGN_ALWAYS	0x80
#define FFB_CMP_MAGN_GT		0x81
#define FFB_CMP_MAGN_EQ		0x82
#define FFB_CMP_MAGN_GE		0x83
#define FFB_CMP_MAGN_NEVER	0x84
#define FFB_CMP_MAGN_LE		0x85
#define FFB_CMP_MAGN_NE		0x86
#define FFB_CMP_MAGN_LT		0x87
#define FFB_CMP_MAGN_A_ALWAYS	0xc0
#define FFB_CMP_MAGN_B_ALWAYS	0xa0

/* User Control and Status */
#define FFB_UCSR_FIFO_MASK	0x00000fff
#define FFB_UCSR_PICK_NO_HIT	0x00020000
#define FFB_UCSR_PICK_HIT	0x00030000
#define FFB_UCSR_PICK_DISABLE	0x00080000
#define FFB_UCSR_PICK_ENABLE	0x000c0000
#define FFB_UCSR_FB_BUSY	0x01000000
#define FFB_UCSR_RP_BUSY	0x02000000
#define FFB_UCSR_ALL_BUSY	(FFB_UCSR_RP_BUSY|FFB_UCSR_FB_BUSY)
#define FFB_UCSR_READ_ERR	0x40000000
#define FFB_UCSR_FIFO_OVFL	0x80000000
#define FFB_UCSR_ALL_ERRORS	(FFB_UCSR_READ_ERR|FFB_UCSR_FIFO_OVFL)

/* Mode Enable Register */
#define FFB_MER_EIRA		0x00000080 /* Enable read-ahead, increasing */
#define FFB_MER_EDRA		0x000000c0 /* Enable read-ahead, decreasing */
#define FFB_MER_DRA		0x00000040 /* No read-ahead */

/* FBram Config 0 */
#define FFB_FBCFG0_RFTIME	0xff800000
#define FFB_FBCFG0_XMAX		0x007c0000
#define FFB_FBCFG0_YMAX		0x0003ffc0
#define FFB_FBCFG0_RES_MASK	0x00000030
#define FFB_FBCFG0_RES_HIGH	0x00000030 /* 1920x1360 */
#define FFB_FBCFG0_RES_STD	0x00000020 /* 1280x1024 */
#define FFB_FBCFG0_RES_STEREO	0x00000010 /* 960x580 */
#define FFB_FBCFG0_RES_PRTRAIT	0x00000000 /* 1280x2048 */
#define FFB_FBCFG0_ITRLACE	0x00000000
#define FFB_FBCFG0_SEQUENTIAL	0x00000008
#define FFB_FBCFG0_DRENA	0x00000004
#define FFB_FBCFG0_BPMODE	0x00000002
#define FFB_FBCFG0_RFRSH_RST	0x00000001

typedef struct _ffb_dac {
	volatile unsigned int	cfg;
	volatile unsigned int	cfgdata;
	volatile unsigned int	cur;
	volatile unsigned int	curdata;
} ffb_dac, *ffb_dacPtr;

/* Writing 2 32-bit registers at a time using 64-bit stores. -DaveM */
#if defined(__GNUC__) && defined(USE_VIS)
/* 64-bit register writing support.
 * Note: "lo" means "low address".
 */
#define FFB_WRITE64_COMMON(__regp, __lo32, __hi32, REG0, REG1) \
do {	__extension__ register unsigned int __r0 __asm__(""#REG0); \
	__extension__ register unsigned int __r1 __asm__(""#REG1); \
	__r0 = (__lo32); \
	__r1 = (__hi32); \
	__asm__ __volatile__ ("sllx\t%0, 32, %%g1\n\t" \
			      "srl\t%1, 0, %1\n\t" \
			      "or\t%%g1, %1, %%g1\n\t" \
			      "stx\t%%g1, %2" \
	 : : "r" (__r0), "r" (__r1), "m" (*(__regp)) : "g1"); \
} while(0)

#define FFB_WRITE64P(__regp, __srcp) \
do {	__asm__ __volatile__ ("ldx\t%0, %%g2;" \
			      "stx\t%%g2, %1" \
	 : : "m" (*(__srcp)), "m" (*(__regp)) \
         : "g2"); \
} while(0)			      

#define FFB_WRITE64(__regp, __lo32, __hi32) \
	FFB_WRITE64_COMMON(__regp, __lo32, __hi32, g2, g3)
#define FFB_WRITE64_2(__regp, __lo32, __hi32) \
	FFB_WRITE64_COMMON(__regp, __lo32, __hi32, g4, g5)
#define FFB_WRITE64_3(__regp, __lo32, __hi32) \
	FFB_WRITE64_COMMON(__regp, __lo32, __hi32, o4, o5)

#else /* Do not use 64-bit writes. */

#define FFB_WRITE64(__regp, __lo32, __hi32) \
do {	volatile unsigned int *__p = (__regp); \
	*__p = (__lo32); \
	*(__p + 1) = (__hi32); \
} while(0)

#define FFB_WRITE64P(__regp, __srcp) \
do {	volatile unsigned int *__p = (__regp); \
	unsigned int *__q = (__srcp); \
	*__p = *__q; \
	*(__p + 1) = *(__q + 1); \
} while(0)

#define FFB_WRITE64_2(__regp, __lo32, __hi32) \
	FFB_WRITE64(__regp, __lo32, __hi32)
#define FFB_WRITE64_3(__regp, __lo32, __hi32) \
	FFB_WRITE64(__regp, __lo32, __hi32)
#endif

#endif /* FFBREGS_H */
