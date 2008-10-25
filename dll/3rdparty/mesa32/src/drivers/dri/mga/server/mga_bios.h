/* $XConsortium: mga_bios.h /main/2 1996/10/28 04:48:23 kaleb $ */
#ifndef MGA_BIOS_H
#define MGA_BIOS_H

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_bios.h,v 1.3 1998/07/25 16:55:51 dawes Exp $ */

/*
 * MGABiosInfo - This struct describes the video BIOS info block.
 *
 * DESCRIPTION
 *   Do not mess with this, unless you know what you are doing.
 *   The data lengths and types are critical.
 *
 * HISTORY
 *   October 7, 1996 - [aem] Andrew E. Mileski
 *   This struct was shamelessly stolen from the MGA DDK.
 *   It has been reformatted, and the data types changed.
 */
typedef struct {
	/* Length of this structure in bytes */
	__u16 StructLen;

	/*
	 * Unique number identifying the product type
	 * 0 : MGA-S1P20 (2MB base with 175MHz Ramdac)
	 * 1 : MGA-S1P21 (2MB base with 220MHz Ramdac)
	 * 2 : Reserved
	 * 3 : Reserved
	 * 4 : MGA-S1P40 (4MB base with 175MHz Ramdac)
	 * 5 : MGA-S1P41 (4MB base with 220MHz Ramdac)
	 */
	__u16 ProductID;

	/* Serial number of the board */
	__u8 SerNo[ 10 ];

	/*
	 * Manufacturing date of the board (at product test)
	 * Format: yyyy yyym mmmd dddd
	 */
	__u16 ManufDate;

	/* Identification of manufacturing site */
	__u16 ManufId;

	/*
	 * Number and revision level of the PCB
	 * Format: nnnn nnnn nnnr rrrr
	 *         n = PCB number ex:576 (from 0->2047)
	 *         r = PCB revision      (from 0->31)
	 */
	__u16 PCBInfo;

	/* Identification of any PMBs */
	__u16 PMBInfo;

	/*
	 * Bit  0-7  : Ramdac speed (0=175MHz, 1=220MHz)
	 * Bit  8-15 : Ramdac type  (0=TVP3026, 1=TVP3027)
	 */
	__u16 RamdacType;

	/* Maximum PCLK of the ramdac */
	__u16 PclkMax;

	/* Maximum LDCLK supported by the WRAM memory */
	__u16 LclkMax;

	/* Maximum MCLK of base board */
	__u16 ClkBase;

	/* Maximum MCLK of 4Mb board */
	__u16 Clk4MB;

	/* Maximum MCLK of 8Mb board */
	__u16 Clk8MB;

	/* Maximum MCLK of board with multimedia module */
	__u16 ClkMod;

	/* Diagnostic test pass frequency */
	__u16 TestClk;

	/* Default VGA mode1 pixel frequency */
	__u16 VGAFreq1;

	/* Default VGA mode2 pixel frequency */
	__u16 VGAFreq2;

	/* Date of last BIOS programming/update */
	__u16 ProgramDate;

	/* Number of times BIOS has been programmed */
	__u16 ProgramCnt;

	/* Support for up to 32 hardware/software options */
	__u32 Options;

	/* Support for up to 32 hardware/software features */
	__u32 FeatFlag;

	/* Definition of VGA mode MCLK */
	__u16 VGAClk;

	/* Indicate the revision level of this header struct */
	__u16 StructRev;

	__u16 Reserved[ 3 ];
} MGABiosInfo;

/* from the PINS structure, refer pins info from MGA */
typedef struct tagParamMGA {
	__u16 	PinID;		/* 0 */
	__u8	StructLen;	/* 2 */
	__u8	Rsvd1;		/* 3 */
	__u16	StructRev;	/* 4 */
	__u16	ProgramDate;	/* 6 */
	__u16	ProgramCnt;	/* 8 */
	__u16	ProductID;	/* 10 */
	__u8	SerNo[16];	/* 12 */
	__u8	PLInfo[6];	/* 28 */
	__u16	PCBInfo;	/* 34 */
	__u32	FeatFlag;	/* 36 */
	__u8	RamdacType;	/* 40 */
	__u8	RamdacSpeed;	/* 41 */
	__u8	PclkMax;	/* 42 */
	__u8	ClkGE;		/* 43 */
	__u8   ClkMem;		/* 44 */
	__u8	Clk4MB;		/* 45 */
	__u8	Clk8MB;		/* 46 */
	__u8	ClkMod;		/* 47 */
	__u8	TestClk;	/* 48 */
	__u8	VGAFreq1;	/* 49 */
	__u8	VGAFreq2;	/* 50 */
	__u8	MCTLWTST;	/* 51 */
	__u8	VidCtrl;	/* 52 */
	__u8	Clk12MB;	/* 53 */
	__u8	Clk16MB;	/* 54 */
	__u8	Reserved[8];	/* 55-62 */
	__u8	PinCheck;	/* 63 */
}	MGABios2Info;

#endif
